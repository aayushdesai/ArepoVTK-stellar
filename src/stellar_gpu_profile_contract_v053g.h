#ifndef AREPO_VTK_STELLAR_GPU_PROFILE_CONTRACT_V053G_H
#define AREPO_VTK_STELLAR_GPU_PROFILE_CONTRACT_V053G_H

#include "stellar_gpu_profile_contract_v053f.h"
#include "stellar_physical_optical_v077.h"

inline bool stellarGpuProfileContractValidV053g(
    int transfer_mode, int palette_profile, int feature_profile,
    int physical_optical_profile)
{
  return stellarGpuProfileContractValidV053f(
      transfer_mode, palette_profile, feature_profile,
      physical_optical_profile) ||
      (stellarGpuProfileContractValidV053d(
           transfer_mode, palette_profile, feature_profile) &&
       physical_optical_profile ==
           STELLAR_PHYSICAL_OPTICAL_NORMALIZED_MOMENT_V077);
}

#endif
