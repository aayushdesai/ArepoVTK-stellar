#ifndef AREPO_VTK_STELLAR_CINEMATIC_DIRECTOR_V056_H
#define AREPO_VTK_STELLAR_CINEMATIC_DIRECTOR_V056_H

#include <cstddef>
#include <istream>
#include <string>
#include <vector>

#include "stellar_camera_v054.h"

enum StellarDirectorEasingV056 {
  STELLAR_DIRECTOR_LINEAR = 0,
  STELLAR_DIRECTOR_SMOOTHSTEP = 1,
  STELLAR_DIRECTOR_SMOOTHERSTEP = 2
};

struct StellarDirectorLandmarkRowV056 {
  unsigned long long snapshot;
  StellarCameraLandmark landmark;
};

struct StellarDirectorShotV056 {
  std::string name;
  unsigned long long start_snapshot;
  unsigned long long end_snapshot;
  int mode;
  std::string subject;
  double azimuth_radians;
  double elevation_radians;
  double framing_margin;
  bool has_orbit_degrees;
  double orbit_radians;
  bool has_orbit_period;
  double orbit_period_seconds;
  double lobe_sign;
  double transition_seconds;
  int easing;
};

struct StellarDirectorPathRowV056 {
  unsigned long long snapshot;
  double time_seconds;
  std::string shot_name;
  std::string subject;
  std::string transition_to;
  double transition_fraction;
  StellarCameraPose pose;
  double center[3];
  double axis[3];
  double material_half_extent_cm;
  double disk_half_extent_cm;
  double outflow_half_extent_cm;
};

struct StellarDirectorDiagnosticRowV056 {
  unsigned long long snapshot;
  double time_seconds;
  std::string shot_name;
  std::string transition_to;
  double transition_fraction;
  double center_speed_cm_per_s;
  double center_acceleration_cm_per_s2;
  double center_jerk_cm_per_s3;
  double camera_speed_cm_per_s;
  double camera_acceleration_cm_per_s2;
  double camera_jerk_cm_per_s3;
  double axis_rate_rad_per_s;
  double roll_rate_rad_per_s;
  double log_zoom_rate_per_s;
  double log_zoom_acceleration_per_s2;
  double material_occupancy;
  double disk_occupancy;
  double outflow_occupancy;
  int material_clipped;
  int disk_clipped;
  int outflow_clipped;
};

struct StellarDirectorResultV056 {
  std::vector<StellarDirectorPathRowV056> path;
  std::vector<StellarDirectorDiagnosticRowV056> diagnostics;
};

bool stellarDirectorLoadLandmarksV056(
    std::istream &input, std::vector<StellarDirectorLandmarkRowV056> *rows,
    std::string *error);
bool stellarDirectorLoadShotsV056(
    std::istream &input, std::vector<StellarDirectorShotV056> *shots,
    std::string *error);
bool stellarDirectorBuildV056(
    const std::vector<StellarDirectorLandmarkRowV056> &landmarks,
    const std::vector<StellarDirectorShotV056> &shots,
    const StellarCameraFilterParameters &filter,
    StellarDirectorResultV056 *result, std::string *error);

std::string stellarDirectorPathTableV056(
    const StellarDirectorResultV056 &result,
    const std::string &landmarks_path, const std::string &shots_path);
std::string stellarDirectorDiagnosticsTableV056(
    const StellarDirectorResultV056 &result);
std::string stellarDirectorManifestV056(
    const StellarDirectorResultV056 &result,
    const std::vector<StellarDirectorShotV056> &shots,
    const StellarCameraFilterParameters &filter,
    const std::string &landmarks_path, const std::string &shots_path,
    bool dry_run);
std::string stellarDirectorPreviewSvgV056(
    const StellarDirectorResultV056 &result);

const char *stellarDirectorModeNameV056(int mode);
const char *stellarDirectorEasingNameV056(int easing);

#endif
