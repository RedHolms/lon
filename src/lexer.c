#include "lexer.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "str.h"
#include "trie.h"

static void push_token(LonLexer* lex, LonTokenID id, char* str) {
  LonToken* tk = (LonToken*)malloc(sizeof(LonToken));
  tk->id = id;
  tk->str = str;
  tk->next = NULL;
  if (lex->tail)
    lex->tail->next = tk;
  else
    lex->tokens = tk;
  lex->tail = tk;
}

void LonLexer_Init(LonLexer* lex, const char* input) {
  lex->p = lex->input = input;
  memset(&lex->keywords, 0, sizeof(lex->keywords));
  trie_insert(&lex->keywords, "function", TK_FUNCTION);
  trie_insert(&lex->keywords, "return", TK_RETURN);
  lex->tokens = NULL;
}

void LonLexer_Destroy(LonLexer* lex) {
  LonToken* tk = lex->tokens;
  while (tk) {
    if (tk->str)
      free(tk->str);

    LonToken* next = tk->next;
    free(tk);
    tk = next;
  }

  lex->tokens = lex->tail = NULL;
}

void LonLexer_Parse(LonLexer* lex) {
  register char cur;
  for (; lex->p; ++lex->p) {
    cur = *lex->p;

    if (isspace(cur))
      continue;

    switch (cur) {
      case '-':
        if (lex->p[1] == '>') {
          ++lex->p;
          push_token(lex, TK_RET_ARROW, NULL);
          break;
        }
      case '(':
      case ')':
      case '{':
      case '}':
      case ';':
        push_token(lex, cur, NULL);
        break;
      default:
        // number
        if (isdigit(cur)) {
          const char* begin = lex->p;

          int length = 0;
          while (isdigit(*lex->p)) ++length, ++lex->p;
          --lex->p;

          push_token(lex, TK_NUMBER, strnclone(begin, length));
          break;
        }

        // string
        if (cur == '"') {
          const char* begin = ++lex->p;

          int length = 0;
          while (*lex->p != '"') {
            if (*lex->p == '\\' && lex->p[1] == '"')
              ++length, ++lex->p;
            ++length, ++lex->p;
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
                  // todo others
              }
            }
            else {
              buffer[j++] = cur;
            }
          }
          buffer[j] = 0;

          push_token(lex, TK_STRING, buffer);
          break;
        }

        // identifier
        if (isalpha(cur) || cur == '_') {
          const char* begin = lex->p;

          int length = 0;
          while (isalnum(*lex->p) || *lex->p == '_')
            ++length, ++lex->p;

          --lex->p;

          char* buffer = strnclone(begin, length);

          long tp = trie_get(&lex->keywords, buffer);
          if (tp != -1) {
            free(buffer);
            push_token(lex, tp, NULL);
            break;
          }

          push_token(lex, TK_ID, buffer);
          break;
        }

        // todo lex error
        break;
    }
  }
}

void LonLexer_Print(LonLexer* lex, FILE* outFile, int startIndent, int indent) {
  int currentIndent = startIndent;

  fprintf(outFile, "%*cLexer result:\n", currentIndent, ' ');
  currentIndent += indent;

  LonToken* tk = lex->tokens;
  while (tk) {
    fprintf(outFile, "%*cToken ", currentIndent, ' ');

    if (tk->id <= 255) {
      fprintf(outFile, "<%c>", tk->id);
    }
    else {
      switch (tk->id) {
        case TK_ID:        fprintf(outFile, "identifier"); break;
        case TK_STRING:    fprintf(outFile, "string"); break;
        case TK_NUMBER:    fprintf(outFile, "number"); break;
        case TK_RET_ARROW: fprintf(outFile, "return arrow"); break;
        case TK_FUNCTION:  fprintf(outFile, "function"); break;
        case TK_RETURN:    fprintf(outFile, "return"); break;
        default:           fprintf(outFile, "unknown<%d>", tk->id); break;
      }
    }

    if (tk->str)
      fprintf(outFile, " \"%s\"", tk->str);

    fprintf(outFile, "\n");
    tk = tk->next;
  }
}
