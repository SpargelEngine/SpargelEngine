#include "spargel/base/tagged_union.h"

#include "spargel/base/check.h"
#include "spargel/base/test.h"
#include "spargel/logging/logging.h"

using spargel::base::Case;
using spargel::base::TaggedUnion;

enum class FooKind { Bar, Baz };

struct C1 {};

struct Bar {
    int x;
};

struct Baz {
    C1 y;
};

using Foo = spargel::base::TaggedUnion<Case<FooKind::Bar, Bar>,
                                       Case<FooKind::Baz, Baz>>;

TEST(TaggedUnion_Basic) {
    Foo x = Foo::make<FooKind::Bar>(123);
    spargel_check(x.tag() == FooKind::Bar);

    Foo y = Foo::make<FooKind::Baz>(C1{});
    spargel_check(y.tag() == FooKind::Baz);

    auto match = [](Foo& foo) {
        return foo.match(
            [](Case<FooKind::Bar>, Bar& bar) {
                spargel_log_info("Bar: %d", bar.x);
                return true;
            },
            // general case
            [](auto, auto) {
                spargel_log_info("not a Bar");
                return false;
            });
    };

    spargel_check(match(x));
    spargel_check(!match(y));
}
