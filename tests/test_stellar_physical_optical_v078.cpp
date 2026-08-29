#include <cassert>
#include <cmath>
#include <iostream>

#include "stellar_physical_optical_v078.h"

namespace {

bool close(float left, float right, float tolerance = 1.0e-6f)
{
  return std::fabs(left - right) <= tolerance;
}

}  // namespace

int main()
{
  StellarFeatureSampleV064 features = {};
  features.merger_weight = 0.5f;
  features.disk_weight = 0.25f;
  features.polar_weight = 0.2f;
  assert(close(stellarPhysicalCompositeFeatureSupportV078(features), 0.7f));

  StellarTransferParameters geometry = {};
  geometry.disk_radius_cm = 3.0e10f;
  StellarPhysicalTransferV071 transfer = {};
  transfer.channel = STELLAR_PHYSICAL_CHANNEL_ROTATIONAL_FRACTION_V071;
  transfer.scale = STELLAR_PHYSICAL_SCALE_LINEAR_V071;
  transfer.range_min = 0.0f;
  transfer.range_max = 1.0f;
  StellarPhysicalOpticalParametersV076 optical = {};
  optical.profile = STELLAR_PHYSICAL_OPTICAL_COMPOSITE_MOMENT_V078;
  optical.target_optical_depth = 0.16f;
  optical.target_emission = 0.35f;
  optical.color_gamma = 2.25f;
  optical.density_support_log10_low = -9.0f;
  optical.density_support_log10_high = -6.0f;
  optical.emission_signal_floor = 0.5f;
  StellarPhysicalSampleV071 dense = {};
  dense.density_cgs = 1.0e-3f;

  StellarFeatureSampleV064 merger = {};
  merger.merger_weight = 0.6f;
  StellarFeatureSampleV064 disk = {};
  disk.disk_weight = 0.6f;
  StellarFeatureSampleV064 polar = {};
  polar.polar_weight = 0.6f;
  const StellarFeatureSampleV064 supported[] = {merger, disk, polar};
  for(const StellarFeatureSampleV064 &feature : supported) {
    const StellarOpticalSample sample =
        evaluateStellarPhysicalOpticalFromValueV078(
            0.4f, dense, feature, transfer, optical, geometry);
    assert(sample.extinction_per_cm > 0.0f);
    assert(sample.emissivity_rgb_per_cm[1] > 0.0f);
  }

  StellarFeatureSampleV064 absent = {};
  const StellarOpticalSample hidden =
      evaluateStellarPhysicalOpticalFromValueV078(
          0.4f, dense, absent, transfer, optical, geometry);
  assert(hidden.extinction_per_cm == 0.0f);
  assert(hidden.emissivity_rgb_per_cm[1] == 0.0f);

  const float moments[3] = {0.5f, 2.0f, 1.0f};
  float decoded[3];
  stellarDecodePhysicalMomentsV078(moments, optical, decoded);
  StellarPhysicalOpticalParametersV076 normalized = optical;
  normalized.profile = STELLAR_PHYSICAL_OPTICAL_NORMALIZED_MOMENT_V077;
  float expected[3];
  stellarDecodePhysicalMomentsV077(moments, normalized, expected);
  for(int component = 0; component < 3; component++)
    assert(close(decoded[component], expected[component]));

  assert(stellarPhysicalOpticalProfileFromNameV078("composite_moment_v078") ==
         STELLAR_PHYSICAL_OPTICAL_COMPOSITE_MOMENT_V078);
  assert(stellarPhysicalOpticalProfileFromNameV078("normalized_moment_v077") ==
         STELLAR_PHYSICAL_OPTICAL_NORMALIZED_MOMENT_V077);

  std::cout << "STELLAR_PHYSICAL_OPTICAL_V078_OK "
            << "support=density_x_merger_disk_polar_union "
            << "decode=normalized_moments"
            << std::endl;
  return 0;
}
