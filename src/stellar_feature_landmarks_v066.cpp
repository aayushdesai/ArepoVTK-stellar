#include "stellar_feature_landmarks_v066.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>

namespace {

bool fail(std::string *error, const std::string &message)
{
  if(error)
    *error = message;
  return false;
}

} // namespace

double stellarWeightedQuantileV066(
    std::vector<StellarWeightedValueV066> values,
    double total_weight,
    double quantile)
{
  if(values.empty() || !(total_weight > 0.0) ||
     !std::isfinite(total_weight) || !std::isfinite(quantile) ||
     quantile < 0.0 || quantile > 1.0)
    return std::numeric_limits<double>::quiet_NaN();
  std::sort(values.begin(), values.end(),
            [](const StellarWeightedValueV066 &first,
               const StellarWeightedValueV066 &second) {
              return first.value < second.value;
            });
  const double target = quantile * total_weight;
  double cumulative = 0.0;
  for(std::size_t index = 0; index < values.size(); ++index) {
    cumulative += values[index].weight;
    if(cumulative >= target)
      return values[index].value;
  }
  return values.back().value;
}

bool stellarWriteFeatureLandmarksV066(
    const std::string &output_path,
    const std::string &scene_path,
    const StellarTransferParameters &parameters,
    const StellarFeatureProbeSummaryV064 &summary,
    std::string *error)
{
  std::ifstream existing(output_path.c_str());
  if(existing.good())
    return fail(error, "refusing to overwrite landmark output: " + output_path);
  std::ofstream output(output_path.c_str());
  if(!output.good())
    return fail(error, "cannot create landmark output: " + output_path);
  output << std::setprecision(17)
         << "# schema=stellar_feature_landmarks_v066\n"
         << "# scene=" << scene_path << "\n"
         << "# cells=" << summary.cells << "\n"
         << "# rays=" << summary.rays << "\n"
         << "# sample_width=" << summary.sample_width << "\n"
         << "# sample_height=" << summary.sample_height << "\n"
         << "# orthographic_projection="
         << (summary.orthographic_projection ? "true" : "false") << "\n"
         << "# projection_support=all_selected_cell_centers_unclipped\n"
         << "# feature_profile="
         << stellarFeatureProfileNameV065(parameters.feature_profile) << "\n"
         << "feature\tminimum_weight\tselected_cells\tselected_weight"
         << "\tweighted_screen_center_x_fraction"
         << "\tweighted_screen_center_y_fraction"
         << "\tweighted_screen_abs_x_q90\tweighted_screen_abs_x_q95"
         << "\tweighted_screen_abs_x_q99"
         << "\tweighted_screen_abs_y_q90\tweighted_screen_abs_y_q95"
         << "\tweighted_screen_abs_y_q99"
         << "\tweighted_screen_half_extent_q90"
         << "\tweighted_screen_half_extent_q95"
         << "\tweighted_screen_half_extent_q99"
         << "\tweighted_radius_over_disk_radius_q90"
         << "\tweighted_radius_over_disk_radius_q95"
         << "\tweighted_radius_over_disk_radius_q99"
         << "\tweighted_absolute_height_over_polar_outer_q90"
         << "\tweighted_absolute_height_over_polar_outer_q95"
         << "\tweighted_absolute_height_over_polar_outer_q99"
         << "\tweighted_signed_height_center_over_polar_outer"
         << "\traw_projected_max_fraction\traw_right_censored\n";
  for(std::size_t index = 0; index < summary.rows.size(); ++index) {
    const StellarFeatureProbeRowV064 &row = summary.rows[index];
    output << row.feature << '\t' << row.minimum_weight << '\t'
           << row.selected_cells << '\t' << row.selected_weight << '\t'
           << row.weighted_screen_center_x_fraction << '\t'
           << row.weighted_screen_center_y_fraction << '\t'
           << row.weighted_screen_abs_x_q90 << '\t'
           << row.weighted_screen_abs_x_q95 << '\t'
           << row.weighted_screen_abs_x_q99 << '\t'
           << row.weighted_screen_abs_y_q90 << '\t'
           << row.weighted_screen_abs_y_q95 << '\t'
           << row.weighted_screen_abs_y_q99 << '\t'
           << row.weighted_screen_half_extent_q90 << '\t'
           << row.weighted_screen_half_extent_q95 << '\t'
           << row.weighted_screen_half_extent_q99 << '\t'
           << row.weighted_radius_over_disk_radius_q90 << '\t'
           << row.weighted_radius_over_disk_radius_q95 << '\t'
           << row.weighted_radius_over_disk_radius_q99 << '\t'
           << row.weighted_absolute_height_over_polar_outer_q90 << '\t'
           << row.weighted_absolute_height_over_polar_outer_q95 << '\t'
           << row.weighted_absolute_height_over_polar_outer_q99 << '\t'
           << row.weighted_signed_height_center_over_polar_outer << '\t'
           << row.projected_max_fraction << '\t'
           << (row.right_censored ? 1 : 0) << '\n';
  }
  output.close();
  if(!output.good())
    return fail(error, "failed while closing landmark output: " + output_path);
  return true;
}
