#pragma once

#include "spargel/base/bit_cast.h"
#include "spargel/base/hash.h"
#include "spargel/base/hash_map.h"
#include "spargel/gpu/gpu_context.h"
#include "spargel/math/vector.h"
#include "spargel/render/atlas_packer.h"
#include "spargel/text/font.h"

namespace spargel::text {
    class TextShaper;
}

namespace spargel::render {
    class UIScene;

    struct SubpixelVariant {
        u8 x;
        u8 y;

        friend bool operator==(SubpixelVariant const& lhs,
                               SubpixelVariant const& rhs) {
            return lhs.x == rhs.x && lhs.y == rhs.y;
        }
        friend void tag_invoke(base::tag<base::hash>, base::HashRun& run,
                               SubpixelVariant const& self) {
            run.combine(self.x);
            run.combine(self.y);
        }
    };

    class UIRenderer {
    public:
        // TODO: Redesign.
        struct TextureHandle {
            u16 x;
            u16 y;
            u16 width;
            u16 height;
        };

        static constexpr usize ATLAS_SIZE = 2048;

        virtual ~UIRenderer() = default;

        gpu::GPUContext* context() const { return context_; }
        text::TextShaper* textShaper() const { return text_shaper_; }

        virtual void render(UIScene const& scene) = 0;

        TextureHandle prepareGlyph(text::GlyphId id, text::Font* font);
        TextureHandle prepareGlyph(text::GlyphId id, text::Font* font,
                                   SubpixelVariant subpixel);
        TextureHandle prepareGlyph(text::GlyphId id, text::Font* font,
                                   math::Vector2f subpixel_position);

        void setScaleFactor(float s) { scale_factor_ = s; }
        float scaleFactor() const { return scale_factor_; }

    protected:
        explicit UIRenderer(gpu::GPUContext* context,
                            text::TextShaper* text_shaper)
            : context_{context},
              text_shaper_{text_shaper},
              packer_{ATLAS_SIZE, ATLAS_SIZE} {}

        virtual void uploadBitmap(TextureHandle handle,
                                  text::Bitmap const& bitmap) = 0;

    private:
        struct GlyphCacheKey {
            text::GlyphId id;
            text::Font* font;
            SubpixelVariant subpixel;

            friend bool operator==(GlyphCacheKey const& lhs,
                                   GlyphCacheKey const& rhs) {
                return lhs.id == rhs.id && lhs.font == rhs.font &&
                       lhs.subpixel == rhs.subpixel;
            }
            friend void tag_invoke(base::tag<base::hash>, base::HashRun& run,
                                   GlyphCacheKey const& self) {
                run.combine(self.id);
                run.combine(base::bit_cast<text::Font const*, u64>(self.font));
                run.combine(self.subpixel);
            }
        };

        gpu::GPUContext* context_;
        text::TextShaper* text_shaper_;
        AtlasPacker packer_;
        float scale_factor_ = 1.0;

        base::HashMap<GlyphCacheKey, TextureHandle> glyph_cache_;
    };
    base::UniquePtr<UIRenderer> makeUIRenderer(gpu::GPUContext* context,
                                               text::TextShaper* text_shaper);
}  // namespace spargel::render
