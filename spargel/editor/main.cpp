#include "spargel/runtime/ui/painter.h"
#include "spargel/runtime/ui/window.h"

namespace ui = spargel::runtime::ui;

int main() {
    auto window = ui::Window::create();
    [[maybe_unused]] auto painter = ui::Painter::create();
    window->set_title("Spargel Editor");
    window->show();
    return 0;
}
