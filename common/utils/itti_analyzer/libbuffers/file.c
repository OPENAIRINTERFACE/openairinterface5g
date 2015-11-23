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
