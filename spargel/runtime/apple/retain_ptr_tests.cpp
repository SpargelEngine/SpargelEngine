#include <CoreFoundation/CoreFoundation.h>
#include <gtest/gtest.h>

#include "spargel/runtime/apple/cftyperef.h"

namespace spargel::apple {
namespace {

TEST(CFTypeRef, Construction) {
  CFStringRef string = CFStringCreateWithCString(nullptr, "hello, spargel",
                                                 kCFStringEncodingASCII);
  EXPECT_EQ(1, CFGetRetainCount(string));
  RetainPtr<CFStringRef> scoped_string{string};
  EXPECT_EQ(1, CFGetRetainCount(scoped_string.get()));
  RetainPtr<CFStringRef> copied_string{scoped_string};
  EXPECT_EQ(scoped_string.get(), copied_string.get());
  EXPECT_EQ(2, CFGetRetainCount(string));
  RetainPtr<CFStringRef> moved_string{std::move(scoped_string)};
  EXPECT_EQ(string, moved_string.get());
  EXPECT_EQ(nullptr, scoped_string.get());
  EXPECT_EQ(2, CFGetRetainCount(string));
}

}  // namespace
}  // namespace spargel::apple
