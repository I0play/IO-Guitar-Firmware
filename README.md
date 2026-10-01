# I/O Guitar Firmware v4.5

Teaching-core firmware architecture for the I/O Guitar instructional instrument.

## Production geometry

- ESP32-P4 target
- 6 isolated string channels
- 22 frets / 23 positions per string
- 138 fret-position LEDs
- Upper-horn TFT for real-time TAB and feedback

## Teaching capabilities

- Count-in and synchronized lesson timeline
- String-specific note targeting
- Continuous pitch accuracy in cents
- Timing feedback
- Chord completeness checking
- Extra-note detection
- Technique evidence and grading
- Adaptive guidance
- Adaptive tempo applied to the runtime lesson timeline
- Skill tracking
- TAB/finger/technique information for the display
- Platform-neutral hardware runtime connecting DSP, teaching, LEDs, TFT, encoder, and audio feedback

## Project layout

```text
include/io_guitar/  shared firmware data and APIs
src/dsp.cpp         six-string sensing / technique features
src/teaching.cpp    lesson state machine and grading
src/ui.cpp          138-LED and TFT frame generation
src/lesson_data.cpp built-in curriculum
src/runtime.cpp    hardware orchestration/runtime loop
platform/esp32-p4/ ESP32-P4 board integration boundary
tests/test_all.cpp  host regression suite
docs/               engineering and verification documentation
```

## Host verification

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The host core intentionally keeps physical ESP32-P4 pin assignments and transport drivers separate from the teaching engine so the firmware can be audited independently of the production schematic.

## v4.4 hardware runtime

The host build now exercises the complete software path from six-channel capture through DSP and teaching to LED/TFT/audio/encoder interfaces. See `docs/HARDWARE_RUNTIME_v4.3.md` and `platform/esp32-p4/README.md`.


## v4.4 — Target-Guided Hexaphonic Teaching

The active lesson target is now shared logically with the six-channel sensing
pipeline. The fret LED/TAB target supplies the DSP with an expected string and
frequency prior while the ADC remains electrically independent of the LED bus.

For every detected note the runtime records target string score, pitch error in
cents, pitch score, onset agreement, and combined target affinity. Weak
sympathetic sustain can be suppressed without hiding strong wrong-string or
wrong-pitch performances from the lesson grader.

## v4.5 Lesson Import and Generators

The firmware now has a common lesson ingestion path:

`TAB/MusicXML/MIDI -> Lesson -> TargetGuidedDetector + LessonEngine -> LEDs/TFT/Audio`

Supported embedded import formats are ASCII TAB, MusicXML, and Standard MIDI. Guitar Pro documents these formats as supported interchange/import formats as well. Proprietary `.gp/.gp5/.gpx` parsing is intentionally kept out of the MCU build; convert those files to MusicXML, MIDI, or ASCII first.

The chord/scale engine generates playable six-string exercises directly into the same lesson representation. This means generated material receives the same timing, finger-placement, pitch, string, technique, LED and target-guided DSP behavior as imported songs.
