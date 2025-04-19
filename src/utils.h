#ifndef LON_UTILS_H_
#define LON_UTILS_H_

#include <stdlib.h>
#include <string.h>

static inline void* malloc_zero(size_t size) {
  void* ptr = malloc(size);
  memset(ptr, 0, size);
  return ptr;
}

#define AllocStruct(st) (st*)malloc_zero(sizeof(st))

#define LinkedList_Append(head, tail, elem) do { elem->next = NULL; if (tail) { tail = tail->next = elem; } else head = tail = elem; } while(0)

#endif // LON_UTILS_H_
