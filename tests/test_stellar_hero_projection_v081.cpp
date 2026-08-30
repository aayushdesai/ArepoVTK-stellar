#include <cassert>
#include <cmath>
#include <iostream>

#include "stellar_hero_projection_v081.h"

namespace {

bool closeEnough(float left, float right, float tolerance = 1.0e-5f)
{
  return std::fabs(left - right) <= tolerance;
}

} // namespace

int main()
{
  StellarHeroDisplayContractV081 contract = {};
  contract.reference_quantile = 0.99f;
  contract.response_floor_fraction = 0.02f;
  contract.material_display_gain = 0.72f;
  contract.outflow_display_gain = 0.72f;
  assert(stellarHeroDisplayContractValidV081(contract));

  StellarHeroSurfaceSampleV081 empty = stellarEmptyHeroSurfaceSampleV081();
  float emptyColor[3];
  stellarDecodeHeroSurfaceV081(
      empty, 1.0f, 1.0f, contract, emptyColor);
  assert(closeEnough(emptyColor[0], 0.015f));
  assert(closeEnough(emptyColor[1], 0.022f));
  assert(closeEnough(emptyColor[2], 0.030f));

  StellarHeroSurfaceSampleV081 early = {};
  early.material_peak_density_g_cm3 = 1.0e5f;
  early.material_rotation_fraction = 0.7f;
  early.material_hot_support = 0.2f;
  early.outflow_peak_flux_g_cm2_s = 2.0e17f;
  float earlyColor[3];
  stellarDecodeHeroSurfaceV081(
      early, 1.0e5f, 2.0e17f, contract, earlyColor);

  StellarHeroSurfaceSampleV081 late = {};
  late.material_peak_density_g_cm3 = 1.0f;
  late.material_rotation_fraction = 0.7f;
  late.material_hot_support = 0.2f;
  late.outflow_peak_flux_g_cm2_s = 2.0e12f;
  float lateColor[3];
  stellarDecodeHeroSurfaceV081(
      late, 1.0f, 2.0e12f, contract, lateColor);
  for(int component = 0; component < 3; component++) {
    assert(closeEnough(earlyColor[component], lateColor[component]));
    assert(earlyColor[component] >= 0.0f && earlyColor[component] <= 1.0f);
  }

  const float maximum = fmaxf(earlyColor[0],
      fmaxf(earlyColor[1], earlyColor[2]));
  const float minimum = fminf(earlyColor[0],
      fminf(earlyColor[1], earlyColor[2]));
  assert(maximum - minimum > 0.1f);

  StellarHeroSurfaceSampleV081 outflow = {};
  outflow.outflow_peak_flux_g_cm2_s = 2.0e17f;
  float outflowColor[3];
  stellarDecodeHeroSurfaceV081(
      outflow, 1.0f, 2.0e17f, contract, outflowColor);
  assert(outflowColor[2] > outflowColor[0]);

  StellarHeroSurfaceSampleV081 hotCore = {};
  hotCore.material_peak_density_g_cm3 = 1.0f;
  hotCore.material_hot_support = 1.0f;
  float hotCoreColor[3];
  stellarDecodeHeroSurfaceV081(
      hotCore, 1.0f, 1.0f, contract, hotCoreColor);
  assert(hotCoreColor[0] > hotCoreColor[2]);
  assert(hotCoreColor[0] - hotCoreColor[1] > 0.1f);

  StellarHeroSurfaceSampleV081 belowFloor = {};
  belowFloor.material_peak_density_g_cm3 = 0.01f;
  float belowFloorColor[3];
  stellarDecodeHeroSurfaceV081(
      belowFloor, 1.0f, 1.0f, contract, belowFloorColor);
  for(int component = 0; component < 3; component++)
    assert(closeEnough(belowFloorColor[component], emptyColor[component]));

  StellarHeroProjectionSampleV080 weak = {};
  weak.material_density_g_cm3 = 2.0f;
  weak.styled_rotation_density_g_cm3 = 0.4f;
  weak.hot_material_density_g_cm3 = 0.2f;
  weak.outward_mass_flux_g_cm2_s = 9.0f;
  StellarHeroProjectionSampleV080 strong = {};
  strong.material_density_g_cm3 = 5.0f;
  strong.styled_rotation_density_g_cm3 = 4.0f;
  strong.hot_material_density_g_cm3 = 1.5f;
  strong.outward_mass_flux_g_cm2_s = 3.0f;
  StellarHeroSurfaceSampleV081 selected = stellarEmptyHeroSurfaceSampleV081();
  stellarUpdateHeroSurfaceV081(&selected, weak);
  stellarUpdateHeroSurfaceV081(&selected, strong);
  assert(closeEnough(selected.material_peak_density_g_cm3, 5.0f));
  assert(closeEnough(selected.material_rotation_fraction, 0.8f));
  assert(closeEnough(selected.material_hot_support, 0.3f));
  assert(closeEnough(selected.outflow_peak_flux_g_cm2_s, 9.0f));

  std::cout << "STELLAR_HERO_PROJECTION_V081_OK "
            << "normalization=per_view_positive_ray_p99 "
            << "projection=per_ray_dominant_cell "
            << "display=camera_lab_direct_rgb "
            << "physics=v080_samples_unchanged\n";
  return 0;
}
