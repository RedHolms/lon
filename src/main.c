#include "lex.h"
#include "parse.h"

int main(void) {
  const char* source =
    "function main() -> integer {\n"
    "  print(\"Hello, World!\");\n"
    "  return 0;\n"
    "}";

  lex_init();
  lex_parse(source);
  // ast_parse();

  return 0;
}
