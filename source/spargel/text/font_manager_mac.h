#pragma once

#include "spargel/base/bit_cast.h"
#include "spargel/base/hash.h"
#include "spargel/base/hash_map.h"
#include "spargel/base/tag_invoke.h"
#include "spargel/text/font_mac.h"
#include "spargel/text/font_manager.h"

//
#include <CoreFoundation/CoreFoundation.h>
#include <CoreText/CoreText.h>

namespace spargel::text {
    // Warp the handle.
    struct CoreTextFont {
        CTFontRef object;
        friend bool operator==(CoreTextFont lhs, CoreTextFont rhs) {
            return CFEqual(lhs.object, rhs.object);
        }
        friend void tag_invoke(base::tag<base::hash>, base::HashRun& run,
                               CoreTextFont font) {
            run.combine(base::bit_cast<CFHashCode, u64>(CFHash(font.object)));
        }
    };

    class FontManagerMac final : public FontManager {
    public:
        FontMac* translate_font(CTFontRef font);
        base::UniquePtr<Font> createDefaultFont() override { return nullptr; }
        FontMac* defaultFont() override;
        Font* matchDescriptor(FontDescriptor const& descriptor) override;

    private:
        base::HashMap<CoreTextFont, FontMac*> fonts_;
        FontMac* default_font_ = nullptr;
    };
}  // namespace spargel::text
