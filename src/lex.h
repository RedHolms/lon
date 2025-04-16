#ifndef LON_LEX_H_
#define LON_LEX_H_

enum {
  // all one-char tokens are just char value
  // because of that, all other tokens start from 256

  // literals
  TK_NAME = 256,
  TK_STRING,
  TK_NUMBER,

  // multiple chars
  TK_RET_ARROW,

  // keywords
  TK_FUNCTION,
  TK_RETURN
};

typedef struct token {
  int tp;
  char* str; // used for literals, must be allocated with malloc or NULL!
  int pos;
  struct token* next;
} token;

void lex_init(void);
void lex_parse(const char* input);
void lex_print(void);
token* lex_result(void);
void lex_clear(void);

#endif // LON_LEX_H_