#include "../src/core/BlockScheduler.hpp"
#include "../src/core/RiffArrangement.hpp"
#include "../src/core/RiffGeneration.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main(){
 Config cfg;
 FillOptions fill;
 std::vector<Step> source(16,Step{Articulation::PalmMute,1,false,false});
 auto song=arrange(source,cfg,fill,16);
 GuitarLive live;
 live.enabled.fill(true);live.articulation.fill(2);
 live.pmMin=30;live.pmMax=40;live.gate=95;
 std::vector<ScheduledMidiEvent> emitted;
 auto sink=[&](ScheduledMidiEvent event){emitted.push_back(event);return true;};
 // Repeated rapid 16th-note PM notes with a varying block width:
 BlockScheduler scheduler;
 double ppq=0.;
 for(int i=0;i<2048;++i){
   const int samples=(i%3==0?64:i%3==1?256:1024);
   const double next=ppq+samples*120./(60.*44100.);
   scheduler.process(song.guitar,ppq,next,samples,song.periodTicks,sink,true,live);
   ppq=next;
 }
 scheduler.process(song.guitar,ppq,ppq+.01,256,song.periodTicks,sink,false,live);
 int on=0,off=0;
 for(const auto& e:emitted)if(e.bus==0) {
   assert(e.pitch==40);
   if(e.on){++on;assert(e.velocity>=30&&e.velocity<=40);}
   else ++off;
 }
 assert(on>30&&on==off);
 // STOP must cut a sustained OPEN and not generate a PM/ghost note.
 emitted.clear();scheduler.reset();
 live.enabled.fill(false);live.enabled[0]=true;live.enabled[4]=true;
 live.articulation[0]=4;live.articulation[4]=0;
 scheduler.process(song.guitar,0.,1.25,6000,song.periodTicks,sink,true,live);
 int opens=0,releases=0;
 for(auto e:emitted){opens+=e.on;releases+=!e.on;}
 assert(opens==1&&releases==1);
 // Double always has two note attacks with matching note-offs.
 emitted.clear();scheduler.reset();
 live.enabled.fill(false);live.enabled[0]=true;
 live.articulation[0]=2;live.doubled[0]=true;
 scheduler.process(song.guitar,0.,.25,6000,song.periodTicks,sink,true,live);
 opens=releases=0;
 for(auto e:emitted){opens+=e.on;releases+=!e.on;}
 assert(opens==2&&releases==2);
 // Gallop regression: attack offsets at 0 / 0.5 / 0.75 PPQ
 // and repeat at 1 PPQ. It must never generate a 32nd re-trigger.
 emitted.clear();scheduler.reset();
 auto gallop=makeGallop(std::vector<Step>(16,Step{Articulation::Off,1,false,false}));
 auto gallopSong=arrange(gallop,cfg,fill,16);
 live.enabled.fill(false);live.doubled.fill(false);live.gate=95;
 for(int step=0;step<16;++step) {
   live.enabled[step]=gallop[step].articulation!=Articulation::Off;
   live.articulation[step]=static_cast<int>(gallop[step].articulation);
 }
 scheduler.process(gallopSong.guitar,0.,4.,88200,gallopSong.periodTicks,sink,true,live);
 int gallopOn=0,gallopOff=0;
 std::vector<int> offsets;
 for(auto e:emitted) {
   if(e.on){++gallopOn;offsets.push_back(e.sampleOffset);}
   else ++gallopOff;
 }
 assert(gallopOn==12&&gallopOff==12);
 assert(offsets[0]==0&&offsets[1]==11025&&offsets[2]==16537&&offsets[3]==22050);
 // Host rejecting NoteOff cannot be followed by new attacks until flush.
 emitted.clear();scheduler.reset();
 live.doubled[0]=false;
 scheduler.process(song.guitar,0.,.1,600,song.periodTicks,sink,true,live);
 int rejected=0;
 auto reject=[&](ScheduledMidiEvent e){if(!e.on){++rejected;return false;}return true;};
 scheduler.process(song.guitar,.1,.2,600,song.periodTicks,reject,false,live);
 assert(rejected>0);
 emitted.clear();
 scheduler.process(song.guitar,.2,.3,600,song.periodTicks,sink,true,live);
 for(auto e:emitted)assert(!e.on);
}
