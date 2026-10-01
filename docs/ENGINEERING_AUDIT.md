# Engineering Audit — I/O Guitar Firmware

## Architecture boundaries

The codebase deliberately separates:

1. **Core teaching + DSP library** (`src/`, `include/io_guitar/`)
   - Platform-neutral
   - Fully unit-tested on host
   - No hard dependency on ESP-IDF or board pins

2. **Platform adapter** (`platform/esp32-p4/`)
   - Pin maps, ADC drivers, LED bus, TFT driver, encoder, audio DAC
   - Thin glue that feeds the core Runtime

3. **Host test harness** (`tests/`)
   - Exercises the complete software path without hardware

## Design decisions that matter

- Continuous pitch in cents (not integer MIDI) for grading
- One-to-one note matching (no note can satisfy multiple targets)
- Technique evidence is optional / honest — unknown stays unknown
- Adaptive tempo scales the runtime timeline; original lesson data is immutable
- Target-guided DSP uses the lesson target as a *prior*, never as a forced success

## Known limitations (documented, not hidden)

- Production-grade pitch detector (YIN/MPM) still to be swapped in
- Full continuous-pitch technique classifier is partial
- External lesson file format + NVS profile persistence not yet present
- Real ESP32-P4 board bring-up depends on final schematic

## Verification posture

Every behavioral claim in the changelogs has a corresponding host unit test.
The goal is that a green `ctest` run gives high confidence that the teaching engine behaves as specified, independent of the physical instrument.
