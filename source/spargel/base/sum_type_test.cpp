#include <spargel/base/algorithm.h>
#include <spargel/base/check.h>
#include <spargel/base/logging.h>
#include <spargel/base/sum_type.h>
#include <spargel/base/test.h>

using namespace spargel;

struct Foo {
    ~Foo() { spargel_log_info("~Foo();"); }
};

TEST(SumType_Basic) {
    using Type1 = base::SumType<int, double, int, Foo>;

    auto x = Type1::make<2>(3);
    spargel_log_info("x.getValue<2>() = %d", x.getValue<2>());
    x.getValue<2>() = 100;
    spargel_log_info("x.getValue<2>() = %d", x.getValue<2>());

    auto y = Type1::make<2>(4);

    spargel_log_info("x = %d, y = %d", x.getValue<2>(), y.getValue<2>());
    base::swap(x, y);
    spargel_log_info("x = %d, y = %d", x.getValue<2>(), y.getValue<2>());

    auto z = Type1::make<1>(1.2);

    spargel_log_info("x.index = %zu, z.index = %zu", x.getIndex(), z.getIndex());
    spargel_log_info("x = %d, z = %.3f", x.getValue<2>(), z.getValue<1>());
    base::swap(x, z);
    spargel_log_info("x.index = %zu, z.index = %zu", x.getIndex(), z.getIndex());
    spargel_log_info("x = %.3f, z = %d", x.getValue<1>(), z.getValue<2>());

    auto w = Type1::make<3>();

    spargel_log_info("x.index = %zu, w.index = %zu", x.getIndex(), w.getIndex());
    base::swap(x, w);
    spargel_log_info("x.index = %zu, w.index = %zu", x.getIndex(), w.getIndex());

    auto w2 = w;

    auto w3 = base::move(w);
}
