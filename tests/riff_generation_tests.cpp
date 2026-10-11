#include "../src/core/RiffGeneration.hpp"
#include <cassert>
using namespace a125::rifferator;
static bool same(const Step&a,const Step&b) {
  return a.articulation==b.articulation&&a.degree==b.degree&&
    a.doubled==b.doubled&&a.locked==b.locked;
}
int main() {
  std::vector<Step> guitar(32,Step{Articulation::PalmMute,1,false,false});
  guitar[5]={Articulation::Open,4,true,true};
  RandomOptions go;go.seed=12345;go.randomPitch=true;
  for(auto command:{GenerationCommand::Generate,GenerationCommand::Variate}) {
    const auto a=generateGuitar(guitar,go,command,45);
    const auto b=generateGuitar(guitar,go,command,45);
    assert(a.size()==guitar.size());
    for(std::size_t i=0;i<a.size();++i)assert(same(a[i],b[i]));
    assert(same(a[5],guitar[5]));
  }
  auto none=generateGuitar(guitar,go,GenerationCommand::Variate,0);
  for(std::size_t i=0;i<none.size();++i)assert(same(none[i],guitar[i]));
  DrumPattern initial=DrumPattern::standard();
  DrumLocks locks{};
  locks.kick[0]=true;locks.snare[4]=true;locks.hat[9]=true;
  DrumGenerationOptions d;d.seed=87654;
  for(auto command:{GenerationCommand::Generate,GenerationCommand::Variate}) {
    const auto a=generateDrums(initial,locks,d,command);
    const auto b=generateDrums(initial,locks,d,command);
    assert(a.kick==b.kick && a.snare==b.snare && a.hat==b.hat);
    assert(a.kick[0]==initial.kick[0]);
    assert(a.snare[4]==initial.snare[4]);
    assert(a.hat[9]==initial.hat[9]);
    assert(a.enabled==initial.enabled);
  }
  d.variationPercent=0;
  const auto zero=generateDrums(initial,locks,d,GenerationCommand::Variate);
  assert(zero.kick==initial.kick && zero.snare==initial.snare &&
         zero.hat==initial.hat);
  // All-locked is an absolute no-op even on Generate.
  DrumLocks all{};
  all.kick.fill(true);all.snare.fill(true);all.hat.fill(true);
  const auto unchanged=generateDrums(initial,all,d,GenerationCommand::Generate);
  assert(unchanged.kick==initial.kick && unchanged.snare==initial.snare &&
         unchanged.hat==initial.hat);
}
