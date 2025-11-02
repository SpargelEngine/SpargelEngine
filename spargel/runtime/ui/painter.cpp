#include "spargel/runtime/ui/painter.h"

#include "spargel/runtime/ui/context.h"

namespace spargel::ui {

constexpr float SCALE = 2.0;

void CommandList::fill_text(char const* text, math::Vec2f orig, uint32_t c) {
  auto& ctx = Context::get();
  auto shaped_line = ctx.layout_text(text);
  for (auto& run : shaped_line.runs) {
    for (size_t i = run.start; i < run.end; i++) {
      auto glyph = shaped_line.glyphs[i];
      auto position = shaped_line.points[i];

      auto glyph_info = run.font->glyph_info(glyph);
      auto const& bbox = glyph_info.bounding_box;

      math::Vec2f subpixel_position;
      math::Vec2f integral_position;
      subpixel_position.x =
          modff((orig.x + position.x + bbox.x) * SCALE, &integral_position.x);
      subpixel_position.y = 1.0f - modff((orig.y - position.y - bbox.y) * SCALE,
                                         &integral_position.y);
      auto handle = ctx.prepare_glyph(glyph, run.font, subpixel_position);
      if (handle.width > 0 && handle.height > 0) {
        sample_texture(
            math::Vec2f{(integral_position.x - 1) / SCALE,
                        (integral_position.y + 2 - handle.height) / SCALE},
            math::Vec2f{handle.width / SCALE, handle.height / SCALE},
            math::Vec2f{(float)handle.x / 256.0f, (float)handle.y / 256.0f},
            math::Vec2f{((float)handle.x + handle.width) / 256.0f,
                        ((float)handle.y + handle.height) / 256.0f},
            c);
      }
    }
  }
}

MeasureResult CommandList::measure_text(char const* text) {
  auto& ctx = Context::get();
  auto shaped_line = ctx.layout_text(text);
  return {shaped_line.width, shaped_line.ascent, shaped_line.descent};
}

void CommandList::sample_texture(math::Vec2f orig, math::Vec2f size,
                                 math::Vec2f uv_min, math::Vec2f uv_max,
                                 uint32_t c) {
  DrawVertex v1, v2, v3, v4;
  v1.position = orig;
  v2.position = {orig.x, orig.y + size.y};
  v3.position = {orig.x + size.x, orig.y + size.y};
  v4.position = {orig.x + size.x, orig.y};
  v1.color = v2.color = v3.color = v4.color = c;
  v1.tex_coord = uv_min;
  v2.tex_coord = {uv_min.x, uv_max.y};
  v3.tex_coord = {uv_max.x, uv_max.y};
  v4.tex_coord = {uv_max.x, uv_min.y};
  uint32_t offset = uint32_t(vertices_.size());
  vertices_.push_back(v1);
  vertices_.push_back(v2);
  vertices_.push_back(v3);
  vertices_.push_back(v4);
  indices_.push_back(offset);
  indices_.push_back(offset + 1);
  indices_.push_back(offset + 2);
  indices_.push_back(offset);
  indices_.push_back(offset + 2);
  indices_.push_back(offset + 3);
}

}  // namespace spargel::ui
