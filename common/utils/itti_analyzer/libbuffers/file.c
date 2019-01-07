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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define G_LOG_DOMAIN ("BUFFERS")

#include <glib.h>

#include "rc.h"
#include "buffers.h"
#include "file.h"

#define READ_BUFFER_SIZE 1024

int file_read_dump(buffer_t **buffer, const char *filename)
{
    int fd = -1;
    buffer_t *new_buf = NULL;
    uint8_t   data[READ_BUFFER_SIZE];
    ssize_t   current_read;

    if (!filename)
        return RC_BAD_PARAM;

    if ((fd = open(filename, O_RDONLY)) == -1) {
        g_warning("Cannot open %s for reading, returned %d:%s\n",
                  filename, errno, strerror(errno));
        return RC_FAIL;
    }

    CHECK_FCT(buffer_new_from_data(&new_buf, NULL, 0, 0));

    do {
        current_read = read(fd, data, READ_BUFFER_SIZE);
        if (current_read == -1)
        {
            g_warning("Failed to read data from file, returned %d:%s\n",
                      errno, strerror(errno));
            return RC_FAIL;
        }
        CHECK_FCT(buffer_append_data(new_buf, data, current_read));
    } while(current_read == READ_BUFFER_SIZE);

    *buffer = new_buf;

    buffer_dump(new_buf, stdout);

    close(fd);

    return RC_OK;
}
