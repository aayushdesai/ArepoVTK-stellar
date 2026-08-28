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
      STELLAR_PALETTE_STRUCTURE_FLUX_LAYERED_V070));
  assert(std::string(stellarPaletteProfileName(
      STELLAR_PALETTE_STRUCTURE_FLUX_LAYERED_V070)) ==
      "structure_flux_layered_v070");

  const StellarPaletteStyle vivid = stellarPaletteStyle(
      STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068);
  const StellarPaletteStyle layered = stellarPaletteStyle(
      STELLAR_PALETTE_STRUCTURE_FLUX_LAYERED_V070);
  assert(layered.kinematic_optical_enabled == 1);
  assert(layered.disk_emissivity_floor < vivid.disk_emissivity_floor);
  assert(layered.disk_extinction_floor < 0.2f * vivid.disk_extinction_floor);
  assert(layered.disk_density_extinction_gain <
         vivid.disk_density_extinction_gain);
  assert(layered.disk_rotation_extinction_gain <
         vivid.disk_rotation_extinction_gain);
  assert(layered.disk_radial_emissivity_gain >
         vivid.disk_radial_emissivity_gain);
  assert(layered.disk_temperature_mix < vivid.disk_temperature_mix);

  for(int channel = 0; channel < 3; channel++) {
    assert(layered.polar_low_density[channel] ==
           vivid.polar_low_density[channel]);
    assert(layered.polar_high_density[channel] ==
           vivid.polar_high_density[channel]);
    assert(layered.polar_accent_low_density[channel] ==
           vivid.polar_accent_low_density[channel]);
    assert(layered.polar_accent_high_density[channel] ==
           vivid.polar_accent_high_density[channel]);
    assert(layered.polar_flux_color[channel] ==
           vivid.polar_flux_color[channel]);
  }
  assert(layered.polar_temperature_mix == vivid.polar_temperature_mix);
  assert(layered.polar_emissivity_floor == vivid.polar_emissivity_floor);
  assert(layered.polar_mass_flux_emissivity_gain ==
         vivid.polar_mass_flux_emissivity_gain);
  assert(layered.polar_extinction_scale == vivid.polar_extinction_scale);
  assert(layered.polar_flux_color_mix == vivid.polar_flux_color_mix);

  const double disk_position[3] = {0.0, 2.0e10, 0.0};
  const float rotating_velocity[3] = {-2.0e8f, 0.0f, 0.0f};
  const float streaming_velocity[3] = {-2.0e8f, 5.0e7f, 0.0f};
  const StellarOpticalSample vivid_low_density = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_DISK,
                 STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068),
      disk_position, 10.4f, 2.5e7f, rotating_velocity);
  const StellarOpticalSample vivid_disk = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_DISK,
                 STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068),
      disk_position, 12.0f, 2.5e7f, rotating_velocity);
  const StellarOpticalSample layered_low_density =
      evaluateStellarOpticalSample(
          parameters(STELLAR_TRANSFER_DISK,
                     STELLAR_PALETTE_STRUCTURE_FLUX_LAYERED_V070),
          disk_position, 10.4f, 2.5e7f, rotating_velocity);
  const StellarOpticalSample layered_high_density =
      evaluateStellarOpticalSample(
          parameters(STELLAR_TRANSFER_DISK,
                     STELLAR_PALETTE_STRUCTURE_FLUX_LAYERED_V070),
          disk_position, 12.0f, 2.5e7f, rotating_velocity);
  const StellarOpticalSample layered_stream = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_DISK,
                 STELLAR_PALETTE_STRUCTURE_FLUX_LAYERED_V070),
      disk_position, 12.0f, 2.5e7f, streaming_velocity);
  assert(flux(layered_low_density) > 0.0f);
  assert(flux(layered_high_density) > flux(layered_low_density));
  const float layered_density_ratio =
      flux(layered_high_density) / flux(layered_low_density);
  const float vivid_density_ratio =
      flux(vivid_disk) / flux(vivid_low_density);
  assert(layered_density_ratio <= 1.05f * vivid_density_ratio);
  assert(flux(layered_stream) > flux(layered_high_density));
  assert(layered_high_density.extinction_per_cm <
         0.30f * vivid_disk.extinction_per_cm);
  assert(chromaticity(layered_stream, 0) >
         chromaticity(layered_high_density, 0));

  const double polar_position[3] = {2.0e9, 0.0, 3.0e10};
  const float strong_outflow[3] = {0.0f, 0.0f, 3.5e8f};
  const StellarOpticalSample vivid_wind = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW,
                 STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068),
      polar_position, 8.2f, 1.3e7f, strong_outflow);
  const StellarOpticalSample layered_wind = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW,
                 STELLAR_PALETTE_STRUCTURE_FLUX_LAYERED_V070),
      polar_position, 8.2f, 1.3e7f, strong_outflow);
  for(int channel = 0; channel < 3; channel++)
    assert(layered_wind.emissivity_rgb_per_cm[channel] ==
           vivid_wind.emissivity_rgb_per_cm[channel]);
  assert(layered_wind.extinction_per_cm == vivid_wind.extinction_per_cm);

  std::cout << "STELLAR_OPTICAL_PROFILE_V070_OK"
            << " disk_extinction_ratio="
            << layered_high_density.extinction_per_cm /
                   vivid_disk.extinction_per_cm
            << " disk_density_flux_ratio="
            << layered_density_ratio
            << " vivid_density_flux_ratio=" << vivid_density_ratio
            << " polar_vivid_exact=1\n";
  return 0;
}
