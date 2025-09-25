#include "spargel/base/tag_invoke.h"
#include "spargel/logging/logging.h"

using namespace spargel;

namespace A {
    namespace __foo {
        struct foo {
            template <typename T>
            constexpr void operator()(T t) const {
                base::tag_invoke(foo{}, t);
            }
        };
        template <typename T>
        void tag_invoke(foo, T t) {
            spargel_log_info("default impl");
        }
    }  // namespace __foo
    inline constexpr __foo::foo foo{};
}  // namespace A

namespace B {
    static_assert(base::IsSame<base::tag<A::foo>, A::__foo::foo>);
    struct bar {};
    void tag_invoke(base::tag<A::foo>, bar b) {
        spargel_log_info("special impl for bar");
    }
}  // namespace B

int main() {
    A::foo(1);
    A::foo(B::bar{});
    return 0;
}
