#include <cuda_runtime.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "stellar_gpu_scene_format_v073.h"
#include "stellar_display_encoding_v053b.h"
#include "stellar_gpu_geometry_v053b.h"
#include "stellar_gpu_ray_status_v053b.h"
#include "stellar_reconstruction_v053.h"
#include "stellar_render_model_v052a.h"
#include "stellar_gpu_profile_contract_v053f.h"
#include "stellar_physical_channel_v072.h"
#include "stellar_physical_optical_v076.h"

namespace {

const int kMaxNeighbors = 800;
const int kMaxCellsPerRay = 10000;

struct RayResult {
  float rgb[3];
  uint32_t cells;
  uint32_t samples;
  uint32_t status;
};

enum RayStatus {
  kRayOk = 0,
  kRayInactive = STELLAR_GPU_RAY_INACTIVE_V053B,
  kRayInvalidCell = STELLAR_GPU_RAY_INVALID_CELL_V053B,
  kRayInvalidEdge = STELLAR_GPU_RAY_INVALID_EDGE_V053B,
  kRayNeighborOverflow = STELLAR_GPU_RAY_NEIGHBOR_OVERFLOW_V053B,
  kRayNoExitFace = STELLAR_GPU_RAY_NO_EXIT_FACE_V053B,
  kRayCellLimit = STELLAR_GPU_RAY_CELL_LIMIT_V053B,
  kRayNonFinite = STELLAR_GPU_RAY_NONFINITE_V053B
};

struct SceneData {
  ArepoStellarSceneHeaderV073 header;
  std::vector<ArepoStellarCellV073> cells;
  std::vector<uint64_t> offsets;
  std::vector<ArepoStellarEdgeV073> edges;
  std::vector<ArepoStellarRayV073> rays;
};

struct DeviceScene {
  const ArepoStellarCellV073 *cells;
  const uint64_t *offsets;
  const ArepoStellarEdgeV073 *edges;
  const ArepoStellarRayV073 *rays;
  const float *palette;
  const int *neighbor_counts;
  const uint64_t *neighbor_offsets;
  const int *neighbor_cache;
  uint64_t num_cells;
  uint64_t num_rays;
  int palette_len;
  int samples_per_cell;
  double box_size;
  double camera_origin[3];
  double ray_max_t;
  int reconstruction_mode;
  float idw_power;
  float sph_support_factor;
  StellarTransferParameters transfer;
  StellarPhysicalTransferV071 physical_transfer;
  StellarPhysicalOpticalParametersV076 physical_optical;
  float exposure;
  float black_point;
  float saturation;
  float display_brightness;
};

#define CUDA_CHECK(call) do { \
  cudaError_t error_ = (call); \
  if(error_ != cudaSuccess) { \
    std::ostringstream message_; \
    message_ << #call << " failed: " << cudaGetErrorString(error_); \
    throw std::runtime_error(message_.str()); \
  } \
} while(0)

template <typename T>
void readArray(std::ifstream &input, std::vector<T> *values, uint64_t count)
{
  if(count > uint64_t(std::numeric_limits<size_t>::max() / sizeof(T)))
    throw std::runtime_error("Scene array is too large for this host.");
  values->resize(size_t(count));
  if(count != 0)
    input.read(reinterpret_cast<char *>(&(*values)[0]), sizeof(T) * count);
  if(!input.good())
    throw std::runtime_error("Truncated GPU scene file.");
}

SceneData loadScene(const std::string &filename)
{
  std::ifstream input(filename.c_str(), std::ios::binary);
  if(!input.good())
    throw std::runtime_error("Cannot open scene: " + filename);
  SceneData scene;
  input.read(reinterpret_cast<char *>(&scene.header), sizeof(scene.header));
  if(!input.good())
    throw std::runtime_error("Cannot read scene header.");
  if(std::memcmp(scene.header.magic, AREPO_STELLAR_SCENE_MAGIC_V073,
                 std::strlen(AREPO_STELLAR_SCENE_MAGIC_V073)) != 0 ||
     scene.header.version != AREPO_STELLAR_SCENE_VERSION_V073 ||
     scene.header.endian_marker != AREPO_STELLAR_SCENE_ENDIAN_MARKER_V073 ||
     scene.header.header_bytes != sizeof(ArepoStellarSceneHeaderV073) ||
     scene.header.cell_bytes != sizeof(ArepoStellarCellV073) ||
     scene.header.edge_bytes != sizeof(ArepoStellarEdgeV073) ||
     scene.header.ray_bytes != sizeof(ArepoStellarRayV073))
    throw std::runtime_error("Unsupported or corrupt GPU scene header.");
  const uint32_t requiredFlags = AREPO_STELLAR_REQUIRED_FIELD_FLAGS_V073 |
      AREPO_STELLAR_ZERO_LEGACY_ABSORPTION_V073;
  if((scene.header.flags & requiredFlags) != requiredFlags)
    throw std::runtime_error("Scene does not match the v073 physical-field contract.");
  if(scene.header.position_unit_cm != 1.0 ||
     scene.header.density_unit_cgs != 1.0 ||
     scene.header.velocity_unit_cm_per_s != 1.0 ||
     scene.header.temperature_unit_kelvin != 1.0 ||
     scene.header.magnetic_field_unit_gauss != 1.0 ||
     scene.header.pressure_unit_dyn_cm2 != 1.0 ||
     scene.header.sound_speed_unit_cm_per_s != 1.0)
    throw std::runtime_error("v073 renderer requires canonical cgs scene values.");
  if(scene.header.num_rays != uint64_t(scene.header.sample_width) * scene.header.sample_height)
    throw std::runtime_error("Scene ray dimensions are inconsistent.");
  if(scene.header.samples_per_cell <= 0 || scene.header.samples_per_cell > 64)
    throw std::runtime_error("Unsupported samples-per-cell value.");

  readArray(input, &scene.cells, scene.header.num_cells);
  readArray(input, &scene.offsets, scene.header.num_cells + 1);
  readArray(input, &scene.edges, scene.header.num_edges);
  readArray(input, &scene.rays, scene.header.num_rays);
  if(scene.offsets.empty() || scene.offsets.front() != 0 ||
     scene.offsets.back() != scene.header.num_edges)
    throw std::runtime_error("Corrupt connectivity offsets.");
  return scene;
}

std::vector<float> loadPalette(const std::string &filename)
{
  std::ifstream input(filename.c_str());
  if(!input.good())
    throw std::runtime_error("Cannot open palette: " + filename);
  std::vector<float> values;
  std::string line;
  while(std::getline(input, line)) {
    const size_t comment = line.find('#');
    if(comment != std::string::npos)
      line.erase(comment);
    std::istringstream parser(line);
    float value;
    while(parser >> value)
      values.push_back(value);
  }
  if(values.empty())
    throw std::runtime_error("Palette is empty.");
  const int count = int(values[0]);
  if(count <= 1 || values.size() != size_t(1 + count * 4))
    throw std::runtime_error("Palette row count is inconsistent.");
  values.erase(values.begin());
  float maximum = 0.0f;
  for(int row = 0; row < count; row++)
    for(int channel = 0; channel < 3; channel++)
      maximum = std::max(maximum, values[row * 4 + channel]);
  if(maximum > 2.0f)
    for(int row = 0; row < count; row++)
      for(int channel = 0; channel < 3; channel++)
        values[row * 4 + channel] /= 255.0f;
  return values;
}

__host__ __device__ inline double clampDouble(double value, double low, double high)
{
  return value < low ? low : (value > high ? high : value);
}

__host__ __device__ inline float clampFloat(float value, float low, float high)
{
  return value < low ? low : (value > high ? high : value);
}

__host__ __device__ inline double wrapPosition(double value, double reference, double box)
{
  return stellarGpuWrapPositionV053b(value, reference, box);
}

__host__ __device__ inline float periodicDistance(double a, double b, double box)
{
  double distance = fabs(a - b);
  if(distance > 0.5 * box)
    distance = box - distance;
  return float(distance);
}

__host__ __device__ inline bool validCell(const DeviceScene &scene, int cell)
{
  return cell >= 0 && uint64_t(cell) < scene.num_cells;
}

__host__ __device__ bool rayBoxInterval(const DeviceScene &scene,
                                        const ArepoStellarRayV073 &ray,
                                        double *entry, double *exit)
{
  double t0 = 0.0;
  double t1 = 3.402823466e+38;
  for(int axis = 0; axis < 3; axis++) {
    const double inverseDirection = 1.0 / ray.direction[axis];
    double nearT = (0.0 - scene.camera_origin[axis]) * inverseDirection;
    double farT = (scene.box_size - scene.camera_origin[axis]) * inverseDirection;
    if(nearT > farT) {
      const double swap = nearT;
      nearT = farT;
      farT = swap;
    }
    if(nearT > t0)
      t0 = nearT;
    if(farT < t1)
      t1 = farT;
  }
  if(scene.ray_max_t > 0.0 && scene.ray_max_t < t1)
    t1 = scene.ray_max_t;
  *entry = t0;
  *exit = t1;
  return t1 > t0;
}

__host__ __device__ inline int edgeCell(const ArepoStellarEdgeV073 &edge)
{
  const uint32_t encoded = edge.packed_neighbor & 0x7fffffffu;
  return encoded == 0 ? -1 : int(encoded - 1u);
}

__host__ __device__ inline bool edgeContributes(const ArepoStellarEdgeV073 &edge)
{
  return (edge.packed_neighbor & 0x80000000u) == 0;
}

__host__ __device__ StellarOpticalSample evaluateTransfer(
    const DeviceScene &scene, int cell, const double point[3], float density,
    float temperature, const float velocity[3])
{
  if(scene.physical_transfer.channel !=
     STELLAR_PHYSICAL_CHANNEL_OPTICAL_V071) {
    const ArepoStellarCellV073 &source = scene.cells[cell];
    const StellarPhysicalSampleV071 base = evaluateStellarPhysicalSampleV071(
        scene.transfer, point, density, temperature, velocity, 0.0f, 0.0f);
    float physicalValue = 0.0f;
    if(stellarPhysicalChannelRequiresAuxiliaryV072(
           scene.physical_transfer.channel)) {
      StellarAuxiliaryFieldsV072 auxiliary;
      for(int component = 0; component < 3; component++)
        auxiliary.magnetic_field_gauss[component] =
            source.magnetic_field_gauss[component];
      auxiliary.pressure_dyn_cm2 = source.pressure_dyn_cm2;
      auxiliary.sound_speed_cm_per_s = source.sound_speed_cm_per_s;
      const StellarExtendedPhysicalSampleV072 extended =
          evaluateStellarExtendedPhysicalSampleV072(
              scene.transfer, point, velocity, base, auxiliary);
      physicalValue = stellarPhysicalValueV072(
          base, extended, scene.physical_transfer.channel);
      if(scene.physical_optical.profile ==
         STELLAR_PHYSICAL_OPTICAL_LEGACY_V072)
        return evaluateStellarPhysicalOpticalV072(
            base, extended, scene.physical_transfer);
    } else {
      physicalValue = stellarPhysicalValueV071(
          base, scene.physical_transfer.channel);
      if(scene.physical_optical.profile ==
         STELLAR_PHYSICAL_OPTICAL_LEGACY_V072)
        return evaluateStellarPhysicalOpticalV071(
            base, scene.physical_transfer);
    }
    const StellarFeatureSampleV064 feature = evaluateStellarFeatureSampleV064(
        scene.transfer, point, density, temperature, velocity);
    return evaluateStellarPhysicalOpticalFromValueV076(
        physicalValue, base, feature, scene.physical_transfer,
        scene.physical_optical, scene.transfer);
  }
  return evaluateStellarOpticalSample(scene.transfer, point, density, temperature,
                                      velocity);
}

__host__ __device__ int gatherFirstRingNeighbors(const DeviceScene &scene,
                                                 int parent, int *handled,
                                                 uint32_t *status)
{
  int handledCount = 0;
  const uint64_t first = scene.offsets[parent];
  const uint64_t last = scene.offsets[parent + 1];
  for(uint64_t edgeIndex = first; edgeIndex < last; edgeIndex++) {
    const ArepoStellarEdgeV073 &edge = scene.edges[edgeIndex];
    const int candidate = edgeCell(edge);
    if(!edgeContributes(edge) || !validCell(scene, candidate) ||
       candidate == parent)
      continue;
    bool duplicate = false;
    for(int i = 0; i < handledCount; i++)
      if(handled[i] == candidate) {
        duplicate = true;
        break;
      }
    if(duplicate)
      continue;
    if(handledCount >= kMaxNeighbors) {
      *status |= kRayNeighborOverflow;
      return -1;
    }
    handled[handledCount++] = candidate;
  }
  return handledCount;
}

__global__ void buildNeighborCacheKernel(DeviceScene scene, int *counts, int *cache)
{
  const uint64_t cell = uint64_t(blockIdx.x) * blockDim.x + threadIdx.x;
  if(cell >= scene.num_cells)
    return;
  uint32_t status = 0;
  int *handled = cache + cell * kMaxNeighbors;
  const int count = gatherFirstRingNeighbors(scene, int(cell), handled, &status);
  counts[cell] = count < 0 || (status & kRayNeighborOverflow) ? -1 : count;
}

__global__ void compactNeighborCacheKernel(uint64_t numCells, const int *counts,
                                           const uint64_t *offsets,
                                           const int *fixedCache, int *compactCache)
{
  const uint64_t cell = uint64_t(blockIdx.x) * blockDim.x + threadIdx.x;
  if(cell >= numCells || counts[cell] <= 0)
    return;
  const int *source = fixedCache + cell * kMaxNeighbors;
  int *destination = compactCache + offsets[cell];
  for(int i = 0; i < counts[cell]; i++)
    destination[i] = source[i];
}

__host__ __device__ void copyCellValues(const ArepoStellarCellV073 &cell,
                                        float *density, float *temperature,
                                        float velocity[3])
{
  *density = cell.density_log10_plus_10;
  *temperature = cell.temperature_kelvin;
  for(int component = 0; component < 3; component++)
    velocity[component] = cell.velocity_cm_per_s[component];
}

__host__ __device__ bool interpolateReconstruction(
    const DeviceScene &scene, int parent, const double point[3],
    const int *handled, int handledCount, float *density, float *temperature,
    float velocity[3], uint32_t *status)
{
  const ArepoStellarCellV073 &parentCell = scene.cells[parent];
  if(scene.reconstruction_mode == STELLAR_RECONSTRUCTION_VORONOI) {
    copyCellValues(parentCell, density, temperature, velocity);
    return true;
  }
  if(fabs(point[0] - parentCell.position[0]) <= 1.0e-11 &&
     fabs(point[1] - parentCell.position[1]) <= 1.0e-11 &&
     fabs(point[2] - parentCell.position[2]) <= 1.0e-11) {
    copyCellValues(parentCell, density, temperature, velocity);
    return true;
  }

  float maximumNeighborDistance = 0.0f;
  if(scene.reconstruction_mode == STELLAR_RECONSTRUCTION_SPH) {
    for(int i = 0; i < handledCount; i++) {
      const ArepoStellarCellV073 &cell = scene.cells[handled[i]];
      const float dx = periodicDistance(cell.position[0], point[0], scene.box_size);
      const float dy = periodicDistance(cell.position[1], point[1], scene.box_size);
      const float dz = periodicDistance(cell.position[2], point[2], scene.box_size);
      const float distance = sqrtf(dx * dx + dy * dy + dz * dz);
      if(distance > maximumNeighborDistance)
        maximumNeighborDistance = distance;
    }
  }
  const float inverseSupport = maximumNeighborDistance > 0.0f ?
      scene.sph_support_factor / maximumNeighborDistance : 0.0f;
  StellarReconstructionAccumulator accumulator =
      stellarEmptyReconstructionAccumulator();
  for(int i = 0; i < handledCount; i++) {
    const ArepoStellarCellV073 &cell = scene.cells[handled[i]];
    const float dx = periodicDistance(cell.position[0], point[0], scene.box_size);
    const float dy = periodicDistance(cell.position[1], point[1], scene.box_size);
    const float dz = periodicDistance(cell.position[2], point[2], scene.box_size);
    const float distance = sqrtf(dx * dx + dy * dy + dz * dz);
    if(!(distance > 0.0f) || !isfinite(distance)) {
      copyCellValues(cell, density, temperature, velocity);
      return true;
    }
    const float weight = stellarReconstructionWeight(
        scene.reconstruction_mode, distance, inverseSupport, scene.idw_power);
    stellarAccumulateReconstruction(&accumulator, weight,
        cell.density_log10_plus_10, cell.temperature_kelvin,
        cell.velocity_cm_per_s);
  }

  const float pdx = periodicDistance(parentCell.position[0], point[0], scene.box_size);
  const float pdy = periodicDistance(parentCell.position[1], point[1], scene.box_size);
  const float pdz = periodicDistance(parentCell.position[2], point[2], scene.box_size);
  const float parentDistance = sqrtf(pdx * pdx + pdy * pdy + pdz * pdz);
  if(!(parentDistance > 0.0f) || !isfinite(parentDistance)) {
    copyCellValues(parentCell, density, temperature, velocity);
    return true;
  }
  const float parentWeight = stellarReconstructionWeight(
      scene.reconstruction_mode, parentDistance, inverseSupport,
      scene.idw_power);
  stellarAccumulateReconstruction(&accumulator, parentWeight,
      parentCell.density_log10_plus_10, parentCell.temperature_kelvin,
      parentCell.velocity_cm_per_s);
  if(!stellarNormalizeReconstruction(&accumulator)) {
    copyCellValues(parentCell, density, temperature, velocity);
    return true;
  }
  *density = accumulator.density;
  *temperature = accumulator.temperature;
  for(int component = 0; component < 3; component++)
    velocity[component] = accumulator.velocity[component];
  if(!isfinite(*density) || !isfinite(*temperature) ||
     !isfinite(velocity[0]) || !isfinite(velocity[1]) ||
     !isfinite(velocity[2])) {
    *status |= kRayNonFinite;
    return false;
  }
  return true;
}

__host__ __device__ RayResult renderRay(const DeviceScene &scene, uint64_t rayIndex)
{
  RayResult result;
  result.rgb[0] = result.rgb[1] = result.rgb[2] = 0.0f;
  result.cells = result.samples = result.status = 0;
  const ArepoStellarRayV073 &ray = scene.rays[rayIndex];
  if(!ray.active) {
    result.status = kRayInactive;
    return result;
  }
  if(!validCell(scene, ray.start_cell)) {
    result.status = kRayInvalidCell;
    return result;
  }

  int cell = ray.start_cell;
  double currentT = ray.t_min;
  const double boxExitT = ray.t_max;
  float transmittance = 1.0f;
  for(int iteration = 0; iteration < kMaxCellsPerRay && currentT < ray.t_max; iteration++) {
    if(!validCell(scene, cell)) {
      result.status |= kRayInvalidCell;
      break;
    }
    result.cells++;
    const double p0[3] = {
      ray.origin[0] + currentT * ray.direction[0],
      ray.origin[1] + currentT * ray.direction[1],
      ray.origin[2] + currentT * ray.direction[2]
    };
    double center[3] = {
      wrapPosition(scene.cells[cell].position[0], p0[0], scene.box_size),
      wrapPosition(scene.cells[cell].position[1], p0[1], scene.box_size),
      wrapPosition(scene.cells[cell].position[2], p0[2], scene.box_size)
    };

    double bestLength = std::numeric_limits<double>::infinity();
    int nextCell = -1;
    const uint64_t first = scene.offsets[cell];
    const uint64_t last = scene.offsets[cell + 1];
    for(uint64_t edgeIndex = first; edgeIndex < last; edgeIndex++) {
      const ArepoStellarEdgeV073 &edge = scene.edges[edgeIndex];
      const int candidateCell = edgeCell(edge);
      if(candidateCell == cell)
        continue;
      if(!validCell(scene, candidateCell)) {
        result.status |= kRayInvalidEdge;
        continue;
      }
      const ArepoStellarCellV073 &neighborCell = scene.cells[candidateCell];
      const double neighbor[3] = {
        wrapPosition(neighborCell.position[0], p0[0], scene.box_size),
        wrapPosition(neighborCell.position[1], p0[1], scene.box_size),
        wrapPosition(neighborCell.position[2], p0[2], scene.box_size)
      };
      double cdotq = 0.0;
      double ddotq = 0.0;
      for(int axis = 0; axis < 3; axis++) {
        const double q = neighbor[axis] - center[axis];
        const double midpoint = 0.5 * (neighbor[axis] + center[axis]);
        cdotq += (midpoint - p0[axis]) * q;
        ddotq += ray.direction[axis] * q;
      }
      double distance;
      if(cdotq > 0.0)
        distance = cdotq / ddotq;
      else if(ddotq > 0.0)
        distance = 0.0;
      else
        distance = std::numeric_limits<double>::infinity();
      if(distance >= 0.0 && distance < bestLength) {
        bestLength = distance;
        nextCell = candidateCell;
      }
    }

    if(nextCell < 0 || !isfinite(bestLength)) {
      result.status |= kRayNoExitFace;
      break;
    }
    double exitT = clampDouble(currentT + bestLength, ray.t_min, boxExitT);
    const double length = exitT - currentT;
    if(length > 1.0e-11) {
      const int handledCount = scene.neighbor_counts[cell];
      if(handledCount < 0) {
        result.status |= kRayNeighborOverflow;
        break;
      }
      const int *handled = scene.neighbor_cache + scene.neighbor_offsets[cell];
      const float step = float(length / scene.samples_per_cell);
      for(int sampleIndex = 0; sampleIndex < scene.samples_per_cell; sampleIndex++) {
        const double sampleT = currentT + (sampleIndex + 0.5) * step;
        const double point[3] = {
          ray.origin[0] + sampleT * ray.direction[0],
          ray.origin[1] + sampleT * ray.direction[1],
          ray.origin[2] + sampleT * ray.direction[2]
        };
        float density = 0.0f;
        float temperature = 0.0f;
        float velocity[3] = {0.0f, 0.0f, 0.0f};
        if(!interpolateReconstruction(scene, cell, point, handled, handledCount,
                                      &density, &temperature, velocity,
                                      &result.status))
          break;
        if(density < 0.0f)
          density = 0.0f;
        const StellarOpticalSample optical =
            evaluateTransfer(scene, cell, point, density, temperature, velocity);
        const StellarIntegratedSegment segment =
            integrateStellarOpticalSegment(optical, step);
        result.rgb[0] += transmittance * segment.radiance[0];
        result.rgb[1] += transmittance * segment.radiance[1];
        result.rgb[2] += transmittance * segment.radiance[2];
        transmittance *= segment.transmittance;
        result.samples++;
      }
    }
    if(result.status & (kRayNeighborOverflow | kRayNonFinite))
      break;
    const double clampedExitT = clampDouble(exitT, currentT, ray.t_max);
    if(fabs(clampedExitT - ray.t_max) <= 1.0e-11)
      break;
    if(clampedExitT <= currentT + 1.0e-11)
      currentT = clampDouble(currentT + 1.0e-11, currentT, ray.t_max);
    else
      currentT = clampedExitT;
    cell = nextCell;
    if(iteration + 1 == kMaxCellsPerRay)
      result.status |= kRayCellLimit;
  }
  if(!isfinite(result.rgb[0]) || !isfinite(result.rgb[1]) || !isfinite(result.rgb[2]))
    result.status |= kRayNonFinite;
  return result;
}

__global__ void renderKernel(DeviceScene scene, RayResult *results)
{
  const uint64_t index = uint64_t(blockIdx.x) * blockDim.x + threadIdx.x;
  if(index >= scene.num_rays)
    return;
  results[index] = renderRay(scene, index);
}

void writeTga(const std::string &filename, const std::vector<RayResult> &results,
              uint32_t width, uint32_t height, float exposure,
              float blackPoint, float saturation, float displayBrightness,
              int physicalOpticalProfile)
{
  unsigned char header[18] = {0};
  header[2] = 2;
  header[12] = width & 255;
  header[13] = (width >> 8) & 255;
  header[14] = height & 255;
  header[15] = (height >> 8) & 255;
  header[16] = 24;
  header[17] = 0x20;
  std::vector<unsigned char> pixels(results.size() * 3);
#pragma omp parallel for schedule(static)
  for(long long pixelIndex = 0; pixelIndex < static_cast<long long>(results.size()); pixelIndex++) {
    const size_t i = static_cast<size_t>(pixelIndex);
    float linearRgb[3];
    stellarDecodePhysicalMomentsV076(
        results[i].rgb, physicalOpticalProfile, linearRgb);
    float mapped[3];
    for(int channel = 0; channel < 3; channel++)
      mapped[channel] = stellarFilmicMap(
          std::max(0.0f, linearRgb[channel]), exposure, blackPoint);
    const float luma = 0.2126f * mapped[0] + 0.7152f * mapped[1] + 0.0722f * mapped[2];
    for(int channel = 0; channel < 3; channel++)
      mapped[channel] = clampFloat(
          displayBrightness *
              (luma + saturation * (mapped[channel] - luma)), 0.0f, 1.0f);
    pixels[i * 3 + 0] = stellarDisplayEncodeByteV053b(mapped[2]);
    pixels[i * 3 + 1] = stellarDisplayEncodeByteV053b(mapped[1]);
    pixels[i * 3 + 2] = stellarDisplayEncodeByteV053b(mapped[0]);
  }
  std::ofstream output(filename.c_str(), std::ios::binary);
  output.write(reinterpret_cast<const char *>(header), sizeof(header));
  output.write(reinterpret_cast<const char *>(&pixels[0]), pixels.size());
  if(!output.good())
    throw std::runtime_error("Failed writing TGA: " + filename);
}

void writeRaw(const std::string &filename, const std::vector<RayResult> &results)
{
  std::ofstream output(filename.c_str(), std::ios::binary);
  output.write(reinterpret_cast<const char *>(&results[0]), sizeof(RayResult) * results.size());
  if(!output.good())
    throw std::runtime_error("Failed writing raw results: " + filename);
}

template <typename T>
void allocateAndCopy(const std::vector<T> &host, T **device)
{
  CUDA_CHECK(cudaMalloc(reinterpret_cast<void **>(device), sizeof(T) * host.size()));
  CUDA_CHECK(cudaMemcpy(*device, &host[0], sizeof(T) * host.size(), cudaMemcpyHostToDevice));
}

ArepoStellarSceneHeaderV073 readSceneHeader(std::ifstream &input)
{
  ArepoStellarSceneHeaderV073 header;
  input.read(reinterpret_cast<char *>(&header), sizeof(header));
  if(!input.good())
    throw std::runtime_error("Cannot read scene header.");
  if(std::memcmp(header.magic, AREPO_STELLAR_SCENE_MAGIC_V073,
                 std::strlen(AREPO_STELLAR_SCENE_MAGIC_V073)) != 0 ||
     header.version != AREPO_STELLAR_SCENE_VERSION_V073 ||
     header.endian_marker != AREPO_STELLAR_SCENE_ENDIAN_MARKER_V073 ||
     header.header_bytes != sizeof(ArepoStellarSceneHeaderV073) ||
     header.cell_bytes != sizeof(ArepoStellarCellV073) ||
     header.edge_bytes != sizeof(ArepoStellarEdgeV073) ||
     header.ray_bytes != sizeof(ArepoStellarRayV073))
    throw std::runtime_error("Unsupported or corrupt GPU scene header.");
  const uint32_t requiredFlags = AREPO_STELLAR_REQUIRED_FIELD_FLAGS_V073 |
      AREPO_STELLAR_ZERO_LEGACY_ABSORPTION_V073;
  if((header.flags & requiredFlags) != requiredFlags)
    throw std::runtime_error("Scene does not match the v073 physical-field contract.");
  if(header.position_unit_cm != 1.0 || header.density_unit_cgs != 1.0 ||
     header.velocity_unit_cm_per_s != 1.0 ||
     header.temperature_unit_kelvin != 1.0 ||
     header.magnetic_field_unit_gauss != 1.0 ||
     header.pressure_unit_dyn_cm2 != 1.0 ||
     header.sound_speed_unit_cm_per_s != 1.0)
    throw std::runtime_error("v073 renderer requires canonical cgs scene values.");
  if(header.num_rays != uint64_t(header.sample_width) * header.sample_height)
    throw std::runtime_error("Scene ray dimensions are inconsistent.");
  if(header.samples_per_cell <= 0 || header.samples_per_cell > 64)
    throw std::runtime_error("Unsupported samples-per-cell value.");
  return header;
}

template <typename T>
void streamArrayToDevice(std::ifstream &input, uint64_t count, T **device)
{
  if(count == 0)
    throw std::runtime_error("Zero-length scene arrays are unsupported.");
  CUDA_CHECK(cudaMalloc(reinterpret_cast<void **>(device), sizeof(T) * count));
  const size_t targetChunkBytes = 8u * 1024u * 1024u;
  const size_t chunkElements = std::max<size_t>(1, targetChunkBytes / sizeof(T));
  std::vector<T> buffer(std::min<uint64_t>(count, chunkElements));
  uint64_t offset = 0;
  while(offset < count) {
    const size_t current = size_t(std::min<uint64_t>(count - offset, buffer.size()));
    input.read(reinterpret_cast<char *>(&buffer[0]), sizeof(T) * current);
    if(!input.good())
      throw std::runtime_error("Truncated GPU scene while streaming to the device.");
    CUDA_CHECK(cudaMemcpy(*device + offset, &buffer[0], sizeof(T) * current,
                          cudaMemcpyHostToDevice));
    offset += current;
  }
}

template <typename T>
void streamArrayIntoDevice(std::ifstream &input, uint64_t count, T *device)
{
  const size_t targetChunkBytes = 8u * 1024u * 1024u;
  const size_t chunkElements = std::max<size_t>(1, targetChunkBytes / sizeof(T));
  std::vector<T> buffer(std::min<uint64_t>(count, chunkElements));
  uint64_t offset = 0;
  while(offset < count) {
    const size_t current = size_t(std::min<uint64_t>(count - offset, buffer.size()));
    input.read(reinterpret_cast<char *>(&buffer[0]), sizeof(T) * current);
    if(!input.good())
      throw std::runtime_error("Truncated ray payload while streaming to the device.");
    CUDA_CHECK(cudaMemcpy(device + offset, &buffer[0], sizeof(T) * current,
                          cudaMemcpyHostToDevice));
    offset += current;
  }
}

struct ViewSpec {
  int index;
  std::string scene_path;
  std::string palette_path;
  std::string output_prefix;
  int transfer_mode;
  int palette_profile;
  int feature_profile;
  int reconstruction_mode;
  float idw_power;
  float sph_support_factor;
  double center[3];
  double axis[3];
  float bulk_velocity_cm_per_s[3];
  float material_radius_cm;
  float disk_radius_cm;
  float disk_half_thickness_cm;
  float polar_inner_cm;
  float polar_outer_cm;
  float polar_cone_ratio;
  float merger_extinction_per_cm;
  float disk_extinction_per_cm;
  float polar_extinction_per_cm;
  float merger_emissivity_per_cm;
  float disk_emissivity_per_cm;
  float polar_emissivity_per_cm;
  float exposure;
  float black_point;
  float saturation;
  std::string physical_channel_name;
  std::string physical_scale_name;
  std::string physical_optical_profile_name;
  StellarPhysicalTransferV071 physical_transfer;
  StellarPhysicalOpticalParametersV076 physical_optical;
  float display_brightness;
};

int parseTransferMode(const std::string &name)
{
  if(name == "merger")
    return STELLAR_TRANSFER_MERGER;
  if(name == "disk")
    return STELLAR_TRANSFER_DISK;
  if(name == "outflow")
    return STELLAR_TRANSFER_OUTFLOW;
  if(name == "composite")
    return STELLAR_TRANSFER_COMPOSITE;
  throw std::runtime_error("Unknown stellar transfer mode: " + name);
}

int parseReconstructionMode(const std::string &name)
{
  if(name == "sph")
    return STELLAR_RECONSTRUCTION_SPH;
  if(name == "idw")
    return STELLAR_RECONSTRUCTION_IDW;
  if(name == "voronoi")
    return STELLAR_RECONSTRUCTION_VORONOI;
  throw std::runtime_error("Unknown stellar reconstruction mode: " + name);
}

int parsePaletteProfile(const std::string &name)
{
  if(name == "legacy_v052")
    return STELLAR_PALETTE_LEGACY_V052;
  if(name == "copper_blue_v057")
    return STELLAR_PALETTE_COPPER_BLUE_V057;
  if(name == "copper_blue_accent_v058")
    return STELLAR_PALETTE_COPPER_BLUE_ACCENT_V058;
  if(name == "structure_flux_balanced_v068")
    return STELLAR_PALETTE_STRUCTURE_FLUX_BALANCED_V068;
  if(name == "structure_flux_vivid_v068")
    return STELLAR_PALETTE_STRUCTURE_FLUX_VIVID_V068;
  if(name == "structure_flux_layered_v070")
    return STELLAR_PALETTE_STRUCTURE_FLUX_LAYERED_V070;
  throw std::runtime_error("Unknown stellar palette profile: " + name);
}

int parseFeatureProfile(const std::string &name)
{
  if(name == "legacy_v064")
    return STELLAR_FEATURE_LEGACY_V064;
  if(name == "stellar_structures_v065")
    return STELLAR_FEATURE_STRUCTURES_V065;
  throw std::runtime_error("Unknown stellar feature profile: " + name);
}

std::vector<ViewSpec> loadViewManifest(const std::string &filename)
{
  std::ifstream input(filename.c_str());
  if(!input.good())
    throw std::runtime_error("Cannot open view manifest: " + filename);
  std::vector<ViewSpec> views;
  bool schemaSeen = false;
  std::string line;
  while(std::getline(input, line)) {
    if(line == "# schema=stellar_gpu_view_manifest_v053f") {
      schemaSeen = true;
      continue;
    }
    if(line.empty() || line[0] == '#')
      continue;
    if(!schemaSeen)
      throw std::runtime_error(
          "View manifest data precedes stellar_gpu_view_manifest_v053f schema.");
    std::istringstream parser(line);
    ViewSpec view;
    std::string transferMode;
    std::string paletteProfile;
    std::string featureProfile;
    std::string reconstructionMode;
    std::string physicalChannel;
    std::string physicalScale;
    std::string physicalOpticalProfile;
    if(!(parser >> view.index >> view.scene_path >> view.palette_path >> view.output_prefix >>
         transferMode >> paletteProfile >> featureProfile >> reconstructionMode >>
         view.idw_power >>
         view.sph_support_factor >> view.center[0] >> view.center[1] >>
         view.center[2] >>
         view.axis[0] >> view.axis[1] >> view.axis[2] >>
         view.bulk_velocity_cm_per_s[0] >> view.bulk_velocity_cm_per_s[1] >>
         view.bulk_velocity_cm_per_s[2] >> view.material_radius_cm >>
         view.disk_radius_cm >> view.disk_half_thickness_cm >>
         view.polar_inner_cm >> view.polar_outer_cm >> view.polar_cone_ratio >>
         view.merger_extinction_per_cm >> view.disk_extinction_per_cm >>
         view.polar_extinction_per_cm >> view.merger_emissivity_per_cm >>
         view.disk_emissivity_per_cm >> view.polar_emissivity_per_cm >>
         view.exposure >> view.black_point >> view.saturation >>
         physicalChannel >> physicalScale >>
         view.physical_transfer.range_min >>
         view.physical_transfer.range_max >>
         view.physical_transfer.symlog_linthresh >>
         view.physical_transfer.extinction_per_cm >>
         view.physical_transfer.emissivity_per_cm >>
         physicalOpticalProfile >>
         view.physical_optical.target_optical_depth >>
         view.physical_optical.target_emission >>
         view.physical_optical.reference_path_cm >>
         view.physical_optical.opacity_signal_threshold >>
         view.physical_optical.color_gamma >>
         view.physical_optical.color_invert >>
         view.physical_optical.density_support_log10_low >>
         view.physical_optical.density_support_log10_high >>
         view.physical_optical.emission_signal_floor >>
         view.display_brightness))
      throw std::runtime_error("Malformed view manifest row: " + line);
    view.transfer_mode = parseTransferMode(transferMode);
    view.palette_profile = parsePaletteProfile(paletteProfile);
    view.feature_profile = parseFeatureProfile(featureProfile);
    view.reconstruction_mode = parseReconstructionMode(reconstructionMode);
    view.physical_channel_name = physicalChannel;
    view.physical_scale_name = physicalScale;
    view.physical_optical_profile_name = physicalOpticalProfile;
    view.physical_transfer.channel =
        stellarPhysicalChannelFromNameV072(physicalChannel);
    view.physical_transfer.scale =
        stellarPhysicalScaleFromNameV071(physicalScale);
    view.physical_optical.profile =
        stellarPhysicalOpticalProfileFromNameV076(physicalOpticalProfile);
    std::string extra;
    if(parser >> extra)
      throw std::runtime_error("View manifest row has extra columns: " + line);
    if(view.index != int(views.size()))
      throw std::runtime_error("View manifest indices must be contiguous from zero.");
    if(!stellarGpuProfileContractValidV053f(
           view.transfer_mode, view.palette_profile, view.feature_profile,
           view.physical_optical.profile))
      throw std::runtime_error("Invalid stellar profile contract: " + line);
    if(view.physical_transfer.channel ==
           STELLAR_PHYSICAL_CHANNEL_INVALID_V071 ||
       view.physical_transfer.scale == STELLAR_PHYSICAL_SCALE_INVALID_V071)
      throw std::runtime_error("Invalid stellar physical channel contract: " + line);
    const bool physical = view.physical_transfer.channel !=
        STELLAR_PHYSICAL_CHANNEL_OPTICAL_V071;
    if(physical && view.reconstruction_mode != STELLAR_RECONSTRUCTION_VORONOI)
      throw std::runtime_error(
          "Physical GPU channels require native Voronoi reconstruction: " + line);
    if(physical &&
       (!(view.physical_transfer.range_max >
          view.physical_transfer.range_min) ||
        !(view.physical_transfer.symlog_linthresh > 0.0f) ||
        view.physical_transfer.extinction_per_cm < 0.0f ||
        !(view.physical_transfer.emissivity_per_cm > 0.0f)))
      throw std::runtime_error("Invalid physical transfer parameters: " + line);
    if(physical &&
       (view.physical_optical.profile ==
            STELLAR_PHYSICAL_OPTICAL_MATERIAL_SUPPORT_V074 ||
        view.physical_optical.profile ==
            STELLAR_PHYSICAL_OPTICAL_SEPARATED_SUPPORT_V075 ||
        view.physical_optical.profile ==
            STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076) &&
       (!(view.physical_optical.target_optical_depth > 0.0f) ||
        !(view.physical_optical.target_emission > 0.0f) ||
        view.physical_optical.reference_path_cm < 0.0f))
      throw std::runtime_error(
          "Invalid path-normalized physical optical parameters: " + line);
    if(physical && view.physical_optical.profile ==
           STELLAR_PHYSICAL_OPTICAL_SEPARATED_SUPPORT_V075 &&
       (!(view.physical_optical.opacity_signal_threshold > 0.0f) ||
        view.physical_optical.opacity_signal_threshold > 1.0f ||
        !(view.physical_optical.color_gamma > 0.0f) ||
        (view.physical_optical.color_invert != 0 &&
         view.physical_optical.color_invert != 1)))
      throw std::runtime_error(
          "Invalid separated_support_v075 parameters: " + line);
    if(physical && view.physical_optical.profile ==
           STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076 &&
       (!(view.physical_optical.density_support_log10_high >
          view.physical_optical.density_support_log10_low) ||
        view.physical_optical.emission_signal_floor < 0.0f ||
        view.physical_optical.emission_signal_floor > 1.0f ||
        !(view.physical_optical.color_gamma > 0.0f) ||
        (view.physical_optical.color_invert != 0 &&
         view.physical_optical.color_invert != 1)))
      throw std::runtime_error(
          "Invalid density_moment_v076 parameters: " + line);
    const double axisNorm = std::sqrt(view.axis[0] * view.axis[0] +
                                      view.axis[1] * view.axis[1] +
                                      view.axis[2] * view.axis[2]);
    if(!(axisNorm > 0.0) || !(view.idw_power > 0.0f) ||
       !(view.sph_support_factor > 0.0f) ||
       !(view.material_radius_cm > 0.0f) ||
       !(view.disk_radius_cm > 0.0f) || !(view.disk_half_thickness_cm > 0.0f) ||
       !(view.polar_inner_cm > 0.0f) ||
       !(view.polar_outer_cm > view.polar_inner_cm) ||
       !(view.polar_cone_ratio > 0.0f) ||
       view.merger_extinction_per_cm < 0.0f ||
       view.disk_extinction_per_cm < 0.0f ||
       view.polar_extinction_per_cm < 0.0f ||
       !(view.merger_emissivity_per_cm > 0.0f) ||
       !(view.disk_emissivity_per_cm > 0.0f) ||
       !(view.polar_emissivity_per_cm > 0.0f) ||
       !(view.exposure > 0.0f) || view.black_point < 0.0f ||
       view.saturation < 0.0f || !(view.display_brightness > 0.0f))
      throw std::runtime_error("Invalid stellar view parameters: " + line);
    for(int component = 0; component < 3; component++)
      view.axis[component] /= axisNorm;
    views.push_back(view);
  }
  if(!schemaSeen)
    throw std::runtime_error(
        "View manifest lacks stellar_gpu_view_manifest_v053f schema.");
  if(views.empty())
    throw std::runtime_error("View manifest has no views.");
  return views;
}

void validateRayHeader(const ArepoStellarSceneHeaderV073 &base,
                       const ArepoStellarSceneHeaderV073 &candidate,
                       bool expectRaysOnly)
{
  const bool raysOnly = (candidate.flags & AREPO_STELLAR_RAYS_ONLY_V073) != 0;
  if(raysOnly != expectRaysOnly)
    throw std::runtime_error("Scene payload kind does not match its view index.");
  if(candidate.num_cells != base.num_cells ||
     candidate.num_rays != base.num_rays ||
     candidate.sample_width != base.sample_width ||
     candidate.sample_height != base.sample_height ||
     candidate.source_width != base.source_width ||
     candidate.source_height != base.source_height ||
     candidate.samples_per_cell != base.samples_per_cell ||
     (candidate.flags & AREPO_STELLAR_REQUIRED_FIELD_FLAGS_V073) !=
         (base.flags & AREPO_STELLAR_REQUIRED_FIELD_FLAGS_V073) ||
     candidate.box_size != base.box_size ||
     candidate.ray_max_t != base.ray_max_t ||
     candidate.position_unit_cm != base.position_unit_cm ||
     candidate.density_unit_cgs != base.density_unit_cgs ||
     candidate.velocity_unit_cm_per_s != base.velocity_unit_cm_per_s ||
     candidate.temperature_unit_kelvin != base.temperature_unit_kelvin ||
     candidate.magnetic_field_unit_gauss != base.magnetic_field_unit_gauss ||
     candidate.pressure_unit_dyn_cm2 != base.pressure_unit_dyn_cm2 ||
     candidate.sound_speed_unit_cm_per_s != base.sound_speed_unit_cm_per_s ||
     candidate.snapshot_time_seconds != base.snapshot_time_seconds)
    throw std::runtime_error("Ray payload is incompatible with the resident mesh.");
  if(expectRaysOnly && candidate.num_edges != 0)
    throw std::runtime_error("Ray-only payload unexpectedly contains mesh edges.");
}

} // namespace

int main(int argc, char **argv)
{
  try {
    if(argc != 4) {
      std::cerr << "Usage: " << argv[0]
                << " view_manifest_v053f.tsv output_directory render_label\n";
      return 2;
    }
    const std::string manifestPath = argv[1];
    const std::string outputDirectory = argv[2];
    const std::string renderLabel = argv[3];
    const std::vector<ViewSpec> views = loadViewManifest(manifestPath);
    const bool writeRawResults = std::getenv("AREPORT_GPU_WRITE_RAW") != 0;

    int deviceId = 0;
    CUDA_CHECK(cudaGetDevice(&deviceId));
    cudaDeviceProp properties;
    CUDA_CHECK(cudaGetDeviceProperties(&properties, deviceId));
    const std::string deviceName(properties.name);
    const bool isA100 = properties.major == 8 && properties.minor == 0 &&
        deviceName.find("A100") != std::string::npos;
    const bool isA10 = properties.major == 8 && properties.minor == 6 &&
        deviceName.find("A10") != std::string::npos;
    const bool isA40 = properties.major == 8 && properties.minor == 6 &&
        deviceName.find("A40") != std::string::npos;
    const bool is2080Ti = properties.major == 7 && properties.minor == 5 &&
        deviceName.find("2080 Ti") != std::string::npos;
    const bool is3090 = properties.major == 8 && properties.minor == 6 &&
        deviceName.find("3090") != std::string::npos;
    const bool isL40S = properties.major == 8 && properties.minor == 9 &&
        deviceName.find("L40S") != std::string::npos;
    if(!isA100 && !isA10 && !isA40 && !is2080Ti && !is3090 && !isL40S)
      throw std::runtime_error(
          "v053f supports only validated A100, A10, A40, RTX 2080 Ti, RTX 3090, and L40S devices.");
    const std::string deviceClass = isA100 ? "A100" :
        isA10 ? "A10" : isA40 ? "A40" : is2080Ti ? "RTX2080Ti" :
        is3090 ? "RTX3090Ti" : "L40S";
    int threads = isA100 ? 256 : 128;
    if(const char *overrideValue = std::getenv("AREPO_GPU_RENDER_THREADS")) {
      char *end = 0;
      const long parsed = std::strtol(overrideValue, &end, 10);
      if(!end || *end != '\0' || (parsed != 64 && parsed != 128 && parsed != 256))
        throw std::runtime_error("AREPO_GPU_RENDER_THREADS must be 64, 128, or 256.");
      threads = int(parsed);
    }

    const auto loadStart = std::chrono::steady_clock::now();
    std::ifstream firstInput(views[0].scene_path.c_str(), std::ios::binary);
    if(!firstInput.good())
      throw std::runtime_error("Cannot open base scene: " + views[0].scene_path);
    const ArepoStellarSceneHeaderV073 baseHeader = readSceneHeader(firstInput);
    validateRayHeader(baseHeader, baseHeader, false);
    if(baseHeader.num_cells == 0 || baseHeader.num_edges == 0)
      throw std::runtime_error("Base scene has no resident mesh.");

    ArepoStellarCellV073 *deviceCells = 0;
    uint64_t *deviceOffsets = 0;
    ArepoStellarEdgeV073 *deviceEdges = 0;
    ArepoStellarRayV073 *deviceRays = 0;
    RayResult *deviceResults = 0;
    int *deviceNeighborCounts = 0;
    uint64_t *deviceNeighborOffsets = 0;
    int *deviceNeighborScratch = 0;
    int *deviceNeighborCache = 0;
    const auto staticTransferStart = std::chrono::steady_clock::now();
    streamArrayToDevice(firstInput, baseHeader.num_cells, &deviceCells);
    streamArrayToDevice(firstInput, baseHeader.num_cells + 1, &deviceOffsets);
    streamArrayToDevice(firstInput, baseHeader.num_edges, &deviceEdges);
    streamArrayToDevice(firstInput, baseHeader.num_rays, &deviceRays);
    firstInput.close();
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void **>(&deviceResults),
                          sizeof(RayResult) * baseHeader.num_rays));
    const auto staticTransferEnd = std::chrono::steady_clock::now();

    DeviceScene scene = {};
    scene.cells = deviceCells;
    scene.offsets = deviceOffsets;
    scene.edges = deviceEdges;
    scene.rays = deviceRays;
    scene.num_cells = baseHeader.num_cells;
    scene.num_rays = baseHeader.num_rays;
    scene.samples_per_cell = baseHeader.samples_per_cell;
    scene.box_size = baseHeader.box_size;
    scene.ray_max_t = baseHeader.ray_max_t;

    if(baseHeader.num_cells > uint64_t(std::numeric_limits<size_t>::max() /
                                       (sizeof(int) * kMaxNeighbors)))
      throw std::runtime_error("Neighbor cache is too large for this host.");
    const size_t neighborCacheEntries = size_t(baseHeader.num_cells) * kMaxNeighbors;
    const auto neighborCacheStart = std::chrono::steady_clock::now();
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void **>(&deviceNeighborCounts),
                          sizeof(int) * baseHeader.num_cells));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void **>(&deviceNeighborScratch),
                          sizeof(int) * neighborCacheEntries));
    scene.neighbor_counts = deviceNeighborCounts;
    const int cacheThreads = 128;
    const int cacheBlocks = int((scene.num_cells + cacheThreads - 1) / cacheThreads);
    buildNeighborCacheKernel<<<cacheBlocks, cacheThreads>>>(scene, deviceNeighborCounts,
                                                           deviceNeighborScratch);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    std::vector<int> neighborCounts(baseHeader.num_cells);
    CUDA_CHECK(cudaMemcpy(&neighborCounts[0], deviceNeighborCounts,
                          sizeof(int) * neighborCounts.size(), cudaMemcpyDeviceToHost));
    int maximumNeighborCount = 0;
    uint64_t totalNeighborCount = 0;
    uint64_t overflowCellCount = 0;
    std::vector<uint64_t> neighborOffsets(baseHeader.num_cells + 1, 0);
    for(size_t i = 0; i < neighborCounts.size(); i++) {
      if(neighborCounts[i] < 0) {
        overflowCellCount++;
      } else {
        maximumNeighborCount = std::max(maximumNeighborCount, neighborCounts[i]);
        totalNeighborCount += neighborCounts[i];
      }
      neighborOffsets[i + 1] = totalNeighborCount;
    }
    if(totalNeighborCount > uint64_t(std::numeric_limits<size_t>::max() / sizeof(int)))
      throw std::runtime_error("Compact neighbor cache is too large for this host.");
    allocateAndCopy(neighborOffsets, &deviceNeighborOffsets);
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void **>(&deviceNeighborCache),
                          sizeof(int) * size_t(totalNeighborCount)));
    compactNeighborCacheKernel<<<cacheBlocks, cacheThreads>>>(
        scene.num_cells, deviceNeighborCounts, deviceNeighborOffsets,
        deviceNeighborScratch, deviceNeighborCache);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    CUDA_CHECK(cudaFree(deviceNeighborScratch));
    deviceNeighborScratch = 0;
    scene.neighbor_offsets = deviceNeighborOffsets;
    scene.neighbor_cache = deviceNeighborCache;
    const auto neighborCacheEnd = std::chrono::steady_clock::now();
    const double neighborCacheSeconds =
        std::chrono::duration<double>(neighborCacheEnd - neighborCacheStart).count();
    const uint64_t compactNeighborCacheBytes = sizeof(int) * totalNeighborCount +
        sizeof(int) * baseHeader.num_cells + sizeof(uint64_t) * (baseHeader.num_cells + 1);

    std::vector<RayResult> gpuResults(baseHeader.num_rays);
    const int blocks = int((scene.num_rays + threads - 1) / threads);
    cudaEvent_t kernelStart, kernelEnd;
    CUDA_CHECK(cudaEventCreate(&kernelStart));
    CUDA_CHECK(cudaEventCreate(&kernelEnd));
    double totalKernelSeconds = 0.0;
    double totalOutputSeconds = 0.0;
    bool allViewsValid = true;

    for(size_t viewNumber = 0; viewNumber < views.size(); viewNumber++) {
      const ViewSpec &view = views[viewNumber];
      ArepoStellarSceneHeaderV073 viewHeader = baseHeader;
      const auto rayTransferStart = std::chrono::steady_clock::now();
      const bool reuseBaseRays = viewNumber != 0 &&
          view.scene_path == views[0].scene_path;
      if(viewNumber != 0 && !reuseBaseRays) {
        std::ifstream rayInput(view.scene_path.c_str(), std::ios::binary);
        if(!rayInput.good())
          throw std::runtime_error("Cannot open ray payload: " + view.scene_path);
        viewHeader = readSceneHeader(rayInput);
        validateRayHeader(baseHeader, viewHeader, true);
        streamArrayIntoDevice(rayInput, viewHeader.num_rays, deviceRays);
        rayInput.close();
      }
      const auto rayTransferEnd = std::chrono::steady_clock::now();

      std::vector<float> palette = loadPalette(view.palette_path);
      float *devicePalette = 0;
      allocateAndCopy(palette, &devicePalette);
      scene.palette = devicePalette;
      scene.palette_len = int(palette.size() / 4);
      scene.reconstruction_mode = view.reconstruction_mode;
      scene.idw_power = view.idw_power;
      scene.sph_support_factor = view.sph_support_factor;
      scene.camera_origin[0] = viewHeader.camera_origin[0];
      scene.camera_origin[1] = viewHeader.camera_origin[1];
      scene.camera_origin[2] = viewHeader.camera_origin[2];
      scene.transfer.mode = view.transfer_mode;
      scene.transfer.palette_profile = view.palette_profile;
      scene.transfer.feature_profile = view.feature_profile;
      scene.transfer.box_size = baseHeader.box_size;
      for(int axis = 0; axis < 3; axis++) {
        scene.transfer.center[axis] = view.center[axis];
        scene.transfer.axis[axis] = view.axis[axis];
        scene.transfer.bulk_velocity_cm_per_s[axis] =
            view.bulk_velocity_cm_per_s[axis];
      }
      scene.transfer.material_radius_cm = view.material_radius_cm;
      scene.transfer.disk_radius_cm = view.disk_radius_cm;
      scene.transfer.disk_half_thickness_cm = view.disk_half_thickness_cm;
      scene.transfer.polar_inner_cm = view.polar_inner_cm;
      scene.transfer.polar_outer_cm = view.polar_outer_cm;
      scene.transfer.polar_cone_ratio = view.polar_cone_ratio;
      scene.transfer.merger_extinction_per_cm = view.merger_extinction_per_cm;
      scene.transfer.disk_extinction_per_cm = view.disk_extinction_per_cm;
      scene.transfer.polar_extinction_per_cm = view.polar_extinction_per_cm;
      scene.transfer.merger_emissivity_per_cm = view.merger_emissivity_per_cm;
      scene.transfer.disk_emissivity_per_cm = view.disk_emissivity_per_cm;
      scene.transfer.polar_emissivity_per_cm = view.polar_emissivity_per_cm;
      scene.physical_transfer = view.physical_transfer;
      scene.physical_optical = view.physical_optical;
      scene.exposure = view.exposure;
      scene.black_point = view.black_point;
      scene.saturation = view.saturation;
      scene.display_brightness = view.display_brightness;

      CUDA_CHECK(cudaEventRecord(kernelStart));
      renderKernel<<<blocks, threads>>>(scene, deviceResults);
      CUDA_CHECK(cudaGetLastError());
      CUDA_CHECK(cudaEventRecord(kernelEnd));
      CUDA_CHECK(cudaEventSynchronize(kernelEnd));
      float kernelMilliseconds = 0.0f;
      CUDA_CHECK(cudaEventElapsedTime(&kernelMilliseconds, kernelStart, kernelEnd));
      totalKernelSeconds += kernelMilliseconds / 1000.0;

      const auto downloadStart = std::chrono::steady_clock::now();
      CUDA_CHECK(cudaMemcpy(&gpuResults[0], deviceResults,
                            sizeof(RayResult) * gpuResults.size(), cudaMemcpyDeviceToHost));
      const auto downloadEnd = std::chrono::steady_clock::now();

      uint64_t statusCounts[STELLAR_GPU_RAY_STATUS_COUNT_V053B] = {0};
      uint64_t totalCells = 0;
      uint64_t totalSamples = 0;
      double rawRgbSum = 0.0;
      float rawRgbMaximum = 0.0f;
      bool viewValid = true;
      for(size_t i = 0; i < gpuResults.size(); i++) {
        totalCells += gpuResults[i].cells;
        totalSamples += gpuResults[i].samples;
        for(int channel = 0; channel < 3; channel++) {
          rawRgbSum += gpuResults[i].rgb[channel];
          rawRgbMaximum = std::max(rawRgbMaximum, gpuResults[i].rgb[channel]);
        }
        viewValid = viewValid &&
            !stellarGpuRayStatusIsFatalV053b(gpuResults[i].status);
        for(size_t bit = 0; bit < STELLAR_GPU_RAY_STATUS_COUNT_V053B; bit++)
          if(gpuResults[i].status & (1u << bit))
            statusCounts[bit]++;
      }
      viewValid = viewValid && stellarGpuStatusCountsValidV053b(
          statusCounts, STELLAR_GPU_RAY_STATUS_COUNT_V053B);
      allViewsValid = allViewsValid && viewValid;

      const std::string outputPrefix = outputDirectory + "/" + view.output_prefix;
      const auto outputStart = std::chrono::steady_clock::now();
      if(writeRawResults)
        writeRaw(outputPrefix + ".raw", gpuResults);
      writeTga(outputPrefix + ".tga", gpuResults,
               baseHeader.sample_width, baseHeader.sample_height,
               view.exposure, view.black_point, view.saturation,
               view.display_brightness, view.physical_optical.profile);
      const auto outputEnd = std::chrono::steady_clock::now();
      const double outputSeconds =
          std::chrono::duration<double>(outputEnd - outputStart).count();
      totalOutputSeconds += outputSeconds;
      std::ofstream report((outputPrefix + ".benchmark.txt").c_str());
      report << std::setprecision(10);
      report << "renderer_version=stellar_v053f\n";
      report << "view_manifest_schema=stellar_gpu_view_manifest_v053f\n";
      report << "display_encoding=arepo_pow_1_over_2_3\n";
      report << "inactive_rays_are_valid=true\n";
      report << "face_geometry=double_cell_positions\n";
      report << "scene_format_version=" << baseHeader.version << "\n";
      report << "render_threads=" << threads << "\n";
      report << "render_label=" << renderLabel << "\n";
      report << "view_index=" << view.index << "\n";
      report << "device=" << properties.name << "\n";
      report << "device_class=" << deviceClass << "\n";
      report << "compute_capability=" << properties.major << "." << properties.minor << "\n";
      report << "base_scene=" << views[0].scene_path << "\n";
      report << "ray_scene=" << view.scene_path << "\n";
      report << "resident_mesh_reused=true\n";
      report << "resident_rays_reused=" << (reuseBaseRays ? "true" : "false")
             << "\n";
      report << "palette=" << view.palette_path << "\n";
      report << "transfer_mode=" << view.transfer_mode << "\n";
      report << "palette_profile="
             << stellarPaletteProfileName(view.palette_profile) << "\n";
      report << "palette_profile_id=" << view.palette_profile << "\n";
      report << "feature_profile="
             << stellarFeatureProfileNameV065(view.feature_profile) << "\n";
      report << "feature_profile_id=" << view.feature_profile << "\n";
      report << "reconstruction_mode="
             << stellarReconstructionModeName(view.reconstruction_mode) << "\n";
      report << "idw_power=" << view.idw_power << "\n";
      report << "sph_support_factor=" << view.sph_support_factor << "\n";
      report << "feature_center=" << view.center[0] << "," << view.center[1]
             << "," << view.center[2] << "\n";
      report << "feature_axis=" << view.axis[0] << "," << view.axis[1]
             << "," << view.axis[2] << "\n";
      report << "bulk_velocity_cm_per_s=" << view.bulk_velocity_cm_per_s[0]
             << "," << view.bulk_velocity_cm_per_s[1] << ","
             << view.bulk_velocity_cm_per_s[2] << "\n";
      report << "material_radius_cm=" << view.material_radius_cm << "\n";
      report << "disk_radius_cm=" << view.disk_radius_cm << "\n";
      report << "disk_half_thickness_cm=" << view.disk_half_thickness_cm << "\n";
      report << "polar_inner_cm=" << view.polar_inner_cm << "\n";
      report << "polar_outer_cm=" << view.polar_outer_cm << "\n";
      report << "polar_cone_ratio=" << view.polar_cone_ratio << "\n";
      report << "fixed_exposure=" << view.exposure << "\n";
      report << "fixed_black_point=" << view.black_point << "\n";
      report << "fixed_saturation=" << view.saturation << "\n";
      report << "physical_channel=" << view.physical_channel_name << "\n";
      report << "physical_channel_id=" << view.physical_transfer.channel << "\n";
      report << "physical_scale=" << view.physical_scale_name << "\n";
      report << "physical_scale_id=" << view.physical_transfer.scale << "\n";
      report << "physical_range=" << view.physical_transfer.range_min << ","
             << view.physical_transfer.range_max << "\n";
      report << "physical_symlog_linthresh="
             << view.physical_transfer.symlog_linthresh << "\n";
      report << "physical_opacity="
             << view.physical_transfer.extinction_per_cm << "\n";
      report << "physical_emission="
             << view.physical_transfer.emissivity_per_cm << "\n";
      report << "physical_optical_profile="
             << view.physical_optical_profile_name << "\n";
      report << "physical_target_optical_depth="
             << view.physical_optical.target_optical_depth << "\n";
      report << "physical_target_emission="
             << view.physical_optical.target_emission << "\n";
      report << "physical_reference_path_cm="
             << stellarPhysicalReferencePathV074(
                    view.physical_transfer.channel, scene.transfer,
                    view.physical_optical.reference_path_cm) << "\n";
      report << "physical_zero_signal_transparent="
             << (view.physical_optical.profile !=
                     STELLAR_PHYSICAL_OPTICAL_LEGACY_V072 &&
                 view.physical_optical.profile !=
                     STELLAR_PHYSICAL_OPTICAL_DENSITY_MOMENT_V076 ?
                 "true" : "false") << "\n";
      report << "physical_opacity_signal_threshold="
             << view.physical_optical.opacity_signal_threshold << "\n";
      report << "physical_color_gamma="
             << view.physical_optical.color_gamma << "\n";
      report << "physical_color_invert="
             << view.physical_optical.color_invert << "\n";
      report << "physical_density_support_log10="
             << view.physical_optical.density_support_log10_low << ","
             << view.physical_optical.density_support_log10_high << "\n";
      report << "physical_emission_signal_floor="
             << view.physical_optical.emission_signal_floor << "\n";
      report << "physical_color_aggregation="
             << (stellarPhysicalOpticalUsesMomentsV076(
                     view.physical_optical.profile) ?
                 "post_ray_scalar_moment" : "per_sample_rgb") << "\n";
      report << "display_brightness=" << view.display_brightness << "\n";
      report << "physical_palette=copper_blue\n";
      report << "raw_rgb_mean="
             << rawRgbSum / (3.0 * gpuResults.size()) << "\n";
      report << "raw_rgb_max=" << rawRgbMaximum << "\n";
      report << "cells=" << baseHeader.num_cells << "\n";
      report << "edges=" << baseHeader.num_edges << "\n";
      report << "rays=" << baseHeader.num_rays << "\n";
      report << "samples_per_cell=" << baseHeader.samples_per_cell << "\n";
      report << "neighbor_cache_seconds=" << neighborCacheSeconds << "\n";
      report << "neighbor_cache_bytes=" << compactNeighborCacheBytes << "\n";
      report << "neighbor_scratch_bytes=" << sizeof(int) * neighborCacheEntries << "\n";
      report << "neighbor_count_max=" << maximumNeighborCount << "\n";
      report << "neighbor_count_total=" << totalNeighborCount << "\n";
      report << "neighbor_overflow_cells=" << overflowCellCount << "\n";
      report << "static_host_to_device_seconds="
             << std::chrono::duration<double>(staticTransferEnd - staticTransferStart).count() << "\n";
      report << "ray_host_to_device_seconds="
             << std::chrono::duration<double>(rayTransferEnd - rayTransferStart).count() << "\n";
      report << "kernel_seconds=" << kernelMilliseconds / 1000.0 << "\n";
      report << "device_to_host_seconds="
             << std::chrono::duration<double>(downloadEnd - downloadStart).count() << "\n";
      report << "host_output_seconds=" << outputSeconds << "\n";
      report << "total_cells=" << totalCells << "\n";
      report << "total_samples=" << totalSamples << "\n";
      report << "status_inactive=" << statusCounts[0] << "\n";
      report << "status_invalid_cell=" << statusCounts[1] << "\n";
      report << "status_invalid_edge=" << statusCounts[2] << "\n";
      report << "status_neighbor_overflow=" << statusCounts[3] << "\n";
      report << "status_no_exit_face=" << statusCounts[4] << "\n";
      report << "status_cell_limit=" << statusCounts[5] << "\n";
      report << "status_nonfinite=" << statusCounts[6] << "\n";
      report << "view_valid=" << (viewValid ? "true" : "false") << "\n";
      report.close();
      CUDA_CHECK(cudaFree(devicePalette));

      std::cout << "GPU_VIEW_OK index=" << view.index
                << " prefix=" << view.output_prefix
                << " palette="
                << stellarPaletteProfileName(view.palette_profile)
                << " feature="
                << stellarFeatureProfileNameV065(view.feature_profile)
                << " physical=" << view.physical_channel_name
                << " optical=" << view.physical_optical_profile_name
                << " kernel_seconds=" << kernelMilliseconds / 1000.0
                << " valid=" << (viewValid ? 1 : 0) << "\n";
    }

    std::ofstream summary((outputDirectory + "/render_summary.txt").c_str());
    summary << std::setprecision(10);
    summary << "renderer_version=stellar_v053f\n";
    summary << "view_manifest_schema=stellar_gpu_view_manifest_v053f\n";
    summary << "profile_contract=manifest_bound\n";
    summary << "physical_channel_contract=v072_all_24\n";
    summary << "physical_optical_contract=v076_legacy_v072_through_density_moment_v076\n";
    summary << "physical_palette=copper_blue\n";
    summary << "display_encoding=arepo_pow_1_over_2_3\n";
    summary << "inactive_rays_are_valid=true\n";
    summary << "face_geometry=double_cell_positions\n";
    summary << "scene_format_version=" << baseHeader.version << "\n";
    summary << "render_threads=" << threads << "\n";
    summary << "render_label=" << renderLabel << "\n";
    summary << "device=" << properties.name << "\n";
    summary << "device_class=" << deviceClass << "\n";
    summary << "view_count=" << views.size() << "\n";
    summary << "resolution=" << baseHeader.sample_width << "x" << baseHeader.sample_height << "\n";
    summary << "mesh_loaded_once=true\n";
    summary << "same_scene_multi_channel_reuse=true\n";
    summary << "compact_ray_bytes=" << sizeof(ArepoStellarRayV073) << "\n";
    summary << "neighbor_cache_seconds=" << neighborCacheSeconds << "\n";
    summary << "neighbor_cache_bytes=" << compactNeighborCacheBytes << "\n";
    summary << "neighbor_scratch_bytes=" << sizeof(int) * neighborCacheEntries << "\n";
    summary << "neighbor_count_max=" << maximumNeighborCount << "\n";
    summary << "neighbor_count_total=" << totalNeighborCount << "\n";
    summary << "neighbor_overflow_cells=" << overflowCellCount << "\n";
    summary << "scene_open_seconds="
            << std::chrono::duration<double>(staticTransferStart - loadStart).count() << "\n";
    summary << "static_host_to_device_seconds="
            << std::chrono::duration<double>(staticTransferEnd - staticTransferStart).count() << "\n";
    summary << "total_kernel_seconds=" << totalKernelSeconds << "\n";
    summary << "total_host_output_seconds=" << totalOutputSeconds << "\n";
    summary << "all_views_valid=" << (allViewsValid ? "true" : "false") << "\n";
    summary.close();

    CUDA_CHECK(cudaEventDestroy(kernelStart));
    CUDA_CHECK(cudaEventDestroy(kernelEnd));
    CUDA_CHECK(cudaFree(deviceNeighborCache));
    CUDA_CHECK(cudaFree(deviceNeighborOffsets));
    CUDA_CHECK(cudaFree(deviceNeighborCounts));
    CUDA_CHECK(cudaFree(deviceResults));
    CUDA_CHECK(cudaFree(deviceRays));
    CUDA_CHECK(cudaFree(deviceEdges));
    CUDA_CHECK(cudaFree(deviceOffsets));
    CUDA_CHECK(cudaFree(deviceCells));

    std::cout << "GPU_RENDER_OK device=" << properties.name
              << " class=" << deviceClass
              << " views=" << views.size()
              << " total_kernel_seconds=" << totalKernelSeconds
              << " mesh_loaded_once=true\n";
    return allViewsValid ? 0 : 3;
  } catch(const std::exception &error) {
    std::cerr << "GPU_RENDER_ERROR " << error.what() << "\n";
    return 1;
  }
}
