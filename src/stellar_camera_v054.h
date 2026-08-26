#ifndef AREPO_VTK_STELLAR_CAMERA_V054_H
#define AREPO_VTK_STELLAR_CAMERA_V054_H

#include <algorithm>
#include <cmath>

enum StellarCameraShotMode {
  STELLAR_CAMERA_DISK_EDGE = 0,
  STELLAR_CAMERA_DISK_OBLIQUE = 1,
  STELLAR_CAMERA_OUTFLOW_SIDE = 2,
  STELLAR_CAMERA_OUTFLOW_AXIS = 3,
  STELLAR_CAMERA_OUTFLOW_FOLLOW = 4,
  STELLAR_CAMERA_ORBIT = 5
};

struct StellarCameraLandmark {
  double time_seconds;
  double center[3];
  double axis[3];
  double material_radius_cm;
  double disk_radius_cm;
  double polar_extent_cm;
};

struct StellarCameraFilterParameters {
  double center_response_seconds;
  double center_max_speed_cm_per_s;
  double center_max_acceleration_cm_per_s2;
  double axis_response_seconds;
  double axis_max_rate_rad_per_s;
  double scale_response_seconds;
  double scale_max_log_rate_per_s;
  double expand_hysteresis_fraction;
  double contract_hysteresis_fraction;
  double minimum_half_extent_cm;
};

struct StellarCameraState {
  int initialized;
  double origin_time_seconds;
  double time_seconds;
  double center[3];
  double center_velocity[3];
  double axis[3];
  double azimuth_zero[3];
  double material_half_extent_cm;
  double disk_half_extent_cm;
  double outflow_half_extent_cm;
};

struct StellarCameraShot {
  int mode;
  double azimuth_radians;
  double elevation_radians;
  double orbit_rate_radians_per_second;
  double framing_margin;
  double lobe_sign;
};

struct StellarCameraPose {
  double position[3];
  double look_at[3];
  double up[3];
  double screen_half_extent_cm;
};

inline double stellarCameraClamp(double value, double low, double high)
{
  return std::max(low, std::min(high, value));
}

inline double stellarCameraDot(const double left[3], const double right[3])
{
  return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

inline double stellarCameraNorm(const double value[3])
{
  return std::sqrt(stellarCameraDot(value, value));
}

inline bool stellarCameraNormalize(double value[3])
{
  const double norm = stellarCameraNorm(value);
  if(!(norm > 0.0) || !std::isfinite(norm))
    return false;
  for(int component = 0; component < 3; component++)
    value[component] /= norm;
  return true;
}

inline void stellarCameraCross(const double left[3], const double right[3],
                               double output[3])
{
  output[0] = left[1] * right[2] - left[2] * right[1];
  output[1] = left[2] * right[0] - left[0] * right[2];
  output[2] = left[0] * right[1] - left[1] * right[0];
}

inline void stellarCameraLimitMagnitude(double value[3], double maximum)
{
  const double norm = stellarCameraNorm(value);
  if(norm > maximum && maximum >= 0.0) {
    const double scale = maximum / norm;
    for(int component = 0; component < 3; component++)
      value[component] *= scale;
  }
}

inline bool stellarCameraFiniteVector(const double value[3])
{
  return std::isfinite(value[0]) && std::isfinite(value[1]) &&
      std::isfinite(value[2]);
}

inline bool stellarCameraPerpendicular(const double axis[3], double output[3])
{
  const double reference_x[3] = {1.0, 0.0, 0.0};
  const double reference_y[3] = {0.0, 1.0, 0.0};
  const double *reference = std::abs(axis[0]) < 0.85 ? reference_x : reference_y;
  stellarCameraCross(axis, reference, output);
  return stellarCameraNormalize(output);
}

inline bool stellarCameraValidFilter(
    const StellarCameraFilterParameters &parameters)
{
  return parameters.center_response_seconds > 0.0 &&
      parameters.center_max_speed_cm_per_s > 0.0 &&
      parameters.center_max_acceleration_cm_per_s2 > 0.0 &&
      parameters.axis_response_seconds > 0.0 &&
      parameters.axis_max_rate_rad_per_s > 0.0 &&
      parameters.scale_response_seconds > 0.0 &&
      parameters.scale_max_log_rate_per_s > 0.0 &&
      parameters.expand_hysteresis_fraction >= 0.0 &&
      parameters.contract_hysteresis_fraction >= 0.0 &&
      parameters.minimum_half_extent_cm > 0.0;
}

inline StellarCameraFilterParameters stellarDefaultCameraFilter()
{
  StellarCameraFilterParameters value = {};
  value.center_response_seconds = 80.0;
  value.center_max_speed_cm_per_s = 3.0e8;
  value.center_max_acceleration_cm_per_s2 = 8.0e5;
  value.axis_response_seconds = 140.0;
  value.axis_max_rate_rad_per_s = 0.003;
  value.scale_response_seconds = 180.0;
  value.scale_max_log_rate_per_s = 0.0035;
  value.expand_hysteresis_fraction = 0.04;
  value.contract_hysteresis_fraction = 0.12;
  value.minimum_half_extent_cm = 2.0e10;
  return value;
}

inline double stellarCameraFilteredExtent(
    double current, double target, double dt,
    const StellarCameraFilterParameters &parameters)
{
  target = std::max(target, parameters.minimum_half_extent_cm);
  if(!(current > 0.0))
    return target;
  const bool expand = target > current *
      (1.0 + parameters.expand_hysteresis_fraction);
  const bool contract = target < current *
      (1.0 - parameters.contract_hysteresis_fraction);
  if(!expand && !contract)
    return current;
  const double alpha = 1.0 - std::exp(-dt / parameters.scale_response_seconds);
  double log_step = alpha * std::log(target / current);
  const double maximum_step = parameters.scale_max_log_rate_per_s * dt;
  log_step = stellarCameraClamp(log_step, -maximum_step, maximum_step);
  return current * std::exp(log_step);
}

inline bool stellarUpdateCameraState(
    StellarCameraState *state, const StellarCameraLandmark &landmark,
    const StellarCameraFilterParameters &parameters)
{
  if(!state || !stellarCameraValidFilter(parameters) ||
     !std::isfinite(landmark.time_seconds) ||
     !stellarCameraFiniteVector(landmark.center) ||
     !stellarCameraFiniteVector(landmark.axis) ||
     !(landmark.material_radius_cm > 0.0) ||
     !(landmark.disk_radius_cm > 0.0) || !(landmark.polar_extent_cm > 0.0))
    return false;
  double target_axis[3] = {landmark.axis[0], landmark.axis[1], landmark.axis[2]};
  if(!stellarCameraNormalize(target_axis))
    return false;
  const double material_target = 1.20 * landmark.material_radius_cm;
  const double disk_target = std::max(0.80 * material_target,
                                      1.30 * landmark.disk_radius_cm);
  const double outflow_target = std::max(1.15 * landmark.disk_radius_cm,
                                         0.58 * landmark.polar_extent_cm);
  if(!state->initialized) {
    state->initialized = 1;
    state->origin_time_seconds = landmark.time_seconds;
    state->time_seconds = landmark.time_seconds;
    for(int component = 0; component < 3; component++) {
      state->center[component] = landmark.center[component];
      state->center_velocity[component] = 0.0;
      state->axis[component] = target_axis[component];
    }
    if(!stellarCameraPerpendicular(state->axis, state->azimuth_zero))
      return false;
    state->material_half_extent_cm =
        std::max(material_target, parameters.minimum_half_extent_cm);
    state->disk_half_extent_cm =
        std::max(disk_target, parameters.minimum_half_extent_cm);
    state->outflow_half_extent_cm =
        std::max(outflow_target, parameters.minimum_half_extent_cm);
    return true;
  }

  const double dt = landmark.time_seconds - state->time_seconds;
  if(!(dt > 0.0) || !std::isfinite(dt))
    return false;
  double desired_velocity[3];
  for(int component = 0; component < 3; component++)
    desired_velocity[component] =
        (landmark.center[component] - state->center[component]) /
        parameters.center_response_seconds;
  stellarCameraLimitMagnitude(desired_velocity,
                              parameters.center_max_speed_cm_per_s);
  double velocity_change[3];
  for(int component = 0; component < 3; component++)
    velocity_change[component] =
        desired_velocity[component] - state->center_velocity[component];
  stellarCameraLimitMagnitude(
      velocity_change, parameters.center_max_acceleration_cm_per_s2 * dt);
  for(int component = 0; component < 3; component++) {
    state->center_velocity[component] += velocity_change[component];
    state->center[component] += state->center_velocity[component] * dt;
  }

  if(stellarCameraDot(state->axis, target_axis) < 0.0)
    for(int component = 0; component < 3; component++)
      target_axis[component] = -target_axis[component];
  const double cosine = stellarCameraClamp(
      stellarCameraDot(state->axis, target_axis), -1.0, 1.0);
  const double angle = std::acos(cosine);
  double previous_azimuth_zero[3] = {state->azimuth_zero[0],
                                     state->azimuth_zero[1],
                                     state->azimuth_zero[2]};
  if(angle > 0.0) {
    const double response_fraction =
        1.0 - std::exp(-dt / parameters.axis_response_seconds);
    const double rate_fraction = std::min(
        1.0, parameters.axis_max_rate_rad_per_s * dt / angle);
    const double fraction = std::min(response_fraction, rate_fraction);
    for(int component = 0; component < 3; component++)
      state->axis[component] = (1.0 - fraction) * state->axis[component] +
          fraction * target_axis[component];
    if(!stellarCameraNormalize(state->axis))
      return false;
  }

  // Parallel transport the roll reference onto the new axis. This avoids the
  // discontinuity caused by rebuilding a basis from a changing world axis.
  const double projection = stellarCameraDot(previous_azimuth_zero, state->axis);
  for(int component = 0; component < 3; component++)
    state->azimuth_zero[component] = previous_azimuth_zero[component] -
        projection * state->axis[component];
  if(!stellarCameraNormalize(state->azimuth_zero) &&
     !stellarCameraPerpendicular(state->axis, state->azimuth_zero))
    return false;

  state->material_half_extent_cm = stellarCameraFilteredExtent(
      state->material_half_extent_cm, material_target, dt, parameters);
  state->disk_half_extent_cm = stellarCameraFilteredExtent(
      state->disk_half_extent_cm, disk_target, dt, parameters);
  state->outflow_half_extent_cm = stellarCameraFilteredExtent(
      state->outflow_half_extent_cm, outflow_target, dt, parameters);
  state->time_seconds = landmark.time_seconds;
  return true;
}

inline bool stellarCameraBasis(const StellarCameraState &state, double first[3],
                               double second[3])
{
  for(int component = 0; component < 3; component++)
    first[component] = state.azimuth_zero[component];
  if(!stellarCameraNormalize(first))
    return false;
  stellarCameraCross(state.axis, first, second);
  return stellarCameraNormalize(second);
}

inline bool stellarCameraPoseForShot(const StellarCameraState &state,
                                     const StellarCameraShot &shot,
                                     StellarCameraPose *pose)
{
  if(!state.initialized || !pose || !(shot.framing_margin > 0.0) ||
     !std::isfinite(shot.azimuth_radians) ||
     !std::isfinite(shot.elevation_radians) ||
     !std::isfinite(shot.orbit_rate_radians_per_second) ||
     !std::isfinite(shot.lobe_sign))
    return false;
  double first[3], second[3];
  if(!stellarCameraBasis(state, first, second))
    return false;
  const double phase = shot.azimuth_radians +
      shot.orbit_rate_radians_per_second *
          (state.time_seconds - state.origin_time_seconds);
  double radial[3];
  for(int component = 0; component < 3; component++)
    radial[component] = std::cos(phase) * first[component] +
        std::sin(phase) * second[component];

  double elevation = shot.elevation_radians;
  double half_extent = state.disk_half_extent_cm;
  for(int component = 0; component < 3; component++)
    pose->look_at[component] = state.center[component];
  if(shot.mode == STELLAR_CAMERA_DISK_EDGE)
    elevation = 0.0;
  else if(shot.mode == STELLAR_CAMERA_DISK_OBLIQUE ||
          shot.mode == STELLAR_CAMERA_ORBIT) {
    if(elevation == 0.0)
      elevation = 0.45;
    if(shot.mode == STELLAR_CAMERA_ORBIT)
      half_extent = std::max(state.disk_half_extent_cm,
                             state.material_half_extent_cm);
  } else if(shot.mode == STELLAR_CAMERA_OUTFLOW_SIDE) {
    elevation = 0.12;
    half_extent = state.outflow_half_extent_cm;
  } else if(shot.mode == STELLAR_CAMERA_OUTFLOW_AXIS) {
    elevation = 1.43;
    half_extent = std::max(state.disk_half_extent_cm,
                           0.35 * state.outflow_half_extent_cm);
  } else if(shot.mode == STELLAR_CAMERA_OUTFLOW_FOLLOW) {
    elevation = 0.10;
    half_extent = std::max(state.disk_half_extent_cm,
                           0.48 * state.outflow_half_extent_cm);
    const double sign = shot.lobe_sign < 0.0 ? -1.0 : 1.0;
    for(int component = 0; component < 3; component++)
      pose->look_at[component] += sign * 0.72 *
          state.outflow_half_extent_cm * state.axis[component];
  } else if(shot.mode != STELLAR_CAMERA_DISK_EDGE) {
    return false;
  }

  double view_offset[3];
  for(int component = 0; component < 3; component++)
    view_offset[component] = std::cos(elevation) * radial[component] +
        std::sin(elevation) * state.axis[component];
  if(!stellarCameraNormalize(view_offset))
    return false;
  pose->screen_half_extent_cm = half_extent * shot.framing_margin;
  const double camera_distance = 4.0 * pose->screen_half_extent_cm;
  for(int component = 0; component < 3; component++)
    pose->position[component] = pose->look_at[component] +
        camera_distance * view_offset[component];

  const double axis_projection = stellarCameraDot(state.axis, view_offset);
  for(int component = 0; component < 3; component++)
    pose->up[component] = state.axis[component] -
        axis_projection * view_offset[component];
  if(!stellarCameraNormalize(pose->up)) {
    for(int component = 0; component < 3; component++)
      pose->up[component] = second[component];
  }
  return true;
}

#endif
