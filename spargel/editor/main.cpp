#include <stdio.h>

#include "spargel/runtime/window.h"

int main() {
    printf("SpargelEditor is under development.\n");
    auto _ = spargel::runtime::Window::create();
    return 0;
}
