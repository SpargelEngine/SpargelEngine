#include "spargel/runtime/painter.h"
#include "spargel/runtime/window.h"

int main() {
    auto window = spargel::runtime::Window::create();
    [[maybe_unused]] auto painter = spargel::runtime::Painter::create();
    window->set_title("Spargel Editor");
    window->show();
    return 0;
}
