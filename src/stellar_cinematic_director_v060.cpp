#include "stellar_cinematic_director_v060.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>

namespace {

const double kPi = 3.141592653589793238462643383279502884;

bool fail(std::string *error, const std::string &message)
{
  if(error)
    *error = message;
  return false;
}

bool parseUnsigned(const std::string &text, unsigned long long *value)
{
  if(text.empty())
    return false;
  for(std::size_t index = 0; index < text.size(); ++index)
    if(text[index] < '0' || text[index] > '9')
      return false;
  errno = 0;
  char *end = 0;
  const unsigned long long parsed = std::strtoull(text.c_str(), &end, 10);
  if(errno == ERANGE || !end || *end != '\0')
    return false;
  *value = parsed;
  return true;
}

bool parseFinite(const std::string &text, double *value)
{
  errno = 0;
  char *end = 0;
  const double parsed = std::strtod(text.c_str(), &end);
  if(errno == ERANGE || end == text.c_str() || !end || *end != '\0' ||
     !std::isfinite(parsed))
    return false;
  *value = parsed;
  return true;
}

bool safeToken(const std::string &text)
{
  if(text.empty())
    return false;
  for(std::size_t index = 0; index < text.size(); ++index) {
    const char value = text[index];
    if(!((value >= 'a' && value <= 'z') ||
         (value >= 'A' && value <= 'Z') ||
         (value >= '0' && value <= '9') || value == '_' || value == '-'))
      return false;
  }
  return true;
}

std::vector<std::string> tokens(const std::string &line)
{
  std::istringstream parser(line);
  std::vector<std::string> result;
  std::string value;
  while(parser >> value)
    result.push_back(value);
  return result;
}

int parseMode(const std::string &name)
{
  if(name == "disk_edge")
    return STELLAR_CAMERA_DISK_EDGE;
  if(name == "disk_oblique")
    return STELLAR_CAMERA_DISK_OBLIQUE;
  if(name == "outflow_side")
    return STELLAR_CAMERA_OUTFLOW_SIDE;
  if(name == "outflow_axis")
    return STELLAR_CAMERA_OUTFLOW_AXIS;
  if(name == "outflow_follow")
    return STELLAR_CAMERA_OUTFLOW_FOLLOW;
  if(name == "orbit")
    return STELLAR_CAMERA_ORBIT;
  return -1;
}

int parseEasing(const std::string &name)
{
  if(name == "linear")
    return STELLAR_DIRECTOR_LINEAR;
  if(name == "smoothstep")
    return STELLAR_DIRECTOR_SMOOTHSTEP;
  if(name == "smootherstep")
    return STELLAR_DIRECTOR_SMOOTHERSTEP;
  return -1;
}

int parseExtentSource(const std::string &name)
{
  if(name == "filtered")
    return STELLAR_DIRECTOR_FILTERED_EXTENTS;
  if(name == "raw")
    return STELLAR_DIRECTOR_RAW_EXTENTS;
  return -1;
}

double clamp01(double value)
{
  return std::max(0.0, std::min(1.0, value));
}

double ease(double value, int easing)
{
  value = clamp01(value);
  if(easing == STELLAR_DIRECTOR_SMOOTHSTEP)
    return value * value * (3.0 - 2.0 * value);
  if(easing == STELLAR_DIRECTOR_SMOOTHERSTEP)
    return value * value * value *
        (value * (value * 6.0 - 15.0) + 10.0);
  return value;
}

bool withinBudget(double value, double limit)
{
  const double tolerance = 1.0e-10 * std::max(1.0, std::abs(limit));
  return value <= limit + tolerance;
}

bool poseForShot(const StellarCameraState &state,
                 const StellarDirectorShotV060 &shot,
                 double raw_material_half_extent_cm,
                 double raw_disk_half_extent_cm,
                 double raw_outflow_half_extent_cm,
                 double minimum_half_extent_cm,
                 double shot_start_time, double shot_end_time,
                 StellarCameraPose *pose)
{
  if(!pose || !(shot.framing_margin > 0.0))
    return false;
  double first[3], second[3];
  if(!stellarCameraBasis(state, first, second))
    return false;
  const double duration = shot_end_time - shot_start_time;
  const double progress = duration > 0.0 ?
      clamp01((state.time_seconds - shot_start_time) / duration) : 0.0;
  double phase = shot.azimuth_radians;
  if(shot.has_orbit_degrees)
    phase += shot.orbit_radians * progress;
  else if(shot.has_orbit_period)
    phase += 2.0 * kPi * std::max(0.0,
        state.time_seconds - shot_start_time) /
        shot.orbit_period_seconds;

  double radial[3];
  for(int component = 0; component < 3; ++component)
    radial[component] = std::cos(phase) * first[component] +
        std::sin(phase) * second[component];

  const bool use_raw = shot.extent_source == STELLAR_DIRECTOR_RAW_EXTENTS;
  const double material_half_extent = use_raw ?
      std::max(minimum_half_extent_cm, raw_material_half_extent_cm) :
      state.material_half_extent_cm;
  const double disk_half_extent = use_raw ?
      std::max(minimum_half_extent_cm, raw_disk_half_extent_cm) :
      state.disk_half_extent_cm;
  const double outflow_half_extent = use_raw ?
      std::max(minimum_half_extent_cm, raw_outflow_half_extent_cm) :
      state.outflow_half_extent_cm;
  double half_extent = disk_half_extent;
  if(shot.mode == STELLAR_CAMERA_ORBIT)
    half_extent = std::max(disk_half_extent, material_half_extent);
  else if(shot.mode == STELLAR_CAMERA_OUTFLOW_SIDE)
    half_extent = outflow_half_extent;
  else if(shot.mode == STELLAR_CAMERA_OUTFLOW_AXIS)
    half_extent = std::max(disk_half_extent, 0.35 * outflow_half_extent);
  else if(shot.mode == STELLAR_CAMERA_OUTFLOW_FOLLOW)
    half_extent = std::max(disk_half_extent, 0.48 * outflow_half_extent);
  else if(shot.mode != STELLAR_CAMERA_DISK_EDGE &&
          shot.mode != STELLAR_CAMERA_DISK_OBLIQUE)
    return false;

  for(int component = 0; component < 3; ++component)
    pose->look_at[component] = state.center[component];
  if(shot.mode == STELLAR_CAMERA_OUTFLOW_FOLLOW)
    for(int component = 0; component < 3; ++component)
      pose->look_at[component] += shot.lobe_sign * 0.72 *
          outflow_half_extent * state.axis[component];

  double view_offset[3];
  for(int component = 0; component < 3; ++component)
    view_offset[component] = std::cos(shot.elevation_radians) *
        radial[component] + std::sin(shot.elevation_radians) *
        state.axis[component];
  if(!stellarCameraNormalize(view_offset))
    return false;
  pose->screen_half_extent_cm = half_extent * shot.framing_margin;
  const double distance = 4.0 * pose->screen_half_extent_cm;
  for(int component = 0; component < 3; ++component)
    pose->position[component] = pose->look_at[component] +
        distance * view_offset[component];

  const double axis_projection = stellarCameraDot(state.axis, view_offset);
  for(int component = 0; component < 3; ++component)
    pose->up[component] = state.axis[component] -
        axis_projection * view_offset[component];
  if(!stellarCameraNormalize(pose->up))
    for(int component = 0; component < 3; ++component)
      pose->up[component] = second[component];
  return true;
}

void rawPhysicalExtents(const StellarCameraLandmark &landmark,
                        double *material_half_extent_cm,
                        double *disk_half_extent_cm,
                        double *outflow_half_extent_cm)
{
  const double material = 1.20 * landmark.material_radius_cm;
  const double disk = std::max(0.80 * material,
                               1.30 * landmark.disk_radius_cm);
  const double outflow = std::max(1.15 * landmark.disk_radius_cm,
                                  0.58 * landmark.polar_extent_cm);
  *material_half_extent_cm = material;
  *disk_half_extent_cm = disk;
  *outflow_half_extent_cm = outflow;
}

bool blendPoses(const StellarCameraPose &left, const StellarCameraPose &right,
                double fraction, StellarCameraPose *pose)
{
  fraction = clamp01(fraction);
  for(int component = 0; component < 3; ++component) {
    pose->position[component] = (1.0 - fraction) * left.position[component] +
        fraction * right.position[component];
    pose->look_at[component] = (1.0 - fraction) * left.look_at[component] +
        fraction * right.look_at[component];
    pose->up[component] = (1.0 - fraction) * left.up[component] +
        fraction * right.up[component];
  }
  pose->screen_half_extent_cm = std::exp(
      (1.0 - fraction) * std::log(left.screen_half_extent_cm) +
      fraction * std::log(right.screen_half_extent_cm));
  double view[3];
  for(int component = 0; component < 3; ++component)
    view[component] = pose->position[component] - pose->look_at[component];
  if(!stellarCameraNormalize(view))
    return false;
  const double projection = stellarCameraDot(pose->up, view);
  for(int component = 0; component < 3; ++component)
    pose->up[component] -= projection * view[component];
  return stellarCameraNormalize(pose->up);
}

std::size_t findLandmark(
    const std::vector<StellarDirectorLandmarkRowV060> &landmarks,
    unsigned long long snapshot)
{
  std::size_t low = 0;
  std::size_t high = landmarks.size();
  while(low < high) {
    const std::size_t middle = low + (high - low) / 2;
    if(landmarks[middle].snapshot < snapshot)
      low = middle + 1;
    else
      high = middle;
  }
  return low < landmarks.size() && landmarks[low].snapshot == snapshot ?
      low : landmarks.size();
}

std::string xmlEscape(const std::string &text)
{
  std::string result;
  for(std::size_t index = 0; index < text.size(); ++index) {
    if(text[index] == '&') result += "&amp;";
    else if(text[index] == '<') result += "&lt;";
    else if(text[index] == '>') result += "&gt;";
    else if(text[index] == '\"') result += "&quot;";
    else result += text[index];
  }
  return result;
}

} // namespace

const char *stellarDirectorModeNameV060(int mode)
{
  if(mode == STELLAR_CAMERA_DISK_EDGE) return "disk_edge";
  if(mode == STELLAR_CAMERA_DISK_OBLIQUE) return "disk_oblique";
  if(mode == STELLAR_CAMERA_OUTFLOW_SIDE) return "outflow_side";
  if(mode == STELLAR_CAMERA_OUTFLOW_AXIS) return "outflow_axis";
  if(mode == STELLAR_CAMERA_OUTFLOW_FOLLOW) return "outflow_follow";
  if(mode == STELLAR_CAMERA_ORBIT) return "orbit";
  return "unknown";
}

const char *stellarDirectorEasingNameV060(int easing)
{
  if(easing == STELLAR_DIRECTOR_LINEAR) return "linear";
  if(easing == STELLAR_DIRECTOR_SMOOTHSTEP) return "smoothstep";
  if(easing == STELLAR_DIRECTOR_SMOOTHERSTEP) return "smootherstep";
  return "unknown";
}

const char *stellarDirectorExtentSourceNameV060(int source)
{
  if(source == STELLAR_DIRECTOR_FILTERED_EXTENTS) return "filtered";
  if(source == STELLAR_DIRECTOR_RAW_EXTENTS) return "raw";
  return "unknown";
}

bool stellarDirectorLoadLandmarksV060(
    std::istream &input, std::vector<StellarDirectorLandmarkRowV060> *rows,
    std::string *error)
{
  if(!rows)
    return fail(error, "Landmark output is null.");
  rows->clear();
  std::string line;
  std::size_t line_number = 0;
  while(std::getline(input, line)) {
    ++line_number;
    const std::string::size_type start = line.find_first_not_of(" \t\r\n");
    if(start == std::string::npos || line[start] == '#')
      continue;
    const std::vector<std::string> value = tokens(line.substr(start));
    if(value.size() != 11)
      return fail(error, "Landmark row must have 11 columns at line " +
                  std::to_string(line_number));
    StellarDirectorLandmarkRowV060 row = {};
    if(!parseUnsigned(value[0], &row.snapshot) ||
       !parseFinite(value[1], &row.landmark.time_seconds) ||
       !parseFinite(value[2], &row.landmark.center[0]) ||
       !parseFinite(value[3], &row.landmark.center[1]) ||
       !parseFinite(value[4], &row.landmark.center[2]) ||
       !parseFinite(value[5], &row.landmark.axis[0]) ||
       !parseFinite(value[6], &row.landmark.axis[1]) ||
       !parseFinite(value[7], &row.landmark.axis[2]) ||
       !parseFinite(value[8], &row.landmark.material_radius_cm) ||
       !parseFinite(value[9], &row.landmark.disk_radius_cm) ||
       !parseFinite(value[10], &row.landmark.polar_extent_cm))
      return fail(error, "Invalid landmark value at line " +
                  std::to_string(line_number));
    if(!rows->empty() &&
       (row.snapshot <= rows->back().snapshot ||
        row.landmark.time_seconds <= rows->back().landmark.time_seconds))
      return fail(error, "Landmark snapshots and times must increase at line " +
                  std::to_string(line_number));
    rows->push_back(row);
  }
  if(rows->empty())
    return fail(error, "Landmark table is empty.");
  if(error)
    error->clear();
  return true;
}

bool stellarDirectorLoadShotsV060(
    std::istream &input, std::vector<StellarDirectorShotV060> *shots,
    std::string *error)
{
  if(!shots)
    return fail(error, "Shot output is null.");
  shots->clear();
  bool schema_seen = false;
  std::string line;
  std::size_t line_number = 0;
  while(std::getline(input, line)) {
    ++line_number;
    const std::string::size_type start = line.find_first_not_of(" \t\r\n");
    if(start == std::string::npos)
      continue;
    if(line[start] == '#') {
      if(line.find("schema=stellar_cinematic_shots_v060", start) !=
         std::string::npos)
        schema_seen = true;
      continue;
    }
    const std::vector<std::string> value = tokens(line.substr(start));
    if(value.size() != 14)
      return fail(error, "Shot row must have 14 columns at line " +
                  std::to_string(line_number));
    StellarDirectorShotV060 shot = {};
    shot.name = value[0];
    shot.subject = value[4];
    shot.mode = parseMode(value[3]);
    shot.extent_source = parseExtentSource(value[11]);
    shot.easing = parseEasing(value[13]);
    double azimuth_degrees = 0.0;
    double elevation_degrees = 0.0;
    double orbit_degrees = 0.0;
    double lobe_sign = 0.0;
    if(!safeToken(shot.name) || !safeToken(shot.subject) || shot.mode < 0 ||
       shot.extent_source < 0 || shot.easing < 0 ||
       !parseUnsigned(value[1], &shot.start_snapshot) ||
       !parseUnsigned(value[2], &shot.end_snapshot) ||
       !parseFinite(value[5], &azimuth_degrees) ||
       !parseFinite(value[6], &elevation_degrees) ||
       !parseFinite(value[7], &shot.framing_margin) ||
       !parseFinite(value[10], &lobe_sign))
      return fail(error, "Invalid shot value at line " +
                  std::to_string(line_number));
    shot.has_orbit_degrees = value[8] != "-";
    shot.has_orbit_period = value[9] != "-";
    shot.has_transition_in = value[12] != "-";
    if((shot.has_orbit_degrees && !parseFinite(value[8], &orbit_degrees)) ||
       (shot.has_orbit_period &&
        !parseFinite(value[9], &shot.orbit_period_seconds)) ||
       (shot.has_transition_in &&
        !parseUnsigned(value[12], &shot.transition_end_snapshot)))
      return fail(error, "Invalid orbit or transition control at line " +
                  std::to_string(line_number));
    if(shot.has_orbit_degrees && shot.has_orbit_period)
      return fail(error, "Use orbit degrees or period, not both, at line " +
                  std::to_string(line_number));
    if(shot.start_snapshot > shot.end_snapshot ||
       !(shot.framing_margin > 0.0) ||
       elevation_degrees <= -89.5 || elevation_degrees >= 89.5 ||
       (shot.has_orbit_period && !(shot.orbit_period_seconds > 0.0)) ||
       std::abs(std::abs(lobe_sign) - 1.0) > 1.0e-12 ||
       (shot.has_transition_in &&
        (shot.transition_end_snapshot <= shot.start_snapshot ||
         shot.transition_end_snapshot > shot.end_snapshot)))
      return fail(error, "Shot bounds or controls are invalid at line " +
                  std::to_string(line_number));
    shot.azimuth_radians = azimuth_degrees * kPi / 180.0;
    shot.elevation_radians = elevation_degrees * kPi / 180.0;
    shot.orbit_radians = orbit_degrees * kPi / 180.0;
    shot.lobe_sign = lobe_sign;
    if(shots->empty() && shot.has_transition_in)
      return fail(error, "The first shot cannot have an incoming transition.");
    if(!shots->empty()) {
      const StellarDirectorShotV060 &previous = shots->back();
      if(previous.end_snapshot == std::numeric_limits<unsigned long long>::max() ||
         shot.start_snapshot != previous.end_snapshot + 1)
        return fail(error, "Shot ranges must be ordered and contiguous at line " +
                    std::to_string(line_number));
      for(std::size_t index = 0; index < shots->size(); ++index)
        if(shots->at(index).name == shot.name)
          return fail(error, "Shot names must be unique at line " +
                      std::to_string(line_number));
    }
    shots->push_back(shot);
  }
  if(!schema_seen)
    return fail(error, "Missing schema=stellar_cinematic_shots_v060.");
  if(shots->empty())
    return fail(error, "Shot table is empty.");
  if(error)
    error->clear();
  return true;
}

bool stellarDirectorBuildV060(
    const std::vector<StellarDirectorLandmarkRowV060> &landmarks,
    const std::vector<StellarDirectorShotV060> &shots,
    const StellarCameraFilterParameters &filter,
    const StellarDirectorMotionBudgetV060 &budget,
    StellarDirectorResultV060 *result, std::string *error)
{
  if(!result || landmarks.empty() || shots.empty() ||
     !stellarCameraValidFilter(filter) ||
     !stellarDirectorValidMotionBudgetV060(budget))
    return fail(error, "Director inputs are incomplete or invalid.");
  result->path.clear();
  result->diagnostics.clear();
  if(shots.front().start_snapshot != landmarks.front().snapshot ||
     shots.back().end_snapshot != landmarks.back().snapshot)
    return fail(error, "Shot coverage must match the landmark range exactly.");

  std::vector<std::size_t> shot_start(shots.size());
  std::vector<std::size_t> shot_end(shots.size());
  std::vector<std::size_t> transition_end(shots.size(), landmarks.size());
  for(std::size_t index = 0; index < shots.size(); ++index) {
    shot_start[index] = findLandmark(landmarks, shots[index].start_snapshot);
    shot_end[index] = findLandmark(landmarks, shots[index].end_snapshot);
    if(shot_start[index] == landmarks.size() ||
       shot_end[index] == landmarks.size())
      return fail(error, "Every shot boundary must have a landmark row.");
    if(shots[index].has_transition_in) {
      transition_end[index] = findLandmark(
          landmarks, shots[index].transition_end_snapshot);
      if(transition_end[index] == landmarks.size())
        return fail(error, "Every transition boundary must have a landmark row.");
    }
  }

  StellarCameraState state = {};
  std::size_t shot_index = 0;
  for(std::size_t index = 0; index < landmarks.size(); ++index) {
    while(shot_index + 1 < shots.size() &&
          landmarks[index].snapshot > shots[shot_index].end_snapshot)
      ++shot_index;
    if(landmarks[index].snapshot < shots[shot_index].start_snapshot ||
       landmarks[index].snapshot > shots[shot_index].end_snapshot)
      return fail(error, "A landmark is not covered by a shot.");
    if(!stellarUpdateCameraState(&state, landmarks[index].landmark, filter))
      return fail(error, "Physical camera filtering failed.");

    StellarDirectorPathRowV060 row = {};
    row.snapshot = landmarks[index].snapshot;
    row.time_seconds = landmarks[index].landmark.time_seconds;
    row.shot_name = shots[shot_index].name;
    row.subject = shots[shot_index].subject;
    row.transition_fraction = 0.0;
    rawPhysicalExtents(landmarks[index].landmark,
                       &row.raw_material_half_extent_cm,
                       &row.raw_disk_half_extent_cm,
                       &row.raw_outflow_half_extent_cm);
    const double start_time =
        landmarks[shot_start[shot_index]].landmark.time_seconds;
    const double end_time =
        landmarks[shot_end[shot_index]].landmark.time_seconds;
    if(!poseForShot(state, shots[shot_index],
                    row.raw_material_half_extent_cm,
                    row.raw_disk_half_extent_cm,
                    row.raw_outflow_half_extent_cm,
                    filter.minimum_half_extent_cm,
                    start_time, end_time, &row.pose))
      return fail(error, "Directed camera pose failed.");

    if(shot_index > 0 && shots[shot_index].has_transition_in &&
       index <= transition_end[shot_index]) {
      const double transition_end_time =
          landmarks[transition_end[shot_index]].landmark.time_seconds;
      const double raw_fraction = (row.time_seconds - start_time) /
          (transition_end_time - start_time);
      row.transition_fraction = ease(raw_fraction, shots[shot_index].easing);
      row.transition_from = shots[shot_index - 1].name;
      row.transition_to = shots[shot_index].name;
      StellarCameraPose previous_pose = {};
      const double previous_start =
          landmarks[shot_start[shot_index - 1]].landmark.time_seconds;
      const double previous_end =
          landmarks[shot_end[shot_index - 1]].landmark.time_seconds;
      if(!poseForShot(state, shots[shot_index - 1],
                      row.raw_material_half_extent_cm,
                      row.raw_disk_half_extent_cm,
                      row.raw_outflow_half_extent_cm,
                      filter.minimum_half_extent_cm,
                      previous_start, previous_end, &previous_pose) ||
         !blendPoses(previous_pose, row.pose, row.transition_fraction,
                     &row.pose))
        return fail(error, "Directed camera transition failed.");
    }
    row.target_screen_half_extent_cm = row.pose.screen_half_extent_cm;
    row.scale_limited = 0;
    if(!result->path.empty()) {
      const double previous_scale =
          result->path.back().pose.screen_half_extent_cm;
      const double desired_log_step = std::log(
          row.target_screen_half_extent_cm / previous_scale);
      const double maximum_log_step = std::log1p(
          budget.max_zoom_percent_per_frame / 100.0);
      const double limited_log_step = std::max(-maximum_log_step,
          std::min(maximum_log_step, desired_log_step));
      if(std::abs(limited_log_step - desired_log_step) > 1.0e-14) {
        double view[3];
        for(int component = 0; component < 3; ++component)
          view[component] = row.pose.position[component] -
              row.pose.look_at[component];
        if(!stellarCameraNormalize(view))
          return fail(error, "Scale-limited camera view is degenerate.");
        row.pose.screen_half_extent_cm = previous_scale *
            std::exp(limited_log_step);
        const double distance = 4.0 * row.pose.screen_half_extent_cm;
        for(int component = 0; component < 3; ++component)
          row.pose.position[component] = row.pose.look_at[component] +
              distance * view[component];
        row.scale_limited = 1;
      }
    }
    for(int component = 0; component < 3; ++component) {
      row.center[component] = state.center[component];
      row.axis[component] = state.axis[component];
    }
    row.material_half_extent_cm = state.material_half_extent_cm;
    row.disk_half_extent_cm = state.disk_half_extent_cm;
    row.outflow_half_extent_cm = state.outflow_half_extent_cm;
    result->path.push_back(row);
  }

  double previous_center_velocity[3] = {};
  double previous_center_acceleration[3] = {};
  double previous_camera_velocity[3] = {};
  double previous_camera_acceleration[3] = {};
  double previous_zoom_rate = 0.0;
  for(std::size_t index = 0; index < result->path.size(); ++index) {
    const StellarDirectorPathRowV060 &row = result->path[index];
    StellarDirectorDiagnosticRowV060 diagnostic = {};
    diagnostic.snapshot = row.snapshot;
    diagnostic.time_seconds = row.time_seconds;
    diagnostic.shot_name = row.shot_name;
    diagnostic.transition_from = row.transition_from;
    diagnostic.transition_to = row.transition_to;
    diagnostic.transition_fraction = row.transition_fraction;
    diagnostic.target_scale_error_percent = 100.0 * std::abs(
        row.pose.screen_half_extent_cm - row.target_screen_half_extent_cm) /
        row.target_screen_half_extent_cm;
    diagnostic.scale_limited = row.scale_limited;
    if(index > 0) {
      const StellarDirectorPathRowV060 &previous = result->path[index - 1];
      const double dt = row.time_seconds - previous.time_seconds;
      double center_velocity[3], camera_velocity[3];
      for(int component = 0; component < 3; ++component) {
        center_velocity[component] =
            (row.center[component] - previous.center[component]) / dt;
        camera_velocity[component] =
            (row.pose.position[component] - previous.pose.position[component]) /
            dt;
      }
      diagnostic.center_speed_cm_per_s = stellarCameraNorm(center_velocity);
      diagnostic.camera_speed_cm_per_s = stellarCameraNorm(camera_velocity);
      double center_acceleration[3] = {};
      double camera_acceleration[3] = {};
      for(int component = 0; component < 3; ++component) {
        if(index > 1) {
          center_acceleration[component] =
              (center_velocity[component] -
               previous_center_velocity[component]) / dt;
          camera_acceleration[component] =
              (camera_velocity[component] -
               previous_camera_velocity[component]) / dt;
        }
      }
      diagnostic.center_acceleration_cm_per_s2 =
          stellarCameraNorm(center_acceleration);
      diagnostic.camera_acceleration_cm_per_s2 =
          stellarCameraNorm(camera_acceleration);
      double center_jerk[3] = {};
      double camera_jerk[3] = {};
      for(int component = 0; component < 3; ++component) {
        if(index > 2) {
          center_jerk[component] =
              (center_acceleration[component] -
               previous_center_acceleration[component]) / dt;
          camera_jerk[component] =
              (camera_acceleration[component] -
               previous_camera_acceleration[component]) / dt;
        }
        previous_center_velocity[component] = center_velocity[component];
        previous_center_acceleration[component] = center_acceleration[component];
        previous_camera_velocity[component] = camera_velocity[component];
        previous_camera_acceleration[component] = camera_acceleration[component];
      }
      diagnostic.center_jerk_cm_per_s3 = stellarCameraNorm(center_jerk);
      diagnostic.camera_jerk_cm_per_s3 = stellarCameraNorm(camera_jerk);
      diagnostic.axis_rate_rad_per_s = std::acos(stellarCameraClamp(
          stellarCameraDot(row.axis, previous.axis), -1.0, 1.0)) / dt;
      diagnostic.roll_rate_rad_per_s = std::acos(stellarCameraClamp(
          stellarCameraDot(row.pose.up, previous.pose.up), -1.0, 1.0)) / dt;
      diagnostic.roll_deg_per_frame =
          diagnostic.roll_rate_rad_per_s * dt * 180.0 / kPi;
      diagnostic.log_zoom_rate_per_s =
          std::log(row.pose.screen_half_extent_cm /
                   previous.pose.screen_half_extent_cm) / dt;
      diagnostic.zoom_percent_per_frame = 100.0 *
          (std::exp(std::abs(diagnostic.log_zoom_rate_per_s) * dt) - 1.0);
      if(index > 1)
        diagnostic.log_zoom_acceleration_per_s2 =
            (diagnostic.log_zoom_rate_per_s - previous_zoom_rate) / dt;
      previous_zoom_rate = diagnostic.log_zoom_rate_per_s;
    }
    diagnostic.material_occupancy = row.material_half_extent_cm /
        row.pose.screen_half_extent_cm;
    diagnostic.disk_occupancy = row.disk_half_extent_cm /
        row.pose.screen_half_extent_cm;
    diagnostic.outflow_occupancy = row.outflow_half_extent_cm /
        row.pose.screen_half_extent_cm;
    diagnostic.raw_material_occupancy = row.raw_material_half_extent_cm /
        row.pose.screen_half_extent_cm;
    diagnostic.raw_disk_occupancy = row.raw_disk_half_extent_cm /
        row.pose.screen_half_extent_cm;
    diagnostic.raw_outflow_occupancy = row.raw_outflow_half_extent_cm /
        row.pose.screen_half_extent_cm;
    diagnostic.material_clipped = diagnostic.material_occupancy > 0.98;
    diagnostic.disk_clipped = diagnostic.disk_occupancy > 0.98;
    diagnostic.outflow_clipped = diagnostic.outflow_occupancy > 0.98;
    diagnostic.raw_material_clipped =
        diagnostic.raw_material_occupancy > 0.98;
    diagnostic.raw_disk_clipped = diagnostic.raw_disk_occupancy > 0.98;
    diagnostic.raw_outflow_clipped = diagnostic.raw_outflow_occupancy > 0.98;
    result->diagnostics.push_back(diagnostic);
  }
  if(error)
    error->clear();
  return true;
}

bool stellarDirectorValidMotionBudgetV060(
    const StellarDirectorMotionBudgetV060 &budget)
{
  return std::isfinite(budget.max_roll_deg_per_frame) &&
      std::isfinite(budget.max_zoom_percent_per_frame) &&
      std::isfinite(budget.max_final_target_scale_error_percent) &&
      budget.max_roll_deg_per_frame > 0.0 &&
      budget.max_zoom_percent_per_frame > 0.0 &&
      budget.max_final_target_scale_error_percent >= 0.0;
}

StellarDirectorMotionAssessmentV060 stellarDirectorAssessMotionV060(
    const StellarDirectorResultV060 &result,
    const StellarDirectorMotionBudgetV060 &budget)
{
  StellarDirectorMotionAssessmentV060 assessment = {};
  assessment.passed = stellarDirectorValidMotionBudgetV060(budget);
  for(std::size_t index = 0; index < result.diagnostics.size(); ++index) {
    const StellarDirectorDiagnosticRowV060 &row = result.diagnostics[index];
    if(row.roll_deg_per_frame > assessment.maximum_roll_deg_per_frame) {
      assessment.maximum_roll_deg_per_frame = row.roll_deg_per_frame;
      assessment.maximum_roll_snapshot = row.snapshot;
    }
    if(row.zoom_percent_per_frame > assessment.maximum_zoom_percent_per_frame) {
      assessment.maximum_zoom_percent_per_frame = row.zoom_percent_per_frame;
      assessment.maximum_zoom_snapshot = row.snapshot;
    }
    if(row.target_scale_error_percent >
       assessment.maximum_target_scale_error_percent) {
      assessment.maximum_target_scale_error_percent =
          row.target_scale_error_percent;
      assessment.maximum_target_scale_error_snapshot = row.snapshot;
    }
    if(row.scale_limited)
      ++assessment.scale_limited_rows;
  }
  if(!result.diagnostics.empty())
    assessment.final_target_scale_error_percent =
        result.diagnostics.back().target_scale_error_percent;
  assessment.passed = assessment.passed &&
      withinBudget(assessment.maximum_roll_deg_per_frame,
                   budget.max_roll_deg_per_frame) &&
      withinBudget(assessment.maximum_zoom_percent_per_frame,
                   budget.max_zoom_percent_per_frame) &&
      withinBudget(assessment.final_target_scale_error_percent,
                   budget.max_final_target_scale_error_percent);
  return assessment;
}

std::string stellarDirectorPathTableV060(
    const StellarDirectorResultV060 &result,
    const std::string &landmarks_path, const std::string &shots_path)
{
  std::ostringstream output;
  output << "# schema=stellar_camera_path_v055\n"
         << "# director_schema=stellar_cinematic_director_v060\n"
         << "# landmarks=" << landmarks_path << "\n"
         << "# shots=" << shots_path << "\n"
         << "# snapshot time_seconds camera_x camera_y camera_z look_x look_y "
            "look_z up_x up_y up_z half_extent center_x center_y center_z "
            "axis_x axis_y axis_z material_extent disk_extent outflow_extent\n";
  output << std::setprecision(17);
  for(std::size_t index = 0; index < result.path.size(); ++index) {
    const StellarDirectorPathRowV060 &row = result.path[index];
    output << row.snapshot << " " << row.time_seconds;
    for(int component = 0; component < 3; ++component)
      output << " " << row.pose.position[component];
    for(int component = 0; component < 3; ++component)
      output << " " << row.pose.look_at[component];
    for(int component = 0; component < 3; ++component)
      output << " " << row.pose.up[component];
    output << " " << row.pose.screen_half_extent_cm;
    for(int component = 0; component < 3; ++component)
      output << " " << row.center[component];
    for(int component = 0; component < 3; ++component)
      output << " " << row.axis[component];
    output << " " << row.material_half_extent_cm
           << " " << row.disk_half_extent_cm
           << " " << row.outflow_half_extent_cm << "\n";
  }
  return output.str();
}

std::string stellarDirectorDiagnosticsTableV060(
    const StellarDirectorResultV060 &result)
{
  std::ostringstream output;
  output << "# schema=stellar_camera_diagnostics_v060\n"
         << "# snapshot time shot transition_from transition_to "
            "transition_fraction "
            "center_speed center_acceleration center_jerk camera_speed "
            "camera_acceleration camera_jerk axis_rate roll_rate "
            "roll_deg_per_frame log_zoom_rate zoom_percent_per_frame "
            "log_zoom_acceleration target_scale_error_percent scale_limited "
            "material_occupancy disk_occupancy outflow_occupancy "
            "raw_material_occupancy raw_disk_occupancy raw_outflow_occupancy "
            "material_clipped disk_clipped outflow_clipped "
            "raw_material_clipped raw_disk_clipped raw_outflow_clipped\n";
  output << std::setprecision(17);
  for(std::size_t index = 0; index < result.diagnostics.size(); ++index) {
    const StellarDirectorDiagnosticRowV060 &row = result.diagnostics[index];
    output << row.snapshot << " " << row.time_seconds << " " << row.shot_name
           << " " << (row.transition_from.empty() ? "-" : row.transition_from)
           << " " << (row.transition_to.empty() ? "-" : row.transition_to)
           << " " << row.transition_fraction
           << " " << row.center_speed_cm_per_s
           << " " << row.center_acceleration_cm_per_s2
           << " " << row.center_jerk_cm_per_s3
           << " " << row.camera_speed_cm_per_s
           << " " << row.camera_acceleration_cm_per_s2
           << " " << row.camera_jerk_cm_per_s3
           << " " << row.axis_rate_rad_per_s
           << " " << row.roll_rate_rad_per_s
           << " " << row.roll_deg_per_frame
           << " " << row.log_zoom_rate_per_s
           << " " << row.zoom_percent_per_frame
           << " " << row.log_zoom_acceleration_per_s2
           << " " << row.target_scale_error_percent
           << " " << row.scale_limited
           << " " << row.material_occupancy
           << " " << row.disk_occupancy
           << " " << row.outflow_occupancy
           << " " << row.raw_material_occupancy
           << " " << row.raw_disk_occupancy
           << " " << row.raw_outflow_occupancy
           << " " << row.material_clipped
           << " " << row.disk_clipped
           << " " << row.outflow_clipped
           << " " << row.raw_material_clipped
           << " " << row.raw_disk_clipped
           << " " << row.raw_outflow_clipped << "\n";
  }
  return output.str();
}

std::string stellarDirectorManifestV060(
    const StellarDirectorResultV060 &result,
    const std::vector<StellarDirectorShotV060> &shots,
    const StellarCameraFilterParameters &filter,
    const StellarDirectorMotionBudgetV060 &budget,
    const StellarDirectorMotionAssessmentV060 &assessment,
    const std::string &landmarks_path, const std::string &shots_path,
    bool dry_run)
{
  std::ostringstream output;
  output << std::setprecision(17)
         << "schema=stellar_cinematic_direction_manifest_v060\n"
         << "landmarks_path=" << landmarks_path << "\n"
         << "shots_path=" << shots_path << "\n"
         << "landmark_rows=" << result.path.size() << "\n"
         << "shot_count=" << shots.size() << "\n"
         << "dry_run=" << (dry_run ? "true" : "false") << "\n"
         << "center_response_seconds=" << filter.center_response_seconds << "\n"
         << "center_max_speed_cm_per_s=" << filter.center_max_speed_cm_per_s << "\n"
         << "center_max_acceleration_cm_per_s2="
         << filter.center_max_acceleration_cm_per_s2 << "\n"
         << "axis_response_seconds=" << filter.axis_response_seconds << "\n"
         << "axis_max_rate_rad_per_s=" << filter.axis_max_rate_rad_per_s << "\n"
         << "scale_response_seconds=" << filter.scale_response_seconds << "\n"
         << "scale_max_log_rate_per_s=" << filter.scale_max_log_rate_per_s << "\n"
         << "expand_hysteresis_fraction="
         << filter.expand_hysteresis_fraction << "\n"
         << "contract_hysteresis_fraction="
         << filter.contract_hysteresis_fraction << "\n"
         << "minimum_half_extent_cm=" << filter.minimum_half_extent_cm << "\n"
         << "framing.extent_selection=per_shot_raw_or_filtered\n"
         << "framing.raw_material_formula=1.20*material_radius\n"
         << "framing.raw_disk_formula=max(0.80*raw_material,1.30*disk_radius)\n"
         << "framing.raw_outflow_formula=max(1.15*disk_radius,0.58*polar_extent)\n"
         << "framing.scale_limiter=path_log_step\n";
  output << "motion_budget.max_roll_deg_per_frame="
         << budget.max_roll_deg_per_frame << "\n"
         << "motion_budget.max_zoom_percent_per_frame="
         << budget.max_zoom_percent_per_frame << "\n"
         << "motion_budget.max_final_target_scale_error_percent="
         << budget.max_final_target_scale_error_percent << "\n"
         << "motion.maximum_roll_deg_per_frame="
         << assessment.maximum_roll_deg_per_frame << "\n"
         << "motion.maximum_roll_snapshot="
         << assessment.maximum_roll_snapshot << "\n"
         << "motion.maximum_zoom_percent_per_frame="
         << assessment.maximum_zoom_percent_per_frame << "\n"
         << "motion.maximum_zoom_snapshot="
         << assessment.maximum_zoom_snapshot << "\n"
         << "motion.maximum_target_scale_error_percent="
         << assessment.maximum_target_scale_error_percent << "\n"
         << "motion.maximum_target_scale_error_snapshot="
         << assessment.maximum_target_scale_error_snapshot << "\n"
         << "motion.final_target_scale_error_percent="
         << assessment.final_target_scale_error_percent << "\n"
         << "motion.scale_limited_rows="
         << assessment.scale_limited_rows << "\n"
         << "motion_budget.passed="
         << (assessment.passed ? "true" : "false") << "\n";
  for(std::size_t index = 0; index < shots.size(); ++index) {
    const StellarDirectorShotV060 &shot = shots[index];
    output << "shot." << index << ".name=" << shot.name << "\n"
           << "shot." << index << ".subject=" << shot.subject << "\n"
           << "shot." << index << ".range=" << shot.start_snapshot << ":"
           << shot.end_snapshot << "\n"
           << "shot." << index << ".mode="
           << stellarDirectorModeNameV060(shot.mode) << "\n"
           << "shot." << index << ".azimuth_radians="
           << shot.azimuth_radians << "\n"
           << "shot." << index << ".elevation_radians="
           << shot.elevation_radians << "\n"
           << "shot." << index << ".framing_margin="
           << shot.framing_margin << "\n"
           << "shot." << index << ".orbit_radians="
           << (shot.has_orbit_degrees ? shot.orbit_radians : 0.0) << "\n"
           << "shot." << index << ".orbit_period_seconds="
           << (shot.has_orbit_period ? shot.orbit_period_seconds : 0.0) << "\n"
           << "shot." << index << ".lobe_sign=" << shot.lobe_sign << "\n"
           << "shot." << index << ".extent_source="
           << stellarDirectorExtentSourceNameV060(shot.extent_source) << "\n"
           << "shot." << index << ".transition_in="
           << (shot.has_transition_in ? "true" : "false") << "\n"
           << "shot." << index << ".transition_end_snapshot="
           << (shot.has_transition_in ? shot.transition_end_snapshot : 0)
           << "\n"
           << "shot." << index << ".easing="
           << stellarDirectorEasingNameV060(shot.easing) << "\n";
  }
  return output.str();
}

std::string stellarDirectorPreviewSvgV060(
    const StellarDirectorResultV060 &result)
{
  const int width = 1200;
  const int height = 680;
  const int left = 90;
  const int right = 30;
  const int plot_width = width - left - right;
  std::ostringstream svg;
  svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width
      << "\" height=\"" << height << "\" viewBox=\"0 0 " << width << " "
      << height << "\">\n"
      << "<rect width=\"100%\" height=\"100%\" fill=\"#f7f8fa\"/>\n"
      << "<style>text{font-family:Arial,sans-serif;letter-spacing:0}"
         ".axis{stroke:#60646c;stroke-width:1}.grid{stroke:#d7d9de;stroke-width:1}"
         ".label{fill:#24262b;font-size:13px}.title{fill:#111318;font-size:18px;font-weight:700}"
         ".small{fill:#4d515a;font-size:11px}</style>\n"
      << "<text x=\"" << left << "\" y=\"30\" class=\"title\">"
         "Stellar cinematic camera dry-run</text>\n";
  if(result.path.empty()) {
    svg << "<text x=\"90\" y=\"80\" class=\"label\">No path rows</text></svg>\n";
    return svg.str();
  }
  const double first_snapshot = static_cast<double>(result.path.front().snapshot);
  const double span = std::max(1.0,
      static_cast<double>(result.path.back().snapshot) - first_snapshot);
  const char *colors[] = {"#2f6f9f", "#b44d3a", "#5d7f45", "#8b5e9b",
                          "#d08a2e", "#467c79"};
  std::string previous_shot;
  std::size_t band_start = 0;
  int band_index = 0;
  for(std::size_t index = 0; index <= result.path.size(); ++index) {
    const std::string current = index < result.path.size() ?
        result.path[index].shot_name : std::string();
    if(index == 0) previous_shot = current;
    if(index == result.path.size() || current != previous_shot) {
      const double x0 = left + plot_width *
          (static_cast<double>(result.path[band_start].snapshot) - first_snapshot) /
          span;
      const double x1 = left + plot_width *
          (static_cast<double>(result.path[index - 1].snapshot) - first_snapshot) /
          span;
      svg << "<rect x=\"" << x0 << "\" y=\"55\" width=\""
          << std::max(2.0, x1 - x0) << "\" height=\"38\" fill=\""
          << colors[band_index % 6] << "\" opacity=\"0.82\"/>\n"
          << "<text x=\"" << x0 + 5 << "\" y=\"79\" fill=\"white\" "
             "font-size=\"12px\">" << xmlEscape(previous_shot) << "</text>\n";
      ++band_index;
      band_start = index;
      previous_shot = current;
    }
  }

  const int panels_y[] = {145, 355};
  const char *panel_titles[] = {"Raw physical occupancy", "Motion rates (normalized)"};
  for(int panel = 0; panel < 2; ++panel) {
    const int top = panels_y[panel];
    const int panel_height = 160;
    svg << "<text x=\"" << left << "\" y=\"" << top - 12
        << "\" class=\"label\">" << panel_titles[panel] << "</text>\n"
        << "<line x1=\"" << left << "\" y1=\"" << top + panel_height
        << "\" x2=\"" << left + plot_width << "\" y2=\""
        << top + panel_height << "\" class=\"axis\"/>\n"
        << "<line x1=\"" << left << "\" y1=\"" << top
        << "\" x2=\"" << left << "\" y2=\"" << top + panel_height
        << "\" class=\"axis\"/>\n";
  }

  const char *occupancy_colors[] = {"#2f6f9f", "#b44d3a", "#5d7f45"};
  for(int series = 0; series < 3; ++series) {
    svg << "<polyline fill=\"none\" stroke=\"" << occupancy_colors[series]
        << "\" stroke-width=\"2\" points=\"";
    for(std::size_t index = 0; index < result.diagnostics.size(); ++index) {
      const StellarDirectorDiagnosticRowV060 &row = result.diagnostics[index];
      const double value = series == 0 ? row.raw_material_occupancy :
          (series == 1 ? row.raw_disk_occupancy : row.raw_outflow_occupancy);
      const double x = left + plot_width *
          (static_cast<double>(row.snapshot) - first_snapshot) / span;
      const double y = 305.0 - 150.0 * std::min(1.2, value) / 1.2;
      svg << x << "," << y << " ";
    }
    svg << "\"/>\n";
  }

  double maximum_motion = 0.0;
  for(std::size_t index = 0; index < result.diagnostics.size(); ++index) {
    const StellarDirectorDiagnosticRowV060 &row = result.diagnostics[index];
    maximum_motion = std::max(maximum_motion,
        std::max(row.axis_rate_rad_per_s, std::abs(row.log_zoom_rate_per_s)));
  }
  maximum_motion = std::max(maximum_motion, 1.0e-12);
  const char *motion_colors[] = {"#8b5e9b", "#d08a2e"};
  for(int series = 0; series < 2; ++series) {
    svg << "<polyline fill=\"none\" stroke=\"" << motion_colors[series]
        << "\" stroke-width=\"2\" points=\"";
    for(std::size_t index = 0; index < result.diagnostics.size(); ++index) {
      const StellarDirectorDiagnosticRowV060 &row = result.diagnostics[index];
      const double value = series == 0 ? row.axis_rate_rad_per_s :
          std::abs(row.log_zoom_rate_per_s);
      const double x = left + plot_width *
          (static_cast<double>(row.snapshot) - first_snapshot) / span;
      const double y = 515.0 - 150.0 * value / maximum_motion;
      svg << x << "," << y << " ";
    }
    svg << "\"/>\n";
  }
  svg << "<text x=\"90\" y=\"555\" class=\"small\">Blue raw material, red raw disk, "
         "green raw outflow occupancy; purple axis rate, amber zoom rate.</text>\n"
      << "<text x=\"90\" y=\"580\" class=\"small\">Values above occupancy 0.98 "
         "are flagged as geometrically clipped in the diagnostics table.</text>\n"
      << "<text x=\"90\" y=\"635\" class=\"small\">This dry-run uses physical "
         "landmarks only and performs no snapshot rendering.</text>\n"
      << "</svg>\n";
  return svg.str();
}
