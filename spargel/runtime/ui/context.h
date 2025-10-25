#pragma once

#include "spargel/runtime/ui/painter.h"

namespace spargel::runtime::ui {
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
class Context {
public:
    static Context& get();

    ~Context();

    void init();
    void set_title(char const* title);
    void start_with(RenderDelegate* delegate);

    CommandList& command_list() { return cmdlist_; }

private:
    static Backend* create_backend();

    Backend* backend_ = nullptr;
    CommandList cmdlist_;
};
}  // namespace spargel::runtime::ui
