#include "../src/core/EbmSchedulerAdapter.hpp"
#include <cassert>
#include <vector>
#include <array>
#include <cmath>
using namespace a125::rifferator;
using namespace a125::rifferator::ebm_integration;
int main() {
    Arrangement a;
    a.periodTicks=3840;
    a.midi.bass={{0,240,36,75,240},{240,480,36,105,480}};
    EbmSchedulerAdapter adapter;
    std::vector<ScheduledMidiEvent> events;
    const auto sink=[&](const ScheduledMidiEvent& e){events.push_back(e);return true;};
    assert(adapter.process(a,0.0,0.6,576,true,sink));
    assert(events.size()==4);
    assert(events[0].on && events[0].velocity==75 && events[0].bus==1);
    assert(!events[1].on && events[1].sampleOffset==events[2].sampleOffset);
    assert(events[2].on && events[2].velocity==105);
    assert(!events[3].on);
    events.clear();
    assert(adapter.process(a,0.6,1.0,384,true,sink));
    assert(events.empty());
    events.clear();
    adapter.reset();
    assert(adapter.process(a,0.0,0.1,480,true,sink));
    assert(events.size()==1 && events[0].on);
    events.clear();
    assert(adapter.process(a,0.1,0.2,480,false,sink));
    assert(events.size()==1 && !events[0].on);
    // Transport seek flushes an active pitch before emitting new notes.
    adapter.reset(); events.clear();
    assert(adapter.process(a,0.0,0.1,480,true,sink));
    events.clear();
    assert(adapter.process(a,4.0,4.1,480,true,sink));
    assert(events.size()>=2 && !events[0].on && events[1].on);
    // Hot swap with uninterrupted DAW transport: flush old note first.
    adapter.reset(); events.clear();
    assert(adapter.process(a,0.0,0.1,480,true,sink));
    events.clear();
    Arrangement replacement;
    replacement.periodTicks=3840;
    replacement.midi.bass={{0,240,48,118,240}};
    assert(adapter.patternChanged(sink));
    assert(events.size()==1 && !events[0].on && events[0].pitch==36);
    events.clear();
    assert(adapter.process(replacement,0.0,0.1,480,true,sink));
    assert(events.size()==1 && events[0].on && events[0].pitch==48);
    events.clear();
    assert(adapter.patternChanged(sink));
    assert(events.size()==1 && !events[0].on && events[0].pitch==48);
    // A refused NoteOff during swap must be retriable, not forgotten.
    events.clear(); adapter.reset();
    assert(adapter.process(a,0.0,0.1,480,true,sink));
    const auto refuseSwap=[&](const ScheduledMidiEvent& e){return e.on;};
    assert(!adapter.patternChanged(refuseSwap));
    events.clear();
    assert(adapter.patternChanged(sink));
    assert(events.size()==1 && !events[0].on && events[0].pitch==36);
    events.clear();
    // Host rejection does not falsely clear the active note; flush can retry.
    adapter.reset(); events.clear();
    assert(adapter.process(a,0.0,0.1,480,true,sink));
    int rejected=0;
    const auto refuse=[&](const ScheduledMidiEvent& e){if(!e.on){++rejected;return false;}return true;};
    assert(!adapter.process(a,0.1,0.2,480,false,refuse));
    assert(rejected>0);
    int recovered=0;
    assert(adapter.flush([&](const ScheduledMidiEvent& e){if(!e.on)++recovered;return true;}));
    assert(recovered==1);
    // Rejected NoteOff must block subsequent NoteOns until recovery succeeds.
    {
        EbmSchedulerAdapter unit;
        Arrangement one;
        one.periodTicks=3840;
        one.midi.bass={{0,240,36,95,240},{240,480,36,105,480}};
        int ons=0,offs=0;
        auto broken=[&](const ScheduledMidiEvent& e) {
            if(!e.on) return false;
            ++ons; return true;
        };
        assert(!unit.process(one,0.0,0.6,576,true,broken));
        assert(ons==1);
        // Still rejected: no further note-on is allowed.
        assert(!unit.process(one,0.6,1.0,384,true,broken));
        assert(ons==1);
        auto accept=[&](const ScheduledMidiEvent& e) {
            if(e.on) ++ons; else ++offs;
            return true;
        };
        assert(!unit.process(one,0.6,1.0,384,true,accept));
        assert(ons==1 && offs==1);
        // Playback can resume from a new valid block with no stuck notes.
        assert(unit.process(one,0.0,0.1,96,true,accept));
        assert(ons==2);
        assert(unit.process(one,0.1,0.2,96,false,accept));
        assert(offs==2);
    }
    // Invalid host transport + refused flush must inhibit all new NoteOns.
    {
        EbmSchedulerAdapter unit;
        Arrangement one;
        one.periodTicks=3840;
        one.midi.bass={{0,240,40,98,240}};
        int noteOns=0, noteOffs=0;
        auto ok=[&](const ScheduledMidiEvent& e){
            if(e.on) ++noteOns; else ++noteOffs;
            return true;
        };
        assert(unit.process(one,0.0,0.1,96,true,ok));
        assert(noteOns==1);
        auto refuseOff=[&](const ScheduledMidiEvent& e){return e.on;};
        assert(!unit.process(one,1.0e20,1.0e20+1.0e10,96,true,refuseOff));
        assert(!unit.process(one,0.0,0.1,96,true,refuseOff));
        assert(noteOns==1);
        assert(!unit.process(one,0.0,0.1,96,true,ok));
        assert(noteOffs==1 && noteOns==1);
        assert(unit.process(one,0.0,0.1,96,true,ok));
        assert(noteOns==2);
    }
    // Empty pattern emits no NoteOns and does not retain a previously active note.
    {
        EbmSchedulerAdapter unit;
        Arrangement sounding;
        sounding.periodTicks=3840;
        sounding.midi.bass={{0,240,36,105,240}};
        std::vector<ScheduledMidiEvent> got;
        auto collect=[&](const ScheduledMidiEvent& e) { got.push_back(e); return true; };
        assert(unit.process(sounding,0.0,0.1,480,true,collect));
        assert(got.size()==1 && got[0].on);
        got.clear();
        assert(unit.patternChanged(collect));
        assert(got.size()==1 && !got[0].on && got[0].pitch==36);
        got.clear();
        sounding.midi.bass.clear();
        assert(unit.process(sounding,0.1,0.2,480,true,collect));
        assert(got.empty());
        // Invalid note range must fail and flush rather than reach an indexed active array.
        sounding.midi.bass={{0,240,128,100,240}};
        assert(!unit.process(sounding,0.2,0.3,480,true,collect));
        assert(got.empty());
    }
    // Extreme host PPQ must be rejected after flushing the sounding note.
    {
        EbmSchedulerAdapter unit;
        Arrangement one;
        one.periodTicks=3840;
        one.midi.bass={{0,240,40,99,240}};
        std::vector<ScheduledMidiEvent> got;
        auto collect=[&](const ScheduledMidiEvent& e){got.push_back(e);return true;};
        assert(unit.process(one,0.0,0.1,64,true,collect));
        assert(got.size()==1 && got[0].on);
        got.clear();
        assert(!unit.process(one,1.0e20,1.0e20+1.0e10,64,true,collect));
        assert(got.size()==1 && !got[0].on);
    }
    // Dense host window must reject overflow explicitly, without emitting partial
    // NoteOn data that could create stuck notes.
    {
        EbmSchedulerAdapter unit;
        Arrangement dense;
        dense.periodTicks=3840;
        for(int i=0;i<16;++i) {
            const int64_t start=static_cast<int64_t>(i)*240;
            dense.midi.bass.push_back({start,start+120,36+(i%4),100,start+120});
        }
        int delivered=0;
        auto count=[&](const ScheduledMidiEvent&){++delivered;return true;};
        assert(!unit.process(dense,0.0,80.0,4096,true,count));
        assert(delivered==0);
        // A regular block after failure must still work.
        assert(unit.process(dense,0.0,0.25,256,true,count));
        assert(delivered==2);
    }
    // An invalid velocity must be rejected without emitting any NoteOns.
    {
        EbmSchedulerAdapter unit;
        Arrangement invalid;
        invalid.periodTicks=3840;
        invalid.midi.bass={{0,120,40,0,120}};
        int delivered=0;
        auto count=[&](const ScheduledMidiEvent&){++delivered;return true;};
        assert(!unit.process(invalid,0.0,0.25,256,true,count));
        assert(delivered==0);
    }
    // Stress: varying block lengths, repeated 16-bar loop and stop recovery.
    {
        Arrangement stress;
        stress.periodTicks=61440;
        for(int step=0;step<256;++step) {
            const int64_t begin=static_cast<int64_t>(step)*240;
            stress.midi.bass.push_back({begin,begin+228,36+(step%3),80+(step%4)*10,begin+228});
        }
        for(const int frames : {1,16,64,256,1024,4096}) {
            EbmSchedulerAdapter unit;
            int noteOns=0,noteOffs=0;
            std::array<bool,128> active{};
            auto collect=[&](const ScheduledMidiEvent& e) {
                assert(e.bus==1);
                assert(e.sampleOffset>=0 && e.sampleOffset<frames);
                assert(e.pitch>=0 && e.pitch<128);
                if(e.on) {
                    assert(!active[static_cast<unsigned>(e.pitch)]);
                    active[static_cast<unsigned>(e.pitch)]=true;
                    ++noteOns;
                } else {
                    assert(active[static_cast<unsigned>(e.pitch)]);
                    active[static_cast<unsigned>(e.pitch)]=false;
                    ++noteOffs;
                }
                return true;
            };
            // One-sixteenth note per block, no sample-rate assumptions.
            for(int step=0;step<256;++step) {
                const double start=static_cast<double>(step)/4.0;
                assert(unit.process(stress,start,start+0.25,frames,true,collect));
            }
            assert(noteOns==256 && noteOffs==256);
            for(const bool on:active) assert(!on);
            // Full cycle repeat must be stable across the 16-bar boundary.
            for(int step=0;step<256;++step) {
                const double start=64.0+static_cast<double>(step)/4.0;
                assert(unit.process(stress,start,start+0.25,frames,true,collect));
            }
            assert(noteOns==512 && noteOffs==512);
            assert(unit.process(stress,128.0,128.25,frames,false,collect));
        }
    }
    // Seek and backward loop must flush old notes.
    {
        Arrangement single;
        single.periodTicks=3840;
        single.midi.bass={{0,240,36,100,240}};
        EbmSchedulerAdapter unit;
        std::vector<ScheduledMidiEvent> recorded;
        auto collect=[&](const ScheduledMidiEvent& e) {recorded.push_back(e);return true;};
        assert(unit.process(single,0.0,0.1,256,true,collect));
        recorded.clear();
        single.midi.bass={{0,240,43,110,240}};
        assert(unit.process(single,0.1,0.2,256,false,collect));
        assert(recorded.size()==1 && !recorded[0].on && recorded[0].pitch==36);
        recorded.clear();
        assert(unit.process(single,0.0,0.1,256,true,collect));
        assert(recorded.size()==1 && recorded[0].on && recorded[0].pitch==43);
        recorded.clear();
        assert(unit.process(single,0.0,0.1,256,true,collect)); // backwards seek
        assert(recorded.size()==2);
        assert(!recorded[0].on && recorded[0].pitch==43);
        assert(recorded[1].on && recorded[1].pitch==43);
        recorded.clear();
        assert(unit.process(single,0.1,0.2,256,false,collect));
        assert(recorded.size()==1 && !recorded[0].on);
    }

}
