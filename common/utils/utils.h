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
#define CHECK_INDEX(ARRAY, INDEX) assert((INDEX) < sizeofArray(ARRAY))

// Prevent double evaluation in max macro
#define cmax(a,b) ({ __typeof__ (a) _a = (a); \
                     __typeof__ (b) _b = (b); \
                     _a > _b ? _a : _b; })


#define cmax3(a,b,c) ( cmax(cmax(a,b), c) )  

// Prevent double evaluation in min macro
#define cmin(a,b) ({ __typeof__ (a) _a = (a); \
                     __typeof__ (b) _b = (b); \
                     _a < _b ? _a : _b; })




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
#    define malloc16(x) memalign(32,x+32)
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
  void *ptr = memalign(32, size+32);
  DevAssert(ptr);
  memset( ptr, 0, size );
  return ptr;
}

static inline void *calloc_or_fail(size_t nmemb, size_t size)
{
  void *ptr = calloc(nmemb, size);

  if (ptr == NULL) {
    fprintf(stderr, "Failed to calloc() %zu elements of %zu bytes: out of memory", nmemb, size);
    exit(EXIT_FAILURE);
  }

  return ptr;
}

static inline void *malloc_or_fail(size_t size)
{
  void *ptr = malloc(size);

  if (ptr == NULL) {
    fprintf(stderr, "Failed to malloc() %zu bytes: out of memory", size);
    exit(EXIT_FAILURE);
  }

  return ptr;
}

#if !defined (msg)
# define msg(aRGS...) LOG_D(PHY, ##aRGS)
#endif
#ifndef malloc16
#    define malloc16(x) memalign(32,x)
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

void set_priority(int priority);

char *itoa(int i);

#define STRINGIFY(S) #S
#define TO_STRING(S) STRINGIFY(S)
int read_version(const char *version, uint8_t *major, uint8_t *minor, uint8_t *patch);

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
