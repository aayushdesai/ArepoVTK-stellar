#include <cassert>
#include <cmath>
#include <iostream>

#include "stellar_physical_optical_v076.h"

int main()
{
  StellarTransferParameters geometry = {};
  geometry.material_radius_cm = 1.2e10f;
  geometry.disk_radius_cm = 3.0e10f;
  geometry.polar_outer_cm = 5.0e11f;

  StellarPhysicalTransferV071 transfer = {};
  transfer.channel = STELLAR_PHYSICAL_CHANNEL_ROTATIONAL_FRACTION_V071;
  transfer.scale = STELLAR_PHYSICAL_SCALE_LINEAR_V071;
  transfer.range_min = 0.0f;
  transfer.range_max = 1.0f;
  transfer.symlog_linthresh = 0.1f;

  StellarPhysicalOpticalParametersV076 moment = {};
  moment.profile = STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076;
  moment.target_optical_depth = 0.3f;
  moment.target_emission = 1.0f;
  moment.color_gamma = 1.0f;
  moment.density_support_log10_low = -9.0f;
  moment.density_support_log10_high = -5.0f;
  moment.emission_signal_floor = 0.25f;

  StellarPhysicalSampleV071 dense = {};
  dense.density_cgs = 1.0e-3f;
  StellarFeatureSampleV064 irrelevant = {};
  const StellarOpticalSample zero_rotation =
      evaluateStellarPhysicalOpticalFromValueV076(
          0.0f, dense, irrelevant, transfer, moment, geometry);
  assert(zero_rotation.extinction_per_cm > 0.0f);
  assert(zero_rotation.emissivity_rgb_per_cm[1] > 0.0f);
  assert(zero_rotation.emissivity_rgb_per_cm[2] > 0.0f);
  assert(std::fabs(zero_rotation.emissivity_rgb_per_cm[2] /
                   zero_rotation.emissivity_rgb_per_cm[1] - 0.25f) < 1.0e-6f);

  StellarPhysicalSampleV071 ambient = {};
  ambient.density_cgs = 1.0e-12f;
  const StellarOpticalSample hidden =
      evaluateStellarPhysicalOpticalFromValueV076(
          0.8f, ambient, irrelevant, transfer, moment, geometry);
  assert(hidden.extinction_per_cm == 0.0f);
  assert(hidden.emissivity_rgb_per_cm[1] == 0.0f);

  // Equal blue and copper samples decode from their scalar mean, rather than
  // averaging their RGB triplets into a desaturated beige sheet.
  const float accumulated[3] = {1.0f, 2.0f, 1.4f};
  float decoded[3];
  stellarDecodePhysicalMomentsV076(
      accumulated, STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076, decoded);
  float expected_color[3];
  stellarCopperBlueV071(0.5f, expected_color);
  for(int component = 0; component < 3; component++)
    assert(std::fabs(decoded[component] -
                     1.4f * expected_color[component]) < 1.0e-6f);

  const float rgb[3] = {0.2f, 0.3f, 0.4f};
  stellarDecodePhysicalMomentsV076(
      rgb, STELLAR_PHYSICAL_OPTICAL_SEPARATED_SUPPORT_V075, decoded);
  for(int component = 0; component < 3; component++)
    assert(decoded[component] == rgb[component]);

  StellarPhysicalOpticalParametersV075 retained_v075 = {};
  retained_v075.profile = STELLAR_PHYSICAL_OPTICAL_SEPARATED_SUPPORT_V075;
  retained_v075.target_optical_depth = 0.7f;
  retained_v075.target_emission = 0.9f;
  retained_v075.opacity_signal_threshold = 0.03f;
  retained_v075.color_gamma = 1.8f;
  StellarPhysicalOpticalParametersV076 delegated = {};
  delegated.profile = retained_v075.profile;
  delegated.target_optical_depth = retained_v075.target_optical_depth;
  delegated.target_emission = retained_v075.target_emission;
  delegated.opacity_signal_threshold = retained_v075.opacity_signal_threshold;
  delegated.color_gamma = retained_v075.color_gamma;
  StellarFeatureSampleV064 disk = {};
  disk.disk_weight = 0.6f;
  const StellarOpticalSample direct = evaluateStellarPhysicalOpticalFromValueV075(
      0.65f, disk, transfer, retained_v075, geometry);
  const StellarOpticalSample through_v076 =
      evaluateStellarPhysicalOpticalFromValueV076(
          0.65f, dense, disk, transfer, delegated, geometry);
  assert(direct.extinction_per_cm == through_v076.extinction_per_cm);
  for(int component = 0; component < 3; component++)
    assert(direct.emissivity_rgb_per_cm[component] ==
           through_v076.emissivity_rgb_per_cm[component]);

  assert(stellarPhysicalOpticalProfileFromNameV076("density_moment_v076") ==
         STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076);
  assert(stellarPhysicalOpticalProfileFromNameV076("separated_support_v075") ==
         STELLAR_PHYSICAL_OPTICAL_SEPARATED_SUPPORT_V075);

  std::cout << "STELLAR_PHYSICAL_OPTICAL_V076_OK "
            << "support=density scalar_color=post_ray_moment "
            << "zero_rotation=visible" << std::endl;
  return 0;
}
