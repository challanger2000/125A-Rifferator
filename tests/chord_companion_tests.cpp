#include "../src/core/ChordCompanion.hpp"
#include "../src/core/BlockScheduler.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main(){
  assert(chordInterval(1,ChordQuality::Minor)==3);
  assert(chordInterval(1,ChordQuality::Major)==4);
  assert(chordInterval(1,ChordQuality::Power)==7);
  assert(chordInterval(1,ChordQuality::Sus2)==2);
  assert(chordInterval(1,ChordQuality::Sus4)==5);
  // Stable C stays harmonic anchor despite an offbeat chromatic C#.
  std::array<bool,16> enabled{};
  std::array<int,16> articulations{};
  std::array<int,16> degrees{};
  degrees.fill(1);
  enabled[0]=enabled[2]=enabled[4]=enabled[8]=true;
  articulations.fill(3);
  // With a chromatic scale the offbeat degree 2 is C#.
  const std::array<int,7> chromatic{0,1,2,3,4,5,6};
  degrees[2]=2;
  assert(inferredGuitarRoot(enabled,articulations,degrees,chromatic,40)==40);
  enabled.fill(false);
  enabled[0]=enabled[4]=true;
  degrees[0]=degrees[4]=2;
  assert(inferredGuitarRoot(enabled,articulations,degrees,chromatic,40)==41);
  auto notes=makeChordCandidates();
  assert(notes.size()==48);
  HarmonyMap h;
  h.roots[4]=45;
  h.roots[8]=0;
  assert(chordOutputPitch(40,0,h,40)==40);
  assert(chordOutputPitch(40,4,h,40)==45);
  assert(chordOutputPitch(40,8,h,40)==0);
  assert(chordOutputPitch(47,8,h,40)==7);
  BlockScheduler scheduler;
  scheduler.setChords(&notes);
  LiveControls live;
  live.chordsEnabled=true;
  live.harmony=h;
  Pair noGuitar;
  std::vector<ScheduledMidiEvent> events;
  auto sink=[&](const ScheduledMidiEvent& e){events.push_back(e);return true;};
  scheduler.process(noGuitar,0.,0.25,512,61440,sink,true,live);
  int on=0;
  for(const auto& e:events){assert(e.bus==3);if(e.on)++on;}
  assert(on==3);
  assert(scheduler.flushBus(3,sink,0));
  events.clear();
  live.chordQuality=1; // MAJOR
  scheduler.process(noGuitar,0.,0.25,512,61440,sink,true,live);
  bool hasMajorThird=false;
  for(const auto& e:events)if(e.on && e.pitch==44)hasMajorThird=true;
  assert(hasMajorThird);
  assert(scheduler.flushBus(3,sink,0));
  live.chordQuality=0;
  live.chordsRootOnly=true;
  events.clear();
  scheduler.process(noGuitar,0.,0.25,512,61440,sink,true,live);
  on=0;
  for(const auto& e:events){assert(e.bus==3);if(e.on){++on;assert(e.pitch==40);}}
  assert(on==1);
  assert(scheduler.flushBus(3,sink,0));
  live.chordsEnabled=false;
  events.clear();
  scheduler.process(noGuitar,0.,0.25,512,61440,sink,true,live);
  assert(events.empty());
}
