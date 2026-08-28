#ifndef AREPO_VTK_STELLAR_GPU_RAY_STATUS_V053B_H
#define AREPO_VTK_STELLAR_GPU_RAY_STATUS_V053B_H

#include <stddef.h>
#include <stdint.h>

enum StellarGpuRayStatusV053b {
  STELLAR_GPU_RAY_INACTIVE_V053B = 1u << 0,
  STELLAR_GPU_RAY_INVALID_CELL_V053B = 1u << 1,
  STELLAR_GPU_RAY_INVALID_EDGE_V053B = 1u << 2,
  STELLAR_GPU_RAY_NEIGHBOR_OVERFLOW_V053B = 1u << 3,
  STELLAR_GPU_RAY_NO_EXIT_FACE_V053B = 1u << 4,
  STELLAR_GPU_RAY_CELL_LIMIT_V053B = 1u << 5,
  STELLAR_GPU_RAY_NONFINITE_V053B = 1u << 6
};

static const size_t STELLAR_GPU_RAY_STATUS_COUNT_V053B = 7;

inline bool stellarGpuRayStatusIsFatalV053b(uint32_t status)
{
  // Inactive rays are expected outside the mesh. Every other known bit, and
  // every unknown future bit, remains a hard validity failure.
  return (status & ~uint32_t(STELLAR_GPU_RAY_INACTIVE_V053B)) != 0u;
}

inline bool stellarGpuStatusCountsValidV053b(
    const uint64_t *counts, size_t count)
{
  if(!counts || count != STELLAR_GPU_RAY_STATUS_COUNT_V053B)
    return false;
  for(size_t bit = 1; bit < count; bit++)
    if(counts[bit] != 0)
      return false;
  return true;
}

#endif
