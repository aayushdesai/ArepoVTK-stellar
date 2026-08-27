#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>

#include "stellar_feature_diagnostics_v064.h"
#include "stellar_gpu_scene_format_v052.h"

namespace {

StellarTransferParameters parameters()
{
  StellarTransferParameters value = {};
  value.mode = STELLAR_TRANSFER_COMPOSITE;
  value.palette_profile = STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058;
  value.axis[2] = 1.0;
  value.box_size = 1.0e12;
  value.material_radius_cm = 2.0e10f;
  value.disk_radius_cm = 4.0e10f;
  value.disk_half_thickness_cm = 8.0e9f;
  value.polar_inner_cm = 5.0e9f;
  value.polar_outer_cm = 8.0e10f;
  value.polar_cone_ratio = 0.5f;
  return value;
}

ArepoStellarCell cell(double x, double y, double z, float density,
                      float temperature, float vx, float vy, float vz,
                      uint64_t id)
{
  ArepoStellarCell value = {};
  value.position[0] = x;
  value.position[1] = y;
  value.position[2] = z;
  value.density_log10_plus_10 = density;
  value.temperature_kelvin = temperature;
  value.velocity_cm_per_s[0] = vx;
  value.velocity_cm_per_s[1] = vy;
  value.velocity_cm_per_s[2] = vz;
  value.particle_id = id;
  return value;
}

void writeScene(const std::string &path)
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
  header.sample_width = 4;
  header.sample_height = 4;
  header.source_width = 4;
  header.source_height = 4;
  header.samples_per_cell = 8;
  header.flags = AREPO_STELLAR_REQUIRED_FIELD_FLAGS |
      AREPO_STELLAR_ZERO_LEGACY_ABSORPTION;
  header.num_cells = 4;
  header.num_edges = 0;
  header.num_rays = 16;
  header.box_size = 1.0e12;
  header.position_unit_cm = 1.0;
  header.density_unit_cgs = 1.0;
  header.velocity_unit_cm_per_s = 1.0;
  header.temperature_unit_kelvin = 1.0;

  std::vector<ArepoStellarCell> cells;
  cells.push_back(cell(0.0, 2.0e9, 0.0, 14.0f, 2.5e7f,
                       -2.0e8f, 0.0f, 0.0f, 1));
  cells.push_back(cell(0.0, 2.0e10, 0.0, 10.8f, 2.5e7f,
                       -2.0e8f, 0.0f, 0.0f, 2));
  cells.push_back(cell(0.0, 0.0, 3.0e10, 8.0f, 5.0e6f,
                       0.0f, 0.0f, 2.0e8f, 3));
  cells.push_back(cell(0.0, 0.0, -3.0e10, 8.0f, 5.0e6f,
                       0.0f, 0.0f, -2.0e8f, 4));
  std::vector<uint64_t> offsets(header.num_cells + 1, 0);
  std::vector<ArepoStellarRay> rays(header.num_rays);
  for(uint32_t y = 0; y < header.sample_height; ++y)
    for(uint32_t x = 0; x < header.sample_width; ++x) {
      ArepoStellarRay &ray = rays[uint64_t(y) * header.sample_width + x];
      std::memset(&ray, 0, sizeof(ray));
      ray.origin[0] = -5.0e10;
      ray.origin[1] = (double(x) - 1.5) * 2.0e10;
      ray.origin[2] = (double(y) - 1.5) * 2.0e10;
      ray.direction[0] = 1.0;
      ray.t_max = 1.0e11;
      ray.start_cell = 0;
      ray.active = 1;
    }

  std::ofstream output(path.c_str(), std::ios::binary);
  assert(output.good());
  output.write(reinterpret_cast<const char *>(&header), sizeof(header));
  output.write(reinterpret_cast<const char *>(&cells[0]),
               cells.size() * sizeof(cells[0]));
  output.write(reinterpret_cast<const char *>(&offsets[0]),
               offsets.size() * sizeof(offsets[0]));
  output.write(reinterpret_cast<const char *>(&rays[0]),
               rays.size() * sizeof(rays[0]));
  output.close();
  assert(output.good());
}

const StellarFeatureProbeRowV064 &row(
    const StellarFeatureProbeSummaryV064 &summary,
    const std::string &feature)
{
  for(std::size_t index = 0; index < summary.rows.size(); ++index)
    if(summary.rows[index].feature == feature)
      return summary.rows[index];
  assert(false);
  return summary.rows[0];
}

} // namespace

int main(int argc, char **argv)
{
  assert(argc == 2);
  const std::string root = argv[1];
  const std::string scene = root + "/feature_scene_v064.bin";
  const std::string report = root + "/feature_report_v064.tsv";
  writeScene(scene);

  const StellarTransferParameters transfer = parameters();
  const double core_position[3] = {0.0, 2.0e9, 0.0};
  const float core_velocity[3] = {-2.0e8f, 0.0f, 0.0f};
  const StellarFeatureSampleV064 core = evaluateStellarFeatureSampleV064(
      transfer, core_position, 14.0f, 2.5e7f, core_velocity);
  assert(core.disk_weight > 0.0f);
  assert(core.merger_weight > 0.0f);
  assert(core.log_density == 4.0f);
  assert(core.cylindrical_radius_cm < 0.35f * transfer.disk_radius_cm);

  StellarFeatureProbeSummaryV064 summary = {};
  std::string error;
  assert(stellarProbeSceneV064(
      scene, transfer, std::vector<float>(1, 0.01f), &summary, &error));
  StellarTransferParameters invalid_transfer = transfer;
  invalid_transfer.disk_radius_cm = 0.0f;
  StellarFeatureProbeSummaryV064 invalid_summary = {};
  assert(!stellarProbeSceneV064(
      scene, invalid_transfer, std::vector<float>(1, 0.01f),
      &invalid_summary, &error));
  assert(summary.cells == 4);
  assert(summary.rays == 16);
  assert(summary.rows.size() == 5);
  const StellarFeatureProbeRowV064 &disk = row(summary, "disk");
  const StellarFeatureProbeRowV064 &positive = row(summary, "polar_positive");
  const StellarFeatureProbeRowV064 &negative = row(summary, "polar_negative");
  assert(disk.selected_cells == 2);
  assert(disk.weighted_high_density_fraction > 0.0);
  assert(disk.weighted_inner_radius_fraction > 0.0);
  assert(disk.weighted_merger_overlap > 0.0);
  assert(positive.selected_cells == 1);
  assert(negative.selected_cells == 1);
  assert(positive.right_censored);
  assert(negative.right_censored);
  assert(stellarWriteFeatureProbeV064(
      report, scene, transfer, summary, &error));
  assert(!stellarWriteFeatureProbeV064(
      report, scene, transfer, summary, &error));

  std::cout << "STELLAR_FEATURE_DIAGNOSTICS_V064_OK"
            << " disk_cells=" << disk.selected_cells
            << " disk_core_fraction=" << disk.weighted_inner_radius_fraction
            << " disk_merger_overlap=" << disk.weighted_merger_overlap
            << " polar_positive=" << positive.selected_cells
            << " polar_negative=" << negative.selected_cells << '\n';
  return 0;
}
