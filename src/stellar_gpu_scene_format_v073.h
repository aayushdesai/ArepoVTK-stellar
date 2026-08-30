#ifndef AREPO_RT_STELLAR_GPU_SCENE_FORMAT_V073_H
#define AREPO_RT_STELLAR_GPU_SCENE_FORMAT_V073_H

#include <stdint.h>

#define AREPO_STELLAR_SCENE_MAGIC_V073 "ARVTKSTARV073A"
#define AREPO_STELLAR_SCENE_VERSION_V073 6u
#define AREPO_STELLAR_SCENE_ENDIAN_MARKER_V073 0x01020304u

enum ArepoStellarSceneFlagsV073 {
  AREPO_STELLAR_DENSITY_LOG10_PLUS_10_V073 = 1u << 0,
  AREPO_STELLAR_TEMPERATURE_KELVIN_V073 = 1u << 1,
  AREPO_STELLAR_NO_GHOST_CONTRIBS_V073 = 1u << 2,
  AREPO_STELLAR_NATURAL_NEIGHBOR_INNER_V073 = 1u << 3,
  AREPO_STELLAR_ZERO_LEGACY_ABSORPTION_V073 = 1u << 4,
  AREPO_STELLAR_RAYS_ONLY_V073 = 1u << 5,
  AREPO_STELLAR_VELOCITY_CM_PER_S_V073 = 1u << 6,
  AREPO_STELLAR_PARTICLE_ID_UINT64_V073 = 1u << 7,
  AREPO_STELLAR_EXPLICIT_UNITS_V073 = 1u << 8,
  AREPO_STELLAR_MAGNETIC_FIELD_GAUSS_V073 = 1u << 9,
  AREPO_STELLAR_PRESSURE_DYN_CM2_V073 = 1u << 10,
  AREPO_STELLAR_SOUND_SPEED_CM_PER_S_V073 = 1u << 11,
  AREPO_STELLAR_ENTRY_RELATIVE_RAY_LIMIT_V080 = 1u << 12
};

static const uint32_t AREPO_STELLAR_REQUIRED_FIELD_FLAGS_V073 =
    AREPO_STELLAR_DENSITY_LOG10_PLUS_10_V073 |
    AREPO_STELLAR_TEMPERATURE_KELVIN_V073 |
    AREPO_STELLAR_VELOCITY_CM_PER_S_V073 |
    AREPO_STELLAR_PARTICLE_ID_UINT64_V073 |
    AREPO_STELLAR_EXPLICIT_UNITS_V073 |
    AREPO_STELLAR_MAGNETIC_FIELD_GAUSS_V073 |
    AREPO_STELLAR_PRESSURE_DYN_CM2_V073 |
    AREPO_STELLAR_SOUND_SPEED_CM_PER_S_V073;

#pragma pack(push, 1)
struct ArepoStellarSceneHeaderV073 {
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
  double magnetic_field_unit_gauss;
  double pressure_unit_dyn_cm2;
  double sound_speed_unit_cm_per_s;
};

struct ArepoStellarCellV073 {
  double position[3];
  float density_log10_plus_10;
  float temperature_kelvin;
  float velocity_cm_per_s[3];
  uint64_t particle_id;
  float magnetic_field_gauss[3];
  float pressure_dyn_cm2;
  float sound_speed_cm_per_s;
};

struct ArepoStellarEdgeV073 {
  float neighbor_delta[3];
  uint32_t packed_neighbor;
};

struct ArepoStellarRayV073 {
  double origin[3];
  double direction[3];
  double t_min;
  double t_max;
  int32_t start_cell;
  int32_t active;
};
#pragma pack(pop)

static_assert(sizeof(ArepoStellarSceneHeaderV073) == 208,
              "Unexpected v073 stellar scene header layout");
static_assert(sizeof(ArepoStellarCellV073) == 72,
              "Unexpected v073 stellar scene cell layout");
static_assert(sizeof(ArepoStellarEdgeV073) == 16,
              "Unexpected v073 stellar scene edge layout");
static_assert(sizeof(ArepoStellarRayV073) == 72,
              "Unexpected v073 stellar scene ray layout");

#endif
