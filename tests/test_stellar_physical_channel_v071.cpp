#include <cassert>
#include <cmath>
#include <iostream>

#include "../src/stellar_physical_channel_v071.h"

int main()
{
  assert(stellarPhysicalChannelFromNameV071("rotational_fraction") ==
         STELLAR_PHYSICAL_CHANNEL_ROTATIONAL_FRACTION_V071);
  assert(stellarPhysicalChannelFromNameV071("not_a_channel") ==
         STELLAR_PHYSICAL_CHANNEL_INVALID_V071);
  assert(stellarPhysicalScaleFromNameV071("symlog") ==
         STELLAR_PHYSICAL_SCALE_SYMLOG_V071);

  StellarTransferParameters geometry = {};
  geometry.center[0] = geometry.center[1] = geometry.center[2] = 0.0;
  geometry.axis[2] = 1.0;
  geometry.box_size = 1.0e14;
  const double position[3] = {2.0e10, 0.0, 1.0e10};
  const float velocity[3] = {1.0e7f, 2.0e8f, 3.0e7f};
  const StellarPhysicalSampleV071 sample = evaluateStellarPhysicalSampleV071(
      geometry, position, 8.0f, 2.0e7f, velocity, 4.0f, 6.0f);
  assert(std::isfinite(sample.rotational_fraction));
  assert(sample.rotational_fraction > 0.9f);
  assert(sample.azimuthal_velocity_cm_per_s > 0.0f);
  assert(sample.outward_axial_velocity_cm_per_s > 0.0f);
  assert(sample.density_cgs > 0.0f);

  StellarPhysicalTransferV071 transfer = {};
  transfer.channel = STELLAR_PHYSICAL_CHANNEL_ROTATIONAL_FRACTION_V071;
  transfer.scale = STELLAR_PHYSICAL_SCALE_LINEAR_V071;
  transfer.range_min = 0.0f;
  transfer.range_max = 1.0f;
  transfer.symlog_linthresh = 0.01f;
  transfer.extinction_per_cm = 1.0e-11f;
  transfer.emissivity_per_cm = 2.0e-11f;
  const StellarOpticalSample optical =
      evaluateStellarPhysicalOpticalV071(sample, transfer);
  assert(optical.extinction_per_cm > 0.0f);
  assert(optical.emissivity_rgb_per_cm[0] > 0.0f);
  assert(optical.emissivity_rgb_per_cm[1] > 0.0f);
  assert(optical.emissivity_rgb_per_cm[2] > 0.0f);

  StellarPhysicalSampleV071 signed_sample = sample;
  signed_sample.radial_velocity_cm_per_s = -1.0e8f;
  transfer.channel = STELLAR_PHYSICAL_CHANNEL_RADIAL_VELOCITY_V071;
  transfer.scale = STELLAR_PHYSICAL_SCALE_SYMLOG_V071;
  transfer.range_min = -4.0f;
  transfer.range_max = 4.0f;
  transfer.symlog_linthresh = 1.0e4f;
  const float transformed = stellarPhysicalTransformV071(
      stellarPhysicalValueV071(signed_sample, transfer.channel), transfer);
  assert(transformed < 0.0f);

  std::cout << "STELLAR_PHYSICAL_CHANNEL_V071_OK channels=13 palette=copper_blue"
            << std::endl;
  return 0;
}
