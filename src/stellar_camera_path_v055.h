#ifndef AREPO_VTK_STELLAR_CAMERA_PATH_V055_H
#define AREPO_VTK_STELLAR_CAMERA_PATH_V055_H

#include <cstddef>
#include <istream>
#include <string>
#include <vector>

#include "stellar_camera_v054.h"

struct StellarCameraPathRowV055 {
  unsigned long long snapshot;
  double time_seconds;
  StellarCameraPose pose;
  double center[3];
  double axis[3];
  double material_half_extent_cm;
  double disk_half_extent_cm;
  double outflow_half_extent_cm;
};

class StellarCameraPathV055 {
public:
  bool loadFile(const std::string &filename, std::string *error);
  bool loadStream(std::istream &input, std::string *error);
  const StellarCameraPathRowV055 *find(unsigned long long snapshot) const;
  std::size_t size() const { return rows.size(); }

private:
  std::vector<StellarCameraPathRowV055> rows;
};

#endif
