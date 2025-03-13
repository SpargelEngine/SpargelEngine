#include <spargel/base/check.h>
#include <spargel/base/test.h>
#include <spargel/lang/syntax.h>

using spargel::lang::Lexer;
using spargel::lang::TokenKind;
using spargel::lang::LexError;

TEST(Lexer_Basic) {
    Lexer l("def hello()\n  print(\"hello, world\")\n");
    spargel_check(!l.isEnd());
    spargel_check(l.peekByte() == 'd');
    l.advance();
    spargel_check(l.peekByte() == 'e');
    l.advance();
    spargel_check(l.peekByte() == 'f');
    l.advance(2);
    spargel_check(l.peekByte() == 'h');
    spargel_check(l.peekByte(5) == '(');

    l.reset();

    l.eatWhile([](char c) { return c >= 'a' && c <= 'z'; });
    spargel_check(l.peekByte() == ' ');
}

TEST(Lexer_Null) {
    Lexer l("\0hello");
    auto x = l.lex();
    spargel_check(x.isRight());
    spargel_check(x.right() == LexError::unexpected_null_byte);
}

TEST(Lexer_Newline) {
    Lexer l("\n\r\n\rxyz\n");
    auto x = l.lex();
    spargel_check(!l.isEnd() && l.peekByte() == 'x');
    spargel_check(x.isLeft());
    spargel_check(x.left().kind == TokenKind::newline);
    spargel_check(x.left().length() == 4);
    spargel_check(x.left().toStringView() == "\n\r\n\r");
}

TEST(Lexer_Whitespace) {
    Lexer l(" \t  xyz\n");
    auto x = l.lex();
    spargel_check(!l.isEnd() && l.peekByte() == 'x');
    spargel_check(x.isLeft());
    spargel_check(x.left().kind == TokenKind::whitespace);
    spargel_check(x.left().length() == 4);
    spargel_check(x.left().toStringView() == " \t  ");
}

TEST(Lexer_LineComment) {
    Lexer l("// hello\nxyz");
    auto x = l.lex();
    spargel_check(!l.isEnd() && l.peekByte() == '\n');
    spargel_check(x.isLeft());
    spargel_check(x.left().kind == TokenKind::line_comment);
    spargel_check(x.left().length() == 6); // " hello"
    spargel_check(x.left().toStringView() == " hello");
}

TEST(Lexer_LineComment_EOF) {
    Lexer l("// hello");
    auto x = l.lex();
    spargel_check(l.isEnd());
    spargel_check(x.isLeft());
    spargel_check(x.left().kind == TokenKind::line_comment);
    spargel_check(x.left().length() == 6); // " hello"
    spargel_check(x.left().toStringView() == " hello");
}

TEST(Lexer_Paren) {
    Lexer l("( )");
    auto x = l.lex();
    spargel_check(x.isLeft());
    spargel_check(x.left().kind == TokenKind::left_paren);
    spargel_check(x.left().toStringView() == "(");
    l.lex();
    auto y = l.lex();
    spargel_check(y.isLeft());
    spargel_check(y.left().kind == TokenKind::right_paren);
    spargel_check(y.left().toStringView() == ")");
}
