#pragma once

#include "spargel/core/core.h"
#include "spargel/coro/coroutine.h"

namespace spargel::coro {

    namespace detail {}

    template <typename T>
    class generator {
    public:
        static_assert(core::is_same<T, core::remove_cvref<T>>);

        class promise_type {
        public:
            suspend_always initial_suspend() const noexcept { return {}; }
            void return_void() const noexcept {}
            void await_transform() = delete;

        private:
            // TODO: `T` -> `_Yielded`
            core::add_pointer<T> ptr_ = nullptr;
        };

        ~generator() {
            if (coro_) {
                coro_.destroy();
            }
        }

    private:
        coroutine_handle<promise_type> coro_;
    };

}  // namespace spargel::coro
