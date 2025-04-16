#ifndef LON_TRIE_H_
#define LON_TRIE_H_

typedef struct trie {
  struct trie** children;
  long value;
} trie;

void trie_insert(trie* t, const char* str, long value);
long trie_get(trie* t, const char* str);

#endif // LON_TRIE_H_
