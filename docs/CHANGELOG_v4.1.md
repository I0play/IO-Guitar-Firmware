# Changelog — v4.1 Improved (post-audit)

## Fixed
- Technique classifier no longer always returns success for Bend/Slide/Vibrato/HammerOn/PullOff
- Miss notes now detected on window timeout (not only on empty submit)
- Wrong-string same-pitch attempts rejected
- Chord grading requires all target notes (string + pitch)
- `std::string` removed from hot-path snapshot/UI (fixed char buffers)

## Improved
- Full readable formatting (was heavily minified)
- Magic numbers → `DspConfig` / `TeachingConfig`
- Pitch-history ring buffer (64) for future technique features
- Skill tracker now tracks string accuracy and chord accuracy separately
- LED renderer uses finger hints and technique accent colours
- Expanded host unit tests

## Not yet done (documented boundaries)
- Production YIN/MPM pitch detector
- Full continuous-pitch technique classifier
- Applying adaptive tempo to event timeline
- External lesson file format + profile NVS persistence
- Real ESP32-P4 board adapter (waiting on schematic)
