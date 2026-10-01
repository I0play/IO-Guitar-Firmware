#include "io_guitar/ui.h"

#include <algorithm>
#include <cstdio>

namespace iog {

namespace {

LedPixel targetColor()     { return {180, 255, 180, 230}; }  // soft green
LedPixel nextColor()       { return { 70, 130, 255, 120}; }  // blue preview
LedPixel timingWarnColor() { return {255, 180,  40, 255}; }  // amber blink
LedPixel fingerColor()     { return {255, 220, 100, 200}; }  // warm yellow

const char* techniqueNameForUi(Technique t) {
    switch (t) {
        case Technique::HammerOn: return "Hammer-on";
        case Technique::PullOff: return "Pull-off";
        case Technique::Slide: return "Slide";
        case Technique::Bend: return "Bend";
        case Technique::Vibrato: return "Vibrato";
        case Technique::PalmMute: return "Palm mute";
        case Technique::Chord: return "Chord";
        case Technique::Normal: return "Normal";
        default: return "Technique";
    }
}

}  // namespace

LedFrame Renderer::leds(const Lesson& lesson,
                        const TeachingSnapshot& snap,
                        uint64_t nowUs) {
    LedFrame frame{};

    auto paintEvent = [&](uint32_t eventId, LedPixel color, bool useFinger) {
        for (const auto& e : lesson.events) {
            if (e.id != eventId)
                continue;
            for (uint8_t i = 0; i < e.noteCount; ++i) {
                const auto& n = e.notes[i];
                const int idx = ledIndex(n.stringIndex, n.fret);
                if (idx < 0 || idx >= kLedCount)
                    continue;
                frame.pixels[idx] = color;

                // Optional finger hint: dim secondary pixel offset toward nut
                // (fret-1) when showFinger and recommendedFinger > 0
                if (useFinger && snap.showFinger && n.recommendedFinger > 0 &&
                    n.fret > 0) {
                    const int hint = ledIndex(n.stringIndex, n.fret - 1);
                    if (hint >= 0 && hint < kLedCount &&
                        frame.pixels[hint].brightness < 50) {
                        frame.pixels[hint] = fingerColor();
                    }
                }
            }
        }
    };

    // Current target
    if (snap.showTarget && snap.currentEventId != 0)
        paintEvent(snap.currentEventId, targetColor(), true);

    // Next-note preview only at FullGuide
    if (snap.guidance == GuidanceLevel::FullGuide && snap.nextEventId != 0)
        paintEvent(snap.nextEventId, nextColor(), false);

    // Timing warning pulse on current target when early/late
    if (snap.timing.detected && (snap.timing.early || snap.timing.late)) {
        const bool pulse = ((nowUs / 100000) % 2) == 0;
        if (pulse) {
            paintEvent(snap.currentEventId, timingWarnColor(), false);
        }
    }

    // Technique accent for advanced techniques
    if (snap.showTechnique && snap.technique.expected != Technique::Normal &&
        snap.technique.expected != Technique::Unknown) {
        for (const auto& e : lesson.events) {
            if (e.id != snap.currentEventId) continue;
            for (uint8_t i = 0; i < e.noteCount; ++i) {
                const int idx = ledIndex(e.notes[i].stringIndex, e.notes[i].fret);
                if (idx >= 0 && idx < kLedCount) {
                    auto& p = frame.pixels[idx];
                    // Blend toward technique colour
                    p.r = static_cast<uint8_t>((p.r + 200) / 2);
                    p.b = static_cast<uint8_t>((p.b + 255) / 2);
                }
            }
        }
    }

    return frame;
}

UiFrame Renderer::tft(const Lesson& lesson,
                      const TeachingSnapshot& snap,
                      uint64_t nowUs) {
    UiFrame ui{};

    std::snprintf(ui.title, sizeof(ui.title), "%s", lesson.title.c_str());
    ui.progressPercent = static_cast<uint8_t>(
        std::clamp(snap.progress01, 0.f, 1.f) * 100.f);
    ui.metronomePulse = ((nowUs / 125000) % 4) == 0;

    std::snprintf(ui.line1, sizeof(ui.line1),
                  "M%u B%u  %u BPM  %u%%",
                  snap.currentMeasure, snap.currentBeat,
                  snap.effectiveBpm, ui.progressPercent);
    std::snprintf(ui.line2, sizeof(ui.line2), "%s", snap.feedbackText);

    if (snap.timing.detected) {
        std::snprintf(ui.line3, sizeof(ui.line3),
                      "Timing %+d ms", snap.timing.errorMs);
    } else {
        std::snprintf(ui.line3, sizeof(ui.line3), "Listen...");
    }

    std::snprintf(ui.line4, sizeof(ui.line4),
                  "Timing %.0f%%  Pitch %.0f%%",
                  snap.skills.timing * 100.f, snap.skills.pitchAccuracy * 100.f);

    if (snap.tabCount > 0) {
        size_t used = 0;
        used += std::snprintf(ui.tab + used, sizeof(ui.tab) - used, "TAB ");
        for (uint8_t i = 0; i < snap.tabCount && used < sizeof(ui.tab); ++i) {
            const auto& n = snap.tab[i];
            const int written = std::snprintf(ui.tab + used, sizeof(ui.tab) - used,
                                              "S%u:%u", n.stringIndex + 1, n.fret);
            if (written <= 0) break;
            used += static_cast<size_t>(written);
            if (i + 1 < snap.tabCount && used + 1 < sizeof(ui.tab))
                ui.tab[used++] = ' ';
        }
    }
    std::snprintf(ui.technique, sizeof(ui.technique), "%s",
                  techniqueNameForUi(snap.technique.expected));

    const uint64_t beatUs = snap.effectiveBpm > 0
        ? 60000000ULL / snap.effectiveBpm : 1000000ULL;
    ui.metronomePulse = snap.state == LessonState::Waiting &&
                        beatUs > 0 && (snap.eventElapsedUs / (beatUs / 2 + 1)) % 2 == 0;
    return ui;
}

}  // namespace iog
