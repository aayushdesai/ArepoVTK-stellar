#ifndef AREPO_VTK_STELLAR_PALETTE_V057_H
#define AREPO_VTK_STELLAR_PALETTE_V057_H

enum StellarPaletteProfile {
  STELLAR_PALETTE_LEGACY_V052 = 0,
  STELLAR_PALETTE_COPPER_BLUE_V057 = 1
};

struct StellarPaletteStyle {
  float disk_low_density[3];
  float disk_high_density[3];
  float polar_low_density[3];
  float polar_high_density[3];
  float disk_temperature_mix;
  float polar_temperature_mix;
  float composite_merger_weight;
  float composite_disk_weight;
  float composite_polar_weight;
  int neutralize_red_blue_overlap;
};

STELLAR_HD inline bool stellarPaletteProfileValid(int profile)
{
  return profile == STELLAR_PALETTE_LEGACY_V052 ||
      profile == STELLAR_PALETTE_COPPER_BLUE_V057;
}

STELLAR_HD inline StellarPaletteStyle stellarPaletteStyle(int profile)
{
  StellarPaletteStyle style;
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
    }
    style.disk_temperature_mix = 0.22f;
    style.polar_temperature_mix = 0.06f;
    style.composite_merger_weight = 0.36f;
    style.composite_disk_weight = 1.00f;
    style.composite_polar_weight = 0.42f;
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
  }
  style.disk_temperature_mix = 0.35f;
  style.polar_temperature_mix = 0.15f;
  style.composite_merger_weight = 0.42f;
  style.composite_disk_weight = 1.00f;
  style.composite_polar_weight = 0.32f;
  style.neutralize_red_blue_overlap = 0;
  return style;
}

inline const char *stellarPaletteProfileName(int profile)
{
  if(profile == STELLAR_PALETTE_COPPER_BLUE_V057)
    return "copper_blue_v057";
  return "legacy_v052";
}

#endif
