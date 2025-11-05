#include "spargel/ui/canvas.h"
#include "spargel/ui/context.h"
#include "spargel/ui/view.h"
#include "spargel/ui/window.h"

namespace ui = spargel::ui;

namespace spargel::editor {
namespace {

class View1 : public ui::View {
public:
  void paint_content(ui::PlatformCanvas* canvas) override {
    canvas->draw_text("View1", 100, 200);
  }
};

class EditorApp : public ui::RenderDelegate, public ui::WindowDelegate {
public:
  EditorApp() { view_ = new View1; }
  // void render() override {
  //   ui::begin_frame();
  //   ui::label("TextRender", {100, 100}, 0xFFFFFFFF);
  // }
  void paint(ui::PlatformCanvas* canvas) override {
    canvas->draw_text("test paint()", 100, 10);
    view_->paint(canvas);
  }

private:
  ui::View* view_ = nullptr;
};

}  // namespace
}  // namespace spargel::editor

int main() {
  // spargel::init_runtime();

  // sol::state lua;
  // lua.open_libraries(sol::lib::base);
  // lua.script("print('hello from lua')");

  spargel::editor::EditorApp app;
  // auto& ui_ctx = ui::Context::get();
  // ui_ctx.set_title("Spargel Editor");
  // ui_ctx.start_with(&app);

  auto* ui = ui::UIManager::create();
  auto* window = ui->create_window();
  window->set_title("Spargel Editor");
  window->set_delgate(&app);
  ui->start();
  return 0;
}
