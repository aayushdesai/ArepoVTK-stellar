#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>

#include "stellar_gpu_scene_format_v052.h"

int main()
{
  ArepoStellarSceneHeader header = {};
  std::memcpy(header.magic, AREPO_STELLAR_SCENE_MAGIC,
              std::strlen(AREPO_STELLAR_SCENE_MAGIC));
  header.version = AREPO_STELLAR_SCENE_VERSION;
  header.endian_marker = AREPO_STELLAR_SCENE_ENDIAN_MARKER;
  header.header_bytes = sizeof(header);
  header.cell_bytes = sizeof(ArepoStellarCell);
  header.edge_bytes = sizeof(ArepoStellarEdge);
  header.ray_bytes = sizeof(ArepoStellarRay);
  header.sample_width = 16;
  header.sample_height = 9;
  header.source_width = 1920;
  header.source_height = 1080;
  header.samples_per_cell = 8;
  header.flags = AREPO_STELLAR_REQUIRED_FIELD_FLAGS |
      AREPO_STELLAR_NO_GHOST_CONTRIBS |
      AREPO_STELLAR_NATURAL_NEIGHBOR_INNER |
      AREPO_STELLAR_ZERO_LEGACY_ABSORPTION;
  header.num_cells = 1;
  header.num_rays = 144;
  header.box_size = 1.0e12;
  header.position_unit_cm = 1.0;
  header.density_unit_cgs = 1.0;
  header.velocity_unit_cm_per_s = 1.0;
  header.temperature_unit_kelvin = 1.0;
  header.snapshot_time_seconds = 760.0;

  ArepoStellarCell cell = {};
  cell.position[0] = 1.0e10;
  cell.position[1] = 2.0e10;
  cell.position[2] = 3.0e10;
  cell.density_log10_plus_10 = 9.25f;
  cell.temperature_kelvin = 2.5e7f;
  cell.velocity_cm_per_s[0] = 1.0e8f;
  cell.velocity_cm_per_s[1] = -2.0e8f;
  cell.velocity_cm_per_s[2] = 3.0e8f;
  cell.particle_id = 0xfedcba9876543210ULL;

  std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
  stream.write(reinterpret_cast<const char *>(&header), sizeof(header));
  stream.write(reinterpret_cast<const char *>(&cell), sizeof(cell));
  assert(stream.good());
  stream.seekg(0);

  ArepoStellarSceneHeader decoded_header = {};
  ArepoStellarCell decoded_cell = {};
  stream.read(reinterpret_cast<char *>(&decoded_header), sizeof(decoded_header));
  stream.read(reinterpret_cast<char *>(&decoded_cell), sizeof(decoded_cell));
  assert(stream.good());
  assert(std::memcmp(decoded_header.magic, AREPO_STELLAR_SCENE_MAGIC,
                     std::strlen(AREPO_STELLAR_SCENE_MAGIC)) == 0);
  assert(decoded_header.version == 5u);
  assert((decoded_header.flags & AREPO_STELLAR_REQUIRED_FIELD_FLAGS) ==
         AREPO_STELLAR_REQUIRED_FIELD_FLAGS);
  assert(decoded_header.cell_bytes == 52u);
  assert(decoded_cell.particle_id == cell.particle_id);
  assert(decoded_cell.velocity_cm_per_s[0] == cell.velocity_cm_per_s[0]);
  assert(decoded_cell.velocity_cm_per_s[1] == cell.velocity_cm_per_s[1]);
  assert(decoded_cell.velocity_cm_per_s[2] == cell.velocity_cm_per_s[2]);
  assert(std::isfinite(decoded_cell.temperature_kelvin));

  ArepoStellarSceneHeader rays_only = decoded_header;
  rays_only.flags |= AREPO_STELLAR_RAYS_ONLY;
  assert((rays_only.flags & AREPO_STELLAR_RAYS_ONLY) != 0u);
  assert((rays_only.flags & AREPO_STELLAR_REQUIRED_FIELD_FLAGS) ==
         AREPO_STELLAR_REQUIRED_FIELD_FLAGS);

  std::cout << "STELLAR_GPU_SCENE_FORMAT_V052_OK\n";
  return 0;
}
