#ifndef AREPO_VTK_STELLAR_CINEMATIC_DIRECTOR_V061_H
#define AREPO_VTK_STELLAR_CINEMATIC_DIRECTOR_V061_H

#include <cstddef>
#include <istream>
#include <string>
#include <vector>

#include "stellar_camera_v054.h"

enum StellarDirectorEasingV061 {
  STELLAR_DIRECTOR_LINEAR = 0,
  STELLAR_DIRECTOR_SMOOTHSTEP = 1,
  STELLAR_DIRECTOR_SMOOTHERSTEP = 2
};

enum StellarDirectorExtentSourceV061 {
  STELLAR_DIRECTOR_FILTERED_EXTENTS = 0,
  STELLAR_DIRECTOR_RAW_EXTENTS = 1
};

enum StellarDirectorTransitionKindV061 {
  STELLAR_DIRECTOR_TRANSITION_NONE = 0,
  STELLAR_DIRECTOR_TRANSITION_CONTINUOUS = 1,
  STELLAR_DIRECTOR_TRANSITION_CUT = 2
};

struct StellarDirectorLandmarkRowV061 {
  unsigned long long snapshot;
  StellarCameraLandmark landmark;
};

struct StellarDirectorShotV061 {
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
  bool has_scale_override;
  double screen_half_extent_override_cm;
  int transition_kind;
  unsigned long long transition_end_snapshot;
  int easing;
};

struct StellarDirectorPathRowV061 {
  unsigned long long snapshot;
  double time_seconds;
  std::string shot_name;
  std::string subject;
  std::string transition_from;
  std::string transition_to;
  double transition_fraction;
  int cut_from_previous;
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

struct StellarDirectorDiagnosticRowV061 {
  unsigned long long snapshot;
  double time_seconds;
  std::string shot_name;
  std::string transition_from;
  std::string transition_to;
  double transition_fraction;
  int cut_from_previous;
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
  double required_zoom_to_target_percent;
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

struct StellarDirectorResultV061 {
  std::vector<StellarDirectorPathRowV061> path;
  std::vector<StellarDirectorDiagnosticRowV061> diagnostics;
};

struct StellarDirectorMotionBudgetV061 {
  double max_roll_deg_per_frame;
  double max_zoom_percent_per_frame;
  double max_final_target_scale_error_percent;
};

struct StellarDirectorMotionAssessmentV061 {
  double maximum_roll_deg_per_frame;
  unsigned long long maximum_roll_snapshot;
  double maximum_zoom_percent_per_frame;
  unsigned long long maximum_zoom_snapshot;
  double maximum_target_scale_error_percent;
  unsigned long long maximum_target_scale_error_snapshot;
  double final_target_scale_error_percent;
  unsigned long long scale_limited_rows;
  unsigned long long declared_cut_count;
  bool passed;
};

bool stellarDirectorLoadLandmarksV061(
    std::istream &input, std::vector<StellarDirectorLandmarkRowV061> *rows,
    std::string *error);
bool stellarDirectorLoadShotsV061(
    std::istream &input, std::vector<StellarDirectorShotV061> *shots,
    std::string *error);
bool stellarDirectorBuildV061(
    const std::vector<StellarDirectorLandmarkRowV061> &landmarks,
    const std::vector<StellarDirectorShotV061> &shots,
    const StellarCameraFilterParameters &filter,
    const StellarDirectorMotionBudgetV061 &budget,
    StellarDirectorResultV061 *result, std::string *error);
bool stellarDirectorValidMotionBudgetV061(
    const StellarDirectorMotionBudgetV061 &budget);
StellarDirectorMotionAssessmentV061 stellarDirectorAssessMotionV061(
    const StellarDirectorResultV061 &result,
    const StellarDirectorMotionBudgetV061 &budget);

std::string stellarDirectorPathTableV061(
    const StellarDirectorResultV061 &result,
    const std::string &landmarks_path, const std::string &shots_path);
std::string stellarDirectorDiagnosticsTableV061(
    const StellarDirectorResultV061 &result);
std::string stellarDirectorManifestV061(
    const StellarDirectorResultV061 &result,
    const std::vector<StellarDirectorShotV061> &shots,
    const StellarCameraFilterParameters &filter,
    const StellarDirectorMotionBudgetV061 &budget,
    const StellarDirectorMotionAssessmentV061 &assessment,
    const std::string &landmarks_path, const std::string &shots_path,
    bool dry_run);
std::string stellarDirectorPreviewSvgV061(
    const StellarDirectorResultV061 &result);

const char *stellarDirectorModeNameV061(int mode);
const char *stellarDirectorEasingNameV061(int easing);
const char *stellarDirectorExtentSourceNameV061(int source);
const char *stellarDirectorTransitionKindNameV061(int kind);

#endif
