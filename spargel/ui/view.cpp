#include "spargel/ui/view.h"

#include "spargel/base/check.h"

namespace spargel::ui {

View::~View() {
  for (auto* child : children_) {
    delete child;
  }
}

void View::set_parent(View* parent) {
  // TODO: remove from parent_ if it is non-null
  parent_ = parent;
}

void View::add_child(View* child) {
  CHECK(child);
  children_.push_back(child);
  child->set_parent(this);
}

void View::paint(PlatformCanvas* canvas) {
  paint_content(canvas);
  paint_children(canvas);
}
void View::paint_content(PlatformCanvas* /*canvas*/) {}
void View::paint_children(PlatformCanvas* canvas) {
  for (auto* child : children_) {
    child->paint(canvas);
  }
}

}  // namespace spargel::ui
