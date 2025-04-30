#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "generator.h"
#include "ast.h"

int main(void) {
  // TODO input file from args
  FILE* sourceFile = fopen("../lang.lon", "rt");

  fseek(sourceFile, 0, SEEK_END);
  int length = ftell(sourceFile);
  fseek(sourceFile, 0, SEEK_SET);

  char* buffer = (char*)malloc(length + 1);
  fread(buffer, sizeof(char), length, sourceFile);
  buffer[length] = 0;
  fclose(sourceFile);

  LonLexer lexer;
  LonLexer_Init(&lexer, buffer);
  LonLexer_Tokenize(&lexer);

  LonParser parser;
  LonParser_Init(&parser, &lexer);
  LonParser_Parse(&parser);

  FILE* file = fopen("out.asm", "w+");
  LonGenerator gen;
  LonGenerator_Init(&gen, parser.rootStatements, file);
  LonGenerator_Generate(&gen);
  fflush(file);
  fclose(file);

  return 0;
}
