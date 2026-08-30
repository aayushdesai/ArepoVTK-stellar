#include <cassert>
#include <cmath>
#include <iostream>

#include "stellar_hero_transfer_v079.h"

namespace {

bool closeEnough(float left, float right, float tolerance = 1.0e-6f)
{
  return std::fabs(left - right) <= tolerance *
      std::fmax(1.0f, std::fmax(std::fabs(left), std::fabs(right)));
}

} // namespace

int main()
{
  assert(stellarRenderProgramFromNameV079("retained_v078") ==
         STELLAR_RENDER_PROGRAM_RETAINED_V078);
  assert(stellarRenderProgramFromNameV079("hero_material_outflow_v079") ==
         STELLAR_RENDER_PROGRAM_HERO_MATERIAL_OUTFLOW_V079);
  assert(stellarRenderProgramFromNameV079("unknown") ==
         STELLAR_RENDER_PROGRAM_INVALID_V079);
  assert(stellarHeroCalibrationSha256ValidV079(std::string(64, 'a')));
  assert(!stellarHeroCalibrationSha256ValidV079(std::string(63, 'a')));
  assert(!stellarHeroCalibrationSha256ValidV079(std::string(64, 'G')));

  StellarHeroCalibrationV079 calibration = {};
  calibration.material_column_reference_g_cm2 = 4.0f;
  calibration.material_optical_depth_at_reference = 1.0f;
  calibration.outflow_column_flux_reference_g_cm_s = 5.0f;
  calibration.outflow_emission_at_reference = 2.0f;
  assert(stellarHeroCalibrationValidV079(calibration));

  StellarPhysicalOpticalParametersV076 style = {};
  style.profile = STELLAR_PHYSICAL_OPTICAL_LEGACY_V072;
  style.color_gamma = 2.0f;
  style.color_invert = 0;
  StellarPhysicalTransferV071 transfer = {};
  transfer.channel = STELLAR_PHYSICAL_CHANNEL_ROTATIONAL_FRACTION_V071;
  transfer.scale = STELLAR_PHYSICAL_SCALE_LINEAR_V071;
  transfer.range_min = 0.05f;
  transfer.range_max = 0.95f;

  StellarPhysicalSampleV071 material = {};
  material.density_cgs = 2.0f;
  material.temperature_kelvin = 1.0e8f;
  material.speed_cm_per_s = 10.0f;
  material.rotational_fraction = 0.8f;
  const StellarOpticalSample material_optical =
      evaluateStellarHeroMaterialOutflowV079(
          material, transfer, style, calibration);
  assert(closeEnough(material_optical.extinction_per_cm, 0.5f));
  assert(material_optical.emissivity_rgb_per_cm[0] > 0.0f);
  assert(material_optical.emissivity_rgb_per_cm[1] > 0.0f);
  assert(material_optical.emissivity_rgb_per_cm[2] > 0.0f);

  StellarPhysicalSampleV071 outflow = material;
  outflow.temperature_kelvin = 1.0e7f;
  outflow.speed_cm_per_s = 10.0f;
  outflow.outward_axial_velocity_cm_per_s = 5.0f;
  const StellarHeroPhysicalTermsV079 terms =
      stellarHeroPhysicalTermsV079(outflow);
  assert(closeEnough(terms.outward_coherence, 5.0e-4f));
  assert(terms.outward_mass_flux_g_cm2_s > 0.0f);
  outflow.speed_cm_per_s = 10.0e7f;
  outflow.outward_axial_velocity_cm_per_s = 5.0e7f;
  const StellarOpticalSample outflow_optical =
      evaluateStellarHeroMaterialOutflowV079(
          outflow, transfer, style, calibration);
  assert(outflow_optical.emissivity_rgb_per_cm[2] >
         material_optical.emissivity_rgb_per_cm[2]);
  assert(closeEnough(outflow_optical.extinction_per_cm,
                     material_optical.extinction_per_cm));

  StellarPhysicalSampleV071 absent = {};
  const StellarOpticalSample absent_optical =
      evaluateStellarHeroMaterialOutflowV079(
          absent, transfer, style, calibration);
  assert(absent_optical.extinction_per_cm == 0.0f);
  assert(absent_optical.emissivity_rgb_per_cm[0] == 0.0f);
  assert(absent_optical.emissivity_rgb_per_cm[1] == 0.0f);
  assert(absent_optical.emissivity_rgb_per_cm[2] == 0.0f);

  StellarTransferParameters geometry = {};
  geometry.box_size = 1.0e12;
  geometry.material_radius_cm = 1.0e10f;
  geometry.disk_radius_cm = 2.0e10f;
  geometry.polar_outer_cm = 5.0e11f;
  StellarFeatureSampleV064 feature = {};
  feature.merger_weight = 0.4f;
  feature.disk_weight = 0.5f;
  feature.polar_weight = 0.6f;
  style.profile = STELLAR_PHYSICAL_OPTICAL_COMPOSITE_MOMENT_V078;
  style.target_optical_depth = 1.0f;
  style.target_emission = 1.0f;
  style.reference_path_cm = 1.0e10f;
  style.density_support_log10_low = -2.0f;
  style.density_support_log10_high = 2.0f;
  style.emission_signal_floor = 0.2f;
  const StellarOpticalSample retained_direct =
      evaluateStellarPhysicalOpticalFromValueV078(
          0.8f, material, feature, transfer, style, geometry);
  const StellarOpticalSample retained_program =
      evaluateStellarRenderProgramV079(
          STELLAR_RENDER_PROGRAM_RETAINED_V078, 0.8f, material, feature,
          transfer, style, geometry, calibration);
  assert(retained_direct.extinction_per_cm ==
         retained_program.extinction_per_cm);
  for(int component = 0; component < 3; component++)
    assert(retained_direct.emissivity_rgb_per_cm[component] ==
           retained_program.emissivity_rgb_per_cm[component]);

  const float encoded[3] = {0.2f, 0.4f, 0.7f};
  float decoded[3] = {};
  stellarDecodeRenderProgramV079(
      STELLAR_RENDER_PROGRAM_HERO_MATERIAL_OUTFLOW_V079,
      encoded, style, decoded);
  assert(decoded[0] == encoded[0]);
  assert(decoded[1] == encoded[1]);
  assert(decoded[2] == encoded[2]);

  std::cout << "STELLAR_HERO_TRANSFER_V079_OK "
            << "components=material_column,outward_mass_flux "
            << "static_structure_weights=false retained_v078_exact=true\n";
  return 0;
}
