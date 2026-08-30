#ifndef AREPO_VTK_STELLAR_GPU_RAY_DEPTH_V080_H
#define AREPO_VTK_STELLAR_GPU_RAY_DEPTH_V080_H

#include <cmath>

#ifdef __CUDACC__
#define STELLAR_GPU_RAY_DEPTH_V080_HD __host__ __device__
#else
#define STELLAR_GPU_RAY_DEPTH_V080_HD
#endif

struct StellarGpuRayDepthDecisionV080 {
  double maximum_t;
  int limited;
  int valid;
};

STELLAR_GPU_RAY_DEPTH_V080_HD inline bool
stellarGpuRayDepthFiniteV080(double value)
{
#ifdef __CUDA_ARCH__
  return isfinite(value);
#else
  return std::isfinite(value);
#endif
}

STELLAR_GPU_RAY_DEPTH_V080_HD inline bool
stellarGpuRayDepthInputsValidV080(double legacy_absolute_maximum_t,
                                  double traversal_length_cm)
{
  return stellarGpuRayDepthFiniteV080(legacy_absolute_maximum_t) &&
      stellarGpuRayDepthFiniteV080(traversal_length_cm) &&
      legacy_absolute_maximum_t >= 0.0 && traversal_length_cm >= 0.0 &&
      !(legacy_absolute_maximum_t > 0.0 && traversal_length_cm > 0.0);
}

// rayMaxT is retained as the historical absolute camera-ray parameter.
// stellarGpuRayTraversalLengthCm is measured from the box entry point, which
// is the unambiguous limit needed by far-camera scene exports.
STELLAR_GPU_RAY_DEPTH_V080_HD inline StellarGpuRayDepthDecisionV080
stellarResolveGpuRayDepthV080(double entry_t, double box_exit_t,
                              double legacy_absolute_maximum_t,
                              double traversal_length_cm)
{
  StellarGpuRayDepthDecisionV080 output = {box_exit_t, 0, 0};
  if(!stellarGpuRayDepthInputsValidV080(
         legacy_absolute_maximum_t, traversal_length_cm) ||
     !stellarGpuRayDepthFiniteV080(entry_t) ||
     !stellarGpuRayDepthFiniteV080(box_exit_t) ||
     !(box_exit_t > entry_t))
    return output;

  double requested_maximum_t = box_exit_t;
  if(traversal_length_cm > 0.0)
    requested_maximum_t = entry_t + traversal_length_cm;
  else if(legacy_absolute_maximum_t > 0.0)
    requested_maximum_t = legacy_absolute_maximum_t;

  if(requested_maximum_t < output.maximum_t) {
    output.maximum_t = requested_maximum_t;
    output.limited = 1;
  }
  output.valid = output.maximum_t > entry_t;
  return output;
}

#undef STELLAR_GPU_RAY_DEPTH_V080_HD

#endif
