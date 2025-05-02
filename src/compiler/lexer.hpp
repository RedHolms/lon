#pragma once

#include <stdio.h>
#include <string>
#include <list>

namespace lon {

  enum {
    // all one-char tokens are just char value
    // because of that, all other tokens start from 256

    // literals
    TK_ID = 256, // variable name, function name, type name, etc.
    TK_STRING,
    TK_NUMBER_INT,
    TK_NUMBER_FLOAT,

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
    std::string strValue; // TK_ID, TK_STRING
    union {
      uint64_t intValue; // TK_NUMBER_INT
      double fltValue; // TK_NUMBER_FLOAT
    };
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

  struct LexerResult {
    std::string inputFileName;
    std::list<Token> tokens;
  };

  class Lexer {
  private:
    std::string m_inputFileName;
    std::string m_content;

    // position of m_pt
    int m_row;
    int m_column;

    // position of the current token beginning
    int m_tkRow;
    int m_tkColumn;

    // current char pointer
    const char* m_pt;

    std::list<Token> m_tokens;

  public:
    Lexer(std::string_view inputFilePath, std::string_view replaceContent = "");
    ~Lexer();

  public:
    void tokenize();
    void debugPrint();

    LexerResult getResult() const;

  private:
    void processNumber();

    void token(TokenID id);
    void token(TokenID id, std::string_view value);
    void token(TokenID id, uint64_t value);
    void token(TokenID id, double value);
    void next(int amount = 1);
  };

} // namespace lon
