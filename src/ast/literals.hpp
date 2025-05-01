#pragma once

#include <stdint.h>
#include <string>

namespace lon {

  enum class LiteralType {
    INT = 1,
    FLOAT,
    STRING
  };

  struct IntLiteral;
  struct FloatLiteral;
  struct StringLiteral;

  struct Literal {
    virtual ~Literal() = default;
    LiteralType type;

    constexpr auto asInt() { return (IntLiteral*)this; }
    constexpr auto asFloat() { return (FloatLiteral*)this; }
    constexpr auto asString() { return (StringLiteral*)this; }
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
