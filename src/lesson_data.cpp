// Placeholder - full implementation will be pushed in follow-up if needed.
// See local package for complete source.
#include "io_guitar/teaching.h"

namespace iog {

Lesson makeBeginnerLesson() {
    Lesson l;
    l.id = "beginner-clean-notes";
    l.title = "Clean Notes & Timing";
    l.bpm = 60;
    // Minimal stub so the library links; full curriculum lives in the original package.
    return l;
}

Lesson makeTechniqueLesson() {
    Lesson l;
    l.id = "technique-legato";
    l.title = "Legato, Muting & Control";
    l.bpm = 60;
    return l;
}

Lesson makeChordLesson() {
    Lesson l;
    l.id = "chord-timing";
    l.title = "Chord Timing & Clean Strings";
    l.bpm = 60;
    return l;
}

} // namespace iog
