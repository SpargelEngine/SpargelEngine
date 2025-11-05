#include "spargel/ui/context.h"

// #include <cstring>

#include "spargel/base/check.h"
#include "spargel/base/logging.h"

namespace spargel::ui {

Context& Context::get() {
  static Context ctx;
  return ctx;
}
Context::~Context() {
  if (backend_) {
    delete backend_;
  }
}
void Context::init() {
  backend_ = create_backend();
  CHECK(backend_);
  LOG_INFO("init ui context (backend: %s)", backend_->name());
  backend_->init();
  font_manager_ = text::FontManager::create();
  font_ = font_manager_->default_font();
  packer_.pack(1, 1);
}
void Context::set_title(char const* title) {
  CHECK(backend_);
  backend_->set_title(title);
}
void Context::start_with(RenderDelegate* delegate) {
  CHECK(backend_);
  CHECK(delegate);
  backend_->start_with(delegate);
}
TexturePart Context::prepare_glyph(uint32_t glyph, text::Font* font,
                                   math::Vec2f subpixel) {
  GlyphCacheKey key{glyph, font};
  auto find_result = glyph_cache_.find(key);
  if (find_result != glyph_cache_.end()) {
    return find_result->second;
  }

  auto bitmap = font->rasterize_glyph(glyph, 2.0, subpixel);
  auto width = bitmap.width;
  auto height = bitmap.height;
  auto pack_result = packer_.pack(width, height);
  if (!pack_result) {
    LOG_ERROR("cannot pack glyph");
    __builtin_trap();
  }
  TexturePart handle{uint16_t(pack_result->x), uint16_t(pack_result->y), width,
                     height};
  if (width > 0 && height > 0) {
    backend_->upload_bitmap(handle, bitmap);
  }

  glyph_cache_.emplace(key, handle);
  return handle;
}

void InputState::dump() {
  LOG_INFO("input state: %.3f %.3f down:%d released:%d", mouse_position.x,
           mouse_position.y, mouse_down, mouse_released);
}

}  // namespace spargel::ui
