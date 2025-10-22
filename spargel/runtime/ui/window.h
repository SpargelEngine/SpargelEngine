#pragma once

namespace spargel::runtime::ui {
class WindowDelegate {
public:
    virtual ~WindowDelegate() = default;
    virtual void on_render() {}
};
class Window {
public:
    static Window* create();

    virtual ~Window() = default;

    virtual void set_title(char const* title) = 0;
    virtual void show() = 0;

    void set_delegate(WindowDelegate* delegate) { delegate_ = delegate; }
    WindowDelegate* delegate() { return delegate_; }

private:
    WindowDelegate* delegate_ = nullptr;
};
}  // namespace spargel::runtime::ui
