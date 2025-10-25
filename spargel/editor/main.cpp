#include "spargel/runtime/init.h"
#include "spargel/runtime/ui/context.h"

namespace ui = spargel::runtime::ui;

namespace spargel::editor {
class EditorApp : public ui::RenderDelegate {
public:
    EditorApp() {
        ui_ctx_ = &ui::Context::get();
        cmdlist_ = &ui_ctx_->command_list();

        cmdlist_->fill_tri({0, 0}, {100, 200}, {200, 100}, 0xFFFF0000);
        cmdlist_->stroke_line({0, 0}, {500, 500}, 2.0, 0xFF00FF00);

        for (int i = 1; i < 10; i++) {
            cmdlist_->stroke_line({0, 10.0f * i}, {500, 10.0f * i}, 1.0,
                                  0xFFCCCCCC);
        }
    }

    void render() override {}

private:
    ui::Context* ui_ctx_ = nullptr;
    ui::CommandList* cmdlist_;
};
}  // namespace spargel::editor

int main() {
    spargel::runtime::init_runtime();
    spargel::editor::EditorApp app;
    auto& ui_ctx = ui::Context::get();
    ui_ctx.set_title("Spargel Editor");
    ui_ctx.start_with(&app);
    return 0;
}
