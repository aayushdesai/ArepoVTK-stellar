#include <cassert>
#include <iostream>

#include "stellar_gpu_profile_contract_v053f.h"

int main()
{
  const int transfer = STELLAR_TRANSFER_COMPOSITE;
  const int palette = STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068;
  const int feature = STELLAR_FEATURE_STRUCTURES_V065;
  assert(stellarGpuProfileContractValidV053f(
      transfer, palette, feature, STELLAR_PHYSICAL_OPTICAL_LEGACY_V072));
  assert(stellarGpuProfileContractValidV053f(
      transfer, palette, feature,
      STELLAR_PHYSICAL_OPTICAL_MATERIAL_SUPPORT_V074));
  assert(stellarGpuProfileContractValidV053f(
      transfer, palette, feature,
      STELLAR_PHYSICAL_OPTICAL_SEPARATED_SUPPORT_V075));
  assert(stellarGpuProfileContractValidV053f(
      transfer, palette, feature,
      STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076));
  assert(!stellarGpuProfileContractValidV053f(
      transfer, palette, feature, STELLAR_PHYSICAL_OPTICAL_INVALID_V074));

  std::cout << "STELLAR_GPU_PHYSICAL_CONTRACT_V053F_OK "
            << "profiles=legacy_v072,material_support_v074,"
               "separated_support_v075,density_moment_v076"
            << std::endl;
  return 0;
}
