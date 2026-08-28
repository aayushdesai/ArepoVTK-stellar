#ifndef AREPO_VTK_STELLAR_CINEMATIC_DIRECTOR_V069_H
#define AREPO_VTK_STELLAR_CINEMATIC_DIRECTOR_V069_H

#include <cstddef>
#include <istream>
#include <string>
#include <vector>

#include "stellar_camera_v054.h"

enum StellarDirectorEasingV069 {
  STELLAR_DIRECTOR_LINEAR = 0,
  STELLAR_DIRECTOR_SMOOTHSTEP = 1,
  STELLAR_DIRECTOR_SMOOTHERSTEP = 2
};

enum StellarDirectorExtentSourceV069 {
  STELLAR_DIRECTOR_FILTERED_EXTENTS = 0,
  STELLAR_DIRECTOR_RAW_EXTENTS = 1
};

enum StellarDirectorTransitionKindV069 {
  STELLAR_DIRECTOR_TRANSITION_NONE = 0,
  STELLAR_DIRECTOR_TRANSITION_CONTINUOUS = 1,
  STELLAR_DIRECTOR_TRANSITION_CUT = 2
};

enum StellarDirectorVisualMetricV069 {
  STELLAR_DIRECTOR_VISUAL_PERCEPTUAL_AREA = 0,
  STELLAR_DIRECTOR_VISUAL_LARGEST_COMPONENT_AREA = 1,
  STELLAR_DIRECTOR_VISUAL_PERCEPTUAL_BBOX = 2,
  STELLAR_DIRECTOR_VISUAL_LARGEST_COMPONENT_BBOX = 3
};

struct StellarDirectorLandmarkRowV069 {
  unsigned long long snapshot;
  StellarCameraLandmark landmark;
};

struct StellarDirectorShotV069 {
  std::string name;
  unsigned long long start_snapshot;
  unsigned long long end_snapshot;
  int mode;
  std::string subject;
  double azimuth_radians;
  double elevation_radians;
  double roll_radians;
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

struct StellarDirectorVisualCalibrationRowV069 {
  unsigned long long snapshot;
  std::string shot_name;
  int metric;
  double observed_fill;
  double target_fill;
  double source_screen_half_extent_cm;
  std::string source_frame_sha256;
  std::string source_camera_path_sha256;
};

struct StellarDirectorSignedFramingRowV069 {
  std::string name;
  std::string shot_name;
  unsigned long long source_snapshot;
  unsigned long long transition_start_snapshot;
  unsigned long long transition_end_snapshot;
  int easing;
  std::string feature_mode;
  std::string features;
  double minimum_weight;
  double coverage;
  double target_half_width_fraction;
  double target_half_height_fraction;
  double look_at_shift_screen_x_fraction;
  double look_at_shift_screen_y_fraction;
  double half_extent_scale_factor;
  double source_screen_half_extent_cm;
  std::string framing_plan_sha256;
  std::string source_frame_sha256;
  std::string source_camera_path_sha256;
  std::string source_camera_row_sha256;
};

struct StellarDirectorPathRowV069 {
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
  int signed_framing_applied;
  std::string signed_framing_name;
  double signed_framing_fraction;
  double signed_pan_x_fraction;
  double signed_pan_y_fraction;
  double signed_absolute_target_half_extent_cm;
  double center[3];
  double axis[3];
  double material_half_extent_cm;
  double disk_half_extent_cm;
  double outflow_half_extent_cm;
  double raw_material_half_extent_cm;
  double raw_disk_half_extent_cm;
  double raw_outflow_half_extent_cm;
};

struct StellarDirectorDiagnosticRowV069 {
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
  int signed_framing_applied;
  std::string signed_framing_name;
  double signed_framing_fraction;
  double signed_pan_x_fraction;
  double signed_pan_y_fraction;
  double artistic_pan_step_fraction_per_frame;
  double artistic_pan_acceleration_fraction_per_frame2;
  double signed_absolute_target_half_extent_cm;
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

struct StellarDirectorResultV069 {
  std::vector<StellarDirectorPathRowV069> path;
  std::vector<StellarDirectorDiagnosticRowV069> diagnostics;
};

struct StellarDirectorMotionBudgetV069 {
  double max_roll_deg_per_frame;
  double max_zoom_percent_per_frame;
  double max_artistic_pan_fraction_per_frame;
  double max_final_target_scale_error_percent;
};

struct StellarDirectorMotionAssessmentV069 {
  double maximum_roll_deg_per_frame;
  unsigned long long maximum_roll_snapshot;
  double maximum_zoom_percent_per_frame;
  unsigned long long maximum_zoom_snapshot;
  double maximum_artistic_pan_fraction_per_frame;
  unsigned long long maximum_artistic_pan_snapshot;
  double maximum_target_scale_error_percent;
  unsigned long long maximum_target_scale_error_snapshot;
  double final_target_scale_error_percent;
  unsigned long long scale_limited_rows;
  unsigned long long declared_cut_count;
  bool passed;
};

bool stellarDirectorLoadLandmarksV069(
    std::istream &input, std::vector<StellarDirectorLandmarkRowV069> *rows,
    std::string *error);
bool stellarDirectorLoadShotsV069(
    std::istream &input, std::vector<StellarDirectorShotV069> *shots,
    std::string *error);
bool stellarDirectorLoadVisualCalibrationV069(
    std::istream &input,
    std::vector<StellarDirectorVisualCalibrationRowV069> *calibration,
    std::string *error);
bool stellarDirectorLoadSignedFramingV069(
    std::istream &input,
    std::vector<StellarDirectorSignedFramingRowV069> *framing,
    std::string *error);
bool stellarDirectorBuildV069(
    const std::vector<StellarDirectorLandmarkRowV069> &landmarks,
    const std::vector<StellarDirectorShotV069> &shots,
    const std::vector<StellarDirectorVisualCalibrationRowV069> &calibration,
    const std::vector<StellarDirectorSignedFramingRowV069> &framing,
    const StellarCameraFilterParameters &filter,
    const StellarDirectorMotionBudgetV069 &budget,
    StellarDirectorResultV069 *result, std::string *error);
bool stellarDirectorValidMotionBudgetV069(
    const StellarDirectorMotionBudgetV069 &budget);
StellarDirectorMotionAssessmentV069 stellarDirectorAssessMotionV069(
    const StellarDirectorResultV069 &result,
    const StellarDirectorMotionBudgetV069 &budget);

std::string stellarDirectorPathTableV069(
    const StellarDirectorResultV069 &result,
    const std::string &landmarks_path, const std::string &shots_path,
    const std::string &calibration_path,
    const std::string &framing_path);
std::string stellarDirectorDiagnosticsTableV069(
    const StellarDirectorResultV069 &result);
std::string stellarDirectorManifestV069(
    const StellarDirectorResultV069 &result,
    const std::vector<StellarDirectorShotV069> &shots,
    const std::vector<StellarDirectorVisualCalibrationRowV069> &calibration,
    const std::vector<StellarDirectorSignedFramingRowV069> &framing,
    const StellarCameraFilterParameters &filter,
    const StellarDirectorMotionBudgetV069 &budget,
    const StellarDirectorMotionAssessmentV069 &assessment,
    const std::string &landmarks_path, const std::string &shots_path,
    const std::string &calibration_path,
    const std::string &framing_path,
    bool dry_run);
std::string stellarDirectorPreviewSvgV069(
    const StellarDirectorResultV069 &result);

const char *stellarDirectorModeNameV069(int mode);
const char *stellarDirectorEasingNameV069(int easing);
const char *stellarDirectorExtentSourceNameV069(int source);
const char *stellarDirectorTransitionKindNameV069(int kind);
const char *stellarDirectorVisualMetricNameV069(int metric);

#endif
