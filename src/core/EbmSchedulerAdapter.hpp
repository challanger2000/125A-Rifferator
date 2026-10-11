#pragma once
#include "RiffArrangement.hpp"
#include "BlockScheduler.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace a125::rifferator::ebm_integration {
// Standalone bass-only event adapter; does not alter shared guitar scheduler.
// Prepared Arrangement must remain immutable during process(). No allocations.
class EbmSchedulerAdapter {
public:
    // reset() is safe only before playback or after a successful flush.
    void reset() noexcept { active_.fill(false); haveExpected_=false; recoveryPending_=false; }
    // Call before replacing a prepared arrangement while transport is running.
    // If the host rejects a NoteOff, retain state and retry on next callback.
    template<class Sink>
    bool patternChanged(Sink&& sink) noexcept {
        if(!flush(sink,0)) { recoveryPending_=true; return false; }
        haveExpected_=false;
        recoveryPending_=false;
        return true;
    }
    template<class Sink>
    bool flush(Sink&& sink,int sampleOffset=0) noexcept {
        bool ok=true;
        for(int pitch=0;pitch<128;++pitch) if(active_[pitch]) {
            if(sink(ScheduledMidiEvent{1,sampleOffset,pitch,0,-1,false}))
                active_[pitch]=false;
            else ok=false;
        }
        return ok;
    }
    template<class Sink>
    bool failClosed(Sink&& sink,int offset=0) noexcept {
        recoveryPending_=true;
        flush(sink,offset);
        haveExpected_=false;
        return false;
    }
    template<class Sink>
    bool process(const Arrangement& arrangement,double startPpq,double endPpq,
                 int frames,bool playing,Sink&& sink,
                 int transpose=0,int gatePercent=100) noexcept {
        transpose=std::clamp(transpose,-127,127);
        gatePercent=std::clamp(gatePercent,0,100);
        if(frames<=0) return true;
        // A gate of zero must release notes that were already sounding.
        // Otherwise skipping note events suppresses their NoteOffs as well.
        if(gatePercent==0) {
            const bool ok=flush(sink,0);
            if(ok)haveExpected_=false;
            else recoveryPending_=true;
            return ok;
        }
        if(recoveryPending_) {
            if(!flush(sink,0)) return false;
            recoveryPending_=false;
            haveExpected_=false;
            return false; // Recovery callback emits no new NoteOns.
        }
        if(!playing || !std::isfinite(startPpq) || !std::isfinite(endPpq)
           || endPpq<=startPpq || arrangement.periodTicks<=0) {
            const bool ok=flush(sink);
            if(ok) haveExpected_=false;
            else recoveryPending_=true;
            return ok;
        }
        if(haveExpected_ && std::abs(startPpq-expectedPpq_)>0.002)
            if(!flush(sink)) { recoveryPending_=true; return false; }
        // Avoid undefined integer conversions on corrupt host transport positions.
        if(std::abs(startPpq)>1.0e12 || std::abs(endPpq)>1.0e12) {
            return failClosed(sink,0);
        }
        expectedPpq_=endPpq; haveExpected_=true;
        struct Event { int offset,pitch,velocity; bool on; double ppq; };
        std::array<Event,512> events{};
        int size=0;
        const double period=static_cast<double>(arrangement.periodTicks)/960.0;
        for(const auto& n:arrangement.midi.bass) {
            if(n.startTick<0 || n.endTick<=n.startTick || n.endTick>arrangement.periodTicks
               || n.note<0 || n.note>127 || n.velocity<1 || n.velocity>127)
                { return failClosed(sink,0); }
            if(gatePercent==0)continue;
            const int outPitch=std::clamp(n.note+transpose,0,127);
            const auto duration=n.endTick-n.startTick;
            const auto gatedEnd=n.startTick+std::max<int64_t>(1,duration*gatePercent/100);
            const double phase=static_cast<double>(n.startTick)/960.0;
            const auto first=static_cast<int64_t>(std::floor((startPpq-phase)/period))-1;
            const auto last=static_cast<int64_t>(std::ceil((endPpq-phase)/period))+1;
            if(last<first || last-first>8) return failClosed(sink,0);
            for(int64_t cycle=first;cycle<=last;++cycle) {
                const double base=static_cast<double>(cycle)*period;
                auto add=[&](double at,bool on) noexcept {
                    if(at<startPpq || at>=endPpq) return true;
                    if(size==static_cast<int>(events.size())) return false;
                    const double fraction=(at-startPpq)/(endPpq-startPpq);
                    const int offset=std::clamp(static_cast<int>(fraction*frames),0,frames-1);
                    events[size++]={offset,outPitch,on?n.velocity:0,on,at};
                    return true;
                };
                if(!add(base+static_cast<double>(gatedEnd)/960.0,false) ||
                   !add(base+phase,true)) return failClosed(sink,0);
            }
        }
        // Small fixed buffer; off events always before simultaneous ons.
        for(int i=1;i<size;++i) {
            const Event key=events[i];
            int j=i-1;
            while(j>=0 && (events[j].offset>key.offset ||
                (events[j].offset==key.offset &&
                 (events[j].ppq>key.ppq ||
                  (events[j].ppq==key.ppq && events[j].on && !key.on))))) {
                events[j+1]=events[j];--j;
            }
            events[j+1]=key;
        }
        for(int i=0;i<size;++i) {
            const auto& e=events[i];
            // One MIDI note per pitch is tracked: overlapping same-pitch NoteOns
            // cannot be represented safely and must never be silently accepted.
            if(e.on && active_[e.pitch]) return failClosed(sink,e.offset);
            if(!e.on && !active_[e.pitch]) continue;
            if(!sink(ScheduledMidiEvent{1,e.offset,e.pitch,e.velocity,-1,e.on}))
                { recoveryPending_=true; flush(sink,e.offset); return false; }
            active_[e.pitch]=e.on;
        }
        return true;
    }
private:
    std::array<bool,128> active_{};
    bool haveExpected_{};
    bool recoveryPending_{};
    double expectedPpq_{};
};
} // namespace a125::rifferator::ebm_integration
