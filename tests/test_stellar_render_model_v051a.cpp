#include <cassert>
#include <cmath>
#include <iostream>

#include "stellar_render_model_v051a.h"

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
  return value;
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
  assert(disk.color[0] > disk.color[1] && disk.color[1] > disk.color[2]);
  const StellarOpticalSample rejected_radial_disk = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_DISK), disk_position, 11.0f, 2.5e7f,
      radial_velocity);
  assert(rejected_radial_disk.extinction_per_cm == 0.0f);

  const StellarOpticalSample polar = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW), polar_position, 8.0f, 5.0e6f,
      outward_velocity);
  assert(polar.extinction_per_cm > 0.0f);
  assert(polar.color[2] > polar.color[0]);
  assert(std::abs(polar.color[0] - polar.color[1]) < 0.15f);
  const StellarOpticalSample rejected_inflow = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW), polar_position, 8.0f, 5.0e6f,
      inward_velocity);
  assert(rejected_inflow.extinction_per_cm == 0.0f);

  const StellarOpticalSample rejected_tail = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW), equatorial_tail, 8.0f, 5.0e6f,
      outward_velocity);
  assert(rejected_tail.extinction_per_cm == 0.0f);
  const StellarOpticalSample rejected_ambient = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW), polar_position, 5.0f, 1.0e3f,
      outward_velocity);
  assert(rejected_ambient.extinction_per_cm == 0.0f);

  StellarTransferParameters moving_parameters = parameters(STELLAR_TRANSFER_OUTFLOW);
  moving_parameters.bulk_velocity_cm_per_s[2] = 2.0e8f;
  const float comoving_velocity[3] = {0.0f, 0.0f, 2.0e8f};
  const float moving_outflow_velocity[3] = {0.0f, 0.0f, 5.0e8f};
  assert(evaluateStellarOpticalSample(moving_parameters, polar_position, 8.0f,
      5.0e6f, comoving_velocity).extinction_per_cm == 0.0f);
  assert(evaluateStellarOpticalSample(moving_parameters, polar_position, 8.0f,
      5.0e6f, moving_outflow_velocity).extinction_per_cm > 0.0f);

  const double wrapped_position[3] = {9.99e11, 5.0e11, 5.0e11};
  StellarTransferParameters wrapped_parameters = parameters(STELLAR_TRANSFER_MERGER);
  wrapped_parameters.center[0] = 1.0e9;
  wrapped_parameters.disk_radius_cm = 5.0e9f;
  const StellarOpticalSample wrapped = evaluateStellarOpticalSample(
      wrapped_parameters, wrapped_position, 12.0f, 1.0e8f);
  assert(wrapped.extinction_per_cm > 0.0f);

  const float dark = stellarFilmicMap(0.001f, 1.4f, 0.002f);
  const float middle = stellarFilmicMap(0.2f, 1.4f, 0.002f);
  const float bright = stellarFilmicMap(2.0f, 1.4f, 0.002f);
  assert(dark == 0.0f);
  assert(middle > dark && bright > middle && bright <= 1.0f);
  assert(std::abs(stellarFilmicMap(0.2f, 1.4f, 0.002f) - middle) < 1.0e-7f);

  std::cout << "STELLAR_RENDER_MODEL_V051A_OK\n";
  return 0;
}
