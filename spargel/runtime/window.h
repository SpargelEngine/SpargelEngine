#pragma once

namespace spargel::runtime {
    class Window {
    public:
        static Window* create();

        virtual void show() = 0;
    };
}  // namespace spargel::runtime
