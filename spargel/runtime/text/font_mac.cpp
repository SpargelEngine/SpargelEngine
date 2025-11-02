#include "spargel/runtime/text/font_mac.h"

#include "spargel/runtime/base/check.h"
#include "spargel/runtime/base/logging.h"

namespace spargel::text {

FontMac::FontMac(CTFontRef font) : font_{font} {
  // TODO(tianjiao): remove
  dump_info();
}
FontMac::~FontMac() { CFRelease(font_); }

Bitmap FontMac::rasterize_glyph(uint32_t id, float scale,
                                math::Vec2f subpixel_position) {
  GlyphInfo info = glyph_info(id);
  auto rect = info.bounding_box;

  size_t width = (size_t)ceil(rect.z * scale);
  size_t height = (size_t)ceil(rect.w * scale);

  if (width == 0 || height == 0) {
    LOG_INFO("zero sized glyph: %u %.3f %.3f", id, rect.z, rect.w);
    return {};
  }

  // Make room for anti-aliasing, 1px in each direction.
  // TODO(tianjiao): Do we really need the extra 1px intended for subpixel
  // position?
  width += 2;
  height += 2;

  // alpha only
  Bitmap bitmap;
  bitmap.width = (uint16_t)width;
  bitmap.height = (uint16_t)height;
  bitmap.data.resize(width * height);
  memset(bitmap.data.data(), 0x00, width * height);

  auto color_space = CGColorSpaceCreateDeviceGray();
  auto ctx = CGBitmapContextCreate(bitmap.data.data(), width, height,
                                   /*bitsPerComponent=*/8,
                                   /*bytesPerRow=*/width, color_space,
                                   kCGImageAlphaOnly);
  // NOTE: Scale does not change the translate part.
  //
  // TODO: Read the docs. Why we have to translate before scale?
  CGContextTranslateCTM(ctx, -rect.x * scale, -rect.y * scale);
  CGContextScaleCTM(ctx, scale, scale);

  // TODO: It seems there's no need for this.
  // TODO: we already do quantization and there is no need for core graphics
  // to quantize CGContextSetAllowsFontSubpixelPositioning(ctx, true);
  //
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
  CGPoint point = CGPointMake(1 + subpixel_position.x, 1 + subpixel_position.y);

  CGGlyph glyph = (CGGlyph)id;
  CTFontDrawGlyphs(font_, &glyph, &point, 1, ctx);

  CFRelease(color_space);
  CGContextRelease(ctx);

  return bitmap;
}

GlyphInfo FontMac::glyph_info(uint32_t id) {
  CHECK(font_);

  CGGlyph glyph = (CGGlyph)id;

  CGRect rect = {};
  CTFontGetBoundingRectsForGlyphs(font_, kCTFontOrientationHorizontal, &glyph,
                                  &rect, 1);

  CGSize advance = {};
  CTFontGetAdvancesForGlyphs(font_, kCTFontOrientationHorizontal, &glyph,
                             &advance, 1);

  GlyphInfo info;
  info.bounding_box.x = (float)rect.origin.x;
  info.bounding_box.y = (float)rect.origin.y;
  info.bounding_box.z = (float)rect.size.width;
  info.bounding_box.w = (float)rect.size.height;
  info.horizontal_advance = (float)advance.width;

  return info;
}

namespace {
char const* bool_to_str(bool b) { return b ? "true" : "false"; }
}  // namespace

void FontMac::dump_info() {
  CFStringRef display_name = CTFontCopyPostScriptName(font_);

  CTFontSymbolicTraits symbolic_traits = CTFontGetSymbolicTraits(font_);
  bool italic = symbolic_traits & kCTFontTraitItalic;
  bool bold = symbolic_traits & kCTFontTraitBold;
  bool monospace = symbolic_traits & kCTFontTraitMonoSpace;
  CTFontStylisticClass stylistic_class =
      symbolic_traits & kCTFontTraitClassMask;
  bool sans_serif = stylistic_class & kCTFontClassSansSerif;

  LOG_INFO(
      "CoreText font: %s %p [italic: %s bold: %s monospace: %s sans-serif: "
      "%s] ",
      CFStringGetCStringPtr(display_name, kCFStringEncodingASCII), (void*)font_,
      bool_to_str(italic), bool_to_str(bold), bool_to_str(monospace),
      bool_to_str(sans_serif));
  CFRelease(display_name);
}

}  // namespace spargel::text
