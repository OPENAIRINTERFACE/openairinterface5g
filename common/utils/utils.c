#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "utils.h"

void *calloc_or_fail(size_t size) {
  void *ptr = calloc(1, size);
  if (ptr == NULL) {
    fprintf(stderr, "[UE] Failed to calloc %zu bytes", size);
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *malloc_or_fail(size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    fprintf(stderr, "[UE] Failed to malloc %zu bytes", size);
    exit(EXIT_FAILURE);
  }
  return ptr;
}
