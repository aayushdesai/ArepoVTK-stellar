#ifndef AREPO_VTK_STELLAR_PALETTE_V057_H
#define AREPO_VTK_STELLAR_PALETTE_V057_H

enum StellarPaletteProfile {
  STELLAR_PALETTE_LEGACY_V052 = 0,
  STELLAR_PALETTE_COPPER_BLUE_V057 = 1,
  STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058 = 2,
  STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068 = 3,
  STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068 = 4
};

struct StellarPaletteStyle {
  float disk_low_density[3];
  float disk_high_density[3];
  float polar_low_density[3];
  float polar_high_density[3];
  float polar_accent_low_density[3];
  float polar_accent_high_density[3];
  float disk_temperature_mix;
  float polar_temperature_mix;
  float polar_accent_temperature_low;
  float polar_accent_temperature_high;
  float polar_accent_speed_low;
  float polar_accent_speed_high;
  float polar_accent_coherence_low;
  float polar_accent_coherence_high;
  float composite_merger_weight;
  float composite_disk_weight;
  float composite_polar_weight;
  int polar_accent_enabled;
  int neutralize_red_blue_overlap;
  int kinematic_optical_enabled;
  float disk_stream_color[3];
  float polar_flux_color[3];
  float disk_stream_color_mix;
  float polar_flux_color_mix;
  float merger_emissivity_floor;
  float merger_radial_emissivity_gain;
  float disk_emissivity_floor;
  float disk_density_emissivity_gain;
  float disk_radial_emissivity_gain;
  float disk_extinction_floor;
  float disk_density_extinction_gain;
  float disk_rotation_extinction_gain;
  float polar_emissivity_floor;
  float polar_mass_flux_emissivity_gain;
  float polar_extinction_scale;
  float disk_density_texture_low;
  float disk_density_texture_high;
  float disk_radial_fraction_low;
  float disk_radial_fraction_high;
  float polar_flux_density_low;
  float polar_flux_density_high;
  float polar_flux_speed_low;
  float polar_flux_speed_high;
  float polar_flux_coherence_low;
  float polar_flux_coherence_high;
};

STELLAR_HD inline bool stellarPaletteProfileValid(int profile)
{
  return profile == STELLAR_PALETTE_LEGACY_V052 ||
      profile == STELLAR_PALETTE_COPPER_BLUE_V057 ||
      profile == STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058 ||
      profile == STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068 ||
      profile == STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068;
}

STELLAR_HD inline StellarPaletteStyle stellarPaletteStyle(int profile)
{
  StellarPaletteStyle style = {};
  style.merger_emissivity_floor = 1.0f;
  style.disk_emissivity_floor = 1.0f;
  style.disk_extinction_floor = 1.0f;
  style.polar_emissivity_floor = 1.0f;
  style.polar_extinction_scale = 1.0f;

  if(profile == STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068 ||
     profile == STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068) {
    const bool vivid =
        profile == STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068;
    const float disk_low[3] = {0.16f, 0.028f, 0.004f};
    const float disk_high[3] = {1.00f, 0.62f, 0.14f};
    const float polar_low[3] = {0.065f, 0.025f, 0.006f};
    const float polar_high[3] = {0.55f, 0.20f, 0.04f};
    const float accent_low[3] = {0.010f, 0.025f, 0.18f};
    const float accent_high[3] = {0.08f, 0.22f, 0.95f};
    const float stream_color[3] = {0.88f, 0.20f, 0.025f};
    const float flux_color[3] = {0.025f, 0.10f, 0.85f};
    for(int channel = 0; channel < 3; channel++) {
      style.disk_low_density[channel] = disk_low[channel];
      style.disk_high_density[channel] = disk_high[channel];
      style.polar_low_density[channel] = polar_low[channel];
      style.polar_high_density[channel] = polar_high[channel];
      style.polar_accent_low_density[channel] = accent_low[channel];
      style.polar_accent_high_density[channel] = accent_high[channel];
      style.disk_stream_color[channel] = stream_color[channel];
      style.polar_flux_color[channel] = flux_color[channel];
    }
    style.disk_temperature_mix = vivid ? 0.38f : 0.32f;
    style.polar_temperature_mix = vivid ? 0.06f : 0.10f;
    style.polar_accent_temperature_low = 6.10f;
    style.polar_accent_temperature_high = 7.20f;
    style.polar_accent_speed_low = 1.0e8f;
    style.polar_accent_speed_high = 3.5e8f;
    style.polar_accent_coherence_low = 0.62f;
    style.polar_accent_coherence_high = 0.90f;
    style.composite_merger_weight = 0.38f;
    style.composite_disk_weight = 1.00f;
    style.composite_polar_weight = vivid ? 0.40f : 0.34f;
    style.polar_accent_enabled = 1;
    style.neutralize_red_blue_overlap = 1;
    style.kinematic_optical_enabled = 1;
    style.disk_stream_color_mix = vivid ? 0.60f : 0.48f;
    style.polar_flux_color_mix = vivid ? 0.78f : 0.58f;
    style.merger_emissivity_floor = 0.80f;
    style.merger_radial_emissivity_gain = vivid ? 0.45f : 0.35f;
    style.disk_emissivity_floor = 0.50f;
    style.disk_density_emissivity_gain = vivid ? 0.70f : 0.55f;
    style.disk_radial_emissivity_gain = vivid ? 0.45f : 0.30f;
    style.disk_extinction_floor = vivid ? 0.16f : 0.20f;
    style.disk_density_extinction_gain = 0.42f;
    style.disk_rotation_extinction_gain = 0.22f;
    style.polar_emissivity_floor = vivid ? 1.65f : 1.35f;
    style.polar_mass_flux_emissivity_gain = vivid ? 4.50f : 3.00f;
    style.polar_extinction_scale = vivid ? 0.45f : 0.55f;
    style.disk_density_texture_low = -1.0f;
    style.disk_density_texture_high = 2.5f;
    style.disk_radial_fraction_low = 0.08f;
    style.disk_radial_fraction_high = 0.55f;
    style.polar_flux_density_low = -3.0f;
    style.polar_flux_density_high = -0.4f;
    style.polar_flux_speed_low = 7.0e7f;
    style.polar_flux_speed_high = 3.5e8f;
    style.polar_flux_coherence_low = 0.50f;
    style.polar_flux_coherence_high = 0.90f;
    return style;
  }
  if(profile == STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058) {
    const float disk_low[3] = {0.24f, 0.055f, 0.008f};
    const float disk_high[3] = {1.00f, 0.58f, 0.12f};
    const float polar_low[3] = {0.08f, 0.035f, 0.008f};
    const float polar_high[3] = {0.52f, 0.24f, 0.07f};
    const float accent_low[3] = {0.012f, 0.028f, 0.14f};
    const float accent_high[3] = {0.08f, 0.18f, 0.75f};
    for(int channel = 0; channel < 3; channel++) {
      style.disk_low_density[channel] = disk_low[channel];
      style.disk_high_density[channel] = disk_high[channel];
      style.polar_low_density[channel] = polar_low[channel];
      style.polar_high_density[channel] = polar_high[channel];
      style.polar_accent_low_density[channel] = accent_low[channel];
      style.polar_accent_high_density[channel] = accent_high[channel];
    }
    style.disk_temperature_mix = 0.22f;
    style.polar_temperature_mix = 0.05f;
    style.polar_accent_temperature_low = 6.30f;
    style.polar_accent_temperature_high = 7.10f;
    style.polar_accent_speed_low = 1.5e8f;
    style.polar_accent_speed_high = 3.5e8f;
    style.polar_accent_coherence_low = 0.70f;
    style.polar_accent_coherence_high = 0.92f;
    style.composite_merger_weight = 0.40f;
    style.composite_disk_weight = 1.00f;
    style.composite_polar_weight = 0.28f;
    style.polar_accent_enabled = 1;
    style.neutralize_red_blue_overlap = 1;
    return style;
  }

  if(profile == STELLAR_PALETTE_COPPER_BLUE_V057) {
    const float disk_low[3] = {0.24f, 0.055f, 0.008f};
    const float disk_high[3] = {1.00f, 0.58f, 0.12f};
    const float polar_low[3] = {0.012f, 0.035f, 0.16f};
    const float polar_high[3] = {0.16f, 0.34f, 1.00f};
    for(int channel = 0; channel < 3; channel++) {
      style.disk_low_density[channel] = disk_low[channel];
      style.disk_high_density[channel] = disk_high[channel];
      style.polar_low_density[channel] = polar_low[channel];
      style.polar_high_density[channel] = polar_high[channel];
      style.polar_accent_low_density[channel] = polar_low[channel];
      style.polar_accent_high_density[channel] = polar_high[channel];
    }
    style.disk_temperature_mix = 0.22f;
    style.polar_temperature_mix = 0.06f;
    style.polar_accent_temperature_low = 0.0f;
    style.polar_accent_temperature_high = 1.0f;
    style.polar_accent_speed_low = 0.0f;
    style.polar_accent_speed_high = 1.0f;
    style.polar_accent_coherence_low = 0.0f;
    style.polar_accent_coherence_high = 1.0f;
    style.composite_merger_weight = 0.36f;
    style.composite_disk_weight = 1.00f;
    style.composite_polar_weight = 0.42f;
    style.polar_accent_enabled = 0;
    style.neutralize_red_blue_overlap = 1;
    return style;
  }

  const float disk_low[3] = {0.40f, 0.045f, 0.006f};
  const float disk_high[3] = {1.00f, 0.73f, 0.20f};
  const float polar_low[3] = {0.015f, 0.025f, 0.12f};
  const float polar_high[3] = {0.32f, 0.48f, 1.00f};
  for(int channel = 0; channel < 3; channel++) {
    style.disk_low_density[channel] = disk_low[channel];
    style.disk_high_density[channel] = disk_high[channel];
    style.polar_low_density[channel] = polar_low[channel];
    style.polar_high_density[channel] = polar_high[channel];
    style.polar_accent_low_density[channel] = polar_low[channel];
    style.polar_accent_high_density[channel] = polar_high[channel];
  }
  style.disk_temperature_mix = 0.35f;
  style.polar_temperature_mix = 0.15f;
  style.polar_accent_temperature_low = 0.0f;
  style.polar_accent_temperature_high = 1.0f;
  style.polar_accent_speed_low = 0.0f;
  style.polar_accent_speed_high = 1.0f;
  style.polar_accent_coherence_low = 0.0f;
  style.polar_accent_coherence_high = 1.0f;
  style.composite_merger_weight = 0.42f;
  style.composite_disk_weight = 1.00f;
  style.composite_polar_weight = 0.32f;
  style.polar_accent_enabled = 0;
  style.neutralize_red_blue_overlap = 0;
  return style;
}

inline const char *stellarPaletteProfileName(int profile)
{
  if(profile == STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068)
    return "structure_flux_vivid_v068";
  if(profile == STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068)
    return "structure_flux_balanced_v068";
  if(profile == STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058)
    return "copper_blue_accent_v058";
  if(profile == STELLAR_PALETTE_COPPER_BLUE_V057)
    return "copper_blue_v057";
  return "legacy_v052";
}

#endif
