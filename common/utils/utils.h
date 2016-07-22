#ifndef _UTILS_H
#define _UTILS_H

#include <sys/types.h>

void *calloc_or_fail(size_t size);
void *malloc_or_fail(size_t size);

#endif
