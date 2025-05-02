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
    LexerResult m_lexerResult;
    std::list<std::shared_ptr<RootStatement>> m_rootStatements;

    // current token
    std::list<Token>::const_iterator m_tk;

  public:
    Parser(LexerResult const& lexerResult);
    ~Parser();

  public:
    void parse();
    void debugPrint();

//    AbstractSourceTree const& getAST() const;
  std::list<std::shared_ptr<RootStatement>> getResult();

  private:
    std::shared_ptr<Type> parseTypeName();
    std::shared_ptr<Expression> parseExpression();
    std::list<std::shared_ptr<Statement>> parseBlock();

    void next();
    bool end();
    void assertToken(TokenID id);

    void printType(Type* tp);
    void printLiteral(Literal* lit);
    void printExpr(Expression* expr, int indent);
    void printBlock(std::list<std::shared_ptr<Statement>> const& statements, int indent);
  };

} // namespace lon
