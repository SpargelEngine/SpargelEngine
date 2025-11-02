#include <format>

#include "spargel/runtime/init.h"
#include "spargel/runtime/ui/commands.h"
#include "spargel/runtime/ui/context.h"

namespace ui = spargel::ui;

namespace spargel::editor {
namespace {

class EditorApp : public ui::RenderDelegate {
public:
  void render() override {
    ui::begin_frame();

    ui::fill_triangle({0, 0}, {100, 200}, {200, 100}, 0xFFFF0000);
    ui::stroke_line({0, 0}, {500, 500}, 2.0, 0xFF00FF00);
    for (int i = 1; i < 10; i++) {
      ui::stroke_line({0, 10.0f * i}, {500, 10.0f * i}, 1.0, 0xFFCCCCCC);
    }

    if (ui::label_button("inc", {200, 200}, {40, 20}, 0xFF0000FF)) {
      count_++;
    }
    if (ui::label_button("dec", {260, 200}, {40, 20}, 0xFF00FF00)) {
      count_--;
    }

    std::string text = std::format("current count: {}", count_);
    ui::label(text.data(), {100, 300}, 0xFF0000FF);

    text = std::format("value: {:.2f}", value_);
    ui::slide(&value_, text.data(), {100, 400}, {80, 18}, 0xFFFF0000);

    // ui::Context::get().input_state().dump();
  }

private:
  int count_ = 0;
  float value_ = 0.7f;
};

}  // namespace
}  // namespace spargel::editor

int main() {
  spargel::init_runtime();
  spargel::editor::EditorApp app;
  auto& ui_ctx = ui::Context::get();
  ui_ctx.set_title("Spargel Editor");
  ui_ctx.start_with(&app);
  return 0;
}
