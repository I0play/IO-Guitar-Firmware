# I/O Guitar Firmware v4.5 — Lesson Import + Chord/Scale Engine

- Added embedded lesson import API for ASCII TAB, MusicXML and MIDI.
- Imported lessons become the same `Lesson`/`MusicalEvent` objects used by the teaching engine, LED renderer, TAB renderer and target-guided DSP.
- Added chord generator with major, minor, dominant 7, major 7, minor 7, diminished, sus2, sus4 and power-5 interval sets.
- Added scale generator with major, natural minor, major/minor pentatonic, blues, Dorian, Mixolydian and harmonic minor patterns.
- Added runtime APIs to load imported lessons and generated exercises directly.
- MIDI notes are automatically mapped to the six-string, 22-fret production neck where a valid position exists.
- MusicXML string/fret technical data is honored when present; otherwise the importer chooses a playable position.
- ASCII TAB is parsed into playable note events; rhythm is normalized for embedded playback.
- Proprietary Guitar Pro formats are intentionally not parsed natively on the MCU; convert them to MusicXML/MIDI/ASCII before loading. These are standard interchange paths used by Guitar Pro. See the engineering note.
