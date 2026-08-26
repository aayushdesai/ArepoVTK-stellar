#ifndef AREPO_VTK_STELLAR_RENDER_MODEL_V052A_H
#define AREPO_VTK_STELLAR_RENDER_MODEL_V052A_H

#include <cmath>

#ifdef __CUDACC__
#define STELLAR_HD __host__ __device__
#else
#define STELLAR_HD
#endif

#include "stellar_palette_v057.h"

enum StellarTransferMode {
  STELLAR_TRANSFER_MERGER = 0,
  STELLAR_TRANSFER_DISK = 1,
  STELLAR_TRANSFER_OUTFLOW = 2,
  STELLAR_TRANSFER_COMPOSITE = 3
};

struct StellarTransferParameters {
  int mode;
  int palette_profile;
  double center[3];
  double axis[3];
  double box_size;
  float bulk_velocity_cm_per_s[3];
  float material_radius_cm;
  float disk_radius_cm;
  float disk_half_thickness_cm;
  float polar_inner_cm;
  float polar_outer_cm;
  float polar_cone_ratio;
  float merger_extinction_per_cm;
  float disk_extinction_per_cm;
  float polar_extinction_per_cm;
  float merger_emissivity_per_cm;
  float disk_emissivity_per_cm;
  float polar_emissivity_per_cm;
};

struct StellarOpticalSample {
  float emissivity_rgb_per_cm[3];
  float extinction_per_cm;
};

struct StellarIntegratedSegment {
  float radiance[3];
  float transmittance;
};

STELLAR_HD inline float stellarClamp(float value, float low, float high)
{
  return value < low ? low : (value > high ? high : value);
}

STELLAR_HD inline bool stellarFinite(float value)
{
#ifdef __CUDA_ARCH__
  return isfinite(value);
#else
  return std::isfinite(value);
#endif
}

STELLAR_HD inline float stellarSmoothstep(float low, float high, float value)
{
  if(!(high > low))
    return value >= high ? 1.0f : 0.0f;
  const float t = stellarClamp((value - low) / (high - low), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

STELLAR_HD inline double stellarPeriodicDelta(double value, double center, double box)
{
  double delta = value - center;
  if(delta > 0.5 * box)
    delta -= box;
  if(delta < -0.5 * box)
    delta += box;
  return delta;
}

STELLAR_HD inline void stellarTemperatureColor(float log_temperature, float output[3])
{
  const float stops[5] = {5.8f, 6.5f, 7.2f, 8.0f, 8.7f};
  const float colors[5][3] = {
      {0.34f, 0.035f, 0.008f},
      {0.72f, 0.13f, 0.025f},
      {1.00f, 0.52f, 0.10f},
      {1.00f, 0.92f, 0.67f},
      {0.64f, 0.79f, 1.00f}};
  int left = 0;
  while(left < 3 && log_temperature > stops[left + 1])
    left++;
  const float t = stellarSmoothstep(stops[left], stops[left + 1], log_temperature);
  for(int channel = 0; channel < 3; channel++)
    output[channel] = colors[left][channel] +
        t * (colors[left + 1][channel] - colors[left][channel]);
}

STELLAR_HD inline StellarOpticalSample evaluateStellarOpticalSample(
    const StellarTransferParameters &parameters, const double position[3],
    float density_log10_plus_10, float temperature_kelvin,
    const float velocity_cm_per_s[3])
{
  StellarOpticalSample output = {{0.0f, 0.0f, 0.0f}, 0.0f};
  if(!(temperature_kelvin > 0.0f) || !stellarFinite(temperature_kelvin) ||
     !stellarFinite(density_log10_plus_10))
    return output;

  const float log_density = density_log10_plus_10 - 10.0f;
  const float log_temperature = log10f(temperature_kelvin);
  const float thermal_support = stellarSmoothstep(5.7f, 6.15f, log_temperature) *
      (1.0f - stellarSmoothstep(8.72f, 8.95f, log_temperature));
  if(thermal_support <= 0.0f)
    return output;

  const double dx = stellarPeriodicDelta(position[0], parameters.center[0], parameters.box_size);
  const double dy = stellarPeriodicDelta(position[1], parameters.center[1], parameters.box_size);
  const double dz = stellarPeriodicDelta(position[2], parameters.center[2], parameters.box_size);
  const double height = dx * parameters.axis[0] + dy * parameters.axis[1] +
      dz * parameters.axis[2];
  const double radius_squared = dx * dx + dy * dy + dz * dz;
  const double cylindrical_squared = radius_squared - height * height;
  const float cylindrical_radius = float(sqrt(cylindrical_squared > 0.0 ? cylindrical_squared : 0.0));
  const float absolute_height = float(fabs(height));

  float relative_velocity[3];
  float speed_squared = 0.0f;
  float axial_velocity = 0.0f;
  float radial_velocity = 0.0f;
  for(int component = 0; component < 3; component++) {
    relative_velocity[component] = velocity_cm_per_s[component] -
        parameters.bulk_velocity_cm_per_s[component];
    speed_squared += relative_velocity[component] * relative_velocity[component];
    axial_velocity += relative_velocity[component] * float(parameters.axis[component]);
  }
  if(cylindrical_radius > 1.0e6f) {
    const double displacement[3] = {dx, dy, dz};
    for(int component = 0; component < 3; component++) {
      const float radial_component = float(displacement[component] -
          height * parameters.axis[component]) / cylindrical_radius;
      radial_velocity += relative_velocity[component] * radial_component;
    }
  }
  float azimuthal_squared = speed_squared - axial_velocity * axial_velocity -
      radial_velocity * radial_velocity;
  if(azimuthal_squared < 0.0f)
    azimuthal_squared = 0.0f;
  const float speed = sqrtf(speed_squared);
  const float azimuthal_velocity = sqrtf(azimuthal_squared);
  const float rotational_fraction = azimuthal_velocity / (speed + 1.0e4f);
  const float outward_axial_velocity = height >= 0.0 ? axial_velocity : -axial_velocity;
  const float outward_axial_fraction = fmaxf(0.0f, outward_axial_velocity) /
      (speed + 1.0e4f);

  const float merger_density = stellarSmoothstep(-3.2f, 2.0f, log_density);
  const float merger_radius = 1.0f - stellarSmoothstep(
      0.78f * parameters.material_radius_cm, parameters.material_radius_cm,
      float(sqrt(radius_squared)));
  const float merger_weight = thermal_support * merger_density * merger_radius;

  const float disk_plane = 1.0f - stellarSmoothstep(
      0.72f * parameters.disk_half_thickness_cm,
      1.30f * parameters.disk_half_thickness_cm, absolute_height);
  const float disk_radius = 1.0f - stellarSmoothstep(
      0.78f * parameters.disk_radius_cm, 1.12f * parameters.disk_radius_cm,
      cylindrical_radius);
  const float disk_density = 0.24f * stellarSmoothstep(-1.0f, 0.35f, log_density) +
      0.76f * stellarSmoothstep(0.35f, 2.05f, log_density);
  const float disk_temperature = (0.28f + 0.72f *
      stellarSmoothstep(6.75f, 7.85f, log_temperature)) *
      (1.0f - stellarSmoothstep(8.25f, 8.62f, log_temperature));
  const float disk_rotation = stellarSmoothstep(0.72f, 0.93f, rotational_fraction);
  const float disk_weight = disk_plane * disk_radius * disk_density * disk_temperature *
      disk_rotation;

  const float cone_coordinate = cylindrical_radius / (absolute_height + 1.0e6f);
  const float scaled_cone = cone_coordinate / parameters.polar_cone_ratio;
  const float axial_shape = 1.0f / (1.0f + scaled_cone * scaled_cone *
      scaled_cone * scaled_cone);
  const float polar_height = stellarSmoothstep(
      0.75f * parameters.polar_inner_cm, 1.8f * parameters.polar_inner_cm,
      absolute_height) *
      (1.0f - stellarSmoothstep(0.90f * parameters.polar_outer_cm,
                               1.08f * parameters.polar_outer_cm, absolute_height));
  const float polar_density = stellarSmoothstep(-3.4f, -0.6f, log_density) *
      (1.0f - stellarSmoothstep(0.2f, 0.9f, log_density));
  const float polar_temperature = (0.24f + 0.76f *
      stellarSmoothstep(5.95f, 7.25f, log_temperature)) *
      (1.0f - stellarSmoothstep(7.45f, 7.85f, log_temperature));
  const float polar_speed = stellarSmoothstep(5.0e7f, 4.0e8f,
                                               outward_axial_velocity);
  const float polar_coherence = stellarSmoothstep(0.45f, 0.90f,
                                                   outward_axial_fraction);
  const float polar_weight = axial_shape * polar_height * polar_density *
      polar_temperature * polar_speed * polar_coherence;

  float blackbody[3];
  stellarTemperatureColor(log_temperature, blackbody);
  const StellarPaletteStyle palette = stellarPaletteStyle(parameters.palette_profile);
  const float disk_color_fraction = stellarSmoothstep(0.20f, 1.85f, log_density);
  const float polar_color_fraction = stellarSmoothstep(-3.0f, -0.70f, log_density);
  float disk_color[3];
  float polar_color[3];
  float polar_accent_fraction = 0.0f;
  if(palette.polar_accent_enabled) {
    polar_accent_fraction =
        stellarSmoothstep(palette.polar_accent_temperature_low,
                          palette.polar_accent_temperature_high,
                          log_temperature) *
        stellarSmoothstep(palette.polar_accent_speed_low,
                          palette.polar_accent_speed_high,
                          outward_axial_velocity) *
        stellarSmoothstep(palette.polar_accent_coherence_low,
                          palette.polar_accent_coherence_high,
                          outward_axial_fraction);
  }
  for(int channel = 0; channel < 3; channel++) {
    const float disk_density_color = palette.disk_low_density[channel] +
        disk_color_fraction *
        (palette.disk_high_density[channel] - palette.disk_low_density[channel]);
    const float polar_density_color = palette.polar_low_density[channel] +
        polar_color_fraction *
        (palette.polar_high_density[channel] - palette.polar_low_density[channel]);
    const float polar_accent_color = palette.polar_accent_low_density[channel] +
        polar_color_fraction *
        (palette.polar_accent_high_density[channel] -
         palette.polar_accent_low_density[channel]);
    const float styled_polar_color = polar_density_color +
        polar_accent_fraction * (polar_accent_color - polar_density_color);
    disk_color[channel] = palette.disk_temperature_mix * blackbody[channel] +
        (1.0f - palette.disk_temperature_mix) * disk_density_color;
    polar_color[channel] = palette.polar_temperature_mix * blackbody[channel] +
        (1.0f - palette.polar_temperature_mix) * styled_polar_color;
  }
  float merger_mix = 0.0f;
  float disk_mix = 0.0f;
  float polar_mix = 0.0f;
  if(parameters.mode == STELLAR_TRANSFER_MERGER)
    merger_mix = merger_weight;
  else if(parameters.mode == STELLAR_TRANSFER_DISK)
    disk_mix = disk_weight;
  else if(parameters.mode == STELLAR_TRANSFER_OUTFLOW)
    polar_mix = polar_weight;
  else {
    merger_mix = palette.composite_merger_weight * merger_weight;
    disk_mix = palette.composite_disk_weight * disk_weight;
    polar_mix = palette.composite_polar_weight * polar_weight;
  }

  const float total = merger_mix + disk_mix + polar_mix;
  if(total <= 0.0f)
    return output;
  for(int channel = 0; channel < 3; channel++) {
    output.emissivity_rgb_per_cm[channel] =
        merger_mix * parameters.merger_emissivity_per_cm * blackbody[channel] +
        disk_mix * parameters.disk_emissivity_per_cm * disk_color[channel] +
        polar_mix * parameters.polar_emissivity_per_cm * polar_color[channel];
  }
  if(palette.neutralize_red_blue_overlap && polar_mix > 0.0f &&
     merger_mix + disk_mix > 0.0f) {
    const float neutral_bridge = fminf(output.emissivity_rgb_per_cm[0],
                                       output.emissivity_rgb_per_cm[2]);
    output.emissivity_rgb_per_cm[1] =
        fmaxf(output.emissivity_rgb_per_cm[1], neutral_bridge);
  }
  output.extinction_per_cm =
      merger_mix * parameters.merger_extinction_per_cm +
      disk_mix * parameters.disk_extinction_per_cm +
      polar_mix * parameters.polar_extinction_per_cm;
  return output;
}

STELLAR_HD inline StellarIntegratedSegment integrateStellarOpticalSegment(
    const StellarOpticalSample &sample, float segment_length_cm)
{
  StellarIntegratedSegment output = {{0.0f, 0.0f, 0.0f}, 1.0f};
  if(!(segment_length_cm > 0.0f) || !stellarFinite(segment_length_cm))
    return output;

  const float extinction = fmaxf(0.0f, sample.extinction_per_cm);
  const float optical_depth = extinction * segment_length_cm;
  output.transmittance = expf(-fminf(optical_depth, 80.0f));
  const float integral = extinction > 1.0e-30f ?
      (1.0f - output.transmittance) / extinction : segment_length_cm;
  for(int channel = 0; channel < 3; channel++) {
    const float emissivity = fmaxf(0.0f, sample.emissivity_rgb_per_cm[channel]);
    output.radiance[channel] = emissivity * integral;
  }
  return output;
}

STELLAR_HD inline StellarOpticalSample evaluateStellarOpticalSample(
    const StellarTransferParameters &parameters, const double position[3],
    float density_log10_plus_10, float temperature_kelvin)
{
  const float zero_velocity[3] = {0.0f, 0.0f, 0.0f};
  return evaluateStellarOpticalSample(parameters, position,
      density_log10_plus_10, temperature_kelvin, zero_velocity);
}

STELLAR_HD inline float stellarFilmicMap(float linear_value, float exposure,
                                         float black_point)
{
  float value = fmaxf(0.0f, linear_value * exposure - black_point);
  value = (value * (2.51f * value + 0.03f)) /
      (value * (2.43f * value + 0.59f) + 0.14f);
  value = stellarClamp(value, 0.0f, 1.0f);
  return powf(value, 1.0f / 2.2f);
}

#undef STELLAR_HD

#endif
