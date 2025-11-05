#pragma once

#include <stdint.h>

#include "spargel/base/vector.h"

namespace spargel::spsl {

// syntax ----------------------------------------------------------------------

enum class TokenKind {
  kw_open,
  kw_def,

  kw_struct,

  kw_let,

  kw_if,
  kw_loop,
  kw_return,

  kw_const,
  kw_mut,
};

class Token {};

enum class SyntaxKind {
  source_file,
  open_item,
  block,
};

class SyntaxNode {};

// we can deal with up to 2^31 tokens
class SyntaxId {
public:
  bool is_token() const { return value_ & 0x1; }
  uint32_t index() const { return value_ >> 1; }

private:
  // the last bit is the tag: 0 for node and 1 for token
  uint32_t value_ = 0;
};

class SyntaxStore {
public:
  auto match(SyntaxId id, auto&& handle_token, auto&& handle_node) {
    if (id.is_token()) {
      return handle_token();
    } else {
      return handle_node();
    }
  }

private:
  base::vector<Token> tokens_;
  base::vector<SyntaxNode> nodes_;
};

class SourceCursor {
public:
  SourceCursor(char const* begin, char const* end)
      : current_{begin}, end_{end} {}

  bool at_end() const { return current_ >= end_; }

  void advance() {
    if (at_end()) {
      return;
    }
    current_++;
  }

  char current() const { return *current_; }
  char peek_next() const {
    if (current_ + 1 >= end_) {
      return 0;
    }
    return current_[1];
  }

  void eat_while(auto&& pred) {
    while (!at_end() && pred(current())) {
      advance();
    }
  }

private:
  char const* current_ = nullptr;
  char const* end_ = nullptr;
};

class Lexer {
public:
  Lexer(char const* begin, char const* end) : cursor_(begin, end) {}

  void lex_all(SyntaxStore& store);

private:
  SourceCursor cursor_;
};

class Parser;

}  // namespace spargel::spsl
