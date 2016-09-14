#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

#include "conversions.h"

static const char hex_to_ascii_table[16] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

static const signed char ascii_to_hex_table[0x100] = {
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
  -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

void hexa_to_ascii(uint8_t *from, char *to, size_t length)
{
  int i;

  for(i = 0; i < length; i++) {
    uint8_t upper = (from[i] & 0xf0) >> 4;
    uint8_t lower =  from[i] & 0x0f;
    to[2 * i] = hex_to_ascii_table[upper];
    to[2 * i + 1] = hex_to_ascii_table[lower];
  }
}

int ascii_to_hex(uint8_t *dst, const char *h)
{
  const unsigned char *hex = (const unsigned char *) h;
  unsigned i = 0;

  for (;;) {
    int high, low;

    while (*hex && isspace(*hex))
      hex++;

    if (!*hex)
      return 1;

    high = ascii_to_hex_table[*hex++];

    if (high < 0)
      return 0;

    while (*hex && isspace(*hex))
      hex++;

    if (!*hex)
      return 0;

    low = ascii_to_hex_table[*hex++];

    if (low < 0)
      return 0;

    dst[i++] = (high << 4) | low;
  }
}

