#ifndef AREPO_VTK_STELLAR_RECONSTRUCTION_V053_H
#define AREPO_VTK_STELLAR_RECONSTRUCTION_V053_H

#include <cmath>

#ifdef __CUDACC__
#define STELLAR_RECON_HD __host__ __device__
#else
#define STELLAR_RECON_HD
#endif

enum StellarReconstructionMode {
  STELLAR_RECONSTRUCTION_SPH = 0,
  STELLAR_RECONSTRUCTION_IDW = 1,
  STELLAR_RECONSTRUCTION_VORONOI = 2
};

struct StellarReconstructionAccumulator {
  float weight;
  float density;
  float temperature;
  float velocity[3];
};

STELLAR_RECON_HD inline const char *stellarReconstructionModeName(int mode)
{
  if(mode == STELLAR_RECONSTRUCTION_SPH)
    return "sph";
  if(mode == STELLAR_RECONSTRUCTION_IDW)
    return "idw";
  if(mode == STELLAR_RECONSTRUCTION_VORONOI)
    return "voronoi";
  return "invalid";
}

STELLAR_RECON_HD inline bool stellarReconstructionFinite(float value)
{
#ifdef __CUDA_ARCH__
  return isfinite(value);
#else
  return std::isfinite(value);
#endif
}

STELLAR_RECON_HD inline float stellarIdwWeight(float distance, float power)
{
  if(!(distance > 0.0f) || !(power > 0.0f) ||
     !stellarReconstructionFinite(distance) ||
     !stellarReconstructionFinite(power))
    return 0.0f;
  return powf(distance, -power);
}

// This is the existing ArepoVTK cubic-spline kernel. Normalization cancels
// when reconstructed fields are divided by the accumulated weight.
STELLAR_RECON_HD inline float stellarSphKernelWeight(float distance,
                                                     float inverse_support)
{
  if(distance < 0.0f || !(inverse_support > 0.0f) ||
     !stellarReconstructionFinite(distance) ||
     !stellarReconstructionFinite(inverse_support))
    return 0.0f;
  const float u = distance * inverse_support;
  if(u < 0.5f)
    return 2.546479089470f + 15.278874536822f * (u - 1.0f) * u * u;
  if(u < 1.0f)
    return 5.092958178941f * (1.0f - u) * (1.0f - u) * (1.0f - u);
  return 0.0f;
}

STELLAR_RECON_HD inline float stellarReconstructionWeight(
    int mode, float distance, float inverse_support, float idw_power)
{
  if(mode == STELLAR_RECONSTRUCTION_SPH)
    return stellarSphKernelWeight(distance, inverse_support);
  if(mode == STELLAR_RECONSTRUCTION_IDW)
    return stellarIdwWeight(distance, idw_power);
  return 0.0f;
}

STELLAR_RECON_HD inline StellarReconstructionAccumulator
stellarEmptyReconstructionAccumulator()
{
  StellarReconstructionAccumulator value = {};
  return value;
}

STELLAR_RECON_HD inline void stellarAccumulateReconstruction(
    StellarReconstructionAccumulator *accumulator, float weight,
    float density, float temperature, const float velocity[3])
{
  if(!accumulator || !(weight > 0.0f) ||
     !stellarReconstructionFinite(weight))
    return;
  accumulator->weight += weight;
  accumulator->density += weight * density;
  accumulator->temperature += weight * temperature;
  for(int component = 0; component < 3; component++)
    accumulator->velocity[component] += weight * velocity[component];
}

STELLAR_RECON_HD inline bool stellarNormalizeReconstruction(
    StellarReconstructionAccumulator *accumulator)
{
  if(!accumulator || !(accumulator->weight > 0.0f) ||
     !stellarReconstructionFinite(accumulator->weight))
    return false;
  const float inverse_weight = 1.0f / accumulator->weight;
  accumulator->density *= inverse_weight;
  accumulator->temperature *= inverse_weight;
  for(int component = 0; component < 3; component++)
    accumulator->velocity[component] *= inverse_weight;
  return stellarReconstructionFinite(accumulator->density) &&
      stellarReconstructionFinite(accumulator->temperature) &&
      stellarReconstructionFinite(accumulator->velocity[0]) &&
      stellarReconstructionFinite(accumulator->velocity[1]) &&
      stellarReconstructionFinite(accumulator->velocity[2]);
}

#undef STELLAR_RECON_HD

#endif
