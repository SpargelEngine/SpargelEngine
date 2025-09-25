#include "spargel/base/either.h"

#include "spargel/base/check.h"
#include "spargel/base/test.h"
#include "spargel/base/unique_ptr.h"
#include "spargel/logging/logging.h"

using namespace spargel::base;

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
    {
        Either<L, R> either1(makeLeft<L, R>());
        Either<L, R> either2(either1);
        either1 = move(either2);
        either2 = either1;
        either2 = makeRight<L, R>();
    }
    {
        unique_ptr<L2> pl2 = make_unique<L2>();
        unique_ptr<R2> pr2 = make_unique<R2>();
        auto either1 = makeLeft<unique_ptr<L2>, unique_ptr<R2>>(move(pl2));
        auto either2 = makeRight<unique_ptr<L2>, unique_ptr<R2>>(move(pr2));
        Either<unique_ptr<L>, unique_ptr<R>> either3(move(either1));
        Either<unique_ptr<L>, unique_ptr<R>> either4 = move(either2);
    }
}

TEST(Either_Access) {
    {
        auto either = makeLeft<L, R>(123);
        spargel_check(either.isLeft());
        spargel_check(!either.isRight());
        spargel_check(either.left().x == 123);
    }
    {
        auto either = makeRight<L, R>(0.123f);
        spargel_check(!either.isLeft());
        spargel_check(either.isRight());
        spargel_check(either.right().y == 0.123f);
    }
}

TEST(Either_Implicit) {
    Either<int, bool> x = Left{123};
    spargel_check(x.isLeft());
    spargel_check(x.left() == 123);

    x = Right{false};
    spargel_check(x.isRight());
    spargel_check(x.right() == false);
}
