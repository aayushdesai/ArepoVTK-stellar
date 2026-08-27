#include "stellar_feature_framing_v067.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>

namespace {

bool fail(std::string *error, const std::string &message)
{
  if(error)
    *error = message;
  return false;
}

bool finiteBounds(
    double x_low, double x_high, double y_low, double y_high)
{
  return std::isfinite(x_low) && std::isfinite(x_high) &&
      std::isfinite(y_low) && std::isfinite(y_high) &&
      x_low <= x_high && y_low <= y_high;
}

bool thresholdMatches(float row_value, double requested)
{
  return std::fabs(double(row_value) - requested) <=
      1.0e-6 * std::max(1.0, std::fabs(requested));
}

bool coverageMatches(double value, double expected)
{
  return std::fabs(value - expected) <= 1.0e-12;
}

} // namespace

bool stellarAssessFeatureFramingV067(
    const StellarFeatureProbeSummaryV064 &summary,
    const StellarFeatureFramingRequestV067 &request,
    StellarFeatureFramingResultV067 *result,
    std::string *error)
{
  if(!result)
    return fail(error, "missing framing result");
  if(request.mode != "single" && request.mode != "bipolar_union")
    return fail(error, "invalid framing mode");
  if((request.mode == "single" && request.features.size() != 1) ||
     (request.mode == "bipolar_union" && request.features.size() != 2))
    return fail(error, "framing mode has invalid feature count");
  std::set<std::string> unique_features(
      request.features.begin(), request.features.end());
  if(unique_features.size() != request.features.size() ||
     unique_features.count("") != 0)
    return fail(error, "framing features must be unique and nonempty");
  if(!std::isfinite(request.minimum_weight) ||
     request.minimum_weight < 0.0 || request.minimum_weight > 1.0)
    return fail(error, "invalid framing minimum weight");
  const bool coverage_90 = coverageMatches(request.coverage, 0.90);
  const bool coverage_98 = coverageMatches(request.coverage, 0.98);
  if(!coverage_90 && !coverage_98)
    return fail(error, "framing coverage must be 0.90 or 0.98");
  if(!std::isfinite(request.target_half_width_fraction) ||
     !std::isfinite(request.target_half_height_fraction) ||
     !(request.target_half_width_fraction > 0.0) ||
     !(request.target_half_height_fraction > 0.0) ||
     request.target_half_width_fraction > 1.0 ||
     request.target_half_height_fraction > 1.0)
    return fail(error, "invalid framing target fraction");

  StellarFeatureFramingResultV067 candidate = {};
  candidate.lower_quantile = coverage_90 ? 0.05 : 0.01;
  candidate.upper_quantile = coverage_90 ? 0.95 : 0.99;
  candidate.combined_x_low = std::numeric_limits<double>::infinity();
  candidate.combined_x_high = -std::numeric_limits<double>::infinity();
  candidate.combined_y_low = std::numeric_limits<double>::infinity();
  candidate.combined_y_high = -std::numeric_limits<double>::infinity();

  for(std::size_t feature_index = 0;
      feature_index < request.features.size(); ++feature_index) {
    const StellarFeatureProbeRowV064 *matched = 0;
    for(std::size_t row_index = 0; row_index < summary.rows.size(); ++row_index) {
      const StellarFeatureProbeRowV064 &row = summary.rows[row_index];
      if(row.feature == request.features[feature_index] &&
         thresholdMatches(row.minimum_weight, request.minimum_weight)) {
        if(matched)
          return fail(error, "duplicate framing feature row");
        matched = &row;
      }
    }
    if(!matched)
      return fail(error, "missing framing feature row");
    if(matched->selected_cells == 0 || !(matched->selected_weight > 0.0))
      return fail(error, "framing feature is absent at requested threshold");

    StellarFeatureFramingMemberV067 member = {};
    member.feature = matched->feature;
    member.selected_cells = matched->selected_cells;
    member.selected_weight = matched->selected_weight;
    if(coverage_90) {
      member.x_low = matched->weighted_screen_x_q05;
      member.x_high = matched->weighted_screen_x_q95;
      member.y_low = matched->weighted_screen_y_q05;
      member.y_high = matched->weighted_screen_y_q95;
    } else {
      member.x_low = matched->weighted_screen_x_q01;
      member.x_high = matched->weighted_screen_x_q99;
      member.y_low = matched->weighted_screen_y_q01;
      member.y_high = matched->weighted_screen_y_q99;
    }
    if(!finiteBounds(
           member.x_low, member.x_high, member.y_low, member.y_high))
      return fail(error, "framing feature has invalid signed bounds");
    member.raw_right_censored = matched->right_censored;
    candidate.combined_x_low =
        std::min(candidate.combined_x_low, member.x_low);
    candidate.combined_x_high =
        std::max(candidate.combined_x_high, member.x_high);
    candidate.combined_y_low =
        std::min(candidate.combined_y_low, member.y_low);
    candidate.combined_y_high =
        std::max(candidate.combined_y_high, member.y_high);
    if(member.raw_right_censored)
      ++candidate.raw_right_censored_members;
    candidate.members.push_back(member);
  }

  if(!finiteBounds(
         candidate.combined_x_low, candidate.combined_x_high,
         candidate.combined_y_low, candidate.combined_y_high))
    return fail(error, "combined framing bounds are invalid");
  candidate.robust_center_x_fraction =
      0.5 * (candidate.combined_x_low + candidate.combined_x_high);
  candidate.robust_center_y_fraction =
      0.5 * (candidate.combined_y_low + candidate.combined_y_high);
  candidate.robust_half_width_fraction =
      0.5 * (candidate.combined_x_high - candidate.combined_x_low);
  candidate.robust_half_height_fraction =
      0.5 * (candidate.combined_y_high - candidate.combined_y_low);
  candidate.half_extent_scale_factor = std::max(
      candidate.robust_half_width_fraction /
          request.target_half_width_fraction,
      candidate.robust_half_height_fraction /
          request.target_half_height_fraction);
  if(!std::isfinite(candidate.half_extent_scale_factor) ||
     !(candidate.half_extent_scale_factor > 0.0))
    return fail(error, "invalid framing scale factor");
  *result = candidate;
  return true;
}

bool stellarWriteFeatureFramingPlanV067(
    const std::string &output_path,
    const std::string &scene_path,
    const StellarTransferParameters &parameters,
    const StellarFeatureFramingRequestV067 &request,
    const StellarFeatureFramingResultV067 &result,
    std::string *error)
{
  std::ifstream existing(output_path.c_str());
  if(existing.good())
    return fail(error, "refusing to overwrite framing output: " + output_path);
  std::ofstream output(output_path.c_str());
  if(!output.good())
    return fail(error, "cannot create framing output: " + output_path);
  output << std::setprecision(17)
         << "# schema=stellar_feature_framing_plan_v067\n"
         << "# scene=" << scene_path << "\n"
         << "# feature_profile="
         << stellarFeatureProfileNameV065(parameters.feature_profile) << "\n"
         << "# projection_support=all_selected_cell_centers_unclipped\n"
         << "# mode=" << request.mode << "\n"
         << "# minimum_weight=" << request.minimum_weight << "\n"
         << "# coverage=" << request.coverage << "\n"
         << "# lower_quantile=" << result.lower_quantile << "\n"
         << "# upper_quantile=" << result.upper_quantile << "\n"
         << "# target_half_width_fraction="
         << request.target_half_width_fraction << "\n"
         << "# target_half_height_fraction="
         << request.target_half_height_fraction << "\n"
         << "# recommendation_scope=diagnostic_only_no_camera_mutation\n"
         << "# look_at_shift_semantics=positive_screen_shift_moves_frame_center_toward_feature\n"
         << "# scale_semantics=new_half_extent_equals_source_half_extent_times_scale\n"
         << "# combined_x_low=" << result.combined_x_low << "\n"
         << "# combined_x_high=" << result.combined_x_high << "\n"
         << "# combined_y_low=" << result.combined_y_low << "\n"
         << "# combined_y_high=" << result.combined_y_high << "\n"
         << "# look_at_shift_screen_x_fraction="
         << result.robust_center_x_fraction << "\n"
         << "# look_at_shift_screen_y_fraction="
         << result.robust_center_y_fraction << "\n"
         << "# robust_half_width_fraction="
         << result.robust_half_width_fraction << "\n"
         << "# robust_half_height_fraction="
         << result.robust_half_height_fraction << "\n"
         << "# half_extent_scale_factor="
         << result.half_extent_scale_factor << "\n"
         << "# raw_right_censored_members="
         << result.raw_right_censored_members << "\n"
         << "feature\tselected_cells\tselected_weight\tx_low\tx_high"
         << "\ty_low\ty_high\traw_right_censored\n";
  for(std::size_t index = 0; index < result.members.size(); ++index) {
    const StellarFeatureFramingMemberV067 &member = result.members[index];
    output << member.feature << '\t' << member.selected_cells << '\t'
           << member.selected_weight << '\t' << member.x_low << '\t'
           << member.x_high << '\t' << member.y_low << '\t'
           << member.y_high << '\t'
           << (member.raw_right_censored ? 1 : 0) << '\n';
  }
  output.close();
  if(!output.good())
    return fail(error, "failed while closing framing output: " + output_path);
  return true;
}
