#ifndef _UTILS_H
#define _UTILS_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <common/utils/assertions.h>

#ifdef MALLOC_TRACE
#define malloc myMalloc
#endif

#define sizeofArray(a) (sizeof(a)/sizeof(*(a)))

#define cmax(a,b)  ((a>b) ? (a) : (b))
#define cmax3(a,b,c) ((cmax(a,b)>c) ? (cmax(a,b)) : (c))
#define cmin(a,b)  ((a<b) ? (a) : (b))

#ifdef __cplusplus
#ifdef min
#undef min
#undef max
#endif
#else
#define max(a,b) cmax(a,b)
#define min(a,b) cmin(a,b)
#endif

#ifndef malloc16
#  ifdef __AVX2__
#    define malloc16(x) memalign(32,x+32)
#  else
#    define malloc16(x) memalign(16,x+16)
#  endif
#endif
#define free16(y,x) free(y)
#define bigmalloc malloc
#define bigmalloc16 malloc16
#define openair_free(y,x) free((y))
#define PAGE_SIZE 4096

#define free_and_zero(PtR) do {     \
    if (PtR) {           \
      free(PtR);         \
      PtR = NULL;        \
    }                    \
  } while (0)

static inline void *malloc16_clear( size_t size ) {
#ifdef __AVX2__
  void *ptr = memalign(32, size+32);
#else
  void *ptr = memalign(16, size+16);
#endif
  DevAssert(ptr);
  memset( ptr, 0, size );
  return ptr;
}


static inline void *calloc_or_fail(size_t size) {
  void *ptr = calloc(1, size);

  if (ptr == NULL) {
    fprintf(stderr, "[UE] Failed to calloc %zu bytes", size);
    exit(EXIT_FAILURE);
  }

  return ptr;
}

static inline void *malloc_or_fail(size_t size) {
  void *ptr = malloc(size);

  if (ptr == NULL) {
    fprintf(stderr, "[UE] Failed to malloc %zu bytes", size);
    exit(EXIT_FAILURE);
  }

  return ptr;
}

#if !defined (msg)
# define msg(aRGS...) LOG_D(PHY, ##aRGS)
#endif
#ifndef malloc16
#  ifdef __AVX2__
#    define malloc16(x) memalign(32,x)
#  else
#    define malloc16(x) memalign(16,x)
#  endif
#endif

#define free16(y,x) free(y)
#define bigmalloc malloc
#define bigmalloc16 malloc16
#define openair_free(y,x) free((y))
#define PAGE_SIZE 4096

#define PAGE_MASK 0xfffff000
#define virt_to_phys(x) (x)

const char *hexdump(const void *data, size_t data_len, char *out, size_t out_len);

// Converts an hexadecimal ASCII coded digit into its value. **
int hex_char_to_hex_value (char c);
// Converts an hexadecimal ASCII coded string into its value.**
int hex_string_to_hex_value (uint8_t *hex_value, const char *hex_string, int size);

void *memcpy1(void *dst,const void *src,size_t n);

void set_priority(int priority);

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
