#include "spargel/text/font_mac.h"

#include "spargel/base/checked_convert.h"
#include "spargel/base/string.h"
#include "spargel/logging/logging.h"

//
#include <math.h>

//
#include <CoreGraphics/CoreGraphics.h>

namespace spargel::text {
    namespace {
        CGGlyph toCGGlyph(GlyphId id) {
            return base::checkedConvert<CGGlyph>(id.value);
        }
    }  // namespace
    FontMac::FontMac(CTFontRef object) : object_{object} {
        CFRetain(object_);

        auto name = CTFontCopyFullName(object_);
        auto range = CFRangeMake(0, CFStringGetLength(name));
        CFIndex len;
        spargel_check(CFStringGetBytes(name, range, kCFStringEncodingUTF8, 0,
                                       false, nullptr, 0, &len) > 0);
        name_.resize(base::checkedConvert<usize>(len));
        spargel_check(CFStringGetBytes(name, range, kCFStringEncodingUTF8, 0,
                                       false, (u8*)name_.data(),
                                       (CFIndex)name_.length(), &len) == len);
        CFRelease(name);
    }
    FontMac::~FontMac() { CFRelease(object_); }
    Bitmap FontMac::rasterizeGlyph(GlyphId id, float scale,
                                   math::Vector2f subpixel_position) {
        auto glyph_info = glyphInfo(id);
        auto rect = glyph_info.bounding_box;

        u32 width = (u32)ceil(rect.size.width * scale);
        u32 height = (u32)ceil(rect.size.height * scale);

        if (width == 0 || height == 0) {
            spargel_log_info("zero sized glyph!");
            return {};
        }

        // spargel_log_info("font raster scale: %.3f", scale);

        // Make room for anti-aliasing, 1px in each direction.
        // TODO: Do we need 1px more for subpixel position?
        width += 2;
        height += 2;

        Bitmap bitmap;
        bitmap.width = width;
        bitmap.height = height;
        bitmap.data.reserve(
            width *
            height);  // only alpha. 8 bits per channel and 1 bytes per pixel
        bitmap.data.set_count(width * height);
        memset(bitmap.data.data(), 0x00, width * height);

        auto color_space = CGColorSpaceCreateDeviceGray();
        auto ctx = CGBitmapContextCreate(bitmap.data.data(), width, height,
                                         8,      // bits per channel
                                         width,  // bytes per row
                                         color_space, kCGImageAlphaOnly);
        // NOTE: Scale does not change the translate part.
        //
        // TODO: Read the docs. Why we have to translate before scale?
        CGContextTranslateCTM(ctx, -rect.origin.x * scale,
                              -rect.origin.y * scale);
        CGContextScaleCTM(ctx, scale, scale);

        // TODO: It seems there's no need for this.
        // CGContextSetAllowsFontSubpixelPositioning(ctx, true);
        // CGContextSetAllowsFontSubpixelQuantization(ctx, true);
        // CGContextSetShouldSubpixelPositionFonts(ctx, true);
        // CGContextSetShouldSubpixelQuantizeFonts(ctx, true);

        // TODO: Shift for anti-aliasing.
        // TODO: Subpixel shifting is done here or above.
        //
        // Maybe we should align texture pixel and CoreGraphics pixel.
        // A consequence is that we need to do the subpixel shift here.
        //
        // Add 1 to make room for anti-aliasing. See the note in `ui_scene.cpp`.
        CGPoint point =
            CGPointMake(1 + subpixel_position.x, 1 + subpixel_position.y);

        CGGlyph glyph = base::checkedConvert<CGGlyph>(id.value);
        CTFontDrawGlyphs(object_, &glyph, &point, 1, ctx);

        CFRelease(color_space);
        CGContextRelease(ctx);

        // spargel_log_info("%.3f %.3f", subpixel_position.x,
        // subpixel_position.y); bitmap.dump();

        return bitmap;
    }
    Bitmap FontMac::rasterizeGlyph(GlyphId id, float scale) {
        return rasterizeGlyph(id, scale, math::Vector2f{0.0f, 0.0f});
    }
    // CoreText Coodinate
    // ------------------
    // The origin is at bottom-left and the y-axis goes up.
    //
    GlyphInfo FontMac::glyphInfo(GlyphId id) {
        CGGlyph glyph = toCGGlyph(id);

        CGRect rect;
        CTFontGetBoundingRectsForGlyphs(object_, kCTFontOrientationHorizontal,
                                        &glyph, &rect, 1);

        CGSize advance;
        CTFontGetAdvancesForGlyphs(object_, kCTFontOrientationHorizontal,
                                   &glyph, &advance, 1);

        GlyphInfo info;
        info.bounding_box.origin.x = (float)rect.origin.x;
        info.bounding_box.origin.y = (float)rect.origin.y;
        info.bounding_box.size.width = (float)rect.size.width;
        info.bounding_box.size.height = (float)rect.size.height;
        info.horizontal_advance = (float)advance.width;

        return info;
    }
}  // namespace spargel::text
