#include "spargel/base/hash.h"

#include "spargel/base/check.h"
#include "spargel/base/platform.h"
#include "spargel/base/string.h"
#include "spargel/base/test.h"
#include "spargel/logging/logging.h"

namespace spargel::base {
    namespace {
        struct Foo {
            u8 x;
            u32 y;
            string s;
        };
        void tag_invoke(tag<hash>, HashRun& run, Foo const& f) {
            run.combine(f.x);
            run.combine(f.y);
            run.combine(f.s);
        }
        TEST(HashIsPure) {
            spargel_check(hash(1) == hash(1));
            // TODO: hash float/double.
            // spargel_check(hash(1.2) == hash(1.2));
            spargel_check(hash(string("hello")) == hash(string("hello")));
            spargel_check(hash(Foo(1, 2, string("xyz"))) ==
                          hash(Foo(1, 2, string("xyz"))));
        }

        TEST(HashIsSparse) {
            spargel_check(hash(1) != hash(2));
            // TODO: hash float/double.
            // spargel_check(hash(1.2) != hash(2.1));
            spargel_check(hash(string("hello")) != hash(string("bonjour")));
            spargel_check(hash(Foo(1, 2, string("xyz"))) !=
                          hash(Foo(3, 5, string("def"))));
        }
    }  // namespace
}  // namespace spargel::base
