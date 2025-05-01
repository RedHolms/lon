#pragma once

#include <stdint.h>
#include <string>
#include <list>
#include "expressions.hpp"
#include "types.hpp"

namespace lon {

  enum class StatementType {
    EXPR = 1,
    BLOCK,
    RETURN
  };

  struct Statement {
    virtual ~Statement() = default;
    StatementType type;
  };

  struct ExpressionStatement : Statement {
    std::shared_ptr<Expression> expr;
  };

  struct BlockStatement : Statement {
    std::list<std::shared_ptr<Statement>> block;
  };

  struct ReturnStatement : Statement {
    std::shared_ptr<Expression> value;
  };

  enum class RootStatementType {
    FUNCTION = 1,
    GLOBAL_VAR
  };

  struct RootStatement {
    virtual ~RootStatement() = default;
    RootStatementType type;
  };

  struct FunctionRootStatement : RootStatement {
    std::string funcName;
    std::list<std::shared_ptr<Type>> argsTypes;
    std::shared_ptr<Type> returnType;
    std::list<std::shared_ptr<Statement>> body;
  };

  struct GlobalVarStatement : RootStatement {
    std::string varName;
    std::shared_ptr<Type> varType;
    // todo inline initialization
  };

} // namespace lon
