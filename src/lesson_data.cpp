#include "io_guitar/teaching.h"

#include <cmath>

namespace iog {

static float midiToHz(int midi) {
    return 440.f * std::pow(2.f, (midi - 69) / 12.f);
}

Lesson makeBeginnerLesson() {
    Lesson lesson;
    lesson.id             = "beginner_clean_timing_01";
    lesson.title          = "Lesson 1 - Clean Notes & Timing";
    lesson.bpm            = 60;
    lesson.beatsPerMeasure = 4;

    // open E, A2, D2, A2, open E
    const int strings[] = {0, 1, 2, 1, 0};
    const int frets[]   = {0, 2, 2, 2, 0};
    const int midis[]   = {40, 47, 52, 47, 40};
    const int fingers[] = {0, 1, 1, 1, 0};

    for (int i = 0; i < 5; ++i) {
        MusicalEvent e;
        e.id          = 100 + i;
        e.startUs     = static_cast<uint64_t>(i) * 1000000ULL;
        e.durationUs  = 700000;
        e.measure     = static_cast<uint16_t>(i / 4 + 1);
        e.beat        = static_cast<uint8_t>(i % 4 + 1);
        e.noteCount   = 1;
        e.notes[0]    = {
            static_cast<uint8_t>(strings[i]),
            static_cast<uint8_t>(frets[i]),
            static_cast<uint8_t>(fingers[i]),
            midiToHz(midis[i]),
            600,   // durationMs
            220,   // timingEarlyMs
            220,   // timingLateMs
            0.05f, // velocityMin
            1.0f,  // velocityMax
            true,  // mustBeClean
            Technique::Normal
        };
        lesson.events.push_back(e);
    }
    return lesson;
}

Lesson makeTechniqueLesson() {
    Lesson lesson;
    lesson.id              = "technique_foundation_01";
    lesson.title           = "Lesson 2 - Legato, Muting & Control";
    lesson.bpm             = 55;
    lesson.beatsPerMeasure = 4;

    struct Entry {
        int s, f, m, fn;
        Technique t;
    };
    const Entry entries[] = {
        {0, 5, 45, 1, Technique::Normal},
        {0, 7, 47, 3, Technique::HammerOn},
        {0, 5, 45, 1, Technique::PullOff},
        {1, 5, 48, 1, Technique::PalmMute},
        {1, 7, 50, 3, Technique::Normal},
        {2, 5, 57, 1, Technique::Normal},
        {2, 7, 59, 3, Technique::Slide},
    };

    for (int i = 0; i < 7; ++i) {
        MusicalEvent e;
        e.id         = 200 + i;
        e.startUs    = static_cast<uint64_t>(i) * 1090909ULL;
        e.durationUs = 700000;
        e.measure    = static_cast<uint16_t>(i / 4 + 1);
        e.beat       = static_cast<uint8_t>(i % 4 + 1);
        e.noteCount  = 1;
        e.notes[0]   = {
            static_cast<uint8_t>(entries[i].s),
            static_cast<uint8_t>(entries[i].f),
            static_cast<uint8_t>(entries[i].fn),
            midiToHz(entries[i].m),
            650, 240, 240, 0.04f, 1.0f, true, entries[i].t
        };
        lesson.events.push_back(e);
    }
    return lesson;
}

Lesson makeChordLesson() {
    Lesson lesson;
    lesson.id              = "chord_timing_01";
    lesson.title           = "Lesson 3 - Chord Timing & Clean Strings";
    lesson.bpm             = 65;
    lesson.beatsPerMeasure = 4;

    // frets[chord][string]; -1 = muted / not played
    const int frets[3][6] = {
        {-1, 3, 2, 0, 1, 0},   // C shape-ish
        { 3, 2, 0, 0, 0, 3},   // G
        {-1, 0, 2, 2, 1, 0},   // Am
    };
    const int midis[3][6] = {
        {0, 48, 52, 57, 61, 64},
        {48, 52, 55, 60, 64, 67},
        {0, 45, 52, 57, 61, 64},
    };

    for (int c = 0; c < 3; ++c) {
        MusicalEvent e;
        e.id         = 300 + c;
        e.startUs    = static_cast<uint64_t>(c) * 923076ULL;
        e.durationUs = 700000;
        e.measure    = 1;
        e.beat       = static_cast<uint8_t>(c + 1);
        e.chord      = true;

        for (int s = 0; s < 6; ++s) {
            if (frets[c][s] < 0)
                continue;
            auto& n = e.notes[e.noteCount++];
            n = {
                static_cast<uint8_t>(s),
                static_cast<uint8_t>(frets[c][s]),
                1,
                midiToHz(midis[c][s]),
                650, 220, 220, 0.03f, 1.0f, true, Technique::Chord
            };
        }
        lesson.events.push_back(e);
    }
    return lesson;
}

}  // namespace iog
