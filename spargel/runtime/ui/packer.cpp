#include "spargel/runtime/ui/packer.h"

namespace spargel::ui {
namespace {
size_t max(size_t a, size_t b) { return a > b ? a : b; }
inline constexpr size_t GAP = 2;
}  // namespace

std::optional<PackResult> AtlasPacker::pack(size_t width, size_t height) {
  if (current_column_ + GAP + width >= width_) {
    current_row_ = next_row_;
    current_column_ = 0;
  }
  size_t x = current_column_ + GAP;
  current_column_ += GAP + width;
  next_row_ = max(next_row_, current_row_ + height);
  if (next_row_ + GAP >= height_) {
    return std::nullopt;
  }
  size_t y = current_row_ + GAP;
  return PackResult{x, y};
}

}  // namespace spargel::ui
