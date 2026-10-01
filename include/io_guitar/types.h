#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace iog {

constexpr int kStrings            = 6;
constexpr int kFrets              = 22;
constexpr int kPositionsPerString = 23;
constexpr int kLedCount           = 138;

constexpr int ledIndex(int s, int f) {
    return s * kPositionsPerString + f;
}

enum class Technique : uint8_t {
    Normal, HammerOn, PullOff, Slide, Bend, Vibrato, PalmMute,
    StringMute, Harmonic, Chord, Rest, Unknown
};

enum class GuidanceLevel : uint8_t { FullGuide, Guided, Rhythm, Challenge, Performance };
enum class LessonState : uint8_t { Idle, Intro, CountIn, Waiting, Playing, Evaluating, Feedback, LoopingError, Paused, Completed };
enum class FeedbackType : uint8_t { None, Success, Encouragement, Early, Late, WrongPitch, WrongString, Missed, Muted, Technique, LowConfidence, PracticeLoop, ExtraNote };
enum class ErrorType : uint8_t { WrongNote, MissedNote, Early, Late, Muted, WrongString, TechniqueFailure, ExtraNote, ExtraStrum };
enum class PickDirection : uint8_t { None, Down, Up, Either };

struct TechniqueFeatures {
    float attack01 = 0.f;
    float sustain01 = 0.f;
    float pitchRiseSemitones = 0.f;
    float pitchFallSemitones = 0.f;
    float pitchDeviationSemitones = 0.f;
    float highFrequencyEnergy01 = 0.f;
    float decayRate01 = 0.f;
    float trajectorySlopeSemitones = 0.f;
    uint8_t oscillationCount = 0;
    bool hasPitchHistory = false;
};

struct NoteTarget {
    uint8_t stringIndex = 0;
    uint8_t fret = 0;
    uint8_t recommendedFinger = 0;
    float frequencyHz = 0.f;
    uint16_t durationMs = 250;
    uint16_t timingEarlyMs = 180;
    uint16_t timingLateMs = 180;
    float velocityMin = 0.05f;
    float velocityMax = 1.0f;
    bool mustBeClean = true;
    Technique technique = Technique::Normal;
};

struct MusicalEvent {
    uint32_t id = 0;
    uint64_t startUs = 0;
    uint32_t durationUs = 500000;
    uint16_t measure = 0;
    uint8_t beat = 0;
    uint8_t subdivision = 1;
    std::array<NoteTarget, 6> notes{};
    uint8_t noteCount = 0;
    bool chord = false;
    bool rest = false;
};

struct DetectedNote {
    uint8_t stringIndex = 0;
    float frequencyHz = 0.f;
    int midiNote = -1;
    float confidence01 = 0.f;
    uint64_t onsetUs = 0;
    float amplitude01 = 0.f;
    bool attackDetected = false;
    bool sustained = false;
    TechniqueFeatures techniqueFeatures{};
    Technique techniqueDetected = Technique::Unknown;
    float techniqueConfidence01 = 0.f;
    float targetStringScore = 0.f;
    float pitchErrorCents = 0.f;
    float pitchScore = 0.f;
    float onsetAgreement = 0.f;
    float targetAffinity = 0.f;
};

struct PerformanceInput {
    uint64_t timestampUs = 0;
    std::array<DetectedNote, 6> notes{};
    uint8_t noteCount = 0;
};

struct TimingResult {
    int32_t errorMs = 0;
    bool detected = false;
    bool early = false;
    bool late = false;
    bool onTime = false;
};

struct TechniqueResult {
    Technique expected = Technique::Normal;
    Technique detected = Technique::Unknown;
    float confidence01 = 0.f;
    float targetPitchSemitones = 0.f;
    float achievedPitchSemitones = 0.f;
    float attackStrength01 = 0.f;
    float sustain01 = 0.f;
    bool success = false;
    bool evaluated = false;
};

struct SkillProfile {
    float timing = 0.5f;
    float pitchAccuracy = 0.5f;
    float stringAccuracy = 0.5f;
    float chordAccuracy = 0.5f;
    float picking = 0.5f;
    float muting = 0.5f;
    float hammerOns = 0.5f;
    float pullOffs = 0.5f;
    float slides = 0.5f;
    float bends = 0.5f;
    float vibrato = 0.5f;
};

struct TabNoteView {
    uint8_t stringIndex = 0;
    uint8_t fret = 0;
    uint8_t finger = 0;
    Technique technique = Technique::Normal;
};

struct TeachingSnapshot {
    LessonState state = LessonState::Idle;
    GuidanceLevel guidance = GuidanceLevel::FullGuide;
    uint32_t currentEventId = 0;
    uint32_t nextEventId = 0;
    float progress01 = 0.f;
    uint16_t currentMeasure = 0;
    uint8_t currentBeat = 0;
    uint8_t currentSubdivision = 1;
    uint8_t tabCount = 0;
    std::array<TabNoteView, 6> tab{};
    uint32_t effectiveBpm = 60;
    uint64_t eventElapsedUs = 0;
    uint64_t eventDurationUs = 0;
    bool showTarget = true;
    bool showFinger = true;
    bool showTiming = true;
    bool showTechnique = true;
    FeedbackType feedback = FeedbackType::None;
    char feedbackText[64] = {};
    TimingResult timing{};
    TechniqueResult technique{};
    SkillProfile skills{};
};

struct LedPixel { uint8_t r=0,g=0,b=0,brightness=255; };
struct LedFrame { std::array<LedPixel, kLedCount> pixels{}; };

struct UiFrame {
    char title[48] = {};
    char line1[48] = {};
    char line2[64] = {};
    char line3[48] = {};
    char line4[48] = {};
    char tab[64] = {};
    char technique[32] = {};
    uint8_t progressPercent = 0;
    bool metronomePulse = false;
};

struct TeachingConfig {
    float skillEmaAlpha = 0.18f;
    float pitchToleranceRatio = 0.025f;
    float pitchToleranceCents = 40.f;
    float minimumConfidence = 0.15f;
    uint64_t feedbackHoldUs = 650000;
    uint64_t countInUs = 2000000;
    uint64_t missGraceUs = 150000;
    float guidancePromote = 0.88f;
    float guidanceDemote = 0.45f;
    float tempoPromote = 0.90f;
    float tempoDemote = 0.45f;
    uint16_t minTempoBpm = 40;
    uint16_t maxTempoBpm = 160;
};

} // namespace iog
