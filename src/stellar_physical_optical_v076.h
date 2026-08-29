#ifndef AREPO_VTK_STELLAR_PHYSICAL_OPTICAL_V076_H
#define AREPO_VTK_STELLAR_PHYSICAL_OPTICAL_V076_H

#include <cmath>
#include <string>

#include "stellar_physical_optical_v075.h"

#ifdef __CUDACC__
#define STELLAR_PHYSICAL_OPTICAL_V076_HD __host__ __device__
#else
#define STELLAR_PHYSICAL_OPTICAL_V076_HD
#endif

enum StellarPhysicalOpticalProfileV076 {
  STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076 = 3
};

struct StellarPhysicalOpticalParametersV076 {
  int profile;
  float target_optical_depth;
  float target_emission;
  float reference_path_cm;
  float opacity_signal_threshold;
  float color_gamma;
  int color_invert;
  float density_support_log10_low;
  float density_support_log10_high;
  float emission_signal_floor;
};

inline int stellarPhysicalOpticalProfileFromNameV076(const std::string &name)
{
  const int retained = stellarPhysicalOpticalProfileFromNameV075(name);
  if(retained != STELLAR_PHYSICAL_OPTICAL_INVALID_V074)
    return retained;
  if(name == "density_moment_v076")
    return STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076;
  return STELLAR_PHYSICAL_OPTICAL_INVALID_V074;
}

inline const char *stellarPhysicalOpticalProfileNameV076(int profile)
{
  if(profile == STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076)
    return "density_moment_v076";
  return stellarPhysicalOpticalProfileNameV075(profile);
}

STELLAR_PHYSICAL_OPTICAL_V076_HD inline bool
stellarPhysicalOpticalUsesMomentsV076(int profile)
{
  return profile == STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076;
}

STELLAR_PHYSICAL_OPTICAL_V076_HD inline float
stellarPhysicalDensitySupportV076(
    const StellarPhysicalSampleV071 &physical,
    const StellarPhysicalOpticalParametersV076 &optical_parameters)
{
  if(!(physical.density_cgs > 0.0f) || !stellarFinite(physical.density_cgs))
    return 0.0f;
  const float density_log10 = log10f(physical.density_cgs);
  return stellarSmoothstep(optical_parameters.density_support_log10_low,
                           optical_parameters.density_support_log10_high,
                           density_log10);
}

// For v076 the three emissivity slots are scalar moments, not display RGB:
// [weighted styled scalar, scalar weight, weighted intensity]. Applying the
// palette once after ray integration avoids chromatic averaging along the ray.
STELLAR_PHYSICAL_OPTICAL_V076_HD inline StellarOpticalSample
evaluateStellarPhysicalOpticalFromValueV076(
    float raw_value, const StellarPhysicalSampleV071 &physical,
    const StellarFeatureSampleV064 &feature,
    const StellarPhysicalTransferV071 &transfer,
    const StellarPhysicalOpticalParametersV076 &optical_parameters,
    const StellarTransferParameters &geometry)
{
  if(optical_parameters.profile !=
     STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076) {
    StellarPhysicalOpticalParametersV075 retained = {};
    retained.profile = optical_parameters.profile;
    retained.target_optical_depth = optical_parameters.target_optical_depth;
    retained.target_emission = optical_parameters.target_emission;
    retained.reference_path_cm = optical_parameters.reference_path_cm;
    retained.opacity_signal_threshold =
        optical_parameters.opacity_signal_threshold;
    retained.color_gamma = optical_parameters.color_gamma;
    retained.color_invert = optical_parameters.color_invert;
    return evaluateStellarPhysicalOpticalFromValueV075(
        raw_value, feature, transfer, retained, geometry);
  }

  StellarOpticalSample output = {{0.0f, 0.0f, 0.0f}, 0.0f};
  const float transformed = stellarPhysicalTransformV071(raw_value, transfer);
  if(!stellarFinite(transformed))
    return output;
  const float support = stellarPhysicalDensitySupportV076(
      physical, optical_parameters);
  if(!(support > 0.0f))
    return output;
  const float reference_path = stellarPhysicalReferencePathV074(
      transfer.channel, geometry, optical_parameters.reference_path_cm);
  if(!(reference_path > 0.0f))
    return output;

  const float fraction = stellarClamp(
      (transformed - transfer.range_min) /
      (transfer.range_max - transfer.range_min), 0.0f, 1.0f);
  const float styled_fraction = stellarPhysicalStyledFractionV075(
      fraction, optical_parameters.color_gamma,
      optical_parameters.color_invert);
  const float signal = stellarPhysicalSignalV074(transformed, transfer);
  const float floor = stellarClamp(
      optical_parameters.emission_signal_floor, 0.0f, 1.0f);
  const float amplitude = floor + (1.0f - floor) * signal;
  const float scalar_weight = optical_parameters.target_emission /
      reference_path * support;

  output.extinction_per_cm = optical_parameters.target_optical_depth /
      reference_path * support;
  output.emissivity_rgb_per_cm[0] = scalar_weight * styled_fraction;
  output.emissivity_rgb_per_cm[1] = scalar_weight;
  output.emissivity_rgb_per_cm[2] = scalar_weight * amplitude;
  return output;
}

STELLAR_PHYSICAL_OPTICAL_V076_HD inline void
stellarDecodePhysicalMomentsV076(const float input[3], int profile,
                                 float output[3])
{
  if(!stellarPhysicalOpticalUsesMomentsV076(profile)) {
    output[0] = input[0];
    output[1] = input[1];
    output[2] = input[2];
    return;
  }
  const float denominator = input[1];
  if(!(denominator > 0.0f) || !stellarFinite(denominator)) {
    output[0] = 0.0f;
    output[1] = 0.0f;
    output[2] = 0.0f;
    return;
  }
  const float fraction = stellarClamp(input[0] / denominator, 0.0f, 1.0f);
  const float intensity = stellarFinite(input[2]) ? fmaxf(input[2], 0.0f) : 0.0f;
  float color[3];
  stellarCopperBlueV071(fraction, color);
  for(int component = 0; component < 3; component++)
    output[component] = intensity * color[component];
}

#undef STELLAR_PHYSICAL_OPTICAL_V076_HD

#endif
