#ifndef AREPO_VTK_STELLAR_GPU_PROFILE_CONTRACT_V053C_H
#define AREPO_VTK_STELLAR_GPU_PROFILE_CONTRACT_V053C_H

#include "stellar_render_model_v052a.h"

inline bool stellarGpuProfileContractValidV053c(
    int transfer_mode, int palette_profile, int feature_profile)
{
  if(transfer_mode < STELLAR_TRANSFER_MERGER ||
     transfer_mode > STELLAR_TRANSFER_COMPOSITE ||
     !stellarPaletteProfileValid(palette_profile) ||
     !stellarFeatureProfileValidV065(feature_profile))
    return false;
  const bool structure_flux =
      palette_profile == STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068 ||
      palette_profile == STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068;
  if(structure_flux && transfer_mode != STELLAR_TRANSFER_COMPOSITE)
    return false;
  if(structure_flux && feature_profile != STELLAR_FEATURE_STRUCTURES_V065)
    return false;
  return true;
}

#endif
