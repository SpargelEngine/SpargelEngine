#include "spargel/render/ui_renderer.h"

#include "spargel/base/checked_convert.h"
#include "spargel/config.h"
#include "spargel/logging/logging.h"
#include "spargel/render/constants.h"

namespace spargel::render {

#if SPARGEL_IS_MACOS
    base::UniquePtr<UIRenderer> makeMetalUIRenderer(gpu::GPUContext* context,
                                                    text::TextShaper* shaper);
#endif

    base::UniquePtr<UIRenderer> makeUIRenderer(gpu::GPUContext* context,
                                               text::TextShaper* shaper) {
#if SPARGEL_IS_MACOS
        return makeMetalUIRenderer(context, shaper);
#endif
        return nullptr;
    }
    // TODO: Cache.
    UIRenderer::TextureHandle UIRenderer::prepareGlyph(text::GlyphId id,
                                                       text::Font* font) {
        auto bitmap = font->rasterizeGlyph(id, scale_factor_);
        auto width = base::checkedConvert<u16>(bitmap.width);
        auto height = base::checkedConvert<u16>(bitmap.height);
        auto pack_result = packer_.pack(width, height);
        if (!pack_result.hasValue()) {
            spargel_log_error("cannot pack glyph");
            spargel_panic_here();
        }
        TextureHandle handle{pack_result.value().x, pack_result.value().y,
                             width, height};
        if (width > 0 && height > 0) {
            uploadBitmap(handle, bitmap);
        }
        return handle;
    }
    UIRenderer::TextureHandle UIRenderer::prepareGlyph(
        text::GlyphId id, text::Font* font, SubpixelVariant subpixel) {
        GlyphCacheKey key{id, font, subpixel};
        auto* result = glyph_cache_.get(key);
        if (result) {
            return *result;
        }
        // TODO: Do not hardcode 0.25.
        constexpr float DELTA = 1.0f / SUBPIXEL_SUBDIVISION;
        auto handle = prepareGlyph(id, font,
                                   math::Vector2f{(float)subpixel.x * DELTA,
                                                  (float)subpixel.y * DELTA});
        glyph_cache_.set(key, handle);
        spargel_check(glyph_cache_.get(key));
        return handle;
    }
    UIRenderer::TextureHandle UIRenderer::prepareGlyph(
        text::GlyphId id, text::Font* font, math::Vector2f subpixel_position) {
        auto bitmap =
            font->rasterizeGlyph(id, scale_factor_, subpixel_position);
        auto width = base::checkedConvert<u16>(bitmap.width);
        auto height = base::checkedConvert<u16>(bitmap.height);
        auto pack_result = packer_.pack(width, height);
        if (!pack_result.hasValue()) {
            spargel_log_error("cannot pack glyph");
            spargel_panic_here();
        }
        TextureHandle handle{pack_result.value().x, pack_result.value().y,
                             width, height};
        if (width > 0 && height > 0) {
            uploadBitmap(handle, bitmap);
        }
        return handle;
    }
}  // namespace spargel::render
