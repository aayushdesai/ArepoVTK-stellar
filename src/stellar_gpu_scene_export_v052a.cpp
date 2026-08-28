#include "transform.h"
#include "spectrum.h"
#include "fileio.h"
#include "arepo.h"
#include "camera.h"
#include "sampler.h"
#include "stellar_gpu_scene_format_v052.h"
#include "stellar_gpu_scene_format_v073.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdint.h>
#include <vector>

namespace {

template <typename T>
bool writeSceneArray(std::ofstream &stream, const T *data, uint64_t count)
{
  if(count == 0)
    return true;
  stream.write(reinterpret_cast<const char *>(data), sizeof(T) * count);
  return stream.good();
}

bool collectCellEdges(int cell, std::vector<int> *edges)
{
  edges->clear();
  int edge = SphP[cell].first_connection;
  const int last = SphP[cell].last_connection;
  if(edge < 0)
    return last < 0;

  for(int guard = 0; guard <= Nvc; guard++) {
    if(edge < 0 || edge >= Nvc)
      return false;
    edges->push_back(edge);
    if(edge == last)
      return true;
    const int next = DC[edge].next;
    if(next == edge)
      return false;
    edge = next;
  }
  return false;
}

uint32_t packNeighbor(const connection &neighbor)
{
  uint32_t packed = 0;
  if(neighbor.index >= 0 && neighbor.index < NumGas)
    packed = uint32_t(neighbor.index) + 1u;
  if(neighbor.dp_index >= NumGas)
    packed |= 0x80000000u;
  return packed;
}

float compactNeighborDelta(double neighbor, double center)
{
  double delta = neighbor - center;
  if(delta > 0.5 * All.BoxSize)
    delta -= All.BoxSize;
  if(delta < -0.5 * All.BoxSize)
    delta += All.BoxSize;
  return float(delta);
}

} // namespace

bool ArepoMesh::ExportStellarGpuScene(const Camera *camera,
                                      const string &filename,
                                      int sampleWidth, int sampleHeight,
                                      bool raysOnly)
{
  if(!Config.stellarTransferEnabled) {
    cerr << "STELLAR_SCENE_EXPORT_ERROR stellar transfer is not enabled." << endl;
    return false;
  }
  if(!camera || sampleWidth <= 0 || sampleHeight <= 0 || NumGas <= 0) {
    cerr << "STELLAR_SCENE_EXPORT_ERROR invalid camera, dimensions, or cell count." << endl;
    return false;
  }
  if(ifstream(filename.c_str(), ios::binary).good()) {
    cerr << "STELLAR_SCENE_EXPORT_ERROR refusing to overwrite " << filename << endl;
    return false;
  }
  if(Config.viStepSize >= 0.0 || -Config.viStepSize > 64.0) {
    cerr << "STELLAR_SCENE_EXPORT_ERROR viStepSize must encode 1..64 samples per cell." << endl;
    return false;
  }
  if(Config.rgbAbsorb[0] != 0.0f || Config.rgbAbsorb[1] != 0.0f ||
     Config.rgbAbsorb[2] != 0.0f) {
    cerr << "STELLAR_SCENE_EXPORT_ERROR legacy absorption must be zero." << endl;
    return false;
  }
  if(RenderTemperature.size() != size_t(NumGas)) {
    cerr << "STELLAR_SCENE_EXPORT_ERROR aligned snapshot temperature is unavailable." << endl;
    return false;
  }
  const char *formatText = getenv("AREPORT_STELLAR_SCENE_FORMAT");
  const bool physicalV073 = formatText && string(formatText) == "v073";
  if(formatText && formatText[0] != '\0' && !physicalV073 &&
     string(formatText) != "v052") {
    cerr << "STELLAR_SCENE_EXPORT_ERROR unknown scene format "
         << formatText << endl;
    return false;
  }
  if(physicalV073 && RenderPhysicalAuxiliary.size() != size_t(NumGas)) {
    cerr << "STELLAR_SCENE_EXPORT_ERROR aligned v073 physical fields are unavailable."
         << endl;
    return false;
  }

  vector<uint64_t> offsets;
  vector<int> cellEdges;
  uint64_t numEdges = 0;
  uint64_t invalidEdges = 0;
  if(!raysOnly) {
    offsets.assign(NumGas + 1, 0);
    for(int cell = 0; cell < NumGas; cell++) {
      if(!collectCellEdges(cell, &cellEdges)) {
        cerr << "STELLAR_SCENE_EXPORT_ERROR malformed connectivity for cell "
             << cell << endl;
        return false;
      }
      for(size_t edgeIndex = 0; edgeIndex < cellEdges.size(); edgeIndex++) {
        const connection &neighbor = DC[cellEdges[edgeIndex]];
        if(neighbor.dp_index < 0 || neighbor.dp_index >= Ndp ||
           neighbor.index < 0 || neighbor.index >= NumGas)
          invalidEdges++;
      }
      numEdges += cellEdges.size();
      offsets[cell + 1] = numEdges;
    }
  }

  const uint64_t numRays = uint64_t(sampleWidth) * uint64_t(sampleHeight);
  vector<ArepoStellarRay> rays(numRays);
  double headerCameraOrigin[3] = {0.0, 0.0, 0.0};
  uint64_t inactiveRays = 0;
  int previousEntryCell = -1;
  for(int y = 0; y < sampleHeight; y++) {
    for(int x = 0; x < sampleWidth; x++) {
      const uint64_t rayIndex = uint64_t(y) * sampleWidth + x;
      ArepoStellarRay &record = rays[rayIndex];
      memset(&record, 0, sizeof(record));
      record.start_cell = -1;

      CameraSample sample;
      sample.imageX = (x + 0.5f) * float(camera->film->xResolution) /
          float(sampleWidth);
      sample.imageY = (y + 0.5f) * float(camera->film->yResolution) /
          float(sampleHeight);
      sample.lensU = sample.lensV = 0.5f;
      sample.time = 0.5f;

      Ray ray;
      camera->GenerateRay(sample, &ray);
      if(rayIndex == 0) {
        headerCameraOrigin[0] = ray.o.x;
        headerCameraOrigin[1] = ray.o.y;
        headerCameraOrigin[2] = ray.o.z;
      }
      double t0 = 0.0;
      double t1 = 0.0;
      if(!IntersectP(ray, &t0, &t1) || t1 <= t0) {
        inactiveRays++;
        continue;
      }
      ray.min_t = t0;
      ray.max_t = t1;
      if(Config.rayMaxT && Config.rayMaxT < ray.max_t)
        ray.max_t = Config.rayMaxT;
      if(ray.max_t <= ray.min_t) {
        inactiveRays++;
        continue;
      }

      LocateEntryCell(ray, &previousEntryCell);
      if(ray.index < 0 || ray.index >= NumGas) {
        inactiveRays++;
        continue;
      }

      record.origin[0] = ray.o.x;
      record.origin[1] = ray.o.y;
      record.origin[2] = ray.o.z;
      record.direction[0] = ray.d.x;
      record.direction[1] = ray.d.y;
      record.direction[2] = ray.d.z;
      record.t_min = ray.min_t;
      record.t_max = ray.max_t;
      record.start_cell = ray.index;
      record.active = 1;
    }
  }

  if(physicalV073) {
    ArepoStellarSceneHeaderV073 header;
    memset(&header, 0, sizeof(header));
    memcpy(header.magic, AREPO_STELLAR_SCENE_MAGIC_V073,
           strlen(AREPO_STELLAR_SCENE_MAGIC_V073));
    header.version = AREPO_STELLAR_SCENE_VERSION_V073;
    header.endian_marker = AREPO_STELLAR_SCENE_ENDIAN_MARKER_V073;
    header.header_bytes = sizeof(header);
    header.cell_bytes = sizeof(ArepoStellarCellV073);
    header.edge_bytes = sizeof(ArepoStellarEdgeV073);
    header.ray_bytes = sizeof(ArepoStellarRayV073);
    header.sample_width = sampleWidth;
    header.sample_height = sampleHeight;
    header.source_width = camera->film->xResolution;
    header.source_height = camera->film->yResolution;
    header.samples_per_cell = int(-Config.viStepSize);
    header.flags = AREPO_STELLAR_REQUIRED_FIELD_FLAGS_V073 |
        AREPO_STELLAR_ZERO_LEGACY_ABSORPTION_V073;
#ifdef NO_GHOST_CONTRIBS
    header.flags |= AREPO_STELLAR_NO_GHOST_CONTRIBS_V073;
#endif
#ifdef NATURAL_NEIGHBOR_INNER
    header.flags |= AREPO_STELLAR_NATURAL_NEIGHBOR_INNER_V073;
#endif
    if(raysOnly)
      header.flags |= AREPO_STELLAR_RAYS_ONLY_V073;
    header.num_cells = NumGas;
    header.num_edges = numEdges;
    header.num_rays = numRays;
    header.invalid_neighbor_edges = invalidEdges;
    header.inactive_rays = inactiveRays;
    header.box_size = All.BoxSize;
    header.ray_max_t = Config.rayMaxT;
    for(int axis = 0; axis < 3; axis++)
      header.camera_origin[axis] = headerCameraOrigin[axis];
    header.position_unit_cm = 1.0;
    header.density_unit_cgs = 1.0;
    header.velocity_unit_cm_per_s = 1.0;
    header.temperature_unit_kelvin = 1.0;
    header.snapshot_time_seconds = All.Time;
    header.magnetic_field_unit_gauss = 1.0;
    header.pressure_unit_dyn_cm2 = 1.0;
    header.sound_speed_unit_cm_per_s = 1.0;

    ofstream output(filename.c_str(), ios::binary | ios::out);
    if(!output.good()) {
      cerr << "STELLAR_SCENE_EXPORT_ERROR cannot create " << filename << endl;
      return false;
    }
    output.write(reinterpret_cast<const char *>(&header), sizeof(header));
    if(!raysOnly) {
      for(int cell = 0; cell < NumGas; cell++) {
        ArepoStellarCellV073 record;
        memset(&record, 0, sizeof(record));
        for(int axis = 0; axis < 3; axis++) {
          record.position[axis] = P[cell].Pos[axis];
          record.velocity_cm_per_s[axis] = P[cell].Vel[axis];
          record.magnetic_field_gauss[axis] =
              RenderPhysicalAuxiliary[cell].magnetic_field_gauss[axis];
        }
        record.density_log10_plus_10 = SphP[cell].Density;
        record.temperature_kelvin = RenderTemperature[cell];
        record.particle_id = uint64_t(P[cell].ID);
        record.pressure_dyn_cm2 =
            RenderPhysicalAuxiliary[cell].pressure_dyn_cm2;
        record.sound_speed_cm_per_s =
            RenderPhysicalAuxiliary[cell].sound_speed_cm_per_s;
        output.write(reinterpret_cast<const char *>(&record), sizeof(record));
      }
      if(!writeSceneArray(output, &offsets[0], offsets.size()))
        return false;
      for(int cell = 0; cell < NumGas; cell++) {
        collectCellEdges(cell, &cellEdges);
        for(size_t edgeIndex = 0; edgeIndex < cellEdges.size(); edgeIndex++) {
          const connection &neighbor = DC[cellEdges[edgeIndex]];
          ArepoStellarEdgeV073 record;
          memset(&record, 0, sizeof(record));
          record.packed_neighbor = packNeighbor(neighbor);
          if(neighbor.dp_index >= 0 && neighbor.dp_index < Ndp) {
            record.neighbor_delta[0] = compactNeighborDelta(
                DP[neighbor.dp_index].x, P[cell].Pos[0]);
            record.neighbor_delta[1] = compactNeighborDelta(
                DP[neighbor.dp_index].y, P[cell].Pos[1]);
            record.neighbor_delta[2] = compactNeighborDelta(
                DP[neighbor.dp_index].z, P[cell].Pos[2]);
          } else {
            record.neighbor_delta[0] = numeric_limits<float>::quiet_NaN();
            record.neighbor_delta[1] = numeric_limits<float>::quiet_NaN();
            record.neighbor_delta[2] = numeric_limits<float>::quiet_NaN();
          }
          output.write(reinterpret_cast<const char *>(&record), sizeof(record));
        }
      }
    }
    if(!writeSceneArray(output, &rays[0], rays.size()))
      return false;
    output.close();
    if(!output.good()) {
      cerr << "STELLAR_SCENE_EXPORT_ERROR failed while closing " << filename
           << endl;
      return false;
    }
    cerr << "STELLAR_SCENE_EXPORT_V073_OK file=" << filename
         << " cells=" << header.num_cells
         << " edges=" << header.num_edges
         << " invalid_edges=" << header.invalid_neighbor_edges
         << " rays=" << header.num_rays
         << " inactive_rays=" << header.inactive_rays
         << " rays_only=" << (raysOnly ? 1 : 0)
         << " samples_per_cell=" << header.samples_per_cell
         << " fields=magnetic_field_gauss,pressure_dyn_cm2,sound_speed_cm_per_s"
         << endl;
    return true;
  }

  ArepoStellarSceneHeader header;
  memset(&header, 0, sizeof(header));
  memcpy(header.magic, AREPO_STELLAR_SCENE_MAGIC,
         strlen(AREPO_STELLAR_SCENE_MAGIC));
  header.version = AREPO_STELLAR_SCENE_VERSION;
  header.endian_marker = AREPO_STELLAR_SCENE_ENDIAN_MARKER;
  header.header_bytes = sizeof(header);
  header.cell_bytes = sizeof(ArepoStellarCell);
  header.edge_bytes = sizeof(ArepoStellarEdge);
  header.ray_bytes = sizeof(ArepoStellarRay);
  header.sample_width = sampleWidth;
  header.sample_height = sampleHeight;
  header.source_width = camera->film->xResolution;
  header.source_height = camera->film->yResolution;
  header.samples_per_cell = int(-Config.viStepSize);
  header.flags = AREPO_STELLAR_REQUIRED_FIELD_FLAGS |
      AREPO_STELLAR_ZERO_LEGACY_ABSORPTION;
#ifdef NO_GHOST_CONTRIBS
  header.flags |= AREPO_STELLAR_NO_GHOST_CONTRIBS;
#endif
#ifdef NATURAL_NEIGHBOR_INNER
  header.flags |= AREPO_STELLAR_NATURAL_NEIGHBOR_INNER;
#endif
  if(raysOnly)
    header.flags |= AREPO_STELLAR_RAYS_ONLY;
  header.num_cells = NumGas;
  header.num_edges = numEdges;
  header.num_rays = numRays;
  header.invalid_neighbor_edges = invalidEdges;
  header.inactive_rays = inactiveRays;
  header.box_size = All.BoxSize;
  header.ray_max_t = Config.rayMaxT;
  header.camera_origin[0] = headerCameraOrigin[0];
  header.camera_origin[1] = headerCameraOrigin[1];
  header.camera_origin[2] = headerCameraOrigin[2];
  header.position_unit_cm = 1.0;
  header.density_unit_cgs = 1.0;
  header.velocity_unit_cm_per_s = 1.0;
  header.temperature_unit_kelvin = 1.0;
  header.snapshot_time_seconds = All.Time;

  ofstream output(filename.c_str(), ios::binary | ios::out);
  if(!output.good()) {
    cerr << "STELLAR_SCENE_EXPORT_ERROR cannot create " << filename << endl;
    return false;
  }
  output.write(reinterpret_cast<const char *>(&header), sizeof(header));

  if(!raysOnly) {
    for(int cell = 0; cell < NumGas; cell++) {
      ArepoStellarCell record;
      memset(&record, 0, sizeof(record));
      record.position[0] = P[cell].Pos[0];
      record.position[1] = P[cell].Pos[1];
      record.position[2] = P[cell].Pos[2];
      record.density_log10_plus_10 = SphP[cell].Density;
      record.temperature_kelvin = RenderTemperature[cell];
      record.velocity_cm_per_s[0] = P[cell].Vel[0];
      record.velocity_cm_per_s[1] = P[cell].Vel[1];
      record.velocity_cm_per_s[2] = P[cell].Vel[2];
      record.particle_id = uint64_t(P[cell].ID);
      output.write(reinterpret_cast<const char *>(&record), sizeof(record));
    }
    if(!writeSceneArray(output, &offsets[0], offsets.size()))
      return false;

    for(int cell = 0; cell < NumGas; cell++) {
      collectCellEdges(cell, &cellEdges);
      for(size_t edgeIndex = 0; edgeIndex < cellEdges.size(); edgeIndex++) {
        const connection &neighbor = DC[cellEdges[edgeIndex]];
        ArepoStellarEdge record;
        memset(&record, 0, sizeof(record));
        record.packed_neighbor = packNeighbor(neighbor);
        if(neighbor.dp_index >= 0 && neighbor.dp_index < Ndp) {
          record.neighbor_delta[0] = compactNeighborDelta(
              DP[neighbor.dp_index].x, P[cell].Pos[0]);
          record.neighbor_delta[1] = compactNeighborDelta(
              DP[neighbor.dp_index].y, P[cell].Pos[1]);
          record.neighbor_delta[2] = compactNeighborDelta(
              DP[neighbor.dp_index].z, P[cell].Pos[2]);
        } else {
          record.neighbor_delta[0] = numeric_limits<float>::quiet_NaN();
          record.neighbor_delta[1] = numeric_limits<float>::quiet_NaN();
          record.neighbor_delta[2] = numeric_limits<float>::quiet_NaN();
        }
        output.write(reinterpret_cast<const char *>(&record), sizeof(record));
      }
    }
  }
  if(!writeSceneArray(output, &rays[0], rays.size()))
    return false;
  output.close();
  if(!output.good()) {
    cerr << "STELLAR_SCENE_EXPORT_ERROR failed while closing " << filename << endl;
    return false;
  }

  cout << "STELLAR_SCENE_EXPORT_OK file=" << filename
       << " cells=" << header.num_cells
       << " edges=" << header.num_edges
       << " invalid_edges=" << header.invalid_neighbor_edges
       << " rays=" << header.num_rays
       << " inactive_rays=" << header.inactive_rays
       << " rays_only=" << (raysOnly ? 1 : 0)
       << " samples_per_cell=" << header.samples_per_cell << endl;
  return true;
}
