#pragma once

#include "types.hpp"
#include "literals.hpp"
#include "expressions.hpp"
#include "statements.hpp"

namespace lon {

  struct AbstractSourceTree {
    std::string fileName;
    std::list<RootStatement> rootStatements;
  };

}
