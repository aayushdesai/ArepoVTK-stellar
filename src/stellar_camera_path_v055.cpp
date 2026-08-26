#include "stellar_camera_path_v055.h"

#include <cerrno>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <sstream>

namespace {

bool fail(std::string *error, const std::string &message)
{
  if(error)
    *error = message;
  return false;
}

bool parseSnapshot(const std::string &text, unsigned long long *snapshot)
{
  if(text.empty())
    return false;
  for(std::size_t index = 0; index < text.size(); ++index)
    if(text[index] < '0' || text[index] > '9')
      return false;
  errno = 0;
  char *end = 0;
  const unsigned long long value = std::strtoull(text.c_str(), &end, 10);
  if(errno == ERANGE || !end || *end != '\0')
    return false;
  *snapshot = value;
  return true;
}

bool validPose(const StellarCameraPathRowV055 &row)
{
  double view[3];
  for(int component = 0; component < 3; component++)
    view[component] = row.pose.position[component] -
        row.pose.look_at[component];
  const double view_norm = stellarCameraNorm(view);
  const double up_norm = stellarCameraNorm(row.pose.up);
  const double axis_norm = stellarCameraNorm(row.axis);
  if(!(view_norm > 0.0) || !std::isfinite(view_norm) ||
     std::abs(up_norm - 1.0) > 1.0e-6 ||
     std::abs(axis_norm - 1.0) > 1.0e-6)
    return false;
  for(int component = 0; component < 3; component++)
    view[component] /= view_norm;
  return std::abs(stellarCameraDot(view, row.pose.up)) <= 1.0e-6;
}

} // namespace

bool StellarCameraPathV055::loadFile(const std::string &filename,
                                     std::string *error)
{
  rows.clear();
  std::ifstream input(filename.c_str());
  if(!input.good())
    return fail(error, "Cannot open stellar camera path: " + filename);
  return loadStream(input, error);
}

bool StellarCameraPathV055::loadStream(std::istream &input, std::string *error)
{
  rows.clear();
  std::vector<StellarCameraPathRowV055> candidate;
  std::string line;
  std::size_t line_number = 0;
  while(std::getline(input, line)) {
    ++line_number;
    const std::string::size_type start = line.find_first_not_of(" \t\r\n");
    if(start == std::string::npos || line[start] == '#')
      continue;
    std::istringstream parser(line.substr(start));
    StellarCameraPathRowV055 row = {};
    std::string snapshot_text;
    if(!(parser >> snapshot_text >> row.time_seconds >>
         row.pose.position[0] >> row.pose.position[1] >> row.pose.position[2] >>
         row.pose.look_at[0] >> row.pose.look_at[1] >> row.pose.look_at[2] >>
         row.pose.up[0] >> row.pose.up[1] >> row.pose.up[2] >>
         row.pose.screen_half_extent_cm >> row.center[0] >>
         row.center[1] >> row.center[2] >> row.axis[0] >> row.axis[1] >>
         row.axis[2] >> row.material_half_extent_cm >>
         row.disk_half_extent_cm >> row.outflow_half_extent_cm))
      return fail(error, "Malformed stellar camera path row at line " +
                   std::to_string(line_number));
    if(!parseSnapshot(snapshot_text, &row.snapshot))
      return fail(error, "Invalid stellar camera snapshot at line " +
                   std::to_string(line_number));
    std::string extra;
    if(parser >> extra)
      return fail(error, "Extra stellar camera path column at line " +
                   std::to_string(line_number));
    if(!stellarCameraFiniteVector(row.pose.position) ||
       !stellarCameraFiniteVector(row.pose.look_at) ||
       !stellarCameraFiniteVector(row.pose.up) ||
       !stellarCameraFiniteVector(row.center) ||
       !stellarCameraFiniteVector(row.axis) ||
       !std::isfinite(row.time_seconds) ||
       !(row.pose.screen_half_extent_cm > 0.0) ||
       !(row.material_half_extent_cm > 0.0) ||
       !(row.disk_half_extent_cm > 0.0) ||
       !(row.outflow_half_extent_cm > 0.0) || !validPose(row))
      return fail(error, "Invalid stellar camera geometry at line " +
                   std::to_string(line_number));
    if(!candidate.empty() &&
       (row.snapshot <= candidate.back().snapshot ||
        row.time_seconds <= candidate.back().time_seconds))
      return fail(error, "Camera snapshots and times must increase at line " +
                   std::to_string(line_number));
    candidate.push_back(row);
  }
  if(candidate.empty())
    return fail(error, "Stellar camera path is empty.");
  rows.swap(candidate);
  if(error)
    error->clear();
  return true;
}

const StellarCameraPathRowV055 *StellarCameraPathV055::find(
    unsigned long long snapshot) const
{
  std::size_t low = 0;
  std::size_t high = rows.size();
  while(low < high) {
    const std::size_t middle = low + (high - low) / 2;
    if(rows[middle].snapshot < snapshot)
      low = middle + 1;
    else
      high = middle;
  }
  if(low == rows.size() || rows[low].snapshot != snapshot)
    return 0;
  return &rows[low];
}
