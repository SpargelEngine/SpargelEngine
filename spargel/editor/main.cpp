#include <stdio.h>

#include "spargel/runtime/window.h"

int main() {
    printf("SpargelEditor is under development.\n");
    auto window = spargel::runtime::Window::create();
    window->show();
    return 0;
}
