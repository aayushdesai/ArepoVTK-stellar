#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "stellar_camera_path_v055.h"
#include "stellar_cinematic_director_v062.h"

namespace {

const char *landmarksText()
{
  return
      "# snapshot time cx cy cz ax ay az material disk polar\n"
      "100 0 0 0 0 0 0 1 10 8 12\n"
      "101 5 1 0 0 0.02 0 0.9998 11 8.5 13\n"
      "102 20 2 0 0 0.04 0.01 0.9991 12 9 15\n"
      "103 30 3 1 0 0.06 0.01 0.9981 13 10 18\n"
      "104 40 4 2 0 0.08 0.02 0.9966 14 11 22\n"
      "105 50 5 3 1 0.10 0.02 0.9948 15 12 28\n"
      "106 60 6 4 2 0.12 0.03 0.9923 16 13 35\n";
}

const char *shotsText()
{
  return
      "# schema=stellar_cinematic_shots_v062\n"
      "approach 100 102 orbit binary 10 20 1.10 30 - 1 filtered 60 none - -\n"
      "disk_reveal 103 104 disk_oblique disk 35 38 1.06 - - 1 raw - continuous 104 smoothstep\n"
      "north_lobe 105 106 outflow_follow positive_lobe 45 8 1.12 - - 1 raw - cut - -\n";
}

const char *calibrationText()
{
  return
      "# schema=stellar_visual_framing_calibration_v062\n"
      "100 approach perceptual_area 0.739 0.45 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "102 approach perceptual_area 0.36 0.45 ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff\n"
      "103 disk_reveal perceptual_area 1.0 0.45 bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n"
      "104 disk_reveal perceptual_area 0.81 0.45 cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\n"
      "105 north_lobe largest_component_area 0.078 0.25 dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\n"
      "106 north_lobe largest_component_area 0.023 0.25 eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\n";
}

bool loadLandmarks(const std::string &text,
                   std::vector<StellarDirectorLandmarkRowV062> *rows,
                   std::string *error)
{
  std::istringstream input(text);
  return stellarDirectorLoadLandmarksV062(input, rows, error);
}

bool loadShots(const std::string &text,
               std::vector<StellarDirectorShotV062> *shots,
               std::string *error)
{
  std::istringstream input(text);
  return stellarDirectorLoadShotsV062(input, shots, error);
}

bool loadCalibration(
    const std::string &text,
    std::vector<StellarDirectorVisualCalibrationRowV062> *calibration,
    std::string *error)
{
  std::istringstream input(text);
  return stellarDirectorLoadVisualCalibrationV062(
      input, calibration, error);
}

bool finiteDiagnostics(const StellarDirectorResultV062 &result)
{
  for(std::size_t index = 0; index < result.diagnostics.size(); ++index) {
    const StellarDirectorDiagnosticRowV062 &row = result.diagnostics[index];
    if(!std::isfinite(row.center_speed_cm_per_s) ||
       !std::isfinite(row.center_acceleration_cm_per_s2) ||
       !std::isfinite(row.center_jerk_cm_per_s3) ||
       !std::isfinite(row.camera_speed_cm_per_s) ||
       !std::isfinite(row.camera_acceleration_cm_per_s2) ||
       !std::isfinite(row.camera_jerk_cm_per_s3) ||
       !std::isfinite(row.axis_rate_rad_per_s) ||
       !std::isfinite(row.roll_rate_rad_per_s) ||
       !std::isfinite(row.roll_deg_per_frame) ||
       !std::isfinite(row.log_zoom_rate_per_s) ||
       !std::isfinite(row.zoom_percent_per_frame) ||
       !std::isfinite(row.required_zoom_to_target_percent) ||
       !std::isfinite(row.log_zoom_acceleration_per_s2) ||
       !std::isfinite(row.target_scale_error_percent) ||
       !std::isfinite(row.visual_scale_factor) ||
       !std::isfinite(row.visual_target_fill) ||
       !std::isfinite(row.raw_material_occupancy) ||
       !std::isfinite(row.raw_disk_occupancy) ||
       !std::isfinite(row.raw_outflow_occupancy))
      return false;
  }
  return true;
}

} // namespace

int main()
{
  std::vector<StellarDirectorLandmarkRowV062> landmarks;
  std::vector<StellarDirectorShotV062> shots;
  std::vector<StellarDirectorVisualCalibrationRowV062> calibration;
  std::string error;
  assert(loadLandmarks(landmarksText(), &landmarks, &error));
  assert(loadShots(shotsText(), &shots, &error));
  assert(loadCalibration(calibrationText(), &calibration, &error));
  assert(calibration.size() == 6);
  assert(calibration[0].metric == STELLAR_DIRECTOR_VISUAL_PERCEPTUAL_AREA);
  assert(calibration[4].metric ==
         STELLAR_DIRECTOR_VISUAL_LARGEST_COMPONENT_AREA);
  const std::vector<StellarDirectorShotV062> valid_shots = shots;
  const std::vector<StellarDirectorVisualCalibrationRowV062>
      valid_calibration = calibration;
  assert(shots.size() == 3);
  assert(shots[2].mode == STELLAR_CAMERA_OUTFLOW_FOLLOW);
  assert(shots[2].lobe_sign == 1.0);
  assert(shots[0].extent_source == STELLAR_DIRECTOR_FILTERED_EXTENTS);
  assert(shots[1].extent_source == STELLAR_DIRECTOR_RAW_EXTENTS);
  assert(shots[0].has_scale_override);
  assert(shots[0].screen_half_extent_override_cm == 60.0);
  assert(shots[1].transition_kind == STELLAR_DIRECTOR_TRANSITION_CONTINUOUS);
  assert(shots[2].transition_kind == STELLAR_DIRECTOR_TRANSITION_CUT);
  assert(!shots[2].has_orbit_degrees);
  assert(!shots[2].has_orbit_period);

  StellarCameraFilterParameters filter = stellarDefaultCameraFilter();
  filter.minimum_half_extent_cm = 1000.0;
  StellarDirectorMotionBudgetV062 loose_budget = {};
  loose_budget.max_roll_deg_per_frame = 180.0;
  loose_budget.max_zoom_percent_per_frame = 1000.0;
  loose_budget.max_final_target_scale_error_percent = 0.0;
  StellarDirectorResultV062 first;
  StellarDirectorResultV062 second;
  assert(stellarDirectorBuildV062(landmarks, shots, calibration, filter,
                                  loose_budget,
                                  &first, &error));
  assert(stellarDirectorBuildV062(landmarks, shots, calibration, filter,
                                  loose_budget,
                                  &second, &error));
  assert(first.path.size() == landmarks.size());
  assert(first.diagnostics.size() == first.path.size());
  assert(first.path[2].transition_to.empty());
  assert(first.path[3].transition_from == "approach");
  assert(first.path[3].transition_to == "disk_reveal");
  assert(first.path[3].transition_fraction == 0.0);
  assert(first.path[4].transition_from == "approach");
  assert(first.path[4].transition_to == "disk_reveal");
  assert(first.path[4].transition_fraction == 1.0);
  assert(first.path[5].transition_from == "disk_reveal");
  assert(first.path[5].transition_to == "north_lobe");
  assert(first.path[5].transition_fraction == 1.0);
  assert(first.path[5].cut_from_previous == 1);
  assert(first.path[6].transition_to.empty());
  assert(first.path[6].cut_from_previous == 0);
  const double opening_factor = std::sqrt(0.739 / 0.45);
  assert(std::abs(first.path[0].pose.screen_half_extent_cm -
                  60.0 * opening_factor) < 1.0e-12);
  assert(first.path[0].visual_calibration_applied == 1);
  assert(first.path[0].visual_metric ==
         STELLAR_DIRECTOR_VISUAL_PERCEPTUAL_AREA);
  assert(std::abs(first.path[0].visual_scale_factor - opening_factor) <
         1.0e-12);
  const double approach_end_factor = std::sqrt(0.36 / 0.45);
  const double expected_time_interpolated_factor = std::exp(
      0.75 * std::log(opening_factor) +
      0.25 * std::log(approach_end_factor));
  assert(std::abs(first.path[1].visual_scale_factor -
                  expected_time_interpolated_factor) < 1.0e-12);
  assert(std::abs(first.path[1].visual_interpolation_fraction - 0.25) <
         1.0e-12);
  const double expected_raw_disk_extent =
      std::max(0.80 * 1.20 * 14.0, 1.30 * 11.0);
  assert(std::abs(first.path[4].target_screen_half_extent_cm -
                  1.06 * expected_raw_disk_extent * std::sqrt(0.81 / 0.45)) <
         1.0e-12);
  assert(first.path[5].visual_scale_factor < 1.0);
  assert(first.path[6].visual_scale_factor <
         first.path[5].visual_scale_factor);
  assert(first.diagnostics[5].cut_from_previous == 1);
  assert(first.diagnostics[5].zoom_percent_per_frame == 0.0);
  assert(first.diagnostics[5].required_zoom_to_target_percent > 0.0);
  assert(first.diagnostics[5].camera_speed_cm_per_s == 0.0);
  assert(first.diagnostics[1].center_acceleration_cm_per_s2 == 0.0);
  assert(first.diagnostics[1].camera_acceleration_cm_per_s2 == 0.0);
  assert(first.diagnostics[2].center_jerk_cm_per_s3 == 0.0);
  assert(first.diagnostics[2].camera_jerk_cm_per_s3 == 0.0);
  assert(finiteDiagnostics(first));
  const StellarDirectorMotionAssessmentV062 loose_assessment =
      stellarDirectorAssessMotionV062(first, loose_budget);
  assert(loose_assessment.passed);
  assert(loose_assessment.declared_cut_count == 1);
  StellarDirectorMotionBudgetV062 tight_budget = {};
  tight_budget.max_roll_deg_per_frame = 1.0e-12;
  tight_budget.max_zoom_percent_per_frame = 1.0e-12;
  tight_budget.max_final_target_scale_error_percent = 0.0;
  const StellarDirectorMotionAssessmentV062 tight_assessment =
      stellarDirectorAssessMotionV062(first, tight_budget);
  assert(!tight_assessment.passed);
  assert(tight_assessment.maximum_roll_snapshot > 0);
  assert(tight_assessment.maximum_zoom_snapshot > 0);

  const std::string path1 = stellarDirectorPathTableV062(
      first, "landmarks.tsv", "shots.tsv", "visual.tsv");
  const std::string path2 = stellarDirectorPathTableV062(
      second, "landmarks.tsv", "shots.tsv", "visual.tsv");
  assert(path1 == path2);
  assert(stellarDirectorDiagnosticsTableV062(first) ==
         stellarDirectorDiagnosticsTableV062(second));
  const std::string manifest = stellarDirectorManifestV062(
      first, shots, calibration, filter, loose_budget, loose_assessment,
      "landmarks.tsv", "shots.tsv", "visual.tsv", false);
  assert(manifest.find("motion_budget.passed=true") != std::string::npos);
  assert(manifest.find("framing.scale_limiter=path_log_step") !=
         std::string::npos);
  assert(manifest.find("framing.raw_extents_ignore_filtered_minimum=true") !=
         std::string::npos);
  assert(manifest.find("shot.1.extent_source=raw") != std::string::npos);
  assert(manifest.find("shot.0.screen_half_extent_override_cm=60") !=
         std::string::npos);
  assert(manifest.find("shot.1.transition_kind=continuous") !=
         std::string::npos);
  assert(manifest.find("shot.2.transition_kind=cut") != std::string::npos);
  assert(manifest.find("shot.1.transition_end_snapshot=104") !=
         std::string::npos);
  assert(manifest.find("framing.visual_calibration.source_hash_bound=true") !=
         std::string::npos);
  assert(manifest.find("visual_calibration_rows=6") != std::string::npos);
  assert(manifest.find("visual_calibration.4.metric=largest_component_area") !=
         std::string::npos);
  assert(manifest.find("visual_calibration.0.source_frame_sha256=aaaaaaaa") !=
         std::string::npos);
  assert(manifest ==
         stellarDirectorManifestV062(
             first, shots, calibration, filter, loose_budget,
             loose_assessment, "landmarks.tsv", "shots.tsv", "visual.tsv",
             false));
  assert(manifest == stellarDirectorManifestV062(
             second, shots, calibration, filter, loose_budget,
             stellarDirectorAssessMotionV062(second, loose_budget),
             "landmarks.tsv", "shots.tsv", "visual.tsv", false));
  const std::string diagnostics = stellarDirectorDiagnosticsTableV062(first);
  assert(diagnostics.find("transition_from transition_to") !=
         std::string::npos);
  assert(diagnostics.find("roll_deg_per_frame") != std::string::npos);
  assert(diagnostics.find("zoom_percent_per_frame") != std::string::npos);
  assert(diagnostics.find("target_scale_error_percent") != std::string::npos);
  assert(diagnostics.find("visual_scale_factor") != std::string::npos);
  assert(diagnostics.find("visual_target_fill") != std::string::npos);
  assert(diagnostics.find("raw_disk_occupancy") != std::string::npos);
  assert(diagnostics.find("cut_from_previous") != std::string::npos);
  assert(diagnostics.find("required_zoom_to_target_percent") !=
         std::string::npos);
  const std::string preview = stellarDirectorPreviewSvgV062(first);
  assert(preview == stellarDirectorPreviewSvgV062(second));
  assert(preview.find("Stellar cinematic camera dry-run") != std::string::npos);
  assert(preview.find("Raw physical occupancy") != std::string::npos);

  StellarCameraPathV055 native_path;
  std::istringstream path_input(path1);
  assert(native_path.loadStream(path_input, &error));
  assert(native_path.size() == first.path.size());
  assert(native_path.find(100));
  assert(native_path.find(106));

  std::vector<StellarDirectorShotV062> elevated = shots;
  elevated[0].elevation_radians += 0.2;
  StellarDirectorResultV062 elevated_result;
  assert(stellarDirectorBuildV062(landmarks, elevated, calibration, filter,
                                  loose_budget,
                                  &elevated_result, &error));
  assert(std::abs(elevated_result.path[0].pose.position[2] -
                  first.path[0].pose.position[2]) > 1.0e-6);

  std::vector<StellarDirectorShotV062> filtered_shots = shots;
  filtered_shots[1].extent_source = STELLAR_DIRECTOR_FILTERED_EXTENTS;
  StellarDirectorResultV062 filtered_result;
  assert(stellarDirectorBuildV062(landmarks, filtered_shots, calibration,
                                  filter, loose_budget, &filtered_result,
                                  &error));
  assert(std::abs(filtered_result.path[4].target_screen_half_extent_cm -
                  first.path[4].target_screen_half_extent_cm) > 1.0e-6);

  StellarDirectorMotionBudgetV062 limited_budget = {};
  limited_budget.max_roll_deg_per_frame = 180.0;
  limited_budget.max_zoom_percent_per_frame = 1.0;
  limited_budget.max_final_target_scale_error_percent = 1.0e9;
  StellarDirectorResultV062 limited_result;
  assert(stellarDirectorBuildV062(landmarks, shots, calibration, filter,
                                  limited_budget,
                                  &limited_result, &error));
  const StellarDirectorMotionAssessmentV062 limited_assessment =
      stellarDirectorAssessMotionV062(limited_result, limited_budget);
  assert(limited_assessment.passed);
  assert(limited_assessment.maximum_zoom_percent_per_frame <=
         limited_budget.max_zoom_percent_per_frame + 1.0e-10);
  assert(limited_assessment.scale_limited_rows > 0);
  assert(limited_assessment.maximum_target_scale_error_percent > 0.0);
  StellarDirectorMotionBudgetV062 no_lag_budget = limited_budget;
  no_lag_budget.max_final_target_scale_error_percent = 0.0;
  const StellarDirectorMotionAssessmentV062 no_lag_assessment =
      stellarDirectorAssessMotionV062(limited_result, no_lag_budget);
  assert(!no_lag_assessment.passed);
  assert(no_lag_assessment.final_target_scale_error_percent > 0.0);

  const std::string missing_schema =
      "one 100 106 disk_edge merger 0 0 1.1 - - 1 raw - none - -\n";
  assert(!loadShots(missing_schema, &shots, &error));
  assert(error.find("Missing schema") != std::string::npos);

  const std::string dual_orbit =
      "# schema=stellar_cinematic_shots_v062\n"
      "one 100 106 orbit binary 0 20 1.1 90 200 1 raw - none - -\n";
  assert(!loadShots(dual_orbit, &shots, &error));
  assert(error.find("not both") != std::string::npos);

  const std::string range_gap =
      "# schema=stellar_cinematic_shots_v062\n"
      "one 100 102 disk_edge merger 0 0 1.1 - - 1 raw - none - -\n"
      "two 104 106 disk_edge merger 0 0 1.1 - - 1 raw - continuous 105 linear\n";
  assert(!loadShots(range_gap, &shots, &error));
  assert(error.find("contiguous") != std::string::npos);

  const std::string duplicate_name =
      "# schema=stellar_cinematic_shots_v062\n"
      "one 100 102 disk_edge merger 0 0 1.1 - - 1 raw - none - -\n"
      "one 103 106 disk_edge merger 0 0 1.1 - - 1 raw - continuous 105 linear\n";
  assert(!loadShots(duplicate_name, &shots, &error));
  assert(error.find("unique") != std::string::npos);

  const std::string first_transition =
      "# schema=stellar_cinematic_shots_v062\n"
      "one 100 106 disk_edge merger 0 0 1.1 - - 1 raw - continuous 104 linear\n";
  assert(!loadShots(first_transition, &shots, &error));
  assert(error.find("first shot") != std::string::npos);

  const std::string transition_outside_shot =
      "# schema=stellar_cinematic_shots_v062\n"
      "one 100 102 disk_edge merger 0 0 1.1 - - 1 raw - none - -\n"
      "two 103 106 disk_edge merger 0 0 1.1 - - 1 raw - continuous 107 linear\n";
  assert(!loadShots(transition_outside_shot, &shots, &error));
  assert(error.find("bounds or controls") != std::string::npos);

  const std::string invalid_extent_source =
      "# schema=stellar_cinematic_shots_v062\n"
      "one 100 106 disk_edge merger 0 0 1.1 - - 1 stale - none - -\n";
  assert(!loadShots(invalid_extent_source, &shots, &error));
  assert(error.find("Invalid shot value") != std::string::npos);

  const std::string cut_with_window =
      "# schema=stellar_cinematic_shots_v062\n"
      "one 100 102 disk_edge merger 0 0 1.1 - - 1 raw - none - -\n"
      "two 103 106 outflow_side wind 0 0 1.1 - - 1 raw - cut 105 linear\n";
  assert(!loadShots(cut_with_window, &shots, &error));
  assert(error.find("bounds or controls") != std::string::npos);

  const std::string implicit_later_transition =
      "# schema=stellar_cinematic_shots_v062\n"
      "one 100 102 disk_edge merger 0 0 1.1 - - 1 raw - none - -\n"
      "two 103 106 outflow_side wind 0 0 1.1 - - 1 raw - none - -\n";
  assert(!loadShots(implicit_later_transition, &shots, &error));
  assert(error.find("later shot") != std::string::npos);

  const std::string invalid_scale_override =
      "# schema=stellar_cinematic_shots_v062\n"
      "one 100 106 disk_edge merger 0 0 1.1 - - 1 filtered -4 none - -\n";
  assert(!loadShots(invalid_scale_override, &shots, &error));
  assert(error.find("bounds or controls") != std::string::npos);

  std::vector<StellarDirectorVisualCalibrationRowV062> invalid_calibration;
  const std::string missing_visual_schema =
      "100 approach perceptual_area 0.5 0.4 "
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n";
  assert(!loadCalibration(missing_visual_schema, &invalid_calibration, &error));
  assert(error.find("Missing schema") != std::string::npos);

  const std::string bad_visual_hash =
      "# schema=stellar_visual_framing_calibration_v062\n"
      "100 approach perceptual_area 0.5 0.4 not-a-hash\n";
  assert(!loadCalibration(bad_visual_hash, &invalid_calibration, &error));
  assert(error.find("Invalid visual calibration") != std::string::npos);

  const std::string mixed_visual_metric =
      "# schema=stellar_visual_framing_calibration_v062\n"
      "100 approach perceptual_area 0.5 0.4 "
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
      "102 approach perceptual_bbox 0.5 0.4 "
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n";
  assert(!loadCalibration(mixed_visual_metric, &invalid_calibration, &error));
  assert(error.find("one visual metric") != std::string::npos);

  std::vector<StellarDirectorVisualCalibrationRowV062> missing_shot =
      valid_calibration;
  missing_shot.erase(missing_shot.begin() + 4, missing_shot.end());
  StellarDirectorResultV062 invalid_result;
  assert(!stellarDirectorBuildV062(landmarks, valid_shots, missing_shot,
                                   filter, loose_budget, &invalid_result,
                                   &error));
  assert(error.find("Every shot requires") != std::string::npos);

  std::vector<StellarDirectorVisualCalibrationRowV062> wrong_snapshot =
      valid_calibration;
  wrong_snapshot[0].snapshot = 99;
  assert(!stellarDirectorBuildV062(landmarks, valid_shots, wrong_snapshot,
                                   filter, loose_budget, &invalid_result,
                                   &error));
  assert(error.find("bind a shot landmark") != std::string::npos);

  std::vector<StellarDirectorVisualCalibrationRowV062> unknown_shot =
      valid_calibration;
  StellarDirectorVisualCalibrationRowV062 extra = valid_calibration.back();
  extra.shot_name = "unknown";
  unknown_shot.push_back(extra);
  assert(!stellarDirectorBuildV062(landmarks, valid_shots, unknown_shot,
                                   filter, loose_budget, &invalid_result,
                                   &error));
  assert(error.find("unknown shot") != std::string::npos);

  std::vector<StellarDirectorLandmarkRowV062> invalid_landmarks;
  const std::string negative_extent =
      "100 0 0 0 0 0 0 1 10 -1 12\n";
  assert(!loadLandmarks(negative_extent, &invalid_landmarks, &error));
  assert(error.find("nonnegative") != std::string::npos);

  std::cout << "STELLAR_CINEMATIC_DIRECTOR_V062_OK\n";
  return 0;
}
