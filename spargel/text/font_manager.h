#pragma once

namespace spargel::text {

class Font;

class FontManager {
public:
  static FontManager* create();

  virtual ~FontManager() = default;

  virtual Font* default_font() = 0;

protected:
  explicit FontManager(char const* backend);
};

}  // namespace spargel::text
