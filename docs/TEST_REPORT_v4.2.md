# v4.2 Host Verification Report

## Build

Command:

```text
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

## Result

- Core library: built successfully
- Test executable: built successfully
- CTest: **100% passed**
- Test count: 1 CTest target containing the complete regression executable

## Covered behaviors

1. 138-position LED geometry
2. Open-low-E pitch sanity
3. Technique classifier does not fabricate advanced-technique success without evidence
4. Count-in does not consume the first event
5. Continuous cents tolerance accepts ~39 cents and rejects ~77 cents with a 40-cent limit
6. Pause/resume preserves musical elapsed time
7. Wrong-string same-pitch attempt is rejected
8. Extra detected note is rejected
9. Technique evidence reaches the lesson grader and can produce a successful technique result
10. Adaptive tempo calculation is active
11. TAB/display renderer receives current lesson TAB data

## Hardware note

This is a host-core verification result. It does not constitute electrical validation of the final ESP32-P4 hardware, ADC front end, LED driver chain, TFT interface, power system or guitar mechanical assembly.
