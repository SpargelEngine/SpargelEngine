#include <spargel/base/check.h>
#include <spargel/lang/syntax.h>

namespace spargel::lang {

    namespace {

        bool isNewline(char c) { return c == '\n' || c == '\r'; }
        bool isWhitespace(char c) { return c == ' ' || c == '\t'; }
        bool isIdentifierBegin(char c) {
            return (c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z')
                || c == '_';
        }
        bool isIdentifierContinuation(char c) {
            return (c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9')
                || c == '_';
        }

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
            
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
            case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
            case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
            case 'V': case 'W': case 'X': case 'Y': case 'Z':
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
            case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
            case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
            case 'v': case 'w': case 'x': case 'y': case 'z':
            case '_':
                return lexIdentifier();

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

    Lexer::LexResult Lexer::lexIdentifier() {
        spargel_check(isIdentifierBegin(peekByte()));

        char const* begin = _cur;
        eatWhile([](char c) {
            return isIdentifierContinuation(c);
        });

        base::string_view ident(begin, _cur);
        base::Optional<KeywordKind> keyword;

        if (ident == "import") { keyword = base::makeOptional<KeywordKind>(KeywordKind::import); }
        if (ident == "define") { keyword = base::makeOptional<KeywordKind>(KeywordKind::define); }
        
        return base::Left{Token(TokenKind::identifier, begin, _cur, keyword)};
    }

    base::Either<ASTDeclaration, ParseError> Parser::parseDeclaration() {
        return base::Right{ParseError::internal_error};
    }

    base::Either<ImportDeclaration, ParseError> Parser::parseImportDeclaration() {
        return base::Right{ParseError::internal_error};
    }

    base::Either<DefineDeclaration, ParseError> Parser::parseDefineDeclaration() {
        return base::Right{ParseError::internal_error};
    }

}
