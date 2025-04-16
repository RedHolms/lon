#include "lex.h"
#include "parse.h"
#include "gen.h"

int main(void) {
  const char* source =
    "function main() -> integer {\n"
    "  print(\"Hello, World!\");\n"
    "  return 0;\n"
    "}";

  lex_init();
  lex_parse(source);
  ast_init();
  ast_parse();
  FILE* file = fopen("out.asm", "w+");
  codegen(file);
  fflush(file);
  fclose(file);

  return 0;
}
