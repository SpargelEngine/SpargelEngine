#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

#include "spargel/cocoa/display_modes.h"

using namespace spargel;

int main() {
    base::Vector<cocoa::DisplayMode> modes;
    cocoa::DisplayMode::query(modes);
    for (auto const& mode : modes) {
        printf("mode: %zu x %zu\n", mode.width, mode.height);
    }
    return 0;

    CFDictionaryRef trait_dict = [] {
        void const* keys[] = {kCTFontSymbolicTrait};
        auto n = kCTFontClassOldStyleSerifs;
        // auto n = kCTFontClassSansSerif;
        auto number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &n);
        void const* values[] = {number};
        return CFDictionaryCreate(kCFAllocatorDefault, keys, values, 1, NULL,
                                  NULL);
    }();
    CFShow(trait_dict);

    CFArrayRef langs = [] {
        auto name = CFSTR("zh");
        void const* values[] = {name};
        return CFArrayCreate(kCFAllocatorDefault, values, 1, nullptr);
    }();

    // auto family_name = CFStringCreateWithCString(nullptr, "Times",
    // kCFStringEncodingUTF8); void const* keys[] =
    // {kCTFontFamilyNameAttribute}; void const* values[] = {family_name};
    void const* keys[] = {kCTFontTraitsAttribute, kCTFontLanguagesAttribute};
    void const* values[] = {trait_dict, langs};
    auto dict =
        CFDictionaryCreate(kCFAllocatorDefault, keys, values, 2, NULL, NULL);
    CFShow(dict);
    auto font_desc = CTFontDescriptorCreateWithAttributes(dict);
    CFShow(font_desc);
    auto font = CTFontCreateWithFontDescriptor(font_desc, 0, nullptr);
    CFShow(font);

    auto fonts =
        CTFontDescriptorCreateMatchingFontDescriptors(font_desc, nullptr);
    CFShow(fonts);

    auto traits = CTFontCopyTraits(font);
    CFShow(traits);
    auto symbolic = CTFontGetSymbolicTraits(font);
    auto stylistic = symbolic & kCTFontTraitClassMask;
    printf("%u\n", stylistic);
    return 0;
}
