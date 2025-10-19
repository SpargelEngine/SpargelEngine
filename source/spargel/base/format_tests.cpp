#include "spargel/base/check.h"
#include "spargel/base/format.h"
#include "spargel/base/string_view.h"
#include "spargel/base/test.h"

namespace spargel::base {
    namespace {
        void eq(detail::FormatString s, StringView t) {
            StringView sv{s.begin(), s.end()};
            spargel_check(sv == t);
        }
        void testSplit(detail::FormatString s, StringView a, StringView b,
                       StringView c) {
            detail::FormatString seg;
            detail::FormatString fmt;
            detail::FormatString res;

            s.split(seg, fmt, res);
            eq(seg, a);
            eq(fmt, b);
            eq(res, c);
        }
        TEST(FormatString_Split) {
            testSplit("", "", "", "");
            testSplit("hello", "hello", "", "");
            testSplit("hello{{", "hello{{", "", "");
            testSplit("{{", "{{", "", "");
            testSplit("}}", "}}", "", "");
            testSplit("{}", "", "{}", "");
            testSplit("a{}", "a", "{}", "");
            testSplit("{b}", "", "{b}", "");
            testSplit("{}c", "", "{}", "c");
            testSplit("a{b}c", "a", "{b}", "c");
            testSplit("a{b}c{{}}", "a", "{b}", "c{{}}");
            testSplit("a{{}}{b}c{{}}", "a{{}}", "{b}", "c{{}}");
            testSplit("a{{}}{b}c{{}}", "a{{}}", "{b}", "c{{}}");
        }
        TEST(Print) {
            print("hello{}\n", StringView{" world"});
            print("{}-{}-{}\n", StringView{"a"}, StringView{"b"}, "c");
            print("{}\n", usize(12345));
            print("{}\n", usize(0));
            print("{}\n", usize(1));
            print("{}\n", usize(12));
        }
    }  // namespace
}  // namespace spargel::base
