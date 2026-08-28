#include <cerrno>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

#include <unistd.h>

#include "stellar_cinematic_director_v069.h"

namespace {

void usage(const char *program)
{
  std::cerr
      << "Usage: " << program << " --landmarks FILE --shots FILE"
      << " --visual-calibration FILE"
      << " --signed-framing FILE"
      << " --diagnostics FILE --manifest FILE --preview FILE"
      << " [--path FILE | --dry-run] [filter options]\n"
      << "Filter options:\n"
      << "  --center-response-seconds VALUE\n"
      << "  --center-max-speed-cm-per-s VALUE\n"
      << "  --center-max-acceleration-cm-per-s2 VALUE\n"
      << "  --axis-response-seconds VALUE\n"
      << "  --axis-max-rate-deg-per-s VALUE\n"
      << "  --scale-response-seconds VALUE\n"
      << "  --scale-max-log-rate-per-s VALUE\n"
      << "  --expand-hysteresis-fraction VALUE\n"
      << "  --contract-hysteresis-fraction VALUE\n"
      << "  --minimum-half-extent-cm VALUE\n"
      << "Motion budget options:\n"
      << "  --max-roll-deg-per-frame VALUE (default 0.25)\n"
      << "  --max-zoom-percent-per-frame VALUE (default 1.0)\n"
      << "  --max-artistic-pan-fraction-per-frame VALUE (default 0.01)\n"
      << "  --max-final-target-scale-error-percent VALUE (default 5.0)\n";
}

double parseFinite(const std::string &text, const std::string &name)
{
  errno = 0;
  char *end = 0;
  const double value = std::strtod(text.c_str(), &end);
  if(errno == ERANGE || end == text.c_str() || !end || *end != '\0' ||
     !std::isfinite(value))
    throw std::runtime_error("Invalid " + name + ": " + text);
  return value;
}

void requireOutput(const std::string &path, const std::string &name)
{
  if(path.empty())
    throw std::runtime_error("Missing required output " + name + ".");
  if(access(path.c_str(), F_OK) == 0)
    throw std::runtime_error("Refusing to overwrite " + name + ": " + path);
  if(errno != ENOENT)
    throw std::runtime_error("Cannot preflight " + name + ": " + path +
                             ": " + std::strerror(errno));
}

void writeNoClobber(const std::string &path, const std::string &contents,
                    const std::string &name)
{
  const int descriptor = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
  if(descriptor < 0)
    throw std::runtime_error("Cannot create " + name + " without overwrite: " +
                             path + ": " + std::strerror(errno));
  std::size_t written = 0;
  while(written < contents.size()) {
    const ssize_t count = write(descriptor, contents.data() + written,
                                contents.size() - written);
    if(count < 0) {
      const int saved_errno = errno;
      close(descriptor);
      throw std::runtime_error("Cannot write " + name + ": " + path + ": " +
                               std::strerror(saved_errno));
    }
    if(count == 0) {
      close(descriptor);
      throw std::runtime_error("Zero-byte write while creating " + name +
                               ": " + path);
    }
    written += static_cast<std::size_t>(count);
  }
  if(close(descriptor) != 0)
    throw std::runtime_error("Cannot close " + name + ": " + path + ": " +
                             std::strerror(errno));
}

std::string required(const std::map<std::string, std::string> &options,
                     const std::string &name)
{
  const std::map<std::string, std::string>::const_iterator value =
      options.find(name);
  if(value == options.end() || value->second.empty())
    throw std::runtime_error("Missing required option " + name + ".");
  return value->second;
}

void setFilterOption(const std::string &name, const std::string &text,
                     StellarCameraFilterParameters *filter)
{
  const double value = parseFinite(text, name);
  const double pi = std::acos(-1.0);
  if(name == "--center-response-seconds")
    filter->center_response_seconds = value;
  else if(name == "--center-max-speed-cm-per-s")
    filter->center_max_speed_cm_per_s = value;
  else if(name == "--center-max-acceleration-cm-per-s2")
    filter->center_max_acceleration_cm_per_s2 = value;
  else if(name == "--axis-response-seconds")
    filter->axis_response_seconds = value;
  else if(name == "--axis-max-rate-deg-per-s")
    filter->axis_max_rate_rad_per_s = value * pi / 180.0;
  else if(name == "--scale-response-seconds")
    filter->scale_response_seconds = value;
  else if(name == "--scale-max-log-rate-per-s")
    filter->scale_max_log_rate_per_s = value;
  else if(name == "--expand-hysteresis-fraction")
    filter->expand_hysteresis_fraction = value;
  else if(name == "--contract-hysteresis-fraction")
    filter->contract_hysteresis_fraction = value;
  else if(name == "--minimum-half-extent-cm")
    filter->minimum_half_extent_cm = value;
  else
    throw std::runtime_error("Unknown filter option " + name + ".");
}

void setMotionBudgetOption(const std::string &name, const std::string &text,
                           StellarDirectorMotionBudgetV069 *budget)
{
  const double value = parseFinite(text, name);
  if(name == "--max-roll-deg-per-frame")
    budget->max_roll_deg_per_frame = value;
  else if(name == "--max-zoom-percent-per-frame")
    budget->max_zoom_percent_per_frame = value;
  else if(name == "--max-artistic-pan-fraction-per-frame")
    budget->max_artistic_pan_fraction_per_frame = value;
  else if(name == "--max-final-target-scale-error-percent")
    budget->max_final_target_scale_error_percent = value;
  else
    throw std::runtime_error("Unknown motion budget option " + name + ".");
}

} // namespace

int main(int argc, char **argv)
{
  try {
    std::map<std::string, std::string> options;
    std::set<std::string> flags;
    const std::set<std::string> path_options = {
        "--landmarks", "--shots", "--visual-calibration", "--signed-framing",
        "--path", "--diagnostics",
        "--manifest", "--preview"};
    const std::set<std::string> filter_options = {
        "--center-response-seconds", "--center-max-speed-cm-per-s",
        "--center-max-acceleration-cm-per-s2", "--axis-response-seconds",
        "--axis-max-rate-deg-per-s", "--scale-response-seconds",
        "--scale-max-log-rate-per-s", "--expand-hysteresis-fraction",
        "--contract-hysteresis-fraction", "--minimum-half-extent-cm"};
    const std::set<std::string> motion_options = {
        "--max-roll-deg-per-frame", "--max-zoom-percent-per-frame",
        "--max-artistic-pan-fraction-per-frame",
        "--max-final-target-scale-error-percent"};

    for(int index = 1; index < argc; ++index) {
      const std::string name = argv[index];
      if(name == "--help") {
        usage(argv[0]);
        return 0;
      }
      if(name == "--dry-run") {
        if(!flags.insert(name).second)
          throw std::runtime_error("Duplicate option " + name + ".");
        continue;
      }
      if(path_options.count(name) == 0 && filter_options.count(name) == 0 &&
         motion_options.count(name) == 0)
        throw std::runtime_error("Unknown or positional option: " + name);
      if(options.count(name) != 0)
        throw std::runtime_error("Duplicate option " + name + ".");
      if(index + 1 >= argc)
        throw std::runtime_error("Option requires a value: " + name);
      options[name] = argv[++index];
    }

    const bool dry_run = flags.count("--dry-run") != 0;
    const std::string landmarks_path = required(options, "--landmarks");
    const std::string shots_path = required(options, "--shots");
    const std::string calibration_path =
        required(options, "--visual-calibration");
    const std::string framing_path = required(options, "--signed-framing");
    const std::string diagnostics_path = required(options, "--diagnostics");
    const std::string manifest_path = required(options, "--manifest");
    const std::string preview_path = required(options, "--preview");
    const bool path_given = options.count("--path") != 0;
    if(dry_run && path_given)
      throw std::runtime_error("--dry-run and --path cannot be combined.");
    if(!dry_run && !path_given)
      throw std::runtime_error("--path is required unless --dry-run is used.");
    const std::string path_path = path_given ? options["--path"] : "";

    StellarCameraFilterParameters filter = stellarDefaultCameraFilter();
    for(std::set<std::string>::const_iterator name = filter_options.begin();
        name != filter_options.end(); ++name) {
      const std::map<std::string, std::string>::const_iterator value =
          options.find(*name);
      if(value != options.end())
        setFilterOption(*name, value->second, &filter);
    }
    if(!stellarCameraValidFilter(filter))
      throw std::runtime_error("Camera filter controls are invalid.");
    StellarDirectorMotionBudgetV069 motion_budget = {0.25, 1.0, 0.01, 5.0};
    for(std::set<std::string>::const_iterator name = motion_options.begin();
        name != motion_options.end(); ++name) {
      const std::map<std::string, std::string>::const_iterator value =
          options.find(*name);
      if(value != options.end())
        setMotionBudgetOption(*name, value->second, &motion_budget);
    }
    if(!stellarDirectorValidMotionBudgetV069(motion_budget))
      throw std::runtime_error("Camera motion budget controls are invalid.");

    std::ifstream landmarks_input(landmarks_path.c_str());
    if(!landmarks_input.good())
      throw std::runtime_error("Cannot open landmarks: " + landmarks_path);
    std::ifstream shots_input(shots_path.c_str());
    if(!shots_input.good())
      throw std::runtime_error("Cannot open shots: " + shots_path);
    std::ifstream calibration_input(calibration_path.c_str());
    if(!calibration_input.good())
      throw std::runtime_error("Cannot open visual calibration: " +
                               calibration_path);
    std::ifstream framing_input(framing_path.c_str());
    if(!framing_input.good())
      throw std::runtime_error("Cannot open signed framing: " + framing_path);
    std::vector<StellarDirectorLandmarkRowV069> landmarks;
    std::vector<StellarDirectorShotV069> shots;
    std::vector<StellarDirectorVisualCalibrationRowV069> calibration;
    std::vector<StellarDirectorSignedFramingRowV069> framing;
    std::string error;
    if(!stellarDirectorLoadLandmarksV069(landmarks_input, &landmarks, &error))
      throw std::runtime_error(error);
    if(!stellarDirectorLoadShotsV069(shots_input, &shots, &error))
      throw std::runtime_error(error);
    if(!stellarDirectorLoadVisualCalibrationV069(
           calibration_input, &calibration, &error))
      throw std::runtime_error(error);
    if(!stellarDirectorLoadSignedFramingV069(
           framing_input, &framing, &error))
      throw std::runtime_error(error);
    StellarDirectorResultV069 result;
    if(!stellarDirectorBuildV069(landmarks, shots, calibration, framing, filter,
                                 motion_budget,
                                 &result, &error))
      throw std::runtime_error(error);
    const StellarDirectorMotionAssessmentV069 motion_assessment =
        stellarDirectorAssessMotionV069(result, motion_budget);

    std::set<std::string> outputs;
    outputs.insert(diagnostics_path);
    outputs.insert(manifest_path);
    outputs.insert(preview_path);
    if(path_given)
      outputs.insert(path_path);
    const std::size_t expected_outputs = path_given ? 4 : 3;
    if(outputs.size() != expected_outputs)
      throw std::runtime_error("Output paths must be distinct.");
    requireOutput(diagnostics_path, "diagnostics");
    requireOutput(manifest_path, "manifest");
    requireOutput(preview_path, "preview");
    if(path_given)
      requireOutput(path_path, "camera path");

    std::string manifest = stellarDirectorManifestV069(
        result, shots, calibration, framing, filter, motion_budget,
        motion_assessment, landmarks_path, shots_path, calibration_path,
        framing_path, dry_run);
    manifest += "path_output=" + (path_given ? path_path : "-") + "\n";
    manifest += std::string("path_written=") +
        (path_given && motion_assessment.passed ? "true" : "false") + "\n";
    manifest += "diagnostics_output=" + diagnostics_path + "\n";
    manifest += "manifest_output=" + manifest_path + "\n";
    manifest += "preview_output=" + preview_path + "\n";
    if(path_given && motion_assessment.passed)
      writeNoClobber(path_path, stellarDirectorPathTableV069(
          result, landmarks_path, shots_path, calibration_path, framing_path),
          "camera path");
    writeNoClobber(diagnostics_path,
                   stellarDirectorDiagnosticsTableV069(result),
                   "diagnostics");
    writeNoClobber(preview_path, stellarDirectorPreviewSvgV069(result),
                   "preview");
    writeNoClobber(manifest_path, manifest, "manifest");
    if(path_given && !motion_assessment.passed) {
      std::cerr << "STELLAR_CAMERA_DIRECTOR_V069_MOTION_REJECTED roll_deg_per_frame="
                << motion_assessment.maximum_roll_deg_per_frame
                << " roll_snapshot=" << motion_assessment.maximum_roll_snapshot
                << " zoom_percent_per_frame="
                << motion_assessment.maximum_zoom_percent_per_frame
                << " zoom_snapshot="
                << motion_assessment.maximum_zoom_snapshot
                << " pan_fraction_per_frame="
                << motion_assessment.maximum_artistic_pan_fraction_per_frame
                << " pan_snapshot="
                << motion_assessment.maximum_artistic_pan_snapshot
                << " final_target_scale_error_percent="
                << motion_assessment.final_target_scale_error_percent
                << " final_target_scale_error_limit_percent="
                << motion_budget.max_final_target_scale_error_percent << "\n";
      return 2;
    }
    std::cout << "STELLAR_CAMERA_DIRECTOR_V069_OK rows="
              << result.path.size() << " shots=" << shots.size()
              << " visual_calibration_rows=" << calibration.size()
              << " signed_framing_rows=" << framing.size()
              << " dry_run=" << (dry_run ? "true" : "false") << "\n";
    return 0;
  } catch(const std::exception &error) {
    std::cerr << "STELLAR_CAMERA_DIRECTOR_V069_ERROR " << error.what()
              << "\n";
    return 1;
  }
}
