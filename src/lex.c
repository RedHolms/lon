#include "lex.h"

#define _CRT_SECURE_NO_WARNINGS // strncpy

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "str.h"
#include "trie.h"

static token* head = NULL;
static token* tail = NULL;
static struct trie keywords = {0};

void pushtk(int tp, char* str) {
  token* elem = (token*)malloc(sizeof(token));
  elem->tp = tp;
  elem->str = str;
  elem->next = NULL;
  if (tail)
    tail->next = elem;
  else
    head = elem;
  tail = elem;
}

void lex_init(void) {
  trie_insert(&keywords, "function", TK_FUNCTION);
  trie_insert(&keywords, "return", TK_RETURN);
}

void lex_parse(const char* input) {
  lex_clear();

  int len = strlen(input);

  register char cur;
  register char next;
  for (int i = 0; i < len; ++i) {
    cur = input[i];

    if (isspace(cur))
      continue;

    switch (cur) {
      case '-':
        next = input[i+1];
        if (next == '>') {
          ++i; pushtk(TK_RET_ARROW, NULL);
          break;
        }
      case '(':
      case ')':
      case '{':
      case '}':
      case ';':
        pushtk(cur, NULL);
        break;
      default:
        if (isdigit(cur)) {
          const char* beg = input + i;

          int numlen = i;
          while (isdigit(input[i])) ++i;
          numlen = i - numlen;
          --i;

          char* buf = strnclone(beg, numlen);
          pushtk(TK_NUMBER, buf);
          break;
        }

        if (cur == '"') {
          const char* beg = input + (++i);

          int strlen = i;
          while (input[i] != '"') {
            if (input[i] == '\\' && input[i+1] == '"')
              ++i;
            ++i;
          }

          strlen = i - strlen;

          char* buf = (char*)malloc(strlen + 1);

          int k = 0;
          for (int j = 0; j < strlen; ++j) {
            cur = beg[j];
            if (cur == '\\') {
              cur = beg[++j];
              switch (cur) {
                case 'n': buf[k++] = '\n'; break;
                case 't': buf[k++] = '\t'; break;
                // todo others
              }
            }
            else {
              buf[k++] = cur;
            }
          }
          buf[k] = 0;

          pushtk(TK_STRING, buf);
          break;
        }

        if (isalpha(cur) || cur == '_') {
          const char* beg = input + i;

          int namelen = i;
          while (isalnum(input[i]) || input[i] == '_')
            ++i;

          namelen = i - namelen;
          --i;

          char* buf = strnclone(beg, namelen);

          long tp = trie_get(&keywords, buf);
          if (tp != -1) {
            free(buf);
            pushtk(tp, NULL);
            break;
          }

          pushtk(TK_NAME, buf);
          break;
        }

        // todo lex error
        break;
    }
  }
}

void lex_print(void) {
  printf("lex result:\n");

  token* it = head;
  while (it) {
    printf(" node:\n");

    if (it->tp <= 255) {
      printf("  tp: char (%c)\n", it->tp);
    }
    else {
      switch (it->tp) {
        case TK_NAME: printf("  tp: TK_NAME\n"); break;
        case TK_STRING: printf("  tp: TK_STRING\n"); break;
        case TK_NUMBER: printf("  tp: TK_NUMBER\n"); break;
        case TK_RET_ARROW: printf("  tp: TK_RET_ARROW\n"); break;
        case TK_FUNCTION: printf("  tp: TK_FUNCTION\n"); break;
        case TK_RETURN: printf("  tp: TK_RETURN\n"); break;
      }
    }

    printf("  str: %s\n", it->str);
    it = it->next;
  }
}

token* lex_result(void) {
  return head;
}

void lex_clear(void) {
  token* it = head;
  while (it) {
    if (it->str)
      free(it->str);

    token* next = it->next;
    free(it);
    it = next;
  }

  head = tail = NULL;
}
