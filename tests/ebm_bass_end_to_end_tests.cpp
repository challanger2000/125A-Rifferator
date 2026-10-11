#include "../src/core/EbmBassArrangement.hpp"
#include "../src/core/EbmSchedulerAdapter.hpp"
#include <array>
#include <cassert>
#include <cstdint>
using namespace a125::rifferator;
using namespace a125::rifferator::ebm_integration;
int main() {
    for(auto mode : {BassMode::Ebm,BassMode::Hybrid}) {
      for(auto style : {ebm::Style::Classic,ebm::Style::Chromatic,
                        ebm::Style::Octave,ebm::Style::Industrial,
                        ebm::Style::Rolling}) {
        BassPlan p;
        p.mode=mode;
        p.tension=Tension::Aggressive;
        p.sequence.style=style;
        p.sequence.root=48;
        p.sequence.seed=12345;
        p.sequence.gatePermille=850;
        p.sequence.variation=80;
        p.sequence.rhythmVariation=40;
        p.phraseVariation=85;
        p.arrangementEvolution=90;
        p.fillIntensity=100;
        p.fillEveryBars=4;
        p.dynamics=90;
        p.harmonicRoots.fill(48);
        for(int i=4;i<8;++i)p.harmonicRoots[static_cast<unsigned>(i)]=53;
        for(int i=8;i<12;++i)p.harmonicRoots[static_cast<unsigned>(i)]=46;
        for(int i=12;i<16;++i)p.harmonicRoots[static_cast<unsigned>(i)]=55;
        Arrangement a;
        a.midi.guitar.push_back({0,120,40,110,120});
        prepare(a,p);
        assert(a.periodTicks==61440);
        assert(a.midi.guitar.size()==1);
        EbmSchedulerAdapter scheduler;
        std::array<bool,128> active{};
        int ons=0,offs=0;
        int firstVelocity=-1;
        int firstPitch=-1;
        auto sink=[&](const ScheduledMidiEvent& e) noexcept {
            assert(e.bus==1 && e.pitch>=0 && e.pitch<128);
            if(e.on) {
                assert(e.velocity>=1 && e.velocity<=127);
                assert(!active[static_cast<unsigned>(e.pitch)]);
                active[static_cast<unsigned>(e.pitch)]=true;
                if(firstVelocity<0){firstVelocity=e.velocity;firstPitch=e.pitch;}
                ++ons;
            } else {
                assert(active[static_cast<unsigned>(e.pitch)]);
                active[static_cast<unsigned>(e.pitch)]=false;
                ++offs;
            }
            return true;
        };
        // 16 bars x 16 steps, twice. Block-boundary MIDI starts and offs
        // must neither duplicate nor drop even when blocks only have 1 sample.
        for(int cycle=0;cycle<2;++cycle) {
          for(int step=0;step<256;++step) {
            const double start=cycle*64.0+step*0.25;
            assert(scheduler.process(a,start,start+0.25,1,true,sink));
          }
          for(bool on:active)assert(!on);
        }
        assert(ons==static_cast<int>(2*a.midi.bass.size()));
        assert(offs==ons);
        assert(firstVelocity==a.midi.bass.front().velocity);
        assert(firstPitch==a.midi.bass.front().note);
        assert(scheduler.process(a,128.0,128.25,128,false,sink));
      }
    }
}
