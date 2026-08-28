#include "stellar_gpu_ray_status_v053b.h"

#include <assert.h>
#include <iostream>

int main()
{
  uint64_t background_fixture[STELLAR_GPU_RAY_STATUS_COUNT_V053B] = {};
  background_fixture[0] = 116082;
  assert(stellarGpuStatusCountsValidV053b(
      background_fixture, STELLAR_GPU_RAY_STATUS_COUNT_V053B));
  assert(!stellarGpuRayStatusIsFatalV053b(0));
  assert(!stellarGpuRayStatusIsFatalV053b(STELLAR_GPU_RAY_INACTIVE_V053B));

  const uint32_t fatal_statuses[] = {
      STELLAR_GPU_RAY_INVALID_CELL_V053B,
      STELLAR_GPU_RAY_INVALID_EDGE_V053B,
      STELLAR_GPU_RAY_NEIGHBOR_OVERFLOW_V053B,
      STELLAR_GPU_RAY_NO_EXIT_FACE_V053B,
      STELLAR_GPU_RAY_CELL_LIMIT_V053B,
      STELLAR_GPU_RAY_NONFINITE_V053B};
  for(size_t index = 0; index < sizeof(fatal_statuses) / sizeof(fatal_statuses[0]);
      index++) {
    assert(stellarGpuRayStatusIsFatalV053b(fatal_statuses[index]));
    assert(stellarGpuRayStatusIsFatalV053b(
        STELLAR_GPU_RAY_INACTIVE_V053B | fatal_statuses[index]));
    uint64_t failing_fixture[STELLAR_GPU_RAY_STATUS_COUNT_V053B] = {};
    failing_fixture[index + 1] = 1;
    assert(!stellarGpuStatusCountsValidV053b(
        failing_fixture, STELLAR_GPU_RAY_STATUS_COUNT_V053B));
  }

  assert(stellarGpuRayStatusIsFatalV053b(1u << 7));
  assert(!stellarGpuStatusCountsValidV053b(0,
      STELLAR_GPU_RAY_STATUS_COUNT_V053B));
  assert(!stellarGpuStatusCountsValidV053b(background_fixture,
      STELLAR_GPU_RAY_STATUS_COUNT_V053B - 1));

  std::cout << "STELLAR_GPU_RAY_STATUS_V053B_OK inactive="
            << background_fixture[0] << " fatal_statuses=6\n";
  return 0;
}
