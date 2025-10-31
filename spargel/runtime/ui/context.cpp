#include "spargel/runtime/ui/context.h"

#include <cstring>

#include "spargel/runtime/base/check.h"
#include "spargel/runtime/base/logging.h"

namespace spargel::ui {

Context& Context::get() {
  static Context ctx;
  return ctx;
}
Context::~Context() {
  for (auto* window : windows_) {
    delete window;
  }
  if (backend_) {
    delete backend_;
  }
}
void Context::init() {
  backend_ = create_backend();
  CHECK(backend_);
  LOG_INFO("init ui context (backend: %s)", backend_->name());
  backend_->init();
}
void Context::set_title(char const* title) {
  CHECK(backend_);
  backend_->set_title(title);
}
void Context::start_with(RenderDelegate* delegate) {
  CHECK(backend_);
  CHECK(delegate);
  backend_->start_with(delegate);
}
Window* Context::find_window(char const* name) {
  for (auto* window : windows_) {
    if (strcmp(window->name, name) == 0) {
      return window;
    }
  }
  return nullptr;
}

}  // namespace spargel::ui
