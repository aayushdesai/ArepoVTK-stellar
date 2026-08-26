#ifndef AREPO_VTK_STELLAR_CINEMATIC_DIRECTOR_V059_H
#define AREPO_VTK_STELLAR_CINEMATIC_DIRECTOR_V059_H

#include <cstddef>
#include <istream>
#include <string>
#include <vector>

#include "stellar_camera_v054.h"

enum StellarDirectorEasingV059 {
  STELLAR_DIRECTOR_LINEAR = 0,
  STELLAR_DIRECTOR_SMOOTHSTEP = 1,
  STELLAR_DIRECTOR_SMOOTHERSTEP = 2
};

struct StellarDirectorLandmarkRowV059 {
  unsigned long long snapshot;
  StellarCameraLandmark landmark;
};

struct StellarDirectorShotV059 {
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
  bool has_transition_in;
  unsigned long long transition_end_snapshot;
  int easing;
};

struct StellarDirectorPathRowV059 {
  unsigned long long snapshot;
  double time_seconds;
  std::string shot_name;
  std::string subject;
  std::string transition_from;
  std::string transition_to;
  double transition_fraction;
  StellarCameraPose pose;
  double center[3];
  double axis[3];
  double material_half_extent_cm;
  double disk_half_extent_cm;
  double outflow_half_extent_cm;
};

struct StellarDirectorDiagnosticRowV059 {
  unsigned long long snapshot;
  double time_seconds;
  std::string shot_name;
  std::string transition_from;
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
  double roll_deg_per_frame;
  double log_zoom_rate_per_s;
  double zoom_percent_per_frame;
  double log_zoom_acceleration_per_s2;
  double material_occupancy;
  double disk_occupancy;
  double outflow_occupancy;
  int material_clipped;
  int disk_clipped;
  int outflow_clipped;
};

struct StellarDirectorResultV059 {
  std::vector<StellarDirectorPathRowV059> path;
  std::vector<StellarDirectorDiagnosticRowV059> diagnostics;
};

struct StellarDirectorMotionBudgetV059 {
  double max_roll_deg_per_frame;
  double max_zoom_percent_per_frame;
};

struct StellarDirectorMotionAssessmentV059 {
  double maximum_roll_deg_per_frame;
  unsigned long long maximum_roll_snapshot;
  double maximum_zoom_percent_per_frame;
  unsigned long long maximum_zoom_snapshot;
  bool passed;
};

bool stellarDirectorLoadLandmarksV059(
    std::istream &input, std::vector<StellarDirectorLandmarkRowV059> *rows,
    std::string *error);
bool stellarDirectorLoadShotsV059(
    std::istream &input, std::vector<StellarDirectorShotV059> *shots,
    std::string *error);
bool stellarDirectorBuildV059(
    const std::vector<StellarDirectorLandmarkRowV059> &landmarks,
    const std::vector<StellarDirectorShotV059> &shots,
    const StellarCameraFilterParameters &filter,
    StellarDirectorResultV059 *result, std::string *error);
bool stellarDirectorValidMotionBudgetV059(
    const StellarDirectorMotionBudgetV059 &budget);
StellarDirectorMotionAssessmentV059 stellarDirectorAssessMotionV059(
    const StellarDirectorResultV059 &result,
    const StellarDirectorMotionBudgetV059 &budget);

std::string stellarDirectorPathTableV059(
    const StellarDirectorResultV059 &result,
    const std::string &landmarks_path, const std::string &shots_path);
std::string stellarDirectorDiagnosticsTableV059(
    const StellarDirectorResultV059 &result);
std::string stellarDirectorManifestV059(
    const StellarDirectorResultV059 &result,
    const std::vector<StellarDirectorShotV059> &shots,
    const StellarCameraFilterParameters &filter,
    const StellarDirectorMotionBudgetV059 &budget,
    const StellarDirectorMotionAssessmentV059 &assessment,
    const std::string &landmarks_path, const std::string &shots_path,
    bool dry_run);
std::string stellarDirectorPreviewSvgV059(
    const StellarDirectorResultV059 &result);

const char *stellarDirectorModeNameV059(int mode);
const char *stellarDirectorEasingNameV059(int easing);

#endif
