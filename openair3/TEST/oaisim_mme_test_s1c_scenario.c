/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "oaisim_mme_test_s1c_scenario.h"

int        scenario_index         = 0;
int        scenario_message_index = 0;
int        debug                  = 0;
int        error_count            = 0;
int        break_on_error         = 0;


extern s1ap_message_test_t s1ap_scenario1[];

s1ap_message_test_t s1ap_scenarios[][] = {s1ap_scenario1};

/* -1 means invalid */
static const signed char hex_digits[0x100] = {
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

//------------------------------------------------------------------------------
void
fail (const char *format, ...)
//------------------------------------------------------------------------------
{
  char str[1024];
  va_list arg_ptr;

  va_start (arg_ptr, format);
  vsnprintf ( str, sizeof(str), format, arg_ptr);
  va_end (arg_ptr);
  fputs(str, stderr);
  error_count++;

  if (break_on_error)
    exit (1);
}

//------------------------------------------------------------------------------
void
success (const char *format, ...)
//------------------------------------------------------------------------------
{
  char str[1024];
  va_list arg_ptr;

  va_start (arg_ptr, format);
  vsnprintf ( str, sizeof(str), format, arg_ptr);
  va_end (arg_ptr);
  fputs(str, stderr);
}

//------------------------------------------------------------------------------
void
escapeprint (const char *str, size_t len)
//------------------------------------------------------------------------------
{
  size_t i;

  printf (" (length %d bytes):\n\t", (int) len);

  for (i = 0; i < len; i++) {
    if (((str[i] & 0xFF) >= 'A' && (str[i] & 0xFF) <= 'Z') ||
        ((str[i] & 0xFF) >= 'a' && (str[i] & 0xFF) <= 'z') ||
        ((str[i] & 0xFF) >= '0' && (str[i] & 0xFF) <= '9')
        || (str[i] & 0xFF) == ' ' || (str[i] & 0xFF) == '.')
      printf ("%c", (str[i] & 0xFF));
    else
      printf ("\\x%02X", (str[i] & 0xFF));

    if ((i + 1) % 16 == 0 && (i + 1) < len)
      printf ("'\n\t'");
  }

  printf ("\n");
}

//------------------------------------------------------------------------------
void
hexprint (const void *_str, size_t len)
//------------------------------------------------------------------------------
{
  size_t i;
  const char* str = _str;

  printf ("\t;; ");

  for (i = 0; i < len; i++) {
    printf ("%02x ", (str[i] & 0xFF));

    if ((i + 1) % 8 == 0)
      printf (" ");

    if ((i + 1) % 16 == 0 && i + 1 < len)
      printf ("\n\t;; ");
  }

  printf ("\n");
}

//------------------------------------------------------------------------------
void
binprint (const void *_str, size_t len)
//------------------------------------------------------------------------------
{
  size_t i;
  const char* str = _str;

  printf ("\t;; ");

  for (i = 0; i < len; i++) {
    printf ("%d%d%d%d%d%d%d%d ",
            (str[i] & 0xFF) & 0x80 ? 1 : 0,
            (str[i] & 0xFF) & 0x40 ? 1 : 0,
            (str[i] & 0xFF) & 0x20 ? 1 : 0,
            (str[i] & 0xFF) & 0x10 ? 1 : 0,
            (str[i] & 0xFF) & 0x08 ? 1 : 0,
            (str[i] & 0xFF) & 0x04 ? 1 : 0,
            (str[i] & 0xFF) & 0x02 ? 1 : 0, (str[i] & 0xFF) & 0x01 ? 1 : 0);

    if ((i + 1) % 3 == 0)
      printf (" ");

    if ((i + 1) % 6 == 0 && i + 1 < len)
      printf ("\n\t;; ");
  }

  printf ("\n");
}

//------------------------------------------------------------------------------
int
compare_buffer(const uint8_t *buffer, const uint32_t length_buffer,
               const uint8_t *pattern, const uint32_t length_pattern)
//------------------------------------------------------------------------------
{
  int i;

  if (length_buffer != length_pattern) {
    printf("Length mismatch, expecting %d bytes, got %d bytes\n", length_pattern,
           length_buffer);
    hexprint(buffer, length_buffer);
    return -1;
  }

  for (i = 0; i < length_buffer; i++) {
    if (pattern[i] != buffer[i]) {
      printf("Expecting:\n");
      hexprint(pattern, length_pattern);
      printf("Received:\n");
      hexprint(buffer, length_buffer);
      printf("Mismatch fount in byte %d\nExpecting 0x%02x, got 0x%02x\n",
             i, pattern[i], buffer[i]);
      return -1;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
unsigned
decode_hex_length(const char *h)
//------------------------------------------------------------------------------
{
  const unsigned char *hex = (const unsigned char *) h;
  unsigned count;
  unsigned i;

  for (count = i = 0; hex[i]; i++) {
    if (isspace(hex[i]))
      continue;

    if (hex_digits[hex[i]] < 0)
      abort();

    count++;
  }

  if (count % 2)
    abort();

  return count / 2;
}

//------------------------------------------------------------------------------
int
decode_hex(uint8_t *dst, const char *h)
//------------------------------------------------------------------------------
{
  const unsigned char *hex = (const unsigned char *) h;
  unsigned i = 0;

  for (;;) {
    int high, low;

    while (*hex && isspace(*hex))
      hex++;

    if (!*hex)
      return 1;

    high = hex_digits[*hex++];

    if (high < 0)
      return 0;

    while (*hex && isspace(*hex))
      hex++;

    if (!*hex)
      return 0;

    low = hex_digits[*hex++];

    if (low < 0)
      return 0;

    dst[i++] = (high << 4) | low;
  }
}

//------------------------------------------------------------------------------
uint8_t *
decode_hex_dup(const char *hex)
//------------------------------------------------------------------------------
{
  uint8_t *p;
  unsigned length = decode_hex_length(hex);

  p = malloc(length * sizeof(uint8_t));

  if (decode_hex(p, hex))
    return p;
  else {
    free(p);
    return NULL;
  }
}

