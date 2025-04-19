#ifndef LON_PARSER_H_
#define LON_PARSER_H_

#include "ast.h"
#include "lexer.h"
#include "trie.h"

typedef struct LonParser {
  trie builtinTypes;
  LonLexer* lexer;
  LonToken* tk;
  LonRootStatement* rootStatements;
  LonRootStatement* tail;
  const char* error; // NULL if no error
} LonParser;

void LonParser_Init(LonParser* parser, LonLexer* lex);
void LonParser_Destroy(LonParser* parser);
void LonParser_Parse(LonParser* parser);
void LonParser_Print(LonParser* parser, FILE* outFile);

#endif // LON_PARSER_H_
