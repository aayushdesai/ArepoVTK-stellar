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

#include "stellar_gpu_scene_format_v041.h"
#include "stellar_render_model_v051a.h"

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
  kRayInactive = 1u << 0,
  kRayInvalidCell = 1u << 1,
  kRayInvalidEdge = 1u << 2,
  kRayNeighborOverflow = 1u << 3,
  kRayNoExitFace = 1u << 4,
  kRayCellLimit = 1u << 5,
  kRayNonFinite = 1u << 6
};

struct SceneData {
  ArepoGpuSceneHeader header;
  std::vector<ArepoGpuCell> cells;
  std::vector<uint64_t> offsets;
  std::vector<ArepoGpuEdge> edges;
  std::vector<ArepoGpuRay> rays;
};

struct DeviceScene {
  const ArepoGpuCell *cells;
  const uint64_t *offsets;
  const ArepoGpuEdge *edges;
  const ArepoGpuRay *rays;
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
  StellarTransferParameters transfer;
  float exposure;
  float black_point;
  float saturation;
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
  if(std::memcmp(scene.header.magic, AREPO_GPU_SCENE_MAGIC,
                 std::strlen(AREPO_GPU_SCENE_MAGIC)) != 0 ||
     scene.header.version != AREPO_GPU_SCENE_VERSION ||
     scene.header.endian_marker != 0x01020304u ||
     scene.header.header_bytes != sizeof(ArepoGpuSceneHeader) ||
     scene.header.cell_bytes != sizeof(ArepoGpuCell) ||
     scene.header.edge_bytes != sizeof(ArepoGpuEdge) ||
     scene.header.ray_bytes != sizeof(ArepoGpuRay))
    throw std::runtime_error("Unsupported or corrupt GPU scene header.");
  const uint32_t requiredFlags = AREPO_GPU_DENSITY_LOG10_PLUS_10 |
      AREPO_GPU_TEMPERATURE_KELVIN | AREPO_GPU_NO_GHOST_CONTRIBS |
      AREPO_GPU_NATURAL_NEIGHBOR_INNER | AREPO_GPU_ZERO_ABSORPTION;
  if((scene.header.flags & requiredFlags) != requiredFlags)
    throw std::runtime_error("Scene does not match the v039b production physics contract.");
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
  const double delta = value - reference;
  if(delta > 0.5 * box)
    value -= box;
  if(delta < -0.5 * box)
    value += box;
  return value;
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
                                        const ArepoGpuRay &ray,
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

__host__ __device__ inline int edgeCell(const ArepoGpuEdge &edge)
{
  const uint32_t encoded = edge.packed_neighbor & 0x7fffffffu;
  return encoded == 0 ? -1 : int(encoded - 1u);
}

__host__ __device__ inline bool edgeContributes(const ArepoGpuEdge &edge)
{
  return (edge.packed_neighbor & 0x80000000u) == 0;
}

__host__ __device__ StellarOpticalSample evaluateTransfer(
    const DeviceScene &scene, const double point[3], float density,
    float temperature)
{
  return evaluateStellarOpticalSample(scene.transfer, point, density, temperature);
}

__host__ __device__ int gatherTwoRingNeighbors(const DeviceScene &scene, int parent,
                                               int *handled, uint32_t *status)
{
  int handledCount = 0;
  const uint64_t first = scene.offsets[parent];
  const uint64_t last = scene.offsets[parent + 1];
  for(uint64_t outer = first; outer < last; outer++) {
    const ArepoGpuEdge &outerEdge = scene.edges[outer];
    const int outerCell = edgeCell(outerEdge);
    if(!edgeContributes(outerEdge) || !validCell(scene, outerCell))
      continue;
    const uint64_t innerFirst = scene.offsets[outerCell];
    const uint64_t innerLast = scene.offsets[outerCell + 1];
    for(uint64_t inner = innerFirst; inner < innerLast; inner++) {
      const ArepoGpuEdge &innerEdge = scene.edges[inner];
      const int candidate = edgeCell(innerEdge);
      if(!edgeContributes(innerEdge) || !validCell(scene, candidate) || candidate == parent)
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
  const int count = gatherTwoRingNeighbors(scene, int(cell), handled, &status);
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

__host__ __device__ bool interpolateIdw(const DeviceScene &scene, int parent,
                                        const double point[3], const int *handled,
                                        int handledCount,
                                        float *density, float *temperature,
                                        uint32_t *status)
{
  const ArepoGpuCell &parentCell = scene.cells[parent];
  if(fabs(point[0] - parentCell.position[0]) <= 1.0e-11 &&
     fabs(point[1] - parentCell.position[1]) <= 1.0e-11 &&
     fabs(point[2] - parentCell.position[2]) <= 1.0e-11) {
    *density = parentCell.density;
    *temperature = parentCell.temperature;
    return true;
  }

  float densitySum = 0.0f;
  float temperatureSum = 0.0f;
  float weightSum = 0.0f;
  for(int i = 0; i < handledCount; i++) {
    const ArepoGpuCell &cell = scene.cells[handled[i]];
    const float dx = periodicDistance(cell.position[0], point[0], scene.box_size);
    const float dy = periodicDistance(cell.position[1], point[1], scene.box_size);
    const float dz = periodicDistance(cell.position[2], point[2], scene.box_size);
    const float distanceSquared = dx * dx + dy * dy + dz * dz;
    if(!(distanceSquared > 0.0f) || !isfinite(distanceSquared)) {
      *density = cell.density;
      *temperature = cell.temperature;
      return true;
    }
    const float weight = 1.0f / powf(sqrtf(distanceSquared), 2.0f);
    weightSum += weight;
    densitySum += weight * cell.density;
    temperatureSum += weight * cell.temperature;
  }

  const float pdx = periodicDistance(parentCell.position[0], point[0], scene.box_size);
  const float pdy = periodicDistance(parentCell.position[1], point[1], scene.box_size);
  const float pdz = periodicDistance(parentCell.position[2], point[2], scene.box_size);
  const float parentDistanceSquared = pdx * pdx + pdy * pdy + pdz * pdz;
  if(!(parentDistanceSquared > 0.0f) || !isfinite(parentDistanceSquared)) {
    *density = parentCell.density;
    *temperature = parentCell.temperature;
    return true;
  }
  const float parentWeight = 1.0f / powf(sqrtf(parentDistanceSquared), 2.0f);
  weightSum += parentWeight;
  densitySum += parentWeight * parentCell.density;
  temperatureSum += parentWeight * parentCell.temperature;
  if(!(weightSum > 0.0f) || !isfinite(weightSum)) {
    *status |= kRayNonFinite;
    return false;
  }
  *density = densitySum / weightSum;
  *temperature = temperatureSum / weightSum;
  return isfinite(*density) && isfinite(*temperature);
}

__host__ __device__ RayResult renderRay(const DeviceScene &scene, uint64_t rayIndex)
{
  RayResult result;
  result.rgb[0] = result.rgb[1] = result.rgb[2] = 0.0f;
  result.cells = result.samples = result.status = 0;
  const ArepoGpuRay &ray = scene.rays[rayIndex];
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
      const ArepoGpuEdge &edge = scene.edges[edgeIndex];
      const int candidateCell = edgeCell(edge);
      if(candidateCell == cell)
        continue;
      if(!validCell(scene, candidateCell)) {
        result.status |= kRayInvalidEdge;
        continue;
      }
      const double neighbor[3] = {
        center[0] + edge.neighbor_delta[0],
        center[1] + edge.neighbor_delta[1],
        center[2] + edge.neighbor_delta[2]
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
        if(!interpolateIdw(scene, cell, point, handled, handledCount,
                           &density, &temperature, &result.status))
          break;
        if(density < 0.0f)
          density = 0.0f;
        const StellarOpticalSample optical =
            evaluateTransfer(scene, point, density, temperature);
        const float alpha = 1.0f - expf(-optical.extinction_per_cm * step);
        if(alpha > 0.0f) {
          result.rgb[0] += transmittance * alpha * optical.color[0];
          result.rgb[1] += transmittance * alpha * optical.color[1];
          result.rgb[2] += transmittance * alpha * optical.color[2];
          transmittance *= 1.0f - alpha;
        }
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
              float blackPoint, float saturation)
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
    float mapped[3];
    for(int channel = 0; channel < 3; channel++)
      mapped[channel] = stellarFilmicMap(
          std::max(0.0f, results[i].rgb[channel]), exposure, blackPoint);
    const float luma = 0.2126f * mapped[0] + 0.7152f * mapped[1] + 0.0722f * mapped[2];
    for(int channel = 0; channel < 3; channel++)
      mapped[channel] = clampFloat(
          luma + saturation * (mapped[channel] - luma), 0.0f, 1.0f);
    pixels[i * 3 + 0] = static_cast<unsigned char>(255.0f * mapped[2]);
    pixels[i * 3 + 1] = static_cast<unsigned char>(255.0f * mapped[1]);
    pixels[i * 3 + 2] = static_cast<unsigned char>(255.0f * mapped[0]);
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

ArepoGpuSceneHeader readSceneHeader(std::ifstream &input)
{
  ArepoGpuSceneHeader header;
  input.read(reinterpret_cast<char *>(&header), sizeof(header));
  if(!input.good())
    throw std::runtime_error("Cannot read scene header.");
  if(std::memcmp(header.magic, AREPO_GPU_SCENE_MAGIC,
                 std::strlen(AREPO_GPU_SCENE_MAGIC)) != 0 ||
     header.version != AREPO_GPU_SCENE_VERSION ||
     header.endian_marker != 0x01020304u ||
     header.header_bytes != sizeof(ArepoGpuSceneHeader) ||
     header.cell_bytes != sizeof(ArepoGpuCell) ||
     header.edge_bytes != sizeof(ArepoGpuEdge) ||
     header.ray_bytes != sizeof(ArepoGpuRay))
    throw std::runtime_error("Unsupported or corrupt GPU scene header.");
  const uint32_t requiredFlags = AREPO_GPU_DENSITY_LOG10_PLUS_10 |
      AREPO_GPU_TEMPERATURE_KELVIN | AREPO_GPU_NO_GHOST_CONTRIBS |
      AREPO_GPU_NATURAL_NEIGHBOR_INNER | AREPO_GPU_ZERO_ABSORPTION;
  if((header.flags & requiredFlags) != requiredFlags)
    throw std::runtime_error("Scene does not match the v039f production physics contract.");
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
  double center[3];
  double axis[3];
  float disk_radius_cm;
  float disk_half_thickness_cm;
  float polar_inner_cm;
  float polar_outer_cm;
  float polar_cone_ratio;
  float exposure;
  float black_point;
  float saturation;
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

std::vector<ViewSpec> loadViewManifest(const std::string &filename)
{
  std::ifstream input(filename.c_str());
  if(!input.good())
    throw std::runtime_error("Cannot open view manifest: " + filename);
  std::vector<ViewSpec> views;
  std::string line;
  while(std::getline(input, line)) {
    if(line.empty() || line[0] == '#')
      continue;
    std::istringstream parser(line);
    ViewSpec view;
    std::string transferMode;
    if(!(parser >> view.index >> view.scene_path >> view.palette_path >> view.output_prefix >>
         transferMode >> view.center[0] >> view.center[1] >> view.center[2] >>
         view.axis[0] >> view.axis[1] >> view.axis[2] >>
         view.disk_radius_cm >> view.disk_half_thickness_cm >>
         view.polar_inner_cm >> view.polar_outer_cm >> view.polar_cone_ratio >>
         view.exposure >> view.black_point >> view.saturation))
      throw std::runtime_error("Malformed view manifest row: " + line);
    view.transfer_mode = parseTransferMode(transferMode);
    std::string extra;
    if(parser >> extra)
      throw std::runtime_error("View manifest row has extra columns: " + line);
    if(view.index != int(views.size()))
      throw std::runtime_error("View manifest indices must be contiguous from zero.");
    views.push_back(view);
  }
  if(views.empty())
    throw std::runtime_error("View manifest has no views.");
  return views;
}

void validateRayHeader(const ArepoGpuSceneHeader &base,
                       const ArepoGpuSceneHeader &candidate,
                       bool expectRaysOnly)
{
  const bool raysOnly = (candidate.flags & AREPO_GPU_RAYS_ONLY) != 0;
  if(raysOnly != expectRaysOnly)
    throw std::runtime_error("Scene payload kind does not match its view index.");
  if(candidate.num_cells != base.num_cells ||
     candidate.num_rays != base.num_rays ||
     candidate.sample_width != base.sample_width ||
     candidate.sample_height != base.sample_height ||
     candidate.source_width != base.source_width ||
     candidate.source_height != base.source_height ||
     candidate.samples_per_cell != base.samples_per_cell ||
     candidate.box_size != base.box_size ||
     candidate.ray_max_t != base.ray_max_t)
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
                << " view_manifest.tsv output_directory render_label\n";
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
          "v050a supports only validated A100, A10, A40, RTX 2080 Ti, RTX 3090, and L40S devices.");
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
    const ArepoGpuSceneHeader baseHeader = readSceneHeader(firstInput);
    validateRayHeader(baseHeader, baseHeader, false);
    if(baseHeader.num_cells == 0 || baseHeader.num_edges == 0)
      throw std::runtime_error("Base scene has no resident mesh.");

    ArepoGpuCell *deviceCells = 0;
    uint64_t *deviceOffsets = 0;
    ArepoGpuEdge *deviceEdges = 0;
    ArepoGpuRay *deviceRays = 0;
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

    DeviceScene scene;
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
      ArepoGpuSceneHeader viewHeader = baseHeader;
      const auto rayTransferStart = std::chrono::steady_clock::now();
      if(viewNumber != 0) {
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
      scene.camera_origin[0] = viewHeader.camera_origin[0];
      scene.camera_origin[1] = viewHeader.camera_origin[1];
      scene.camera_origin[2] = viewHeader.camera_origin[2];
      scene.transfer.mode = view.transfer_mode;
      scene.transfer.box_size = baseHeader.box_size;
      for(int axis = 0; axis < 3; axis++) {
        scene.transfer.center[axis] = view.center[axis];
        scene.transfer.axis[axis] = view.axis[axis];
      }
      scene.transfer.disk_radius_cm = view.disk_radius_cm;
      scene.transfer.disk_half_thickness_cm = view.disk_half_thickness_cm;
      scene.transfer.polar_inner_cm = view.polar_inner_cm;
      scene.transfer.polar_outer_cm = view.polar_outer_cm;
      scene.transfer.polar_cone_ratio = view.polar_cone_ratio;
      scene.exposure = view.exposure;
      scene.black_point = view.black_point;
      scene.saturation = view.saturation;

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

      uint64_t statusCounts[7] = {0};
      uint64_t totalCells = 0;
      uint64_t totalSamples = 0;
      double rawRgbSum = 0.0;
      float rawRgbMaximum = 0.0f;
      for(size_t i = 0; i < gpuResults.size(); i++) {
        totalCells += gpuResults[i].cells;
        totalSamples += gpuResults[i].samples;
        for(int channel = 0; channel < 3; channel++) {
          rawRgbSum += gpuResults[i].rgb[channel];
          rawRgbMaximum = std::max(rawRgbMaximum, gpuResults[i].rgb[channel]);
        }
        for(int bit = 0; bit < 7; bit++)
          if(gpuResults[i].status & (1u << bit))
            statusCounts[bit]++;
      }
      bool viewValid = true;
      for(int bit = 0; bit < 7; bit++)
        viewValid = viewValid && statusCounts[bit] == 0;
      allViewsValid = allViewsValid && viewValid;

      const std::string outputPrefix = outputDirectory + "/" + view.output_prefix;
      const auto outputStart = std::chrono::steady_clock::now();
      if(writeRawResults)
        writeRaw(outputPrefix + ".raw", gpuResults);
      writeTga(outputPrefix + ".tga", gpuResults,
               baseHeader.sample_width, baseHeader.sample_height,
               view.exposure, view.black_point, view.saturation);
      const auto outputEnd = std::chrono::steady_clock::now();
      const double outputSeconds =
          std::chrono::duration<double>(outputEnd - outputStart).count();
      totalOutputSeconds += outputSeconds;
      std::ofstream report((outputPrefix + ".benchmark.txt").c_str());
      report << std::setprecision(10);
      report << "renderer_version=stellar_v051a\n";
      report << "render_threads=" << threads << "\n";
      report << "render_label=" << renderLabel << "\n";
      report << "view_index=" << view.index << "\n";
      report << "device=" << properties.name << "\n";
      report << "device_class=" << deviceClass << "\n";
      report << "compute_capability=" << properties.major << "." << properties.minor << "\n";
      report << "base_scene=" << views[0].scene_path << "\n";
      report << "ray_scene=" << view.scene_path << "\n";
      report << "palette=" << view.palette_path << "\n";
      report << "transfer_mode=" << view.transfer_mode << "\n";
      report << "feature_center=" << view.center[0] << "," << view.center[1]
             << "," << view.center[2] << "\n";
      report << "feature_axis=" << view.axis[0] << "," << view.axis[1]
             << "," << view.axis[2] << "\n";
      report << "disk_radius_cm=" << view.disk_radius_cm << "\n";
      report << "disk_half_thickness_cm=" << view.disk_half_thickness_cm << "\n";
      report << "polar_inner_cm=" << view.polar_inner_cm << "\n";
      report << "polar_outer_cm=" << view.polar_outer_cm << "\n";
      report << "polar_cone_ratio=" << view.polar_cone_ratio << "\n";
      report << "fixed_exposure=" << view.exposure << "\n";
      report << "fixed_black_point=" << view.black_point << "\n";
      report << "fixed_saturation=" << view.saturation << "\n";
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
                << " kernel_seconds=" << kernelMilliseconds / 1000.0
                << " valid=" << (viewValid ? 1 : 0) << "\n";
    }

    std::ofstream summary((outputDirectory + "/render_summary.txt").c_str());
    summary << std::setprecision(10);
    summary << "renderer_version=stellar_v051a\n";
    summary << "render_threads=" << threads << "\n";
    summary << "render_label=" << renderLabel << "\n";
    summary << "device=" << properties.name << "\n";
    summary << "device_class=" << deviceClass << "\n";
    summary << "view_count=" << views.size() << "\n";
    summary << "resolution=" << baseHeader.sample_width << "x" << baseHeader.sample_height << "\n";
    summary << "mesh_loaded_once=true\n";
    summary << "compact_ray_bytes=" << sizeof(ArepoGpuRay) << "\n";
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
