#pragma once

#include <string>
#include <fstream>
#include <sstream>

namespace lon {

  std::string slurpFile(std::ifstream& in) {
    std::ostringstream stream;
    stream << in.rdbuf();
    return stream.str();
  }

}
