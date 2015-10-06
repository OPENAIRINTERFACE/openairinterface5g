/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
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
