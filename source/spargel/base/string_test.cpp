#include <spargel/base/check.h>
#include <spargel/base/string.h>
#include <spargel/base/test.h>

using namespace spargel;

TEST(String_Empty) {
    base::string s;
    spargel_check(s.length() == 0);
    spargel_check(s.data() == nullptr);
    // spargel_check(*s.data() == '\0');

    base::string s1, s2;
    spargel_check(s1 == s);
    spargel_check(s2 == s);
    auto s3 = s1 + s2;
    spargel_check(s3 == s);
}
