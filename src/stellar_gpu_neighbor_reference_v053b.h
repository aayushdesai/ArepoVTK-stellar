#ifndef AREPO_VTK_STELLAR_GPU_NEIGHBOR_REFERENCE_V053B_H
#define AREPO_VTK_STELLAR_GPU_NEIGHBOR_REFERENCE_V053B_H

#ifdef __CUDACC__
#define STELLAR_GPU_NEIGHBOR_HD __host__ __device__
#else
#define STELLAR_GPU_NEIGHBOR_HD
#endif

// Positive references contribute to reconstruction and support. Negative
// references are ghost connections that contribute only to SPH support.
// Zero is reserved as invalid, so cell indices are stored with a +1 offset.
STELLAR_GPU_NEIGHBOR_HD inline int stellarGpuEncodeNeighborV053b(
    int cell, bool contributes)
{
  if(cell < 0)
    return 0;
  const int encoded = cell + 1;
  return contributes ? encoded : -encoded;
}

STELLAR_GPU_NEIGHBOR_HD inline int stellarGpuDecodeNeighborV053b(int reference)
{
  if(reference == 0)
    return -1;
  return (reference > 0 ? reference : -reference) - 1;
}

STELLAR_GPU_NEIGHBOR_HD inline bool stellarGpuNeighborContributesV053b(
    int reference)
{
  return reference > 0;
}

#undef STELLAR_GPU_NEIGHBOR_HD

#endif
