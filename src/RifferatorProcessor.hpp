#pragma once
#include "core/BlockScheduler.hpp"
#include "core/RiffArrangement.hpp"
#include "RifferatorState.hpp"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include <array>
#include <vector>

namespace a125::rifferator {
class Processor final : public Steinberg::Vst::AudioEffect {
public:
  Processor();
  static Steinberg::FUnknown* createInstance(void*);
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) override;
  Steinberg::uint32 PLUGIN_API getProcessContextRequirements() override;
  Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32) override;
  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) override;
  Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool) override;
  Steinberg::tresult PLUGIN_API setProcessing(Steinberg::TBool) override;
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) override;
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) override;
private:
  GuitarSnapshot state_{};
  Config config_{};
  FillOptions fill_{};
  std::array<Arrangement,11> variants_{};
  GuitarLive live_{};
  BlockScheduler scheduler_{};
  int fillIndex_{};
  bool flushPending_{};
  void prepareVariants();
};
} // namespace a125::rifferator
