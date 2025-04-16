#include "trie.h"

#include <stdlib.h>
#include <string.h>

void trie_insert(trie* t, const char* str, long value) {
  if (t->children == NULL) {
    t->children = malloc(sizeof(trie) * 128);
    memset(t->children, 0, sizeof(trie) * 128);
  }

  while ((*str) != '\0') {
    if (t->children[*str] == NULL) {
      t->children[*str] = malloc(sizeof(trie));
      memset(t->children[*str], 0, sizeof(trie));
      t->children[*str]->children = malloc(sizeof(trie) * 128);
      memset(t->children[*str]->children, 0, sizeof(trie) * 128);
    }

    t = t->children[*str];
    t->value = -1;
    ++str;
  }

  t->value = value;
}

long trie_get(trie* t, const char* str) {
  while ((*str) != '\0') {
    t = t->children[*str];
    if (t == NULL) return -1;
    ++str;
  }

  return t->value;
}
