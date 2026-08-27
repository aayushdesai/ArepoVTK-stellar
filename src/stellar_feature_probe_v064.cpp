#include "stellar_feature_diagnostics_v064.h"
#include "stellar_feature_framing_v067.h"
#include "stellar_feature_landmarks_v066.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

namespace {

bool parseFinite(const char *text, double *value)
{
  errno = 0;
  char *end = 0;
  const double parsed = std::strtod(text, &end);
  if(errno == ERANGE || end == text || !end || *end != '\0' ||
     !std::isfinite(parsed))
    return false;
  *value = parsed;
  return true;
}

bool parseVector(int argc, char **argv, int *index, double value[3])
{
  if(*index + 3 >= argc)
    return false;
  for(int component = 0; component < 3; ++component)
    if(!parseFinite(argv[++(*index)], &value[component]))
      return false;
  return true;
}

bool parseFloatVector(int argc, char **argv, int *index, float value[3])
{
  double parsed[3];
  if(!parseVector(argc, argv, index, parsed))
    return false;
  for(int component = 0; component < 3; ++component)
    value[component] = float(parsed[component]);
  return true;
}

bool parseThresholds(const std::string &text, std::vector<float> *values)
{
  values->clear();
  std::istringstream stream(text);
  std::string token;
  while(std::getline(stream, token, ',')) {
    double value = 0.0;
    if(!parseFinite(token.c_str(), &value) || value < 0.0 || value > 1.0)
      return false;
    values->push_back(float(value));
  }
  return !values->empty();
}

bool parseFeatures(
    const std::string &text, std::vector<std::string> *features)
{
  features->clear();
  std::istringstream stream(text);
  std::string token;
  while(std::getline(stream, token, ',')) {
    if(token.empty())
      return false;
    features->push_back(token);
  }
  return !features->empty();
}

bool assignPositiveFloat(const char *text, float *value)
{
  double parsed = 0.0;
  if(!parseFinite(text, &parsed) || !(parsed > 0.0))
    return false;
  *value = float(parsed);
  return std::isfinite(*value) && *value > 0.0f;
}

bool pathExists(const std::string &path)
{
  std::ifstream input(path.c_str());
  return input.good();
}

} // namespace

int main(int argc, char **argv)
{
  StellarTransferParameters parameters = {};
  parameters.mode = STELLAR_TRANSFER_COMPOSITE;
  parameters.palette_profile = STELLAR_PALETTE_LEGACY_V052;
  parameters.feature_profile = STELLAR_FEATURE_LEGACY_V064;
  std::string scene_path;
  std::string output_path;
  std::string landmark_output_path;
  std::string framing_output_path;
  StellarFeatureFramingRequestV067 framing_request = {};
  bool has_framing_mode = false;
  bool has_framing_features = false;
  bool has_framing_minimum_weight = false;
  bool has_framing_coverage = false;
  bool has_framing_target_width = false;
  bool has_framing_target_height = false;
  std::vector<float> thresholds = {0.01f, 0.05f, 0.10f, 0.25f, 0.50f};
  bool has_center = false;
  bool has_axis = false;
  bool has_bulk = false;
  for(int index = 1; index < argc; ++index) {
    const std::string option = argv[index];
    if(option == "--scene" && index + 1 < argc)
      scene_path = argv[++index];
    else if(option == "--output" && index + 1 < argc)
      output_path = argv[++index];
    else if(option == "--landmark-output" && index + 1 < argc)
      landmark_output_path = argv[++index];
    else if(option == "--framing-output" && index + 1 < argc)
      framing_output_path = argv[++index];
    else if(option == "--framing-mode" && index + 1 < argc) {
      framing_request.mode = argv[++index];
      has_framing_mode = true;
    } else if(option == "--framing-features" && index + 1 < argc) {
      has_framing_features = parseFeatures(
          argv[++index], &framing_request.features);
      if(!has_framing_features) {
        std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR invalid framing features\n";
        return 2;
      }
    } else if(option == "--framing-minimum-weight" && index + 1 < argc) {
      has_framing_minimum_weight = parseFinite(
          argv[++index], &framing_request.minimum_weight);
      if(!has_framing_minimum_weight) {
        std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR invalid framing minimum weight\n";
        return 2;
      }
    } else if(option == "--framing-coverage" && index + 1 < argc) {
      has_framing_coverage = parseFinite(
          argv[++index], &framing_request.coverage);
      if(!has_framing_coverage) {
        std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR invalid framing coverage\n";
        return 2;
      }
    } else if(option == "--framing-target-half-width" && index + 1 < argc) {
      has_framing_target_width = parseFinite(
          argv[++index], &framing_request.target_half_width_fraction);
      if(!has_framing_target_width) {
        std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR invalid framing target width\n";
        return 2;
      }
    } else if(option == "--framing-target-half-height" && index + 1 < argc) {
      has_framing_target_height = parseFinite(
          argv[++index], &framing_request.target_half_height_fraction);
      if(!has_framing_target_height) {
        std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR invalid framing target height\n";
        return 2;
      }
    }
    else if(option == "--feature-profile" && index + 1 < argc) {
      const std::string profile = argv[++index];
      if(profile == "legacy_v064")
        parameters.feature_profile = STELLAR_FEATURE_LEGACY_V064;
      else if(profile == "stellar_structures_v065")
        parameters.feature_profile = STELLAR_FEATURE_STRUCTURES_V065;
      else {
        std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR invalid feature profile\n";
        return 2;
      }
    }
    else if(option == "--center")
      has_center = parseVector(argc, argv, &index, parameters.center);
    else if(option == "--axis")
      has_axis = parseVector(argc, argv, &index, parameters.axis);
    else if(option == "--bulk")
      has_bulk = parseFloatVector(
          argc, argv, &index, parameters.bulk_velocity_cm_per_s);
    else if(option == "--box-size" && index + 1 < argc) {
      if(!parseFinite(argv[++index], &parameters.box_size) ||
         !(parameters.box_size > 0.0)) {
        std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR invalid box size\n";
        return 2;
      }
    } else if(option == "--material-radius" && index + 1 < argc) {
      if(!assignPositiveFloat(argv[++index], &parameters.material_radius_cm)) {
        std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR invalid material radius\n";
        return 2;
      }
    } else if(option == "--disk-radius" && index + 1 < argc) {
      if(!assignPositiveFloat(argv[++index], &parameters.disk_radius_cm)) {
        std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR invalid disk radius\n";
        return 2;
      }
    } else if(option == "--disk-half-thickness" && index + 1 < argc) {
      if(!assignPositiveFloat(
             argv[++index], &parameters.disk_half_thickness_cm)) {
        std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR invalid disk thickness\n";
        return 2;
      }
    } else if(option == "--polar-inner" && index + 1 < argc) {
      if(!assignPositiveFloat(argv[++index], &parameters.polar_inner_cm)) {
        std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR invalid polar inner extent\n";
        return 2;
      }
    } else if(option == "--polar-outer" && index + 1 < argc) {
      if(!assignPositiveFloat(argv[++index], &parameters.polar_outer_cm)) {
        std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR invalid polar outer extent\n";
        return 2;
      }
    } else if(option == "--polar-cone-ratio" && index + 1 < argc) {
      if(!assignPositiveFloat(argv[++index], &parameters.polar_cone_ratio)) {
        std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR invalid polar cone ratio\n";
        return 2;
      }
    } else if(option == "--minimum-weights" && index + 1 < argc) {
      if(!parseThresholds(argv[++index], &thresholds)) {
        std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR invalid weight thresholds\n";
        return 2;
      }
    } else {
      std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR unknown or incomplete option: "
                << option << '\n';
      return 2;
    }
  }
  const double axis_norm = std::sqrt(
      parameters.axis[0] * parameters.axis[0] +
      parameters.axis[1] * parameters.axis[1] +
      parameters.axis[2] * parameters.axis[2]);
  if(scene_path.empty() || output_path.empty() || !has_center || !has_axis ||
     !has_bulk || !(parameters.box_size > 0.0) ||
     !(parameters.material_radius_cm > 0.0f) ||
     !(parameters.disk_radius_cm > 0.0f) ||
     !(parameters.disk_half_thickness_cm > 0.0f) ||
     !(parameters.polar_inner_cm > 0.0f) ||
     !(parameters.polar_outer_cm > parameters.polar_inner_cm) ||
     !(parameters.polar_cone_ratio > 0.0f) ||
     !stellarFeatureProfileValidV065(parameters.feature_profile) ||
     !(axis_norm > 0.0) || !std::isfinite(axis_norm)) {
    std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR missing or invalid required input\n";
    return 2;
  }
  for(int component = 0; component < 3; ++component)
    parameters.axis[component] /= axis_norm;

  const bool has_any_framing_option = has_framing_mode ||
      has_framing_features || has_framing_minimum_weight ||
      has_framing_coverage || has_framing_target_width ||
      has_framing_target_height;
  if((framing_output_path.empty() && has_any_framing_option) ||
     (!framing_output_path.empty() &&
      !(has_framing_mode && has_framing_features &&
        has_framing_minimum_weight && has_framing_coverage &&
        has_framing_target_width && has_framing_target_height))) {
    std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR incomplete framing request\n";
    return 2;
  }

  if(!landmark_output_path.empty() || !framing_output_path.empty()) {
    std::vector<std::string> output_paths;
    output_paths.push_back(output_path);
    if(!landmark_output_path.empty())
      output_paths.push_back(landmark_output_path);
    if(!framing_output_path.empty())
      output_paths.push_back(framing_output_path);
    const std::set<std::string> unique_paths(
        output_paths.begin(), output_paths.end());
    bool any_exists = false;
    for(std::size_t index = 0; index < output_paths.size(); ++index)
      any_exists = any_exists || pathExists(output_paths[index]);
    if(unique_paths.size() != output_paths.size() || any_exists) {
      std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR refusing to overwrite "
                << "profile, landmark, or framing output\n";
      return 3;
    }
  }

  StellarFeatureProbeSummaryV064 summary = {};
  StellarFeatureFramingResultV067 framing_result = {};
  std::string error;
  if(!stellarProbeSceneV064(
         scene_path, parameters, thresholds, &summary, &error) ||
     (!framing_output_path.empty() &&
      !stellarAssessFeatureFramingV067(
          summary, framing_request, &framing_result, &error)) ||
     !stellarWriteFeatureProbeV064(
         output_path, scene_path, parameters, summary, &error) ||
     (!landmark_output_path.empty() &&
      !stellarWriteFeatureLandmarksV066(
          landmark_output_path, scene_path, parameters, summary, &error)) ||
     (!framing_output_path.empty() &&
      !stellarWriteFeatureFramingPlanV067(
          framing_output_path, scene_path, parameters,
          framing_request, framing_result, &error))) {
    std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR " << error << '\n';
    return 3;
  }
  std::cout << "STELLAR_FEATURE_PROBE_V064_OK scene=" << scene_path
            << " output=" << output_path
            << " feature_profile="
            << stellarFeatureProfileNameV065(parameters.feature_profile)
            << " cells=" << summary.cells
            << " rays=" << summary.rays
            << " rows=" << summary.rows.size()
            << " orthographic=1";
  if(!landmark_output_path.empty())
    std::cout << " landmark_output=" << landmark_output_path;
  if(!framing_output_path.empty())
    std::cout << " framing_output=" << framing_output_path;
  std::cout << '\n';
  return 0;
}
