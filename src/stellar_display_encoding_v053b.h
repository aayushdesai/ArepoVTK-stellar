#ifndef AREPO_VTK_STELLAR_DISPLAY_ENCODING_V053B_H
#define AREPO_VTK_STELLAR_DISPLAY_ENCODING_V053B_H

#include <cmath>
#include <stdint.h>

// Keep GPU products byte-compatible with ArepoVTK's established PNG/TGA
// display encoding in fileio_img.cpp.
inline uint8_t stellarDisplayEncodeByteV053b(float linear_value)
{
  const float encoded = 255.0f * powf(linear_value, 1.0f / 2.3f);
  const float clamped = encoded < 0.0f ? 0.0f :
      (encoded > 255.0f ? 255.0f : encoded);
  return static_cast<uint8_t>(clamped);
}

#endif
