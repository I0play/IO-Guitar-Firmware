#pragma once

#include "types.h"
#include <optional>

namespace iog {

struct Lesson {
    std::string id;
    std::string title;
    uint16_t    bpm             = 60;
    uint8_t     beatsPerMeasure = 4;
    std::vector<MusicalEvent> events;
};

class SkillTracker {
public:
    explicit SkillTracker(TeachingConfig cfg = {}) : cfg_(cfg) {}

    void update(SkillProfile& skills,
                const MusicalEvent& event,
                const PerformanceInput& input,
                const TimingResult& timing,
                const TechniqueResult& technique,
                bool stringMatch,
                bool pitchMatch,
                bool chordOk);

private:
    float ema(float prev, float sample) const {
        return prev + cfg_.skillEmaAlpha * (sample - prev);
    }
    TeachingConfig cfg_;
};

class AdaptiveEngine {
public:
    explicit AdaptiveEngine(TeachingConfig cfg = {}) : cfg_(cfg) {}

    GuidanceLevel guidance(const SkillProfile& skills, GuidanceLevel current) const;
    uint16_t      tempo(const SkillProfile& skills, uint16_t baseBpm) const;

private:
    TeachingConfig cfg_;
};

/**
 * LessonEngine – core teaching state machine.
 *
 * Changes vs v4.0:
 * - Timeout-based miss detection in update()
 * - String + pitch matching (not frequency-only)
 * - Technique results never fabricated
 * - Fixed-size feedback text (no std::string on hot path)
 * - Explicit Waiting state while listening
 */
class LessonEngine {
public:
    explicit LessonEngine(TeachingConfig cfg = {});

    void load(const Lesson& lesson);
    void start(uint64_t nowUs);
    void update(uint64_t nowUs);
    void submit(const PerformanceInput& input);
    void pause(uint64_t nowUs);
    void pause() { pause(lastNowUs_); }
    void resume(uint64_t nowUs);
    void stop();
    void setGuidance(GuidanceLevel g);

    const TeachingSnapshot& snapshot() const { return snap_; }
    const Lesson&           lesson()   const { return lesson_; }
    const TeachingConfig&   config()   const { return cfg_; }

private:
    const MusicalEvent* currentEvent() const;
    const MusicalEvent* nextEvent()    const;

    TimingResult gradeTiming(const MusicalEvent& event,
                             const PerformanceInput& input) const;
    TechniqueResult gradeTechnique(const NoteTarget& target,
                                   const DetectedNote* detected) const;
    uint64_t scaledEventStartUs(const MusicalEvent& event) const;
    uint64_t scaledEventDurationUs(const MusicalEvent& event) const;
    uint64_t eventExpectedUs(const MusicalEvent& event) const;
    void recomputeAdaptiveTempo();

    /** Returns true if the attempt was accepted (advance). */
    bool grade(const MusicalEvent& event, const PerformanceInput& input);

    void emitFeedback(FeedbackType type, const char* text, uint64_t nowUs);
    void refreshSnapshot(uint64_t nowUs);
    void checkMissTimeout(uint64_t nowUs);

    TeachingConfig  cfg_;
    Lesson          lesson_;
    LessonState     state_   = LessonState::Idle;
    GuidanceLevel   guidance_= GuidanceLevel::FullGuide;
    size_t          idx_     = 0;
    uint64_t        startUs_ = 0; // wall-clock time corresponding to musical t=0
    uint64_t        feedbackUntilUs_ = 0;
    uint64_t        pausedElapsedUs_ = 0;
    uint64_t        lastNowUs_ = 0;
    uint16_t        effectiveBpm_ = 60;
    SkillProfile    skills_{};
    TeachingSnapshot snap_{};
    SkillTracker    tracker_;
    AdaptiveEngine  adaptive_;
    std::optional<TimingResult>    lastTiming_;
    std::optional<TechniqueResult> lastTechnique_;
};

// Built-in curriculum
Lesson makeBeginnerLesson();
Lesson makeTechniqueLesson();
Lesson makeChordLesson();

}  // namespace iog
