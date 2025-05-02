#pragma once

#include <string>
#include <variant>

namespace lon {

  enum class LiteralType {
    INT,
    FLOAT,
    STRING
  };

  struct Literal {
    LiteralType getType() const noexcept { return (LiteralType)data.index(); }

    std::variant<uint64_t, double, std::string> data;
  };

} // namespace lon
