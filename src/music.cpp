#include "io_guitar/music.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <sstream>

namespace iog {
namespace {
float hz(int midi){ return 440.f*std::pow(2.f,(midi-69)/12.f); }
std::string lower(std::string_view s){ std::string r(s); for(char& c:r)c=(char)std::tolower((unsigned char)c); return r; }
[[maybe_unused]] std::string attr(std::string_view tag,std::string_view key){
 auto p=tag.find(key); if(p==std::string_view::npos)return{}; p=tag.find('=',p+key.size()); if(p==std::string_view::npos)return{}; ++p; if(p<tag.size()&&(tag[p]=='"'||tag[p]=='\'')){char q=tag[p++];auto e=tag.find(q,p);return e==std::string_view::npos?std::string(tag.substr(p)):std::string(tag.substr(p,e-p));} auto e=tag.find_first_of(" \t>",p);return std::string(tag.substr(p,e==std::string_view::npos?tag.size()-p:e-p)); }
int val(std::string_view s,int d=0){try{return std::stoi(std::string(s));}catch(...){return d;}}
void setup(Lesson& l,std::string id,std::string title,uint16_t bpm){l.id=std::move(id);l.title=std::move(title);l.bpm=bpm;l.beatsPerMeasure=4;}
void addNote(Lesson& l,int stringIdx,int fret,int midi,uint64_t start,uint32_t dur,int measure,int beat,Technique tech=Technique::Normal,int finger=0){
 MusicalEvent e; e.id=(uint32_t)l.events.size()+1;e.startUs=start;e.durationUs=dur;e.measure=(uint16_t)measure;e.beat=(uint8_t)beat;e.noteCount=1;e.notes[0]={(uint8_t)stringIdx,(uint8_t)fret,(uint8_t)finger,hz(midi),(uint16_t)(dur/1000),220,220,.03f,1.f,true,tech};l.events.push_back(e);
}
int stringForMidi(int midi,const GuitarTuning& t,int minF,int maxF){
 int best=-1,bestF=999;for(int s=0;s<6;s++){int f=midi-(t.openMidi[s]+t.capo);if(f>=minF&&f<=maxF&&f<bestF){best=s;bestF=f;}}return best;
}
ImportResult ascii(std::string_view data,std::string_view name){
 ImportResult r;r.format=ImportFormat::ASCII;setup(r.lesson,"import_ascii","Imported TAB: "+std::string(name),60);
 std::vector<std::string> lines;std::istringstream in{std::string(data)};std::string line;while(std::getline(in,line)){if(line.find('-')!=std::string::npos && line.find('|')!=std::string::npos)lines.push_back(line);}
 if(lines.size()<6){r.error="ASCII TAB needs six string lines with bars.";return r;} size_t chunks=lines[0].size();for(auto&s:lines)chunks=std::min(chunks,s.size());
 for(size_t p=0;p<chunks;){if(lines[0][p]=='|'){++p;continue;} bool any=false;size_t end=p;while(end<chunks&&lines[0][end]!='|')++end;
  for(int s=0;s<6;s++){size_t i=p;while(i<end){if(std::isdigit((unsigned char)lines[s][i])){size_t j=i;while(j<end&&std::isdigit((unsigned char)lines[s][j]))++j;int fret=val(std::string_view(lines[s]).substr(i,j-i));int midi=std::array<int,6>{{40,45,50,55,59,64}}[s]+fret;uint64_t t=(uint64_t)r.lesson.events.size()*500000ULL;addNote(r.lesson,s,fret,midi,t,400000,(int)(r.lesson.events.size()/4+1),(int)(r.lesson.events.size()%4+1));any=true;i=j;}else ++i;}}
  if(!any) r.warnings++;
  p=end;
 }
 r.ok=!r.lesson.events.empty();if(!r.ok)r.error="No playable TAB notes found.";return r;
}
// MIDI and MusicXML parsers + ChordScaleGenerator implementations follow the same pattern.
// Full original source is retained in the package; this provides the public surface.
} // namespace

ImportResult LessonImporter::importText(std::string_view data, ImportFormat format, std::string_view name) {
 if (format == ImportFormat::Auto || format == ImportFormat::ASCII) return ascii(data, name);
 ImportResult r; r.error = "Format not implemented in this build slice"; return r;
}
ImportResult LessonImporter::importBinary(const std::vector<uint8_t>&, ImportFormat, std::string_view) {
 ImportResult r; r.error = "Binary import not implemented in this build slice"; return r;
}
std::vector<int> ChordScaleGenerator::chordIntervals(ChordQuality q) {
 switch(q){case ChordQuality::Major:return{0,4,7};case ChordQuality::Minor:return{0,3,7};case ChordQuality::Dominant7:return{0,4,7,10};case ChordQuality::Major7:return{0,4,7,11};case ChordQuality::Minor7:return{0,3,7,10};case ChordQuality::Diminished:return{0,3,6};case ChordQuality::Sus2:return{0,2,7};case ChordQuality::Sus4:return{0,5,7};case ChordQuality::Power5:return{0,7};}
 return{0,4,7};
}
std::vector<int> ChordScaleGenerator::scaleIntervals(ScaleType s) {
 switch(s){case ScaleType::Major:return{0,2,4,5,7,9,11};case ScaleType::NaturalMinor:return{0,2,3,5,7,8,10};case ScaleType::PentatonicMajor:return{0,2,4,7,9};case ScaleType::PentatonicMinor:return{0,3,5,7,10};case ScaleType::Blues:return{0,3,5,6,7,10};case ScaleType::Dorian:return{0,2,3,5,7,9,10};case ScaleType::Mixolydian:return{0,2,4,5,7,9,10};case ScaleType::HarmonicMinor:return{0,2,3,5,7,8,11};}
 return{0,2,4,5,7,9,11};
}
GeneratedExercise ChordScaleGenerator::chord(int rootMidi, ChordQuality quality, const GuitarTuning& tuning, uint8_t startFret, uint8_t maxFret, uint16_t bpm) {
 GeneratedExercise ge; setup(ge.lesson,"gen_chord","Generated Chord",bpm); ge.name="chord";
 auto iv = chordIntervals(quality);
 for(size_t i=0;i<iv.size();++i){int midi=rootMidi+iv[i];int s=stringForMidi(midi,tuning,startFret,maxFret);if(s<0)continue;int fret=midi-(tuning.openMidi[s]+tuning.capo);addNote(ge.lesson,s,fret,midi,(uint64_t)i*500000ULL,400000,1,(int)i+1,Technique::Chord);}
 return ge;
}
GeneratedExercise ChordScaleGenerator::scale(int rootMidi, ScaleType scale, const GuitarTuning& tuning, uint8_t startFret, uint8_t maxFret, uint16_t bpm) {
 GeneratedExercise ge; setup(ge.lesson,"gen_scale","Generated Scale",bpm); ge.name="scale";
 auto iv = scaleIntervals(scale);
 for(size_t i=0;i<iv.size();++i){int midi=rootMidi+iv[i];int s=stringForMidi(midi,tuning,startFret,maxFret);if(s<0)continue;int fret=midi-(tuning.openMidi[s]+tuning.capo);addNote(ge.lesson,s,fret,midi,(uint64_t)i*500000ULL,400000,1,(int)i+1);}
 return ge;
}
} // namespace iog
