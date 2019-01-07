#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>
#include <sys/types.h>

void *calloc_or_fail(size_t size);
void *malloc_or_fail(size_t size);

// Converts an hexadecimal ASCII coded digit into its value. **
int hex_char_to_hex_value (char c);
// Converts an hexadecimal ASCII coded string into its value.**
int hex_string_to_hex_value (uint8_t *hex_value, const char *hex_string, int size);

char *itoa(int i);

#endif
