#include <spargel/base/check.h>
#include <spargel/base/either.h>
#include <spargel/base/logging.h>
#include <spargel/base/test.h>
#include <spargel/base/unique_ptr.h>

using namespace spargel;

struct L {
    int x;
    L(int x = 0) : x(x) {}
};

struct R {
    float y;
    R(float y = 0.0f) : y(y) {}
};

struct L2 : L {};

struct R2 : R {};

TEST(Either_Copy_Move_Constructor) {
    base::Either<L, R> either1(base::makeLeft<L, R>());
    base::Either<L, R> either2(either1);
    either1 = base::move(either2);
    either2 = either1;
    either2 = base::makeRight<L, R>();

    base::unique_ptr<L2> pl2 = base::make_unique<L2>();
    base::unique_ptr<R2> pr2 = base::make_unique<R2>();
    auto either3 = base::makeLeft<base::unique_ptr<L2>, base::unique_ptr<R2>>(base::move(pl2));
    auto either4 = base::makeRight<base::unique_ptr<L2>, base::unique_ptr<R2>>(base::move(pr2));
    base::Either<base::unique_ptr<L>, base::unique_ptr<R>> either5(base::move(either3));
    base::Either<base::unique_ptr<L>, base::unique_ptr<R>> either6 = base::move(either4);
}

TEST(Either_Access) {
    auto either1 = base::makeLeft<L, R>(123);
    spargel_check(either1.isLeft());
    spargel_check(!either1.isRight());
    spargel_check(either1.left().x == 123);

    auto either2 = base::makeRight<L, R>(0.123f);
    spargel_check(!either2.isLeft());
    spargel_check(either2.isRight());
    spargel_check(either2.right().y == 0.123f);
}
