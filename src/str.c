#include "str.h"

#include <stdlib.h>
#include <string.h>

char* strclone(const char* str) {
  return strnclone(str, strlen(str));
}

char* strnclone(const char* str, int len) {
  char* buf = (char*)malloc(len + 1);
  strncpy(buf, str, len);
  buf[len] = 0;
  return buf;
}
