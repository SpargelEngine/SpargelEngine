#pragma once

#include <unordered_map>

#include "spargel/text/font_mac.h"
#include "spargel/text/font_manager.h"

namespace spargel::text {

class FontManagerMac final : public FontManager {
public:
  FontManagerMac() : FontManager("CoreText") { create_default_font(); }

  ~FontManagerMac() {
    if (default_font_) {
      delete default_font_;
    }
  }

  Font* default_font() override { return default_font_; }
  FontMac* translate_font(CTFontRef font);

private:
  void create_default_font();

  FontMac* default_font_ = nullptr;
  std::unordered_map<CTFontRef, FontMac*> fonts_;
};

}  // namespace spargel::text
