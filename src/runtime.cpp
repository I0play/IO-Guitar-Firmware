#include "io_guitar/runtime.h"
#include <algorithm>

namespace iog {

FirmwareRuntime::FirmwareRuntime(SixChannelAudioInput& audio,
                                 FretLedOutput& leds,
                                 HornDisplayOutput& display,
                                 EncoderInput& encoder,
                                 AudioFeedbackOutput& feedback,
                                 RuntimeConfig cfg)
    : audio_(audio), leds_(leds), display_(display), encoder_(encoder),
      feedback_(feedback), cfg_(cfg),
      dsp_(cfg.dsp), targetGuide_(cfg.targetGuidance), engine_(cfg.teaching) {}

void FirmwareRuntime::loadLesson(const Lesson& lesson) {
    engine_.load(lesson);
}

bool FirmwareRuntime::loadImportedText(const char* name, const std::string& data, ImportFormat format) {
    auto r = LessonImporter::importText(data, format, name ? name : "import");
    if (!r.ok) return false;
    engine_.load(r.lesson);
    return true;
}

bool FirmwareRuntime::loadImportedBinary(const char* name, const std::vector<uint8_t>& data, ImportFormat format) {
    auto r = LessonImporter::importBinary(data, format, name ? name : "import");
    if (!r.ok) return false;
    engine_.load(r.lesson);
    return true;
}

void FirmwareRuntime::loadGeneratedChord(int rootMidi, ChordQuality quality, uint8_t startFret, uint8_t maxFret) {
    auto ge = ChordScaleGenerator::chord(rootMidi, quality, {}, startFret, maxFret);
    engine_.load(ge.lesson);
}

void FirmwareRuntime::loadGeneratedScale(int rootMidi, ScaleType scale, uint8_t startFret, uint8_t maxFret) {
    auto ge = ChordScaleGenerator::scale(rootMidi, scale, {}, startFret, maxFret);
    engine_.load(ge.lesson);
}

void FirmwareRuntime::start(uint64_t nowUs) { engine_.start(nowUs); }
void FirmwareRuntime::stop() { engine_.stop(); }

const MusicalEvent* FirmwareRuntime::currentEvent() const {
    const auto& lesson = engine_.lesson();
    const auto& snap = engine_.snapshot();
    for (const auto& e : lesson.events)
        if (e.id == snap.currentEventId) return &e;
    return nullptr;
}

void FirmwareRuntime::tick(uint64_t nowUs) {
    handleEncoder(nowUs);

    std::array<const float*, kStrings> channels{};
    size_t sampleCount = 0;
    uint64_t ts = nowUs;
    if (audio_.capture(channels, sampleCount, ts)) {
        auto notes = dsp_.analyze(channels, sampleCount, ts);
        const MusicalEvent* ev = currentEvent();
        targetGuide_.apply(ev, notes);
        classifyTechniques(notes, ev);
        PerformanceInput input = buildPerformance(notes, ts, ev);
        engine_.submit(input);
    }

    engine_.update(nowUs);
    publish(nowUs);
    lastTickUs_ = nowUs;
}

PerformanceInput FirmwareRuntime::buildPerformance(const std::array<DetectedNote, 6>& notes,
                                                   uint64_t timestampUs,
                                                   const MusicalEvent*) {
    PerformanceInput in;
    in.timestampUs = timestampUs;
    for (const auto& n : notes) {
        if (n.frequencyHz > 0.f && n.confidence01 >= cfg_.dsp.minConfidence) {
            if (isNewOnset(n))
                in.notes[in.noteCount++] = n;
        }
    }
    return in;
}

void FirmwareRuntime::classifyTechniques(std::array<DetectedNote, 6>& notes,
                                         const MusicalEvent* event) {
    if (!event) return;
    for (auto& n : notes) {
        Technique expected = Technique::Normal;
        for (uint8_t i = 0; i < event->noteCount; ++i)
            if (event->notes[i].stringIndex == n.stringIndex)
                expected = event->notes[i].technique;
        auto feat = dsp_.string(n.stringIndex).features(nullptr, 0, n);
        auto tr = dsp_.string(n.stringIndex).classify(n, feat, expected);
        n.techniqueDetected = tr.detected;
        n.techniqueConfidence01 = tr.confidence01;
    }
}

bool FirmwareRuntime::isNewOnset(const DetectedNote& note) const {
    if (!note.attackDetected) return false;
    return note.onsetUs > lastOnsetUs_[note.stringIndex] + 30000ULL;
}

void FirmwareRuntime::handleEncoder(uint64_t nowUs) {
    if (!cfg_.enableEncoder) return;
    switch (encoder_.poll()) {
        case EncoderAction::Press:
            if (engine_.snapshot().state == LessonState::Idle ||
                engine_.snapshot().state == LessonState::Completed)
                engine_.start(nowUs);
            else if (engine_.snapshot().state == LessonState::Paused)
                engine_.resume(nowUs);
            else
                engine_.pause(nowUs);
            break;
        case EncoderAction::Clockwise:
            // tempo up could be wired here
            break;
        case EncoderAction::CounterClockwise:
            break;
        default: break;
    }
}

void FirmwareRuntime::publish(uint64_t nowUs) {
    const uint64_t minInterval = 1000000ULL / std::max(1u, cfg_.maxFramesPerSecond);
    if (nowUs - lastPublishedUs_ < minInterval) return;

    const auto& snap = engine_.snapshot();
    publishFeedback(lastSnapshot_, snap);

    LedFrame leds = Renderer::leds(engine_.lesson(), snap, nowUs);
    UiFrame ui = Renderer::tft(engine_.lesson(), snap, nowUs);
    leds_.write(leds);
    display_.write(ui);

    lastSnapshot_ = snap;
    lastPublishedUs_ = nowUs;
}

void FirmwareRuntime::publishFeedback(const TeachingSnapshot& before,
                                      const TeachingSnapshot& after) {
    if (!cfg_.enableAudioFeedback) return;
    if (before.feedback != FeedbackType::Success && after.feedback == FeedbackType::Success)
        feedback_.success();
    else if (after.feedback == FeedbackType::WrongPitch ||
             after.feedback == FeedbackType::WrongString ||
             after.feedback == FeedbackType::Missed ||
             after.feedback == FeedbackType::ExtraNote)
        feedback_.error();
}

} // namespace iog
