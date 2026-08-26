#include <cerrno>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>

#include "stellar_camera_v054.h"

namespace {

struct InputRow {
  unsigned long long snapshot;
  StellarCameraLandmark landmark;
};

double parseFiniteDouble(const char *text, const std::string &label)
{
  errno = 0;
  char *end = 0;
  const double value = std::strtod(text, &end);
  if(errno == ERANGE || end == text || !end || *end != '\0' ||
     !std::isfinite(value))
    throw std::runtime_error("Invalid " + label + ": " + text);
  return value;
}

int parseShotMode(const std::string &name)
{
  if(name == "disk_edge")
    return STELLAR_CAMERA_DISK_EDGE;
  if(name == "disk_oblique")
    return STELLAR_CAMERA_DISK_OBLIQUE;
  if(name == "outflow_side")
    return STELLAR_CAMERA_OUTFLOW_SIDE;
  if(name == "outflow_axis")
    return STELLAR_CAMERA_OUTFLOW_AXIS;
  if(name == "outflow_follow")
    return STELLAR_CAMERA_OUTFLOW_FOLLOW;
  if(name == "orbit")
    return STELLAR_CAMERA_ORBIT;
  throw std::runtime_error("Unknown camera shot mode: " + name);
}

std::vector<InputRow> loadLandmarks(const std::string &filename)
{
  std::ifstream input(filename.c_str());
  if(!input.good())
    throw std::runtime_error("Cannot open landmark table: " + filename);
  std::vector<InputRow> rows;
  std::string line;
  while(std::getline(input, line)) {
    if(line.empty() || line[0] == '#')
      continue;
    std::istringstream parser(line);
    InputRow row = {};
    if(!(parser >> row.snapshot >> row.landmark.time_seconds >>
         row.landmark.center[0] >> row.landmark.center[1] >>
         row.landmark.center[2] >> row.landmark.axis[0] >>
         row.landmark.axis[1] >> row.landmark.axis[2] >>
         row.landmark.material_radius_cm >> row.landmark.disk_radius_cm >>
         row.landmark.polar_extent_cm))
      throw std::runtime_error("Malformed landmark row: " + line);
    std::string extra;
    if(parser >> extra)
      throw std::runtime_error("Landmark row has extra columns: " + line);
    if(!rows.empty() &&
       row.landmark.time_seconds <= rows.back().landmark.time_seconds)
      throw std::runtime_error("Landmark times must increase strictly.");
    if(!rows.empty() && row.snapshot <= rows.back().snapshot)
      throw std::runtime_error("Snapshot numbers must increase strictly.");
    rows.push_back(row);
  }
  if(rows.empty())
    throw std::runtime_error("Landmark table is empty.");
  return rows;
}


void writeNoClobber(const std::string &filename, const std::string &contents)
{
  const int descriptor = open(filename.c_str(), O_WRONLY | O_CREAT | O_EXCL,
                              0644);
  if(descriptor < 0)
    throw std::runtime_error("Cannot create camera path without overwrite: " +
                             filename + ": " + std::strerror(errno));
  size_t written = 0;
  while(written < contents.size()) {
    const ssize_t count = write(descriptor, contents.data() + written,
                                contents.size() - written);
    if(count < 0) {
      const int saved_errno = errno;
      close(descriptor);
      throw std::runtime_error("Cannot write camera path: " + filename +
                               ": " + std::strerror(saved_errno));
    }
    if(count == 0) {
      close(descriptor);
      throw std::runtime_error("Zero-byte write while creating camera path: " +
                               filename);
    }
    written += static_cast<size_t>(count);
  }
  if(close(descriptor) != 0)
    throw std::runtime_error("Cannot close camera path: " + filename +
                             ": " + std::strerror(errno));
}

} // namespace

int main(int argc, char **argv)
{
  try {
    if(argc < 4 || argc > 7) {
      std::cerr << "Usage: " << argv[0]
                << " landmarks.tsv output.tsv shot_mode"
                << " [azimuth_degrees [orbit_period_seconds [lobe_sign]]]\n";
      return 2;
    }
    const std::string inputPath = argv[1];
    const std::string outputPath = argv[2];
    const std::vector<InputRow> rows = loadLandmarks(inputPath);
    StellarCameraShot shot = {};
    shot.mode = parseShotMode(argv[3]);
    shot.framing_margin = 1.08;
    shot.lobe_sign = argc >= 7 ? parseFiniteDouble(argv[6], "lobe sign") : 1.0;
    if(shot.lobe_sign == 0.0)
      throw std::runtime_error("Lobe sign must be nonzero.");
    const double pi = std::acos(-1.0);
    shot.azimuth_radians = argc >= 5 ?
        parseFiniteDouble(argv[4], "azimuth") * pi / 180.0 : 0.0;
    if(argc >= 6) {
      const double period = parseFiniteDouble(argv[5], "orbit period");
      if(!(period > 0.0))
        throw std::runtime_error("Orbit period must be positive.");
      shot.orbit_rate_radians_per_second = 2.0 * pi / period;
    }

    std::ostringstream output;
    output << "# snapshot time_seconds camera_x camera_y camera_z look_x look_y "
              "look_z up_x up_y up_z half_extent center_x center_y center_z "
              "axis_x axis_y axis_z material_extent disk_extent outflow_extent\n";
    output << std::setprecision(17);
    StellarCameraState state = {};
    const StellarCameraFilterParameters parameters = stellarDefaultCameraFilter();
    for(size_t index = 0; index < rows.size(); index++) {
      if(!stellarUpdateCameraState(&state, rows[index].landmark, parameters))
        throw std::runtime_error("Camera filter failed at input row.");
      StellarCameraPose pose = {};
      if(!stellarCameraPoseForShot(state, shot, &pose))
        throw std::runtime_error("Camera pose failed at input row.");
      output << rows[index].snapshot << " " << state.time_seconds;
      for(int component = 0; component < 3; component++)
        output << " " << pose.position[component];
      for(int component = 0; component < 3; component++)
        output << " " << pose.look_at[component];
      for(int component = 0; component < 3; component++)
        output << " " << pose.up[component];
      output << " " << pose.screen_half_extent_cm;
      for(int component = 0; component < 3; component++)
        output << " " << state.center[component];
      for(int component = 0; component < 3; component++)
        output << " " << state.axis[component];
      output << " " << state.material_half_extent_cm
             << " " << state.disk_half_extent_cm
             << " " << state.outflow_half_extent_cm << "\n";
    }
    writeNoClobber(outputPath, output.str());
    std::cout << "STELLAR_CAMERA_PLAN_V054_OK rows=" << rows.size()
              << " output=" << outputPath << "\n";
    return 0;
  } catch(const std::exception &error) {
    std::cerr << "STELLAR_CAMERA_PLAN_V054_ERROR " << error.what() << "\n";
    return 1;
  }
}
