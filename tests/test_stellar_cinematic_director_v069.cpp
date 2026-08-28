#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "stellar_camera_path_v055.h"
#include "stellar_cinematic_director_v069.h"

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
      "# schema=stellar_cinematic_shots_v069\n"
      "continuous_story 100 106 orbit remnant 105.28474080291014 "
      "34.80401175271321 20 1.0 6 - 1 raw 60 none - -\n";
}

const char *calibrationText()
{
  return
      "# schema=stellar_visual_framing_calibration_v069\n"
      "100 continuous_story perceptual_area 0.4 0.4 60 "
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa "
      "1111111111111111111111111111111111111111111111111111111111111111\n"
      "106 continuous_story perceptual_area 0.4 0.4 60 "
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb "
      "1111111111111111111111111111111111111111111111111111111111111111\n";
}

const char *framingText()
{
  return
      "# schema=stellar_signed_framing_application_v069\n"
      "disk_recenter continuous_story 102 101 103 smootherstep single disk "
      "0.10 0.98 0.75 0.75 0.10 -0.20 1.5 80 "
      "2222222222222222222222222222222222222222222222222222222222222222 "
      "3333333333333333333333333333333333333333333333333333333333333333 "
      "4444444444444444444444444444444444444444444444444444444444444444 "
      "5555555555555555555555555555555555555555555555555555555555555555\n"
      "bipolar_reveal continuous_story 105 104 106 smootherstep bipolar_union "
      "polar_positive-polar_negative 0.25 0.98 0.80 0.80 -0.05 0.35 2.0 90 "
      "6666666666666666666666666666666666666666666666666666666666666666 "
      "7777777777777777777777777777777777777777777777777777777777777777 "
      "8888888888888888888888888888888888888888888888888888888888888888 "
      "9999999999999999999999999999999999999999999999999999999999999999\n";
}

bool loadLandmarks(std::vector<StellarDirectorLandmarkRowV069> *rows,
                   std::string *error)
{
  std::istringstream input(landmarksText());
  return stellarDirectorLoadLandmarksV069(input, rows, error);
}

bool loadShots(const std::string &text,
               std::vector<StellarDirectorShotV069> *shots,
               std::string *error)
{
  std::istringstream input(text);
  return stellarDirectorLoadShotsV069(input, shots, error);
}

bool loadCalibration(
    std::vector<StellarDirectorVisualCalibrationRowV069> *calibration,
    std::string *error)
{
  std::istringstream input(calibrationText());
  return stellarDirectorLoadVisualCalibrationV069(input, calibration, error);
}

bool loadFraming(
    const std::string &text,
    std::vector<StellarDirectorSignedFramingRowV069> *framing,
    std::string *error)
{
  std::istringstream input(text);
  return stellarDirectorLoadSignedFramingV069(input, framing, error);
}

bool finiteDiagnostics(const StellarDirectorResultV069 &result)
{
  for(std::size_t index = 0; index < result.diagnostics.size(); ++index) {
    const StellarDirectorDiagnosticRowV069 &row = result.diagnostics[index];
    if(!std::isfinite(row.roll_deg_per_frame) ||
       !std::isfinite(row.zoom_percent_per_frame) ||
       !std::isfinite(row.target_scale_error_percent) ||
       !std::isfinite(row.signed_pan_x_fraction) ||
       !std::isfinite(row.signed_pan_y_fraction) ||
       !std::isfinite(row.artistic_pan_step_fraction_per_frame) ||
       !std::isfinite(row.signed_absolute_target_half_extent_cm))
      return false;
  }
  return true;
}

double normalizedDot(const double left[3], const double right[3])
{
  double dot = 0.0;
  double left_norm = 0.0;
  double right_norm = 0.0;
  for(int component = 0; component < 3; ++component) {
    dot += left[component] * right[component];
    left_norm += left[component] * left[component];
    right_norm += right[component] * right[component];
  }
  return dot / std::sqrt(left_norm * right_norm);
}

} // namespace

int main()
{
  std::vector<StellarDirectorLandmarkRowV069> landmarks;
  std::vector<StellarDirectorShotV069> shots;
  std::vector<StellarDirectorVisualCalibrationRowV069> calibration;
  std::vector<StellarDirectorSignedFramingRowV069> framing;
  std::string error;
  assert(loadLandmarks(&landmarks, &error));
  assert(loadShots(shotsText(), &shots, &error));
  assert(loadCalibration(&calibration, &error));
  assert(loadFraming(framingText(), &framing, &error));
  const std::vector<StellarDirectorShotV069> valid_shots = shots;
  assert(shots.size() == 1);
  assert(framing.size() == 2);
  assert(framing[1].feature_mode == "bipolar_union");
  assert(framing[1].features == "polar_positive-polar_negative");

  StellarCameraFilterParameters filter = stellarDefaultCameraFilter();
  filter.minimum_half_extent_cm = 1000.0;
  StellarDirectorMotionBudgetV069 loose_budget = {180.0, 1000.0, 10.0, 0.0};
  StellarDirectorResultV069 first;
  StellarDirectorResultV069 second;
  assert(stellarDirectorBuildV069(landmarks, shots, calibration, framing,
                                  filter, loose_budget, &first, &error));
  assert(stellarDirectorBuildV069(landmarks, shots, calibration, framing,
                                  filter, loose_budget, &second, &error));
  assert(first.path.size() == 7);
  assert(first.path[0].signed_framing_applied == 0);
  assert(first.path[0].signed_framing_name == "identity");
  assert(first.path[1].signed_framing_name == "disk_recenter");
  assert(first.path[1].signed_framing_fraction == 0.0);
  assert(first.path[3].signed_framing_fraction == 1.0);
  assert(std::abs(first.path[3].pose.screen_half_extent_cm - 120.0) < 1e-10);
  assert(std::abs(first.path[3].signed_pan_x_fraction - 8.0 / 120.0) < 1e-10);
  assert(std::abs(first.path[3].signed_pan_y_fraction + 16.0 / 120.0) < 1e-10);
  assert(first.path[4].signed_framing_name == "bipolar_reveal");
  assert(first.path[4].signed_framing_fraction == 0.0);
  assert(first.path[6].signed_framing_fraction == 1.0);
  assert(std::abs(first.path[6].pose.screen_half_extent_cm - 180.0) < 1e-10);
  assert(std::abs(first.path[6].signed_pan_x_fraction + 4.5 / 180.0) < 1e-10);
  assert(std::abs(first.path[6].signed_pan_y_fraction - 31.5 / 180.0) < 1e-10);
  assert(finiteDiagnostics(first));

  std::string zero_roll_text = shotsText();
  const std::string authored_roll = "34.80401175271321 20 1.0";
  const std::string zero_roll = "34.80401175271321 0 1.0";
  zero_roll_text.replace(zero_roll_text.find(authored_roll),
                         authored_roll.size(), zero_roll);
  std::vector<StellarDirectorShotV069> zero_roll_shots;
  assert(loadShots(zero_roll_text, &zero_roll_shots, &error));
  StellarDirectorResultV069 zero_roll_result;
  assert(stellarDirectorBuildV069(landmarks, zero_roll_shots, calibration,
                                  framing, filter, loose_budget,
                                  &zero_roll_result, &error));
  assert(zero_roll_result.path.size() == first.path.size());
  for(std::size_t index = 0; index < first.path.size(); ++index) {
    double rolled_view[3];
    double unrolled_view[3];
    for(int component = 0; component < 3; ++component) {
      rolled_view[component] = first.path[index].pose.position[component] -
          first.path[index].pose.look_at[component];
      unrolled_view[component] =
          zero_roll_result.path[index].pose.position[component] -
          zero_roll_result.path[index].pose.look_at[component];
    }
    assert(std::abs(normalizedDot(rolled_view, unrolled_view) - 1.0) < 1e-12);
    assert(std::abs(first.path[index].pose.screen_half_extent_cm -
                    zero_roll_result.path[index].pose.screen_half_extent_cm) <
           1e-12);
    assert(std::abs(first.path[index].signed_pan_x_fraction -
                    zero_roll_result.path[index].signed_pan_x_fraction) <
           1e-12);
    assert(std::abs(first.path[index].signed_pan_y_fraction -
                    zero_roll_result.path[index].signed_pan_y_fraction) <
           1e-12);
  }
  const double final_up_dot = normalizedDot(first.path.back().pose.up,
      zero_roll_result.path.back().pose.up);
  assert(std::abs(final_up_dot - std::cos(20.0 * std::acos(-1.0) / 180.0)) <
         1e-12);

  const StellarDirectorMotionAssessmentV069 loose_assessment =
      stellarDirectorAssessMotionV069(first, loose_budget);
  assert(loose_assessment.passed);
  assert(loose_assessment.declared_cut_count == 0);
  assert(loose_assessment.maximum_roll_deg_per_frame > 0.0);
  assert(loose_assessment.maximum_artistic_pan_fraction_per_frame > 0.0);
  StellarDirectorMotionBudgetV069 tight_pan = loose_budget;
  tight_pan.max_artistic_pan_fraction_per_frame = 1.0e-12;
  assert(!stellarDirectorAssessMotionV069(first, tight_pan).passed);

  const std::string path1 = stellarDirectorPathTableV069(
      first, "landmarks.tsv", "shots.tsv", "visual.tsv", "framing.tsv");
  const std::string path2 = stellarDirectorPathTableV069(
      second, "landmarks.tsv", "shots.tsv", "visual.tsv", "framing.tsv");
  assert(path1 == path2);
  StellarCameraPathV055 native_path;
  std::istringstream path_input(path1);
  assert(native_path.loadStream(path_input, &error));
  assert(native_path.size() == first.path.size());

  const std::string diagnostics = stellarDirectorDiagnosticsTableV069(first);
  assert(diagnostics == stellarDirectorDiagnosticsTableV069(second));
  assert(diagnostics.find("artistic_pan_step_fraction_per_frame") !=
         std::string::npos);
  assert(diagnostics.find("signed_framing_name") != std::string::npos);
  const std::string manifest = stellarDirectorManifestV069(
      first, shots, calibration, framing, filter, loose_budget,
      loose_assessment, "landmarks.tsv", "shots.tsv", "visual.tsv",
      "framing.tsv", false);
  assert(manifest.find("schema=stellar_cinematic_direction_manifest_v069") !=
         std::string::npos);
  assert(manifest.find("transitions.hard_cuts_allowed=false") !=
         std::string::npos);
  assert(manifest.find("signed_framing.1.features=polar_positive-polar_negative") !=
         std::string::npos);
  assert(manifest.find("signed_framing.1.framing_plan_sha256=66666666") !=
         std::string::npos);
  assert(manifest.find("motion.declared_cut_count=0") != std::string::npos);
  assert(manifest.find("shot.0.roll_deg=20") != std::string::npos);
  assert(manifest == stellarDirectorManifestV069(
      second, shots, calibration, framing, filter, loose_budget,
      stellarDirectorAssessMotionV069(second, loose_budget),
      "landmarks.tsv", "shots.tsv", "visual.tsv", "framing.tsv", false));

  StellarDirectorMotionBudgetV069 limited_budget = {180.0, 1.0, 10.0, 1e9};
  StellarDirectorResultV069 limited;
  assert(stellarDirectorBuildV069(landmarks, shots, calibration, framing,
                                  filter, limited_budget, &limited, &error));
  const StellarDirectorMotionAssessmentV069 limited_assessment =
      stellarDirectorAssessMotionV069(limited, limited_budget);
  assert(limited_assessment.passed);
  assert(limited_assessment.maximum_zoom_percent_per_frame <= 1.0 + 1e-10);
  assert(limited_assessment.scale_limited_rows > 0);

  const std::string cut_shots =
      "# schema=stellar_cinematic_shots_v069\n"
      "one 100 102 orbit remnant 0 20 0 1 - - 1 raw 60 none - -\n"
      "two 103 106 outflow_side wind 0 20 0 1 - - 1 raw - cut - -\n";
  assert(!loadShots(cut_shots, &shots, &error));
  assert(error.find("cuts are not allowed") != std::string::npos);

  const std::string excessive_roll_shots =
      "# schema=stellar_cinematic_shots_v069\n"
      "one 100 106 orbit remnant 0 20 181 1 - - 1 raw 60 none - -\n";
  assert(!loadShots(excessive_roll_shots, &shots, &error));
  assert(error.find("Shot bounds or controls are invalid") !=
         std::string::npos);

  std::vector<StellarDirectorSignedFramingRowV069> invalid_framing;
  const std::string missing_schema =
      "one continuous_story 102 101 103 smootherstep single disk 0.1 0.98 "
      "0.75 0.75 0 0 1 60 "
      "2222222222222222222222222222222222222222222222222222222222222222 "
      "3333333333333333333333333333333333333333333333333333333333333333 "
      "4444444444444444444444444444444444444444444444444444444444444444 "
      "5555555555555555555555555555555555555555555555555555555555555555\n";
  assert(!loadFraming(missing_schema, &invalid_framing, &error));
  assert(error.find("Missing schema") != std::string::npos);

  std::string overlap = framingText();
  const std::string old_window = "bipolar_reveal continuous_story 105 104 106";
  const std::string new_window = "bipolar_reveal continuous_story 105 103 106";
  overlap.replace(overlap.find(old_window), old_window.size(), new_window);
  assert(!loadFraming(overlap, &invalid_framing, &error));
  assert(error.find("nonoverlapping") != std::string::npos);

  std::string missing_source = framingText();
  const std::string old_source = "disk_recenter continuous_story 102";
  const std::string new_source = "disk_recenter continuous_story 99";
  missing_source.replace(missing_source.find(old_source), old_source.size(),
                         new_source);
  assert(loadFraming(missing_source, &invalid_framing, &error));
  StellarDirectorResultV069 invalid_result;
  assert(!stellarDirectorBuildV069(landmarks, valid_shots, calibration,
                                   invalid_framing, filter,
                                   loose_budget, &invalid_result, &error));
  assert(error.find("source and window boundary") != std::string::npos);

  std::cout << "STELLAR_CINEMATIC_DIRECTOR_V069_OK"
            << " signed_rows=2 hard_cuts=0 pan_budget=1 roll_control=1\n";
  return 0;
}
