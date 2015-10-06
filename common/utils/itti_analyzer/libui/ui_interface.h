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


#include <glib.h>

#ifndef UI_INTERFACE_H_
#define UI_INTERFACE_H_

/*******************************************************************************
 * Functions used between dissectors and GUI to update signal dissection
 ******************************************************************************/

typedef gboolean (*ui_set_signal_text_cb_t) (gpointer user_data, gchar *text, gint length);

/*******************************************************************************
 * Pipe interface between GUI thread and other thread
 ******************************************************************************/

typedef gboolean (*pipe_input_cb_t) (gint source, gpointer user_data);

typedef struct {
    int             source_fd;
    guint           pipe_input_id;
    GIOChannel     *pipe_channel;

    pipe_input_cb_t input_cb;
    gpointer        user_data;
} pipe_input_t;

int ui_pipe_new(int pipe_fd[2], pipe_input_cb_t input_cb, gpointer user_data);

int ui_pipe_write_message(int pipe_fd, const uint16_t message_type,
                          const void * const message, const uint16_t message_size);

typedef struct {
    uint16_t message_size;
    uint16_t message_type;
} pipe_input_header_t;

enum ui_pipe_messages_id_e {
    /* Other thread -> GUI interface ids */
    UI_PIPE_CONNECTION_FAILED,
    UI_PIPE_CONNECTION_LOST,
    UI_PIPE_XML_DEFINITION,
    UI_PIPE_UPDATE_SIGNAL_LIST,

    /* GUI -> other threads */
    UI_PIPE_DISCONNECT_EVT
};

typedef struct {
    char  *xml_definition;
    size_t xml_definition_length;
} pipe_xml_definition_message_t;

typedef struct {
    GList *signal_list;
} pipe_new_signals_list_message_t;

#endif /* UI_INTERFACE_H_ */
