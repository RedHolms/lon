#ifndef LON_LEX_H_
#define LON_LEX_H_

#include <stdio.h>
#include "trie.h"

typedef enum LonTokenID {
  // all one-char tokens are just char value
  // because of that, all other tokens start from 256

  // literals
  TK_ID = 256, // variable, function, etc.
  TK_STRING,
  TK_NUMBER,

  // multiple chars
  TK_RET_ARROW,

  // keywords
  TK_FUNCTION,
  TK_RETURN
} LonTokenID;

typedef struct LonToken {
  LonTokenID id;
  int row; // starts from 1
  int column; // starts from 1
  char* string; // must be allocated with malloc or NULL
  struct LonToken* next;
} LonToken;

typedef struct LonLexer {
  const char* input;
  const char* p;
  int row; // starts from 1
  int column; // starts from 1
  trie keywords;
  LonToken* tokens;
  LonToken* tail;
  const char* error; // NULL if no error
} LonLexer;

void LonLexer_Init(LonLexer* lex, const char* input);
void LonLexer_Destroy(LonLexer* lex);
void LonLexer_Tokenize(LonLexer* lex);
void LonLexer_Print(LonLexer* lex, FILE* outFile, int allIndent, int tokensIndent);

#endif // LON_LEX_H_