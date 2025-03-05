#include <spargel/base/check.h>
#include <spargel/base/inline_array.h>
#include <spargel/base/test.h>

using namespace spargel;

TEST(InlineArray_Basic) {
    base::InlineArray<int, 3> x = {1, 2, 3};
    spargel_check(x.count() == 3);
    spargel_check(x[0] == 1);
    spargel_check(x[1] == 2);
    spargel_check(x[2] == 3);
}
