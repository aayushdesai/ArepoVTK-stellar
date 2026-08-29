#include <cassert>
#include <iostream>

#include "stellar_gpu_profile_contract_v053e.h"

int main()
{
  assert(stellarGpuProfileContractValidV053e(
      STELLAR_TRANSFER_COMPOSITE, STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068,
      STELLAR_FEATURE_STRUCTURES_V065,
      STELLAR_PHYSICAL_OPTICAL_LEGACY_V072));
  assert(stellarGpuProfileContractValidV053e(
      STELLAR_TRANSFER_COMPOSITE, STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068,
      STELLAR_FEATURE_STRUCTURES_V065,
      STELLAR_PHYSICAL_OPTICAL_MATERIAL_SUPPORT_V074));
  assert(!stellarGpuProfileContractValidV053e(
      STELLAR_TRANSFER_COMPOSITE, STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068,
      STELLAR_FEATURE_STRUCTURES_V065,
      STELLAR_PHYSICAL_OPTICAL_INVALID_V074));
  std::cout << "STELLAR_GPU_PHYSICAL_CONTRACT_V053E_OK "
            << "physical_optical=v074 legacy_preserved=true" << std::endl;
  return 0;
}
