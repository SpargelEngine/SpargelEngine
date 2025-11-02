#pragma once

#include <stdlib.h>

#include <optional>

namespace spargel::ui {

struct PackResult {
  size_t x;
  size_t y;
};
// An online rect packer.
//
// Currently this is a dummy implementation.
//
// TODO: Support deletion and reusing space.
//
class AtlasPacker {
public:
  AtlasPacker(size_t width, size_t height)
      : width_{width},
        height_{height},
        current_row_{},
        current_column_{},
        next_row_{} {}

  size_t width() const { return width_; }
  size_t height() const { return height_; }

  std::optional<PackResult> pack(size_t width, size_t height);

private:
  size_t width_;
  size_t height_;

  size_t current_row_;
  size_t current_column_;
  size_t next_row_;
};

}  // namespace spargel::ui
