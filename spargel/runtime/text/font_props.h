#pragma once

namespace spargel::text {

// This follows CSS Fonts Module Level 3.
// TODO(tianjiao): CSS Fonts Module Level 4 introduces oblique angle.
enum FontStyle {
  normal,
  italic,
  oblique,
};

// light    300
// normal   400
// bold     700
struct FontWeight {
  float value;
};

// absolute keyword value:
//      condensed    75%
//      normal       100%
//      expanded     125%
struct FontWidth {
  float value;
};

struct FontProps {
  FontStyle style;
  FontWeight weight;
  FontWidth width;
};

}  // namespace spargel::text
