#pragma once

#include <CoreText/CoreText.h>

#include "spargel/runtime/text/font.h"

namespace spargel::text {

class FontMac final : public Font {
public:
  explicit FontMac(CTFontRef font);
  ~FontMac();

  Bitmap rasterize_glyph(uint32_t id, float scale,
                         math::Vec2f subpixel_position) override;
  GlyphInfo glyph_info(uint32_t id) override;

  CTFontRef get() { return font_; }

  void dump_info();

private:
  CTFontRef font_;
};

}  // namespace spargel::text
