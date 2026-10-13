#pragma once
#include "pluginterfaces/vst/vsttypes.h"
namespace a125::rifferator {
enum : Steinberg::Vst::ParamID {
  // Persist the historical guitar IDs: projects and host automation depend on them.
  kGuitarGate=100, kRootPitch=103, kFillIntensity=105,
  kPmMin=112, kPmMax=113,
  kGuitarStepBase=300, kGuitarArticulationBase=400,
  kGuitarDegreeBase=416, kGuitarLockBase=432, kGuitarDoubleBase=448,
  kGuitarLengthBase=464 // 0 = 1/16; 1 = 1/8
};
}
