# Changelog — I/O Guitar Firmware v4.3

## Hardware runtime layer

- Added platform-neutral `Runtime` that owns the main loop and connects:
  - six-channel capture
  - DSP / StringAnalyzer
  - LessonEngine / teaching
  - LED renderer
  - TFT / TAB renderer
  - encoder / user input
  - audio feedback
- Host build now exercises the complete software path without requiring the physical board.
- ESP32-P4 pin assignments and transport drivers remain outside the core library.

## Documentation

- Added HARDWARE_RUNTIME_v4.3.md describing the runtime boundary.
