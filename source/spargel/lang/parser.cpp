#include "spargel/lang/parser.h"

namespace spargel::lang {
    SyntaxNode Parser::parse() {
        handleSourceFile();
        return builder_.result();
    }
    void Parser::handleSourceFile() {
        builder_.unsafeEmplace(SyntaxKind::source_file);
        while (true) {
            consumeWhitespace();
            auto& token = currentToken();
            if (token.kind == TokenKind::end_of_file) {
                break;
            }
            handleSourceFileItem();
        }
    }
    void Parser::handleSourceFileItem() {
        beginNode(SyntaxKind::source_file_item);
        auto& token = currentToken();
        switch (token.kind) {
        case TokenKind::kw_open:
            handleOpenDecl();
            break;
        case TokenKind::kw_func:
            handleFuncDecl();
            break;
        default:
            handleUnknown();
            break;
        }
        endNode();
    }
    void Parser::handleOpenDecl() {
        beginNode(SyntaxKind::open_decl);
        consumeToken(TokenKind::kw_open);
        consumeWhitespace();
        handleModuleName();
        endNode();
    }
    void Parser::handleModuleName() {
        beginNode(SyntaxKind::module_name);
        consumeToken(TokenKind::identifier);
        endNode();
    }
    void Parser::handleFuncDecl() {
        beginNode(SyntaxKind::func_decl);
        consumeToken(TokenKind::kw_func);
        consumeWhitespace();
        consumeToken(TokenKind::identifier);
        consumeWhitespace();
        handleFuncSig();
        endNode();
    }
    void Parser::handleFuncSig() {
        beginNode(SyntaxKind::func_sig);
        handleParamList();
        consumeWhitespace();
        handleRetClause();
        // TODO: parameter list and return clause
        endNode();
    }
    void Parser::handleParamList() {
        beginNode(SyntaxKind::param_list);
        consumeToken(TokenKind::left_paren);
        consumeWhitespace();
        // TODO
        consumeToken(TokenKind::right_paren);
        endNode();
    }
    void Parser::handleRetClause() {
        beginNode(SyntaxKind::ret_clause);
        endNode();
    }
    void Parser::handleUnknown() {
        beginNode(SyntaxKind::unknown);
        consumeToken();
        endNode();
    }
    void Parser::consumeToken() {
        builder_.pushToken(currentToken());
        advanceCursor();
    }
    void Parser::consumeToken(TokenKind kind) {
        if (currentToken().kind == kind) {
            consumeToken();
            return;
        }
        builder_.pushToken({kind, "", true});
    }
    void Parser::consumeWhitespace() {
        while (true) {
            if (currentToken().kind != TokenKind::whitespace) {
                break;
            }
            consumeToken(TokenKind::whitespace);
        }
    }
}  // namespace spargel::lang
