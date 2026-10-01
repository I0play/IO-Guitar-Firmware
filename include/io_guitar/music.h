#pragma once
#include "teaching.h"
#include "types.h"
#include <string>
#include <string_view>
#include <vector>

namespace iog {

enum class ImportFormat : uint8_t { Auto, ASCII, MIDI, MusicXML, IONative };
struct ImportResult { bool ok=false; ImportFormat format=ImportFormat::Auto; std::string error; Lesson lesson; size_t warnings=0; };

// Portable import path intended for SD-card files. ASCII, MIDI and MusicXML are
// implemented without dynamic third-party dependencies; GP-family files should
// be converted to MusicXML/MIDI/ASCII before loading on the embedded target.
class LessonImporter {
public:
 static ImportResult importText(std::string_view data, ImportFormat format=ImportFormat::Auto,
                                std::string_view name="import");
 static ImportResult importBinary(const std::vector<uint8_t>& data, ImportFormat format=ImportFormat::Auto,
                                  std::string_view name="import.mid");
};

struct GuitarTuning { std::array<int,6> openMidi{{40,45,50,55,59,64}}; int capo=0; };

enum class ChordQuality : uint8_t { Major, Minor, Dominant7, Major7, Minor7, Diminished, Sus2, Sus4, Power5 };
enum class ScaleType : uint8_t { Major, NaturalMinor, PentatonicMajor, PentatonicMinor, Blues, Dorian, Mixolydian, HarmonicMinor };
struct GeneratedExercise { Lesson lesson; std::string name; };

class ChordScaleGenerator {
public:
 static GeneratedExercise chord(int rootMidi, ChordQuality quality, const GuitarTuning& tuning={},
                                uint8_t startFret=0, uint8_t maxFret=12, uint16_t bpm=60);
 static GeneratedExercise scale(int rootMidi, ScaleType scale, const GuitarTuning& tuning={},
                                uint8_t startFret=0, uint8_t maxFret=12, uint16_t bpm=60);
 static std::vector<int> chordIntervals(ChordQuality q);
 static std::vector<int> scaleIntervals(ScaleType s);
};

} // namespace iog
