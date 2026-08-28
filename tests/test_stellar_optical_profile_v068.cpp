#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

#include "stellar_render_model_v052a.h"

namespace {

StellarTransferParameters parameters(int mode, int palette_profile)
{
  StellarTransferParameters value = {};
  value.mode = mode;
  value.palette_profile = palette_profile;
  value.feature_profile = STELLAR_FEATURE_STRUCTURES_V065;
  value.axis[2] = 1.0;
  value.box_size = 1.0e12;
  value.material_radius_cm = 2.0e10f;
  value.disk_radius_cm = 4.0e10f;
  value.disk_half_thickness_cm = 8.0e9f;
  value.polar_inner_cm = 5.0e9f;
  value.polar_outer_cm = 8.0e10f;
  value.polar_cone_ratio = 0.5f;
  value.merger_extinction_per_cm = 3.0e-11f;
  value.disk_extinction_per_cm = 1.5e-11f;
  value.polar_extinction_per_cm = 1.2e-12f;
  value.merger_emissivity_per_cm = 3.0e-11f;
  value.disk_emissivity_per_cm = 1.5e-11f;
  value.polar_emissivity_per_cm = 1.2e-12f;
  return value;
}

float flux(const StellarOpticalSample &sample)
{
  return sample.emissivity_rgb_per_cm[0] +
      sample.emissivity_rgb_per_cm[1] +
      sample.emissivity_rgb_per_cm[2];
}

float chromaticity(const StellarOpticalSample &sample, int channel)
{
  assert(flux(sample) > 0.0f);
  return sample.emissivity_rgb_per_cm[channel] / flux(sample);
}

} // namespace

int main()
{
  assert(stellarPaletteProfileValid(
      STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068));
  assert(stellarPaletteProfileValid(
      STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068));
  assert(std::string(stellarPaletteProfileName(
      STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068)) ==
      "structure_flux_balanced_v068");
  assert(std::string(stellarPaletteProfileName(
      STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068)) ==
      "structure_flux_vivid_v068");

  const StellarPaletteStyle balanced = stellarPaletteStyle(
      STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068);
  const StellarPaletteStyle vivid = stellarPaletteStyle(
      STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068);
  assert(balanced.kinematic_optical_enabled == 1);
  assert(vivid.kinematic_optical_enabled == 1);
  assert(vivid.polar_mass_flux_emissivity_gain >
         balanced.polar_mass_flux_emissivity_gain);
  assert(vivid.polar_flux_color_mix > balanced.polar_flux_color_mix);
  assert(balanced.disk_extinction_floor < 1.0f);
  assert(balanced.polar_extinction_scale < 1.0f);

  const double disk_position[3] = {0.0, 2.0e10, 0.0};
  const float rotating_velocity[3] = {-2.0e8f, 0.0f, 0.0f};
  const float rotating_stream_velocity[3] = {-2.0e8f, 1.5e8f, 0.0f};
  const StellarOpticalSample rotating_disk = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_DISK,
                 STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068),
      disk_position, 10.8f, 2.5e7f, rotating_velocity);
  const StellarOpticalSample streaming_disk = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_DISK,
                 STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068),
      disk_position, 10.8f, 2.5e7f, rotating_stream_velocity);
  const StellarOpticalSample legacy_disk = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_DISK,
                 STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058),
      disk_position, 10.8f, 2.5e7f, rotating_velocity);
  assert(flux(rotating_disk) > 0.0f);
  assert(flux(streaming_disk) > 0.0f);
  assert(chromaticity(streaming_disk, 0) >
         chromaticity(rotating_disk, 0));
  assert(rotating_disk.extinction_per_cm < legacy_disk.extinction_per_cm);

  const double polar_position[3] = {2.0e9, 0.0, 3.0e10};
  const float weak_outflow[3] = {1.0e8f, 0.0f, 8.0e7f};
  const float strong_outflow[3] = {0.0f, 0.0f, 3.5e8f};
  const StellarOpticalSample weak_wind = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW,
                 STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068),
      polar_position, 7.2f, 8.0e6f, weak_outflow);
  const StellarOpticalSample balanced_wind = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW,
                 STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068),
      polar_position, 8.2f, 1.3e7f, strong_outflow);
  const StellarOpticalSample vivid_wind = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW,
                 STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068),
      polar_position, 8.2f, 1.3e7f, strong_outflow);
  assert(flux(weak_wind) > 0.0f);
  assert(flux(balanced_wind) > flux(weak_wind));
  assert(chromaticity(balanced_wind, 2) >
         chromaticity(weak_wind, 2));
  assert(flux(vivid_wind) > flux(balanced_wind));
  assert(chromaticity(vivid_wind, 2) >
         chromaticity(balanced_wind, 2));

  StellarTransferParameters balanced_parameters = parameters(
      STELLAR_TRANSFER_COMPOSITE,
      STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068);
  StellarTransferParameters vivid_parameters = balanced_parameters;
  vivid_parameters.palette_profile =
      STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068;
  const StellarFeatureSampleV064 balanced_feature =
      evaluateStellarFeatureSampleV064(balanced_parameters, polar_position,
          8.2f, 1.3e7f, strong_outflow);
  const StellarFeatureSampleV064 vivid_feature =
      evaluateStellarFeatureSampleV064(vivid_parameters, polar_position,
          8.2f, 1.3e7f, strong_outflow);
  assert(balanced_feature.merger_weight == vivid_feature.merger_weight);
  assert(balanced_feature.disk_weight == vivid_feature.disk_weight);
  assert(balanced_feature.polar_weight == vivid_feature.polar_weight);
  assert(balanced_wind.extinction_per_cm >= 0.0f);
  assert(vivid_wind.extinction_per_cm >= 0.0f);
  assert(std::isfinite(balanced_wind.extinction_per_cm));
  assert(std::isfinite(vivid_wind.extinction_per_cm));

  std::cout << "STELLAR_OPTICAL_PROFILE_V068_OK"
            << " disk_extinction_ratio="
            << rotating_disk.extinction_per_cm / legacy_disk.extinction_per_cm
            << " balanced_wind_flux=" << flux(balanced_wind)
            << " vivid_wind_flux=" << flux(vivid_wind) << '\n';
  return 0;
}
