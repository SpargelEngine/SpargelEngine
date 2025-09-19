#include "spargel/lang/lexer.h"

#include <stdio.h>

namespace spargel::lang {
    namespace {
        bool isWhitespace(char c) { return c == ' ' || c == '\n'; }
        bool isIdentifierStart(char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
        }
        bool isIdentifierContinuation(char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   c == '_' || (c >= '0' && c <= '9');
        }
    }  // namespace
    SyntaxToken Cursor::nextToken() {
        auto start = input_;
        if (isAtEnd()) {
            return makeToken(start, TokenKind::end_of_file);
        }

        auto c = peek();

        if (isWhitespace(c)) {
            return handleWhitespace();
        }

        if (isIdentifierStart(c)) {
            return handleIdentifierOrKeyword();
        }

        switch (c) {
        case '{': {
            advance();
            return makeToken(start, TokenKind::left_brace);
        }
        case '}': {
            advance();
            return makeToken(start, TokenKind::right_brace);
        }
        case '(': {
            advance();
            return makeToken(start, TokenKind::left_paren);
        }
        case ')': {
            advance();
            return makeToken(start, TokenKind::right_paren);
        }
        case '[': {
            advance();
            return makeToken(start, TokenKind::left_square);
        }
        case ']': {
            advance();
            return makeToken(start, TokenKind::right_square);
        }
        case '@': {
            advance();
            return makeToken(start, TokenKind::at_sign);
        }
        case ':': {
            advance();
            return makeToken(start, TokenKind::colon);
        }
        case ',': {
            advance();
            return makeToken(start, TokenKind::comma);
        }
        case '.': {
            advance();
            return makeToken(start, TokenKind::period);
        }
        default: {
            advance();
            return makeToken(start, TokenKind::unknown);
        }
        }
        // unreachable
    }
    SyntaxToken Cursor::handleWhitespace() {
        auto start = input_;
        advanceWhile([](char c) { return isWhitespace(c); });
        return makeToken(start, TokenKind::whitespace);
    }
    SyntaxToken Cursor::handleIdentifierOrKeyword() {
        auto start = input_;
        advanceWhile([](char c) { return isIdentifierContinuation(c); });
        base::StringView text{start, input_};
        auto result = tryConvertKeyword(text);
        if (result) {
            return SyntaxToken{result.value(), text};
        }
        return SyntaxToken{TokenKind::identifier, text};
    }
}  // namespace spargel::lang
