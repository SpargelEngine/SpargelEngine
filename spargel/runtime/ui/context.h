#pragma once

#include <unordered_map>

#include "spargel/runtime/text/font.h"
#include "spargel/runtime/text/font_manager.h"
#include "spargel/runtime/text/shaping.h"
#include "spargel/runtime/ui/packer.h"
#include "spargel/runtime/ui/painter.h"

namespace spargel::ui {

class RenderDelegate {
public:
  virtual ~RenderDelegate() = default;
  virtual void render() {}
};

struct TexturePart {
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
};

class Backend {
public:
  virtual ~Backend() = default;
  virtual char const* name() const = 0;
  virtual void init() = 0;
  virtual void set_title(char const*) = 0;
  virtual void start_with(RenderDelegate* delegate) = 0;
  virtual void upload_bitmap(TexturePart part, text::Bitmap const& bitmap) = 0;
};

struct InputState {
  math::Vec2f mouse_position;
  bool mouse_down = false;
  bool mouse_released = false;

  void end_frame() { mouse_released = false; }
};

struct GlyphCacheKey {
  uint32_t id;
  text::Font* font;

  friend bool operator==(GlyphCacheKey const& lhs, GlyphCacheKey const& rhs) {
    return lhs.id == rhs.id && lhs.font == rhs.font;
  }
};
}  // namespace spargel::ui

template <>
struct std::hash<spargel::ui::GlyphCacheKey> {
  std::size_t operator()(const spargel::ui::GlyphCacheKey& s) const noexcept {
    std::size_t h1 = std::hash<uint32_t>{}(s.id);
    std::size_t h2 = std::hash<void*>{}(s.font);
    return h1 ^ (h2 << 1);
  }
};

namespace spargel::ui {

class Context {
public:
  static Context& get();

  ~Context();

  void init();
  void set_title(char const* title);
  void start_with(RenderDelegate* delegate);
  text::TextLine layout_text(char const* text) {
    return text::shape_line(text, font_, font_manager_);
  }

  InputState& input_state() { return input_state_; }
  InputState const& input_state() const { return input_state_; }
  CommandList& command_list() { return cmdlist_; }
  AtlasPacker& packer() { return packer_; }

  TexturePart prepare_glyph(uint32_t glyph, text::Font* font,
                            math::Vec2f subpixel);

private:
  static constexpr size_t ATLAS_SIZE = 256;

  static Backend* create_backend();

  Backend* backend_ = nullptr;
  InputState input_state_;
  CommandList cmdlist_;
  text::FontManager* font_manager_ = nullptr;
  text::Font* font_ = nullptr;
  AtlasPacker packer_{ATLAS_SIZE, ATLAS_SIZE};
  std::unordered_map<GlyphCacheKey, TexturePart> glyph_cache_;
};

}  // namespace spargel::ui
