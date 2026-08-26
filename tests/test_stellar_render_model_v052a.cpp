#include <cassert>
#include <cmath>
#include <iostream>

#include "stellar_render_model_v052a.h"

namespace {

StellarTransferParameters parameters(int mode)
{
  StellarTransferParameters value = {};
  value.mode = mode;
  value.center[0] = value.center[1] = value.center[2] = 5.0e11;
  value.axis[2] = 1.0;
  value.box_size = 1.0e12;
  value.material_radius_cm = 1.2e10f;
  value.disk_radius_cm = 3.0e10f;
  value.disk_half_thickness_cm = 3.0e9f;
  value.polar_inner_cm = 5.0e9f;
  value.polar_outer_cm = 5.0e11f;
  value.polar_cone_ratio = 0.7f;
  value.merger_extinction_per_cm = 3.0e-11f;
  value.disk_extinction_per_cm = 1.5e-11f;
  value.polar_extinction_per_cm = 1.2e-12f;
  value.merger_emissivity_per_cm = 3.0e-11f;
  value.disk_emissivity_per_cm = 1.5e-11f;
  value.polar_emissivity_per_cm = 1.2e-12f;
  return value;
}

float chromaticity(const StellarOpticalSample &sample, int channel)
{
  const float total = sample.emissivity_rgb_per_cm[0] +
      sample.emissivity_rgb_per_cm[1] + sample.emissivity_rgb_per_cm[2];
  assert(total > 0.0f);
  return sample.emissivity_rgb_per_cm[channel] / total;
}

bool isDark(const StellarOpticalSample &sample)
{
  return sample.extinction_per_cm == 0.0f &&
      sample.emissivity_rgb_per_cm[0] == 0.0f &&
      sample.emissivity_rgb_per_cm[1] == 0.0f &&
      sample.emissivity_rgb_per_cm[2] == 0.0f;
}

} // namespace

int main()
{
  const double disk_position[3] = {5.08e11, 5.0e11, 5.005e11};
  const double polar_position[3] = {5.02e11, 5.0e11, 6.0e11};
  const double equatorial_tail[3] = {6.0e11, 5.0e11, 5.02e11};
  const float rotating_velocity[3] = {0.0f, 3.0e8f, 0.0f};
  const float radial_velocity[3] = {3.0e8f, 0.0f, 0.0f};
  const float outward_velocity[3] = {0.0f, 0.0f, 3.0e8f};
  const float inward_velocity[3] = {0.0f, 0.0f, -3.0e8f};
  const StellarOpticalSample disk = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_DISK), disk_position, 11.0f, 2.5e7f,
      rotating_velocity);
  assert(disk.extinction_per_cm > 0.0f);
  assert(disk.emissivity_rgb_per_cm[0] > disk.emissivity_rgb_per_cm[1] &&
         disk.emissivity_rgb_per_cm[1] > disk.emissivity_rgb_per_cm[2]);
  const StellarOpticalSample lower_density_disk = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_DISK), disk_position, 10.45f, 2.5e7f,
      rotating_velocity);
  assert(lower_density_disk.extinction_per_cm > 0.0f);
  assert(std::abs(chromaticity(lower_density_disk, 0) - chromaticity(disk, 0)) >
         0.02f);
  const StellarOpticalSample rejected_radial_disk = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_DISK), disk_position, 11.0f, 2.5e7f,
      radial_velocity);
  assert(isDark(rejected_radial_disk));

  const StellarOpticalSample polar = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW), polar_position, 8.0f, 5.0e6f,
      outward_velocity);
  assert(polar.extinction_per_cm > 0.0f);
  assert(polar.emissivity_rgb_per_cm[2] > polar.emissivity_rgb_per_cm[0]);
  assert(std::abs(chromaticity(polar, 0) - chromaticity(polar, 1)) < 0.15f);
  const StellarOpticalSample denser_polar = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW), polar_position, 8.8f, 5.0e6f,
      outward_velocity);
  assert(denser_polar.extinction_per_cm > polar.extinction_per_cm);
  assert(denser_polar.emissivity_rgb_per_cm[2] > polar.emissivity_rgb_per_cm[2]);
  const StellarOpticalSample rejected_inflow = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW), polar_position, 8.0f, 5.0e6f,
      inward_velocity);
  assert(isDark(rejected_inflow));

  const StellarOpticalSample rejected_tail = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW), equatorial_tail, 8.0f, 5.0e6f,
      outward_velocity);
  assert(isDark(rejected_tail));
  const StellarOpticalSample rejected_ambient = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW), polar_position, 5.0f, 1.0e3f,
      outward_velocity);
  assert(isDark(rejected_ambient));

  StellarTransferParameters moving_parameters = parameters(STELLAR_TRANSFER_OUTFLOW);
  moving_parameters.bulk_velocity_cm_per_s[2] = 2.0e8f;
  const float comoving_velocity[3] = {0.0f, 0.0f, 2.0e8f};
  const float moving_outflow_velocity[3] = {0.0f, 0.0f, 5.0e8f};
  assert(isDark(evaluateStellarOpticalSample(moving_parameters, polar_position,
      8.0f, 5.0e6f, comoving_velocity)));
  assert(evaluateStellarOpticalSample(moving_parameters, polar_position, 8.0f,
      5.0e6f, moving_outflow_velocity).extinction_per_cm > 0.0f);

  const double wrapped_position[3] = {9.99e11, 5.0e11, 5.0e11};
  StellarTransferParameters wrapped_parameters = parameters(STELLAR_TRANSFER_MERGER);
  wrapped_parameters.center[0] = 1.0e9;
  wrapped_parameters.disk_radius_cm = 5.0e9f;
  const StellarOpticalSample wrapped = evaluateStellarOpticalSample(
      wrapped_parameters, wrapped_position, 12.0f, 1.0e8f);
  assert(wrapped.extinction_per_cm > 0.0f);

  StellarTransferParameters transparent_parameters =
      parameters(STELLAR_TRANSFER_OUTFLOW);
  transparent_parameters.polar_extinction_per_cm = 0.0f;
  const StellarOpticalSample transparent_polar = evaluateStellarOpticalSample(
      transparent_parameters, polar_position, 8.0f, 5.0e6f, outward_velocity);
  assert(transparent_polar.extinction_per_cm == 0.0f);
  assert(transparent_polar.emissivity_rgb_per_cm[2] > 0.0f);

  StellarTransferParameters faint_absorption = parameters(STELLAR_TRANSFER_OUTFLOW);
  faint_absorption.polar_extinction_per_cm *= 0.1f;
  const StellarOpticalSample faint_absorption_polar = evaluateStellarOpticalSample(
      faint_absorption, polar_position, 8.0f, 5.0e6f, outward_velocity);
  assert(faint_absorption_polar.extinction_per_cm < polar.extinction_per_cm);
  for(int channel = 0; channel < 3; channel++)
    assert(std::abs(faint_absorption_polar.emissivity_rgb_per_cm[channel] -
                    polar.emissivity_rgb_per_cm[channel]) < 1.0e-20f);

  StellarOpticalSample optically_thin = {{2.0e-12f, 1.0e-12f, 0.5e-12f}, 0.0f};
  const StellarIntegratedSegment thin_segment =
      integrateStellarOpticalSegment(optically_thin, 1.0e10f);
  assert(thin_segment.transmittance == 1.0f);
  assert(std::abs(thin_segment.radiance[0] - 0.02f) < 1.0e-6f);
  assert(std::abs(thin_segment.radiance[1] - 0.01f) < 1.0e-6f);

  StellarOpticalSample absorbing = {{4.0e-10f, 2.0e-10f, 1.0e-10f}, 2.0e-10f};
  const StellarIntegratedSegment absorbing_segment =
      integrateStellarOpticalSegment(absorbing, 5.0e9f);
  const float one_minus_exp_minus_one = 1.0f - std::exp(-1.0f);
  assert(std::abs(absorbing_segment.transmittance - std::exp(-1.0f)) < 1.0e-6f);
  assert(std::abs(absorbing_segment.radiance[0] -
                  2.0f * one_minus_exp_minus_one) < 1.0e-5f);
  assert(std::abs(absorbing_segment.radiance[1] -
                  one_minus_exp_minus_one) < 1.0e-5f);

  const StellarIntegratedSegment empty_segment =
      integrateStellarOpticalSegment(absorbing, 0.0f);
  assert(empty_segment.transmittance == 1.0f);
  assert(empty_segment.radiance[0] == 0.0f);

  const float dark = stellarFilmicMap(0.001f, 1.4f, 0.002f);
  const float middle = stellarFilmicMap(0.2f, 1.4f, 0.002f);
  const float bright = stellarFilmicMap(2.0f, 1.4f, 0.002f);
  assert(dark == 0.0f);
  assert(middle > dark && bright > middle && bright <= 1.0f);
  assert(std::abs(stellarFilmicMap(0.2f, 1.4f, 0.002f) - middle) < 1.0e-7f);

  std::cout << "STELLAR_RENDER_MODEL_V052A_OK\n";
  return 0;
}
