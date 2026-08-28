#ifndef AREPO_VTK_STELLAR_PHYSICAL_CHANNEL_V071_H
#define AREPO_VTK_STELLAR_PHYSICAL_CHANNEL_V071_H

#include <cmath>
#include <string>

#include "stellar_render_model_v052a.h"

enum StellarPhysicalChannelV071 {
  STELLAR_PHYSICAL_CHANNEL_INVALID_V071 = -2,
  STELLAR_PHYSICAL_CHANNEL_OPTICAL_V071 = -1,
  STELLAR_PHYSICAL_CHANNEL_DENSITY_V071 = 0,
  STELLAR_PHYSICAL_CHANNEL_TEMPERATURE_V071 = 1,
  STELLAR_PHYSICAL_CHANNEL_SPEED_V071 = 2,
  STELLAR_PHYSICAL_CHANNEL_RADIAL_VELOCITY_V071 = 3,
  STELLAR_PHYSICAL_CHANNEL_AZIMUTHAL_VELOCITY_V071 = 4,
  STELLAR_PHYSICAL_CHANNEL_ROTATIONAL_FRACTION_V071 = 5,
  STELLAR_PHYSICAL_CHANNEL_ANGULAR_MOMENTUM_ALIGNMENT_V071 = 6,
  STELLAR_PHYSICAL_CHANNEL_OUTWARD_AXIAL_VELOCITY_V071 = 7,
  STELLAR_PHYSICAL_CHANNEL_OUTWARD_MASS_FLUX_V071 = 8,
  STELLAR_PHYSICAL_CHANNEL_CYLINDRICAL_RADIUS_V071 = 9,
  STELLAR_PHYSICAL_CHANNEL_AXIAL_POSITION_V071 = 10,
  STELLAR_PHYSICAL_CHANNEL_ENTROPY_PROXY_V071 = 11,
  STELLAR_PHYSICAL_CHANNEL_MAGNETIC_FIELD_STRENGTH_V071 = 12
};

enum StellarPhysicalScaleV071 {
  STELLAR_PHYSICAL_SCALE_INVALID_V071 = -1,
  STELLAR_PHYSICAL_SCALE_LINEAR_V071 = 0,
  STELLAR_PHYSICAL_SCALE_LOG10_V071 = 1,
  STELLAR_PHYSICAL_SCALE_SYMLOG_V071 = 2
};

struct StellarPhysicalSampleV071 {
  float density_cgs;
  float temperature_kelvin;
  float speed_cm_per_s;
  float radial_velocity_cm_per_s;
  float azimuthal_velocity_cm_per_s;
  float rotational_fraction;
  float angular_momentum_alignment;
  float outward_axial_velocity_cm_per_s;
  float outward_mass_flux_proxy;
  float cylindrical_radius_cm;
  float axial_position_cm;
  float entropy_proxy;
  float magnetic_field_strength_microgauss;
};

struct StellarPhysicalTransferV071 {
  int channel;
  int scale;
  float range_min;
  float range_max;
  float symlog_linthresh;
  float extinction_per_cm;
  float emissivity_per_cm;
};

inline int stellarPhysicalChannelFromNameV071(const std::string &name)
{
  if(name == "optical") return STELLAR_PHYSICAL_CHANNEL_OPTICAL_V071;
  if(name == "density") return STELLAR_PHYSICAL_CHANNEL_DENSITY_V071;
  if(name == "temperature") return STELLAR_PHYSICAL_CHANNEL_TEMPERATURE_V071;
  if(name == "speed") return STELLAR_PHYSICAL_CHANNEL_SPEED_V071;
  if(name == "radial_velocity") return STELLAR_PHYSICAL_CHANNEL_RADIAL_VELOCITY_V071;
  if(name == "azimuthal_velocity") return STELLAR_PHYSICAL_CHANNEL_AZIMUTHAL_VELOCITY_V071;
  if(name == "rotational_fraction") return STELLAR_PHYSICAL_CHANNEL_ROTATIONAL_FRACTION_V071;
  if(name == "angular_momentum_alignment") return STELLAR_PHYSICAL_CHANNEL_ANGULAR_MOMENTUM_ALIGNMENT_V071;
  if(name == "outward_axial_velocity") return STELLAR_PHYSICAL_CHANNEL_OUTWARD_AXIAL_VELOCITY_V071;
  if(name == "outward_mass_flux_proxy") return STELLAR_PHYSICAL_CHANNEL_OUTWARD_MASS_FLUX_V071;
  if(name == "cylindrical_radius") return STELLAR_PHYSICAL_CHANNEL_CYLINDRICAL_RADIUS_V071;
  if(name == "axial_position") return STELLAR_PHYSICAL_CHANNEL_AXIAL_POSITION_V071;
  if(name == "entropy_proxy") return STELLAR_PHYSICAL_CHANNEL_ENTROPY_PROXY_V071;
  if(name == "magnetic_field_strength") return STELLAR_PHYSICAL_CHANNEL_MAGNETIC_FIELD_STRENGTH_V071;
  return STELLAR_PHYSICAL_CHANNEL_INVALID_V071;
}

inline int stellarPhysicalScaleFromNameV071(const std::string &name)
{
  if(name == "linear") return STELLAR_PHYSICAL_SCALE_LINEAR_V071;
  if(name == "log10") return STELLAR_PHYSICAL_SCALE_LOG10_V071;
  if(name == "symlog") return STELLAR_PHYSICAL_SCALE_SYMLOG_V071;
  return STELLAR_PHYSICAL_SCALE_INVALID_V071;
}

inline StellarPhysicalSampleV071 evaluateStellarPhysicalSampleV071(
    const StellarTransferParameters &parameters, const double position[3],
    float density_log10_plus_10, float temperature_kelvin,
    const float velocity_cm_per_s[3], float entropy_proxy,
    float magnetic_field_strength_microgauss)
{
  StellarPhysicalSampleV071 output = {};
  output.density_cgs = powf(10.0f, density_log10_plus_10 - 10.0f);
  output.temperature_kelvin = temperature_kelvin;
  output.entropy_proxy = entropy_proxy;
  output.magnetic_field_strength_microgauss =
      magnetic_field_strength_microgauss;

  double displacement[3];
  double radius_squared = 0.0;
  double height = 0.0;
  float relative_velocity[3];
  float speed_squared = 0.0f;
  float axial_velocity = 0.0f;
  for(int component = 0; component < 3; component++) {
    displacement[component] = stellarPeriodicDelta(
        position[component], parameters.center[component], parameters.box_size);
    radius_squared += displacement[component] * displacement[component];
    height += displacement[component] * parameters.axis[component];
    relative_velocity[component] = velocity_cm_per_s[component] -
        parameters.bulk_velocity_cm_per_s[component];
    speed_squared += relative_velocity[component] * relative_velocity[component];
    axial_velocity += relative_velocity[component] *
        float(parameters.axis[component]);
  }
  const float radius = float(sqrt(radius_squared));
  const double cylindrical_squared = radius_squared - height * height;
  output.cylindrical_radius_cm = float(sqrt(
      cylindrical_squared > 0.0 ? cylindrical_squared : 0.0));
  output.axial_position_cm = float(height);
  output.speed_cm_per_s = sqrtf(speed_squared);
  if(radius > 1.0e6f)
    for(int component = 0; component < 3; component++)
      output.radial_velocity_cm_per_s += relative_velocity[component] *
          float(displacement[component]) / radius;
  if(output.cylindrical_radius_cm > 1.0e6f) {
    float cylindrical_unit[3];
    float azimuthal_unit[3];
    for(int component = 0; component < 3; component++)
      cylindrical_unit[component] = float(displacement[component] -
          height * parameters.axis[component]) / output.cylindrical_radius_cm;
    azimuthal_unit[0] = float(parameters.axis[1]) * cylindrical_unit[2] -
        float(parameters.axis[2]) * cylindrical_unit[1];
    azimuthal_unit[1] = float(parameters.axis[2]) * cylindrical_unit[0] -
        float(parameters.axis[0]) * cylindrical_unit[2];
    azimuthal_unit[2] = float(parameters.axis[0]) * cylindrical_unit[1] -
        float(parameters.axis[1]) * cylindrical_unit[0];
    for(int component = 0; component < 3; component++)
      output.azimuthal_velocity_cm_per_s +=
          relative_velocity[component] * azimuthal_unit[component];
  }
  output.rotational_fraction = fabsf(output.azimuthal_velocity_cm_per_s) /
      fmaxf(output.speed_cm_per_s, 1.0f);
  if(radius > 1.0e6f && output.speed_cm_per_s > 1.0f) {
    const double angular_momentum[3] = {
        displacement[1] * relative_velocity[2] -
            displacement[2] * relative_velocity[1],
        displacement[2] * relative_velocity[0] -
            displacement[0] * relative_velocity[2],
        displacement[0] * relative_velocity[1] -
            displacement[1] * relative_velocity[0]};
    output.angular_momentum_alignment = float(
        (angular_momentum[0] * parameters.axis[0] +
         angular_momentum[1] * parameters.axis[1] +
         angular_momentum[2] * parameters.axis[2]) /
        (double(radius) * output.speed_cm_per_s));
  }
  output.outward_axial_velocity_cm_per_s =
      height >= 0.0 ? axial_velocity : -axial_velocity;
  output.outward_mass_flux_proxy = output.density_cgs *
      fmaxf(output.outward_axial_velocity_cm_per_s, 0.0f);
  return output;
}

inline float stellarPhysicalValueV071(
    const StellarPhysicalSampleV071 &sample, int channel)
{
  switch(channel) {
    case STELLAR_PHYSICAL_CHANNEL_DENSITY_V071: return sample.density_cgs;
    case STELLAR_PHYSICAL_CHANNEL_TEMPERATURE_V071: return sample.temperature_kelvin;
    case STELLAR_PHYSICAL_CHANNEL_SPEED_V071: return sample.speed_cm_per_s;
    case STELLAR_PHYSICAL_CHANNEL_RADIAL_VELOCITY_V071: return sample.radial_velocity_cm_per_s;
    case STELLAR_PHYSICAL_CHANNEL_AZIMUTHAL_VELOCITY_V071: return sample.azimuthal_velocity_cm_per_s;
    case STELLAR_PHYSICAL_CHANNEL_ROTATIONAL_FRACTION_V071: return sample.rotational_fraction;
    case STELLAR_PHYSICAL_CHANNEL_ANGULAR_MOMENTUM_ALIGNMENT_V071: return sample.angular_momentum_alignment;
    case STELLAR_PHYSICAL_CHANNEL_OUTWARD_AXIAL_VELOCITY_V071: return sample.outward_axial_velocity_cm_per_s;
    case STELLAR_PHYSICAL_CHANNEL_OUTWARD_MASS_FLUX_V071: return sample.outward_mass_flux_proxy;
    case STELLAR_PHYSICAL_CHANNEL_CYLINDRICAL_RADIUS_V071: return sample.cylindrical_radius_cm;
    case STELLAR_PHYSICAL_CHANNEL_AXIAL_POSITION_V071: return sample.axial_position_cm;
    case STELLAR_PHYSICAL_CHANNEL_ENTROPY_PROXY_V071: return sample.entropy_proxy;
    case STELLAR_PHYSICAL_CHANNEL_MAGNETIC_FIELD_STRENGTH_V071: return sample.magnetic_field_strength_microgauss;
  }
  return 0.0f;
}

inline float stellarPhysicalTransformV071(
    float value, const StellarPhysicalTransferV071 &parameters)
{
  if(parameters.scale == STELLAR_PHYSICAL_SCALE_LOG10_V071)
    return value > 0.0f ? log10f(value) : -INFINITY;
  if(parameters.scale == STELLAR_PHYSICAL_SCALE_SYMLOG_V071)
    return copysignf(log10f(1.0f + fabsf(value) /
        parameters.symlog_linthresh), value);
  return value;
}

inline void stellarCopperBlueV071(float fraction, float output[3])
{
  const float stops[4][3] = {
      {0.027451f, 0.062745f, 0.109804f},
      {0.160784f, 0.380392f, 0.549020f},
      {0.721569f, 0.337255f, 0.121569f},
      {1.000000f, 0.878431f, 0.619608f}};
  const float scaled = stellarClamp(fraction, 0.0f, 1.0f) * 3.0f;
  const int left = scaled >= 3.0f ? 2 : int(floorf(scaled));
  const float local = scaled - float(left);
  for(int component = 0; component < 3; component++)
    output[component] = stops[left][component] + local *
        (stops[left + 1][component] - stops[left][component]);
}

inline StellarOpticalSample evaluateStellarPhysicalOpticalV071(
    const StellarPhysicalSampleV071 &sample,
    const StellarPhysicalTransferV071 &parameters)
{
  StellarOpticalSample output = {{0.0f, 0.0f, 0.0f}, 0.0f};
  const float transformed = stellarPhysicalTransformV071(
      stellarPhysicalValueV071(sample, parameters.channel), parameters);
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
