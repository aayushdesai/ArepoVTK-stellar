#ifndef AREPO_VTK_STELLAR_GPU_GEOMETRY_V053B_H
#define AREPO_VTK_STELLAR_GPU_GEOMETRY_V053B_H

#ifdef __CUDACC__
#define STELLAR_GPU_GEOMETRY_HD __host__ __device__
#else
#define STELLAR_GPU_GEOMETRY_HD
#endif

// Match periodic_wrap_point() in the native Voronoi traversal while retaining
// the scene's double-precision cell coordinates.
STELLAR_GPU_GEOMETRY_HD inline double stellarGpuWrapPositionV053b(
    double value, double reference, double box)
{
  const double delta = value - reference;
  if(delta > 0.5 * box)
    value -= box;
  if(delta < -0.5 * box)
    value += box;
  return value;
}

#undef STELLAR_GPU_GEOMETRY_HD

#endif
