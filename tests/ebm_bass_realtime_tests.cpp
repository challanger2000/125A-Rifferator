#include "../src/core/EbmBassArrangement.hpp"
#include "../src/core/EbmSchedulerAdapter.hpp"
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <new>
#include <cstddef>
static std::atomic<bool> monitor{false};
static std::atomic<unsigned> allocated{0};
void* operator new(std::size_t n) {
    if(monitor.load(std::memory_order_relaxed)) allocated.fetch_add(1,std::memory_order_relaxed);
    if(void* p=std::malloc(n?n:1)) return p;
    throw std::bad_alloc{};
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p,std::size_t) noexcept { std::free(p); }
void* operator new[](std::size_t n) {
    if(monitor.load(std::memory_order_relaxed)) allocated.fetch_add(1,std::memory_order_relaxed);
    if(void* p=std::malloc(n?n:1)) return p;
    throw std::bad_alloc{};
}
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p,std::size_t) noexcept { std::free(p); }
using namespace a125::rifferator;
using namespace a125::rifferator::ebm_integration;
int main() {
    BassPlan plan;
    plan.mode=BassMode::Ebm;
    plan.sequence.style=ebm::Style::Chromatic;
    plan.sequence.gatePermille=850;
    plan.sequence.rhythmVariation=80;
    plan.arrangementEvolution=95;
    plan.fillIntensity=75;
    Arrangement snapshot;
    prepare(snapshot,plan); // Off audio thread; may allocate.
    EbmSchedulerAdapter scheduler;
    unsigned ons=0,offs=0;
    auto sink=[&](const ScheduledMidiEvent& e) noexcept {
        if(e.on) ++ons; else ++offs;
        return true;
    };
    monitor.store(true,std::memory_order_relaxed);
    bool ok=true;
    for(int cycle=0;cycle<4;++cycle) {
        for(int step=0;step<256;++step) {
            const double start=cycle*64.0+step*0.25;
            if(!scheduler.process(snapshot,start,start+0.25,64,true,sink)) ok=false;
        }
    }
    if(!scheduler.process(snapshot,256.0,256.25,64,false,sink)) ok=false;
    monitor.store(false,std::memory_order_relaxed);
    assert(ok);
    assert(allocated.load(std::memory_order_relaxed)==0);
    assert(ons==offs);
    assert(ons==4*snapshot.midi.bass.size());
    // This test measures adapter/sink allocations only. It does not verify
    // host VST3 allocation behavior, scheduling time, or snapshot publication.
}
