#include "stellar_feature_diagnostics_v064.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

#include "stellar_gpu_scene_format_v052.h"
#include "stellar_feature_landmarks_v066.h"

namespace {

bool fail(std::string *error, const std::string &message)
{
  if(error)
    *error = message;
  return false;
}

template <typename T>
bool readArray(std::ifstream *stream, std::vector<T> *values, uint64_t count)
{
  const uint64_t maximum_count = std::min(
      uint64_t(std::numeric_limits<std::size_t>::max() / sizeof(T)),
      uint64_t(std::numeric_limits<std::streamsize>::max() / sizeof(T)));
  if(count > maximum_count)
    return false;
  values->resize(std::size_t(count));
  if(count == 0)
    return true;
  stream->read(reinterpret_cast<char *>(&(*values)[0]),
               std::streamsize(sizeof(T) * count));
  return stream->good();
}

double dot3(const double first[3], const double second[3])
{
  return first[0] * second[0] + first[1] * second[1] +
      first[2] * second[2];
}

bool normalize3(double value[3], double *length)
{
  const double norm = std::sqrt(dot3(value, value));
  if(!(norm > 0.0) || !std::isfinite(norm))
    return false;
  for(int component = 0; component < 3; ++component)
    value[component] /= norm;
  if(length)
    *length = norm;
  return true;
}

struct ProjectionV064 {
  double origin[3];
  double direction[3];
  double pixel_x_dual[3];
  double pixel_y_dual[3];
};

bool inferProjection(const std::vector<ArepoStellarRay> &rays,
                     uint32_t width, uint32_t height,
                     ProjectionV064 *projection)
{
  if(!projection || width < 2 || height < 2 ||
     rays.size() != uint64_t(width) * uint64_t(height))
    return false;
  uint64_t active_count = 0;
  double mean_x = 0.0;
  double mean_y = 0.0;
  double mean_origin[3] = {0.0, 0.0, 0.0};
  double direction_sum[3] = {0.0, 0.0, 0.0};
  double reference_direction[3] = {0.0, 0.0, 0.0};
  bool has_reference_direction = false;
  for(std::size_t index = 0; index < rays.size(); ++index)
    if(rays[index].active) {
      double direction[3] = {rays[index].direction[0],
                             rays[index].direction[1],
                             rays[index].direction[2]};
      if(!normalize3(direction, 0))
        return false;
      if(!has_reference_direction) {
        for(int component = 0; component < 3; ++component)
          reference_direction[component] = direction[component];
        has_reference_direction = true;
      }
      if(dot3(direction, reference_direction) < 0.0)
        for(int component = 0; component < 3; ++component)
          direction[component] = -direction[component];
      const uint32_t x = uint32_t(uint64_t(index) % width);
      const uint32_t y = uint32_t(uint64_t(index) / width);
      ++active_count;
      mean_x += x;
      mean_y += y;
      for(int component = 0; component < 3; ++component) {
        mean_origin[component] += rays[index].origin[component];
        direction_sum[component] += direction[component];
      }
    }
  if(active_count < 4)
    return false;
  mean_x /= active_count;
  mean_y /= active_count;
  for(int component = 0; component < 3; ++component) {
    mean_origin[component] /= active_count;
    projection->direction[component] = direction_sum[component];
  }
  if(!normalize3(projection->direction, 0))
    return false;

  double covariance_xx = 0.0;
  double covariance_xy = 0.0;
  double covariance_yy = 0.0;
  double origin_covariance_x[3] = {0.0, 0.0, 0.0};
  double origin_covariance_y[3] = {0.0, 0.0, 0.0};
  for(std::size_t index = 0; index < rays.size(); ++index) {
    if(!rays[index].active)
      continue;
    const double centered_x = double(uint64_t(index) % width) - mean_x;
    const double centered_y = double(uint64_t(index) / width) - mean_y;
    covariance_xx += centered_x * centered_x;
    covariance_xy += centered_x * centered_y;
    covariance_yy += centered_y * centered_y;
    for(int component = 0; component < 3; ++component) {
      const double centered_origin =
          rays[index].origin[component] - mean_origin[component];
      origin_covariance_x[component] += centered_x * centered_origin;
      origin_covariance_y[component] += centered_y * centered_origin;
    }
  }
  const double covariance_determinant =
      covariance_xx * covariance_yy - covariance_xy * covariance_xy;
  if(!(covariance_determinant > 0.0) ||
     !std::isfinite(covariance_determinant))
    return false;

  double fitted_x[3];
  double fitted_y[3];
  for(int component = 0; component < 3; ++component) {
    fitted_x[component] =
        (origin_covariance_x[component] * covariance_yy -
         origin_covariance_y[component] * covariance_xy) /
        covariance_determinant;
    fitted_y[component] =
        (origin_covariance_y[component] * covariance_xx -
         origin_covariance_x[component] * covariance_xy) /
        covariance_determinant;
    projection->origin[component] = mean_origin[component] -
        mean_x * fitted_x[component] - mean_y * fitted_y[component];
  }

  double residual_squared = 0.0;
  double maximum_residual = 0.0;
  for(std::size_t index = 0; index < rays.size(); ++index) {
    if(!rays[index].active)
      continue;
    const double x = uint64_t(index) % width;
    const double y = uint64_t(index) / width;
    double squared = 0.0;
    for(int component = 0; component < 3; ++component) {
      const double predicted = projection->origin[component] +
          x * fitted_x[component] + y * fitted_y[component];
      const double residual = rays[index].origin[component] - predicted;
      squared += residual * residual;
    }
    residual_squared += squared;
    maximum_residual = std::max(maximum_residual, std::sqrt(squared));
  }

  const double fitted_x_direction = dot3(fitted_x, projection->direction);
  const double fitted_y_direction = dot3(fitted_y, projection->direction);
  for(int component = 0; component < 3; ++component) {
    fitted_x[component] -= fitted_x_direction *
        projection->direction[component];
    fitted_y[component] -= fitted_y_direction *
        projection->direction[component];
  }
  const double fitted_x_squared = dot3(fitted_x, fitted_x);
  const double fitted_y_squared = dot3(fitted_y, fitted_y);
  const double fitted_xy = dot3(fitted_x, fitted_y);
  const double grid_determinant =
      fitted_x_squared * fitted_y_squared - fitted_xy * fitted_xy;
  const double minimum_step = std::sqrt(
      std::min(fitted_x_squared, fitted_y_squared));
  const double rms_residual = std::sqrt(residual_squared / active_count);
  if(!(fitted_x_squared > 0.0) || !(fitted_y_squared > 0.0) ||
     !(grid_determinant > 0.0) || !(minimum_step > 0.0) ||
     std::fabs(fitted_xy) /
         std::sqrt(fitted_x_squared * fitted_y_squared) > 0.01 ||
     rms_residual > 0.02 * minimum_step ||
     maximum_residual > 0.10 * minimum_step)
    return false;
  for(int component = 0; component < 3; ++component) {
    projection->pixel_x_dual[component] =
        (fitted_y_squared * fitted_x[component] -
         fitted_xy * fitted_y[component]) / grid_determinant;
    projection->pixel_y_dual[component] =
        (fitted_x_squared * fitted_y[component] -
         fitted_xy * fitted_x[component]) / grid_determinant;
  }

  for(std::size_t index = 0; index < rays.size(); ++index) {
    if(!rays[index].active)
      continue;
    double direction[3] = {rays[index].direction[0], rays[index].direction[1],
                           rays[index].direction[2]};
    if(!normalize3(direction, 0) ||
       dot3(direction, projection->direction) < 1.0 - 1.0e-8)
      return false;
  }
  return true;
}

double weightedMedian(std::vector<StellarWeightedValueV066> values,
                      double total_weight)
{
  return stellarWeightedQuantileV066(values, total_weight, 0.5);
}

float featureWeight(const StellarFeatureSampleV064 &sample, int feature)
{
  if(feature == 0)
    return sample.merger_weight;
  if(feature == 1)
    return sample.disk_weight;
  if(feature == 2)
    return sample.polar_weight;
  if(feature == 3)
    return sample.signed_height_cm >= 0.0f ? sample.polar_weight : 0.0f;
  return sample.signed_height_cm < 0.0f ? sample.polar_weight : 0.0f;
}

const char *featureName(int feature)
{
  const char *names[5] = {"merger", "disk", "polar",
                          "polar_positive", "polar_negative"};
  return names[feature];
}

} // namespace

bool stellarProbeSceneV064(
    const std::string &scene_path,
    const StellarTransferParameters &parameters,
    const std::vector<float> &minimum_weights,
    StellarFeatureProbeSummaryV064 *summary,
    std::string *error)
{
  if(!summary || minimum_weights.empty())
    return fail(error, "invalid output or empty weight threshold list");
  if(!stellarFeatureProfileValidV065(parameters.feature_profile) ||
     !(parameters.box_size > 0.0) ||
     !std::isfinite(parameters.box_size) ||
     !(parameters.material_radius_cm > 0.0f) ||
     !std::isfinite(parameters.material_radius_cm) ||
     !(parameters.disk_radius_cm > 0.0f) ||
     !std::isfinite(parameters.disk_radius_cm) ||
     !(parameters.disk_half_thickness_cm > 0.0f) ||
     !std::isfinite(parameters.disk_half_thickness_cm) ||
     !(parameters.polar_inner_cm > 0.0f) ||
     !std::isfinite(parameters.polar_inner_cm) ||
     !(parameters.polar_outer_cm > parameters.polar_inner_cm) ||
     !std::isfinite(parameters.polar_outer_cm) ||
     !(parameters.polar_cone_ratio > 0.0f) ||
     !std::isfinite(parameters.polar_cone_ratio))
    return fail(error, "invalid stellar transfer geometry");
  for(std::size_t index = 0; index < minimum_weights.size(); ++index)
    if(!std::isfinite(minimum_weights[index]) ||
       minimum_weights[index] < 0.0f || minimum_weights[index] > 1.0f)
      return fail(error, "weight thresholds must be finite and in [0,1]");

  std::ifstream input(scene_path.c_str(), std::ios::binary);
  if(!input.good())
    return fail(error, "cannot open scene: " + scene_path);
  ArepoStellarSceneHeader header = {};
  input.read(reinterpret_cast<char *>(&header), sizeof(header));
  if(!input.good() ||
     std::memcmp(header.magic, AREPO_STELLAR_SCENE_MAGIC,
                 std::strlen(AREPO_STELLAR_SCENE_MAGIC)) != 0 ||
     header.version != AREPO_STELLAR_SCENE_VERSION ||
     header.endian_marker != AREPO_STELLAR_SCENE_ENDIAN_MARKER ||
     header.header_bytes != sizeof(header) ||
     header.cell_bytes != sizeof(ArepoStellarCell) ||
     header.edge_bytes != sizeof(ArepoStellarEdge) ||
     header.ray_bytes != sizeof(ArepoStellarRay) ||
     (header.flags & AREPO_STELLAR_REQUIRED_FIELD_FLAGS) !=
         AREPO_STELLAR_REQUIRED_FIELD_FLAGS ||
     (header.flags & AREPO_STELLAR_RAYS_ONLY) != 0 ||
     header.num_cells == 0 || header.num_rays == 0 ||
     header.sample_width < 2 || header.sample_height < 2 ||
     header.num_rays !=
         uint64_t(header.sample_width) * uint64_t(header.sample_height) ||
     !(header.box_size > 0.0) || !std::isfinite(header.box_size) ||
     header.position_unit_cm != 1.0 ||
     header.density_unit_cgs != 1.0 ||
     header.velocity_unit_cm_per_s != 1.0 ||
     header.temperature_unit_kelvin != 1.0 ||
     std::fabs(header.box_size - parameters.box_size) >
         1.0e-12 * std::max(header.box_size, parameters.box_size))
    return fail(error, "unsupported or invalid packed stellar scene");

  std::vector<ArepoStellarCell> cells;
  if(!readArray(&input, &cells, header.num_cells))
    return fail(error, "truncated stellar cell array");
  const uint64_t skipped_bytes =
      (header.num_cells + 1u) * sizeof(uint64_t) +
      header.num_edges * sizeof(ArepoStellarEdge);
  if(skipped_bytes > uint64_t(std::numeric_limits<std::streamoff>::max()))
    return fail(error, "scene offset exceeds stream range");
  input.seekg(std::streamoff(skipped_bytes), std::ios::cur);
  if(!input.good())
    return fail(error, "truncated stellar connectivity arrays");
  std::vector<ArepoStellarRay> rays;
  if(!readArray(&input, &rays, header.num_rays))
    return fail(error, "truncated stellar ray array");

  ProjectionV064 projection = {};
  if(!inferProjection(rays, header.sample_width, header.sample_height,
                      &projection))
    return fail(error, "scene rays are not a supported orthographic grid");

  std::vector<StellarFeatureSampleV064> features(cells.size());
  for(std::size_t index = 0; index < cells.size(); ++index) {
    for(int component = 0; component < 3; ++component)
      if(!std::isfinite(cells[index].position[component]) ||
         !std::isfinite(cells[index].velocity_cm_per_s[component]))
        return fail(error, "non-finite stellar cell position or velocity");
    if(!std::isfinite(cells[index].density_log10_plus_10) ||
       !std::isfinite(cells[index].temperature_kelvin))
      return fail(error, "non-finite stellar cell thermodynamic field");
    features[index] = evaluateStellarFeatureSampleV064(
        parameters, cells[index].position,
        cells[index].density_log10_plus_10,
        cells[index].temperature_kelvin,
        cells[index].velocity_cm_per_s);
  }

  summary->cells = header.num_cells;
  summary->rays = header.num_rays;
  summary->sample_width = header.sample_width;
  summary->sample_height = header.sample_height;
  summary->orthographic_projection = true;
  summary->rows.clear();
  for(int feature_index = 0; feature_index < 5; ++feature_index) {
    for(std::size_t threshold_index = 0;
        threshold_index < minimum_weights.size(); ++threshold_index) {
      StellarFeatureProbeRowV064 row = {};
      row.feature = featureName(feature_index);
      row.minimum_weight = minimum_weights[threshold_index];
      double merger_overlap = 0.0;
      double high_density = 0.0;
      double inner_radius = 0.0;
      double min_x = std::numeric_limits<double>::infinity();
      double max_x = -std::numeric_limits<double>::infinity();
      double min_y = std::numeric_limits<double>::infinity();
      double max_y = -std::numeric_limits<double>::infinity();
      std::vector<StellarWeightedValueV066> density_values;
      std::vector<StellarWeightedValueV066> radius_values;
      std::vector<StellarWeightedValueV066> height_values;
      std::vector<StellarWeightedValueV066> polar_height_values;
      std::vector<StellarWeightedValueV066> rotation_values;
      std::vector<StellarWeightedValueV066> screen_abs_x_values;
      std::vector<StellarWeightedValueV066> screen_abs_y_values;
      std::vector<StellarWeightedValueV066> screen_half_extent_values;
      double weighted_screen_x = 0.0;
      double weighted_screen_y = 0.0;
      double weighted_signed_height = 0.0;
      for(std::size_t cell_index = 0; cell_index < cells.size(); ++cell_index) {
        const StellarFeatureSampleV064 &feature = features[cell_index];
        const float weight = featureWeight(feature, feature_index);
        if(!(weight >= row.minimum_weight) || !(weight > 0.0f))
          continue;
        ++row.selected_cells;
        row.selected_weight += weight;
        merger_overlap += weight * feature.merger_weight;
        if(feature.log_density >= 2.0f)
          high_density += weight;
        if(feature.cylindrical_radius_cm < 0.35f * parameters.disk_radius_cm)
          inner_radius += weight;
        density_values.push_back({feature.log_density, weight});
        radius_values.push_back({
            feature.cylindrical_radius_cm / parameters.disk_radius_cm, weight});
        height_values.push_back({
            feature.absolute_height_cm / parameters.disk_half_thickness_cm,
            weight});
        polar_height_values.push_back({
            feature.absolute_height_cm / parameters.polar_outer_cm, weight});
        rotation_values.push_back({feature.rotational_fraction, weight});

        double delta[3];
        for(int component = 0; component < 3; ++component)
          delta[component] = cells[cell_index].position[component] -
              projection.origin[component];
        const double pixel_x = dot3(delta, projection.pixel_x_dual);
        const double pixel_y = dot3(delta, projection.pixel_y_dual);
        const double half_width = std::max(
            0.5, 0.5 * double(header.sample_width - 1));
        const double half_height = std::max(
            0.5, 0.5 * double(header.sample_height - 1));
        const double screen_x =
            (pixel_x - 0.5 * double(header.sample_width - 1)) / half_width;
        const double screen_y =
            (pixel_y - 0.5 * double(header.sample_height - 1)) / half_height;
        weighted_screen_x += weight * screen_x;
        weighted_screen_y += weight * screen_y;
        weighted_signed_height += weight * feature.signed_height_cm /
            parameters.polar_outer_cm;
        screen_abs_x_values.push_back({std::fabs(screen_x), weight});
        screen_abs_y_values.push_back({std::fabs(screen_y), weight});
        screen_half_extent_values.push_back({
            std::max(std::fabs(screen_x), std::fabs(screen_y)), weight});
        min_x = std::min(min_x, pixel_x);
        max_x = std::max(max_x, pixel_x);
        min_y = std::min(min_y, pixel_y);
        max_y = std::max(max_y, pixel_y);
      }
      if(row.selected_cells > 0 && row.selected_weight > 0.0) {
        row.weighted_merger_overlap = merger_overlap / row.selected_weight;
        row.weighted_high_density_fraction = high_density / row.selected_weight;
        row.weighted_inner_radius_fraction = inner_radius / row.selected_weight;
        row.weighted_median_log_density =
            weightedMedian(density_values, row.selected_weight);
        row.weighted_median_radius_over_disk_radius =
            weightedMedian(radius_values, row.selected_weight);
        row.weighted_median_absolute_height_over_disk_thickness =
            weightedMedian(height_values, row.selected_weight);
        row.weighted_median_rotational_fraction =
            weightedMedian(rotation_values, row.selected_weight);
        row.weighted_screen_center_x_fraction =
            weighted_screen_x / row.selected_weight;
        row.weighted_screen_center_y_fraction =
            weighted_screen_y / row.selected_weight;
        row.weighted_screen_abs_x_q90 = stellarWeightedQuantileV066(
            screen_abs_x_values, row.selected_weight, 0.90);
        row.weighted_screen_abs_x_q95 = stellarWeightedQuantileV066(
            screen_abs_x_values, row.selected_weight, 0.95);
        row.weighted_screen_abs_x_q99 = stellarWeightedQuantileV066(
            screen_abs_x_values, row.selected_weight, 0.99);
        row.weighted_screen_abs_y_q90 = stellarWeightedQuantileV066(
            screen_abs_y_values, row.selected_weight, 0.90);
        row.weighted_screen_abs_y_q95 = stellarWeightedQuantileV066(
            screen_abs_y_values, row.selected_weight, 0.95);
        row.weighted_screen_abs_y_q99 = stellarWeightedQuantileV066(
            screen_abs_y_values, row.selected_weight, 0.99);
        row.weighted_screen_half_extent_q90 = stellarWeightedQuantileV066(
            screen_half_extent_values, row.selected_weight, 0.90);
        row.weighted_screen_half_extent_q95 = stellarWeightedQuantileV066(
            screen_half_extent_values, row.selected_weight, 0.95);
        row.weighted_screen_half_extent_q99 = stellarWeightedQuantileV066(
            screen_half_extent_values, row.selected_weight, 0.99);
        row.weighted_radius_over_disk_radius_q90 =
            stellarWeightedQuantileV066(
                radius_values, row.selected_weight, 0.90);
        row.weighted_radius_over_disk_radius_q95 =
            stellarWeightedQuantileV066(
                radius_values, row.selected_weight, 0.95);
        row.weighted_radius_over_disk_radius_q99 =
            stellarWeightedQuantileV066(
                radius_values, row.selected_weight, 0.99);
        row.weighted_absolute_height_over_polar_outer_q90 =
            stellarWeightedQuantileV066(
                polar_height_values, row.selected_weight, 0.90);
        row.weighted_absolute_height_over_polar_outer_q95 =
            stellarWeightedQuantileV066(
                polar_height_values, row.selected_weight, 0.95);
        row.weighted_absolute_height_over_polar_outer_q99 =
            stellarWeightedQuantileV066(
                polar_height_values, row.selected_weight, 0.99);
        row.weighted_signed_height_center_over_polar_outer =
            weighted_signed_height / row.selected_weight;
        row.projected_width_fraction = (max_x - min_x) /
            std::max(1.0, double(header.sample_width - 1));
        row.projected_height_fraction = (max_y - min_y) /
            std::max(1.0, double(header.sample_height - 1));
        row.projected_max_fraction = std::max(
            row.projected_width_fraction, row.projected_height_fraction);
        row.touches_left = min_x <= 0.5;
        row.touches_right = max_x >= double(header.sample_width) - 1.5;
        row.touches_bottom = min_y <= 0.5;
        row.touches_top = max_y >= double(header.sample_height) - 1.5;
        row.right_censored = row.touches_left || row.touches_right ||
            row.touches_bottom || row.touches_top;
      } else {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        row.weighted_merger_overlap = nan;
        row.weighted_high_density_fraction = nan;
        row.weighted_inner_radius_fraction = nan;
        row.weighted_median_log_density = nan;
        row.weighted_median_radius_over_disk_radius = nan;
        row.weighted_median_absolute_height_over_disk_thickness = nan;
        row.weighted_median_rotational_fraction = nan;
        row.weighted_screen_center_x_fraction = nan;
        row.weighted_screen_center_y_fraction = nan;
        row.weighted_screen_abs_x_q90 = nan;
        row.weighted_screen_abs_x_q95 = nan;
        row.weighted_screen_abs_x_q99 = nan;
        row.weighted_screen_abs_y_q90 = nan;
        row.weighted_screen_abs_y_q95 = nan;
        row.weighted_screen_abs_y_q99 = nan;
        row.weighted_screen_half_extent_q90 = nan;
        row.weighted_screen_half_extent_q95 = nan;
        row.weighted_screen_half_extent_q99 = nan;
        row.weighted_radius_over_disk_radius_q90 = nan;
        row.weighted_radius_over_disk_radius_q95 = nan;
        row.weighted_radius_over_disk_radius_q99 = nan;
        row.weighted_absolute_height_over_polar_outer_q90 = nan;
        row.weighted_absolute_height_over_polar_outer_q95 = nan;
        row.weighted_absolute_height_over_polar_outer_q99 = nan;
        row.weighted_signed_height_center_over_polar_outer = nan;
        row.projected_width_fraction = nan;
        row.projected_height_fraction = nan;
        row.projected_max_fraction = nan;
      }
      summary->rows.push_back(row);
    }
  }
  return true;
}

bool stellarWriteFeatureProbeV064(
    const std::string &output_path,
    const std::string &scene_path,
    const StellarTransferParameters &parameters,
    const StellarFeatureProbeSummaryV064 &summary,
    std::string *error)
{
  std::ifstream existing(output_path.c_str());
  if(existing.good())
    return fail(error, "refusing to overwrite output: " + output_path);
  std::ofstream output(output_path.c_str());
  if(!output.good())
    return fail(error, "cannot create output: " + output_path);
  output << std::setprecision(17)
         << "# schema=stellar_feature_probe_v064\n"
         << "# scene=" << scene_path << "\n"
         << "# cells=" << summary.cells << "\n"
         << "# rays=" << summary.rays << "\n"
         << "# sample_width=" << summary.sample_width << "\n"
         << "# sample_height=" << summary.sample_height << "\n"
         << "# orthographic_projection="
         << (summary.orthographic_projection ? "true" : "false") << "\n"
         << "# feature_profile="
         << stellarFeatureProfileNameV065(parameters.feature_profile) << "\n"
         << "# disk_radius_cm=" << parameters.disk_radius_cm << "\n"
         << "# disk_half_thickness_cm="
         << parameters.disk_half_thickness_cm << "\n"
         << "feature\tminimum_weight\tselected_cells\tselected_weight"
         << "\tweighted_merger_overlap\tweighted_high_density_fraction"
         << "\tweighted_inner_radius_fraction"
         << "\tweighted_median_log_density"
         << "\tweighted_median_radius_over_disk_radius"
         << "\tweighted_median_absolute_height_over_disk_thickness"
         << "\tweighted_median_rotational_fraction"
         << "\tprojected_width_fraction\tprojected_height_fraction"
         << "\tprojected_max_fraction\ttouches_left\ttouches_right"
         << "\ttouches_bottom\ttouches_top\tright_censored\n";
  for(std::size_t index = 0; index < summary.rows.size(); ++index) {
    const StellarFeatureProbeRowV064 &row = summary.rows[index];
    output << row.feature << '\t' << row.minimum_weight << '\t'
           << row.selected_cells << '\t' << row.selected_weight << '\t'
           << row.weighted_merger_overlap << '\t'
           << row.weighted_high_density_fraction << '\t'
           << row.weighted_inner_radius_fraction << '\t'
           << row.weighted_median_log_density << '\t'
           << row.weighted_median_radius_over_disk_radius << '\t'
           << row.weighted_median_absolute_height_over_disk_thickness << '\t'
           << row.weighted_median_rotational_fraction << '\t'
           << row.projected_width_fraction << '\t'
           << row.projected_height_fraction << '\t'
           << row.projected_max_fraction << '\t'
           << (row.touches_left ? 1 : 0) << '\t'
           << (row.touches_right ? 1 : 0) << '\t'
           << (row.touches_bottom ? 1 : 0) << '\t'
           << (row.touches_top ? 1 : 0) << '\t'
           << (row.right_censored ? 1 : 0) << '\n';
  }
  output.close();
  if(!output.good())
    return fail(error, "failed while closing output: " + output_path);
  return true;
}
