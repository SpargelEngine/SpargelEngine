#pragma once

namespace spargel::runtime {
    class Window {
    public:
        static Window* create();

        virtual void set_title(char const* title) = 0;
        virtual void show() = 0;
    };
}  // namespace spargel::runtime
