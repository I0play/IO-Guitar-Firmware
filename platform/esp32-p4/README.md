# ESP32-P4 Platform Adapter

This directory is the **only** place that should contain ESP-IDF specific code,
pin maps, ADC drivers, LED bus, TFT driver, encoder, and audio DAC glue.

The core library (`src/`, `include/io_guitar/`) must remain free of ESP-IDF
headers so it can be fully unit-tested on host.

## Expected contents (to be filled during board bring-up)

- `main/` or component structure for ESP-IDF
- Pin assignments for 6-channel ADC, LED data, TFT, encoder, audio
- Thin implementations of:
  - `SixChannelAudioInput`
  - `FretLedOutput`
  - `HornDisplayOutput`
  - `EncoderInput`
  - `AudioFeedbackOutput`

## Build (once ESP-IDF project is present)

```bash
idf.py -C platform/esp32-p4 build
```

See also `docs/HARDWARE_RUNTIME_v4.3.md`.
