#ifndef AREPO_VTK_STELLAR_GPU_PROFILE_CONTRACT_V053I_H
#define AREPO_VTK_STELLAR_GPU_PROFILE_CONTRACT_V053I_H

#include "stellar_gpu_profile_contract_v053h.h"
#include "stellar_hero_transfer_v079.h"

inline bool stellarGpuProfileContractValidV053i(
    int render_program, int transfer_mode, int palette_profile,
    int feature_profile, int physical_optical_profile)
{
  if(render_program == STELLAR_RENDER_PROGRAM_RETAINED_V078)
    return stellarGpuProfileContractValidV053h(
        transfer_mode, palette_profile, feature_profile,
        physical_optical_profile);
  return render_program ==
             STELLAR_RENDER_PROGRAM_HERO_MATERIAL_OUTFLOW_V079 &&
      transfer_mode == STELLAR_TRANSFER_COMPOSITE &&
      palette_profile == STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058 &&
      feature_profile == STELLAR_FEATURE_STRUCTURES_V065 &&
      physical_optical_profile == STELLAR_PHYSICAL_OPTICAL_LEGACY_V072;
}

#endif
