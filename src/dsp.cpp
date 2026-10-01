#include "io_guitar/dsp.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace iog {

StringAnalyzer::StringAnalyzer(DspConfig c) : c_(c) {}

void StringAnalyzer::reset() {
    prevRms_ = 0.f;
    histWrite_ = 0;
    histCount_ = 0;
    std::memset(pitchHist_, 0, sizeof(pitchHist_));
    std::memset(confHist_, 0, sizeof(confHist_));
}

float StringAnalyzer::rms(const float* x, size_t n) const {
    if (!x || n == 0) return 0.f;
    float s = 0.f;
    for (size_t i = 0; i < n; ++i) s += x[i] * x[i];
    return std::sqrt(s / static_cast<float>(n));
}

float StringAnalyzer::zcr(const float* x, size_t n) const {
    if (!x || n < 2) return 0.f;
    size_t z = 0;
    for (size_t i = 1; i < n; ++i)
        if ((x[i-1] >= 0.f) != (x[i] >= 0.f)) ++z;
    return static_cast<float>(z) / static_cast<float>(n - 1);
}

float StringAnalyzer::pitch(const float* x, size_t n, float& confidence) const {
    confidence = 0.f;
    if (!x || n < 32) return 0.f;
    // Simple autocorrelation pitch estimate (placeholder for production YIN/MPM)
    const float sr = c_.sampleRateHz;
    int minLag = static_cast<int>(sr / c_.maxFrequencyHz);
    int maxLag = static_cast<int>(sr / c_.minFrequencyHz);
    if (maxLag >= static_cast<int>(n)) maxLag = static_cast<int>(n) - 1;
    if (minLag < 1) minLag = 1;
    float best = 0.f;
    int bestLag = minLag;
    for (int lag = minLag; lag <= maxLag; ++lag) {
        float sum = 0.f;
        for (size_t i = 0; i + lag < n; ++i) sum += x[i] * x[i + lag];
        if (sum > best) { best = sum; bestLag = lag; }
    }
    float energy = 0.f;
    for (size_t i = 0; i < n; ++i) energy += x[i] * x[i];
    confidence = (energy > 1e-9f) ? std::clamp(best / energy, 0.f, 1.f) : 0.f;
    if (confidence < c_.minConfidence) return 0.f;
    return sr / static_cast<float>(bestLag);
}

DetectedNote StringAnalyzer::analyze(const float* samples, size_t n,
                                     uint8_t stringIndex, uint64_t timestampUs) {
    DetectedNote note;
    note.stringIndex = stringIndex;
    note.onsetUs = timestampUs;
    if (!samples || n == 0) return note;

    const float r = rms(samples, n);
    note.amplitude01 = std::clamp(r * 8.f, 0.f, 1.f);
    note.attackDetected = (r > c_.noiseFloor * c_.onsetRatio && r > prevRms_ * 1.3f);
    note.sustained = (r > c_.noiseFloor);

    float conf = 0.f;
    note.frequencyHz = pitch(samples, n, conf);
    note.confidence01 = conf;
    if (note.frequencyHz > 0.f)
        note.midiNote = static_cast<int>(std::round(69.f + 12.f * std::log2(note.frequencyHz / 440.f)));

    prevRms_ = r;
    return note;
}

TechniqueFeatures StringAnalyzer::features(const float*, size_t, const DetectedNote& note) const {
    TechniqueFeatures f;
    f.attack01 = note.attackDetected ? 0.8f : 0.2f;
    f.sustain01 = note.sustained ? 0.7f : 0.1f;
    f.hasPitchHistory = histCount_ > 4;
    return f;
}

TechniqueResult StringAnalyzer::classify(const DetectedNote& note,
                                         const TechniqueFeatures& feat,
                                         Technique expected) const {
    TechniqueResult r;
    r.expected = expected;
    r.detected = Technique::Unknown;
    r.confidence01 = 0.f;
    r.evaluated = false;
    r.success = false;

    if (expected == Technique::Normal || expected == Technique::Chord) {
        r.detected = expected;
        r.confidence01 = note.confidence01;
        r.evaluated = true;
        r.success = note.confidence01 >= c_.minConfidence;
        return r;
    }
    if (expected == Technique::PalmMute) {
        r.detected = (feat.sustain01 < 0.35f) ? Technique::PalmMute : Technique::Normal;
        r.confidence01 = 0.6f;
        r.evaluated = true;
        r.success = (r.detected == Technique::PalmMute);
        return r;
    }
    // Advanced techniques require pitch history — stay honest
    if (!feat.hasPitchHistory) {
        r.evaluated = false;
        return r;
    }
    r.detected = expected;
    r.confidence01 = 0.5f;
    r.evaluated = true;
    r.success = false; // conservative until full trajectory analysis is active
    return r;
}

void StringAnalyzer::pushPitchHistory(float frequencyHz, float confidence01) {
    pitchHist_[histWrite_] = frequencyHz;
    confHist_[histWrite_] = confidence01;
    histWrite_ = (histWrite_ + 1) % kHistoryLen;
    if (histCount_ < kHistoryLen) ++histCount_;
}

float TargetGuidedDetector::cents(float detectedHz, float targetHz) {
    if (detectedHz <= 0.f || targetHz <= 0.f) return 9999.f;
    return 1200.f * std::log2(detectedHz / targetHz);
}

float TargetGuidedDetector::pitchScore(float absCents, const TargetGuidanceConfig& c) {
    if (absCents <= c.pitchFullScoreCents) return 1.f;
    if (absCents >= c.pitchZeroScoreCents) return 0.f;
    return 1.f - (absCents - c.pitchFullScoreCents) / (c.pitchZeroScoreCents - c.pitchFullScoreCents);
}

const NoteTarget* TargetGuidedDetector::bestTarget(const MusicalEvent* event,
                                                   const DetectedNote& note) const {
    if (!event || event->noteCount == 0) return nullptr;
    const NoteTarget* best = nullptr;
    float bestScore = -1.f;
    for (uint8_t i = 0; i < event->noteCount; ++i) {
        const auto& t = event->notes[i];
        float stringScore = (t.stringIndex == note.stringIndex) ? 1.f : 0.f;
        float pc = std::fabs(cents(note.frequencyHz, t.frequencyHz));
        float pScore = pitchScore(pc, c_);
        float score = c_.stringWeight * stringScore + c_.pitchWeight * pScore;
        if (score > bestScore) { bestScore = score; best = &t; }
    }
    return best;
}

void TargetGuidedDetector::apply(const MusicalEvent* event, std::array<DetectedNote, 6>& notes) const {
    for (auto& n : notes) {
        if (n.frequencyHz <= 0.f) continue;
        const NoteTarget* t = bestTarget(event, n);
        if (!t) continue;
        n.pitchErrorCents = cents(n.frequencyHz, t->frequencyHz);
        n.pitchScore = pitchScore(std::fabs(n.pitchErrorCents), c_);
        n.targetStringScore = (t->stringIndex == n.stringIndex) ? 1.f : 0.f;
        n.onsetAgreement = n.attackDetected ? 1.f : 0.3f;
        n.targetAffinity = c_.stringWeight * n.targetStringScore
                         + c_.pitchWeight * n.pitchScore
                         + c_.onsetWeight * n.onsetAgreement;
        if (n.targetAffinity < c_.minimumKeepScore && !n.attackDetected)
            n.confidence01 = std::min(n.confidence01, c_.resonanceConfidenceCeiling);
    }
}

SixStringAnalyzer::SixStringAnalyzer(DspConfig c) {
    for (auto& a : a_) a = StringAnalyzer(c);
}

std::array<DetectedNote, 6> SixStringAnalyzer::analyze(
    const std::array<const float*, 6>& channels, size_t n, uint64_t timestampUs) {
    std::array<DetectedNote, 6> out{};
    for (size_t i = 0; i < 6; ++i)
        out[i] = a_[i].analyze(channels[i], n, static_cast<uint8_t>(i), timestampUs);
    return out;
}

} // namespace iog
