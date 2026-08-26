#ifndef AREPO_RT_STELLAR_GPU_SCENE_FORMAT_V052_H
#define AREPO_RT_STELLAR_GPU_SCENE_FORMAT_V052_H

#include <stdint.h>

#define AREPO_STELLAR_SCENE_MAGIC "ARVTKSTARV052A"
#define AREPO_STELLAR_SCENE_VERSION 5u
#define AREPO_STELLAR_SCENE_ENDIAN_MARKER 0x01020304u

enum ArepoStellarSceneFlags {
  AREPO_STELLAR_DENSITY_LOG10_PLUS_10 = 1u << 0,
  AREPO_STELLAR_TEMPERATURE_KELVIN = 1u << 1,
  AREPO_STELLAR_NO_GHOST_CONTRIBS = 1u << 2,
  AREPO_STELLAR_NATURAL_NEIGHBOR_INNER = 1u << 3,
  AREPO_STELLAR_ZERO_LEGACY_ABSORPTION = 1u << 4,
  AREPO_STELLAR_RAYS_ONLY = 1u << 5,
  AREPO_STELLAR_VELOCITY_CM_PER_S = 1u << 6,
  AREPO_STELLAR_PARTICLE_ID_UINT64 = 1u << 7,
  AREPO_STELLAR_EXPLICIT_UNITS = 1u << 8
};

static const uint32_t AREPO_STELLAR_REQUIRED_FIELD_FLAGS =
    AREPO_STELLAR_DENSITY_LOG10_PLUS_10 |
    AREPO_STELLAR_TEMPERATURE_KELVIN |
    AREPO_STELLAR_VELOCITY_CM_PER_S |
    AREPO_STELLAR_PARTICLE_ID_UINT64 |
    AREPO_STELLAR_EXPLICIT_UNITS;

#pragma pack(push, 1)
struct ArepoStellarSceneHeader {
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
  double position_unit_cm;
  double density_unit_cgs;
  double velocity_unit_cm_per_s;
  double temperature_unit_kelvin;
  double snapshot_time_seconds;
  uint8_t reserved[24];
};

struct ArepoStellarCell {
  double position[3];
  float density_log10_plus_10;
  float temperature_kelvin;
  float velocity_cm_per_s[3];
  uint64_t particle_id;
};

struct ArepoStellarEdge {
  float neighbor_delta[3];
  uint32_t packed_neighbor;
};

struct ArepoStellarRay {
  double origin[3];
  double direction[3];
  double t_min;
  double t_max;
  int32_t start_cell;
  int32_t active;
};
#pragma pack(pop)

static_assert(sizeof(ArepoStellarSceneHeader) == 208,
              "Unexpected stellar scene header layout");
static_assert(sizeof(ArepoStellarCell) == 52,
              "Unexpected stellar scene cell layout");
static_assert(sizeof(ArepoStellarEdge) == 16,
              "Unexpected stellar scene edge layout");
static_assert(sizeof(ArepoStellarRay) == 72,
              "Unexpected stellar scene ray layout");

#endif
