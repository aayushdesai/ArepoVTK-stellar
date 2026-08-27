#ifndef AREPO_VTK_STELLAR_CINEMATIC_DIRECTOR_V063_H
#define AREPO_VTK_STELLAR_CINEMATIC_DIRECTOR_V063_H

#include <cstddef>
#include <istream>
#include <string>
#include <vector>

#include "stellar_camera_v054.h"

enum StellarDirectorEasingV063 {
  STELLAR_DIRECTOR_LINEAR = 0,
  STELLAR_DIRECTOR_SMOOTHSTEP = 1,
  STELLAR_DIRECTOR_SMOOTHERSTEP = 2
};

enum StellarDirectorExtentSourceV063 {
  STELLAR_DIRECTOR_FILTERED_EXTENTS = 0,
  STELLAR_DIRECTOR_RAW_EXTENTS = 1
};

enum StellarDirectorTransitionKindV063 {
  STELLAR_DIRECTOR_TRANSITION_NONE = 0,
  STELLAR_DIRECTOR_TRANSITION_CONTINUOUS = 1,
  STELLAR_DIRECTOR_TRANSITION_CUT = 2
};

enum StellarDirectorVisualMetricV063 {
  STELLAR_DIRECTOR_VISUAL_PERCEPTUAL_AREA = 0,
  STELLAR_DIRECTOR_VISUAL_LARGEST_COMPONENT_AREA = 1,
  STELLAR_DIRECTOR_VISUAL_PERCEPTUAL_BBOX = 2,
  STELLAR_DIRECTOR_VISUAL_LARGEST_COMPONENT_BBOX = 3
};

struct StellarDirectorLandmarkRowV063 {
  unsigned long long snapshot;
  StellarCameraLandmark landmark;
};

struct StellarDirectorShotV063 {
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

struct StellarDirectorVisualCalibrationRowV063 {
  unsigned long long snapshot;
  std::string shot_name;
  int metric;
  double observed_fill;
  double target_fill;
  double source_screen_half_extent_cm;
  std::string source_frame_sha256;
  std::string source_camera_path_sha256;
};

struct StellarDirectorPathRowV063 {
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
  int visual_calibration_applied;
  int visual_metric;
  unsigned long long visual_left_snapshot;
  unsigned long long visual_right_snapshot;
  double visual_interpolation_fraction;
  double visual_scale_factor;
  double visual_target_fill;
  double visual_source_screen_half_extent_cm;
  double visual_absolute_target_half_extent_cm;
  double center[3];
  double axis[3];
  double material_half_extent_cm;
  double disk_half_extent_cm;
  double outflow_half_extent_cm;
  double raw_material_half_extent_cm;
  double raw_disk_half_extent_cm;
  double raw_outflow_half_extent_cm;
};

struct StellarDirectorDiagnosticRowV063 {
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
  int visual_calibration_applied;
  int visual_metric;
  unsigned long long visual_left_snapshot;
  unsigned long long visual_right_snapshot;
  double visual_interpolation_fraction;
  double visual_scale_factor;
  double visual_target_fill;
  double visual_source_screen_half_extent_cm;
  double visual_absolute_target_half_extent_cm;
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

struct StellarDirectorResultV063 {
  std::vector<StellarDirectorPathRowV063> path;
  std::vector<StellarDirectorDiagnosticRowV063> diagnostics;
};

struct StellarDirectorMotionBudgetV063 {
  double max_roll_deg_per_frame;
  double max_zoom_percent_per_frame;
  double max_final_target_scale_error_percent;
};

struct StellarDirectorMotionAssessmentV063 {
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

bool stellarDirectorLoadLandmarksV063(
    std::istream &input, std::vector<StellarDirectorLandmarkRowV063> *rows,
    std::string *error);
bool stellarDirectorLoadShotsV063(
    std::istream &input, std::vector<StellarDirectorShotV063> *shots,
    std::string *error);
bool stellarDirectorLoadVisualCalibrationV063(
    std::istream &input,
    std::vector<StellarDirectorVisualCalibrationRowV063> *calibration,
    std::string *error);
bool stellarDirectorBuildV063(
    const std::vector<StellarDirectorLandmarkRowV063> &landmarks,
    const std::vector<StellarDirectorShotV063> &shots,
    const std::vector<StellarDirectorVisualCalibrationRowV063> &calibration,
    const StellarCameraFilterParameters &filter,
    const StellarDirectorMotionBudgetV063 &budget,
    StellarDirectorResultV063 *result, std::string *error);
bool stellarDirectorValidMotionBudgetV063(
    const StellarDirectorMotionBudgetV063 &budget);
StellarDirectorMotionAssessmentV063 stellarDirectorAssessMotionV063(
    const StellarDirectorResultV063 &result,
    const StellarDirectorMotionBudgetV063 &budget);

std::string stellarDirectorPathTableV063(
    const StellarDirectorResultV063 &result,
    const std::string &landmarks_path, const std::string &shots_path,
    const std::string &calibration_path);
std::string stellarDirectorDiagnosticsTableV063(
    const StellarDirectorResultV063 &result);
std::string stellarDirectorManifestV063(
    const StellarDirectorResultV063 &result,
    const std::vector<StellarDirectorShotV063> &shots,
    const std::vector<StellarDirectorVisualCalibrationRowV063> &calibration,
    const StellarCameraFilterParameters &filter,
    const StellarDirectorMotionBudgetV063 &budget,
    const StellarDirectorMotionAssessmentV063 &assessment,
    const std::string &landmarks_path, const std::string &shots_path,
    const std::string &calibration_path,
    bool dry_run);
std::string stellarDirectorPreviewSvgV063(
    const StellarDirectorResultV063 &result);

const char *stellarDirectorModeNameV063(int mode);
const char *stellarDirectorEasingNameV063(int easing);
const char *stellarDirectorExtentSourceNameV063(int source);
const char *stellarDirectorTransitionKindNameV063(int kind);
const char *stellarDirectorVisualMetricNameV063(int metric);

#endif
