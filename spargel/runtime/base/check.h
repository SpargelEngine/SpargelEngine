#pragma once

#define CHECK(expr)     \
  do {                  \
    if (!(expr)) {      \
      __builtin_trap(); \
    }                   \
  } while (0)
