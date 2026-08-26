#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

#include "stellar_render_model_v052a.h"

namespace {

StellarTransferParameters parameters(int mode, int profile)
{
  StellarTransferParameters value = {};
  value.mode = mode;
  value.palette_profile = profile;
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

float chromaticity(const StellarOpticalSample &sample, int channel)
{
  const float total = sample.emissivity_rgb_per_cm[0] +
      sample.emissivity_rgb_per_cm[1] + sample.emissivity_rgb_per_cm[2];
  assert(total > 0.0f);
  return sample.emissivity_rgb_per_cm[channel] / total;
}

} // namespace

int main()
{
  assert(stellarPaletteProfileValid(STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058));
  assert(std::string(stellarPaletteProfileName(
      STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058)) ==
      "copper_blue_accent_v058");

  const StellarPaletteStyle style =
      stellarPaletteStyle(STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058);
  assert(style.polar_accent_enabled == 1);
  assert(style.polar_low_density[0] > style.polar_low_density[1]);
  assert(style.polar_low_density[1] > style.polar_low_density[2]);
  assert(style.polar_high_density[0] > style.polar_high_density[1]);
  assert(style.polar_high_density[1] > style.polar_high_density[2]);
  assert(style.polar_accent_high_density[2] >
         3.0f * style.polar_accent_high_density[1]);
  assert(style.composite_polar_weight <
         stellarPaletteStyle(STELLAR_PALETTE_COPPER_BLUE_V057).
             composite_polar_weight);

  const double polar_position[3] = {5.02e11, 5.0e11, 6.0e11};
  const float moderate_outflow[3] = {0.0f, 0.0f, 1.0e8f};
  const float hot_fast_outflow[3] = {0.0f, 0.0f, 4.0e8f};
  const StellarOpticalSample warm_wind = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW,
                 STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058),
      polar_position, 8.2f, 5.0e6f, moderate_outflow);
  assert(warm_wind.emissivity_rgb_per_cm[0] >
         warm_wind.emissivity_rgb_per_cm[1]);
  assert(warm_wind.emissivity_rgb_per_cm[1] >
         warm_wind.emissivity_rgb_per_cm[2]);

  const StellarOpticalSample blue_accent = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW,
                 STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058),
      polar_position, 8.2f, 1.3e7f, hot_fast_outflow);
  assert(blue_accent.emissivity_rgb_per_cm[2] >
         blue_accent.emissivity_rgb_per_cm[0]);
  assert(blue_accent.emissivity_rgb_per_cm[2] >
         2.0f * blue_accent.emissivity_rgb_per_cm[1]);
  assert(chromaticity(blue_accent, 2) > chromaticity(warm_wind, 2) + 0.30f);

  const StellarOpticalSample legacy_selection = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW, STELLAR_PALETTE_COPPER_BLUE_V057),
      polar_position, 8.2f, 1.3e7f, hot_fast_outflow);
  assert(std::abs(legacy_selection.extinction_per_cm -
                  blue_accent.extinction_per_cm) < 1.0e-20f);

  std::cout << "STELLAR_PALETTE_V058_OK profile=copper_blue_accent_v058\n";
  return 0;
}
