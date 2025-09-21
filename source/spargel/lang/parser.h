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

        void consumeToken();
        // Consume a token of given `kind` and add to the current node.
        // If the current token does not match `kind`, then add a
        // token marked as "missing".
        void consumeToken(TokenKind kind);

        void consumeWhitespace();

        void beginNode(SyntaxKind kind) { builder_.beginNode(kind); }
        void endNode() { builder_.endNode(); }

        void handleSourceFile();
        void handleSourceFileItem();
        void handleOpenDecl();
        void handleModuleName();
        void handleFuncDecl();
        void handleParamList();
        void handleRetClause();
        void handleFuncSig();

        void handleUnknown();

        Cursor cursor_;
        SyntaxToken current_token_;
        SyntaxBuilder builder_;
    };
}  // namespace spargel::lang
