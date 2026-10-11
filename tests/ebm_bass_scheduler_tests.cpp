#include "../src/core/EbmSchedulerAdapter.hpp"
#include <cassert>
#include <vector>
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
}
