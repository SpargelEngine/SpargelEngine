#include <spargel/base/tagged_union.h>
#include <spargel/base/test.h>

enum class FooTag {
    Bar,
    Baz
};

struct Bar {};

struct Baz;

using Foo = spargel::base::TaggedUnion<FooTag>;

TEST(TaggedUnion_Stub) {
}
