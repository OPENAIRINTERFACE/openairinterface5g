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

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define G_LOG_DOMAIN ("BUFFERS")

#include <glib.h>

#include "rc.h"
#include "buffers.h"

extern int debug_buffers;

static
int buffer_fetch(buffer_t *buffer, uint32_t offset, int size, void *value);

/* Try to fetch 8 bits unsigned from the buffer */
uint8_t buffer_get_uint8_t(buffer_t *buffer, uint32_t offset)
{
    uint8_t value;

    buffer_fetch(buffer, offset, 1, &value);

    return value;
}

/* Try to fetch 16 bits unsigned from the buffer */
uint16_t buffer_get_uint16_t(buffer_t *buffer, uint32_t offset)
{
    uint16_t value;

    buffer_fetch(buffer, offset, 2, &value);

    return value;
}

/* Try to fetch 32 bits unsigned from the buffer */
uint32_t buffer_get_uint32_t(buffer_t *buffer, uint32_t offset)
{
    uint32_t value;

    buffer_fetch(buffer, offset, 4, &value);

    return value;
}

/* Try to fetch 64 bits unsigned from the buffer */
uint64_t buffer_get_uint64_t(buffer_t *buffer, uint32_t offset)
{
    uint64_t value;

    buffer_fetch(buffer, offset, 8, &value);

    return value;
}

static
int buffer_fetch(buffer_t *buffer, uint32_t offset, int size, void *value)
{
    if (buffer == NULL || value == NULL)
        return -1;
    if (size <= 0)
        return -1;

    if (buffer->size_bytes < ((offset >> 3) + size)) {
        g_warning("Not enough data to fetch");
        return -1;
    }

    memcpy(value, &buffer->data[offset >> 3], size);
    buffer->buffer_current = &buffer->data[(offset >> 3) + size];

    return 0;
}

int buffer_fetch_nbytes(buffer_t *buffer, uint32_t offset, int n_bytes, uint8_t *value)
{
    if (buffer->size_bytes < ((offset >> 3) + n_bytes)) {
        g_warning("Not enough data to fetch");
        return -1;
    }
    memcpy(&value[0], &buffer->data[offset >> 3], n_bytes);

    return 0;
}

int buffer_fetch_bits(buffer_t *buffer, uint32_t offset, int nbits, uint32_t *value)
{
    uint32_t temp = 0;
    int i;

    if (buffer == NULL || value == NULL)
        return RC_BAD_PARAM;

    /* We cannot fetch more than 32 bits */
    if (nbits > 32)
        return RC_BAD_PARAM;

    for (i = 0; i < nbits; i++)
    {
        temp |= ((buffer->data[(offset + i) / 8] >> ((offset + i) % 8)) & 1) << i;
    }

    *value = temp;

    return RC_OK;
}

/**
 * @brief Create a new buffer from data
 * @param buffer caller reference where created buffer will be stored
 * @param data Data to attach to the new buffer
 * @param length Length of data buffer
 * @param data_static flag that indicates if data pointer has been statically (= 0) or not (!= 0)
 */
int buffer_new_from_data(buffer_t **buffer, uint8_t *data, const uint32_t length,
                         int data_static)
{
    buffer_t *new;

    if (!buffer)
        return RC_BAD_PARAM;

    new = malloc(sizeof(buffer_t));

    new->size_bytes = length;
    if (data && length > 0) {
        if (data_static == 0) {
            new->data = malloc(sizeof(uint8_t) * new->size_bytes);
            memcpy(new->data, data, new->size_bytes);
        } else {
            new->data = data;
        }
        new->buffer_current = &new->data[0];
    } else {
        new->buffer_current = NULL;
    }

    *buffer = new;

    return 0;
}

#define INDENTED(fILE, x, y)            \
do {                                    \
    int indentation = x;                \
    while(indentation--) fprintf(fILE, " ");  \
    y;                                  \
} while(0)

#define INDENT_BREAK    20

void buffer_dump(buffer_t *buffer, FILE *to)
{
    FILE *file = to;
    uint32_t i;

    if (!buffer)
        return;
    if (!to)
        to = stdout;

    fprintf(file, "<Buffer>\n");
    INDENTED(file, 4, fprintf(file, "<Length>%u<Length>\n", buffer->size_bytes));
    INDENTED(file, 4, fprintf(file, "<Bytes>\n"));
    for (i = 0; i < buffer->size_bytes; i++)
    {
        if ((i % INDENT_BREAK) == 0)
            fprintf(file, "        ");
        fprintf(file, "0x%02x ", buffer->data[i]);
        if ((i % INDENT_BREAK) == (INDENT_BREAK - 1))
            fprintf(file, "\n");
    }
    if ((i % INDENT_BREAK) != (INDENT_BREAK - 1))
        fprintf(file, "\n");
    INDENTED(file, 4, fprintf(file, "</Bytes>\n"));
    fprintf(file, "</Buffer>\n");
}

int buffer_append_data(buffer_t *buffer, const uint8_t *data, const uint32_t length)
{
    if (!buffer)
        return -1;

    if (data && length > 0) {
        if (!buffer->data) {
            buffer->size_bytes = length;
            buffer->data = malloc(sizeof(uint8_t) * buffer->size_bytes);
            memcpy(buffer->data, data, buffer->size_bytes);
        } else {
            buffer->data = realloc(buffer->data, sizeof(uint8_t) * (buffer->size_bytes + length));
            memcpy(&buffer->data[buffer->size_bytes], data, length);
            buffer->size_bytes += length;
        }
        buffer->buffer_current = &buffer->data[0];
    }

    return 0;
}

int buffer_has_enouch_data(buffer_t *buffer, uint32_t offset, uint32_t to_get)
{
    int underflow;
    if (!buffer)
        return -1;
    underflow = (buffer->size_bytes >= ((offset + to_get) / 8)) ? 0 : -1;
    if (underflow && debug_buffers)
        g_warning("Detected Underflow offset %u, to_get %u, buffer size %u\n",
                  offset, to_get, buffer->size_bytes);
    return underflow;
}

void *buffer_at_offset(buffer_t *buffer, uint32_t offset)
{
//     if (buffer_has_enouch_data(buffer, 0, offset) != 0) {
//         return NULL;
//     }
    return &buffer->data[offset / 8];
}
