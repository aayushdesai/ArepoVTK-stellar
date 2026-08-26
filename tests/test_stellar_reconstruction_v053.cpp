#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

#include "stellar_reconstruction_v053.h"

namespace {

bool close(float left, float right, float tolerance = 1.0e-6f)
{
  return std::abs(left - right) <= tolerance;
}

float legacySphWeight(float distance, float inverse_support)
{
  const float u = distance * inverse_support;
  if(u < 0.5f)
    return 2.546479089470f + 15.278874536822f * (u - 1.0f) * u * u;
  if(u < 1.0f)
    return 5.092958178941f * (1.0f - u) * (1.0f - u) * (1.0f - u);
  return 0.0f;
}

} // namespace

int main()
{
  assert(std::strcmp(stellarReconstructionModeName(STELLAR_RECONSTRUCTION_SPH),
                     "sph") == 0);
  assert(std::strcmp(stellarReconstructionModeName(STELLAR_RECONSTRUCTION_IDW),
                     "idw") == 0);
  assert(std::strcmp(stellarReconstructionModeName(STELLAR_RECONSTRUCTION_VORONOI),
                     "voronoi") == 0);
  assert(std::strcmp(stellarReconstructionModeName(-1), "invalid") == 0);

  const float distances[] = {0.0f, 0.2f, 0.49f, 0.5f, 0.8f, 1.0f, 1.2f};
  for(size_t index = 0; index < sizeof(distances) / sizeof(distances[0]); index++)
    assert(close(stellarSphKernelWeight(distances[index], 1.0f),
                 legacySphWeight(distances[index], 1.0f)));
  assert(close(stellarIdwWeight(2.0f, 2.0f), 0.25f));
  assert(stellarIdwWeight(0.0f, 2.0f) == 0.0f);
  assert(stellarIdwWeight(2.0f, 0.0f) == 0.0f);

  const float constant_velocity[3] = {4.0f, -2.0f, 1.0f};
  for(int mode = STELLAR_RECONSTRUCTION_SPH;
      mode <= STELLAR_RECONSTRUCTION_IDW; mode++) {
    StellarReconstructionAccumulator accumulator =
        stellarEmptyReconstructionAccumulator();
    const float sample_distances[3] = {0.20f, 0.35f, 0.70f};
    for(int sample = 0; sample < 3; sample++) {
      const float weight = stellarReconstructionWeight(
          mode, sample_distances[sample], 1.0f, 2.0f);
      stellarAccumulateReconstruction(&accumulator, weight, 7.0f, 11.0f,
                                      constant_velocity);
    }
    assert(stellarNormalizeReconstruction(&accumulator));
    assert(close(accumulator.density, 7.0f));
    assert(close(accumulator.temperature, 11.0f));
    for(int component = 0; component < 3; component++)
      assert(close(accumulator.velocity[component], constant_velocity[component]));
  }

  // A symmetric fixture must retain a zero first moment for both smoothing
  // modes. This is the landmark guard used before image metrics.
  for(int mode = STELLAR_RECONSTRUCTION_SPH;
      mode <= STELLAR_RECONSTRUCTION_IDW; mode++) {
    float weighted_position = 0.0f;
    float total_weight = 0.0f;
    const float positions[2] = {-0.4f, 0.4f};
    for(int sample = 0; sample < 2; sample++) {
      const float weight = stellarReconstructionWeight(
          mode, std::abs(positions[sample]), 1.5f, 2.0f);
      weighted_position += weight * positions[sample];
      total_weight += weight;
    }
    assert(total_weight > 0.0f);
    assert(std::abs(weighted_position / total_weight) < 1.0e-7f);
  }

  // Voronoi-native reconstruction is exactly the containing cell value.
  StellarReconstructionAccumulator voronoi =
      stellarEmptyReconstructionAccumulator();
  const float parent_velocity[3] = {3.0f, 4.0f, 5.0f};
  stellarAccumulateReconstruction(&voronoi, 1.0f, 9.0f, 13.0f,
                                  parent_velocity);
  assert(stellarNormalizeReconstruction(&voronoi));
  assert(voronoi.weight == 1.0f);
  assert(voronoi.density == 9.0f && voronoi.temperature == 13.0f);
  assert(voronoi.velocity[0] == 3.0f && voronoi.velocity[2] == 5.0f);

  std::cout << "STELLAR_RECONSTRUCTION_V053_OK\n";
  return 0;
}
