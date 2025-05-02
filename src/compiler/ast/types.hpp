#pragma once

#include <memory>
#include <variant>

namespace lon {

  enum {
    TPF_NONE = 0,
    TPF_CONST = 1 << 0
  };

  using TypeFlags = uint32_t;

  enum {
    TID_VOID,
    TID_NUMBER,
    TID_STRING,
    TID_POINTER,
    TID_REFERENCE,
    TID_CHAR,

    TID_USER_START = 0xFFFF,
    // user defined types
  };

  using TypeID = uint32_t;

  struct Type {
    struct Empty {};

    struct Number {
      uint8_t width; // width in bytes = 1 << width
      bool isSigned;
    };

    TypeID id = TID_VOID;
    TypeFlags flags = 0;
    std::variant<Empty, Number, std::unique_ptr<Type>> data;
  };

} // namespace lon
