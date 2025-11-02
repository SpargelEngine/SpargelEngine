#include <CoreFoundation/CoreFoundation.h>

#include "spargel/runtime/text/font_mac.h"
#include "spargel/runtime/text/font_manager_mac.h"
#include "spargel/runtime/text/shaping.h"

namespace spargel::text {

TextLine shape_line(const char* text, Font* font, FontManager* manager) {
  FontMac* font_mac = static_cast<FontMac*>(font);
  FontManagerMac* manager_mac = static_cast<FontManagerMac*>(manager);

  // convert to CFString
  CFStringRef cfstr = CFStringCreateWithBytes(
      nullptr, (unsigned char const*)text, (CFIndex)strlen(text),
      kCFStringEncodingUTF8, false);

  // Create CFAttributedString
  void const* keys[] = {kCTFontAttributeName, kCTLigatureAttributeName};
  int val = 2;
  CFNumberRef number = CFNumberCreate(nullptr, kCFNumberSInt32Type, &val);
  void const* values[] = {font_mac->get(), number};
  CFDictionaryRef dict = CFDictionaryCreate(kCFAllocatorDefault, keys, values,
                                            1, nullptr, nullptr);
  CFAttributedStringRef attr_str =
      CFAttributedStringCreate(nullptr, cfstr, dict);

  // Shape using Core Text.
  auto line = CTLineCreateWithAttributedString(attr_str);

  auto glyph_runs = CTLineGetGlyphRuns(line);
  auto run_count = CFArrayGetCount(glyph_runs);

  TextLine result;

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

  std::vector<CGGlyph> glyphs;
  std::vector<CGPoint> points;

  for (CFIndex i = 0; i < run_count; i++) {
    CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(glyph_runs, i);
    size_t count = (size_t)CTRunGetGlyphCount(run);

    auto attr = CTRunGetAttributes(run);

    auto font = (CTFontRef)CFDictionaryGetValue(attr, kCTFontAttributeName);

    TextRun result_run;
    result_run.font = manager_mac->translate_font(font);

    glyphs.resize(count);
    CTRunGetGlyphs(run, CFRangeMake(0, (CFIndex)count), glyphs.data());

    points.resize(count);
    CTRunGetPositions(run, CFRangeMake(0, (CFIndex)count), points.data());

    size_t offset = result.glyphs.size();
    result_run.start = offset;
    result_run.end = offset + count;

    result.glyphs.resize(offset + count);
    result.points.resize(offset + count);

    for (size_t j = 0; j < count; j++) {
      result.glyphs[j] = glyphs[j];
      result.points[j].x = (float)points[j].x;
      result.points[j].y = (float)points[j].y;
    }

    result_run.width = (float)CTRunGetTypographicBounds(
        run, CFRangeMake(0, (CFIndex)count), nullptr, nullptr, nullptr);
    result.runs.push_back(result_run);
  }

  CFRelease(line);
  CFRelease(attr_str);
  CFRelease(dict);
  CFRelease(number);
  CFRelease(cfstr);

  return result;
}

}  // namespace spargel::text
