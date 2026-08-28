#include <cassert>
#include <cmath>
#include <iostream>

#include "stellar_gpu_scene_format_v073.h"
#include "stellar_physical_channel_v072.h"

int main()
{
  assert(sizeof(ArepoStellarSceneHeaderV073) == 208);
  assert(sizeof(ArepoStellarCellV073) == 72);
  assert(stellarPhysicalChannelFromNameV072("density") == 0);
  assert(stellarPhysicalChannelFromNameV072("rotational_fraction") == 5);
  assert(stellarPhysicalChannelFromNameV072("magnetic_field_strength") == 12);
  assert(stellarPhysicalChannelFromNameV072("mach_number") == 23);
  assert(stellarPhysicalChannelFromNameV072("not_a_channel") ==
         STELLAR_PHYSICAL_CHANNEL_INVALID_V071);

  StellarTransferParameters geometry = {};
  geometry.center[0] = geometry.center[1] = geometry.center[2] = 0.0;
  geometry.axis[2] = 1.0;
  geometry.box_size = 1.0e12;
  const double position[3] = {2.0e10, 0.0, 1.0e10};
  const float velocity[3] = {0.0f, 3.0e8f, 4.0e8f};
  const StellarPhysicalSampleV071 base = evaluateStellarPhysicalSampleV071(
      geometry, position, 7.0f, 2.0e7f, velocity, 0.0f, 0.0f);
  assert(std::fabs(base.rotational_fraction - 0.6f) < 1.0e-6f);

  StellarAuxiliaryFieldsV072 auxiliary = {};
  auxiliary.magnetic_field_gauss[1] = 10.0f;
  auxiliary.pressure_dyn_cm2 = 100.0f;
  auxiliary.sound_speed_cm_per_s = 1.0e8f;
  const StellarExtendedPhysicalSampleV072 extended =
      evaluateStellarExtendedPhysicalSampleV072(
          geometry, position, velocity, base, auxiliary);
  assert(std::fabs(extended.magnetic_field_strength_gauss - 10.0f) < 1.0e-6f);
  assert(std::fabs(extended.mach_number - 5.0f) < 1.0e-6f);
  assert(extended.magnetic_pressure_dyn_cm2 > 0.0f);
  assert(extended.plasma_beta > 0.0f);

  StellarPhysicalTransferV071 transfer = {};
  transfer.channel = STELLAR_PHYSICAL_CHANNEL_ROTATIONAL_FRACTION_V071;
  transfer.scale = STELLAR_PHYSICAL_SCALE_LINEAR_V071;
  transfer.range_min = 0.0f;
  transfer.range_max = 1.0f;
  transfer.symlog_linthresh = 1.0f;
  transfer.extinction_per_cm = 1.0e-11f;
  transfer.emissivity_per_cm = 1.0e-11f;
  const StellarOpticalSample optical =
      evaluateStellarPhysicalOpticalV071(base, transfer);
  assert(optical.extinction_per_cm > 0.0f);
  assert(optical.emissivity_rgb_per_cm[0] > 0.0f);

  std::cout << "STELLAR_GPU_PHYSICAL_CONTRACT_V053D_OK channels=24 "
            << "scene=v073 palette=copper_blue" << std::endl;
  return 0;
}
