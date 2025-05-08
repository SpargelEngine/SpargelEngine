#include <spargel/base/check.h>
#include <spargel/base/string.h>
#include <spargel/base/test.h>

using namespace spargel::base;

TEST(String_Empty) {
    string s;
    spargel_check(s.length() == 0);
    spargel_check(s.data() == nullptr);
    // spargel_check(*s.data() == '\0');

    string s1, s2;
    spargel_check(s1 == s);
    spargel_check(s2 == s);
    auto s3 = s1 + s2;
    spargel_check(s3 == s);
}

TEST(String_Copy_Zero) {
    {
        // fill the heap with data
        vector<char> v;
        for (int i = 0; i < 10000; i++) {
            v.push('0');
        }
    }
    {
        string s1("ABC");
        spargel_check(s1.data()[3] == '\0');
        string s2(s1);
        spargel_check(s2.data()[3] == '\0');
    }
}
