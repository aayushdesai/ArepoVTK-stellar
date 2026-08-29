#ifndef AREPO_VTK_STELLAR_PHYSICAL_OPTICAL_V075_H
#define AREPO_VTK_STELLAR_PHYSICAL_OPTICAL_V075_H

#include <cmath>
#include <string>

#include "stellar_physical_optical_v074.h"

#ifdef __CUDACC__
#define STELLAR_PHYSICAL_OPTICAL_V075_HD __host__ __device__
#else
#define STELLAR_PHYSICAL_OPTICAL_V075_HD
#endif

enum StellarPhysicalOpticalProfileV075 {
  STELLAR_PHYSICAL_OPTICAL_SEPARATED_SUPPORT_V075 = 2
};

struct StellarPhysicalOpticalParametersV075 {
  int profile;
  float target_optical_depth;
  float target_emission;
  float reference_path_cm;
  float opacity_signal_threshold;
  float color_gamma;
  int color_invert;
};

inline int stellarPhysicalOpticalProfileFromNameV075(const std::string &name)
{
  const int retained = stellarPhysicalOpticalProfileFromNameV074(name);
  if(retained != STELLAR_PHYSICAL_OPTICAL_INVALID_V074)
    return retained;
  if(name == "separated_support_v075")
    return STELLAR_PHYSICAL_OPTICAL_SEPARATED_SUPPORT_V075;
  return STELLAR_PHYSICAL_OPTICAL_INVALID_V074;
}

inline const char *stellarPhysicalOpticalProfileNameV075(int profile)
{
  if(profile == STELLAR_PHYSICAL_OPTICAL_SEPARATED_SUPPORT_V075)
    return "separated_support_v075";
  return stellarPhysicalOpticalProfileNameV074(profile);
}

STELLAR_PHYSICAL_OPTICAL_V075_HD inline float
stellarPhysicalStyledFractionV075(float fraction, float gamma, int invert)
{
  const float effective_gamma = gamma > 0.0f ? gamma : 1.0f;
  float styled = powf(stellarClamp(fraction, 0.0f, 1.0f),
                      1.0f / effective_gamma);
  if(invert)
    styled = 1.0f - styled;
  return stellarClamp(styled, 0.0f, 1.0f);
}

STELLAR_PHYSICAL_OPTICAL_V075_HD inline StellarOpticalSample
evaluateStellarPhysicalOpticalFromValueV075(
    float raw_value, const StellarFeatureSampleV064 &feature,
    const StellarPhysicalTransferV071 &transfer,
    const StellarPhysicalOpticalParametersV075 &optical_parameters,
    const StellarTransferParameters &geometry)
{
  if(optical_parameters.profile !=
     STELLAR_PHYSICAL_OPTICAL_SEPARATED_SUPPORT_V075) {
    StellarPhysicalOpticalParametersV074 retained = {};
    retained.profile = optical_parameters.profile;
    retained.target_optical_depth = optical_parameters.target_optical_depth;
    retained.target_emission = optical_parameters.target_emission;
    retained.reference_path_cm = optical_parameters.reference_path_cm;
    return evaluateStellarPhysicalOpticalFromValueV074(
        raw_value, feature, transfer, retained, geometry);
  }

  StellarOpticalSample output = {{0.0f, 0.0f, 0.0f}, 0.0f};
  const float transformed = stellarPhysicalTransformV071(raw_value, transfer);
  if(!stellarFinite(transformed))
    return output;
  const float denominator = transfer.range_max - transfer.range_min;
  const float fraction = stellarClamp(
      (transformed - transfer.range_min) / denominator, 0.0f, 1.0f);
  const float signal = stellarPhysicalSignalV074(transformed, transfer);
  const float support = stellarClamp(stellarPhysicalMaterialSupportV074(
      transfer.channel, feature), 0.0f, 1.0f);
  if(!(signal > 0.0f) || !(support > 0.0f))
    return output;
  const float reference_path = stellarPhysicalReferencePathV074(
      transfer.channel, geometry, optical_parameters.reference_path_cm);
  if(!(reference_path > 0.0f))
    return output;

  const float relevance = stellarSmoothstep(
      0.0f, optical_parameters.opacity_signal_threshold, signal);
  output.extinction_per_cm = optical_parameters.target_optical_depth /
      reference_path * support * relevance;
  const float emission = optical_parameters.target_emission /
      reference_path * support * signal;
  const float styled_fraction = stellarPhysicalStyledFractionV075(
      fraction, optical_parameters.color_gamma,
      optical_parameters.color_invert);
  float color[3];
  stellarCopperBlueV071(styled_fraction, color);
  for(int component = 0; component < 3; component++)
    output.emissivity_rgb_per_cm[component] = emission * color[component];
  return output;
}

#undef STELLAR_PHYSICAL_OPTICAL_V075_HD

#endif
