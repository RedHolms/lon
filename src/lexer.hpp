#pragma once

#include <stdio.h>
#include <string>
#include <string_view>
#include <list>

namespace lon {

  enum TokenID {
    // all one-char tokens are just char value
    // because of that, all other tokens start from 256

    // literals
    TK_ID = 256, // variable, function, etc.
    TK_STRING,
    TK_NUMBER,

    // multiple chars
    TK_RET_ARROW,

    // keywords
    TK_FUNCTION,
    TK_RETURN
  };

  struct Token {
    TokenID id;
    int row; // starts from 1
    int column; // starts from 1
    std::string value; // can be empty
  };

  class LexerError : public std::exception {
  private:
    int m_row;
    int m_column;

  public:
    LexerError(const char* info, int row, int column)
      : m_row(row), m_column(column), std::exception(info) {}

    inline int row() const { return m_row; }
    inline int column() const { return m_column; }
  };

  class Lexer {
  private:
    std::list<Token> m_tokens;
    int m_row;
    int m_column;

    // current char pointer
    const char* m_pt;

  public:
    Lexer();
    ~Lexer();

  public:
    void feed(const char* input);
    void debugPrint();

    std::list<Token> const& getResult() const {
      return m_tokens;
    }

  private:
    void token(TokenID id, const char* value, int row, int column);
    void next();
  };

} // namespace lon
