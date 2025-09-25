#pragma once

#include "spargel/base/coroutine.h"

namespace spargel::task {
    // this should be copyable
    // a shared_ptr to internal state
    class Event {};
    class EventAwaitable {};
    // Example:
    //   Task foo(Event e) {
    //     print_line("before event");
    //     co_await e;
    //     print_line("after event");
    //   }
    class Task {
    public:
        struct PromiseType {
            Task get_return_object();
            base::AlwaysSuspend initial_suspend() { return {}; }
            base::AlwaysSuspend final_suspend() noexcept { return {}; }
            void return_void() {}
            void unhandled_exception() {}
            EventAwaitable await_transform(Event e);
        };

        using promise_type = PromiseType;
        using HandleType = base::CoroutineHandle<promise_type>;

        explicit Task(HandleType h) : handle_{h} {}

        Task(Task const&) = delete;

        Task(Task&& other) : handle_{other.handle_} { other.handle_ = nullptr; }
        Task& operator=(Task&& other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = other.handle_;
            other.handle_ = nullptr;
            return *this;
        }

        ~Task() {
            if (handle_) {
                handle_.destroy();
            }
        }

        void resume() { handle_.resume(); }
        bool done() { return handle_.done(); }

    private:
        HandleType handle_;
    };
    class TaskRunner {
    public:
        // bind the current thread to the runner
        void bindCurrentThread();

        void postTask(Task&& t);
        // Example:
        //   Event e1, e2;
        //   int a = 1;
        //   postTask([&a, e1, e2]() -> Task {
        //     co_await e1;
        //     a = 2;
        //     e2.notify();
        //   });
        //   postTask([&a, e2]() -> Task {
        //     co_await e2;
        //     a = 3;
        //   });
        //   e1.notify();
        template <typename F>
        void postTask(F&& f);
    };
}  // namespace spargel::task
