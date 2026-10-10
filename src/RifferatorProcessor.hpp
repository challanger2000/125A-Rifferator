#pragma once
#include "core/BlockScheduler.hpp"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
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
private:
  Config config_{};
  std::vector<Step> steps_{};
  Pair rendered_{};
  BlockScheduler scheduler_{};
  bool flushOnNextProcess_{};
};
}
