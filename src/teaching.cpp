#include "io_guitar/teaching.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace iog {

void SkillTracker::update(SkillProfile& skills,
                          const MusicalEvent& /*event*/,
                          const PerformanceInput& /*input*/,
                          const TimingResult& timing,
                          const TechniqueResult& technique,
                          bool stringMatch,
                          bool pitchMatch,
                          bool chordOk) {
    float tScore = timing.onTime ? 1.f : (timing.detected ? 0.4f : 0.f);
    skills.timing = ema(skills.timing, tScore);
    skills.pitchAccuracy = ema(skills.pitchAccuracy, pitchMatch ? 1.f : 0.2f);
    skills.stringAccuracy = ema(skills.stringAccuracy, stringMatch ? 1.f : 0.2f);
    skills.chordAccuracy = ema(skills.chordAccuracy, chordOk ? 1.f : 0.3f);
    if (technique.evaluated)
        skills.hammerOns = ema(skills.hammerOns, technique.success ? 1.f : 0.3f);
}

GuidanceLevel AdaptiveEngine::guidance(const SkillProfile& skills, GuidanceLevel current) const {
    float avg = (skills.timing + skills.pitchAccuracy + skills.stringAccuracy) / 3.f;
    if (avg >= cfg_.guidancePromote) {
        if (current == GuidanceLevel::FullGuide) return GuidanceLevel::Guided;
        if (current == GuidanceLevel::Guided) return GuidanceLevel::Rhythm;
        if (current == GuidanceLevel::Rhythm) return GuidanceLevel::Challenge;
    } else if (avg <= cfg_.guidanceDemote) {
        if (current == GuidanceLevel::Challenge) return GuidanceLevel::Rhythm;
        if (current == GuidanceLevel::Rhythm) return GuidanceLevel::Guided;
        if (current == GuidanceLevel::Guided) return GuidanceLevel::FullGuide;
    }
    return current;
}

uint16_t AdaptiveEngine::tempo(const SkillProfile& skills, uint16_t baseBpm) const {
    float avg = (skills.timing + skills.pitchAccuracy) / 2.f;
    float scale = 1.f;
    if (avg >= cfg_.tempoPromote) scale = 1.08f;
    else if (avg <= cfg_.tempoDemote) scale = 0.92f;
    uint16_t bpm = static_cast<uint16_t>(baseBpm * scale);
    return std::clamp(bpm, cfg_.minTempoBpm, cfg_.maxTempoBpm);
}

LessonEngine::LessonEngine(TeachingConfig cfg)
    : cfg_(cfg), tracker_(cfg), adaptive_(cfg) {}

void LessonEngine::load(const Lesson& lesson) {
    lesson_ = lesson;
    state_ = LessonState::Idle;
    idx_ = 0;
    effectiveBpm_ = lesson.bpm;
    snap_ = TeachingSnapshot{};
}

void LessonEngine::start(uint64_t nowUs) {
    if (lesson_.events.empty()) return;
    state_ = LessonState::CountIn;
    idx_ = 0;
    startUs_ = nowUs + cfg_.countInUs; // musical t=0 after count-in
    pausedElapsedUs_ = 0;
    lastNowUs_ = nowUs;
    recomputeAdaptiveTempo();
    refreshSnapshot(nowUs);
}

void LessonEngine::stop() {
    state_ = LessonState::Idle;
    refreshSnapshot(lastNowUs_);
}

void LessonEngine::pause(uint64_t nowUs) {
    if (state_ == LessonState::Playing || state_ == LessonState::Waiting ||
        state_ == LessonState::CountIn) {
        pausedElapsedUs_ = (nowUs > startUs_) ? (nowUs - startUs_) : 0;
        state_ = LessonState::Paused;
    }
    lastNowUs_ = nowUs;
    refreshSnapshot(nowUs);
}

void LessonEngine::resume(uint64_t nowUs) {
    if (state_ == LessonState::Paused) {
        startUs_ = nowUs - pausedElapsedUs_;
        state_ = LessonState::Waiting;
    }
    lastNowUs_ = nowUs;
    refreshSnapshot(nowUs);
}

void LessonEngine::setGuidance(GuidanceLevel g) { guidance_ = g; }

const MusicalEvent* LessonEngine::currentEvent() const {
    if (idx_ >= lesson_.events.size()) return nullptr;
    return &lesson_.events[idx_];
}

const MusicalEvent* LessonEngine::nextEvent() const {
    if (idx_ + 1 >= lesson_.events.size()) return nullptr;
    return &lesson_.events[idx_ + 1];
}

uint64_t LessonEngine::scaledEventStartUs(const MusicalEvent& event) const {
    if (lesson_.bpm == 0) return event.startUs;
    float scale = static_cast<float>(lesson_.bpm) / static_cast<float>(effectiveBpm_);
    return static_cast<uint64_t>(event.startUs * scale);
}

uint64_t LessonEngine::scaledEventDurationUs(const MusicalEvent& event) const {
    if (lesson_.bpm == 0) return event.durationUs;
    float scale = static_cast<float>(lesson_.bpm) / static_cast<float>(effectiveBpm_);
    return static_cast<uint64_t>(event.durationUs * scale);
}

uint64_t LessonEngine::eventExpectedUs(const MusicalEvent& event) const {
    return startUs_ + scaledEventStartUs(event);
}

void LessonEngine::recomputeAdaptiveTempo() {
    effectiveBpm_ = adaptive_.tempo(skills_, lesson_.bpm);
}

TimingResult LessonEngine::gradeTiming(const MusicalEvent& event,
                                       const PerformanceInput& input) const {
    TimingResult t;
    if (input.noteCount == 0) return t;
    const uint64_t expected = eventExpectedUs(event);
    const uint64_t detected = input.notes[0].onsetUs;
    int32_t err = static_cast<int32_t>(static_cast<int64_t>(detected) - static_cast<int64_t>(expected)) / 1000;
    t.errorMs = err;
    t.detected = true;
    const auto& n = event.notes[0];
    t.early = err < -static_cast<int32_t>(n.timingEarlyMs);
    t.late  = err >  static_cast<int32_t>(n.timingLateMs);
    t.onTime = !t.early && !t.late;
    return t;
}

TechniqueResult LessonEngine::gradeTechnique(const NoteTarget& target,
                                             const DetectedNote* detected) const {
    TechniqueResult r;
    r.expected = target.technique;
    if (!detected) { r.evaluated = false; return r; }
    r.detected = detected->techniqueDetected;
    r.confidence01 = detected->techniqueConfidence01;
    r.evaluated = (target.technique == Technique::Normal ||
                   target.technique == Technique::Chord ||
                   detected->techniqueConfidence01 > 0.f);
    r.success = r.evaluated && (r.detected == target.technique || target.technique == Technique::Normal);
    return r;
}

bool LessonEngine::grade(const MusicalEvent& event, const PerformanceInput& input) {
    bool stringMatch = false, pitchMatch = false, chordOk = true;
    TimingResult timing = gradeTiming(event, input);
    lastTiming_ = timing;

    if (event.chord) {
        // Require every target tone to be matched one-to-one
        std::array<bool, 6> used{};
        size_t matched = 0;
        for (uint8_t ti = 0; ti < event.noteCount; ++ti) {
            const auto& tgt = event.notes[ti];
            bool found = false;
            for (uint8_t di = 0; di < input.noteCount; ++di) {
                if (used[di]) continue;
                const auto& det = input.notes[di];
                if (det.stringIndex == tgt.stringIndex) {
                    float cents = 1200.f * std::log2(std::max(1.f, det.frequencyHz) / std::max(1.f, tgt.frequencyHz));
                    if (std::fabs(cents) <= cfg_.pitchToleranceCents) {
                        used[di] = true;
                        found = true;
                        ++matched;
                        break;
                    }
                }
            }
            if (!found) chordOk = false;
        }
        stringMatch = pitchMatch = (matched == event.noteCount);
        // Extra notes?
        if (input.noteCount > matched) chordOk = false;
    } else if (event.noteCount > 0 && input.noteCount > 0) {
        const auto& tgt = event.notes[0];
        const auto& det = input.notes[0];
        stringMatch = (det.stringIndex == tgt.stringIndex);
        float cents = 1200.f * std::log2(std::max(1.f, det.frequencyHz) / std::max(1.f, tgt.frequencyHz));
        pitchMatch = std::fabs(cents) <= cfg_.pitchToleranceCents;
        if (input.noteCount > 1) chordOk = false; // extra note
    }

    TechniqueResult tech{};
    if (event.noteCount > 0)
        tech = gradeTechnique(event.notes[0], input.noteCount ? &input.notes[0] : nullptr);
    lastTechnique_ = tech;

    tracker_.update(skills_, event, input, timing, tech, stringMatch, pitchMatch, chordOk);
    guidance_ = adaptive_.guidance(skills_, guidance_);
    recomputeAdaptiveTempo();

    bool ok = stringMatch && pitchMatch && timing.onTime && chordOk;
    if (ok) {
        emitFeedback(FeedbackType::Success, "Good!", lastNowUs_);
        ++idx_;
        if (idx_ >= lesson_.events.size())
            state_ = LessonState::Completed;
        else
            state_ = LessonState::Waiting;
    } else if (!stringMatch) {
        emitFeedback(FeedbackType::WrongString, "Wrong string", lastNowUs_);
    } else if (!pitchMatch) {
        emitFeedback(FeedbackType::WrongPitch, "Pitch off", lastNowUs_);
    } else if (timing.early) {
        emitFeedback(FeedbackType::Early, "Too early", lastNowUs_);
    } else if (timing.late) {
        emitFeedback(FeedbackType::Late, "Too late", lastNowUs_);
    } else if (!chordOk) {
        emitFeedback(FeedbackType::ExtraNote, "Extra note", lastNowUs_);
    }
    return ok;
}

void LessonEngine::submit(const PerformanceInput& input) {
    if (state_ != LessonState::Waiting && state_ != LessonState::Playing) return;
    const MusicalEvent* ev = currentEvent();
    if (!ev) return;
    grade(*ev, input);
    refreshSnapshot(lastNowUs_);
}

void LessonEngine::checkMissTimeout(uint64_t nowUs) {
    const MusicalEvent* ev = currentEvent();
    if (!ev) return;
    uint64_t deadline = eventExpectedUs(*ev) + scaledEventDurationUs(*ev) + cfg_.missGraceUs;
    if (nowUs > deadline) {
        emitFeedback(FeedbackType::Missed, "Missed", nowUs);
        ++idx_;
        if (idx_ >= lesson_.events.size())
            state_ = LessonState::Completed;
        else
            state_ = LessonState::Waiting;
    }
}

void LessonEngine::update(uint64_t nowUs) {
    lastNowUs_ = nowUs;
    if (state_ == LessonState::CountIn) {
        if (nowUs >= startUs_) state_ = LessonState::Waiting;
    } else if (state_ == LessonState::Waiting || state_ == LessonState::Playing) {
        checkMissTimeout(nowUs);
    }
    if (feedbackUntilUs_ && nowUs > feedbackUntilUs_)
        snap_.feedback = FeedbackType::None;
    refreshSnapshot(nowUs);
}

void LessonEngine::emitFeedback(FeedbackType type, const char* text, uint64_t nowUs) {
    snap_.feedback = type;
    std::snprintf(snap_.feedbackText, sizeof(snap_.feedbackText), "%s", text ? text : "");
    feedbackUntilUs_ = nowUs + cfg_.feedbackHoldUs;
}

void LessonEngine::refreshSnapshot(uint64_t nowUs) {
    snap_.state = state_;
    snap_.guidance = guidance_;
    snap_.effectiveBpm = effectiveBpm_;
    snap_.skills = skills_;
    if (lastTiming_) snap_.timing = *lastTiming_;
    if (lastTechnique_) snap_.technique = *lastTechnique_;

    const MusicalEvent* cur = currentEvent();
    const MusicalEvent* nxt = nextEvent();
    snap_.currentEventId = cur ? cur->id : 0;
    snap_.nextEventId = nxt ? nxt->id : 0;
    snap_.progress01 = lesson_.events.empty() ? 0.f
        : static_cast<float>(idx_) / static_cast<float>(lesson_.events.size());

    if (cur) {
        snap_.currentMeasure = cur->measure;
        snap_.currentBeat = cur->beat;
        snap_.eventElapsedUs = (nowUs > eventExpectedUs(*cur)) ? (nowUs - eventExpectedUs(*cur)) : 0;
        snap_.eventDurationUs = scaledEventDurationUs(*cur);
        snap_.tabCount = cur->noteCount;
        for (uint8_t i = 0; i < cur->noteCount; ++i) {
            snap_.tab[i] = {cur->notes[i].stringIndex, cur->notes[i].fret,
                            cur->notes[i].recommendedFinger, cur->notes[i].technique};
        }
    }
    snap_.showTarget = (guidance_ == GuidanceLevel::FullGuide || guidance_ == GuidanceLevel::Guided);
    snap_.showFinger = (guidance_ == GuidanceLevel::FullGuide);
    snap_.showTiming = true;
    snap_.showTechnique = true;
}

} // namespace iog
