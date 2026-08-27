#ifndef AREPO_VTK_STELLAR_FEATURE_DIAGNOSTICS_V064_H
#define AREPO_VTK_STELLAR_FEATURE_DIAGNOSTICS_V064_H

#include <stdint.h>

#include <string>
#include <vector>

#include "stellar_render_model_v052a.h"

struct StellarFeatureProbeRowV064 {
  std::string feature;
  float minimum_weight;
  uint64_t selected_cells;
  double selected_weight;
  double weighted_merger_overlap;
  double weighted_high_density_fraction;
  double weighted_inner_radius_fraction;
  double weighted_median_log_density;
  double weighted_median_radius_over_disk_radius;
  double weighted_median_absolute_height_over_disk_thickness;
  double weighted_median_rotational_fraction;
  double projected_width_fraction;
  double projected_height_fraction;
  double projected_max_fraction;
  bool touches_left;
  bool touches_right;
  bool touches_bottom;
  bool touches_top;
  bool right_censored;
};

struct StellarFeatureProbeSummaryV064 {
  uint64_t cells;
  uint64_t rays;
  uint32_t sample_width;
  uint32_t sample_height;
  bool orthographic_projection;
  std::vector<StellarFeatureProbeRowV064> rows;
};

bool stellarProbeSceneV064(
    const std::string &scene_path,
    const StellarTransferParameters &parameters,
    const std::vector<float> &minimum_weights,
    StellarFeatureProbeSummaryV064 *summary,
    std::string *error);

bool stellarWriteFeatureProbeV064(
    const std::string &output_path,
    const std::string &scene_path,
    const StellarTransferParameters &parameters,
    const StellarFeatureProbeSummaryV064 &summary,
    std::string *error);

#endif
