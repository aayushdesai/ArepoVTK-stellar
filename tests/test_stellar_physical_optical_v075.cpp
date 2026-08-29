#include <cassert>
#include <cmath>
#include <iostream>

#include "stellar_physical_optical_v075.h"

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

  StellarFeatureSampleV064 disk = {};
  disk.disk_weight = 0.8f;
  StellarPhysicalOpticalParametersV075 separated = {};
  separated.profile = STELLAR_PHYSICAL_OPTICAL_SEPARATED_SUPPORT_V075;
  separated.target_optical_depth = 1.0f;
  separated.target_emission = 1.0f;
  separated.opacity_signal_threshold = 0.02f;
  separated.color_gamma = 2.25f;

  const StellarOpticalSample zero = evaluateStellarPhysicalOpticalFromValueV075(
      0.0f, disk, transfer, separated, geometry);
  assert(zero.extinction_per_cm == 0.0f);
  assert(zero.emissivity_rgb_per_cm[0] == 0.0f);

  const StellarOpticalSample low = evaluateStellarPhysicalOpticalFromValueV075(
      0.01f, disk, transfer, separated, geometry);
  const StellarOpticalSample high = evaluateStellarPhysicalOpticalFromValueV075(
      0.8f, disk, transfer, separated, geometry);
  const float path = 2.0f * geometry.disk_radius_cm;
  assert(low.extinction_per_cm > 0.0f);
  assert(low.extinction_per_cm < 0.8f / path);
  assert(std::fabs(high.extinction_per_cm - 0.8f / path) < 1.0e-18f);
  float high_color[3];
  stellarCopperBlueV071(
      stellarPhysicalStyledFractionV075(0.8f, 2.25f, 0), high_color);
  assert(std::fabs(high.emissivity_rgb_per_cm[0] -
      0.8f * 0.8f / path * high_color[0]) < 1.0e-18f);

  const StellarFeatureSampleV064 irrelevant = {};
  const StellarOpticalSample unsupported =
      evaluateStellarPhysicalOpticalFromValueV075(
          0.8f, irrelevant, transfer, separated, geometry);
  assert(unsupported.extinction_per_cm == 0.0f);

  assert(std::fabs(stellarPhysicalStyledFractionV075(0.36f, 2.0f, 0) -
                   0.6f) < 1.0e-6f);
  assert(std::fabs(stellarPhysicalStyledFractionV075(0.36f, 2.0f, 1) -
                   0.4f) < 1.0e-6f);
  assert(stellarPhysicalOpticalProfileFromNameV075("legacy_v072") ==
         STELLAR_PHYSICAL_OPTICAL_LEGACY_V072);
  assert(stellarPhysicalOpticalProfileFromNameV075("material_support_v074") ==
         STELLAR_PHYSICAL_OPTICAL_MATERIAL_SUPPORT_V074);
  assert(stellarPhysicalOpticalProfileFromNameV075("separated_support_v075") ==
         STELLAR_PHYSICAL_OPTICAL_SEPARATED_SUPPORT_V075);

  std::cout << "STELLAR_PHYSICAL_OPTICAL_V075_OK "
            << "scalar_color=separate support=feature_weighted "
            << "opacity=thresholded emission=signal_weighted" << std::endl;
  return 0;
}
