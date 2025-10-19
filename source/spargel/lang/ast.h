#pragma once

#include "spargel/lang/syntax_kind.h"

namespace spargel::lang {
    class SyntaxNode;

    class ASTBase {
    public:
        SyntaxNode& syntax() { return *syntax_; }

    private:
        SyntaxNode* syntax_;
    };

    class NodeProxy {
    public:
        struct Iterator {};

        explicit NodeProxy(SyntaxKind kind) : kind_{kind} {}

        Iterator begin();
        Iterator end();

    private:
        SyntaxNode* node_;
        SyntaxKind kind_;
    };

    struct SourceFile : public ASTBase {
        auto items() {}
    };
}  // namespace spargel::lang
