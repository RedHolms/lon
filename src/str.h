#ifndef LON_STR_H_
#define LON_STR_H_

// Means that this string was allocated with malloc
//  and should be freed
typedef char* OwnedStr;

char* strclone(const char* str);
char* strnclone(const char* str, int len);

#endif // LON_STR_H_
