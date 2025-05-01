#pragma once

#include <stdint.h>
#include <list>
#include <string>
#include <memory>
#include "literals.hpp"

namespace lon {

  enum class ExpressionType {
    CALL = 1,
    LITERAL
  };

  struct Expression {
    virtual ~Expression() = default;
    ExpressionType type;
  };

  struct CallExpression : Expression {
    std::string funcName;
    std::list<std::shared_ptr<Expression>> args;
  };

  struct LiteralExpression : Expression {
    std::shared_ptr<Literal> value;
  };

} // namespace lon
