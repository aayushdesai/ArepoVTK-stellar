#ifndef AREPO_VTK_STELLAR_PALETTE_V057_H
#define AREPO_VTK_STELLAR_PALETTE_V057_H

enum StellarPaletteProfile {
  STELLAR_PALETTE_LEGACY_V052 = 0,
  STELLAR_PALETTE_COPPER_BLUE_V057 = 1,
  STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058 = 2
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
};

STELLAR_HD inline bool stellarPaletteProfileValid(int profile)
{
  return profile == STELLAR_PALETTE_LEGACY_V052 ||
      profile == STELLAR_PALETTE_COPPER_BLUE_V057 ||
      profile == STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058;
}

STELLAR_HD inline StellarPaletteStyle stellarPaletteStyle(int profile)
{
  StellarPaletteStyle style;
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
  if(profile == STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058)
    return "copper_blue_accent_v058";
  if(profile == STELLAR_PALETTE_COPPER_BLUE_V057)
    return "copper_blue_v057";
  return "legacy_v052";
}

#endif
