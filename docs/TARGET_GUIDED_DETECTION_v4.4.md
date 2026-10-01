# Target-Guided Detection v4.4

## Detection flow

```text
Lesson event
   |
   +--> fret LED / TAB
   |
   +--> target model
           |
           +--> expected string
           +--> expected frequency
           +--> expected timing
           +--> expected technique
                         |
6 sensors -> AFE -> 6ch ADC -> raw DSP -> target-guided scoring
                                           |
                                           +--> string affinity
                                           +--> pitch affinity (cents)
                                           +--> onset affinity
                                           +--> sympathetic-resonance suppression
                                           |
                                           v
                                    teaching grader
```

## Why this is safer than injecting the LED signal into the ADC

The LED data bus is digital and can generate electrical noise. It must not be
used as an analog input or tied into the piezo signal path. The useful
relationship is semantic: both systems receive the same target information
from the lesson engine.

## Wrong-string handling

The target is a prior, not a command to declare success. If the lesson requests
Low E/fret 5 but the player hits the A string at the same pitch, the detector
records:

- pitch affinity: high
- string affinity: low
- target match: false

The teaching engine can therefore report a wrong-string error instead of
accepting the note merely because its pitch is correct.

## Sympathetic resonance

A ringing string that lacks a fresh attack and has weak target affinity is
deprioritized. Strong attacks remain visible even when they are wrong because
the teacher must be able to diagnose mistakes. This does not attempt to
subtract sympathetic resonance mathematically; it uses timing and target
context as evidence.

## Chords

Each chord tone carries its own string/frequency target. The existing
one-to-one matching in the teaching engine remains responsible for final chord
grade; v4.4 simply gives every detected channel the relevant target context
first.

## Hardware boundary

No LED-to-ADC electrical link is required. Keep:

- piezo/saddle sensors electrically isolated from LED power/data
- analog front end properly biased/protected
- ADC synchronized across all six channels
- ESP32-P4 as the shared digital decision layer

The shared target model is implemented in firmware.
