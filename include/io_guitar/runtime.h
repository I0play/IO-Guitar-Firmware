#pragma once

#include "dsp.h"
#include "teaching.h"
#include "ui.h"
#include "music.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace iog {

/**
 * Hardware-facing interfaces. These intentionally contain no ESP-IDF types so
 * the teaching/runtime layer can be host-tested and the board driver can be
 * replaced without touching lesson logic.
 */
class SixChannelAudioInput {
public:
    virtual ~SixChannelAudioInput() = default;
    virtual bool capture(std::array<const float*, kStrings>& channels,
                         size_t& sampleCount,
                         uint64_t& timestampUs) = 0;
};

class FretLedOutput {
public:
    virtual ~FretLedOutput() = default;
    virtual void write(const LedFrame& frame) = 0;
};

class HornDisplayOutput {
public:
    virtual ~HornDisplayOutput() = default;
    virtual void write(const UiFrame& frame) = 0;
};

enum class EncoderAction : uint8_t { None, Press, Clockwise, CounterClockwise };

class EncoderInput {
public:
    virtual ~EncoderInput() = default;
    virtual EncoderAction poll() = 0;
};

class AudioFeedbackOutput {
public:
    virtual ~AudioFeedbackOutput() = default;
    virtual void success() = 0;
    virtual void error() = 0;
    virtual void tick() = 0;
};

class LessonStorage {
public:
    virtual ~LessonStorage() = default;
    virtual bool loadLesson(const char* id, Lesson& lesson) = 0;
};

struct RuntimeConfig {
    DspConfig dsp{};
    TeachingConfig teaching{};
    TargetGuidanceConfig targetGuidance{};
    uint32_t maxFramesPerSecond = 100;
    bool enableAudioFeedback = true;
    bool enableEncoder = true;
};

/**
 * FirmwareRuntime is the production orchestration layer:
 *
 * audio -> six-string DSP -> performance frame -> LessonEngine
 *        -> teaching snapshot -> LEDs/TFT/audio feedback
 *
 * It also owns encoder transport controls so the physical guitar can operate
 * without a phone or computer.
 */
class FirmwareRuntime {
public:
    FirmwareRuntime(SixChannelAudioInput& audio,
                    FretLedOutput& leds,
                    HornDisplayOutput& display,
                    EncoderInput& encoder,
                    AudioFeedbackOutput& feedback,
                    RuntimeConfig cfg = {});

    void loadLesson(const Lesson& lesson);
    bool loadImportedText(const char* name, const std::string& data, ImportFormat format = ImportFormat::Auto);
    bool loadImportedBinary(const char* name, const std::vector<uint8_t>& data, ImportFormat format = ImportFormat::Auto);
    void loadGeneratedChord(int rootMidi, ChordQuality quality, uint8_t startFret = 0, uint8_t maxFret = 12);
    void loadGeneratedScale(int rootMidi, ScaleType scale, uint8_t startFret = 0, uint8_t maxFret = 12);
    void start(uint64_t nowUs);
    void stop();
    void tick(uint64_t nowUs);

    const TeachingSnapshot& snapshot() const { return engine_.snapshot(); }
    LessonEngine& engine() { return engine_; }
    const LessonEngine& engine() const { return engine_; }
    SixStringAnalyzer& dsp() { return dsp_; }

private:
    const MusicalEvent* currentEvent() const;
    PerformanceInput buildPerformance(const std::array<DetectedNote, 6>& notes,
                                      uint64_t timestampUs,
                                      const MusicalEvent* event);
    void classifyTechniques(std::array<DetectedNote, 6>& notes,
                            const MusicalEvent* event);
    bool isNewOnset(const DetectedNote& note) const;
    void handleEncoder(uint64_t nowUs);
    void publish(uint64_t nowUs);
    void publishFeedback(const TeachingSnapshot& before,
                         const TeachingSnapshot& after);

    SixChannelAudioInput& audio_;
    FretLedOutput& leds_;
    HornDisplayOutput& display_;
    EncoderInput& encoder_;
    AudioFeedbackOutput& feedback_;
    RuntimeConfig cfg_;
    SixStringAnalyzer dsp_;
    TargetGuidedDetector targetGuide_;
    LessonEngine engine_;
    uint64_t lastPublishedUs_ = 0;
    uint64_t lastTickUs_ = 0;
    std::array<uint64_t, kStrings> lastOnsetUs_{};
    TeachingSnapshot lastSnapshot_{};
};

} // namespace iog
