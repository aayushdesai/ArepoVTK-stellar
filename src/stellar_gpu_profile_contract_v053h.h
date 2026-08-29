#ifndef AREPO_VTK_STELLAR_GPU_PROFILE_CONTRACT_V053H_H
#define AREPO_VTK_STELLAR_GPU_PROFILE_CONTRACT_V053H_H

#include "stellar_gpu_profile_contract_v053g.h"
#include "stellar_physical_optical_v078.h"

inline bool stellarGpuProfileContractValidV053h(
    int transfer_mode, int palette_profile, int feature_profile,
    int physical_optical_profile)
{
  return stellarGpuProfileContractValidV053g(
      transfer_mode, palette_profile, feature_profile,
      physical_optical_profile) ||
      (stellarGpuProfileContractValidV053d(
           transfer_mode, palette_profile, feature_profile) &&
       physical_optical_profile ==
           STELLAR_PHYSICAL_OPTICAL_COMPOSITE_MOMENT_V078);
}

#endif
