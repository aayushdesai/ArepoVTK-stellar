#ifndef AREPO_VTK_STELLAR_PHYSICAL_OPTICAL_V074_H
#define AREPO_VTK_STELLAR_PHYSICAL_OPTICAL_V074_H

#include <cmath>
#include <string>

#include "stellar_physical_channel_v072.h"

#ifdef __CUDACC__
#define STELLAR_PHYSICAL_OPTICAL_HD __host__ __device__
#else
#define STELLAR_PHYSICAL_OPTICAL_HD
#endif

enum StellarPhysicalOpticalProfileV074 {
  STELLAR_PHYSICAL_OPTICAL_INVALID_V074 = -1,
  STELLAR_PHYSICAL_OPTICAL_LEGACY_V072 = 0,
  STELLAR_PHYSICAL_OPTICAL_MATERIAL_SUPPORT_V074 = 1
};

enum StellarPhysicalSupportClassV074 {
  STELLAR_PHYSICAL_SUPPORT_MATERIAL_V074 = 0,
  STELLAR_PHYSICAL_SUPPORT_DISK_V074 = 1,
  STELLAR_PHYSICAL_SUPPORT_POLAR_V074 = 2
};

struct StellarPhysicalOpticalParametersV074 {
  int profile;
  float target_optical_depth;
  float target_emission;
  float reference_path_cm;
};

inline int stellarPhysicalOpticalProfileFromNameV074(const std::string &name)
{
  if(name == "legacy_v072")
    return STELLAR_PHYSICAL_OPTICAL_LEGACY_V072;
  if(name == "material_support_v074")
    return STELLAR_PHYSICAL_OPTICAL_MATERIAL_SUPPORT_V074;
  return STELLAR_PHYSICAL_OPTICAL_INVALID_V074;
}

inline const char *stellarPhysicalOpticalProfileNameV074(int profile)
{
  if(profile == STELLAR_PHYSICAL_OPTICAL_LEGACY_V072)
    return "legacy_v072";
  if(profile == STELLAR_PHYSICAL_OPTICAL_MATERIAL_SUPPORT_V074)
    return "material_support_v074";
  return "invalid";
}

STELLAR_PHYSICAL_OPTICAL_HD inline int stellarPhysicalSupportClassV074(
    int channel)
{
  if(channel == STELLAR_PHYSICAL_CHANNEL_AZIMUTHAL_VELOCITY_V071 ||
     channel == STELLAR_PHYSICAL_CHANNEL_ROTATIONAL_FRACTION_V071 ||
     channel == STELLAR_PHYSICAL_CHANNEL_ANGULAR_MOMENTUM_ALIGNMENT_V071 ||
     channel == STELLAR_PHYSICAL_CHANNEL_CYLINDRICAL_RADIUS_V071 ||
     channel == STELLAR_PHYSICAL_CHANNEL_MAGNETIC_FIELD_AZIMUTHAL_V072 ||
     channel == STELLAR_PHYSICAL_CHANNEL_TOROIDAL_FIELD_FRACTION_V072)
    return STELLAR_PHYSICAL_SUPPORT_DISK_V074;
  if(channel == STELLAR_PHYSICAL_CHANNEL_OUTWARD_AXIAL_VELOCITY_V071 ||
     channel == STELLAR_PHYSICAL_CHANNEL_OUTWARD_MASS_FLUX_V071)
    return STELLAR_PHYSICAL_SUPPORT_POLAR_V074;
  return STELLAR_PHYSICAL_SUPPORT_MATERIAL_V074;
}

STELLAR_PHYSICAL_OPTICAL_HD inline float stellarPhysicalReferencePathV074(
    int channel, const StellarTransferParameters &geometry,
    float explicit_reference_path_cm)
{
  if(explicit_reference_path_cm > 0.0f)
    return explicit_reference_path_cm;
  const int support_class = stellarPhysicalSupportClassV074(channel);
  if(support_class == STELLAR_PHYSICAL_SUPPORT_DISK_V074)
    return 2.0f * geometry.disk_radius_cm;
  if(support_class == STELLAR_PHYSICAL_SUPPORT_POLAR_V074)
    return 2.0f * geometry.polar_outer_cm;
  return 2.0f * fmaxf(geometry.material_radius_cm,
                      geometry.disk_radius_cm);
}

STELLAR_PHYSICAL_OPTICAL_HD inline bool stellarPhysicalSignedChannelV074(
    int channel)
{
  return channel == STELLAR_PHYSICAL_CHANNEL_RADIAL_VELOCITY_V071 ||
      channel == STELLAR_PHYSICAL_CHANNEL_AZIMUTHAL_VELOCITY_V071 ||
      channel == STELLAR_PHYSICAL_CHANNEL_ANGULAR_MOMENTUM_ALIGNMENT_V071 ||
      channel == STELLAR_PHYSICAL_CHANNEL_AXIAL_POSITION_V071 ||
      channel == STELLAR_PHYSICAL_CHANNEL_MAGNETIC_FIELD_AXIAL_V072 ||
      channel == STELLAR_PHYSICAL_CHANNEL_MAGNETIC_FIELD_AZIMUTHAL_V072 ||
      channel == STELLAR_PHYSICAL_CHANNEL_FIELD_VELOCITY_ALIGNMENT_V072;
}

STELLAR_PHYSICAL_OPTICAL_HD inline float stellarPhysicalSignalV074(
    float transformed, const StellarPhysicalTransferV071 &transfer)
{
  const float denominator = transfer.range_max - transfer.range_min;
  const float fraction = stellarClamp(
      (transformed - transfer.range_min) / denominator, 0.0f, 1.0f);
  if(!stellarPhysicalSignedChannelV074(transfer.channel))
    return fraction;
  const float transformed_zero = stellarPhysicalTransformV071(0.0f, transfer);
  const float zero_fraction = stellarFinite(transformed_zero) ? stellarClamp(
      (transformed_zero - transfer.range_min) / denominator, 0.0f, 1.0f) : 0.0f;
  return stellarClamp(fabsf(fraction - zero_fraction) /
      fmaxf(zero_fraction, 1.0f - zero_fraction), 0.0f, 1.0f);
}

STELLAR_PHYSICAL_OPTICAL_HD inline float stellarPhysicalMaterialSupportV074(
    int channel, const StellarFeatureSampleV064 &feature)
{
  const int support_class = stellarPhysicalSupportClassV074(channel);
  if(support_class == STELLAR_PHYSICAL_SUPPORT_DISK_V074)
    return feature.disk_weight;
  if(support_class == STELLAR_PHYSICAL_SUPPORT_POLAR_V074)
    return feature.polar_weight;
  return fmaxf(feature.merger_weight,
               fmaxf(feature.disk_weight, feature.polar_weight));
}

STELLAR_PHYSICAL_OPTICAL_HD inline StellarOpticalSample
evaluateStellarPhysicalOpticalFromValueV074(
    float raw_value, const StellarFeatureSampleV064 &feature,
    const StellarPhysicalTransferV071 &transfer,
    const StellarPhysicalOpticalParametersV074 &optical_parameters,
    const StellarTransferParameters &geometry)
{
  StellarOpticalSample output = {{0.0f, 0.0f, 0.0f}, 0.0f};
  const float transformed = stellarPhysicalTransformV071(raw_value, transfer);
  if(!stellarFinite(transformed))
    return output;
  const float fraction = stellarClamp(
      (transformed - transfer.range_min) /
      (transfer.range_max - transfer.range_min), 0.0f, 1.0f);
  float color[3];
  stellarCopperBlueV071(fraction, color);

  if(optical_parameters.profile == STELLAR_PHYSICAL_OPTICAL_LEGACY_V072) {
    const float amplitude = 0.08f + 0.92f * fraction;
    for(int component = 0; component < 3; component++)
      output.emissivity_rgb_per_cm[component] =
          transfer.emissivity_per_cm * amplitude * color[component];
    output.extinction_per_cm = transfer.extinction_per_cm * amplitude;
    return output;
  }
  if(optical_parameters.profile !=
     STELLAR_PHYSICAL_OPTICAL_MATERIAL_SUPPORT_V074)
    return output;

  const float signal = stellarPhysicalSignalV074(transformed, transfer);
  const float material_support = stellarPhysicalMaterialSupportV074(
      transfer.channel, feature);
  const float support = signal * stellarClamp(material_support, 0.0f, 1.0f);
  if(!(support > 0.0f))
    return output;
  const float reference_path = stellarPhysicalReferencePathV074(
      transfer.channel, geometry, optical_parameters.reference_path_cm);
  if(!(reference_path > 0.0f))
    return output;
  const float extinction = optical_parameters.target_optical_depth /
      reference_path * support;
  const float emission = optical_parameters.target_emission /
      reference_path * support;
  output.extinction_per_cm = extinction;
  for(int component = 0; component < 3; component++)
    output.emissivity_rgb_per_cm[component] = emission * color[component];
  return output;
}

#undef STELLAR_PHYSICAL_OPTICAL_HD

#endif
