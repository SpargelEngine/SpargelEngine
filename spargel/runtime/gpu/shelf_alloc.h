#pragma once

#include <cstddef>

namespace spargel::gpu {

class ShelfAlloc {
public:
  size_t width() const { return width_; }
  size_t height() const { return height_; }

private:
  size_t width_ = 0;
  size_t height_ = 0;
};

}  // namespace spargel::gpu
