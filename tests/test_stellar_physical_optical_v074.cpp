#include <cassert>
#include <cmath>
#include <iostream>

#include "stellar_physical_optical_v074.h"

int main()
{
  StellarTransferParameters geometry = {};
  geometry.material_radius_cm = 1.2e10f;
  geometry.disk_radius_cm = 3.0e10f;
  geometry.disk_half_thickness_cm = 3.0e9f;
  geometry.polar_outer_cm = 5.0e11f;

  StellarPhysicalTransferV071 transfer = {};
  transfer.channel = STELLAR_PHYSICAL_CHANNEL_ROTATIONAL_FRACTION_V071;
  transfer.scale = STELLAR_PHYSICAL_SCALE_LINEAR_V071;
  transfer.range_min = 0.0f;
  transfer.range_max = 1.0f;
  transfer.symlog_linthresh = 0.1f;
  transfer.extinction_per_cm = 1.0e-11f;
  transfer.emissivity_per_cm = 1.0e-11f;

  StellarFeatureSampleV064 material = {};
  material.disk_weight = 0.8f;
  StellarPhysicalOpticalParametersV074 supported = {};
  supported.profile = STELLAR_PHYSICAL_OPTICAL_MATERIAL_SUPPORT_V074;
  supported.target_optical_depth = 1.0f;
  supported.target_emission = 1.0f;

  const StellarOpticalSample zero = evaluateStellarPhysicalOpticalFromValueV074(
      0.0f, material, transfer, supported, geometry);
  assert(zero.extinction_per_cm == 0.0f);
  assert(zero.emissivity_rgb_per_cm[0] == 0.0f);

  const StellarFeatureSampleV064 irrelevant = {};
  const StellarOpticalSample unsupported =
      evaluateStellarPhysicalOpticalFromValueV074(
          0.8f, irrelevant, transfer, supported, geometry);
  assert(unsupported.extinction_per_cm == 0.0f);

  StellarFeatureSampleV064 compact_core = {};
  compact_core.merger_weight = 1.0f;
  const StellarOpticalSample core_rejected =
      evaluateStellarPhysicalOpticalFromValueV074(
          0.8f, compact_core, transfer, supported, geometry);
  assert(core_rejected.extinction_per_cm == 0.0f);

  const StellarOpticalSample structured =
      evaluateStellarPhysicalOpticalFromValueV074(
          0.8f, material, transfer, supported, geometry);
  assert(structured.extinction_per_cm > 0.0f);
  assert(std::fabs(structured.extinction_per_cm -
      (0.8f * 0.8f / (2.0f * geometry.disk_radius_cm))) < 1.0e-18f);

  StellarPhysicalOpticalParametersV074 legacy = {};
  legacy.profile = STELLAR_PHYSICAL_OPTICAL_LEGACY_V072;
  const StellarOpticalSample retained =
      evaluateStellarPhysicalOpticalFromValueV074(
          0.0f, irrelevant, transfer, legacy, geometry);
  const float expected_legacy = transfer.extinction_per_cm * 0.08f;
  assert(std::fabs(retained.extinction_per_cm - expected_legacy) < 1.0e-20f);

  transfer.channel = STELLAR_PHYSICAL_CHANNEL_OUTWARD_MASS_FLUX_V071;
  assert(stellarPhysicalReferencePathV074(
      transfer.channel, geometry, 0.0f) == 2.0f * geometry.polar_outer_cm);
  transfer.channel = STELLAR_PHYSICAL_CHANNEL_DENSITY_V071;
  assert(stellarPhysicalReferencePathV074(
      transfer.channel, geometry, 0.0f) == 2.0f * geometry.disk_radius_cm);

  std::cout << "STELLAR_PHYSICAL_OPTICAL_V074_OK legacy=retained "
            << "zero_floor=0 support=feature_weighted path=channel_normalized"
            << std::endl;
  return 0;
}
