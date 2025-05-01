#pragma once

#include <stdint.h>
#include <string>

namespace lon {

  enum class LiteralType {
    INT = 1,
    FLOAT,
    STRING
  };

  struct Literal {
    virtual ~Literal() = default;
    LiteralType type;
  };

  struct IntLiteral : Literal {
    uint64_t value;
  };

  struct FloatLiteral : Literal {
    double value;
  };

  struct StringLiteral : Literal {
    std::string value;
  };

} // namespace lon
