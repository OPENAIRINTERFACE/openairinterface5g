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
