#pragma once

#include <vector>

#include "spargel/math/vecmath.h"

namespace spargel::text {

class Font;
class FontManager;

struct TextRun {
  // these are offset in the buffers in TextLine
  size_t start;
  size_t end;
  Font* font;
  float width;
};

struct TextLine {
  float ascent;
  float descent;
  float leading;
  float width;
  std::vector<TextRun> runs;
  std::vector<uint32_t> glyphs;
  std::vector<math::Vec2f> points;
};

TextLine shape_line(char const* text, Font* font, FontManager* manager);

}  // namespace spargel::text
