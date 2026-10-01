# Contributing to I/O Guitar Firmware

Thanks for helping improve the teaching core.

## Workflow

1. Create a branch from `main`:
   ```bash
   git checkout -b feature/your-change
   ```
2. Make your changes.
3. Run the host tests locally:
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build -j
   ctest --test-dir build --output-on-failure
   ```
4. Open a Pull Request against `main`.
5. CI must be green. Conversations must be resolved.

## Code style

- C++17
- Prefer fixed-size buffers on hot paths (no `std::string` in real-time code)
- Keep the core library free of ESP-IDF headers
- Platform-specific code lives only under `platform/`

## What belongs where

| Area | Location |
|------|----------|
| Teaching / DSP / lesson logic | `src/`, `include/io_guitar/` |
| Host unit tests | `tests/` |
| Board drivers / pin maps | `platform/esp32-p4/` |
| Design notes & changelogs | `docs/` |

## Commit messages

Use short, imperative summaries:

- `Fix wrong-string rejection in LessonEngine`
- `Add MIDI import path for LessonImporter`
- `docs: clarify target-guided prior vs forced success`

## Releases

Releases are tagged on `main` as `vX.Y.Z`. Update the matching changelog under `docs/` before tagging.
