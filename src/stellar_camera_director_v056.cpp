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

#include "stellar_cinematic_director_v056.h"

namespace {

void usage(const char *program)
{
  std::cerr
      << "Usage: " << program << " --landmarks FILE --shots FILE"
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
      << "  --minimum-half-extent-cm VALUE\n";
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

} // namespace

int main(int argc, char **argv)
{
  try {
    std::map<std::string, std::string> options;
    std::set<std::string> flags;
    const std::set<std::string> path_options = {
        "--landmarks", "--shots", "--path", "--diagnostics",
        "--manifest", "--preview"};
    const std::set<std::string> filter_options = {
        "--center-response-seconds", "--center-max-speed-cm-per-s",
        "--center-max-acceleration-cm-per-s2", "--axis-response-seconds",
        "--axis-max-rate-deg-per-s", "--scale-response-seconds",
        "--scale-max-log-rate-per-s", "--expand-hysteresis-fraction",
        "--contract-hysteresis-fraction", "--minimum-half-extent-cm"};

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
      if(path_options.count(name) == 0 && filter_options.count(name) == 0)
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

    std::ifstream landmarks_input(landmarks_path.c_str());
    if(!landmarks_input.good())
      throw std::runtime_error("Cannot open landmarks: " + landmarks_path);
    std::ifstream shots_input(shots_path.c_str());
    if(!shots_input.good())
      throw std::runtime_error("Cannot open shots: " + shots_path);
    std::vector<StellarDirectorLandmarkRowV056> landmarks;
    std::vector<StellarDirectorShotV056> shots;
    std::string error;
    if(!stellarDirectorLoadLandmarksV056(landmarks_input, &landmarks, &error))
      throw std::runtime_error(error);
    if(!stellarDirectorLoadShotsV056(shots_input, &shots, &error))
      throw std::runtime_error(error);
    StellarDirectorResultV056 result;
    if(!stellarDirectorBuildV056(landmarks, shots, filter, &result, &error))
      throw std::runtime_error(error);

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

    std::string manifest = stellarDirectorManifestV056(
        result, shots, filter, landmarks_path, shots_path, dry_run);
    manifest += "path_output=" + (path_given ? path_path : "-") + "\n";
    manifest += "diagnostics_output=" + diagnostics_path + "\n";
    manifest += "manifest_output=" + manifest_path + "\n";
    manifest += "preview_output=" + preview_path + "\n";
    if(path_given)
      writeNoClobber(path_path, stellarDirectorPathTableV056(
          result, landmarks_path, shots_path), "camera path");
    writeNoClobber(diagnostics_path,
                   stellarDirectorDiagnosticsTableV056(result),
                   "diagnostics");
    writeNoClobber(preview_path, stellarDirectorPreviewSvgV056(result),
                   "preview");
    writeNoClobber(manifest_path, manifest, "manifest");
    std::cout << "STELLAR_CAMERA_DIRECTOR_V056_OK rows="
              << result.path.size() << " shots=" << shots.size()
              << " dry_run=" << (dry_run ? "true" : "false") << "\n";
    return 0;
  } catch(const std::exception &error) {
    std::cerr << "STELLAR_CAMERA_DIRECTOR_V056_ERROR " << error.what()
              << "\n";
    return 1;
  }
}
