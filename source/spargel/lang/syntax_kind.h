#pragma once

namespace spargel::lang {
    enum class SyntaxKind {
#define SYNTAX_KIND(name) name,
#include "spargel/lang/syntax_kind.inc"
    };
    char const* toStringLiteral(SyntaxKind kind);
}  // namespace spargel::lang
