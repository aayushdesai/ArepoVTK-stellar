#ifndef AREPO_RT_GPU_SCENE_FORMAT_H
#define AREPO_RT_GPU_SCENE_FORMAT_H

#include <stdint.h>

#define AREPO_GPU_SCENE_MAGIC "ARVTKGPUV041A"
#define AREPO_GPU_SCENE_VERSION 4u

enum ArepoGpuSceneFlags {
  AREPO_GPU_DENSITY_LOG10_PLUS_10 = 1u << 0,
  AREPO_GPU_TEMPERATURE_KELVIN = 1u << 1,
  AREPO_GPU_NO_GHOST_CONTRIBS = 1u << 2,
  AREPO_GPU_NATURAL_NEIGHBOR_INNER = 1u << 3,
  AREPO_GPU_ZERO_ABSORPTION = 1u << 4,
  AREPO_GPU_RAYS_ONLY = 1u << 5
};

#pragma pack(push, 1)
struct ArepoGpuSceneHeader {
  char magic[16];
  uint32_t version;
  uint32_t endian_marker;
  uint32_t header_bytes;
  uint32_t cell_bytes;
  uint32_t edge_bytes;
  uint32_t ray_bytes;
  uint32_t sample_width;
  uint32_t sample_height;
  uint32_t source_width;
  uint32_t source_height;
  int32_t samples_per_cell;
  uint32_t flags;
  uint64_t num_cells;
  uint64_t num_edges;
  uint64_t num_rays;
  uint64_t invalid_neighbor_edges;
  uint64_t inactive_rays;
  double box_size;
  double ray_max_t;
  double camera_origin[3];
  uint8_t reserved[16];
};

struct ArepoGpuCell {
  double position[3];
  float density;
  float temperature;
};

struct ArepoGpuEdge {
  float neighbor_delta[3];
  uint32_t packed_neighbor;
};

struct ArepoGpuRay {
  double origin[3];
  double direction[3];
  double t_min;
  double t_max;
  int32_t start_cell;
  int32_t active;
};
#pragma pack(pop)

static_assert(sizeof(ArepoGpuSceneHeader) == 160, "Unexpected GPU scene header layout");
static_assert(sizeof(ArepoGpuCell) == 32, "Unexpected GPU cell layout");
static_assert(sizeof(ArepoGpuEdge) == 16, "Unexpected GPU edge layout");
static_assert(sizeof(ArepoGpuRay) == 72, "Unexpected GPU ray layout");

#endif
