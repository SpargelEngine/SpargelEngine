#include "spargel/base/ref_ptr.h"
#include "spargel/base/test.h"
#include "spargel/logging/logging.h"

namespace spargel::base {
    namespace {
        struct Foo : RefCount<Foo> {
            Foo() {}
            ~Foo() { spargel_log_debug("~Foo"); }
        };
        TEST(RefPtr_Basics) {
            auto f = makeRef<Foo>();
            auto g = f;
        }
    }  // namespace
}  // namespace spargel::base
