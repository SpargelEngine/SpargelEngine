#include "spargel/text/font.h"

#include "spargel/base/bit_cast.h"

//
#include <stdio.h>

namespace spargel::text {
    void Bitmap::dump() {
        for (usize i = 0; i < height; i++) {
            for (usize j = 0; j < width; j++) {
                auto value = base::bit_cast<base::Byte, u8>(data[i * width + j]);
                int code = value * (255 - 232) / 256 + 232;
                printf("\033[38;5;196;48;5;%dm ", code);
            }
            printf("\033[0m\n");
        }
    }
}  // namespace spargel::text
