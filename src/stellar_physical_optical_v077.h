#ifndef AREPO_VTK_STELLAR_PHYSICAL_OPTICAL_V077_H
#define AREPO_VTK_STELLAR_PHYSICAL_OPTICAL_V077_H

#include <cmath>
#include <string>

#include "stellar_physical_optical_v076.h"

#ifdef __CUDACC__
#define STELLAR_PHYSICAL_OPTICAL_V077_HD __host__ __device__
#else
#define STELLAR_PHYSICAL_OPTICAL_V077_HD
#endif

enum StellarPhysicalOpticalProfileV077 {
  STELLAR_PHYSICAL_OPTICAL_NORMALIZED_MOMENT_V077 = 4
};

inline int stellarPhysicalOpticalProfileFromNameV077(const std::string &name)
{
  const int retained = stellarPhysicalOpticalProfileFromNameV076(name);
  if(retained != STELLAR_PHYSICAL_OPTICAL_INVALID_V074)
    return retained;
  if(name == "normalized_moment_v077")
    return STELLAR_PHYSICAL_OPTICAL_NORMALIZED_MOMENT_V077;
  return STELLAR_PHYSICAL_OPTICAL_INVALID_V074;
}

inline const char *stellarPhysicalOpticalProfileNameV077(int profile)
{
  if(profile == STELLAR_PHYSICAL_OPTICAL_NORMALIZED_MOMENT_V077)
    return "normalized_moment_v077";
  return stellarPhysicalOpticalProfileNameV076(profile);
}

STELLAR_PHYSICAL_OPTICAL_V077_HD inline bool
stellarPhysicalOpticalUsesNormalizedMomentsV077(int profile)
{
  return profile == STELLAR_PHYSICAL_OPTICAL_NORMALIZED_MOMENT_V077;
}

STELLAR_PHYSICAL_OPTICAL_V077_HD inline StellarOpticalSample
evaluateStellarPhysicalOpticalFromValueV077(
    float raw_value, const StellarPhysicalSampleV071 &physical,
    const StellarFeatureSampleV064 &feature,
    const StellarPhysicalTransferV071 &transfer,
    const StellarPhysicalOpticalParametersV076 &optical_parameters,
    const StellarTransferParameters &geometry)
{
  if(!stellarPhysicalOpticalUsesNormalizedMomentsV077(
         optical_parameters.profile))
    return evaluateStellarPhysicalOpticalFromValueV076(
        raw_value, physical, feature, transfer, optical_parameters, geometry);

  StellarPhysicalOpticalParametersV076 retained = optical_parameters;
  retained.profile = STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076;
  return evaluateStellarPhysicalOpticalFromValueV076(
      raw_value, physical, feature, transfer, retained, geometry);
}

// v076 stores three attenuated ray moments:
// [weighted styled scalar, scalar weight, weighted amplitude]. The old v076
// decoder normalized only the scalar and exposed the third line integral as
// display intensity. For low target optical depths that line integral tends to
// target_emission / target_optical_depth and clips every color channel. V077
// normalizes amplitude as well and recovers bounded emitted coverage from the
// denominator moment before applying the palette once per ray.
STELLAR_PHYSICAL_OPTICAL_V077_HD inline void
stellarDecodePhysicalMomentsV077(
    const float input[3],
    const StellarPhysicalOpticalParametersV076 &optical_parameters,
    float output[3])
{
  if(!stellarPhysicalOpticalUsesNormalizedMomentsV077(
         optical_parameters.profile)) {
    stellarDecodePhysicalMomentsV076(
        input, optical_parameters.profile, output);
    return;
  }

  const float denominator = input[1];
  if(!(denominator > 0.0f) || !stellarFinite(denominator) ||
     !(optical_parameters.target_optical_depth > 0.0f) ||
     !(optical_parameters.target_emission > 0.0f)) {
    output[0] = 0.0f;
    output[1] = 0.0f;
    output[2] = 0.0f;
    return;
  }

  const float fraction = stellarClamp(input[0] / denominator, 0.0f, 1.0f);
  const float mean_amplitude = stellarFinite(input[2]) ?
      stellarClamp(input[2] / denominator, 0.0f, 1.0f) : 0.0f;
  const float emitted_coverage = stellarClamp(
      denominator * optical_parameters.target_optical_depth, 0.0f,
      optical_parameters.target_emission);
  const float intensity = emitted_coverage * mean_amplitude;
  float color[3];
  stellarCopperBlueV071(fraction, color);
  for(int component = 0; component < 3; component++)
    output[component] = intensity * color[component];
}

#undef STELLAR_PHYSICAL_OPTICAL_V077_HD

#endif
