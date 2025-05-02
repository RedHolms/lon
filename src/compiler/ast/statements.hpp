#pragma once

#include <stdint.h>
#include <string>
#include <list>
#include <variant>
#include "expressions.hpp"
#include "types.hpp"

namespace lon {

  enum class StatementType {
    EXPR,
    RETURN,
    BLOCK
  };

  struct FunctionDefinition;

  struct Statement {
    struct Return {
      Expression value;
    };

    StatementType getType() const noexcept { return (StatementType)data.index(); }

    std::variant<Expression, Return, std::list<Statement>> data;
  };

  struct FunctionDefinition {
    std::string funcName;
    std::list<Type> argsTypes;
    Type returnType;
    std::list<Statement> body;
  };

} // namespace lon
