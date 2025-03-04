#include <spargel/base/allocator.h>
#include <spargel/base/array_storage.h>
#include <spargel/base/check.h>
#include <spargel/base/meta.h>
#include <spargel/base/test.h>

using namespace spargel;

TEST(ArrayStorage_ConstructByCapacity) {
    base::ArrayStorage<int> x(10, base::default_allocator());
    spargel_check(x.count() == 10);
}

TEST(ArrayStorage_DefaultConstructor) {
    base::ArrayStorage<int> y(/*alloc=*/nullptr);
    spargel_check(y.count() == 0);
}

TEST(ArrayStorage_MoveConstructor) {
    base::ArrayStorage<int> x(10, base::default_allocator());
    base::ArrayStorage<int> z(base::move(x));
    spargel_check(x.count() == 0);
    spargel_check(z.count() == 10);
}
