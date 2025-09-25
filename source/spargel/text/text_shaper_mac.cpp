#include "spargel/text/text_shaper_mac.h"

#include "spargel/text/font_mac.h"
#include "spargel/text/font_manager.h"
#include "spargel/text/font_manager_mac.h"
#include "spargel/text/styled_text.h"

namespace spargel::text {
    ShapedLine TextShaperMac::shapeLine(StyledText const& text) {
        auto font = static_cast<FontMac*>(text.font());

        // Convert to CFString
        auto cfstr = CFStringCreateWithBytes(
            kCFAllocatorDefault, (u8 const*)text.text().data(),
            (CFIndex)text.text().length(), kCFStringEncodingUTF8, false);

        // Create CFAttributedString
        void const* keys[] = {kCTFontAttributeName, kCTLigatureAttributeName};
        int val = 2;
        auto number =
            CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &val);
        void const* values[] = {font->object_, number};
        auto dict = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 1,
                                       NULL, NULL);
        auto attr_str =
            CFAttributedStringCreate(kCFAllocatorDefault, cfstr, dict);

        // Shape using Core Text.
        auto line = CTLineCreateWithAttributedString(attr_str);

        auto glyph_runs = CTLineGetGlyphRuns(line);
        auto run_count = CFArrayGetCount(glyph_runs);

        base::Vector<CGGlyph> glyphs;
        base::Vector<CGPoint> points;

        ShapedLine result;
        ShapedSegment segment;

        {
            CGFloat ascent;
            CGFloat descent;
            CGFloat leading;
            CGFloat width =
                CTLineGetTypographicBounds(line, &ascent, &descent, &leading);

            result.ascent = static_cast<float>(ascent);
            result.descent = -static_cast<float>(descent);
            result.leading = static_cast<float>(leading);
            result.width = static_cast<float>(width);
        }

        for (CFIndex i = 0; i < run_count; i++) {
            CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(glyph_runs, i);
            auto count = CTRunGetGlyphCount(run);

            auto attr = CTRunGetAttributes(run);

            auto font =
                (CTFontRef)CFDictionaryGetValue(attr, kCTFontAttributeName);
            segment.font = font_manager_->translateFont(font);

            glyphs.reserve((usize)count);
            CTRunGetGlyphs(run, CFRangeMake(0, count), glyphs.data());
            glyphs.set_count((usize)count);

            points.reserve((usize)count);
            CTRunGetPositions(run, CFRangeMake(0, count), points.data());
            points.set_count((usize)count);

            segment.glyphs.reserve((usize)count);
            segment.glyphs.set_count((usize)count);
            for (CFIndex j = 0; j < count; j++) {
                segment.glyphs[(usize)j] = {glyphs[(usize)j]};
            }
            segment.positions.reserve((usize)count);
            segment.positions.set_count((usize)count);
            for (CFIndex j = 0; j < count; j++) {
                segment.positions[(usize)j].x = (float)points[(usize)j].x;
                segment.positions[(usize)j].y = (float)points[(usize)j].y;
            }

            segment.width = (float)CTRunGetTypographicBounds(
                run, CFRangeMake(0, count), NULL, NULL, NULL);

            result.segments.emplace(base::move(segment));
        }

        CFRelease(line);
        CFRelease(attr_str);
        CFRelease(dict);
        CFRelease(number);
        CFRelease(cfstr);

        return result;
    }
    base::UniquePtr<TextShaper> TextShaper::create(FontManager* font_manager) {
        return base::makeUnique<TextShaperMac>(
            static_cast<FontManagerMac*>(font_manager));
    }
}  // namespace spargel::text
