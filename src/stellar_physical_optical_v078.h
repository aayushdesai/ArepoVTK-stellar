#ifndef AREPO_VTK_STELLAR_PHYSICAL_OPTICAL_V078_H
#define AREPO_VTK_STELLAR_PHYSICAL_OPTICAL_V078_H

#include <cmath>
#include <string>

#include "stellar_physical_optical_v077.h"

#ifdef __CUDACC__
#define STELLAR_PHYSICAL_OPTICAL_V078_HD __host__ __device__
#else
#define STELLAR_PHYSICAL_OPTICAL_V078_HD
#endif

enum StellarPhysicalOpticalProfileV078 {
  STELLAR_PHYSICAL_OPTICAL_COMPOSITE_MOMENT_V078 = 5
};

inline int stellarPhysicalOpticalProfileFromNameV078(const std::string &name)
{
  const int retained = stellarPhysicalOpticalProfileFromNameV077(name);
  if(retained != STELLAR_PHYSICAL_OPTICAL_INVALID_V074)
    return retained;
  if(name == "composite_moment_v078")
    return STELLAR_PHYSICAL_OPTICAL_COMPOSITE_MOMENT_V078;
  return STELLAR_PHYSICAL_OPTICAL_INVALID_V074;
}

inline const char *stellarPhysicalOpticalProfileNameV078(int profile)
{
  if(profile == STELLAR_PHYSICAL_OPTICAL_COMPOSITE_MOMENT_V078)
    return "composite_moment_v078";
  return stellarPhysicalOpticalProfileNameV077(profile);
}

STELLAR_PHYSICAL_OPTICAL_V078_HD inline bool
stellarPhysicalOpticalUsesCompositeMomentsV078(int profile)
{
  return profile == STELLAR_PHYSICAL_OPTICAL_COMPOSITE_MOMENT_V078;
}

STELLAR_PHYSICAL_OPTICAL_V078_HD inline float
stellarPhysicalCompositeFeatureSupportV078(
    const StellarFeatureSampleV064 &feature)
{
  const float merger = stellarClamp(feature.merger_weight, 0.0f, 1.0f);
  const float disk = stellarClamp(feature.disk_weight, 0.0f, 1.0f);
  const float polar = stellarClamp(feature.polar_weight, 0.0f, 1.0f);
  return 1.0f - (1.0f - merger) * (1.0f - disk) * (1.0f - polar);
}

STELLAR_PHYSICAL_OPTICAL_V078_HD inline StellarOpticalSample
evaluateStellarPhysicalOpticalFromValueV078(
    float raw_value, const StellarPhysicalSampleV071 &physical,
    const StellarFeatureSampleV064 &feature,
    const StellarPhysicalTransferV071 &transfer,
    const StellarPhysicalOpticalParametersV076 &optical_parameters,
    const StellarTransferParameters &geometry)
{
  if(!stellarPhysicalOpticalUsesCompositeMomentsV078(
         optical_parameters.profile))
    return evaluateStellarPhysicalOpticalFromValueV077(
        raw_value, physical, feature, transfer, optical_parameters, geometry);

  StellarOpticalSample output = {{0.0f, 0.0f, 0.0f}, 0.0f};
  const float transformed = stellarPhysicalTransformV071(raw_value, transfer);
  if(!stellarFinite(transformed))
    return output;
  const float density_support = stellarPhysicalDensitySupportV076(
      physical, optical_parameters);
  const float feature_support = stellarPhysicalCompositeFeatureSupportV078(
      feature);
  const float support = density_support * feature_support;
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

STELLAR_PHYSICAL_OPTICAL_V078_HD inline void
stellarDecodePhysicalMomentsV078(
    const float input[3],
    const StellarPhysicalOpticalParametersV076 &optical_parameters,
    float output[3])
{
  if(!stellarPhysicalOpticalUsesCompositeMomentsV078(
         optical_parameters.profile)) {
    stellarDecodePhysicalMomentsV077(input, optical_parameters, output);
    return;
  }
  StellarPhysicalOpticalParametersV076 normalized = optical_parameters;
  normalized.profile = STELLAR_PHYSICAL_OPTICAL_NORMALIZED_MOMENT_V077;
  stellarDecodePhysicalMomentsV077(input, normalized, output);
}

#undef STELLAR_PHYSICAL_OPTICAL_V078_HD

#endif
