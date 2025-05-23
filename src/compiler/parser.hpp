#pragma once

#include <list>
#include "lexer.hpp"
#include "ast/ast.hpp"

namespace lon {

  class ParserError : public std::exception {
  private:
    std::string m_info;
    int m_row;
    int m_column;

  public:
    ParserError(std::string_view info, int row, int column)
      : m_row(row), m_column(column), m_info(info), std::exception() {}

    ParserError(std::string_view info, Token* tk)
      : ParserError(info, tk->row, tk->column) {}
    ParserError(std::string_view info, std::list<Token>::const_iterator tk)
      : ParserError(info, tk->row, tk->column) {}

    virtual const char* what() const { return m_info.c_str(); }

    inline int row() const { return m_row; }
    inline int column() const { return m_column; }
  };

  class Parser {
  private:
    std::string m_inputFileName;
    std::list<Token> m_textTokens;

    // current token
    std::list<Token>::const_iterator m_tk;

    AbstractSourceTree m_ast;

  public:
    Parser(LexerResult const& lexerResult);
    ~Parser();

  public:
    void parse();
    void debugPrint();

    AbstractSourceTree const& getAST() const {
      return m_ast;
    }

  private:
    Type parseTypeName();
    Expression parseExpression();
    std::list<Statement> parseBlock();

    void next();
    bool end();
    void assertToken(TokenID id);

    void printType(Type const* tp);
    void printLiteral(Literal const* lit);
    void printExpr(Expression const* expr, int indent);
    void printBlock(std::list<Statement> const& statements, int indent);
  };

} // namespace lon
