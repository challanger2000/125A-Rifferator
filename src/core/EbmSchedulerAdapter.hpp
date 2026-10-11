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
    void reset() noexcept { active_.fill(false); haveExpected_=false; }
    // Call before replacing a prepared arrangement while transport is running.
    // If the host rejects a NoteOff, retain state and retry on next callback.
    template<class Sink>
    bool patternChanged(Sink&& sink) noexcept {
        if(!flush(sink,0)) return false;
        haveExpected_=false;
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
    bool process(const Arrangement& arrangement,double startPpq,double endPpq,
                 int frames,bool playing,Sink&& sink) noexcept {
        if(frames<=0) return true;
        if(!playing || !std::isfinite(startPpq) || !std::isfinite(endPpq)
           || endPpq<=startPpq || arrangement.periodTicks<=0) {
            const bool ok=flush(sink);
            if(ok) haveExpected_=false;
            return ok;
        }
        if(haveExpected_ && std::abs(startPpq-expectedPpq_)>0.002)
            if(!flush(sink)) return false;
        // Avoid undefined integer conversions on corrupt host transport positions.
        if(std::abs(startPpq)>1.0e12 || std::abs(endPpq)>1.0e12) {
            flush(sink,0); haveExpected_=false; return false;
        }
        expectedPpq_=endPpq; haveExpected_=true;
        struct Event { int offset,pitch,velocity; bool on; double ppq; };
        std::array<Event,512> events{};
        int size=0;
        const double period=static_cast<double>(arrangement.periodTicks)/960.0;
        for(const auto& n:arrangement.midi.bass) {
            if(n.startTick<0 || n.endTick<=n.startTick || n.endTick>arrangement.periodTicks
               || n.note<0 || n.note>127 || n.velocity<1 || n.velocity>127)
                { flush(sink,0); return false; }
            const double phase=static_cast<double>(n.startTick)/960.0;
            const auto first=static_cast<int64_t>(std::floor((startPpq-phase)/period))-1;
            const auto last=static_cast<int64_t>(std::ceil((endPpq-phase)/period))+1;
            if(last<first || last-first>8) { flush(sink,0); return false; }
            for(int64_t cycle=first;cycle<=last;++cycle) {
                const double base=static_cast<double>(cycle)*period;
                auto add=[&](double at,bool on) noexcept {
                    if(at<startPpq || at>=endPpq) return true;
                    if(size==static_cast<int>(events.size())) return false;
                    const double fraction=(at-startPpq)/(endPpq-startPpq);
                    const int offset=std::clamp(static_cast<int>(fraction*frames),0,frames-1);
                    events[size++]={offset,n.note,on?n.velocity:0,on,at};
                    return true;
                };
                if(!add(base+static_cast<double>(n.endTick)/960.0,false) ||
                   !add(base+phase,true)) { flush(sink,0); return false; }
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
            if(!e.on && !active_[e.pitch]) continue;
            if(!sink(ScheduledMidiEvent{1,e.offset,e.pitch,e.velocity,-1,e.on}))
                { flush(sink,e.offset); return false; }
            active_[e.pitch]=e.on;
        }
        return true;
    }
private:
    std::array<bool,128> active_{};
    bool haveExpected_{};
    double expectedPpq_{};
};
} // namespace a125::rifferator::ebm_integration
