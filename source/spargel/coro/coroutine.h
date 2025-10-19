#pragma once

#include "spargel/base/types.h"  // IWYU pragma: keep

namespace std {
    template <typename T, typename... Args>
    struct coroutine_traits {
        using promise_type = T::promise_type;
    };

    // `P` is the promise type.
    template <typename P = void>
    class coroutine_handle;

    template <>
    class coroutine_handle<void> {
    public:
        static constexpr coroutine_handle from_address(void* addr) noexcept {
            coroutine_handle h;
            h.handle_ = addr;
            return h;
        }
        constexpr coroutine_handle() noexcept = default;
        constexpr coroutine_handle(nullptr_t) noexcept {}

        void operator()() const { __builtin_coro_resume(handle_); }

        bool done() const noexcept { return __builtin_coro_done(handle_); }
        void resume() const { __builtin_coro_resume(handle_); }
        void destroy() const { __builtin_coro_destroy(handle_); }

        constexpr explicit operator bool() const noexcept {
            return handle_ != nullptr;
        }

    private:
        void* handle_ = nullptr;
    };

    template <typename P>
    class coroutine_handle {
    public:
        static constexpr coroutine_handle from_address(void* addr) noexcept {
            coroutine_handle h;
            h.handle_ = addr;
            return h;
        }
        static coroutine_handle from_promise(P& p) {
            coroutine_handle h;
            h.handle_ = __builtin_coro_promise(&p, alignof(P), true);
            return h;
        }

        constexpr coroutine_handle() noexcept = default;
        constexpr coroutine_handle(nullptr_t) noexcept {}

        constexpr operator coroutine_handle<>() noexcept {
            return coroutine_handle<void>::from_address(handle_);
        }

        void operator()() const { resume(); }

        bool done() const { return __builtin_coro_done(handle_); }
        void resume() const { __builtin_coro_resume(handle_); }
        void destroy() const { __builtin_coro_destroy(handle_); }

        P& promise() const {
            return *static_cast<P*>(
                __builtin_coro_promise(handle_, alignof(P), false));
        }

        constexpr explicit operator bool() const noexcept {
            return handle_ != nullptr;
        }

    private:
        void* handle_ = nullptr;
    };

    struct suspend_never {
        constexpr bool await_ready() noexcept { return true; }
        constexpr void await_suspend(coroutine_handle<>) noexcept {}
        constexpr void await_resume() noexcept {}
    };
    struct suspend_always {
        constexpr bool await_ready() noexcept { return false; }
        constexpr void await_suspend(coroutine_handle<>) noexcept {}
        constexpr void await_resume() noexcept {}
    };
}  // namespace std

namespace spargel::coro {
    using std::coroutine_handle;
    using std::suspend_always;
    using std::suspend_never;
}  // namespace spargel::coro
