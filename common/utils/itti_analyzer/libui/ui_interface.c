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
#include <stdint.h>
#include <unistd.h>

#define G_LOG_DOMAIN ("UI_INTER")

#include <gtk/gtk.h>

#include "ui_interface.h"

#include "ui_tree_view.h"
#include "ui_notifications.h"
#include "ui_signal_dissect_view.h"

#include "socket.h"
#include "xml_parse.h"

static
gboolean ui_callback_on_pipe_notification(
    GIOChannel *source, GIOCondition condition, gpointer user_data)
{
    pipe_input_t *pipe_input = (pipe_input_t *)user_data;

    /* avoid reentrancy problems and stack overflow */
    g_source_remove(pipe_input->pipe_input_id);

    g_debug("Received new data on pipe %d", pipe_input->pipe_input_id);

    if (pipe_input->input_cb(pipe_input->source_fd, pipe_input->user_data)) {
        /* restore pipe handler */
        pipe_input->pipe_input_id = g_io_add_watch_full(pipe_input->pipe_channel,
                                                        G_PRIORITY_HIGH,
                                                        (GIOCondition)(G_IO_IN|G_IO_ERR|G_IO_HUP),
                                                        ui_callback_on_pipe_notification,
                                                        pipe_input,
                                                        NULL);
    }
    return TRUE;
}

int ui_pipe_new(int pipe_fd[2], pipe_input_cb_t input_cb, gpointer user_data)
{
    static pipe_input_t pipe_input;

    g_assert(pipe_fd != NULL);

    /* Create a pipe between GUI and a thread or a process */
    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, pipe_fd) < 0) {
        g_warning("Failed to create socketpair %s", g_strerror(errno));
        return RC_FAIL;
    }

    /* Source taken from wireshark SVN repository */

    pipe_input.source_fd = pipe_fd[0];
    pipe_input.input_cb  = input_cb;
    pipe_input.user_data = user_data;

    pipe_input.pipe_channel = g_io_channel_unix_new(pipe_fd[0]);
    g_io_channel_set_encoding(pipe_input.pipe_channel, NULL, NULL);
    pipe_input.pipe_input_id = g_io_add_watch_full(pipe_input.pipe_channel,
                                                   G_PRIORITY_HIGH,
                                                   G_IO_IN | G_IO_ERR | G_IO_HUP,
                                                   ui_callback_on_pipe_notification,
                                                   &pipe_input,
                                                   NULL);

    return RC_OK;
}

int ui_pipe_write_message(int pipe_fd, const uint16_t message_type,
                          const void * const message, const uint16_t message_size)
{
    int ret;
    pipe_input_header_t pipe_input_header;

    pipe_input_header.message_size = message_size + sizeof(pipe_input_header);
    pipe_input_header.message_type = message_type;

    if (message_size > 0 && message == NULL) {
        g_error("message size = %u but message is NULL", message_size);
        g_assert_not_reached();
    }

    ret = write(pipe_fd, &pipe_input_header, sizeof(pipe_input_header));
    if (ret < 0) {
        g_warning("Failed to write header to pipe: %s", g_strerror(errno));
        return ret;
    }

    /* Only write the message to pipe if valid data to transmit */
    if (message_size > 0) {
        ret = write(pipe_fd, message, message_size);
        if (ret < 0) {
            g_warning("Failed to write message of size %u at 0x%p to pipe: %s",
                    message_size, message, g_strerror(errno));
            return ret;
        }
    }
    return 0;
}
