#include "../src/core/GuitarStepControls.hpp"
#include <cassert>
using namespace a125::rifferator;
int main() {
 auto g=GuitarStepControls::initial();
 assert(g.steps[0].articulation==Articulation::PalmMute);
 assert(g.steps[1].articulation==Articulation::Off);
 assert(g.setArticulation(0,Articulation::Open));
 assert(g.steps[0].articulation==Articulation::Open);
 assert(g.setArticulation(0,Articulation::Harmonic));
 assert(g.steps[0].articulation==Articulation::Harmonic);
 assert(!g.setArticulation(0,static_cast<Articulation>(12)));
 assert(g.steps[0].articulation==Articulation::Harmonic);
 assert(g.setDegree(0,-13));
 assert(g.setDegree(0,15));
 assert(!g.setDegree(0,16));
 assert(g.setDoubled(0,true));
 g.setLock(0,true);
 const auto saved=g.steps[0];
 assert(!g.setArticulation(0,Articulation::Off));
 assert(!g.setDegree(0,1));
 assert(!g.setDoubled(0,false));
 assert(g.steps[0].articulation==saved.articulation);
 assert(g.steps[0].degree==saved.degree);
 assert(g.steps[0].doubled==saved.doubled);
 assert(!g.setArticulation(-1,Articulation::Open));
 assert(!g.setDegree(16,1));
 g.setLock(0,false);
 assert(g.setArticulation(0,Articulation::Off));
 assert(g.steps[0].articulation==Articulation::Off);
}
