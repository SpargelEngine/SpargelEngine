#pragma once

#include "spargel/math/vector.h"

namespace spargel::math {
    struct Line {
        Vector2f start;
        Vector2f end;

        float length() const { return (start - end).length(); }
    };
}  // namespace spargel::math
