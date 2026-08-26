#include <cassert>
#include <iostream>

#include "stellar_gpu_scene_format_v052.h"
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

StellarOpticalSample evaluate(const StellarTransferParameters &transfer,
                              const ArepoStellarCell &cell)
{
  return evaluateStellarOpticalSample(
      transfer, cell.position, cell.density_log10_plus_10,
      cell.temperature_kelvin, cell.velocity_cm_per_s);
}

} // namespace

int main()
{
  ArepoStellarCell disk = {};
  disk.position[0] = 5.08e11;
  disk.position[1] = 5.0e11;
  disk.position[2] = 5.005e11;
  disk.density_log10_plus_10 = 11.0f;
  disk.temperature_kelvin = 2.5e7f;
  disk.velocity_cm_per_s[1] = 3.0e8f;
  disk.particle_id = 101;

  ArepoStellarCell radial = disk;
  radial.velocity_cm_per_s[0] = 3.0e8f;
  radial.velocity_cm_per_s[1] = 0.0f;
  radial.particle_id = 102;

  const StellarOpticalSample accepted_disk =
      evaluate(parameters(STELLAR_TRANSFER_DISK), disk);
  const StellarOpticalSample rejected_radial =
      evaluate(parameters(STELLAR_TRANSFER_DISK), radial);
  assert(accepted_disk.extinction_per_cm > 0.0f);
  assert(rejected_radial.extinction_per_cm == 0.0f);
  assert(disk.particle_id != radial.particle_id);

  ArepoStellarCell outflow = {};
  outflow.position[0] = 5.02e11;
  outflow.position[1] = 5.0e11;
  outflow.position[2] = 6.0e11;
  outflow.density_log10_plus_10 = 8.0f;
  outflow.temperature_kelvin = 5.0e6f;
  outflow.velocity_cm_per_s[2] = 3.0e8f;
  outflow.particle_id = 201;

  ArepoStellarCell inflow = outflow;
  inflow.velocity_cm_per_s[2] = -3.0e8f;
  inflow.particle_id = 202;

  const StellarOpticalSample accepted_outflow =
      evaluate(parameters(STELLAR_TRANSFER_OUTFLOW), outflow);
  const StellarOpticalSample rejected_inflow =
      evaluate(parameters(STELLAR_TRANSFER_OUTFLOW), inflow);
  assert(accepted_outflow.extinction_per_cm > 0.0f);
  assert(accepted_outflow.emissivity_rgb_per_cm[2] > 0.0f);
  assert(rejected_inflow.extinction_per_cm == 0.0f);
  assert(rejected_inflow.emissivity_rgb_per_cm[2] == 0.0f);

  StellarTransferParameters moving = parameters(STELLAR_TRANSFER_OUTFLOW);
  moving.bulk_velocity_cm_per_s[2] = 2.0e8f;
  ArepoStellarCell comoving = outflow;
  comoving.velocity_cm_per_s[2] = 2.0e8f;
  ArepoStellarCell moving_outflow = outflow;
  moving_outflow.velocity_cm_per_s[2] = 5.0e8f;
  assert(evaluate(moving, comoving).extinction_per_cm == 0.0f);
  assert(evaluate(moving, moving_outflow).extinction_per_cm > 0.0f);

  const StellarIntegratedSegment segment =
      integrateStellarOpticalSegment(accepted_outflow, 1.0e9f);
  assert(segment.radiance[2] > 0.0f);
  assert(segment.transmittance > 0.0f && segment.transmittance < 1.0f);

  std::cout << "STELLAR_SCENE_PHYSICS_V052_OK\n";
  return 0;
}
