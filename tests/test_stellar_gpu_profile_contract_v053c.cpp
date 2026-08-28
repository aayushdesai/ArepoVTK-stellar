#include <cassert>
#include <iostream>

#include "stellar_gpu_profile_contract_v053c.h"

int main()
{
  assert(stellarGpuProfileContractValidV053c(
      STELLAR_TRANSFER_COMPOSITE,
      STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058,
      STELLAR_FEATURE_STRUCTURES_V065));
  assert(stellarGpuProfileContractValidV053c(
      STELLAR_TRANSFER_COMPOSITE,
      STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068,
      STELLAR_FEATURE_STRUCTURES_V065));
  assert(stellarGpuProfileContractValidV053c(
      STELLAR_TRANSFER_COMPOSITE,
      STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068,
      STELLAR_FEATURE_STRUCTURES_V065));
  assert(!stellarGpuProfileContractValidV053c(
      STELLAR_TRANSFER_DISK,
      STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068,
      STELLAR_FEATURE_STRUCTURES_V065));
  assert(!stellarGpuProfileContractValidV053c(
      STELLAR_TRANSFER_COMPOSITE,
      STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068,
      STELLAR_FEATURE_LEGACY_V064));
  assert(!stellarGpuProfileContractValidV053c(
      STELLAR_TRANSFER_COMPOSITE, 999,
      STELLAR_FEATURE_STRUCTURES_V065));
  assert(!stellarGpuProfileContractValidV053c(
      STELLAR_TRANSFER_COMPOSITE,
      STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068, 999));
  assert(!stellarGpuProfileContractValidV053c(
      999, STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058,
      STELLAR_FEATURE_STRUCTURES_V065));

  std::cout << "STELLAR_GPU_PROFILE_CONTRACT_V053C_OK"
            << " profiles=control,balanced,vivid"
            << " required_transfer=composite"
            << " required_feature=stellar_structures_v065\n";
  return 0;
}
