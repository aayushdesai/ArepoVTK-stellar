#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "stellar_camera_path_v055.h"
#include "stellar_cinematic_director_v056.h"

namespace {

const char *landmarksText()
{
  return
      "# snapshot time cx cy cz ax ay az material disk polar\n"
      "100 0 0 0 0 0 0 1 10 8 12\n"
      "101 10 1 0 0 0.02 0 0.9998 11 8.5 13\n"
      "102 20 2 0 0 0.04 0.01 0.9991 12 9 15\n"
      "103 30 3 1 0 0.06 0.01 0.9981 13 10 18\n"
      "104 40 4 2 0 0.08 0.02 0.9966 14 11 22\n"
      "105 50 5 3 1 0.10 0.02 0.9948 15 12 28\n"
      "106 60 6 4 2 0.12 0.03 0.9923 16 13 35\n";
}

const char *shotsText()
{
  return
      "# schema=stellar_cinematic_shots_v056\n"
      "approach 100 102 orbit binary 10 20 1.10 30 - 1 10 smootherstep\n"
      "disk_reveal 103 104 disk_oblique disk 35 38 1.06 - - 1 10 smoothstep\n"
      "north_lobe 105 106 outflow_follow positive_lobe 45 8 1.12 - - 1 0 linear\n";
}

bool loadLandmarks(const std::string &text,
                   std::vector<StellarDirectorLandmarkRowV056> *rows,
                   std::string *error)
{
  std::istringstream input(text);
  return stellarDirectorLoadLandmarksV056(input, rows, error);
}

bool loadShots(const std::string &text,
               std::vector<StellarDirectorShotV056> *shots,
               std::string *error)
{
  std::istringstream input(text);
  return stellarDirectorLoadShotsV056(input, shots, error);
}

bool finiteDiagnostics(const StellarDirectorResultV056 &result)
{
  for(std::size_t index = 0; index < result.diagnostics.size(); ++index) {
    const StellarDirectorDiagnosticRowV056 &row = result.diagnostics[index];
    if(!std::isfinite(row.center_speed_cm_per_s) ||
       !std::isfinite(row.center_acceleration_cm_per_s2) ||
       !std::isfinite(row.center_jerk_cm_per_s3) ||
       !std::isfinite(row.camera_speed_cm_per_s) ||
       !std::isfinite(row.camera_acceleration_cm_per_s2) ||
       !std::isfinite(row.camera_jerk_cm_per_s3) ||
       !std::isfinite(row.axis_rate_rad_per_s) ||
       !std::isfinite(row.roll_rate_rad_per_s) ||
       !std::isfinite(row.log_zoom_rate_per_s) ||
       !std::isfinite(row.log_zoom_acceleration_per_s2))
      return false;
  }
  return true;
}

} // namespace

int main()
{
  std::vector<StellarDirectorLandmarkRowV056> landmarks;
  std::vector<StellarDirectorShotV056> shots;
  std::string error;
  assert(loadLandmarks(landmarksText(), &landmarks, &error));
  assert(loadShots(shotsText(), &shots, &error));
  assert(shots.size() == 3);
  assert(shots[2].mode == STELLAR_CAMERA_OUTFLOW_FOLLOW);
  assert(shots[2].lobe_sign == 1.0);
  assert(!shots[2].has_orbit_degrees);
  assert(!shots[2].has_orbit_period);

  StellarCameraFilterParameters filter = stellarDefaultCameraFilter();
  filter.minimum_half_extent_cm = 1.0;
  StellarDirectorResultV056 first;
  StellarDirectorResultV056 second;
  assert(stellarDirectorBuildV056(landmarks, shots, filter, &first, &error));
  assert(stellarDirectorBuildV056(landmarks, shots, filter, &second, &error));
  assert(first.path.size() == landmarks.size());
  assert(first.diagnostics.size() == first.path.size());
  assert(first.path[2].transition_to == "disk_reveal");
  assert(first.path[2].transition_fraction == 1.0);
  assert(first.path[4].transition_to == "north_lobe");
  assert(first.path[4].transition_fraction == 1.0);
  assert(first.diagnostics[1].center_acceleration_cm_per_s2 == 0.0);
  assert(first.diagnostics[1].camera_acceleration_cm_per_s2 == 0.0);
  assert(first.diagnostics[2].center_jerk_cm_per_s3 == 0.0);
  assert(first.diagnostics[2].camera_jerk_cm_per_s3 == 0.0);
  assert(finiteDiagnostics(first));

  const std::string path1 = stellarDirectorPathTableV056(
      first, "landmarks.tsv", "shots.tsv");
  const std::string path2 = stellarDirectorPathTableV056(
      second, "landmarks.tsv", "shots.tsv");
  assert(path1 == path2);
  assert(stellarDirectorDiagnosticsTableV056(first) ==
         stellarDirectorDiagnosticsTableV056(second));
  assert(stellarDirectorManifestV056(first, shots, filter, "landmarks.tsv",
                                     "shots.tsv", false) ==
         stellarDirectorManifestV056(second, shots, filter, "landmarks.tsv",
                                     "shots.tsv", false));
  const std::string preview = stellarDirectorPreviewSvgV056(first);
  assert(preview == stellarDirectorPreviewSvgV056(second));
  assert(preview.find("Stellar cinematic camera dry-run") != std::string::npos);
  assert(preview.find("Geometric occupancy") != std::string::npos);

  StellarCameraPathV055 native_path;
  std::istringstream path_input(path1);
  assert(native_path.loadStream(path_input, &error));
  assert(native_path.size() == first.path.size());
  assert(native_path.find(100));
  assert(native_path.find(106));

  std::vector<StellarDirectorShotV056> elevated = shots;
  elevated[0].elevation_radians += 0.2;
  StellarDirectorResultV056 elevated_result;
  assert(stellarDirectorBuildV056(landmarks, elevated, filter,
                                  &elevated_result, &error));
  assert(std::abs(elevated_result.path[0].pose.position[2] -
                  first.path[0].pose.position[2]) > 1.0e-6);

  const std::string missing_schema =
      "one 100 106 disk_edge merger 0 0 1.1 - - 1 0 linear\n";
  assert(!loadShots(missing_schema, &shots, &error));
  assert(error.find("Missing schema") != std::string::npos);

  const std::string dual_orbit =
      "# schema=stellar_cinematic_shots_v056\n"
      "one 100 106 orbit binary 0 20 1.1 90 200 1 0 linear\n";
  assert(!loadShots(dual_orbit, &shots, &error));
  assert(error.find("not both") != std::string::npos);

  const std::string range_gap =
      "# schema=stellar_cinematic_shots_v056\n"
      "one 100 102 disk_edge merger 0 0 1.1 - - 1 0 linear\n"
      "two 104 106 disk_edge merger 0 0 1.1 - - 1 0 linear\n";
  assert(!loadShots(range_gap, &shots, &error));
  assert(error.find("contiguous") != std::string::npos);

  const std::string duplicate_name =
      "# schema=stellar_cinematic_shots_v056\n"
      "one 100 102 disk_edge merger 0 0 1.1 - - 1 0 linear\n"
      "one 103 106 disk_edge merger 0 0 1.1 - - 1 0 linear\n";
  assert(!loadShots(duplicate_name, &shots, &error));
  assert(error.find("unique") != std::string::npos);

  const std::string final_transition =
      "# schema=stellar_cinematic_shots_v056\n"
      "one 100 106 disk_edge merger 0 0 1.1 - - 1 2 linear\n";
  assert(!loadShots(final_transition, &shots, &error));
  assert(error.find("final shot transition") != std::string::npos);

  std::cout << "STELLAR_CINEMATIC_DIRECTOR_V056_OK\n";
  return 0;
}
