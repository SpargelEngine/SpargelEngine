#include <spargel/base/check.h>
#include <spargel/lang/syntax.h>

namespace spargel::lang {

    namespace {

        bool isNewline(char c) { return c == '\n' || c == '\r'; }
        bool isWhitespace(char c) { return c == ' ' || c == '\t'; }

    }

    Lexer::LexResult Lexer::lex() {
        if (isEnd()) return base::Right{LexError::end_of_buffer};

        char c = peekByte();

        auto form_token = [start = _cur, this](TokenKind k) {
            return Token(k, start, _cur);
        };

        switch (c) {
            
            // null byte
            case 0:
                return base::Right{LexError::unexpected_null_byte};

            // newline
            case '\n':
            case '\r':
                return lexNewline();

            // whitespace
            case ' ':
            case '\t':
                return lexWhitespace();

            case '/': {
                char next = peekByte(1);
                if (next == '/') {
                    return lexLineComment();
                }
                // TODO
                break;
            }

            case '(': { advance(); return base::Left{form_token(TokenKind::left_paren)}; }
            case ')': { advance(); return base::Left{form_token(TokenKind::right_paren)}; }
            case '{': { advance(); return base::Left{form_token(TokenKind::left_brace)}; }
            case '}': { advance(); return base::Left{form_token(TokenKind::right_brace)}; }
            case '@': { advance(); return base::Left{form_token(TokenKind::at)}; }
            case ',': { advance(); return base::Left{form_token(TokenKind::comma)}; }

            default:
                break;
        }

        return base::Right{LexError::internal_error};
    }

    Lexer::LexResult Lexer::lexNewline() {
        spargel_check(isNewline(peekByte()));

        char const* begin = _cur;
        eatWhile([](char c) {
            return isNewline(c);
        });
        return base::Left{Token(TokenKind::newline, begin, _cur)};
    }

    Lexer::LexResult Lexer::lexWhitespace() {
        spargel_check(isWhitespace(peekByte()));

        char const* begin = _cur;
        eatWhile([](char c) {
            return isWhitespace(c);
        });
        return base::Left{Token(TokenKind::whitespace, begin, _cur)};
    }

    Lexer::LexResult Lexer::lexLineComment() {
        spargel_check(peekByte() == '/' && peekByte(1) == '/');
        
        advance(2);

        char const* begin = _cur;
        eatWhile([](char c) {
            return !isNewline(c);
        });
        return base::Left{Token(TokenKind::line_comment, begin, _cur)};
    }

}
