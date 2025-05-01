#pragma once

#include <list>
#include "lexer.hpp"
#include "ast/statements.hpp"

namespace lon {

  class ParserError : public std::exception {
  private:
    Token const* m_causedToken;

  public:
    ParserError(const char* info, Token const* causedToken)
      : m_causedToken(causedToken), std::exception(info) {}
    ParserError(const char* info, std::list<Token>::const_iterator causedToken)
      : m_causedToken(causedToken.operator->()), std::exception(info) {}

    inline Token const* causedToken() const { return m_causedToken; }
  };

  class Parser {
  private:
    std::list<std::shared_ptr<RootStatement>> m_result;
    std::list<Token> const* m_inputTokens;
    std::list<Token>::const_iterator m_tk;

  public:
    Parser();
    ~Parser();

  public:
    void feed(std::list<Token> const& tokens);
    void debugPrint();

    std::list<std::shared_ptr<RootStatement>> const& getResult() const {
      return m_result;
    }

  private:
    std::shared_ptr<Type> parseTypeName();
    std::shared_ptr<Expression> parseExpression();
    std::list<std::shared_ptr<Statement>> parseBlock();

    void next();
    bool end();
    void assertToken(TokenID id);
  };

} // namespace lon
