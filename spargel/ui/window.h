#pragma once

namespace spargel::ui {

class PlatformCanvas;

enum class PaintMethod {
  custom,
  platform_canvas,
};

class WindowDelegate {
public:
  virtual ~WindowDelegate() = default;

  // PaintMethod::custom
  virtual void paint() {}
  // PaintMethod::platform_canvas
  virtual void paint(PlatformCanvas* /*canvas*/) {}
};

class Window {
public:
  virtual ~Window() = default;

  void set_delgate(WindowDelegate* delegate) { delegate_ = delegate; }
  WindowDelegate* delegate() { return delegate_; }

  void set_paint_method(PaintMethod method) { paint_method_ = method; }
  PaintMethod paint_method() const { return paint_method_; }

  virtual void set_title(char const* title) = 0;

private:
  WindowDelegate* delegate_ = nullptr;
  PaintMethod paint_method_ = PaintMethod::custom;
};

class UIManager {
public:
  static UIManager* create();

  virtual ~UIManager() = default;

  virtual Window* create_window() = 0;

  virtual void start() = 0;
};

}  // namespace spargel::ui
