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
  value.disk_radius_cm = 3.0e10f;
  value.disk_half_thickness_cm = 3.0e9f;
  value.polar_inner_cm = 5.0e9f;
  value.polar_outer_cm = 5.0e11f;
  value.polar_cone_ratio = 0.7f;
  return value;
}

} // namespace

int main()
{
  const double disk_position[3] = {5.08e11, 5.0e11, 5.005e11};
  const double polar_position[3] = {5.02e11, 5.0e11, 6.0e11};
  const double equatorial_tail[3] = {6.0e11, 5.0e11, 5.02e11};
  const StellarOpticalSample disk = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_DISK), disk_position, 11.0f, 2.5e7f);
  assert(disk.extinction_per_cm > 0.0f);
  assert(disk.color[0] > disk.color[1] && disk.color[1] > disk.color[2]);

  const StellarOpticalSample polar = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW), polar_position, 8.0f, 5.0e6f);
  assert(polar.extinction_per_cm > 0.0f);
  assert(polar.color[2] > polar.color[1] && polar.color[1] > polar.color[0]);

  const StellarOpticalSample rejected_tail = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW), equatorial_tail, 8.0f, 5.0e6f);
  assert(rejected_tail.extinction_per_cm == 0.0f);
  const StellarOpticalSample rejected_ambient = evaluateStellarOpticalSample(
      parameters(STELLAR_TRANSFER_OUTFLOW), polar_position, 5.0f, 1.0e3f);
  assert(rejected_ambient.extinction_per_cm == 0.0f);

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
