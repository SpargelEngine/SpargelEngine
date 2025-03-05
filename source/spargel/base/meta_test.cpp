#include <spargel/base/meta.h>
#include <spargel/base/test.h>

using namespace spargel::base;

static_assert(is_same<bool, bool>);
static_assert(is_same<int, int>);
static_assert(!is_same<int, bool>);

static_assert(is_same<conditional<true, int, bool>, int>);
static_assert(is_same<conditional<false, int, bool>, bool>);

static_assert(is_array<int[]>);
static_assert(is_array<int[1]>);
static_assert(is_array<int[1][2]>);
static_assert(!is_array<int>);
static_assert(!is_array<int*>);
static_assert(!is_array<int&>);

static_assert(is_same<remove_extent<int[]>, int>);
static_assert(is_same<remove_extent<int[1]>, int>);
static_assert(is_same<remove_extent<int[1][2]>, int[2]>);

static_assert(is_same<add_pointer<int>, int*>);
static_assert(is_same<add_pointer<int&>, int*>);

static_assert(!is_function<int>);
static_assert(!is_function<int&>);
static_assert(is_function<int(int)>);

static_assert(is_same<decay<int>, int>);
static_assert(is_same<decay<int&>, int>);
static_assert(is_same<decay<int&&>, int>);
static_assert(is_same<decay<int const&>, int>);
static_assert(is_same<decay<int const&&>, int>);
static_assert(is_same<decay<int[1]>, int*>);
static_assert(is_same<decay<int[1][2]>, int (*)[2]>);
static_assert(is_same<decay<int(int)>, int (*)(int)>);

static_assert(!is_same<decay<int>, bool>);

TEST(Meta_Stub) {}
