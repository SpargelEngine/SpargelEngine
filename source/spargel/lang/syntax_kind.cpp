#include "spargel/lang/syntax_kind.h"

namespace spargel::lang {
    char const* toStringLiteral(SyntaxKind kind) {
        switch (kind) {
#define SYNTAX_KIND(name)  \
    case SyntaxKind::name: \
        return #name;
#include "spargel/lang/syntax_kind.inc"
        }
    }
}  // namespace spargel::lang
