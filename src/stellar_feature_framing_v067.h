#ifndef AREPO_VTK_STELLAR_FEATURE_FRAMING_V067_H
#define AREPO_VTK_STELLAR_FEATURE_FRAMING_V067_H

#include <stdint.h>

#include <string>
#include <vector>

#include "stellar_feature_diagnostics_v064.h"

struct StellarFeatureFramingRequestV067 {
  std::string mode;
  std::vector<std::string> features;
  double minimum_weight;
  double coverage;
  double target_half_width_fraction;
  double target_half_height_fraction;
};

struct StellarFeatureFramingMemberV067 {
  std::string feature;
  uint64_t selected_cells;
  double selected_weight;
  double x_low;
  double x_high;
  double y_low;
  double y_high;
  bool raw_right_censored;
};

struct StellarFeatureFramingResultV067 {
  double lower_quantile;
  double upper_quantile;
  double combined_x_low;
  double combined_x_high;
  double combined_y_low;
  double combined_y_high;
  double robust_center_x_fraction;
  double robust_center_y_fraction;
  double robust_half_width_fraction;
  double robust_half_height_fraction;
  double half_extent_scale_factor;
  uint64_t raw_right_censored_members;
  std::vector<StellarFeatureFramingMemberV067> members;
};

bool stellarAssessFeatureFramingV067(
    const StellarFeatureProbeSummaryV064 &summary,
    const StellarFeatureFramingRequestV067 &request,
    StellarFeatureFramingResultV067 *result,
    std::string *error);

bool stellarWriteFeatureFramingPlanV067(
    const std::string &output_path,
    const std::string &scene_path,
    const StellarTransferParameters &parameters,
    const StellarFeatureFramingRequestV067 &request,
    const StellarFeatureFramingResultV067 &result,
    std::string *error);

#endif
