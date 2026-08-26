#include <cassert>
#include <cmath>
#include <iostream>

#include "stellar_render_model_v052a.h"

namespace {

StellarTransferParameters parameters(int mode, int palette_profile)
{
  StellarTransferParameters value = {};
  value.mode = mode;
  value.palette_profile = palette_profile;
  value.center[0] = value.center[1] = value.center[2] = 5.0e11;
  value.axis[2] = 1.0;
  value.box_size = 1.0e12;
  value.material_radius_cm = 1.2e10f;
  value.disk_radius_cm = 3.0e10f;
  value.disk_half_thickness_cm = 3.0e9f;
  value.polar_inner_cm = 5.0e9f;
  value.polar_outer_cm = 5.0e11f;
  value.polar_cone_ratio = 0.7f;
  value.merger_extinction_per_cm = 3.0e-11f;
  value.disk_extinction_per_cm = 1.5e-11f;
  value.polar_extinction_per_cm = 1.2e-12f;
  value.merger_emissivity_per_cm = 3.0e-11f;
  value.disk_emissivity_per_cm = 1.5e-11f;
  value.polar_emissivity_per_cm = 1.2e-12f;
  return value;
}

} // namespace

int main()
{
  assert(stellarPaletteProfileValid(STELLAR_PALETTE_LEGACY_V052));
  assert(stellarPaletteProfileValid(STELLAR_PALETTE_COPPER_BLUE_V057));
  assert(!stellarPaletteProfileValid(-1));
  assert(!stellarPaletteProfileValid(2));

  const StellarPaletteStyle legacy =
      stellarPaletteStyle(STELLAR_PALETTE_LEGACY_V052);
  assert(legacy.disk_low_density[0] == 0.40f);
  assert(legacy.disk_high_density[1] == 0.73f);
  assert(legacy.polar_low_density[2] == 0.12f);
  assert(legacy.polar_high_density[0] == 0.32f);
  assert(legacy.disk_temperature_mix == 0.35f);
  assert(legacy.polar_temperature_mix == 0.15f);
  assert(legacy.composite_merger_weight == 0.42f);
  assert(legacy.composite_disk_weight == 1.0f);
  assert(legacy.composite_polar_weight == 0.32f);
  assert(legacy.neutralize_red_blue_overlap == 0);

  const StellarPaletteStyle copper =
      stellarPaletteStyle(STELLAR_PALETTE_COPPER_BLUE_V057);
  assert(copper.disk_low_density[0] > copper.disk_low_density[1]);
  assert(copper.disk_low_density[1] > copper.disk_low_density[2]);
  assert(copper.disk_high_density[0] > copper.disk_high_density[1]);
  assert(copper.disk_high_density[1] > copper.disk_high_density[2]);
  assert(copper.polar_low_density[2] > 3.0f * copper.polar_low_density[1]);
  assert(copper.polar_high_density[2] > 2.0f * copper.polar_high_density[1]);
  assert(copper.neutralize_red_blue_overlap == 1);

  const double disk_position[3] = {5.08e11, 5.0e11, 5.005e11};
  const double polar_position[3] = {5.02e11, 5.0e11, 6.0e11};
  const float rotating_velocity[3] = {0.0f, 3.0e8f, 0.0f};
  const float outward_velocity[3] = {0.0f, 0.0f, 3.0e8f};

  const StellarOpticalSample disk = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_DISK, STELLAR_PALETTE_COPPER_BLUE_V057),
      disk_position, 11.0f, 2.5e7f, rotating_velocity);
  assert(disk.emissivity_rgb_per_cm[0] > disk.emissivity_rgb_per_cm[1]);
  assert(disk.emissivity_rgb_per_cm[1] > disk.emissivity_rgb_per_cm[2]);

  const StellarOpticalSample polar = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW, STELLAR_PALETTE_COPPER_BLUE_V057),
      polar_position, 8.0f, 5.0e6f, outward_velocity);
  assert(polar.emissivity_rgb_per_cm[2] >
         2.0f * polar.emissivity_rgb_per_cm[1]);
  assert(polar.emissivity_rgb_per_cm[1] > polar.emissivity_rgb_per_cm[0]);

  StellarTransferParameters overlap =
      parameters(STELLAR_TRANSFER_COMPOSITE, STELLAR_PALETTE_COPPER_BLUE_V057);
  overlap.disk_half_thickness_cm = 1.2e10f;
  overlap.merger_emissivity_per_cm = 1.5e-12f;
  overlap.disk_emissivity_per_cm = 1.5e-12f;
  overlap.polar_emissivity_per_cm = 1.5e-12f;
  const double overlap_position[3] = {5.001e11, 5.0e11, 5.07e11};
  const float overlap_velocity[3] = {0.0f, 5.0e8f, 3.0e8f};
  const StellarOpticalSample composite = evaluateStellarOpticalSample(
      overlap, overlap_position, 8.5f, 6.0e6f, overlap_velocity);
  assert(composite.emissivity_rgb_per_cm[0] > 0.0f);
  assert(composite.emissivity_rgb_per_cm[2] > 0.0f);
  assert(composite.emissivity_rgb_per_cm[1] + 1.0e-30f >=
         std::fmin(composite.emissivity_rgb_per_cm[0],
                   composite.emissivity_rgb_per_cm[2]));

  std::cout << "STELLAR_PALETTE_V057_OK profile=copper_blue_v057\n";
  return 0;
}
