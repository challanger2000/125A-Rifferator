#pragma once
#include "core/BlockScheduler.hpp"
#include "core/RiffArrangement.hpp"
#include "core/DrumCompanion.hpp"
#include "core/ChordCompanion.hpp"
#include "core/EbmBassArrangement.hpp"
#include "core/EbmSchedulerAdapter.hpp"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include <vector>
#include <array>

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
  Steinberg::tresult PLUGIN_API setProcessing(Steinberg::TBool state) override;
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) override;
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) override;
private:
  Config config_{};
  std::vector<Step> steps_{};
  FillOptions fill_{};
  std::array<Arrangement,11> fillVariants_{};
  int fillIndex_{};
  void prepareFillVariants();
  DrumPattern drumPattern_{DrumPattern::standard()};
  std::vector<MidiNote> drumNotes_{};
  std::vector<MidiNote> chordNotes_{};
  BlockScheduler scheduler_{};
  std::array<Arrangement,5> ebmPrepared_{};
  ebm_integration::EbmSchedulerAdapter ebmScheduler_{};
  int bassMode_{};
  int activeBassMode_{};
  int ebmStyle_{};
  bool bassSwitchPending_{};
  bool flushOnNextProcess_{};
  bool drumFlushPending_{};
  bool chordFlushPending_{};
  bool guitarFlushPending_{};
  LiveControls live_{};
  int preparedRootMidi_{40};
};
}
