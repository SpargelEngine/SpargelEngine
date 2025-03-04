#include <spargel/base/check.h>
#include <spargel/base/hash.h>
#include <spargel/base/logging.h>
#include <spargel/base/platform.h>
#include <spargel/base/string.h>
#include <spargel/base/test.h>

using namespace spargel;

struct Foo {
    u8 x;
    u32 y;
    base::string s;
};

void tag_invoke(base::tag<base::hash>, base::HashRun& run, Foo const& f) {
    run.combine(f.x);
    run.combine(f.y);
    run.combine(f.s);
}

TEST(HashIsPure) {
    spargel_check(base::hash(1) == base::hash(1));
    // TODO: hash float/double.
    // spargel_check(base::hash(1.2) == base::hash(1.2));
    spargel_check(base::hash(base::string("hello")) == base::hash(base::string("hello")));
    spargel_check(base::hash(Foo(1, 2, base::string("xyz"))) == base::hash(Foo(1, 2, base::string("xyz"))));
}

TEST(HashIsSparse) {
    spargel_check(base::hash(1) != base::hash(2));
    // TODO: hash float/double.
    // spargel_check(base::hash(1.2) != base::hash(2.1));
    spargel_check(base::hash(base::string("hello")) != base::hash(base::string("bonjour")));
    spargel_check(base::hash(Foo(1, 2, base::string("xyz"))) != base::hash(Foo(3, 5, base::string("def"))));
}
