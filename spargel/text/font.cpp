#include "spargel/text/font.h"

#include <stdio.h>

namespace spargel::text {

void Bitmap::dump() {
  for (uint16_t i = 0; i < height; i++) {
    for (uint16_t j = 0; j < width; j++) {
      char value = data[i * width + j];
      int code = value * (255 - 232) / 256 + 232;
      printf("\033[38;5;196;48;5;%dm ", code);
    }
    printf("\033[0m\n");
  }
}

}  // namespace spargel::text
