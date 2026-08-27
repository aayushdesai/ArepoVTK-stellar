#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

#include "stellar_feature_framing_v067.h"

namespace {

StellarFeatureProbeRowV064 makeRow(
    const std::string &feature,
    float threshold,
    double x01,
    double x05,
    double x95,
    double x99,
    double y01,
    double y05,
    double y95,
    double y99,
    bool censored)
{
  StellarFeatureProbeRowV064 row = {};
  row.feature = feature;
  row.minimum_weight = threshold;
  row.selected_cells = 100;
  row.selected_weight = 75.0;
  row.weighted_screen_x_q01 = x01;
  row.weighted_screen_x_q05 = x05;
  row.weighted_screen_x_q50 = 0.5 * (x05 + x95);
  row.weighted_screen_x_q95 = x95;
  row.weighted_screen_x_q99 = x99;
  row.weighted_screen_y_q01 = y01;
  row.weighted_screen_y_q05 = y05;
  row.weighted_screen_y_q50 = 0.5 * (y05 + y95);
  row.weighted_screen_y_q95 = y95;
  row.weighted_screen_y_q99 = y99;
  row.right_censored = censored;
  return row;
}

bool nearlyEqual(double first, double second)
{
  return std::fabs(first - second) < 1.0e-12;
}

} // namespace

int main(int argc, char **argv)
{
  assert(argc == 2);
  StellarFeatureProbeSummaryV064 summary = {};
  summary.rows.push_back(makeRow(
      "disk", 0.10f, -0.60, -0.50, 0.30, 0.40,
      -0.20, -0.10, 0.70, 0.80, true));
  summary.rows.push_back(makeRow(
      "polar_positive", 0.25f, -0.20, -0.15, 0.15, 0.20,
      -0.80, -0.70, -0.30, -0.20, false));
  summary.rows.push_back(makeRow(
      "polar_negative", 0.25f, -0.30, -0.25, 0.25, 0.30,
      0.10, 0.20, 1.30, 1.50, true));

  StellarFeatureFramingRequestV067 request = {};
  request.mode = "single";
  request.features.push_back("disk");
  request.minimum_weight = 0.10;
  request.coverage = 0.98;
  request.target_half_width_fraction = 0.50;
  request.target_half_height_fraction = 0.50;
  StellarFeatureFramingResultV067 result = {};
  std::string error;
  assert(stellarAssessFeatureFramingV067(summary, request, &result, &error));
  assert(nearlyEqual(result.robust_center_x_fraction, -0.10));
  assert(nearlyEqual(result.robust_center_y_fraction, 0.30));
  assert(nearlyEqual(result.robust_half_width_fraction, 0.50));
  assert(nearlyEqual(result.robust_half_height_fraction, 0.50));
  assert(nearlyEqual(result.half_extent_scale_factor, 1.0));
  assert(result.raw_right_censored_members == 1);

  request.mode = "bipolar_union";
  request.features.clear();
  request.features.push_back("polar_positive");
  request.features.push_back("polar_negative");
  request.minimum_weight = 0.25;
  request.target_half_width_fraction = 0.80;
  request.target_half_height_fraction = 0.80;
  assert(stellarAssessFeatureFramingV067(summary, request, &result, &error));
  assert(nearlyEqual(result.combined_x_low, -0.30));
  assert(nearlyEqual(result.combined_x_high, 0.30));
  assert(nearlyEqual(result.combined_y_low, -0.80));
  assert(nearlyEqual(result.combined_y_high, 1.50));
  assert(nearlyEqual(result.robust_center_x_fraction, 0.0));
  assert(nearlyEqual(result.robust_center_y_fraction, 0.35));
  assert(nearlyEqual(result.robust_half_width_fraction, 0.30));
  assert(nearlyEqual(result.robust_half_height_fraction, 1.15));
  assert(nearlyEqual(result.half_extent_scale_factor, 1.4375));
  assert(result.raw_right_censored_members == 1);

  StellarTransferParameters parameters = {};
  parameters.feature_profile = STELLAR_FEATURE_STRUCTURES_V065;
  const std::string output =
      std::string(argv[1]) + "/framing_plan_v067.tsv";
  assert(stellarWriteFeatureFramingPlanV067(
      output, "scene.bin", parameters, request, result, &error));
  assert(!stellarWriteFeatureFramingPlanV067(
      output, "scene.bin", parameters, request, result, &error));
  assert(error.find("refusing to overwrite") != std::string::npos);

  request.coverage = 0.90;
  assert(stellarAssessFeatureFramingV067(summary, request, &result, &error));
  assert(nearlyEqual(result.combined_y_low, -0.70));
  assert(nearlyEqual(result.combined_y_high, 1.30));
  assert(nearlyEqual(result.half_extent_scale_factor, 1.25));

  StellarFeatureProbeSummaryV064 absent = summary;
  absent.rows[2].selected_cells = 0;
  absent.rows[2].selected_weight = 0.0;
  assert(!stellarAssessFeatureFramingV067(absent, request, &result, &error));
  assert(error.find("absent") != std::string::npos);

  request.coverage = 0.97;
  assert(!stellarAssessFeatureFramingV067(summary, request, &result, &error));
  assert(error.find("coverage") != std::string::npos);

  StellarFeatureProbeSummaryV064 degenerate = summary;
  degenerate.rows[0] = makeRow(
      "disk", 0.10f, 0.20, 0.20, 0.20, 0.20,
      -0.10, -0.10, -0.10, -0.10, false);
  request.mode = "single";
  request.features.clear();
  request.features.push_back("disk");
  request.minimum_weight = 0.10;
  request.coverage = 0.98;
  request.target_half_width_fraction = 0.50;
  request.target_half_height_fraction = 0.50;
  assert(!stellarAssessFeatureFramingV067(
      degenerate, request, &result, &error));
  assert(error.find("scale factor") != std::string::npos);

  std::cout << "STELLAR_FEATURE_FRAMING_V067_OK"
            << " bipolar_center_y=0.35 bipolar_scale=1.4375"
            << " no_camera_mutation=1\n";
  return 0;
}
