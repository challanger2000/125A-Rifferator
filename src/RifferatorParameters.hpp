#pragma once
#include "pluginterfaces/vst/vsttypes.h"
namespace a125::rifferator {
enum : Steinberg::Vst::ParamID {
  kGuitarGate=100, kBassGate=101, kBassEnabled=102,
  kRootPitch=103, kBassVelocity=104, kFillIntensity=105
};
}
