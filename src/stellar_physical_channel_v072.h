#ifndef AREPO_VTK_STELLAR_PHYSICAL_CHANNEL_V072_H
#define AREPO_VTK_STELLAR_PHYSICAL_CHANNEL_V072_H

#include <cmath>
#include <string>

#include "stellar_physical_channel_v071.h"

enum StellarPhysicalChannelV072 {
  STELLAR_PHYSICAL_CHANNEL_MAGNETIC_FIELD_AXIAL_V072 = 13,
  STELLAR_PHYSICAL_CHANNEL_MAGNETIC_FIELD_AZIMUTHAL_V072 = 14,
  STELLAR_PHYSICAL_CHANNEL_MAGNETIC_PRESSURE_V072 = 15,
  STELLAR_PHYSICAL_CHANNEL_ALFVEN_SPEED_V072 = 16,
  STELLAR_PHYSICAL_CHANNEL_FIELD_VELOCITY_ALIGNMENT_V072 = 17,
  STELLAR_PHYSICAL_CHANNEL_TOROIDAL_FIELD_FRACTION_V072 = 18,
  STELLAR_PHYSICAL_CHANNEL_POLOIDAL_FIELD_FRACTION_V072 = 19,
  STELLAR_PHYSICAL_CHANNEL_PLASMA_BETA_V072 = 20,
  STELLAR_PHYSICAL_CHANNEL_GAS_PRESSURE_V072 = 21,
  STELLAR_PHYSICAL_CHANNEL_SOUND_SPEED_V072 = 22,
  STELLAR_PHYSICAL_CHANNEL_MACH_NUMBER_V072 = 23
};

struct StellarAuxiliaryFieldsV072 {
  float magnetic_field_gauss[3];
  float pressure_dyn_cm2;
  float sound_speed_cm_per_s;
};

struct StellarExtendedPhysicalSampleV072 {
  float magnetic_field_strength_gauss;
  float magnetic_field_axial_gauss;
  float magnetic_field_azimuthal_gauss;
  float magnetic_pressure_dyn_cm2;
  float alfven_speed_cm_per_s;
  float field_velocity_alignment;
  float toroidal_field_fraction;
  float poloidal_field_fraction;
  float plasma_beta;
  float gas_pressure_dyn_cm2;
  float sound_speed_cm_per_s;
  float mach_number;
  float entropy_proxy_cgs;
};

inline int stellarPhysicalChannelFromNameV072(const std::string &name)
{
  const int retained = stellarPhysicalChannelFromNameV071(name);
  if(retained != STELLAR_PHYSICAL_CHANNEL_INVALID_V071)
    return retained;
  if(name == "magnetic_field_axial")
    return STELLAR_PHYSICAL_CHANNEL_MAGNETIC_FIELD_AXIAL_V072;
  if(name == "magnetic_field_azimuthal")
    return STELLAR_PHYSICAL_CHANNEL_MAGNETIC_FIELD_AZIMUTHAL_V072;
  if(name == "magnetic_pressure")
    return STELLAR_PHYSICAL_CHANNEL_MAGNETIC_PRESSURE_V072;
  if(name == "alfven_speed")
    return STELLAR_PHYSICAL_CHANNEL_ALFVEN_SPEED_V072;
  if(name == "field_velocity_alignment")
    return STELLAR_PHYSICAL_CHANNEL_FIELD_VELOCITY_ALIGNMENT_V072;
  if(name == "toroidal_field_fraction")
    return STELLAR_PHYSICAL_CHANNEL_TOROIDAL_FIELD_FRACTION_V072;
  if(name == "poloidal_field_fraction")
    return STELLAR_PHYSICAL_CHANNEL_POLOIDAL_FIELD_FRACTION_V072;
  if(name == "plasma_beta")
    return STELLAR_PHYSICAL_CHANNEL_PLASMA_BETA_V072;
  if(name == "gas_pressure")
    return STELLAR_PHYSICAL_CHANNEL_GAS_PRESSURE_V072;
  if(name == "sound_speed")
    return STELLAR_PHYSICAL_CHANNEL_SOUND_SPEED_V072;
  if(name == "mach_number")
    return STELLAR_PHYSICAL_CHANNEL_MACH_NUMBER_V072;
  return STELLAR_PHYSICAL_CHANNEL_INVALID_V071;
}

inline bool stellarPhysicalChannelRequiresAuxiliaryV072(int channel)
{
  return channel == STELLAR_PHYSICAL_CHANNEL_MAGNETIC_FIELD_STRENGTH_V071 ||
      channel == STELLAR_PHYSICAL_CHANNEL_ENTROPY_PROXY_V071 || channel >= 13;
}

inline StellarExtendedPhysicalSampleV072 evaluateStellarExtendedPhysicalSampleV072(
    const StellarTransferParameters &parameters, const double position[3],
    const float velocity_cm_per_s[3], const StellarPhysicalSampleV071 &base,
    const StellarAuxiliaryFieldsV072 &auxiliary)
{
  StellarExtendedPhysicalSampleV072 output = {};
  double displacement[3];
  double height = 0.0;
  double radius_squared = 0.0;
  float relative_velocity[3];
  for(int component = 0; component < 3; component++) {
    displacement[component] = stellarPeriodicDelta(
        position[component], parameters.center[component], parameters.box_size);
    height += displacement[component] * parameters.axis[component];
    radius_squared += displacement[component] * displacement[component];
    relative_velocity[component] = velocity_cm_per_s[component] -
        parameters.bulk_velocity_cm_per_s[component];
  }
  const double cylindrical_squared = radius_squared - height * height;
  const float cylindrical_radius = float(sqrt(
      cylindrical_squared > 0.0 ? cylindrical_squared : 0.0));
  float cylindrical_unit[3] = {0.0f, 0.0f, 0.0f};
  if(cylindrical_radius > 1.0e6f)
    for(int component = 0; component < 3; component++)
      cylindrical_unit[component] = float(displacement[component] -
          height * parameters.axis[component]) / cylindrical_radius;
  const float azimuthal_unit[3] = {
      float(parameters.axis[1]) * cylindrical_unit[2] -
          float(parameters.axis[2]) * cylindrical_unit[1],
      float(parameters.axis[2]) * cylindrical_unit[0] -
          float(parameters.axis[0]) * cylindrical_unit[2],
      float(parameters.axis[0]) * cylindrical_unit[1] -
          float(parameters.axis[1]) * cylindrical_unit[0]};

  float field_cylindrical = 0.0f;
  float field_velocity_dot = 0.0f;
  for(int component = 0; component < 3; component++) {
    const float field = auxiliary.magnetic_field_gauss[component];
    output.magnetic_field_strength_gauss += field * field;
    output.magnetic_field_axial_gauss +=
        field * float(parameters.axis[component]);
    output.magnetic_field_azimuthal_gauss += field * azimuthal_unit[component];
    field_cylindrical += field * cylindrical_unit[component];
    field_velocity_dot += field * relative_velocity[component];
  }
  output.magnetic_field_strength_gauss =
      sqrtf(output.magnetic_field_strength_gauss);
  const float field_poloidal = hypotf(
      output.magnetic_field_axial_gauss, field_cylindrical);
  const float safe_field = fmaxf(output.magnetic_field_strength_gauss, 1.0e-30f);
  const float pi = 3.14159265358979323846f;
  output.magnetic_pressure_dyn_cm2 =
      output.magnetic_field_strength_gauss *
      output.magnetic_field_strength_gauss / (8.0f * pi);
  output.alfven_speed_cm_per_s = output.magnetic_field_strength_gauss /
      sqrtf(4.0f * pi * fmaxf(base.density_cgs, 1.0e-30f));
  output.field_velocity_alignment = field_velocity_dot /
      fmaxf(output.magnetic_field_strength_gauss * base.speed_cm_per_s,
            1.0e-30f);
  output.toroidal_field_fraction =
      fabsf(output.magnetic_field_azimuthal_gauss) / safe_field;
  output.poloidal_field_fraction = field_poloidal / safe_field;
  output.gas_pressure_dyn_cm2 = auxiliary.pressure_dyn_cm2;
  output.plasma_beta = output.gas_pressure_dyn_cm2 /
      fmaxf(output.magnetic_pressure_dyn_cm2, 1.0e-30f);
  output.sound_speed_cm_per_s = auxiliary.sound_speed_cm_per_s;
  output.mach_number = base.speed_cm_per_s /
      fmaxf(output.sound_speed_cm_per_s, 1.0e-30f);
  output.entropy_proxy_cgs = output.gas_pressure_dyn_cm2 /
      powf(fmaxf(base.density_cgs, 1.0e-30f), 5.0f / 3.0f);
  return output;
}

inline float stellarPhysicalValueV072(
    const StellarPhysicalSampleV071 &base,
    const StellarExtendedPhysicalSampleV072 &extended, int channel)
{
  if(channel == STELLAR_PHYSICAL_CHANNEL_MAGNETIC_FIELD_STRENGTH_V071)
    return extended.magnetic_field_strength_gauss;
  if(channel == STELLAR_PHYSICAL_CHANNEL_ENTROPY_PROXY_V071)
    return extended.entropy_proxy_cgs;
  if(channel < 13)
    return stellarPhysicalValueV071(base, channel);
  switch(channel) {
    case STELLAR_PHYSICAL_CHANNEL_MAGNETIC_FIELD_AXIAL_V072:
      return extended.magnetic_field_axial_gauss;
    case STELLAR_PHYSICAL_CHANNEL_MAGNETIC_FIELD_AZIMUTHAL_V072:
      return extended.magnetic_field_azimuthal_gauss;
    case STELLAR_PHYSICAL_CHANNEL_MAGNETIC_PRESSURE_V072:
      return extended.magnetic_pressure_dyn_cm2;
    case STELLAR_PHYSICAL_CHANNEL_ALFVEN_SPEED_V072:
      return extended.alfven_speed_cm_per_s;
    case STELLAR_PHYSICAL_CHANNEL_FIELD_VELOCITY_ALIGNMENT_V072:
      return extended.field_velocity_alignment;
    case STELLAR_PHYSICAL_CHANNEL_TOROIDAL_FIELD_FRACTION_V072:
      return extended.toroidal_field_fraction;
    case STELLAR_PHYSICAL_CHANNEL_POLOIDAL_FIELD_FRACTION_V072:
      return extended.poloidal_field_fraction;
    case STELLAR_PHYSICAL_CHANNEL_PLASMA_BETA_V072:
      return extended.plasma_beta;
    case STELLAR_PHYSICAL_CHANNEL_GAS_PRESSURE_V072:
      return extended.gas_pressure_dyn_cm2;
    case STELLAR_PHYSICAL_CHANNEL_SOUND_SPEED_V072:
      return extended.sound_speed_cm_per_s;
    case STELLAR_PHYSICAL_CHANNEL_MACH_NUMBER_V072:
      return extended.mach_number;
  }
  return 0.0f;
}

inline StellarOpticalSample evaluateStellarPhysicalOpticalV072(
    const StellarPhysicalSampleV071 &base,
    const StellarExtendedPhysicalSampleV072 &extended,
    const StellarPhysicalTransferV071 &parameters)
{
  StellarOpticalSample output = {{0.0f, 0.0f, 0.0f}, 0.0f};
  const float transformed = stellarPhysicalTransformV071(
      stellarPhysicalValueV072(base, extended, parameters.channel), parameters);
  if(!stellarFinite(transformed))
    return output;
  const float fraction = stellarClamp(
      (transformed - parameters.range_min) /
      (parameters.range_max - parameters.range_min), 0.0f, 1.0f);
  float color[3];
  stellarCopperBlueV071(fraction, color);
  const float amplitude = 0.08f + 0.92f * fraction;
  for(int component = 0; component < 3; component++)
    output.emissivity_rgb_per_cm[component] =
        parameters.emissivity_per_cm * amplitude * color[component];
  output.extinction_per_cm = parameters.extinction_per_cm * amplitude;
  return output;
}

#endif
