#pragma once

namespace spargel::text {

class FontDesc {};

// A concrete, resolved font face.
class Font {
public:
  virtual ~Font() = default;
};

}  // namespace spargel::text
