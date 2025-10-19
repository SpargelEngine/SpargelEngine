#pragma once

#include "spargel/math/vector.h"

namespace spargel::math {
    struct RectSize {
        float width;
        float height;
    };
    struct Rect {
        Vector2f origin;
        RectSize size;

        bool contains(Vector2f p) const {
            return (p.x >= origin.x) && (p.y >= origin.y) &&
                   (p.x <= origin.x + size.width) &&
                   (p.y <= origin.y + size.height);
        }
    };
}  // namespace spargel::math
