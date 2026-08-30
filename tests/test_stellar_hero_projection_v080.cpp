#include <cassert>
#include <cmath>
#include <iostream>

#include "stellar_gpu_ray_depth_v080.h"
#include "stellar_hero_projection_v080.h"

namespace {

bool closeEnough(float a, float b, float tolerance = 1.0e-5f)
{
  return std::fabs(a - b) <= tolerance;
}

} // namespace

int main()
{
  StellarPhysicalTransferV071 transfer = {};
  transfer.range_min = 0.0034541641f;
  transfer.range_max = 0.99983037f;
  StellarPhysicalOpticalParametersV076 style = {};
  style.color_gamma = 3.0f;
  style.color_invert = 0;

  StellarPhysicalSampleV071 cold = {};
  cold.density_cgs = 2.0f;
  cold.temperature_kelvin = 1.0e7f;
  cold.rotational_fraction = 0.0f;
  cold.speed_cm_per_s = 1.0e7f;
  const StellarHeroProjectionSampleV080 hidden =
      stellarHeroProjectionSampleV080(cold, transfer, style);
  assert(hidden.material_density_g_cm3 == 0.0f);
  assert(hidden.outward_mass_flux_g_cm2_s == 0.0f);

  StellarPhysicalSampleV071 rotating = cold;
  rotating.rotational_fraction = 0.72f;
  const StellarHeroProjectionSampleV080 visible =
      stellarHeroProjectionSampleV080(rotating, transfer, style);
  assert(closeEnough(visible.material_density_g_cm3, 2.0f));
  assert(visible.styled_rotation_density_g_cm3 > 0.0f);
  assert(visible.hot_material_density_g_cm3 == 0.0f);

  StellarPhysicalSampleV071 hot = cold;
  hot.temperature_kelvin = 1.0e9f;
  const StellarHeroProjectionSampleV080 hotCore =
      stellarHeroProjectionSampleV080(hot, transfer, style);
  assert(closeEnough(hotCore.material_density_g_cm3, 2.0f));
  assert(closeEnough(hotCore.hot_material_density_g_cm3, 2.0f));

  StellarPhysicalSampleV071 nonfiniteRotation = hot;
  nonfiniteRotation.rotational_fraction = NAN;
  const StellarHeroProjectionSampleV080 finiteHotCore =
      stellarHeroProjectionSampleV080(nonfiniteRotation, transfer, style);
  assert(std::isfinite(finiteHotCore.styled_rotation_density_g_cm3));
  assert(finiteHotCore.styled_rotation_density_g_cm3 == 0.0f);

  StellarPhysicalSampleV071 wind = cold;
  wind.outward_axial_velocity_cm_per_s = 8.0e6f;
  wind.speed_cm_per_s = 1.0e7f;
  const StellarHeroProjectionSampleV080 outflow =
      stellarHeroProjectionSampleV080(wind, transfer, style);
  assert(outflow.material_density_g_cm3 == 0.0f);
  assert(outflow.outward_mass_flux_g_cm2_s > 0.0f);

  StellarHeroProjectionMomentsV080 moments =
      stellarEmptyHeroProjectionMomentsV080();
  stellarAccumulateHeroProjectionV080(&moments, visible, 5.0f);
  stellarAccumulateHeroProjectionV080(&moments, hotCore, 2.0f);
  stellarAccumulateHeroProjectionV080(&moments, outflow, 4.0f);
  assert(closeEnough(moments.material_column_g_cm2, 14.0f));
  assert(closeEnough(moments.hot_material_column_g_cm2, 4.0f));
  assert(moments.outflow_column_flux_g_cm_s > 0.0f);

  StellarHeroProjectionCalibrationV080 calibration = {};
  calibration.material_column_reference_g_cm2 = 10.0f;
  calibration.material_display_gain = 0.22f;
  calibration.outflow_column_flux_reference_g_cm_s =
      moments.outflow_column_flux_g_cm_s;
  calibration.outflow_display_gain = 0.35f;
  assert(stellarHeroProjectionCalibrationValidV080(calibration));
  float color[3];
  stellarDecodeHeroProjectionV080(moments, calibration, color);
  for(int component = 0; component < 3; component++) {
    assert(std::isfinite(color[component]));
    assert(color[component] >= 0.0f && color[component] <= 1.0f);
  }
  assert(color[0] + color[1] + color[2] > 0.0f);

  StellarHeroProjectionMomentsV080 windOnly =
      stellarEmptyHeroProjectionMomentsV080();
  windOnly.outflow_column_flux_g_cm_s =
      calibration.outflow_column_flux_reference_g_cm_s;
  stellarDecodeHeroProjectionV080(windOnly, calibration, color);
  assert(color[2] > color[0]);

  const StellarGpuRayDepthDecisionV080 legacy =
      stellarResolveGpuRayDepthV080(
          2.0e12, 4.0e12, 1.5e12, 0.0);
  assert(!legacy.valid);
  const StellarGpuRayDepthDecisionV080 relative =
      stellarResolveGpuRayDepthV080(
          2.0e12, 4.0e12, 0.0, 1.5e12);
  assert(relative.valid && relative.limited);
  assert(std::fabs(relative.maximum_t - 3.5e12) < 1.0);
  const StellarGpuRayDepthDecisionV080 unlimited =
      stellarResolveGpuRayDepthV080(
          2.0e12, 4.0e12, 0.0, 0.0);
  assert(unlimited.valid && !unlimited.limited);
  assert(std::fabs(unlimited.maximum_t - 4.0e12) < 1.0);
  assert(!stellarGpuRayDepthInputsValidV080(1.0, 1.0));

  std::cout << "STELLAR_HERO_PROJECTION_V080_OK "
            << "moments=material,rotation,hot,outflow "
            << "palette=post_ray_copper_blue "
            << "ray_depth=entry_relative\n";
  return 0;
}
