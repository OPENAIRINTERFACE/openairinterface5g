#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>
#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif

#define sizeofArray(a) (sizeof(a)/sizeof(*(a)))
void *calloc_or_fail(size_t size);
void *malloc_or_fail(size_t size);

// Converts an hexadecimal ASCII coded digit into its value. **
int hex_char_to_hex_value (char c);
// Converts an hexadecimal ASCII coded string into its value.**
int hex_string_to_hex_value (uint8_t *hex_value, const char *hex_string, int size);

void *memcpy1(void *dst,const void *src,size_t n);


char *itoa(int i);

#define findInList(keY, result, list, element_type) {\
    int i;\
    for (i=0; i<sizeof(list)/sizeof(element_type) ; i++)\
      if (list[i].key==keY) {\
        result=list[i].val;\
        break;\
      }\
    AssertFatal(i < sizeof(list)/sizeof(element_type), "List %s doesn't contain %s\n",#list, #keY); \
  }
#ifdef __cplusplus
}
#endif

#endif
