#pragma once

#include <vector>

#include "spargel/runtime/ui/painter.h"

namespace spargel::ui {
class RenderDelegate {
public:
    virtual ~RenderDelegate() = default;
    virtual void render() {}
};
class Backend {
public:
    virtual ~Backend() = default;
    virtual char const* name() const = 0;
    virtual void init() = 0;
    virtual void set_title(char const*) = 0;
    virtual void start_with(RenderDelegate* delegate) = 0;
};
// a virtual window managed by the framework
struct Window {
    char const* name;
    math::Vec2f position;
    math::Vec2f size;
};
struct InputState {
    math::Vec2f mouse_position;
};
class Context {
public:
    static Context& get();

    ~Context();

    void init();
    void set_title(char const* title);
    void start_with(RenderDelegate* delegate);

    CommandList& command_list() { return cmdlist_; }

    Window* find_window(char const* name);

private:
    static Backend* create_backend();

    Backend* backend_ = nullptr;
    CommandList cmdlist_;

    // this is persisted between frames
    std::vector<Window*> windows_;
};
}  // namespace spargel::ui
