#ifndef AREPO_VTK_STELLAR_RENDER_MODEL_V052A_H
#define AREPO_VTK_STELLAR_RENDER_MODEL_V052A_H

#include <cmath>

#ifdef __CUDACC__
#define STELLAR_HD __host__ __device__
#else
#define STELLAR_HD
#endif

#include "stellar_palette_v057.h"
#include "stellar_feature_profile_v065.h"

enum StellarTransferMode {
  STELLAR_TRANSFER_MERGER = 0,
  STELLAR_TRANSFER_DISK = 1,
  STELLAR_TRANSFER_OUTFLOW = 2,
  STELLAR_TRANSFER_COMPOSITE = 3
};

struct StellarTransferParameters {
  int mode;
  int palette_profile;
  int feature_profile;
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

struct StellarFeatureSampleV064 {
  float log_density;
  float log_temperature;
  float thermal_support;
  float cylindrical_radius_cm;
  float signed_height_cm;
  float absolute_height_cm;
  float speed_cm_per_s;
  float radial_velocity_cm_per_s;
  float azimuthal_velocity_cm_per_s;
  float rotational_fraction;
  float outward_axial_velocity_cm_per_s;
  float outward_axial_fraction;
  float merger_weight;
  float disk_weight;
  float polar_weight;
  float disk_annulus_support;
  float disk_density_retention;
  float polar_confidence_retention;
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

STELLAR_HD inline StellarFeatureSampleV064 evaluateStellarFeatureSampleV064(
    const StellarTransferParameters &parameters, const double position[3],
    float density_log10_plus_10, float temperature_kelvin,
    const float velocity_cm_per_s[3])
{
  StellarFeatureSampleV064 output = {};
  if(!(temperature_kelvin > 0.0f) || !stellarFinite(temperature_kelvin) ||
     !stellarFinite(density_log10_plus_10))
    return output;

  output.log_density = density_log10_plus_10 - 10.0f;
  output.log_temperature = log10f(temperature_kelvin);
  output.thermal_support =
      stellarSmoothstep(5.7f, 6.15f, output.log_temperature) *
      (1.0f - stellarSmoothstep(8.72f, 8.95f, output.log_temperature));
  if(output.thermal_support <= 0.0f)
    return output;

  const double dx = stellarPeriodicDelta(
      position[0], parameters.center[0], parameters.box_size);
  const double dy = stellarPeriodicDelta(
      position[1], parameters.center[1], parameters.box_size);
  const double dz = stellarPeriodicDelta(
      position[2], parameters.center[2], parameters.box_size);
  const double height = dx * parameters.axis[0] +
      dy * parameters.axis[1] + dz * parameters.axis[2];
  const double radius_squared = dx * dx + dy * dy + dz * dz;
  const double cylindrical_squared = radius_squared - height * height;
  output.cylindrical_radius_cm = float(sqrt(
      cylindrical_squared > 0.0 ? cylindrical_squared : 0.0));
  output.signed_height_cm = float(height);
  output.absolute_height_cm = float(fabs(height));

  float relative_velocity[3];
  float speed_squared = 0.0f;
  float axial_velocity = 0.0f;
  for(int component = 0; component < 3; component++) {
    relative_velocity[component] = velocity_cm_per_s[component] -
        parameters.bulk_velocity_cm_per_s[component];
    speed_squared += relative_velocity[component] * relative_velocity[component];
    axial_velocity += relative_velocity[component] *
        float(parameters.axis[component]);
  }
  if(output.cylindrical_radius_cm > 1.0e6f) {
    const double displacement[3] = {dx, dy, dz};
    for(int component = 0; component < 3; component++) {
      const float radial_component = float(displacement[component] -
          height * parameters.axis[component]) /
          output.cylindrical_radius_cm;
      output.radial_velocity_cm_per_s +=
          relative_velocity[component] * radial_component;
    }
  }
  float azimuthal_squared = speed_squared - axial_velocity * axial_velocity -
      output.radial_velocity_cm_per_s * output.radial_velocity_cm_per_s;
  if(azimuthal_squared < 0.0f)
    azimuthal_squared = 0.0f;
  output.speed_cm_per_s = sqrtf(speed_squared);
  output.azimuthal_velocity_cm_per_s = sqrtf(azimuthal_squared);
  output.rotational_fraction = output.azimuthal_velocity_cm_per_s /
      (output.speed_cm_per_s + 1.0e4f);
  output.outward_axial_velocity_cm_per_s =
      height >= 0.0 ? axial_velocity : -axial_velocity;
  output.outward_axial_fraction =
      fmaxf(0.0f, output.outward_axial_velocity_cm_per_s) /
      (output.speed_cm_per_s + 1.0e4f);

  const float merger_density =
      stellarSmoothstep(-3.2f, 2.0f, output.log_density);
  const float merger_radius = 1.0f - stellarSmoothstep(
      0.78f * parameters.material_radius_cm, parameters.material_radius_cm,
      float(sqrt(radius_squared)));
  output.merger_weight =
      output.thermal_support * merger_density * merger_radius;

  const float disk_plane = 1.0f - stellarSmoothstep(
      0.72f * parameters.disk_half_thickness_cm,
      1.30f * parameters.disk_half_thickness_cm,
      output.absolute_height_cm);
  const float disk_radius = 1.0f - stellarSmoothstep(
      0.78f * parameters.disk_radius_cm, 1.12f * parameters.disk_radius_cm,
      output.cylindrical_radius_cm);
  const float disk_density =
      0.24f * stellarSmoothstep(-1.0f, 0.35f, output.log_density) +
      0.76f * stellarSmoothstep(0.35f, 2.05f, output.log_density);
  const float disk_temperature = (0.28f + 0.72f *
      stellarSmoothstep(6.75f, 7.85f, output.log_temperature)) *
      (1.0f - stellarSmoothstep(8.25f, 8.62f, output.log_temperature));
  const float disk_rotation =
      stellarSmoothstep(0.72f, 0.93f, output.rotational_fraction);
  output.disk_weight = disk_plane * disk_radius * disk_density *
      disk_temperature * disk_rotation;
  output.disk_annulus_support = 1.0f;
  output.disk_density_retention = 1.0f;

  const float cone_coordinate = output.cylindrical_radius_cm /
      (output.absolute_height_cm + 1.0e6f);
  const float scaled_cone = cone_coordinate / parameters.polar_cone_ratio;
  const float axial_shape = 1.0f /
      (1.0f + scaled_cone * scaled_cone * scaled_cone * scaled_cone);
  const float polar_height = stellarSmoothstep(
      0.75f * parameters.polar_inner_cm, 1.8f * parameters.polar_inner_cm,
      output.absolute_height_cm) *
      (1.0f - stellarSmoothstep(0.90f * parameters.polar_outer_cm,
                               1.08f * parameters.polar_outer_cm,
                               output.absolute_height_cm));
  const float polar_density =
      stellarSmoothstep(-3.4f, -0.6f, output.log_density) *
      (1.0f - stellarSmoothstep(0.2f, 0.9f, output.log_density));
  const float polar_temperature = (0.24f + 0.76f *
      stellarSmoothstep(5.95f, 7.25f, output.log_temperature)) *
      (1.0f - stellarSmoothstep(7.45f, 7.85f, output.log_temperature));
  const float polar_speed = stellarSmoothstep(
      5.0e7f, 4.0e8f, output.outward_axial_velocity_cm_per_s);
  const float polar_coherence = stellarSmoothstep(
      0.45f, 0.90f, output.outward_axial_fraction);
  output.polar_weight = axial_shape * polar_height * polar_density *
      polar_temperature * polar_speed * polar_coherence;
  output.polar_confidence_retention = 1.0f;
  if(parameters.feature_profile == STELLAR_FEATURE_STRUCTURES_V065) {
    const StellarFeatureProfileStyleV065 style =
        stellarFeatureProfileStyleV065(parameters.feature_profile);
    const float disk_radius_fraction = output.cylindrical_radius_cm /
        parameters.disk_radius_cm;
    output.disk_annulus_support = stellarSmoothstep(
        style.disk_inner_start_fraction, style.disk_inner_full_fraction,
        disk_radius_fraction);
    output.disk_density_retention = 1.0f -
        (1.0f - style.disk_density_floor) * stellarSmoothstep(
            style.disk_density_taper_low, style.disk_density_taper_high,
            output.log_density);
    output.disk_weight *= output.disk_annulus_support *
        output.disk_density_retention;
    output.polar_confidence_retention = style.polar_envelope_floor +
        (1.0f - style.polar_envelope_floor) * stellarSmoothstep(
            style.polar_confidence_low, style.polar_confidence_high,
            output.polar_weight);
    output.polar_weight *= output.polar_confidence_retention;
  }
  return output;
}

STELLAR_HD inline StellarOpticalSample evaluateStellarOpticalSample(
    const StellarTransferParameters &parameters, const double position[3],
    float density_log10_plus_10, float temperature_kelvin,
    const float velocity_cm_per_s[3])
{
  StellarOpticalSample output = {{0.0f, 0.0f, 0.0f}, 0.0f};
  const StellarFeatureSampleV064 feature = evaluateStellarFeatureSampleV064(
      parameters, position, density_log10_plus_10, temperature_kelvin,
      velocity_cm_per_s);
  if(feature.thermal_support <= 0.0f)
    return output;

  float blackbody[3];
  stellarTemperatureColor(feature.log_temperature, blackbody);
  const StellarPaletteStyle palette = stellarPaletteStyle(parameters.palette_profile);
  const float disk_color_fraction =
      stellarSmoothstep(0.20f, 1.85f, feature.log_density);
  const float polar_color_fraction =
      stellarSmoothstep(-3.0f, -0.70f, feature.log_density);
  float disk_color[3];
  float polar_color[3];
  float polar_accent_fraction = 0.0f;
  if(palette.polar_accent_enabled) {
    polar_accent_fraction =
        stellarSmoothstep(palette.polar_accent_temperature_low,
                          palette.polar_accent_temperature_high,
                          feature.log_temperature) *
        stellarSmoothstep(palette.polar_accent_speed_low,
                          palette.polar_accent_speed_high,
                          feature.outward_axial_velocity_cm_per_s) *
        stellarSmoothstep(palette.polar_accent_coherence_low,
                          palette.polar_accent_coherence_high,
                          feature.outward_axial_fraction);
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
  float merger_emissivity_scale = 1.0f;
  float disk_emissivity_scale = 1.0f;
  float disk_extinction_scale = 1.0f;
  float polar_emissivity_scale = 1.0f;
  float polar_extinction_scale = 1.0f;
  if(palette.kinematic_optical_enabled) {
    const float radial_fraction = fabsf(feature.radial_velocity_cm_per_s) /
        (feature.speed_cm_per_s + 1.0e4f);
    const float disk_density_texture = stellarSmoothstep(
        palette.disk_density_texture_low,
        palette.disk_density_texture_high, feature.log_density);
    const float disk_radial_texture = stellarSmoothstep(
        palette.disk_radial_fraction_low,
        palette.disk_radial_fraction_high, radial_fraction);
    const float disk_rotation_texture = stellarSmoothstep(
        0.72f, 0.97f, feature.rotational_fraction);
    const float polar_flux_density = stellarSmoothstep(
        palette.polar_flux_density_low, palette.polar_flux_density_high,
        feature.log_density);
    const float polar_flux_speed = stellarSmoothstep(
        palette.polar_flux_speed_low, palette.polar_flux_speed_high,
        feature.outward_axial_velocity_cm_per_s);
    const float polar_flux_coherence = stellarSmoothstep(
        palette.polar_flux_coherence_low,
        palette.polar_flux_coherence_high,
        feature.outward_axial_fraction);
    const float normalized_mass_flux = polar_flux_density *
        polar_flux_speed * polar_flux_coherence;
    const float disk_stream_mix = palette.disk_stream_color_mix *
        disk_radial_texture;
    const float polar_flux_mix = palette.polar_flux_color_mix *
        normalized_mass_flux;
    for(int channel = 0; channel < 3; channel++) {
      disk_color[channel] += disk_stream_mix *
          (palette.disk_stream_color[channel] - disk_color[channel]);
      polar_color[channel] += polar_flux_mix *
          (palette.polar_flux_color[channel] - polar_color[channel]);
    }
    merger_emissivity_scale = palette.merger_emissivity_floor +
        palette.merger_radial_emissivity_gain * disk_radial_texture;
    disk_emissivity_scale = palette.disk_emissivity_floor +
        palette.disk_density_emissivity_gain * disk_density_texture +
        palette.disk_radial_emissivity_gain * disk_radial_texture;
    disk_extinction_scale = palette.disk_extinction_floor +
        palette.disk_density_extinction_gain * disk_density_texture +
        palette.disk_rotation_extinction_gain * disk_rotation_texture;
    polar_emissivity_scale = palette.polar_emissivity_floor +
        palette.polar_mass_flux_emissivity_gain * normalized_mass_flux;
    polar_extinction_scale = palette.polar_extinction_scale;
  }
  float merger_mix = 0.0f;
  float disk_mix = 0.0f;
  float polar_mix = 0.0f;
  if(parameters.mode == STELLAR_TRANSFER_MERGER)
    merger_mix = feature.merger_weight;
  else if(parameters.mode == STELLAR_TRANSFER_DISK)
    disk_mix = feature.disk_weight;
  else if(parameters.mode == STELLAR_TRANSFER_OUTFLOW)
    polar_mix = feature.polar_weight;
  else {
    merger_mix = palette.composite_merger_weight * feature.merger_weight;
    disk_mix = palette.composite_disk_weight * feature.disk_weight;
    polar_mix = palette.composite_polar_weight * feature.polar_weight;
  }

  const float total = merger_mix + disk_mix + polar_mix;
  if(total <= 0.0f)
    return output;
  if(palette.kinematic_optical_enabled) {
    for(int channel = 0; channel < 3; channel++) {
      output.emissivity_rgb_per_cm[channel] =
          merger_mix * parameters.merger_emissivity_per_cm *
              merger_emissivity_scale * blackbody[channel] +
          disk_mix * parameters.disk_emissivity_per_cm *
              disk_emissivity_scale * disk_color[channel] +
          polar_mix * parameters.polar_emissivity_per_cm *
              polar_emissivity_scale * polar_color[channel];
    }
  } else {
    for(int channel = 0; channel < 3; channel++) {
      output.emissivity_rgb_per_cm[channel] =
          merger_mix * parameters.merger_emissivity_per_cm *
              blackbody[channel] +
          disk_mix * parameters.disk_emissivity_per_cm * disk_color[channel] +
          polar_mix * parameters.polar_emissivity_per_cm *
              polar_color[channel];
    }
  }
  if(palette.neutralize_red_blue_overlap && polar_mix > 0.0f &&
     merger_mix + disk_mix > 0.0f) {
    const float neutral_bridge = fminf(output.emissivity_rgb_per_cm[0],
                                       output.emissivity_rgb_per_cm[2]);
    output.emissivity_rgb_per_cm[1] =
        fmaxf(output.emissivity_rgb_per_cm[1], neutral_bridge);
  }
  if(palette.kinematic_optical_enabled) {
    output.extinction_per_cm =
        merger_mix * parameters.merger_extinction_per_cm +
        disk_mix * parameters.disk_extinction_per_cm *
            disk_extinction_scale +
        polar_mix * parameters.polar_extinction_per_cm *
            polar_extinction_scale;
  } else {
    output.extinction_per_cm =
        merger_mix * parameters.merger_extinction_per_cm +
        disk_mix * parameters.disk_extinction_per_cm +
        polar_mix * parameters.polar_extinction_per_cm;
  }
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
