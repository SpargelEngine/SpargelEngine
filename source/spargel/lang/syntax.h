#pragma once

#include <spargel/base/either.h>
#include <spargel/base/optional.h>
#include <spargel/base/string_view.h>
#include <spargel/base/tagged_union.h>

namespace spargel::lang {

    enum class TokenKind {
        //------------------------
        // Newline and Whitespace
        //
        // Definition (cf. UTR #13 and UAX #31)
        //  - NLF (new line function) : any of { CR, LF, CRLF, NEL (U+0085) }, depending on the platform
        //  - LS (line separator) : U+2028
        //  - PS (paragraph separator) : U+2029
        //  - Pattern_White_Space characters :
        //      the set { ??? }
        //
        // The syntax is sensitive to line breaks. So newline is separated from whitespace.
        //
        // Decision:
        //  - Only U+000D (CR) and U+000A (LF) are considered as newline.
        //  - Only U+0020 (space) and U+0009 (horizontal tab) are considered as whitespace.
        //
        // Other choices:
        //  - https://go.dev/ref/spec#Tokens
        //  - https://docs.swift.org/swift-book/documentation/the-swift-programming-language/lexicalstructure/#Whitespace-and-Comments
        //
        newline,
        whitespace,

        line_comment,

        identifier,

        left_paren,
        right_paren,
        left_brace,
        right_brace,

        at,
        comma,
        equal,
    };

    enum class KeywordKind {
        import,
        define,
    };

    struct Token {
        TokenKind kind;
        char const* begin;
        char const* end;
        base::Optional<KeywordKind> maybe_keyword;

        Token(TokenKind k, char const* b, char const* e) : Token(k, b, e, base::nullopt) {}

        Token(TokenKind k, char const* b, char const* e, base::Optional<KeywordKind> keyword) : kind{k}, begin{b}, end{e}, maybe_keyword(keyword) {
            spargel_check(b <= e);
        }

        usize length() const { return end - begin; }

        base::string_view toStringView() const { return base::string_view(begin, end); }

        bool isKeywordLike() const { return maybe_keyword.hasValue(); }
    };

    enum class LexError {
        // The cursor has reached the end of the buffer. So there is nothing to lex.
        end_of_buffer,

        // Null character is not allowed inside the buffer.
        unexpected_null_byte,

        // Usually some feature not implemented yet.
        internal_error,
    };

    //=============
    // class Lexer
    //
    // A lossless tokenizer.
    //
    class Lexer {
    public:
        using LexResult = base::Either<Token, LexError>;

        Lexer(base::string_view source) : _begin{source.begin()}, _end{source.end()}, _cur{source.begin()} {}

        // Whether the end of buffer is reached.
        bool isEnd() const { return _cur >= _end; }

        // Advance the cursor by `n`.
        // 
        // Note:
        //  - The cursor stops at the end of the buffer.
        //
        void advance(usize n = 1) {
            if (_cur + n >= _end) {
                _cur = _end;
            } else {
                _cur += n;
            }
        }

        // Get the `n`-th byte from the current position. Return 0 if the `n`-th byte goes out the buffer.
        char peekByte(usize n = 0) const {
            if (_cur + n >= _end) return 0;
            return *(_cur + n);
        }

        LexResult lex();
        LexResult lexNewline();
        LexResult lexWhitespace();
        LexResult lexLineComment();
        LexResult lexIdentifier();
        
        // Keep advancing until the condition on the character is false.
        void eatWhile(auto&& f) {
            while (!isEnd()) {
                if (!f(peekByte())) return;
                advance();
            }
        }

        // Reset state.
        void reset() { _cur = _begin; }

    private:
        char const* _begin;
        char const* _end;
        char const* _cur;
    };

    class SourceRange {
    public:
    private:
        char const* _begin;
        char const* _end;
    };

    class ASTNodeBase {
    public:
        SourceRange range() const { return _range; }

    private:
        SourceRange _range;
    };

    enum class DeclarationKind {
        // `import Builtin`
        import,
        // `def pi = 3.14159`
        define,
    };

    class ImportDeclaration : public ASTNodeBase {
    public:
    private:
    };

    class DefineDeclaration : public ASTNodeBase {
    public:
    private:
    };

    using ASTDeclaration = base::TaggedUnion<
        base::Case<DeclarationKind::import, ImportDeclaration>,
        base::Case<DeclarationKind::define, DefineDeclaration>>;

    enum class ParseError {
        internal_error,
    };

    //==============
    // class Parser
    //
    // Parse into the loseless AST.
    //
    class Parser {
    public:
        Parser(base::string_view source) : _lexer(source) {}

        base::Either<ASTDeclaration, ParseError> parseDeclaration();
        base::Either<ImportDeclaration, ParseError> parseImportDeclaration();
        base::Either<DefineDeclaration, ParseError> parseDefineDeclaration();

    private:
        Lexer _lexer;
    };

}
