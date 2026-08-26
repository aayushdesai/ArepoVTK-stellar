#ifndef AREPO_VTK_STELLAR_CINEMATIC_DIRECTOR_V060_H
#define AREPO_VTK_STELLAR_CINEMATIC_DIRECTOR_V060_H

#include <cstddef>
#include <istream>
#include <string>
#include <vector>

#include "stellar_camera_v054.h"

enum StellarDirectorEasingV060 {
  STELLAR_DIRECTOR_LINEAR = 0,
  STELLAR_DIRECTOR_SMOOTHSTEP = 1,
  STELLAR_DIRECTOR_SMOOTHERSTEP = 2
};

enum StellarDirectorExtentSourceV060 {
  STELLAR_DIRECTOR_FILTERED_EXTENTS = 0,
  STELLAR_DIRECTOR_RAW_EXTENTS = 1
};

struct StellarDirectorLandmarkRowV060 {
  unsigned long long snapshot;
  StellarCameraLandmark landmark;
};

struct StellarDirectorShotV060 {
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
  int extent_source;
  bool has_transition_in;
  unsigned long long transition_end_snapshot;
  int easing;
};

struct StellarDirectorPathRowV060 {
  unsigned long long snapshot;
  double time_seconds;
  std::string shot_name;
  std::string subject;
  std::string transition_from;
  std::string transition_to;
  double transition_fraction;
  StellarCameraPose pose;
  double target_screen_half_extent_cm;
  int scale_limited;
  double center[3];
  double axis[3];
  double material_half_extent_cm;
  double disk_half_extent_cm;
  double outflow_half_extent_cm;
  double raw_material_half_extent_cm;
  double raw_disk_half_extent_cm;
  double raw_outflow_half_extent_cm;
};

struct StellarDirectorDiagnosticRowV060 {
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
  double target_scale_error_percent;
  int scale_limited;
  double material_occupancy;
  double disk_occupancy;
  double outflow_occupancy;
  double raw_material_occupancy;
  double raw_disk_occupancy;
  double raw_outflow_occupancy;
  int material_clipped;
  int disk_clipped;
  int outflow_clipped;
  int raw_material_clipped;
  int raw_disk_clipped;
  int raw_outflow_clipped;
};

struct StellarDirectorResultV060 {
  std::vector<StellarDirectorPathRowV060> path;
  std::vector<StellarDirectorDiagnosticRowV060> diagnostics;
};

struct StellarDirectorMotionBudgetV060 {
  double max_roll_deg_per_frame;
  double max_zoom_percent_per_frame;
  double max_final_target_scale_error_percent;
};

struct StellarDirectorMotionAssessmentV060 {
  double maximum_roll_deg_per_frame;
  unsigned long long maximum_roll_snapshot;
  double maximum_zoom_percent_per_frame;
  unsigned long long maximum_zoom_snapshot;
  double maximum_target_scale_error_percent;
  unsigned long long maximum_target_scale_error_snapshot;
  double final_target_scale_error_percent;
  unsigned long long scale_limited_rows;
  bool passed;
};

bool stellarDirectorLoadLandmarksV060(
    std::istream &input, std::vector<StellarDirectorLandmarkRowV060> *rows,
    std::string *error);
bool stellarDirectorLoadShotsV060(
    std::istream &input, std::vector<StellarDirectorShotV060> *shots,
    std::string *error);
bool stellarDirectorBuildV060(
    const std::vector<StellarDirectorLandmarkRowV060> &landmarks,
    const std::vector<StellarDirectorShotV060> &shots,
    const StellarCameraFilterParameters &filter,
    const StellarDirectorMotionBudgetV060 &budget,
    StellarDirectorResultV060 *result, std::string *error);
bool stellarDirectorValidMotionBudgetV060(
    const StellarDirectorMotionBudgetV060 &budget);
StellarDirectorMotionAssessmentV060 stellarDirectorAssessMotionV060(
    const StellarDirectorResultV060 &result,
    const StellarDirectorMotionBudgetV060 &budget);

std::string stellarDirectorPathTableV060(
    const StellarDirectorResultV060 &result,
    const std::string &landmarks_path, const std::string &shots_path);
std::string stellarDirectorDiagnosticsTableV060(
    const StellarDirectorResultV060 &result);
std::string stellarDirectorManifestV060(
    const StellarDirectorResultV060 &result,
    const std::vector<StellarDirectorShotV060> &shots,
    const StellarCameraFilterParameters &filter,
    const StellarDirectorMotionBudgetV060 &budget,
    const StellarDirectorMotionAssessmentV060 &assessment,
    const std::string &landmarks_path, const std::string &shots_path,
    bool dry_run);
std::string stellarDirectorPreviewSvgV060(
    const StellarDirectorResultV060 &result);

const char *stellarDirectorModeNameV060(int mode);
const char *stellarDirectorEasingNameV060(int easing);
const char *stellarDirectorExtentSourceNameV060(int source);

#endif
