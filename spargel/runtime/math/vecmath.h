#pragma once

namespace spargel::math {

template <typename T>
struct Vec2 {
  T x, y;
};

template <typename T>
struct Vec4 {
  T x, y, z, w;
};

using Vec2f = Vec2<float>;
using Vec4f = Vec4<float>;

}  // namespace spargel::math
