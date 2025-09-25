#include "spargel/logging/logging.h"
#include "spargel/ui/platform.h"
#include "spargel/ui/window.h"

using namespace spargel;

class Delegate final : public ui::WindowDelegate {
public:
    void onKeyboard(ui::KeyboardEvent& e) override {
        switch (e.action) {
        case ui::KeyboardAction::press:
            spargel_log_info("key press:  \t '%c'", e.toChar());
            break;
        case ui::KeyboardAction::release:
            spargel_log_info("key release:\t '%c'", e.toChar());
            break;
        }
    }
    void onMouseDown(float x, float y) override {
        spargel_log_info("mouse down : %.3f %.3f", x, y);
    }
};

int main() {
    auto platform = ui::makePlatform();
    auto window = platform->makeWindow(500, 500);
    window->setTitle("Spargel Demo - Window");
    window->setAnimating(false);
    Delegate d;
    window->setDelegate(&d);
    platform->startLoop();
    return 0;
}
