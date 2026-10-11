#include "../src/core/BlockScheduler.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main(){
 Pair p;
 p.guitar.push_back({0,960,40,110,960});
 p.bass.push_back({0,240,28,95,240});
 std::vector<MidiNote> drums{{0,120,36,110,120}};
 auto chords=makeChordCandidates();
 BlockScheduler s;
 s.setDrums(&drums); s.setChords(&chords);
 LiveControls live;
 live.guitarSteps.fill(true);
 live.guitarArticulation.fill(2);
 live.guitarDegree.fill(1);
 live.drumSteps[0][0]=true;
 live.chordsEnabled=true;
 std::vector<ScheduledMidiEvent> received;
 auto sink=[&](ScheduledMidiEvent e){received.push_back(e);return true;};
 s.process(p,0.,0.125,512,16*3840,sink,true,live);
 bool on[4]{};
 for(const auto& e:received) {
   assert(e.bus>=0 && e.bus<4);
   assert(e.sampleOffset>=0 && e.sampleOffset<512);
   assert(e.pitch>=0 && e.pitch<=127);
   if(e.on)on[e.bus]=true;
 }
 for(bool v:on)assert(v);
 received.clear();
 s.process(p,0.125,0.25,512,16*3840,sink,false,live);
 bool off[4]{};
 for(const auto& e:received)if(!e.on)off[e.bus]=true;
 for(bool v:off)assert(v);
 received.clear();
 s.process(p,0.,0.125,512,16*3840,sink,true,live);
 int restarts[4]{};
 for(const auto& e:received)if(e.on)++restarts[e.bus];
 for(int v:restarts)assert(v>0);
 // Failing output sink must not falsely confirm delivered note-ons.
 BlockScheduler rejected;
 rejected.setDrums(&drums);rejected.setChords(&chords);
 auto failSink=[](ScheduledMidiEvent){return false;};
 rejected.process(p,0.,0.125,512,16*3840,failSink,true,live);
}
