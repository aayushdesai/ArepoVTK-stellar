#include "stellar_gpu_geometry_v053b.h"

#include <assert.h>
#include <cmath>
#include <iostream>

int main()
{
  const double box = 1.0e12;
  const double reference = 5.0e11;
  const double neighbor = 5.00123456789123e11;
  assert(stellarGpuWrapPositionV053b(neighbor, reference, box) == neighbor);

  const double periodic_reference = 1.0e10;
  const double periodic_neighbor = 9.9e11;
  assert(stellarGpuWrapPositionV053b(
      periodic_neighbor, periodic_reference, box) == -1.0e10);

  const float legacy_delta = static_cast<float>(neighbor - reference);
  const double legacy_reconstructed = reference + legacy_delta;
  const double double_reconstructed = stellarGpuWrapPositionV053b(
      neighbor, reference, box);
  assert(double_reconstructed == neighbor);
  assert(std::fabs(legacy_reconstructed - neighbor) > 0.1);

  std::cout << "STELLAR_GPU_GEOMETRY_V053B_OK legacy_error_cm="
            << std::fabs(legacy_reconstructed - neighbor)
            << " periodic_neighbor="
            << stellarGpuWrapPositionV053b(
                   periodic_neighbor, periodic_reference, box)
            << "\n";
  return 0;
}
