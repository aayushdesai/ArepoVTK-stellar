#ifndef AREPO_VTK_STELLAR_GPU_PROFILE_CONTRACT_V053F_H
#define AREPO_VTK_STELLAR_GPU_PROFILE_CONTRACT_V053F_H

#include "stellar_gpu_profile_contract_v053d.h"
#include "stellar_physical_optical_v076.h"

inline bool stellarGpuProfileContractValidV053f(
    int transfer_mode, int palette_profile, int feature_profile,
    int physical_optical_profile)
{
  return stellarGpuProfileContractValidV053d(
      transfer_mode, palette_profile, feature_profile) &&
      (physical_optical_profile == STELLAR_PHYSICAL_OPTICAL_LEGACY_V072 ||
       physical_optical_profile ==
           STELLAR_PHYSICAL_OPTICAL_MATERIAL_SUPPORT_V074 ||
       physical_optical_profile ==
           STELLAR_PHYSICAL_OPTICAL_SEPARATED_SUPPORT_V075 ||
       physical_optical_profile ==
           STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076);
}

#endif
