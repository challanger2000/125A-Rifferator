#pragma once
#include "pluginterfaces/vst/vsttypes.h"
namespace a125::rifferator {
enum : Steinberg::Vst::ParamID {
  kGuitarGate=100, kBassGate=101, kBassEnabled=102,
  kRootPitch=103, kBassVelocity=104, kFillIntensity=105,
  kDrumEnabled=106, kBassMode=107, kEbmStyle=108, kChordsEnabled=109, kChordsMode=110, kChordsQuality=111, kHybridRootBase=600, kGuitarStepBase=300, kGuitarArticulationBase=400, kGuitarDegreeBase=416, kGuitarLockBase=432, kGuitarDoubleBase=448, kDrumStepBase=200 // 200..215 Kick, 216..231 Snare, 232..247 Hat
};
}
