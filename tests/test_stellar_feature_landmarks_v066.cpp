#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "stellar_feature_landmarks_v066.h"

int main(int argc, char **argv)
{
  assert(argc == 2);
  std::vector<StellarWeightedValueV066> values;
  for(int index = 0; index < 100; ++index)
    values.push_back({0.4, 1.0});
  values.push_back({4.0, 0.5});
  assert(stellarWeightedQuantileV066(values, 100.5, 0.90) == 0.4);
  assert(stellarWeightedQuantileV066(values, 100.5, 0.95) == 0.4);
  assert(stellarWeightedQuantileV066(values, 100.5, 0.99) == 0.4);
  assert(stellarWeightedQuantileV066(values, 100.5, 1.0) == 4.0);
  assert(std::isnan(stellarWeightedQuantileV066(
      std::vector<StellarWeightedValueV066>(), 0.0, 0.90)));
  assert(std::isnan(stellarWeightedQuantileV066(values, 100.5, 1.1)));

  StellarFeatureProbeRowV064 row = {};
  row.feature = "polar_positive";
  row.minimum_weight = 0.1f;
  row.selected_cells = 101;
  row.selected_weight = 100.5;
  row.weighted_screen_center_x_fraction = 0.02;
  row.weighted_screen_center_y_fraction = 0.35;
  row.weighted_screen_abs_x_q90 = 0.30;
  row.weighted_screen_abs_x_q95 = 0.34;
  row.weighted_screen_abs_x_q99 = 0.40;
  row.weighted_screen_abs_y_q90 = 0.50;
  row.weighted_screen_abs_y_q95 = 0.60;
  row.weighted_screen_abs_y_q99 = 0.70;
  row.weighted_screen_half_extent_q90 = 0.50;
  row.weighted_screen_half_extent_q95 = 0.60;
  row.weighted_screen_half_extent_q99 = 0.70;
  row.weighted_radius_over_disk_radius_q90 = 1.2;
  row.weighted_radius_over_disk_radius_q95 = 1.4;
  row.weighted_radius_over_disk_radius_q99 = 1.8;
  row.weighted_absolute_height_over_polar_outer_q90 = 0.65;
  row.weighted_absolute_height_over_polar_outer_q95 = 0.75;
  row.weighted_absolute_height_over_polar_outer_q99 = 0.90;
  row.weighted_signed_height_center_over_polar_outer = 0.55;
  row.projected_max_fraction = 3.2;
  row.right_censored = true;

  StellarFeatureProbeSummaryV064 summary = {};
  summary.cells = 101;
  summary.rays = 16;
  summary.sample_width = 4;
  summary.sample_height = 4;
  summary.orthographic_projection = true;
  summary.rows.push_back(row);
  StellarTransferParameters parameters = {};
  parameters.feature_profile = STELLAR_FEATURE_STRUCTURES_V065;
  const std::string output =
      std::string(argv[1]) + "/landmark_fixture_v066.tsv";
  std::string error;
  assert(stellarWriteFeatureLandmarksV066(
      output, "fixture_scene.bin", parameters, summary, &error));
  assert(!stellarWriteFeatureLandmarksV066(
      output, "fixture_scene.bin", parameters, summary, &error));
  assert(error.find("refusing to overwrite") != std::string::npos);

  std::cout << "STELLAR_FEATURE_LANDMARKS_V066_OK"
            << " q99=" << stellarWeightedQuantileV066(values, 100.5, 0.99)
            << " raw_outlier=" << values.back().value << '\n';
  return 0;
}
