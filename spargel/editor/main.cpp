#include <stdlib.h>

#include "spargel/runtime/ui/painter.h"
#include "spargel/runtime/ui/window.h"

namespace spargel::editor {
namespace ui = spargel::runtime::ui;

class EditorApp : public ui::WindowDelegate {
public:
    EditorApp() {
        window_ = ui::Window::create();
        window_->set_title("Spargel Editor");
        window_->set_delegate(this);
        painter_ = ui::Painter::create();
        painter_->bind_window(window_);
        cmdlist_.set_clip(0, 0, 500, 500);

        for (int i = 0; i < 1280; i++) {
            int x = rand() % 500;
            int y = rand() % 500;
            int r = rand() % 20;
            uint32_t c = rand();
            cmdlist_.fill_circle(x * 1.0f, y * 1.0f, r * 1.0f, c);
        }
    }

    void run() { window_->show(); }

    void on_render() override { painter_->render(cmdlist_); }

private:
    ui::Window* window_ = nullptr;
    ui::Painter* painter_ = nullptr;
    ui::CommandList cmdlist_;
};
}  // namespace spargel::editor

int main() {
    spargel::editor::EditorApp app;
    app.run();
    return 0;
}
