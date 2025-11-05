#pragma once

#include <vector>

namespace spargel::ui {

class PlatformCanvas;

class View {
public:
  virtual ~View();

  // hierarchy -----------------------------------------------------------------

  View* parent() { return parent_; }
  View const* parent() const { return parent_; }
  void set_parent(View* parent);

  void add_child(View* child);

  // style ---------------------------------------------------------------------

  // event ---------------------------------------------------------------------

  void paint(PlatformCanvas* canvas);
  virtual void paint_content(PlatformCanvas* canvas);
  virtual void paint_children(PlatformCanvas* canvas);

private:
  View* parent_ = nullptr;
  std::vector<View*> children_;
};

}  // namespace spargel::ui
