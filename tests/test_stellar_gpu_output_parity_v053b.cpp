#include "stellar_display_encoding_v053b.h"
#include "stellar_render_model_v052a.h"

#include <assert.h>
#include <cmath>
#include <iostream>

namespace {

uint8_t nativeReferenceByte(float value)
{
  const float encoded = 255.0f * powf(value, 1.0f / 2.3f);
  const float clamped = encoded < 0.0f ? 0.0f :
      (encoded > 255.0f ? 255.0f : encoded);
  return static_cast<uint8_t>(clamped);
}

void postprocess(const float linear[3], float exposure, float black_point,
                 float saturation, uint8_t output[3], uint8_t obsolete[3])
{
  float mapped[3];
  for(int channel = 0; channel < 3; channel++)
    mapped[channel] = stellarFilmicMap(
        linear[channel] > 0.0f ? linear[channel] : 0.0f,
        exposure, black_point);
  const float luma = 0.2126f * mapped[0] + 0.7152f * mapped[1] +
      0.0722f * mapped[2];
  for(int channel = 0; channel < 3; channel++) {
    const float saturated = stellarClamp(
        luma + saturation * (mapped[channel] - luma), 0.0f, 1.0f);
    output[channel] = stellarDisplayEncodeByteV053b(saturated);
    obsolete[channel] = static_cast<uint8_t>(255.0f * saturated);
    assert(output[channel] == nativeReferenceByte(saturated));
  }
}

} // namespace

int main()
{
  const float scalar_fixture[] = {
      0.0f, 1.0e-6f, 5.0e-4f, 2.0e-3f, 1.0e-2f,
      1.0e-1f, 5.0e-1f, 1.0f, 2.0f};
  for(size_t index = 0; index < sizeof(scalar_fixture) / sizeof(scalar_fixture[0]);
      index++)
    assert(stellarDisplayEncodeByteV053b(scalar_fixture[index]) ==
           nativeReferenceByte(scalar_fixture[index]));

  const float pixels[][3] = {
      {0.0f, 0.0f, 0.0f},
      {3.0e-4f, 1.0e-4f, 4.0e-5f},
      {1.0e-3f, 2.0e-3f, 8.0e-3f},
      {7.81500712e-3f, 2.0e-3f, 8.0e-4f},
      {2.0e-2f, 5.0e-2f, 1.0e-1f},
      {2.5e-1f, 5.0e-1f, 1.0f}};
  uint64_t encoded_flux = 0;
  uint64_t obsolete_linear_flux = 0;
  uint64_t faint_encoded_flux = 0;
  uint64_t faint_obsolete_linear_flux = 0;
  size_t changed_channels = 0;
  for(size_t pixel = 0; pixel < sizeof(pixels) / sizeof(pixels[0]); pixel++) {
    uint8_t encoded[3];
    uint8_t obsolete[3];
    postprocess(pixels[pixel], 2.2f, 5.0e-4f, 0.84f, encoded, obsolete);
    for(int channel = 0; channel < 3; channel++) {
      encoded_flux += encoded[channel];
      obsolete_linear_flux += obsolete[channel];
      if(pixel < 4) {
        faint_encoded_flux += encoded[channel];
        faint_obsolete_linear_flux += obsolete[channel];
      }
      if(encoded[channel] != obsolete[channel])
        changed_channels++;
    }
  }
  assert(changed_channels >= 12);
  assert(encoded_flux > obsolete_linear_flux);
  assert(faint_encoded_flux > 3 * faint_obsolete_linear_flux);

  std::cout << "STELLAR_GPU_OUTPUT_PARITY_V053B_OK pixels=6 changed_channels="
            << changed_channels << " encoded_flux=" << encoded_flux
            << " obsolete_linear_flux=" << obsolete_linear_flux
            << " faint_encoded_flux=" << faint_encoded_flux
            << " faint_obsolete_linear_flux=" << faint_obsolete_linear_flux
            << "\n";
  return 0;
}
