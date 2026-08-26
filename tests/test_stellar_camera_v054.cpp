#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

#include "stellar_camera_v054.h"

namespace {

StellarCameraLandmark landmark(double time)
{
  StellarCameraLandmark value = {};
  value.time_seconds = time;
  value.center[0] = 5.0e11;
  value.center[1] = 5.0e11;
  value.center[2] = 5.0e11;
  value.axis[2] = 1.0;
  value.material_radius_cm = 1.2e10;
  value.disk_radius_cm = 3.0e10;
  value.polar_extent_cm = 3.0e11;
  return value;
}

bool close(double left, double right, double tolerance)
{
  return std::abs(left - right) <= tolerance;
}

} // namespace

int main()
{
  const StellarCameraFilterParameters parameters = stellarDefaultCameraFilter();
  StellarCameraState state = {};
  StellarCameraLandmark first = landmark(0.0);
  assert(stellarUpdateCameraState(&state, first, parameters));
  assert(state.initialized && close(stellarCameraNorm(state.axis), 1.0, 1.0e-12));

  // Axis sign ambiguity must not cause a 180-degree camera flip.
  StellarCameraLandmark sign_flip = landmark(5.0);
  sign_flip.axis[2] = -1.0;
  assert(stellarUpdateCameraState(&state, sign_flip, parameters));
  assert(state.axis[2] > 0.999);

  // Axis motion is bounded independently of noisy landmark jumps.
  StellarCameraState axis_state = {};
  assert(stellarUpdateCameraState(&axis_state, landmark(0.0), parameters));
  const double old_axis[3] = {axis_state.axis[0], axis_state.axis[1],
                              axis_state.axis[2]};
  StellarCameraLandmark tilted = landmark(1.0);
  tilted.axis[0] = 1.0;
  tilted.axis[2] = 0.0;
  assert(stellarUpdateCameraState(&axis_state, tilted, parameters));
  const double axis_step = std::acos(stellarCameraClamp(
      stellarCameraDot(old_axis, axis_state.axis), -1.0, 1.0));
  assert(axis_step <= parameters.axis_max_rate_rad_per_s * 1.01);

  // Center motion is bounded by the configured acceleration.
  const double old_velocity = state.center_velocity[0];
  StellarCameraLandmark displaced = landmark(10.0);
  displaced.center[0] += 2.0e11;
  assert(stellarUpdateCameraState(&state, displaced, parameters));
  assert(std::abs(state.center_velocity[0] - old_velocity) <=
         parameters.center_max_acceleration_cm_per_s2 * 5.0 + 1.0e-6);

  // A small scale change stays inside hysteresis; a large outflow growth moves
  // smoothly and cannot jump to the target in one frame.
  const double disk_before = state.disk_half_extent_cm;
  StellarCameraLandmark small_scale = displaced;
  small_scale.time_seconds = 15.0;
  small_scale.disk_radius_cm *= 1.01;
  assert(stellarUpdateCameraState(&state, small_scale, parameters));
  assert(close(state.disk_half_extent_cm, disk_before, 1.0e-6));
  const double outflow_before = state.outflow_half_extent_cm;
  StellarCameraLandmark large_scale = small_scale;
  large_scale.time_seconds = 20.0;
  large_scale.polar_extent_cm *= 2.0;
  assert(stellarUpdateCameraState(&state, large_scale, parameters));
  assert(state.outflow_half_extent_cm > outflow_before);
  assert(state.outflow_half_extent_cm < 0.58 * large_scale.polar_extent_cm);
  assert(state.material_half_extent_cm < state.disk_half_extent_cm);

  StellarCameraShot shot = {};
  shot.mode = STELLAR_CAMERA_DISK_OBLIQUE;
  shot.framing_margin = 1.08;
  shot.orbit_rate_radians_per_second = 2.0e-4;
  StellarCameraPose first_pose = {};
  StellarCameraPose repeated_pose = {};
  assert(stellarCameraPoseForShot(state, shot, &first_pose));
  assert(stellarCameraPoseForShot(state, shot, &repeated_pose));
  assert(std::memcmp(&first_pose, &repeated_pose, sizeof(first_pose)) == 0);

  double view[3];
  for(int component = 0; component < 3; component++)
    view[component] = first_pose.position[component] -
        first_pose.look_at[component];
  assert(stellarCameraNormalize(view));
  assert(std::abs(stellarCameraDot(view, first_pose.up)) < 1.0e-12);

  shot.mode = STELLAR_CAMERA_OUTFLOW_SIDE;
  StellarCameraPose outflow_side = {};
  assert(stellarCameraPoseForShot(state, shot, &outflow_side));
  assert(outflow_side.screen_half_extent_cm > first_pose.screen_half_extent_cm);

  shot.mode = STELLAR_CAMERA_OUTFLOW_AXIS;
  StellarCameraPose axis_pose = {};
  assert(stellarCameraPoseForShot(state, shot, &axis_pose));
  assert(axis_pose.screen_half_extent_cm < outflow_side.screen_half_extent_cm);

  shot.mode = STELLAR_CAMERA_OUTFLOW_FOLLOW;
  shot.lobe_sign = 1.0;
  StellarCameraPose follow = {};
  assert(stellarCameraPoseForShot(state, shot, &follow));
  assert(follow.look_at[2] > state.center[2]);
  assert(follow.screen_half_extent_cm < outflow_side.screen_half_extent_cm);

  for(int component = 0; component < 3; component++) {
    assert(std::isfinite(axis_pose.position[component]));
    assert(std::isfinite(axis_pose.up[component]));
  }

  // Crossing the fallback-axis threshold must not cause a roll discontinuity.
  StellarCameraFilterParameters fast_axis = parameters;
  fast_axis.axis_response_seconds = 1.0e-3;
  fast_axis.axis_max_rate_rad_per_s = 1.0;
  StellarCameraState roll_state = {};
  StellarCameraLandmark before_threshold = landmark(0.0);
  before_threshold.axis[0] = 0.84;
  before_threshold.axis[2] = std::sqrt(1.0 - 0.84 * 0.84);
  assert(stellarUpdateCameraState(&roll_state, before_threshold, fast_axis));
  shot.mode = STELLAR_CAMERA_DISK_EDGE;
  shot.orbit_rate_radians_per_second = 0.0;
  StellarCameraPose before_pose = {};
  assert(stellarCameraPoseForShot(roll_state, shot, &before_pose));
  StellarCameraLandmark after_threshold = landmark(1.0);
  after_threshold.axis[0] = 0.86;
  after_threshold.axis[2] = std::sqrt(1.0 - 0.86 * 0.86);
  assert(stellarUpdateCameraState(&roll_state, after_threshold, fast_axis));
  StellarCameraPose after_pose = {};
  assert(stellarCameraPoseForShot(roll_state, shot, &after_pose));
  double before_view[3], after_view[3];
  for(int component = 0; component < 3; component++) {
    before_view[component] = before_pose.position[component] -
        before_pose.look_at[component];
    after_view[component] = after_pose.position[component] -
        after_pose.look_at[component];
  }
  assert(stellarCameraNormalize(before_view));
  assert(stellarCameraNormalize(after_view));
  assert(stellarCameraDot(before_view, after_view) > 0.99);

  std::cout << "STELLAR_CAMERA_V054_OK\n";
  return 0;
}
