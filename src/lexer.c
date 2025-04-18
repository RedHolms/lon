#include "lexer.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "str.h"
#include "trie.h"

static void push_token(LonLexer* lex, LonTokenID id, char* str, int row, int column) {
  LonToken* tk = (LonToken*)malloc(sizeof(LonToken));
  tk->id = id;
  tk->row = row;
  tk->column = column;
  tk->string = str;
  tk->next = NULL;
  if (lex->tail)
    lex->tail->next = tk;
  else
    lex->tokens = tk;
  lex->tail = tk;
}

static inline void next_char(LonLexer* lex) {
  ++lex->p; ++lex->column;
}

static inline void set_lex_error(LonLexer* lex, const char* info) {
  lex->error = info;
}

void LonLexer_Init(LonLexer* lex, const char* input) {
  lex->p = lex->input = input;
  lex->row = lex->column = 1;
  memset(&lex->keywords, 0, sizeof(lex->keywords));
  trie_insert(&lex->keywords, "function", TK_FUNCTION);
  trie_insert(&lex->keywords, "return", TK_RETURN);
  lex->tokens = NULL;
  lex->error = NULL;
}

void LonLexer_Destroy(LonLexer* lex) {
  LonToken* tk = lex->tokens;
  while (tk) {
    if (tk->string)
      free(tk->string);

    LonToken* next = tk->next;
    free(tk);
    tk = next;
  }

  lex->tokens = lex->tail = NULL;
}

void LonLexer_Tokenize(LonLexer* lex) {
  register char cur;
  register int no_next = 0;
  for (; *lex->p; !no_next ? next_char(lex) : (no_next = 0)) {
    cur = *lex->p;

    if (cur == '\n') {
      lex->column = 0; // will be 1
      ++lex->row;
      continue;
    }

    if (isspace(cur))
      continue;

    // begin of a token
    int row = lex->row, column = lex->column;

    switch (cur) {
      case '/':
        if (lex->p[1] == '/') {
          // single line comment
          while (lex->p[1] != '\n')
            next_char(lex);

          continue;
        }
        else if (lex->p[1] == '*') {
          // multi line comment
          while (*lex->p != '*' || lex->p[1] != '/')
            next_char(lex);

          next_char(lex);
          continue;
        }

        goto SINGLE_CHAR_TOKEN;
      case '-':
        if (lex->p[1] == '>') {
          next_char(lex);
          push_token(lex, TK_RET_ARROW, NULL, row, column);
          break;
        }
        // fallthrough
      case '(':
      case ')':
      case '{':
      case '}':
      case ';':
      SINGLE_CHAR_TOKEN:
        push_token(lex, cur, NULL, row, column);
        break;
      default:
        // number
        if (isdigit(cur)) {
          const char* begin = lex->p;

          int length = 0;
          while (isdigit(*lex->p)) ++length, next_char(lex);

          push_token(lex, TK_NUMBER, strnclone(begin, length), row, column);

          no_next = 1;
          break;
        }

        // string
        if (cur == '"') {
          next_char(lex);
          const char* begin = lex->p;

          // FIXME maybe do another way?

          int length = 0;
          while (*lex->p != '"') {
            switch (*lex->p) {
              case '\r':
              case '\n':
                set_lex_error(lex, "Unexpected line break in string");
                return;
              case '\\':
                ++length, next_char(lex);
                break;
            }

            ++length, next_char(lex);
          }

          char* buffer = (char*)malloc(length + 1);

          int j = 0;
          for (int i = 0; i < length; ++i) {
            cur = begin[i];
            if (cur == '\\') {
              cur = begin[++i];
              switch (cur) {
                case 'n': buffer[j++] = '\n'; break;
                case 't': buffer[j++] = '\t'; break;
                default:
                  set_lex_error(lex, "Unknown escape sequence");
                  return;
              }
            }
            else {
              buffer[j++] = cur;
            }
          }

          buffer[j] = 0;
          push_token(lex, TK_STRING, buffer, row, column);
          break;
        }

        // identifier
        if (isalpha(cur) || cur == '_') {
          const char* begin = lex->p;

          int length = 0;
          while (isalnum(*lex->p) || *lex->p == '_')
            ++length, next_char(lex);

          no_next = 1;

          char* buffer = strnclone(begin, length);

          long tp = trie_get(&lex->keywords, buffer);
          if (tp != -1) {
            free(buffer);
            push_token(lex, tp, NULL, row, column);
            break;
          }

          push_token(lex, TK_ID, buffer, row, column);
          break;
        }

        printf("%c\n", cur);
        set_lex_error(lex, "Unknown token");
        return;
    }
  }
}

void LonLexer_Print(LonLexer* lex, FILE* outFile, int allIndent, int tokensIndent) {
  if (allIndent > 0)
    fprintf(outFile, "%*c", allIndent, ' ');

  if (lex->error) {
    fprintf(outFile, "Lexer error! At %d:%d. %s.", lex->row, lex->column, lex->error);
    return;
  }

  fprintf(outFile, "Lexer result:\n");

  LonToken* tk = lex->tokens;
  while (tk) {
    if (tokensIndent > 0)
      fprintf(outFile, "%*c", tokensIndent, ' ');

    fprintf(outFile, "Token ");

    if (tk->id <= 255) {
      fprintf(outFile, "%c", tk->id);
    }
    else {
      switch (tk->id) {
        case TK_ID:        fprintf(outFile, "identifier"); break;
        case TK_STRING:    fprintf(outFile, "string"); break;
        case TK_NUMBER:    fprintf(outFile, "number"); break;
        case TK_RET_ARROW: fprintf(outFile, "->"); break;
        case TK_FUNCTION:  fprintf(outFile, "keyword <function>"); break;
        case TK_RETURN:    fprintf(outFile, "keyword <return>"); break;
        default:           fprintf(outFile, "unknown<%d>", tk->id); break;
      }
    }

    if (tk->string)
      fprintf(outFile, " \"%s\"", tk->string);

    fprintf(outFile, " at %d:%d\n", tk->row, tk->column);
    tk = tk->next;
  }
}
