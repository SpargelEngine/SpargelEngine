#include "spargel/text/font_manager_mac.h"

#include <CoreText/CoreText.h>

#include "spargel/text/font_mac.h"

namespace spargel::text {

void FontManagerMac::create_default_font() {
  CTFontRef font =
      CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, 40.0, nullptr);
  default_font_ = translate_font(font);
}

FontMac* FontManagerMac::translate_font(CTFontRef font) {
  auto result = fonts_.find(font);
  if (result != fonts_.end()) {
    return result->second;
  }
  auto new_entry = new FontMac(font);
  fonts_.insert({font, new_entry});
  return new_entry;
}

FontManager* FontManager::create() { return new FontManagerMac; }

}  // namespace spargel::text
