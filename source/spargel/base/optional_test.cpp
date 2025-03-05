#include <spargel/base/algorithm.h>
#include <spargel/base/check.h>
#include <spargel/base/logging.h>
#include <spargel/base/meta.h>
#include <spargel/base/optional.h>
#include <spargel/base/test.h>
#include <spargel/base/unique_ptr.h>

using namespace spargel;

struct Foo {
    Foo(int x) { spargel_log_info("Foo()"); }
    ~Foo() { spargel_log_info("~Foo()"); }
};

struct Base {};

struct Derived : Base {};

TEST(Optional_Basic) {
    base::unique_ptr<Derived> pd = base::make_unique<Derived>();
    base::Optional<base::unique_ptr<Derived>> od =
        base::makeOptional<base::unique_ptr<Derived>>(base::move(pd));
    base::Optional<base::unique_ptr<Base>> b = base::move(od);
        // base::makeOptional<base::unique_ptr<Base>>(base::move(pd));

    base::Optional<int> x;
    spargel_check(!x.hasValue());
    base::Optional<int> y = base::makeOptional<int>(2);
    spargel_check(y.hasValue());
    spargel_check(y.value() == 2);
    base::swap(x, y);
    spargel_check(x.hasValue());
    spargel_check(!y.hasValue());
    spargel_check(x.value() == 2);

    base::Optional<Foo> f = base::makeOptional<Foo>(123);
    base::Optional<Foo> g = f;
    base::Optional<Foo> h = base::move(f);
}
