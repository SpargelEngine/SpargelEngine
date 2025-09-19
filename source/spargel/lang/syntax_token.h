#pragma once

#include "spargel/base/string_view.h"
#include "spargel/lang/token_kind.h"

namespace spargel::lang {
    struct SyntaxToken {
        TokenKind kind;
        base::StringView text;
        bool missing = false;

        void dump(usize indent) const;
    };
}  // namespace spargel::lang
