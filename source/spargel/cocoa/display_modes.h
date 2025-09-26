#pragma once

#include "spargel/base/types.h"
#include "spargel/base/vector.h"

namespace spargel::cocoa {
    struct DisplayMode {
        static void query(base::Vector<DisplayMode>& modes);

        usize width;
        usize height;
    };
}  // namespace spargel::cocoa
