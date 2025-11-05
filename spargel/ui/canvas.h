#pragma once

namespace spargel::ui {

class PlatformCanvas {
public:
  virtual ~PlatformCanvas() = default;
  virtual void draw_text(char const* text, float x, float y) = 0;
};

}  // namespace spargel::ui
