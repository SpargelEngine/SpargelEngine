#pragma once

#include "spargel/base/bit_cast.h"
#include "spargel/base/hash.h"
#include "spargel/base/string_view.h"

namespace spargel::text {
    class Font;

    // StyledText is the input of the text rendering pipeline (and others as
    // well).
    //
    // The basic model is text with attributes, which is the same as CoreText
    // and Pango (TODO: others). Every attached attribute has a start and end
    // index (Unicode codepoints).
    //
    // Example (taken from Pango):
    //
    //   Blue text is cool.
    //   ---------    ----
    //   +    -------   +--> font: { style: italic }
    //   +      +---> font: { size: 12pt }
    //   +-> color: blue
    //
    // StyledText is platform independent.
    //
    class StyledText {
    public:
        StyledText(base::StringView s, Font* font) : text_{s}, font_{font} {}

        base::StringView text() const { return text_; }
        Font* font() const { return font_; }

        void setText(base::StringView s) { text_ = s; }

        friend bool operator==(StyledText const& lhs, StyledText const& rhs) {
            return lhs.text_ == rhs.text_ && lhs.font_ == rhs.font_;
        }

        friend void tag_invoke(base::tag<base::hash>, base::HashRun& run,
                               StyledText const& self) {
            run.combine(self.text_);
            run.combine(base::bit_cast<Font*, u64>(self.font_));
        }

    private:
        base::StringView text_;
        Font* font_;
    };
}  // namespace spargel::text
