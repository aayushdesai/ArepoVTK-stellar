#include "stellar_gpu_neighbor_reference_v053b.h"
#include "stellar_reconstruction_v053.h"

#include <assert.h>
#include <cmath>
#include <iostream>

int main()
{
  const int references[] = {
      stellarGpuEncodeNeighborV053b(4, true),
      stellarGpuEncodeNeighborV053b(4, true),
      stellarGpuEncodeNeighborV053b(7, false)};
  assert(references[0] == 5 && references[1] == 5 && references[2] == -8);
  assert(stellarGpuDecodeNeighborV053b(references[0]) == 4);
  assert(stellarGpuDecodeNeighborV053b(references[2]) == 7);
  assert(stellarGpuNeighborContributesV053b(references[0]));
  assert(!stellarGpuNeighborContributesV053b(references[2]));
  assert(stellarGpuDecodeNeighborV053b(0) == -1);

  StellarReconstructionAccumulator accumulator =
      stellarEmptyReconstructionAccumulator();
  const float velocity[3] = {3.0f, 4.0f, 5.0f};
  for(size_t index = 0; index < sizeof(references) / sizeof(references[0]); index++) {
    if(stellarGpuNeighborContributesV053b(references[index]))
      stellarAccumulateReconstruction(
          &accumulator, 1.0f, 10.0f, 20.0f, velocity);
  }
  const float parent_velocity[3] = {0.0f, 0.0f, 0.0f};
  stellarAccumulateReconstruction(
      &accumulator, 1.0f, 0.0f, 0.0f, parent_velocity);
  assert(stellarNormalizeReconstruction(&accumulator));
  assert(std::fabs(accumulator.density - 20.0f / 3.0f) < 1.0e-6f);
  assert(std::fabs(accumulator.temperature - 40.0f / 3.0f) < 1.0e-6f);
  assert(std::fabs(accumulator.velocity[0] - 2.0f) < 1.0e-6f);

  std::cout << "STELLAR_GPU_NEIGHBOR_REFERENCE_V053B_OK duplicate_contributors=2"
            << " support_only_ghosts=1 density=" << accumulator.density << "\n";
  return 0;
}
