#include "stellar_feature_diagnostics_v064.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iostream>
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

bool assignPositiveFloat(const char *text, float *value)
{
  double parsed = 0.0;
  if(!parseFinite(text, &parsed) || !(parsed > 0.0))
    return false;
  *value = float(parsed);
  return std::isfinite(*value) && *value > 0.0f;
}

} // namespace

int main(int argc, char **argv)
{
  StellarTransferParameters parameters = {};
  parameters.mode = STELLAR_TRANSFER_COMPOSITE;
  parameters.palette_profile = STELLAR_PALETTE_LEGACY_V052;
  std::string scene_path;
  std::string output_path;
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
     !(axis_norm > 0.0) || !std::isfinite(axis_norm)) {
    std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR missing or invalid required input\n";
    return 2;
  }
  for(int component = 0; component < 3; ++component)
    parameters.axis[component] /= axis_norm;

  StellarFeatureProbeSummaryV064 summary = {};
  std::string error;
  if(!stellarProbeSceneV064(
         scene_path, parameters, thresholds, &summary, &error) ||
     !stellarWriteFeatureProbeV064(
         output_path, scene_path, parameters, summary, &error)) {
    std::cerr << "STELLAR_FEATURE_PROBE_V064_ERROR " << error << '\n';
    return 3;
  }
  std::cout << "STELLAR_FEATURE_PROBE_V064_OK scene=" << scene_path
            << " output=" << output_path
            << " cells=" << summary.cells
            << " rays=" << summary.rays
            << " rows=" << summary.rows.size()
            << " orthographic=1\n";
  return 0;
}
