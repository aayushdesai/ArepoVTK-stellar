#ifndef AREPO_VTK_STELLAR_FEATURE_PROFILE_V065_H
#define AREPO_VTK_STELLAR_FEATURE_PROFILE_V065_H

enum StellarFeatureProfileV065 {
  STELLAR_FEATURE_LEGACY_V064 = 0,
  STELLAR_FEATURE_STRUCTURES_V065 = 1
};

struct StellarFeatureProfileStyleV065 {
  float disk_inner_start_fraction;
  float disk_inner_full_fraction;
  float disk_density_taper_low;
  float disk_density_taper_high;
  float disk_density_floor;
  float polar_confidence_low;
  float polar_confidence_high;
  float polar_envelope_floor;
};

inline bool stellarFeatureProfileValidV065(int profile)
{
  return profile == STELLAR_FEATURE_LEGACY_V064 ||
      profile == STELLAR_FEATURE_STRUCTURES_V065;
}

inline const char *stellarFeatureProfileNameV065(int profile)
{
  return profile == STELLAR_FEATURE_STRUCTURES_V065 ?
      "stellar_structures_v065" : "legacy_v064";
}

inline StellarFeatureProfileStyleV065 stellarFeatureProfileStyleV065(
    int profile)
{
  StellarFeatureProfileStyleV065 style = {};
  if(profile == STELLAR_FEATURE_STRUCTURES_V065) {
    style.disk_inner_start_fraction = 0.12f;
    style.disk_inner_full_fraction = 0.32f;
    style.disk_density_taper_low = 2.5f;
    style.disk_density_taper_high = 4.5f;
    style.disk_density_floor = 0.08f;
    style.polar_confidence_low = 0.08f;
    style.polar_confidence_high = 0.28f;
    style.polar_envelope_floor = 0.20f;
  }
  return style;
}

#endif
