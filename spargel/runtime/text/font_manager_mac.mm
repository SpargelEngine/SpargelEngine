#include "spargel/runtime/text/font_manager.h"

#include <utility>

#include <CoreText/CoreText.h>

#include "spargel/runtime/apple/bridge_cast.h"
#include "spargel/runtime/apple/retain_ptr.h"
#include "spargel/runtime/base/logging.h"
#include "spargel/runtime/text/font.h"

namespace spargel::text {
namespace {
char const* bool_to_str(bool b) { return b ? "true" : "false"; }
}  // namespace

class FontMac final : public Font {
public:
  explicit FontMac(apple::RetainPtr<CTFontRef> font)
      : ct_font_{std::move(font)} {
    // TODO(tianjiao): remove
    dump_info();
  }

  apple::RetainPtr<CTFontRef> get() { return ct_font_; }

  void dump_info() {
    apple::RetainPtr<CFStringRef> display_name{
        CTFontCopyDisplayName(ct_font_.get())};
    apple::RetainPtr<CFStringRef> full_name{CTFontCopyFullName(ct_font_.get())};
    apple::RetainPtr<CFStringRef> family_name{
        CTFontCopyFamilyName(ct_font_.get())};
    apple::RetainPtr<CFStringRef> postscript_name{
        CTFontCopyPostScriptName(ct_font_.get())};
    LOG_INFO("CoreText font:");
    LOG_INFO("=> display name: %s",
             CFStringGetCStringPtr(display_name.get(), kCFStringEncodingASCII));
    LOG_INFO("=> full name: %s",
             CFStringGetCStringPtr(full_name.get(), kCFStringEncodingASCII));
    LOG_INFO("=> family name: %s",
             CFStringGetCStringPtr(family_name.get(), kCFStringEncodingASCII));
    LOG_INFO(
        "=> postscript name: %s",
        CFStringGetCStringPtr(postscript_name.get(), kCFStringEncodingASCII));

    CTFontSymbolicTraits symbolic_traits =
        CTFontGetSymbolicTraits(ct_font_.get());
    bool italic = symbolic_traits & kCTFontTraitItalic;
    bool bold = symbolic_traits & kCTFontTraitBold;
    bool monospace = symbolic_traits & kCTFontTraitMonoSpace;
    CTFontStylisticClass stylistic_class =
        symbolic_traits & kCTFontTraitClassMask;
    bool sans_serif = stylistic_class & kCTFontClassSansSerif;
    LOG_INFO("=> italic: %s", bool_to_str(italic));
    LOG_INFO("=> bold: %s", bool_to_str(bold));
    LOG_INFO("=> monospace: %s", bool_to_str(monospace));
    LOG_INFO("=> sans_serif: %s", bool_to_str(sans_serif));
  }

private:
  apple::RetainPtr<CTFontRef> ct_font_;
};

class FontManagerMac final : public FontManager {
public:
  FontManagerMac() : FontManager("CoreText") { create_default_font(); }

  ~FontManagerMac() {
    if (default_font_) {
      delete default_font_;
    }
  }

  Font* default_font() override { return default_font_; }

private:
  void create_default_font() {
    LOG_INFO("default font: Helvetica 13.0");
    CTFontRef font =
        CTFontCreateWithName(apple::bridge_cast(@"Helvetica"), 13.0, nullptr);
    default_font_ = new FontMac(apple::RetainPtr(font));
  }
  FontMac* default_font_ = nullptr;
};

FontManager* FontManager::create() { return new FontManagerMac; }

}  // namespace spargel::text
