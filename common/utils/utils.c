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

/****************************************************************************
 **                                                                        **
 ** Name:        hex_char_to_hex_value()                                   **
 **                                                                        **
 ** Description: Converts an hexadecimal ASCII coded digit into its value. **
 **                                                                        **
 ** Inputs:      c:             A char holding the ASCII coded value       **
 **              Others:        None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **              Return:        Converted value                            **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
uint8_t hex_char_to_hex_value (char c)
{
  if (c >= 'A') {
    /* Remove case bit */
    c &= ~('a' ^ 'A');

    return (c - 'A' + 10);
  } else {
    return (c - '0');
  }
}

/****************************************************************************
 **                                                                        **
 ** Name:        hex_string_to_hex_value()                                 **
 **                                                                        **
 ** Description: Converts an hexadecimal ASCII coded string into its value.**
 **                                                                        **
 ** Inputs:      hex_value:     A pointer to the location to store the     **
 **                             conversion result                          **
 **              size:          The size of hex_value in bytes             **
 **              Others:        None                                       **
 **                                                                        **
 ** Outputs:     hex_value:     Converted value                            **
 **              Return:        None                                       **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
void hex_string_to_hex_value (uint8_t *hex_value, const char *hex_string, int size)
{
  int i;

  for (i=0; i < size; i++) {
    hex_value[i] = (hex_char_to_hex_value(hex_string[2 * i]) << 4) | hex_char_to_hex_value(hex_string[2 * i + 1]);
  }
}
