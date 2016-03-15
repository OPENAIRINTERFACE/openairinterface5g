#ifndef _T_defs_H_
#define _T_defs_H_

/* maximum size of a message - increase if needed */
#define T_BUFFER_MAX (1024*64)

/* size of the local cache for messages (must be pow(2,something)) */
#define T_CACHE_SIZE (8192 * 2)

typedef struct {
  volatile int busy;
  char buffer[T_BUFFER_MAX];
  int length;
} T_cache_t;

#define T_SHM_FILENAME "/T_shm_segment"

#endif /* _T_defs_H_ */
