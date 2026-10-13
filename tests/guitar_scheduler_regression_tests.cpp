#include "../src/core/BlockScheduler.hpp"
#include "../src/core/RiffArrangement.hpp"
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
 // Manual short gallop: player sets PM at steps 0, 2, 3 and silence
 // at 1. Other steps remain independently programmable; no global mode.
 emitted.clear();scheduler.reset();
 std::vector<Step> manual(16,Step{Articulation::Off,1,false,false});
 for(int step:{0,2,3})manual[step].articulation=Articulation::PalmMute;
 auto manualSong=arrange(manual,cfg,fill,16);
 live.enabled.fill(false);live.doubled.fill(false);live.gate=95;
 for(int step:{0,2,3})live.enabled[step]=true;
 scheduler.process(manualSong.guitar,0.,1.,44100,manualSong.periodTicks,sink,true,live);
 int shortOn=0,shortOff=0;
 std::vector<int> shortOffsets;
 for(const auto& e:emitted){
   if(e.on){++shortOn;shortOffsets.push_back(e.sampleOffset);}
   else ++shortOff;
 }
 assert(shortOn==3&&shortOff==3);
 assert(shortOffsets.size()==3);
 assert(shortOffsets[0]==0&&shortOffsets[1]==22050&&shortOffsets[2]==33075);
 // Per-step DOUBLE is independent of the manual gallop grid.
 // Both DOUBLE notes must use the CURRENT guitar library's PM range.
 // Kontakt-specific fixed second velocities must never leak into Heavier7Strings.
 // Heavier7Strings AUTO selects physical stroke direction itself.
 emitted.clear();scheduler.reset();
 live.enabled.fill(false);live.enabled[8]=true;live.doubled[8]=true;
 scheduler.process(song.guitar,2.,2.25,6000,song.periodTicks,sink,true,live);
 int doubleOn=0,doubleOff=0;
 std::vector<int> doubleOffsets,doubleVels;
 for(auto e:emitted){
   if(e.on){++doubleOn;doubleOffsets.push_back(e.sampleOffset);doubleVels.push_back(e.velocity);}
   else ++doubleOff;
 }
 assert(doubleOn==2&&doubleOff==2);
 assert(doubleOffsets[0]==0&&doubleOffsets[1]==3000);
 assert(doubleVels[0]>=30&&doubleVels[0]<=40);
 assert(doubleVels[1]>=30&&doubleVels[1]<=40);
 assert(doubleVels[1]<=doubleVels[0]);
 // A narrow PM band cannot cause the second strike to escape its bounds.
 emitted.clear();scheduler.reset();
 live.pmMin=36;live.pmMax=36;
 scheduler.process(song.guitar,2.,2.25,6000,song.periodTicks,sink,true,live);
 int narrowOn=0;
 for(auto e:emitted)if(e.on){++narrowOn;assert(e.velocity==36);}
 assert(narrowOn==2);
 live.pmMin=30;live.pmMax=40;
 // Pure PM steps MUST NOT inject secondary hits unless DOUBLE enabled.
 emitted.clear();scheduler.reset();
 live.doubled.fill(false);
 scheduler.process(song.guitar,2.,2.25,6000,song.periodTicks,sink,true,live);
 int singleOn=0;
 for(auto e:emitted)if(e.on)++singleOn;
 assert(singleOn==1);
 // A normal manually-programmed metal gallop has 1/8 then 1/16 + 1/16
 // WITHOUT applying DOUBLE anywhere: long PM at step 0, empty step 1,
 // then short PMs at steps 2 and 3. Duration MUST differ, not just spacing.
 emitted.clear();scheduler.reset();
 live.enabled.fill(false);live.doubled.fill(false);live.eighth.fill(false);
 for(int i:{0,2,3}) {live.enabled[i]=true;live.articulation[i]=2;}
 live.eighth[0]=true;
 scheduler.process(song.guitar,0.,1.,44100,song.periodTicks,sink,true,live);
 int manualOns=0,manualOffs=0;
 std::vector<int> hits,ends;
 for(const auto& e:emitted) {
   if(e.on){++manualOns;hits.push_back(e.sampleOffset);}
   else {++manualOffs;ends.push_back(e.sampleOffset);}
 }
 assert(manualOns==3&&manualOffs==3);
 assert(hits[0]==0&&hits[1]==22050&&hits[2]==33075);
 assert(ends[0]>=20946&&ends[0]<=20949);
 assert(ends[1]>=32523&&ends[1]<=32525);
 assert(ends[2]>=43548&&ends[2]<=43550);
 // Editing only one step's length must not alter the following note.
 emitted.clear();scheduler.reset();
 live.eighth[0]=false;
 scheduler.process(song.guitar,0.,1.,44100,song.periodTicks,sink,true,live);
 int short0Off=-1;
 for(const auto& e:emitted)if(!e.on&&e.pitch==40){short0Off=e.sampleOffset;break;}
 assert(short0Off>=10473&&short0Off<=10475);
 // If another attack starts at the 2nd sixteenth, the eighth must end
 // BEFORE that new attack; overlapping same-pitch NoteOff is forbidden.
 emitted.clear();scheduler.reset();
 live.enabled[1]=true;live.eighth[0]=true;
 scheduler.process(song.guitar,0.,.5,22050,song.periodTicks,sink,true,live);
 int firstEnd=-1,nextAttack=-1,onCount=0;
 for(const auto& e:emitted){
   if(e.on){++onCount;if(onCount==2)nextAttack=e.sampleOffset;}
   else if(firstEnd<0)firstEnd=e.sampleOffset;
 }
 assert(firstEnd>=0&&nextAttack>=0&&firstEnd<nextAttack);
 // Per-step DOUBLE remains independent of LEN=1/8, and sends exactly
 // two sequential 1/16 PM strikes for one lengthened step.
 emitted.clear();scheduler.reset();
 live.enabled.fill(false);live.enabled[0]=true;
 live.eighth.fill(false);live.eighth[0]=true;
 live.doubled.fill(false);live.doubled[0]=true;
 scheduler.process(song.guitar,0.,.5,22050,song.periodTicks,sink,true,live);
 hits.clear();ends.clear();
 for(const auto& e:emitted){if(e.on)hits.push_back(e.sampleOffset);else ends.push_back(e.sampleOffset);}
 assert(hits.size()==2&&ends.size()==2);
 assert(hits[0]==0&&hits[1]==11025);
 assert(ends[0]<hits[1]&&ends[1]>hits[1]);
 // Host rejecting NoteOff cannot be followed by new attacks until flush.
 emitted.clear();scheduler.reset();
 live.enabled.fill(false);live.enabled[0]=true;
 live.doubled.fill(false);live.eighth.fill(false);
 scheduler.process(song.guitar,0.,.1,600,song.periodTicks,sink,true,live);
 int rejected=0;
 auto reject=[&](ScheduledMidiEvent e){if(!e.on){++rejected;return false;}return true;};
 scheduler.process(song.guitar,.1,.2,600,song.periodTicks,reject,false,live);
 assert(rejected>0);
 emitted.clear();
 scheduler.process(song.guitar,.2,.3,600,song.periodTicks,sink,true,live);
 for(auto e:emitted)assert(!e.on);
}
