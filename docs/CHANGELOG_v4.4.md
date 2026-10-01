# Changelog — I/O Guitar Firmware v4.4 Target-Guided Detection

## Target-Guided Hexaphonic Teaching

- The active lesson target is now shared logically with the six-channel sensing pipeline.
- Fret LED / TAB target supplies the DSP with an expected string and frequency prior.
- ADC remains electrically independent of the LED bus.
- For every detected note the runtime records:
  - target string score
  - pitch error in cents
  - pitch score
  - onset agreement
  - combined target affinity
- Weak sympathetic sustain can be suppressed without hiding strong wrong-string or wrong-pitch performances from the lesson grader.

## Detection model

See docs/TARGET_GUIDED_DETECTION_v4.4.md for the full design.
