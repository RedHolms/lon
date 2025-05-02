#pragma once

#include <list>
#include <string>
#include <variant>
#include "literals.hpp"

namespace lon {

  enum class ExpressionType {
    CALL,
    LITERAL
  };

  struct Expression {
    struct Call {
      std::string funcName;
      std::list<Expression> args;
    };

    ExpressionType getType() const noexcept { return (ExpressionType)data.index(); }

    std::variant<Call, Literal> data;
  };

} // namespace lon
