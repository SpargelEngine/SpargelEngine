#include "spargel/init.h"
#include "spargel/ui/commands.h"
#include "spargel/ui/context.h"

#include <sol/sol.hpp>

namespace ui = spargel::ui;

namespace spargel::editor {
namespace {

class EditorApp : public ui::RenderDelegate {
public:
  void render() override {
    ui::begin_frame();
    ui::label("Hello, world!", {100, 100}, 0xFFFFFFFF);
  }
};

}  // namespace
}  // namespace spargel::editor

int main() {
  spargel::init_runtime();

  sol::state lua;
	lua.open_libraries(sol::lib::base);
	lua.script("print('hello from lua')");

  spargel::editor::EditorApp app;
  auto& ui_ctx = ui::Context::get();
  ui_ctx.set_title("Spargel Editor");
  ui_ctx.start_with(&app);
  return 0;
}
