#pragma once

#include "spargel/base/meta.h"  // IWYU pragma: keep

namespace spargel::cocoa_utils {
    namespace detail {
#ifdef __OJBC__
        template <typename T>
        inline constexpr bool is_ns_type = base::is_convertible<T, id>;
#else
        template <typename T>
        inline constexpr bool is_ns_type = false;
#endif
        template <typename T, bool = is_ns_type<T>>
        struct ptr_storage;
        template <typename T>
        struct ptr_storage<T, true> {
            using type = T;
        };
    }  // namespace detail
    // RetainPtr can be used to manage CF/NS objects.
    template <typename T>
    class RetainPtr {
    public:
    private:
        void release() {}
    };
}  // namespace spargel::cocoa_utils
