#include <CoreText/CoreText.h>
#include <stdio.h>

extern "C" CTFontDescriptorRef CTFontDescriptorCreateForCSSFamily(
    CFStringRef cssFamily, CFStringRef language);

int main() {
    auto family =
        CFStringCreateWithCString(nullptr, "sans-serif", kCFStringEncodingASCII);
    auto language =
        CFStringCreateWithCString(nullptr, "zh", kCFStringEncodingASCII);
    auto desc = CTFontDescriptorCreateForCSSFamily(family, language);
    CFShow(desc);
    CFRelease(desc);
    CFRelease(family);
    CFRelease(language);
    return 0;
}
