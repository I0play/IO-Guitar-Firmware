# Hardware Runtime v4.3

## Purpose

The `Runtime` class is the single owner of the main loop on both host and target.

It wires together:

- Capture (6-channel ADC samples or host simulation)
- DSP / StringAnalyzer
- LessonEngine + teaching state machine
- LED frame generation
- TFT / TAB frame generation
- Encoder / button input
- Audio feedback

## Host vs target

On host the capture side is simulated and the output side writes frames to test buffers.
On ESP32-P4 the same Runtime calls real drivers.

This keeps the teaching logic identical in both environments.

## Boundary rules

- Core library never includes ESP-IDF headers
- Platform code never implements teaching policy
- All timing is driven from a single monotonic microsecond clock provided by the platform
