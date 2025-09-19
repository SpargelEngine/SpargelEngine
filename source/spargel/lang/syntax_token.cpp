#include "spargel/lang/syntax_token.h"

#include "spargel/base/console.h"
#include "spargel/lang/token_kind.h"

//
#include <stdio.h>

namespace spargel::lang {
    void SyntaxToken::dump(usize indent) const {
        auto& console = base::Console::instance();
        console.write_n(' ', indent);
        console.write("[token] ");
        console.write(toStringLiteral(kind));
        console.write(" ");
        if (kind != TokenKind::whitespace) {
            console.write(text);
        }
        if (missing) {
            console.write(" <missing>");
        }
        console.write("\n");
    }
}  // namespace spargel::lang
