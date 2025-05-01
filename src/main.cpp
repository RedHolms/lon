#include <stdio.h>
#include <sstream>
#include <fstream>
#include "lexer.hpp"
#include "parser.hpp"
#include "generator.hpp"

static std::string slurpFile(std::ifstream& in) {
  std::ostringstream stream;
  stream << in.rdbuf();
  return stream.str();
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "no input file\n");
    return 1;
  }

  std::ifstream file;
  try {
    file.exceptions(std::ifstream::failbit);
    file.open(argv[1], std::ios::in);
  }
  catch (std::exception& error) {
    fprintf(stderr, "failed to open input file: %s\n", error.what());
    return 1;
  }

  std::string content = slurpFile(file);
  file.close();

  lon::Lexer lexer;

  try {
    lexer.feed(content.c_str());
  }
  catch (lon::LexerError& error) {
    fprintf(stderr, "Syntax error at %d:%d: %s\n", error.row(), error.column(), error.what());
    return 1;
  }

  lon::Parser parser;
  try {
    parser.feed(lexer.getResult());
  }
  catch (lon::ParserError& error) {
    if (error.causedToken()) {
      auto tk = error.causedToken();
      fprintf(stderr, "Parser error at %d:%d: %s\n", tk->row, tk->column, error.what());
    }
    else {
      fprintf(stderr, "Parser error at EOF: %s\n", error.what());
    }
    return 1;
  }

  parser.debugPrint();

  lon::Generator generator;

  generator.generate(parser.getResult(), fopen("out.asm", "w+"));

  return 0;
}
