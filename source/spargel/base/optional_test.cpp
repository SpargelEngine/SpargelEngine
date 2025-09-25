#include "spargel/base/optional.h"

#include "spargel/base/algorithm.h"
#include "spargel/base/check.h"
#include "spargel/base/meta.h"
#include "spargel/base/test.h"
#include "spargel/base/unique_ptr.h"
#include "spargel/logging/logging.h"

namespace spargel::base {
    namespace {
        struct Foo {
            Foo(int) { spargel_log_info("Foo()"); }
            ~Foo() { spargel_log_info("~Foo()"); }
        };

        struct Base {};

        struct Derived : Base {};

        TEST(Optional_Basic) {
            unique_ptr<Derived> pd = make_unique<Derived>();
            Optional<unique_ptr<Derived>> od =
                makeOptional<unique_ptr<Derived>>(move(pd));
            Optional<unique_ptr<Base>> b = move(od);
            // makeOptional<unique_ptr<Base>>(move(pd));

            Optional<int> x;
            spargel_check(!x.hasValue());
            Optional<int> y = makeOptional<int>(2);
            spargel_check(y.hasValue());
            spargel_check(y.value() == 2);
            swap(x, y);
            spargel_check(x.hasValue());
            spargel_check(!y.hasValue());
            spargel_check(x.value() == 2);

            x = nullopt;
            spargel_check(!x.hasValue());

            Optional<Foo> f = makeOptional<Foo>(123);
            Optional<Foo> g = f;
            Optional<Foo> h = move(f);
        }

        TEST(Optional_Assignment) {
            Optional<int> x = nullopt;
            spargel_check(!x.hasValue());
            x = 2;
            spargel_check(x.hasValue());
            spargel_check(x.value() == 2);
        }
    }  // namespace
}  // namespace spargel::base
