#pragma once

#include "spargel/base/unique_ptr.h"
#include "spargel/base/vector.h"
#include "spargel/text/font.h"

namespace spargel::text {
    class FontManager;
    class StyledText;

    struct ShapedSegment {
        base::Vector<GlyphId> glyphs;
        base::Vector<math::Vector2f> positions;
        float width;
        Font* font;
    };
    struct ShapedLine {
        base::Vector<ShapedSegment> segments;
        float ascent;
        float descent;
        float width;
        float leading;
    };

    class TextShaper {
    public:
        static base::UniquePtr<TextShaper> create(FontManager* font_manager);

        virtual ~TextShaper() = default;

        virtual ShapedLine shapeLine(StyledText const& text) = 0;
    };
}  // namespace spargel::text
