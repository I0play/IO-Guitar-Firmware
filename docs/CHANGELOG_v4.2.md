# Changelog — I/O Guitar Firmware v4.2 Teaching Core

## Implemented corrections

### Timeline / count-in
- Musical time now begins **after** the count-in.
- The first lesson event cannot expire while the count-in is playing.
- Event timing, duration and miss timeout use the same musical timeline.
- Pause/resume preserves elapsed musical time instead of reconstructing the timeline from the current event start.

### Pitch grading
- Removed integer-MIDI pitch grading as the primary accuracy test.
- Pitch is now compared from continuous detected/target frequencies using cents:
  `1200 * log2(detectedHz / targetHz)`.
- The configured 40-cent tolerance therefore means an actual 40-cent window rather than a rounded-semitone window.

### Technique teaching
- Technique evidence is now carried in `DetectedNote` / `PerformanceInput`.
- `StringAnalyzer` exports pitch trajectory, direction and oscillation evidence.
- Hammer-on / pull-off observations use trajectory direction as an additional discriminator.
- The lesson engine now evaluates the detected technique against the target technique.
- Advanced technique attempts without sufficient evidence remain unevaluated instead of being silently accepted.
- Technique results now feed the skill tracker.

### Note matching
- Target notes and detected notes are matched one-to-one.
- A detected note cannot satisfy multiple target notes.
- Chords require every target tone to be matched.
- Unmatched detected notes are reported as `ExtraNote` and prevent a clean pass.
- Wrong-string same-pitch attempts remain rejected.

### Adaptive tempo
- Adaptive BPM is now applied to event start times and event durations through a runtime time scale.
- Original lesson timestamps remain immutable.
- The active BPM is exposed in `TeachingSnapshot` for UI use.

### Real-time TAB / display model
- `TeachingSnapshot` now contains current measure, beat, subdivision, TAB notes, finger hints and technique metadata.
- `Renderer::tft()` now renders TAB-oriented data, active BPM, timing, pitch skill and technique information.
- This provides the firmware-side data model required by the upper-horn real-time TAB display.

## Regression coverage
Added host tests for:
- count-in timing
- continuous cents tolerance
- pause/resume musical time
- wrong-string rejection
- extra-note rejection
- technique-to-lesson integration
- adaptive tempo behavior
- TAB snapshot fields
