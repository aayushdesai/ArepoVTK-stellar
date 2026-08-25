#ifndef AREPO_VTK_STELLAR_RENDER_MODEL_V051A_H
#define AREPO_VTK_STELLAR_RENDER_MODEL_V051A_H

#include <cmath>

#ifdef __CUDACC__
#define STELLAR_HD __host__ __device__
#else
#define STELLAR_HD
#endif

enum StellarTransferMode {
  STELLAR_TRANSFER_MERGER = 0,
  STELLAR_TRANSFER_DISK = 1,
  STELLAR_TRANSFER_OUTFLOW = 2,
  STELLAR_TRANSFER_COMPOSITE = 3
};

struct StellarTransferParameters {
  int mode;
  double center[3];
  double axis[3];
  double box_size;
  float disk_radius_cm;
  float disk_half_thickness_cm;
  float polar_inner_cm;
  float polar_outer_cm;
  float polar_cone_ratio;
};

struct StellarOpticalSample {
  float color[3];
  float extinction_per_cm;
};

STELLAR_HD inline float stellarClamp(float value, float low, float high)
{
  return value < low ? low : (value > high ? high : value);
}

STELLAR_HD inline bool stellarFinite(float value)
{
#ifdef __CUDA_ARCH__
  return isfinite(value);
#else
  return std::isfinite(value);
#endif
}

STELLAR_HD inline float stellarSmoothstep(float low, float high, float value)
{
  if(!(high > low))
    return value >= high ? 1.0f : 0.0f;
  const float t = stellarClamp((value - low) / (high - low), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

STELLAR_HD inline double stellarPeriodicDelta(double value, double center, double box)
{
  double delta = value - center;
  if(delta > 0.5 * box)
    delta -= box;
  if(delta < -0.5 * box)
    delta += box;
  return delta;
}

STELLAR_HD inline void stellarTemperatureColor(float log_temperature, float output[3])
{
  const float stops[5] = {5.8f, 6.5f, 7.2f, 8.0f, 8.7f};
  const float colors[5][3] = {
      {0.34f, 0.035f, 0.008f},
      {0.72f, 0.13f, 0.025f},
      {1.00f, 0.52f, 0.10f},
      {1.00f, 0.92f, 0.67f},
      {0.64f, 0.79f, 1.00f}};
  int left = 0;
  while(left < 3 && log_temperature > stops[left + 1])
    left++;
  const float t = stellarSmoothstep(stops[left], stops[left + 1], log_temperature);
  for(int channel = 0; channel < 3; channel++)
    output[channel] = colors[left][channel] +
        t * (colors[left + 1][channel] - colors[left][channel]);
}

STELLAR_HD inline StellarOpticalSample evaluateStellarOpticalSample(
    const StellarTransferParameters &parameters, const double position[3],
    float density_log10_plus_10, float temperature_kelvin)
{
  StellarOpticalSample output = {{0.0f, 0.0f, 0.0f}, 0.0f};
  if(!(temperature_kelvin > 0.0f) || !stellarFinite(temperature_kelvin) ||
     !stellarFinite(density_log10_plus_10))
    return output;

  const float log_density = density_log10_plus_10 - 10.0f;
  const float log_temperature = log10f(temperature_kelvin);
  const float thermal_support = stellarSmoothstep(5.7f, 6.15f, log_temperature) *
      (1.0f - stellarSmoothstep(8.72f, 8.95f, log_temperature));
  if(thermal_support <= 0.0f)
    return output;

  const double dx = stellarPeriodicDelta(position[0], parameters.center[0], parameters.box_size);
  const double dy = stellarPeriodicDelta(position[1], parameters.center[1], parameters.box_size);
  const double dz = stellarPeriodicDelta(position[2], parameters.center[2], parameters.box_size);
  const double height = dx * parameters.axis[0] + dy * parameters.axis[1] +
      dz * parameters.axis[2];
  const double radius_squared = dx * dx + dy * dy + dz * dz;
  const double cylindrical_squared = radius_squared - height * height;
  const float cylindrical_radius = float(sqrt(cylindrical_squared > 0.0 ? cylindrical_squared : 0.0));
  const float absolute_height = float(fabs(height));

  const float merger_density = stellarSmoothstep(-3.6f, -2.5f, log_density);
  const float merger_radius = 1.0f - stellarSmoothstep(
      0.82f * parameters.disk_radius_cm, parameters.disk_radius_cm,
      float(sqrt(radius_squared)));
  const float merger_weight = thermal_support * merger_density * merger_radius;

  const float disk_plane = 1.0f - stellarSmoothstep(
      0.65f * parameters.disk_half_thickness_cm,
      parameters.disk_half_thickness_cm, absolute_height);
  const float disk_radius = 1.0f - stellarSmoothstep(
      0.82f * parameters.disk_radius_cm, parameters.disk_radius_cm,
      cylindrical_radius);
  const float disk_density = stellarSmoothstep(-0.9f, 0.25f, log_density);
  const float disk_temperature = stellarSmoothstep(6.75f, 7.15f, log_temperature) *
      (1.0f - stellarSmoothstep(8.25f, 8.62f, log_temperature));
  const float disk_weight = disk_plane * disk_radius * disk_density * disk_temperature;

  const float cone_coordinate = cylindrical_radius / (absolute_height + 1.0e6f);
  const float cone = 1.0f - stellarSmoothstep(
      0.72f * parameters.polar_cone_ratio,
      parameters.polar_cone_ratio, cone_coordinate);
  const float polar_height = stellarSmoothstep(
      parameters.polar_inner_cm, 1.35f * parameters.polar_inner_cm,
      absolute_height) *
      (1.0f - stellarSmoothstep(0.92f * parameters.polar_outer_cm,
                               parameters.polar_outer_cm, absolute_height));
  const float polar_density = stellarSmoothstep(-4.2f, -3.35f, log_density) *
      (1.0f - stellarSmoothstep(0.25f, 0.85f, log_density));
  const float polar_temperature = stellarSmoothstep(5.95f, 6.35f, log_temperature) *
      (1.0f - stellarSmoothstep(7.45f, 7.85f, log_temperature));
  const float polar_weight = cone * polar_height * polar_density * polar_temperature;

  float blackbody[3];
  stellarTemperatureColor(log_temperature, blackbody);
  const float disk_color[3] = {1.00f, 0.53f, 0.11f};
  const float polar_color[3] = {0.16f, 0.30f, 1.00f};
  float merger_mix = 0.0f;
  float disk_mix = 0.0f;
  float polar_mix = 0.0f;
  if(parameters.mode == STELLAR_TRANSFER_MERGER)
    merger_mix = merger_weight;
  else if(parameters.mode == STELLAR_TRANSFER_DISK)
    disk_mix = disk_weight;
  else if(parameters.mode == STELLAR_TRANSFER_OUTFLOW)
    polar_mix = polar_weight;
  else {
    merger_mix = 0.65f * merger_weight;
    disk_mix = disk_weight;
    polar_mix = polar_weight;
  }

  const float total = merger_mix + disk_mix + polar_mix;
  if(total <= 0.0f)
    return output;
  for(int channel = 0; channel < 3; channel++)
    output.color[channel] = (merger_mix * blackbody[channel] +
        disk_mix * disk_color[channel] + polar_mix * polar_color[channel]) / total;
  output.extinction_per_cm = merger_mix * 2.5e-10f +
      disk_mix * 4.0e-10f + polar_mix * 1.2e-11f;
  return output;
}

STELLAR_HD inline float stellarFilmicMap(float linear_value, float exposure,
                                         float black_point)
{
  float value = fmaxf(0.0f, linear_value * exposure - black_point);
  value = (value * (2.51f * value + 0.03f)) /
      (value * (2.43f * value + 0.59f) + 0.14f);
  value = stellarClamp(value, 0.0f, 1.0f);
  return powf(value, 1.0f / 2.2f);
}

#undef STELLAR_HD

#endif
