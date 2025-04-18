#ifndef LON_PARSER_H_
#define LON_PARSER_H_

#include "ast.h"

typedef struct LonParser {
  trie builtinTypes;
  LonLexer* lexer;
  LonRootStatement* rootStatements;
  LonRootStatement* tail;

  int haveError;
  const char* errorInfo;
} LonParser;

void LonParser_Init(LonParser* psr, LonLexer* lex);
void LonParser_Destroy(LonParser* psr);
void LonParser_Parse(LonParser* psr);
void LonParser_Print(LonParser* psr, FILE* outFile, int allIndent, int tokensIndent);

#endif // LON_PARSER_H_
