#pragma once

#include <vector>

#include "spargel/runtime/math/vecmath.h"

namespace spargel::text {

struct Bitmap {
  uint16_t width;
  uint16_t height;
  std::vector<char> data;

  // debug
  void dump();
};

struct GlyphInfo {
  // The bounding box of the glyph.
  math::Vec4f bounding_box;
  // The origin of the next glyph.
  float horizontal_advance;
  // The (signed) distance to the top of the bounding box.
  float ascent() const { return bounding_box.y + bounding_box.w; }
  // The (signed) distance to the bottom of the bounding box.
  float descent() const { return bounding_box.y; }
  float width() const { return bounding_box.z; }
  float height() const { return bounding_box.w; }
};

// A concrete, resolved font face.
class Font {
public:
  virtual ~Font() = default;

  virtual Bitmap rasterize_glyph(uint32_t id, float scale,
                                 math::Vec2f subpixel_position) = 0;
  virtual GlyphInfo glyph_info(uint32_t id) = 0;
};

}  // namespace spargel::text
