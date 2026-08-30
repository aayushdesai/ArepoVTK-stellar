#ifndef AREPO_VTK_STELLAR_HERO_PROJECTION_V081_H
#define AREPO_VTK_STELLAR_HERO_PROJECTION_V081_H

#include <cmath>
#include <string>

#include "stellar_hero_projection_v080.h"

#ifdef __CUDACC__
#define STELLAR_HERO_V081_HD __host__ __device__
#else
#define STELLAR_HERO_V081_HD
#endif

enum StellarRenderProgramV081 {
  STELLAR_RENDER_PROGRAM_INVALID_V081 = -1,
  STELLAR_RENDER_PROGRAM_RETAINED_V078_V081 = 0,
  STELLAR_RENDER_PROGRAM_HERO_ADAPTIVE_COPPER_BLUE_V081 = 1
};

struct StellarHeroDisplayContractV081 {
  float reference_quantile;
  float response_floor_fraction;
  float material_display_gain;
  float outflow_display_gain;
};

struct StellarHeroSurfaceSampleV081 {
  float material_peak_density_g_cm3;
  float material_rotation_fraction;
  float material_hot_support;
  float outflow_peak_flux_g_cm2_s;
};

inline int stellarRenderProgramFromNameV081(const std::string &name)
{
  if(name == "retained_v078")
    return STELLAR_RENDER_PROGRAM_RETAINED_V078_V081;
  if(name == "hero_adaptive_copper_blue_v081")
    return STELLAR_RENDER_PROGRAM_HERO_ADAPTIVE_COPPER_BLUE_V081;
  return STELLAR_RENDER_PROGRAM_INVALID_V081;
}

inline const char *stellarRenderProgramNameV081(int program)
{
  if(program == STELLAR_RENDER_PROGRAM_RETAINED_V078_V081)
    return "retained_v078";
  if(program == STELLAR_RENDER_PROGRAM_HERO_ADAPTIVE_COPPER_BLUE_V081)
    return "hero_adaptive_copper_blue_v081";
  return "invalid_v081";
}

inline bool stellarHeroDisplayContractSha256ValidV081(
    const std::string &value)
{
  return stellarHeroProjectionCalibrationSha256ValidV080(value);
}

STELLAR_HERO_V081_HD inline bool stellarHeroDisplayContractValidV081(
    const StellarHeroDisplayContractV081 &contract)
{
  return stellarFinite(contract.reference_quantile) &&
      contract.reference_quantile >= 0.90f &&
      contract.reference_quantile <= 1.0f &&
      stellarFinite(contract.response_floor_fraction) &&
      contract.response_floor_fraction >= 0.0f &&
      contract.response_floor_fraction < 0.25f &&
      stellarFinite(contract.material_display_gain) &&
      contract.material_display_gain > 0.0f &&
      contract.material_display_gain <= 1.0f &&
      stellarFinite(contract.outflow_display_gain) &&
      contract.outflow_display_gain > 0.0f &&
      contract.outflow_display_gain <= 1.0f;
}

STELLAR_HERO_V081_HD inline float stellarHeroAdaptiveResponseV081(
    float value, float reference, float floor_fraction, float gain)
{
  if(!(value > 0.0f) || !(reference > 0.0f) || !(gain > 0.0f) ||
     !stellarFinite(value) || !stellarFinite(reference) ||
     !stellarFinite(floor_fraction) || !stellarFinite(gain))
    return 0.0f;
  const float normalized = stellarClamp(value / reference, 0.0f, 1.0f);
  if(!(normalized > floor_fraction))
    return 0.0f;
  const float visible = (normalized - floor_fraction) /
      fmaxf(1.0f - floor_fraction, 1.0e-6f);
  const float compressed = log1pf(4.0f * visible) / logf(5.0f);
  return stellarClamp(gain * compressed, 0.0f, 1.0f);
}

STELLAR_HERO_V081_HD inline StellarHeroSurfaceSampleV081
stellarEmptyHeroSurfaceSampleV081()
{
  const StellarHeroSurfaceSampleV081 output = {0.0f, 0.0f, 0.0f, 0.0f};
  return output;
}

STELLAR_HERO_V081_HD inline void stellarUpdateHeroSurfaceV081(
    StellarHeroSurfaceSampleV081 *surface,
    const StellarHeroProjectionSampleV080 &sample)
{
  if(!surface)
    return;
  if(stellarFinite(sample.material_density_g_cm3) &&
     sample.material_density_g_cm3 > surface->material_peak_density_g_cm3) {
    surface->material_peak_density_g_cm3 = sample.material_density_g_cm3;
    surface->material_rotation_fraction = stellarClamp(
        sample.styled_rotation_density_g_cm3 /
            sample.material_density_g_cm3,
        0.0f, 1.0f);
    surface->material_hot_support = stellarClamp(
        sample.hot_material_density_g_cm3 /
            sample.material_density_g_cm3,
        0.0f, 1.0f);
  }
  if(stellarFinite(sample.outward_mass_flux_g_cm2_s) &&
     sample.outward_mass_flux_g_cm2_s > surface->outflow_peak_flux_g_cm2_s)
    surface->outflow_peak_flux_g_cm2_s =
        sample.outward_mass_flux_g_cm2_s;
}

STELLAR_HERO_V081_HD inline bool stellarHeroSurfaceFiniteV081(
    const StellarHeroSurfaceSampleV081 &surface)
{
  return stellarFinite(surface.material_peak_density_g_cm3) &&
      stellarFinite(surface.material_rotation_fraction) &&
      stellarFinite(surface.material_hot_support) &&
      stellarFinite(surface.outflow_peak_flux_g_cm2_s);
}

STELLAR_HERO_V081_HD inline void stellarHeroBackgroundV081(float output[3])
{
  output[0] = 0.015f;
  output[1] = 0.022f;
  output[2] = 0.030f;
}

// V081 evaluates the exact v080 physical sample definitions but retains only
// the strongest material and coherent-outflow sample along each Voronoi ray.
// This maximum-intensity surface projection is intentionally sharp and is not
// a claim of frequency-dependent radiative transfer.
STELLAR_HERO_V081_HD inline void stellarDecodeHeroSurfaceV081(
    const StellarHeroSurfaceSampleV081 &surface,
    float material_reference_g_cm3,
    float outflow_reference_g_cm2_s,
    const StellarHeroDisplayContractV081 &contract,
    float output[3])
{
  stellarHeroBackgroundV081(output);
  if(!stellarHeroDisplayContractValidV081(contract))
    return;

  const float material_amplitude = stellarHeroAdaptiveResponseV081(
      surface.material_peak_density_g_cm3, material_reference_g_cm3,
      contract.response_floor_fraction, contract.material_display_gain);
  const float outflow_amplitude = stellarHeroAdaptiveResponseV081(
      surface.outflow_peak_flux_g_cm2_s, outflow_reference_g_cm2_s,
      contract.response_floor_fraction, contract.outflow_display_gain);
  if(!(material_amplitude > 0.0f) && !(outflow_amplitude > 0.0f))
    return;

  float material_color[3];
  float outflow_color[3];
  const float material_palette_fraction = fminf(
      2.0f / 3.0f,
      fmaxf(surface.material_rotation_fraction,
            (2.0f / 3.0f) * surface.material_hot_support));
  stellarCopperBlueV071(material_palette_fraction, material_color);
  stellarCopperBlueV071(1.0f / 3.0f, outflow_color);

  const float weighted_outflow = 1.25f * outflow_amplitude;
  const bool outflow_dominant = weighted_outflow > material_amplitude;
  const float dominant_amplitude = outflow_dominant ?
      weighted_outflow : material_amplitude;
  const float secondary_amplitude = outflow_dominant ?
      material_amplitude : weighted_outflow;
  const float secondary_mix = dominant_amplitude > 0.0f ?
      0.08f * secondary_amplitude / dominant_amplitude : 0.0f;
  const float hot_boost = 0.12f * sqrtf(
      stellarClamp(surface.material_hot_support, 0.0f, 1.0f));
  const float intensity = stellarClamp(
      fmaxf(material_amplitude, outflow_amplitude) +
          0.20f * fminf(material_amplitude, outflow_amplitude) +
          hot_boost * material_amplitude,
      0.0f, 1.0f);
  float background[3];
  stellarHeroBackgroundV081(background);
  for(int component = 0; component < 3; component++) {
    const float primary = outflow_dominant ?
        outflow_color[component] : material_color[component];
    const float secondary = outflow_dominant ?
        material_color[component] : outflow_color[component];
    const float color = primary + secondary_mix * (secondary - primary);
    output[component] = background[component] +
        intensity * (color - background[component]);
  }
}

#undef STELLAR_HERO_V081_HD

#endif
