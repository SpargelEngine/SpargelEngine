#pragma once

#include <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>

#include "spargel/runtime/base/check.h"

namespace spargel::apple {

inline NSString* bridge_cast(CFStringRef CF_CONSUMED value) {
  CHECK(!value || CFStringGetTypeID() == CFGetTypeID(value));
  return (__bridge NSString*)value;
}
inline CFStringRef bridge_cast(NSString* value) {
  CFStringRef cf_value = (__bridge CFStringRef)value;
  CHECK(!cf_value || CFStringGetTypeID() == CFGetTypeID(cf_value));
  return cf_value;
}
inline CFAttributedStringRef bridge_cast(NSAttributedString* value) {
  CFAttributedStringRef cf_value = (__bridge CFAttributedStringRef)value;
  CHECK(!cf_value || CFAttributedStringGetTypeID() == CFGetTypeID(cf_value));
  return cf_value;
}

}  // namespace spargel::apple
