#include <stdio.h>
#include <sstream>
#include <fstream>
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
//#include "compiler/generator.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "no input file\n");
    return 1;
  }

  lon::LexerResult lexerResult;
  try {
    lon::Lexer lexer(argv[1]);
    lexer.tokenize();
    lexerResult = lexer.getResult();
  }
  catch (lon::LexerError& error) {
    fprintf(stderr, "Syntax error at %d:%d: %s\n", error.row(), error.column(), error.what());
    return 1;
  }
  catch (std::exception& error) {
    fprintf(stderr, "%s\n", error.what());
    return 1;
  }

  lon::Parser parser(lexerResult);

  try {
    parser.parse();
  }
  catch (lon::ParserError& error) {
    if (error.row() != -1) {
      fprintf(stderr, "Parser error at %d:%d: %s\n", error.row(), error.column(), error.what());
    }
    else {
      fprintf(stderr, "Parser error at EOF: %s\n", error.what());
    }
    return 1;
  }

  parser.debugPrint();

//  lon::Generator generator;

//  generator.generate(parser.getResult(), fopen("out.asm", "w+"));

  return 0;
}
