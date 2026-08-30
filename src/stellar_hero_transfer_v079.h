#ifndef AREPO_VTK_STELLAR_HERO_TRANSFER_V079_H
#define AREPO_VTK_STELLAR_HERO_TRANSFER_V079_H

#include <cmath>
#include <string>

#include "stellar_physical_optical_v078.h"

#ifdef __CUDACC__
#define STELLAR_HERO_V079_HD __host__ __device__
#else
#define STELLAR_HERO_V079_HD
#endif

enum StellarRenderProgramV079 {
  STELLAR_RENDER_PROGRAM_INVALID_V079 = -1,
  STELLAR_RENDER_PROGRAM_RETAINED_V078 = 0,
  STELLAR_RENDER_PROGRAM_HERO_MATERIAL_OUTFLOW_V079 = 1
};

struct StellarHeroCalibrationV079 {
  float material_column_reference_g_cm2;
  float material_optical_depth_at_reference;
  float outflow_column_flux_reference_g_cm_s;
  float outflow_emission_at_reference;
};

struct StellarHeroPhysicalTermsV079 {
  float material_density_g_cm3;
  float outward_mass_flux_g_cm2_s;
  float outward_coherence;
};

inline int stellarRenderProgramFromNameV079(const std::string &name)
{
  if(name == "retained_v078")
    return STELLAR_RENDER_PROGRAM_RETAINED_V078;
  if(name == "hero_material_outflow_v079")
    return STELLAR_RENDER_PROGRAM_HERO_MATERIAL_OUTFLOW_V079;
  return STELLAR_RENDER_PROGRAM_INVALID_V079;
}

inline const char *stellarRenderProgramNameV079(int program)
{
  if(program == STELLAR_RENDER_PROGRAM_RETAINED_V078)
    return "retained_v078";
  if(program == STELLAR_RENDER_PROGRAM_HERO_MATERIAL_OUTFLOW_V079)
    return "hero_material_outflow_v079";
  return "invalid_v079";
}

inline bool stellarHeroCalibrationSha256ValidV079(const std::string &value)
{
  if(value.size() != 64)
    return false;
  for(size_t index = 0; index < value.size(); index++) {
    const char character = value[index];
    if(!((character >= '0' && character <= '9') ||
         (character >= 'a' && character <= 'f')))
      return false;
  }
  return true;
}

STELLAR_HERO_V079_HD inline bool stellarHeroCalibrationValidV079(
    const StellarHeroCalibrationV079 &calibration)
{
  return stellarFinite(calibration.material_column_reference_g_cm2) &&
      calibration.material_column_reference_g_cm2 > 0.0f &&
      stellarFinite(calibration.material_optical_depth_at_reference) &&
      calibration.material_optical_depth_at_reference > 0.0f &&
      stellarFinite(calibration.outflow_column_flux_reference_g_cm_s) &&
      calibration.outflow_column_flux_reference_g_cm_s > 0.0f &&
      stellarFinite(calibration.outflow_emission_at_reference) &&
      calibration.outflow_emission_at_reference > 0.0f;
}

STELLAR_HERO_V079_HD inline StellarHeroPhysicalTermsV079
stellarHeroPhysicalTermsV079(const StellarPhysicalSampleV071 &physical)
{
  StellarHeroPhysicalTermsV079 terms = {};
  if(!(physical.density_cgs > 0.0f) || !stellarFinite(physical.density_cgs))
    return terms;
  const float speed = fmaxf(physical.speed_cm_per_s, 1.0e4f);
  const float outward = fmaxf(
      physical.outward_axial_velocity_cm_per_s, 0.0f);
  const float coherence = stellarClamp(outward / speed, 0.0f, 1.0f);
  terms.material_density_g_cm3 = physical.density_cgs;
  terms.outward_coherence = coherence;
  terms.outward_mass_flux_g_cm2_s = physical.density_cgs * outward *
      coherence * coherence;
  return terms;
}

STELLAR_HERO_V079_HD inline void stellarHeroMaterialColorV079(
    const StellarPhysicalSampleV071 &physical,
    const StellarPhysicalTransferV071 &transfer,
    const StellarPhysicalOpticalParametersV076 &style, float output[3])
{
  const float rotation_fraction = stellarClamp(
      (physical.rotational_fraction - transfer.range_min) /
          (transfer.range_max - transfer.range_min),
      0.0f, 1.0f);
  const float styled_rotation = stellarPhysicalStyledFractionV075(
      rotation_fraction, style.color_gamma, style.color_invert);
  float copper_blue[3];
  stellarCopperBlueV071(styled_rotation, copper_blue);

  const float log_temperature = physical.temperature_kelvin > 0.0f ?
      log10f(physical.temperature_kelvin) : 0.0f;
  float temperature_color[3];
  stellarTemperatureColor(log_temperature, temperature_color);
  const float hot_mix = 0.72f * stellarSmoothstep(
      7.85f, 8.70f, log_temperature);
  for(int component = 0; component < 3; component++)
    output[component] = copper_blue[component] + hot_mix *
        (temperature_color[component] - copper_blue[component]);
}

STELLAR_HERO_V079_HD inline void stellarHeroOutflowColorV079(
    float outward_coherence, float output[3])
{
  const float blue_fraction = 0.20f + 0.11f *
      stellarClamp(outward_coherence, 0.0f, 1.0f);
  stellarCopperBlueV071(blue_fraction, output);
}

STELLAR_HERO_V079_HD inline StellarOpticalSample
evaluateStellarHeroMaterialOutflowV079(
    const StellarPhysicalSampleV071 &physical,
    const StellarPhysicalTransferV071 &transfer,
    const StellarPhysicalOpticalParametersV076 &style,
    const StellarHeroCalibrationV079 &calibration)
{
  StellarOpticalSample output = {{0.0f, 0.0f, 0.0f}, 0.0f};
  if(!stellarHeroCalibrationValidV079(calibration))
    return output;
  const StellarHeroPhysicalTermsV079 terms =
      stellarHeroPhysicalTermsV079(physical);
  if(!(terms.material_density_g_cm3 > 0.0f))
    return output;

  const float material_extinction =
      calibration.material_optical_depth_at_reference *
      terms.material_density_g_cm3 /
      calibration.material_column_reference_g_cm2;
  output.extinction_per_cm = material_extinction;

  const float log_temperature = physical.temperature_kelvin > 0.0f ?
      log10f(physical.temperature_kelvin) : 0.0f;
  const float thermal_source = 0.25f + 0.75f * stellarSmoothstep(
      6.0f, 8.7f, log_temperature);
  float material_color[3];
  float outflow_color[3];
  stellarHeroMaterialColorV079(physical, transfer, style, material_color);
  stellarHeroOutflowColorV079(terms.outward_coherence, outflow_color);
  const float outflow_emission =
      calibration.outflow_emission_at_reference *
      terms.outward_mass_flux_g_cm2_s /
      calibration.outflow_column_flux_reference_g_cm_s;
  for(int component = 0; component < 3; component++)
    output.emissivity_rgb_per_cm[component] =
        material_extinction * thermal_source * material_color[component] +
        outflow_emission * outflow_color[component];
  return output;
}

STELLAR_HERO_V079_HD inline StellarOpticalSample
evaluateStellarRenderProgramV079(
    int render_program, float raw_value,
    const StellarPhysicalSampleV071 &physical,
    const StellarFeatureSampleV064 &feature,
    const StellarPhysicalTransferV071 &transfer,
    const StellarPhysicalOpticalParametersV076 &optical_parameters,
    const StellarTransferParameters &geometry,
    const StellarHeroCalibrationV079 &hero_calibration)
{
  if(render_program ==
     STELLAR_RENDER_PROGRAM_HERO_MATERIAL_OUTFLOW_V079)
    return evaluateStellarHeroMaterialOutflowV079(
        physical, transfer, optical_parameters, hero_calibration);
  return evaluateStellarPhysicalOpticalFromValueV078(
      raw_value, physical, feature, transfer, optical_parameters, geometry);
}

STELLAR_HERO_V079_HD inline void stellarDecodeRenderProgramV079(
    int render_program, const float input[3],
    const StellarPhysicalOpticalParametersV076 &optical_parameters,
    float output[3])
{
  if(render_program ==
     STELLAR_RENDER_PROGRAM_HERO_MATERIAL_OUTFLOW_V079) {
    output[0] = input[0];
    output[1] = input[1];
    output[2] = input[2];
    return;
  }
  stellarDecodePhysicalMomentsV078(input, optical_parameters, output);
}

#undef STELLAR_HERO_V079_HD

#endif
