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
            cmdlist_.fill_circle(0, 0, 100, 0xFFFF0000);
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
