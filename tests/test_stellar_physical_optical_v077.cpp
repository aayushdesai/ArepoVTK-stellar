#include <cassert>
#include <cmath>
#include <iostream>

#include "stellar_physical_optical_v077.h"

namespace {

bool close(float left, float right, float tolerance = 1.0e-6f)
{
  return std::fabs(left - right) <= tolerance;
}

}  // namespace

int main()
{
  StellarPhysicalOpticalParametersV076 optical = {};
  optical.profile = STELLAR_PHYSICAL_OPTICAL_NORMALIZED_MOMENT_V077;
  optical.target_optical_depth = 0.2f;
  optical.target_emission = 1.5f;
  optical.color_gamma = 2.25f;
  optical.density_support_log10_low = -8.0f;
  optical.density_support_log10_high = -5.0f;
  optical.emission_signal_floor = 0.2f;

  StellarTransferParameters geometry = {};
  geometry.disk_radius_cm = 3.0e10f;
  StellarPhysicalTransferV071 transfer = {};
  transfer.channel = STELLAR_PHYSICAL_CHANNEL_ROTATIONAL_FRACTION_V071;
  transfer.scale = STELLAR_PHYSICAL_SCALE_LINEAR_V071;
  transfer.range_min = 0.0f;
  transfer.range_max = 1.0f;
  StellarPhysicalSampleV071 physical = {};
  physical.density_cgs = 1.0e-3f;
  StellarFeatureSampleV064 feature = {};
  const StellarOpticalSample through_v077 =
      evaluateStellarPhysicalOpticalFromValueV077(
          0.4f, physical, feature, transfer, optical, geometry);
  StellarPhysicalOpticalParametersV076 v076 = optical;
  v076.profile = STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076;
  const StellarOpticalSample direct_v076 =
      evaluateStellarPhysicalOpticalFromValueV076(
          0.4f, physical, feature, transfer, v076, geometry);
  assert(through_v077.extinction_per_cm == direct_v076.extinction_per_cm);
  for(int component = 0; component < 3; component++)
    assert(through_v077.emissivity_rgb_per_cm[component] ==
           direct_v076.emissivity_rgb_per_cm[component]);

  // A saturated ray has denominator E/tau. V077 recovers E and then applies
  // the normalized mean amplitude, instead of exposing E/tau as intensity.
  const float saturated[3] = {1.875f, 7.5f, 3.0f};
  float decoded[3];
  stellarDecodePhysicalMomentsV077(saturated, optical, decoded);
  float color[3];
  stellarCopperBlueV071(0.25f, color);
  for(int component = 0; component < 3; component++)
    assert(close(decoded[component], 0.6f * color[component]));

  // A half-covered ray remains half as bright while retaining identical hue
  // and amplitude. This is the intended path-length response.
  const float half_covered[3] = {0.9375f, 3.75f, 1.5f};
  float half_decoded[3];
  stellarDecodePhysicalMomentsV077(half_covered, optical, half_decoded);
  for(int component = 0; component < 3; component++)
    assert(close(half_decoded[component], 0.5f * decoded[component]));

  // Numerically excessive line integrals cannot exceed target emission.
  const float excessive[3] = {18.75f, 75.0f, 30.0f};
  float bounded[3];
  stellarDecodePhysicalMomentsV077(excessive, optical, bounded);
  for(int component = 0; component < 3; component++)
    assert(close(bounded[component], decoded[component]));

  const float absent[3] = {0.0f, 0.0f, 0.0f};
  float zero[3];
  stellarDecodePhysicalMomentsV077(absent, optical, zero);
  assert(zero[0] == 0.0f && zero[1] == 0.0f && zero[2] == 0.0f);

  assert(stellarPhysicalOpticalProfileFromNameV077("normalized_moment_v077") ==
         STELLAR_PHYSICAL_OPTICAL_NORMALIZED_MOMENT_V077);
  assert(stellarPhysicalOpticalProfileFromNameV077("density_moment_v076") ==
         STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076);

  std::cout << "STELLAR_PHYSICAL_OPTICAL_V077_OK "
            << "scalar=normalized amplitude=normalized coverage=bounded"
            << std::endl;
  return 0;
}
