/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

   Contact Information
   OpenAirInterface Admin: openair_admin@eurecom.fr
   OpenAirInterface Tech : openair_tech@eurecom.fr
   OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

   Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/
#include <stdlib.h>
#include <stdint.h>

#define MME_TEST_S1_MAX_BUF_LENGTH (1024)
#define MME_TEST_S1_MAX_BYTES_TEST (32)

typedef enum entity_s{
  MME,
  ENB
} entity_t;

typedef struct s1ap_message_test_s{
  char    *procedure_name;
  uint8_t  buffer[MME_TEST_S1_MAX_BUF_LENGTH];
  uint16_t dont_check[MME_TEST_S1_MAX_BYTES_TEST]; // bytes index test that can be omitted
  uint32_t buf_len;
  entity_t originating;
  uint16_t sctp_stream_id;
  uint32_t assoc_id;
} s1ap_message_test_t;

void     fail (const char *format, ...);
void     success (const char *format, ...);
void     escapeprint (const char *str, size_t len);
void     hexprint (const void *_str, size_t len);
void     binprint (const void *_str, size_t len);
int      compare_buffer(const uint8_t *buffer, const uint32_t length_buffer, const uint8_t *pattern, const uint32_t length_pattern);
unsigned decode_hex_length(const char *h);
int      decode_hex(uint8_t *dst, const char *h);
uint8_t *decode_hex_dup(const char *hex);
