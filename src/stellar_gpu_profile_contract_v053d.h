#ifndef AREPO_VTK_STELLAR_GPU_PROFILE_CONTRACT_V053D_H
#define AREPO_VTK_STELLAR_GPU_PROFILE_CONTRACT_V053D_H

#include "stellar_gpu_profile_contract_v053c.h"

inline bool stellarGpuProfileContractValidV053d(
    int transfer_mode, int palette_profile, int feature_profile)
{
  return stellarGpuProfileContractValidV053c(
      transfer_mode, palette_profile, feature_profile);
}

#endif
