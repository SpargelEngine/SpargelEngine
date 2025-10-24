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

        cmdlist_.fill_tri({0, 0}, {100, 200}, {200, 100}, 0xFFFF0000);
        cmdlist_.stroke_line({0, 0}, {500, 500}, 2.0, 0xFF00FF00);

        for (int i = 1; i < 10; i++) {
            cmdlist_.stroke_line({0, 10.0f * i}, {500, 10.0f * i}, 1.0,
                                 0xFFCCCCCC);
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
