#pragma once

#include "types.h"
#include <array>
#include <cstddef>
#include <optional>

namespace iog {

struct DspConfig {
    float sampleRateHz   = 16000.f;
    float noiseFloor     = 0.004f;
    float onsetRatio     = 2.2f;
    float minFrequencyHz = 70.f;
    float maxFrequencyHz = 1100.f;
    // Pitch confidence mapping (autocorrelation quality → 0..1)
    float confFloor      = 0.25f;
    float confRange      = 0.65f;
    float minConfidence  = 0.15f;   // below this → midiNote = -1
    float techniqueMinConf = 0.45f;
};

/**
 * Per-string analyzer.
 *
 * Pitch: normalized autocorrelation with constrained lag range.
 * Technique: PalmMute has a real rule; advanced techniques require a pitch
 * history buffer (API already present). Until history is fed, those techniques
 * return evaluated=false / Technique::Unknown instead of fake success.
 */
class StringAnalyzer {
public:
    explicit StringAnalyzer(DspConfig c = {});

    void reset();

    DetectedNote analyze(const float* samples, size_t n,
                         uint8_t stringIndex, uint64_t timestampUs);

    TechniqueFeatures features(const float* samples, size_t n,
                               const DetectedNote& note) const;

    /**
     * Classify technique. Never fabricates success for Bend/Slide/Vibrato/
     * HammerOn/PullOff when pitch history is absent.
     */
    TechniqueResult classify(const DetectedNote& note,
                             const TechniqueFeatures& feat,
                             Technique expected) const;

    /** Optional: push a pitch sample into the history ring (call after analyze). */
    void pushPitchHistory(float frequencyHz, float confidence01);

    static constexpr size_t kHistoryLen = 64;

private:
    float rms(const float* x, size_t n) const;
    float zcr(const float* x, size_t n) const;
    float pitch(const float* x, size_t n, float& confidence) const;

    DspConfig c_;
    float     prevRms_ = 0.f;

    // Chronological pitch/confidence history used by technique analysis.
    float    pitchHist_[kHistoryLen] = {};
    float    confHist_[kHistoryLen]  = {};
    size_t   histWrite_ = 0;
    size_t   histCount_ = 0;
};


/**
 * TargetGuidance converts the active lesson event into a DSP prior. The
 * lesson target does not override measured audio; it ranks and gates evidence
 * so sympathetic resonance is less likely to become a false played note.
 *
 * A target contributes three independent pieces of evidence:
 *   - expected physical string
 *   - expected fundamental frequency
 *   - attack/onset agreement
 *
 * Wrong-string and wrong-pitch observations remain visible to the grader.
 */
struct TargetGuidanceConfig {
    float pitchFullScoreCents = 35.f;
    float pitchZeroScoreCents = 180.f;
    float stringWeight = 0.45f;
    float pitchWeight = 0.45f;
    float onsetWeight = 0.10f;
    float minimumKeepScore = 0.18f;
    float resonanceConfidenceCeiling = 0.42f;
};

class TargetGuidedDetector {
public:
    explicit TargetGuidedDetector(TargetGuidanceConfig c = {}) : c_(c) {}

    void apply(const MusicalEvent* event, std::array<DetectedNote, 6>& notes) const;

    static float cents(float detectedHz, float targetHz);
    static float pitchScore(float absCents, const TargetGuidanceConfig& c);

private:
    const NoteTarget* bestTarget(const MusicalEvent* event,
                                 const DetectedNote& note) const;
    TargetGuidanceConfig c_;
};

class SixStringAnalyzer {
public:
    explicit SixStringAnalyzer(DspConfig c = {});

    std::array<DetectedNote, 6> analyze(
        const std::array<const float*, 6>& channels,
        size_t n,
        uint64_t timestampUs);

    StringAnalyzer& string(size_t i) { return a_[i]; }
    const StringAnalyzer& string(size_t i) const { return a_[i]; }

private:
    std::array<StringAnalyzer, 6> a_;
};

}  // namespace iog
