#pragma once

#define CHECK(expr)    \
    do {               \
        if (!(expr)) { \
            throw;     \
        }              \
    } while (0)
