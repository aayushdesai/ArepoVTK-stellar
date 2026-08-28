#include <cassert>
#include <cmath>
#include <iostream>

#include "../src/stellar_physical_channel_v072.h"

int main()
{
  assert(stellarPhysicalChannelFromNameV072("magnetic_pressure") ==
         STELLAR_PHYSICAL_CHANNEL_MAGNETIC_PRESSURE_V072);
  assert(stellarPhysicalChannelFromNameV072("mach_number") ==
         STELLAR_PHYSICAL_CHANNEL_MACH_NUMBER_V072);
  assert(stellarPhysicalChannelFromNameV072("rotational_fraction") ==
         STELLAR_PHYSICAL_CHANNEL_ROTATIONAL_FRACTION_V071);

  StellarTransferParameters geometry = {};
  geometry.axis[2] = 1.0;
  geometry.box_size = 1.0e14;
  const double position[3] = {2.0e10, 0.0, 1.0e10};
  const float velocity[3] = {1.0e7f, 2.0e8f, 3.0e7f};
  const StellarPhysicalSampleV071 base = evaluateStellarPhysicalSampleV071(
      geometry, position, 8.0f, 2.0e7f, velocity, 4.0f, 6.0f);
  StellarAuxiliaryFieldsV072 auxiliary = {};
  auxiliary.magnetic_field_gauss[0] = 3.0f;
  auxiliary.magnetic_field_gauss[1] = 4.0f;
  auxiliary.pressure_dyn_cm2 = 25.0f;
  auxiliary.sound_speed_cm_per_s = 1.0e8f;
  const StellarExtendedPhysicalSampleV072 extended =
      evaluateStellarExtendedPhysicalSampleV072(
          geometry, position, velocity, base, auxiliary);
  assert(fabsf(extended.magnetic_field_strength_gauss - 5.0f) < 1.0e-5f);
  assert(extended.magnetic_pressure_dyn_cm2 > 0.0f);
  assert(extended.alfven_speed_cm_per_s > 0.0f);
  assert(extended.gas_pressure_dyn_cm2 == 25.0f);
  assert(extended.sound_speed_cm_per_s == 1.0e8f);
  assert(extended.mach_number > 1.0f);
  assert(extended.entropy_proxy_cgs > 0.0f);

  StellarPhysicalTransferV071 transfer = {};
  transfer.channel = STELLAR_PHYSICAL_CHANNEL_MAGNETIC_PRESSURE_V072;
  transfer.scale = STELLAR_PHYSICAL_SCALE_LOG10_V071;
  transfer.range_min = -2.0f;
  transfer.range_max = 2.0f;
  transfer.symlog_linthresh = 0.01f;
  transfer.extinction_per_cm = 1.0e-11f;
  transfer.emissivity_per_cm = 1.0e-11f;
  const StellarOpticalSample optical = evaluateStellarPhysicalOpticalV072(
      base, extended, transfer);
  assert(optical.extinction_per_cm > 0.0f);
  assert(optical.emissivity_rgb_per_cm[2] > 0.0f);

  std::cout << "STELLAR_PHYSICAL_CHANNEL_V072_OK channels=24 "
               "palette=copper_blue auxiliary_units=gauss,dyn_cm2,cm_s"
            << std::endl;
  return 0;
}
