#include "spargel/base/vector.h"

#include "spargel/base/check.h"
#include "spargel/base/meta.h"
#include "spargel/base/test.h"

namespace spargel::base {
    namespace {
        TEST(Vector_Basic) {
            Vector<int> v1;
            spargel_check(v1.count() == 0);
            spargel_check(v1.capacity() == 0);
            for (int i = 1; i <= 100; i++) {
                v1.emplace(i);
                spargel_check(v1.count() == (usize)i);
            }
            spargel_check(v1[50] == 51);
            Vector<int> v2(spargel::base::move(v1));
            spargel_check(v2.count() == 100);
            spargel_check(v1.count() ==
                          0);  // NOLINT(clang-analyzer-cplusplus.Move)
            spargel_check(v1.capacity() ==
                          0);  // NOLINT(clang-analyzer-cplusplus.Move)
            Vector<int> v3 = v2;
            spargel_check(v2.count() == 100);
            spargel_check(v3.count() == 100);
            spargel_check(v2[2] == 3);
            spargel_check(v3[0] == 1);
            v3.reserve(200);
            spargel_check(v3.count() == 100);
            spargel_check(v3.capacity() >= 200);
            v2.clear();
            spargel_check(v2.count() == 0);
        }

        TEST(Vector_eraseFast) {
            Vector<int> x;
            x.push(1);
            x.push(2);
            x.push(3);
            spargel_check(x.count() == 3);
            x.eraseFast(1);
            spargel_check(x.count() == 2);
            spargel_check(x[0] == 1);
            spargel_check(x[1] == 3);
            x.eraseFast(0);
            spargel_check(x.count() == 1);
            spargel_check(x[0] == 3);
        }

        TEST(Vector_eraseFast2) {
            Vector<int> x;
            for (int i = 0; i < 10; i++) {
                x.push(i);
            }
            x.eraseIfFast([](int n) { return n % 2 == 0; });
            spargel_check(x.count() == 5);
        }
    }  // namespace
}  // namespace spargel::base
