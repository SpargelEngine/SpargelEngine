#pragma once

#include "spargel/coro/coroutine.h"

namespace spargel::coro {
    class Task {
    public:
        struct promise_type {
            Task get_return_object();
            suspend_always initial_suspend() const noexcept { return {}; }
            suspend_never final_suspend() const noexcept { return {}; }
            void return_void() const noexcept {}
        };
        void resume() { handle_.resume(); }
        bool done() const { return handle_.done(); }

    private:
        coroutine_handle<promise_type> handle_;
    };
    class Event {
    public:
        void notify();
    };
    class Scheduler {
    public:
        static Scheduler& instance();
        void bind();
        void schedule(Task&& task);
        void sync(Event event);
    };
}  // namespace spargel::coro
