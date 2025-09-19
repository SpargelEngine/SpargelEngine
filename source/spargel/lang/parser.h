#pragma once

#include "spargel/lang/lexer.h"
#include "spargel/lang/syntax_node.h"

namespace spargel::lang {
    class Parser {
    public:
        explicit Parser(Cursor cursor) : cursor_{cursor} {
            // Fill the first token.
            current_token_ = cursor_.nextToken();
        }

        SyntaxNode parse();

    private:
        SyntaxToken const& currentToken() const { return current_token_; }
        void advanceCursor() { current_token_ = cursor_.nextToken(); }

        // Consume a token of given `kind` and add to the current node. This
        // will panic if the kind of the current token doesn't match.
        void consumeToken();
        void consumeToken(TokenKind kind);

        void consumeWhitespace();

        void beginNode(SyntaxKind kind) { builder_.beginNode(kind); }
        void endNode() { builder_.endNode(); }

        // SourceFile =
        //   OpenDecl
        //   FuncDecl
        void handleSourceFile();
        void handleSourceFileItem();
        void handleOpenDecl();
        void handleModuleName();
        void handleFuncDecl();

        void handleUnknown();

        Cursor cursor_;
        SyntaxToken current_token_;
        SyntaxBuilder builder_;
    };
}  // namespace spargel::lang
