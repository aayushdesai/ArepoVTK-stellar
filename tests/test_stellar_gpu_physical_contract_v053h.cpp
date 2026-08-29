#include <cassert>
#include <iostream>

#include "stellar_gpu_profile_contract_v053h.h"

int main()
{
  const int transfer = STELLAR_TRANSFER_COMPOSITE;
  const int palette = STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068;
  const int feature = STELLAR_FEATURE_STRUCTURES_V065;
  const int profiles[] = {
      STELLAR_PHYSICAL_OPTICAL_LEGACY_V072,
      STELLAR_PHYSICAL_OPTICAL_MATERIAL_SUPPORT_V074,
      STELLAR_PHYSICAL_OPTICAL_SEPARATED_SUPPORT_V075,
      STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076,
      STELLAR_PHYSICAL_OPTICAL_NORMALIZED_MOMENT_V077,
      STELLAR_PHYSICAL_OPTICAL_COMPOSITE_MOMENT_V078};
  for(int profile : profiles)
    assert(stellarGpuProfileContractValidV053h(
        transfer, palette, feature, profile));
  assert(!stellarGpuProfileContractValidV053h(
      transfer, palette, feature, STELLAR_PHYSICAL_OPTICAL_INVALID_V074));

  std::cout << "STELLAR_GPU_PHYSICAL_CONTRACT_V053H_OK "
            << "profiles=legacy_v072_through_composite_moment_v078"
            << std::endl;
  return 0;
}
