#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

#include "stellar_render_model_v052a.h"

namespace {

StellarTransferParameters parameters(int profile)
{
  StellarTransferParameters value = {};
  value.mode = STELLAR_TRANSFER_COMPOSITE;
  value.palette_profile = STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058;
  value.feature_profile = profile;
  value.axis[2] = 1.0;
  value.box_size = 1.0e12;
  value.material_radius_cm = 2.0e10f;
  value.disk_radius_cm = 4.0e10f;
  value.disk_half_thickness_cm = 8.0e9f;
  value.polar_inner_cm = 5.0e9f;
  value.polar_outer_cm = 8.0e10f;
  value.polar_cone_ratio = 0.5f;
  return value;
}

} // namespace

int main()
{
  assert(stellarFeatureProfileValidV065(STELLAR_FEATURE_LEGACY_V064));
  assert(stellarFeatureProfileValidV065(STELLAR_FEATURE_STRUCTURES_V065));
  assert(!stellarFeatureProfileValidV065(2));
  assert(std::string(stellarFeatureProfileNameV065(
      STELLAR_FEATURE_LEGACY_V064)) == "legacy_v064");
  assert(std::string(stellarFeatureProfileNameV065(
      STELLAR_FEATURE_STRUCTURES_V065)) == "stellar_structures_v065");

  const double core_position[3] = {0.0, 2.0e9, 0.0};
  const double disk_position[3] = {0.0, 2.0e10, 0.0};
  const double polar_position[3] = {0.0, 0.0, 3.0e10};
  const float rotating_velocity[3] = {-2.0e8f, 0.0f, 0.0f};
  const float outward_velocity[3] = {0.0f, 0.0f, 2.0e8f};

  const StellarFeatureSampleV064 legacy_core =
      evaluateStellarFeatureSampleV064(parameters(STELLAR_FEATURE_LEGACY_V064),
          core_position, 14.0f, 2.5e7f, rotating_velocity);
  const StellarFeatureSampleV064 structures_core =
      evaluateStellarFeatureSampleV064(
          parameters(STELLAR_FEATURE_STRUCTURES_V065), core_position,
          14.0f, 2.5e7f, rotating_velocity);
  assert(legacy_core.disk_weight > 0.0f);
  assert(structures_core.disk_weight < 0.01f * legacy_core.disk_weight);
  assert(structures_core.disk_annulus_support == 0.0f);

  const StellarFeatureSampleV064 structures_disk =
      evaluateStellarFeatureSampleV064(
          parameters(STELLAR_FEATURE_STRUCTURES_V065), disk_position,
          10.8f, 2.5e7f, rotating_velocity);
  assert(structures_disk.disk_annulus_support == 1.0f);
  assert(structures_disk.disk_density_retention > 0.9f);
  assert(structures_disk.disk_weight > 0.1f);

  const StellarFeatureSampleV064 legacy_polar =
      evaluateStellarFeatureSampleV064(parameters(STELLAR_FEATURE_LEGACY_V064),
          polar_position, 8.0f, 5.0e6f, outward_velocity);
  const StellarFeatureSampleV064 structures_polar =
      evaluateStellarFeatureSampleV064(
          parameters(STELLAR_FEATURE_STRUCTURES_V065), polar_position,
          8.0f, 5.0e6f, outward_velocity);
  assert(legacy_polar.polar_weight > 0.0f);
  assert(structures_polar.polar_weight > 0.0f);
  assert(structures_polar.polar_weight <= legacy_polar.polar_weight);
  assert(structures_polar.polar_confidence_retention >= 0.2f);
  assert(structures_polar.polar_confidence_retention <= 1.0f);

  const StellarFeatureProfileStyleV065 style =
      stellarFeatureProfileStyleV065(STELLAR_FEATURE_STRUCTURES_V065);
  assert(style.disk_inner_start_fraction == 0.12f);
  assert(style.disk_inner_full_fraction == 0.32f);
  assert(style.disk_density_taper_low == 2.5f);
  assert(style.disk_density_taper_high == 4.5f);
  assert(style.disk_density_floor == 0.08f);
  assert(style.polar_confidence_low == 0.08f);
  assert(style.polar_confidence_high == 0.28f);
  assert(style.polar_envelope_floor == 0.20f);

  std::cout << "STELLAR_FEATURE_PROFILE_V065_OK"
            << " core_ratio="
            << structures_core.disk_weight / legacy_core.disk_weight
            << " disk_weight=" << structures_disk.disk_weight
            << " polar_retention="
            << structures_polar.polar_confidence_retention << '\n';
  return 0;
}
