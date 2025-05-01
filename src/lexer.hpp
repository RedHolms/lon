#pragma once

#include <stdio.h>
#include <string>
#include <string_view>
#include <list>

namespace lon {

  enum {
    // all one-char tokens are just char value
    // because of that, all other tokens start from 256

    // literals
    TK_ID = 256, // variable name, function name, type name, etc.
    TK_STRING,
    TK_NUMBER,

    // multiple chars
    TK_RET_ARROW,

    // keywords
    TK_FUNCTION,
    TK_RETURN,
    TK_CONST,
    TK_SIGNED,
    TK_UNSIGNED,
    TK_VOID,
    TK_BYTE,
    TK_SHORT,
    TK_INTEGER,
    TK_LONG,
    TK_CHAR,
    TK_BOOLEAN
  };

  // do like that so we can implicitly convert chars to token id
  using TokenID = short;

  std::string TokenIDToString(TokenID id);

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
