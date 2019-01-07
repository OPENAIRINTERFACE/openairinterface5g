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

#include <stdint.h>

#ifndef BUFFERS_H_
#define BUFFERS_H_

typedef struct buffer_s {
    /* The size in bytes as read from socket */
    uint32_t size_bytes;

    /* Current position */
    uint8_t *buffer_current;

    /* The complete data */
    uint8_t *data;

    /* The message number as read from socket */
    uint32_t message_number;

    uint32_t message_id;
} buffer_t;

uint8_t buffer_get_uint8_t(buffer_t *buffer, uint32_t offset);

uint16_t buffer_get_uint16_t(buffer_t *buffer, uint32_t offset);

uint32_t buffer_get_uint32_t(buffer_t *buffer, uint32_t offset);

uint64_t buffer_get_uint64_t(buffer_t *buffer, uint32_t offset);

int buffer_fetch_bits(buffer_t *buffer, uint32_t offset, int nbits, uint32_t *value);

int buffer_fetch_nbytes(buffer_t *buffer, uint32_t offset, int n_bytes, uint8_t *value);

void buffer_dump(buffer_t *buffer, FILE *to);

int buffer_append_data(buffer_t *buffer, const uint8_t *data, const uint32_t length);

int buffer_new_from_data(buffer_t **buffer, uint8_t *data, const uint32_t length,
                         int data_static);

int buffer_has_enouch_data(buffer_t *buffer, uint32_t offset, uint32_t to_get);

void *buffer_at_offset(buffer_t *buffer, uint32_t offset);

#endif /* BUFFERS_H_ */
