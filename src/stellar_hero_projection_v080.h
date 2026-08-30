#ifndef AREPO_VTK_STELLAR_HERO_PROJECTION_V080_H
#define AREPO_VTK_STELLAR_HERO_PROJECTION_V080_H

#include <cmath>
#include <string>

#include "stellar_hero_transfer_v079.h"

#ifdef __CUDACC__
#define STELLAR_HERO_V080_HD __host__ __device__
#else
#define STELLAR_HERO_V080_HD
#endif

enum StellarRenderProgramV080 {
  STELLAR_RENDER_PROGRAM_INVALID_V080 = -1,
  STELLAR_RENDER_PROGRAM_RETAINED_V078_V080 = 0,
  STELLAR_RENDER_PROGRAM_HERO_PROJECTED_COPPER_BLUE_V080 = 1
};

struct StellarHeroProjectionCalibrationV080 {
  float material_column_reference_g_cm2;
  float material_display_gain;
  float outflow_column_flux_reference_g_cm_s;
  float outflow_display_gain;
};

struct StellarHeroProjectionMomentsV080 {
  float material_column_g_cm2;
  float styled_rotation_column_g_cm2;
  float hot_material_column_g_cm2;
  float outflow_column_flux_g_cm_s;
};

struct StellarHeroProjectionSampleV080 {
  float material_density_g_cm3;
  float styled_rotation_density_g_cm3;
  float hot_material_density_g_cm3;
  float outward_mass_flux_g_cm2_s;
};

inline int stellarRenderProgramFromNameV080(const std::string &name)
{
  if(name == "retained_v078")
    return STELLAR_RENDER_PROGRAM_RETAINED_V078_V080;
  if(name == "hero_projected_copper_blue_v080")
    return STELLAR_RENDER_PROGRAM_HERO_PROJECTED_COPPER_BLUE_V080;
  return STELLAR_RENDER_PROGRAM_INVALID_V080;
}

inline const char *stellarRenderProgramNameV080(int program)
{
  if(program == STELLAR_RENDER_PROGRAM_RETAINED_V078_V080)
    return "retained_v078";
  if(program == STELLAR_RENDER_PROGRAM_HERO_PROJECTED_COPPER_BLUE_V080)
    return "hero_projected_copper_blue_v080";
  return "invalid_v080";
}

inline bool stellarHeroProjectionCalibrationSha256ValidV080(
    const std::string &value)
{
  return stellarHeroCalibrationSha256ValidV079(value);
}

STELLAR_HERO_V080_HD inline bool stellarHeroProjectionCalibrationValidV080(
    const StellarHeroProjectionCalibrationV080 &calibration)
{
  return stellarFinite(calibration.material_column_reference_g_cm2) &&
      calibration.material_column_reference_g_cm2 > 0.0f &&
      stellarFinite(calibration.material_display_gain) &&
      calibration.material_display_gain > 0.0f &&
      calibration.material_display_gain <= 1.0f &&
      stellarFinite(calibration.outflow_column_flux_reference_g_cm_s) &&
      calibration.outflow_column_flux_reference_g_cm_s > 0.0f &&
      stellarFinite(calibration.outflow_display_gain) &&
      calibration.outflow_display_gain > 0.0f &&
      calibration.outflow_display_gain <= 1.0f;
}

STELLAR_HERO_V080_HD inline StellarHeroProjectionMomentsV080
stellarEmptyHeroProjectionMomentsV080()
{
  const StellarHeroProjectionMomentsV080 output = {0.0f, 0.0f, 0.0f, 0.0f};
  return output;
}

STELLAR_HERO_V080_HD inline StellarHeroProjectionSampleV080
stellarHeroProjectionSampleV080(
    const StellarPhysicalSampleV071 &physical,
    const StellarPhysicalTransferV071 &transfer,
    const StellarPhysicalOpticalParametersV076 &style)
{
  StellarHeroProjectionSampleV080 output = {0.0f, 0.0f, 0.0f, 0.0f};
  if(!(physical.density_cgs > 0.0f) ||
     !stellarFinite(physical.density_cgs) ||
     !(transfer.range_max > transfer.range_min))
    return output;

  const bool rotation_visible =
      stellarFinite(physical.rotational_fraction) &&
      physical.rotational_fraction >= transfer.range_min &&
      physical.rotational_fraction <= transfer.range_max;
  const float fraction = rotation_visible ? stellarClamp(
      (physical.rotational_fraction - transfer.range_min) /
          (transfer.range_max - transfer.range_min),
      0.0f, 1.0f) : 0.0f;
  const float styled_rotation = stellarPhysicalStyledFractionV075(
      fraction, style.color_gamma, style.color_invert);
  const float log_temperature = physical.temperature_kelvin > 0.0f ?
      log10f(physical.temperature_kelvin) : -INFINITY;
  const float hot_support = stellarSmoothstep(8.0f, 8.8f, log_temperature);

  // This is a diagnostic projection support, not opacity. The reviewed
  // rotational range matches the WebGL visibility contract; hot dense matter
  // is admitted separately so the white-dwarf cores cannot disappear merely
  // because their local velocity decomposition falls outside that range.
  const float material_support = fmaxf(rotation_visible ? 1.0f : 0.0f,
                                       hot_support);
  output.material_density_g_cm3 =
      physical.density_cgs * material_support;
  output.styled_rotation_density_g_cm3 =
      output.material_density_g_cm3 * styled_rotation;
  output.hot_material_density_g_cm3 =
      physical.density_cgs * hot_support;

  const float speed = fmaxf(physical.speed_cm_per_s, 1.0e4f);
  const float outward = fmaxf(
      physical.outward_axial_velocity_cm_per_s, 0.0f);
  const float coherence = stellarClamp(outward / speed, 0.0f, 1.0f);
  output.outward_mass_flux_g_cm2_s =
      physical.density_cgs * outward * coherence * coherence;
  return output;
}

STELLAR_HERO_V080_HD inline void stellarAccumulateHeroProjectionV080(
    StellarHeroProjectionMomentsV080 *moments,
    const StellarHeroProjectionSampleV080 &sample, float path_length_cm)
{
  if(!moments || !(path_length_cm > 0.0f) ||
     !stellarFinite(path_length_cm))
    return;
  moments->material_column_g_cm2 +=
      sample.material_density_g_cm3 * path_length_cm;
  moments->styled_rotation_column_g_cm2 +=
      sample.styled_rotation_density_g_cm3 * path_length_cm;
  moments->hot_material_column_g_cm2 +=
      sample.hot_material_density_g_cm3 * path_length_cm;
  moments->outflow_column_flux_g_cm_s +=
      sample.outward_mass_flux_g_cm2_s * path_length_cm;
}

STELLAR_HERO_V080_HD inline float stellarHeroProjectedResponseV080(
    float value, float reference, float gain)
{
  if(!(value > 0.0f) || !(reference > 0.0f) || !(gain > 0.0f) ||
     !stellarFinite(value) || !stellarFinite(reference) ||
     !stellarFinite(gain))
    return 0.0f;
  const float normalized = value / reference;
  const float visible = fmaxf(normalized - 0.02f, 0.0f);
  const float compressed = log1pf(8.0f * visible) / logf(9.0f);
  return stellarClamp(gain * compressed, 0.0f, 1.0f);
}

// Copper-blue is applied once after the exact Voronoi traversal. This avoids
// treating a false-color lookup table as a physical emission spectrum and
// therefore avoids the gray/white color mixing seen in v079.
STELLAR_HERO_V080_HD inline void stellarDecodeHeroProjectionV080(
    const StellarHeroProjectionMomentsV080 &moments,
    const StellarHeroProjectionCalibrationV080 &calibration,
    float output[3])
{
  output[0] = output[1] = output[2] = 0.0f;
  if(!stellarHeroProjectionCalibrationValidV080(calibration))
    return;

  const float material_amplitude = stellarHeroProjectedResponseV080(
      moments.material_column_g_cm2,
      calibration.material_column_reference_g_cm2,
      calibration.material_display_gain);
  const float outflow_amplitude = stellarHeroProjectedResponseV080(
      moments.outflow_column_flux_g_cm_s,
      calibration.outflow_column_flux_reference_g_cm_s,
      calibration.outflow_display_gain);
  if(!(material_amplitude > 0.0f) && !(outflow_amplitude > 0.0f))
    return;

  const float mean_rotation = moments.material_column_g_cm2 > 0.0f ?
      stellarClamp(moments.styled_rotation_column_g_cm2 /
                       moments.material_column_g_cm2,
                   0.0f, 1.0f) : 0.0f;
  const float hot_fraction = moments.material_column_g_cm2 > 0.0f ?
      stellarClamp(moments.hot_material_column_g_cm2 /
                       moments.material_column_g_cm2,
                   0.0f, 1.0f) : 0.0f;
  float material_color[3];
  float outflow_color[3];
  stellarCopperBlueV071(mean_rotation, material_color);
  stellarCopperBlueV071(1.0f / 3.0f, outflow_color);
  const float hot_mix = 0.62f * sqrtf(hot_fraction);
  const float hot_color[3] = {1.0f, 0.93f, 0.78f};
  for(int component = 0; component < 3; component++)
    material_color[component] += hot_mix *
        (hot_color[component] - material_color[component]);

  const float color_weight = material_amplitude + 1.25f * outflow_amplitude;
  if(!(color_weight > 0.0f))
    return;
  const float intensity = stellarClamp(
      fmaxf(material_amplitude, outflow_amplitude) +
          0.25f * fminf(material_amplitude, outflow_amplitude),
      0.0f, 1.0f);
  for(int component = 0; component < 3; component++) {
    const float color = (material_amplitude * material_color[component] +
        1.25f * outflow_amplitude * outflow_color[component]) / color_weight;
    output[component] = intensity * color;
  }
}

STELLAR_HERO_V080_HD inline void stellarDecodeRenderProgramV080(
    int render_program, const float input[3],
    const StellarPhysicalOpticalParametersV076 &optical_parameters,
    float output[3])
{
  if(render_program ==
     STELLAR_RENDER_PROGRAM_HERO_PROJECTED_COPPER_BLUE_V080) {
    output[0] = input[0];
    output[1] = input[1];
    output[2] = input[2];
    return;
  }
  stellarDecodePhysicalMomentsV078(input, optical_parameters, output);
}

#undef STELLAR_HERO_V080_HD

#endif
