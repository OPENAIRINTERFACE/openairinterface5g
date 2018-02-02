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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define G_LOG_DOMAIN ("BUFFERS")

#include <gtk/gtk.h>

#include "logs.h"
#include "itti_types.h"
#include "rc.h"

#include "ui_interface.h"
#include "ui_notifications.h"
#include "ui_notif_dlg.h"

#include "socket.h"
#include "buffers.h"

#include "xml_parse.h"

/* Retry connection after 100 ms */
#define SOCKET_US_BEFORE_CONNECT_RETRY      (100 * 1000)
/* About 10 minutes time-out for connecting to peer */
#define SOCKET_NB_CONNECT_RETRY             ((10 * 60 * 1000 * 1000) / SOCKET_US_BEFORE_CONNECT_RETRY)

#define SOCKET_NB_SIGNALS_BEFORE_SIGNALLING 10
#define SOCKET_MS_BEFORE_SIGNALLING         100

gboolean socket_abort_connection = FALSE;

void *socket_thread_fct(void *arg);

static ssize_t socket_read_data(socket_data_t *socket_data, void *buffer, size_t size, int flags)
{
    ssize_t recv_ret;

    recv_ret = recv(socket_data->sd, buffer, size, flags);
    if (recv_ret == -1) {
        /* Failure case */
        switch (errno) {
//             case EWOULDBLOCK:
            case EAGAIN:
                return -1;
            default:
                g_info("recv failed: %s", g_strerror(errno));
                pthread_exit(NULL);
                break;
        }
    } else if (recv_ret == 0) {
        /* We lost the connection with other peer or shutdown asked */
        ui_pipe_write_message(socket_data->pipe_fd,
                              UI_PIPE_CONNECTION_LOST, NULL, 0);
        free(socket_data->ip_address);
        free(socket_data);
        pthread_exit(NULL);
    }

    return recv_ret;
}

static void socket_notify_gui_update(socket_data_t *socket_data)
{
    pipe_new_signals_list_message_t pipe_signal_list_message;

    pipe_signal_list_message.signal_list = socket_data->signal_list;

    socket_data->signal_list                  = NULL;
    socket_data->nb_signals_since_last_update = 0;

    /* Send an update notification */
    ui_pipe_write_message(socket_data->pipe_fd,
                          UI_PIPE_UPDATE_SIGNAL_LIST, &pipe_signal_list_message,
                          sizeof(pipe_signal_list_message));

    /* Acquire the last data notification */
    socket_data->last_data_notification = g_get_monotonic_time();
}

static int socket_read_itti_message(socket_data_t        *socket_data,
                                    itti_socket_header_t *message_header)
{
    itti_signal_header_t  itti_signal_header;
    buffer_t             *buffer;
    uint8_t              *data;
    size_t                data_length;
    ssize_t               data_read = 0;
    ssize_t               total_data_read = 0;

    g_assert(message_header != NULL);

    g_debug("Attempting to read signal header from socket");

    /* Read the sub-header of signal */
    while (data_read != sizeof(itti_signal_header_t)) {
        data_read = socket_read_data(socket_data, &itti_signal_header,
                                       sizeof(itti_signal_header_t), 0);
    }

    data_length = message_header->message_size - sizeof(itti_socket_header_t) - sizeof(itti_signal_header_t);
    data = malloc(sizeof(uint8_t) * data_length);

    while (total_data_read < data_length) {
        data_read = socket_read_data(socket_data, &data[total_data_read],
                                     data_length - total_data_read, 0);
        /* We are waiting for data */
        if (data_read < 0) {
            usleep(10);
        } else {
            total_data_read += data_read;
        }
    }

    /* Create the new buffer */
    if (buffer_new_from_data(&buffer, data, data_length - sizeof(itti_message_types_t), 1) != RC_OK) {
        g_error("Failed to create new buffer");
        g_assert_not_reached();
    }

    sscanf (itti_signal_header.message_number_char, MESSAGE_NUMBER_CHAR_FORMAT, &buffer->message_number);
//     buffer_dump(buffer, stdout);

    /* Update the number of signals received since last GUI update */
    socket_data->nb_signals_since_last_update++;

    socket_data->signal_list = g_list_append(socket_data->signal_list, (gpointer)buffer);

    if (socket_data->nb_signals_since_last_update >= SOCKET_NB_SIGNALS_BEFORE_SIGNALLING) {
        socket_notify_gui_update(socket_data);
    }

    g_debug("Successfully read new signal %u from socket", buffer->message_number);

    return total_data_read + sizeof(itti_signal_header);
}

static int socket_read_xml_definition(socket_data_t *socket_data,
                                      itti_socket_header_t *message_header)
{
    ssize_t                        data_read;
    ssize_t                        total_data_read = 0;
    char                          *xml_definition;
    size_t                         xml_definition_length;
    pipe_xml_definition_message_t  pipe_xml_definition_message;

    xml_definition_length = message_header->message_size - sizeof(*message_header);
    xml_definition        = malloc(xml_definition_length * sizeof(char));

    g_debug("Attempting to read XML definition of size %zu from socket",
            xml_definition_length);

    /* XML definition is a long message... so function may take some time */

    do {
        data_read = socket_read_data(socket_data, &xml_definition[total_data_read],
                                     xml_definition_length - total_data_read, 0);

        /* We are waiting for data */
        if (data_read < 0) {
            usleep(10);
        } else {
            total_data_read += data_read;
        }
    } while (total_data_read != xml_definition_length);

    pipe_xml_definition_message.xml_definition        = xml_definition;
    pipe_xml_definition_message.xml_definition_length = xml_definition_length - sizeof(itti_message_types_t);

    g_debug("Received XML definition of size %zu, effectively read %zu bytes",
            xml_definition_length, total_data_read);

    ui_pipe_write_message(socket_data->pipe_fd, UI_PIPE_XML_DEFINITION,
                          &pipe_xml_definition_message, sizeof(pipe_xml_definition_message));

    return total_data_read;
}

static int socket_read(socket_data_t *socket_data)
{
    int ret = 0;
    itti_socket_header_t message_header;

    while (ret >= 0) {
        ret = socket_read_data(socket_data, &message_header, sizeof(message_header), 0);

        if (ret == -1) {
            return 0;
        }

        switch(message_header.message_type) {
            case ITTI_DUMP_XML_DEFINITION:
                socket_read_xml_definition(socket_data, &message_header);
                break;

            case ITTI_DUMP_MESSAGE_TYPE:
                socket_read_itti_message(socket_data, &message_header);
                break;

            case ITTI_STATISTIC_MESSAGE_TYPE:
            default:
                g_warning("Received unknow (or not implemented) message from socket type: %d",
                          message_header.message_type);
                break;
        }
    }

    return 0;
}

static int socket_handle_disconnect_evt(socket_data_t *socket_data)
{
    /* Send shutdown to remote host */
    CHECK_FCT_POSIX(shutdown(socket_data->sd, SHUT_RDWR));
    /* Close file descriptor */
    CHECK_FCT_POSIX(close(socket_data->sd));

    socket_data->sd = -1;

    /* Close pipe */
    close(socket_data->pipe_fd);

    /* Leaving the thread */
    pthread_exit(NULL);

    return 0;
}

static int pipe_read_message(socket_data_t *socket_data)
{
    pipe_input_header_t  input_header;
    uint8_t             *input_data = NULL;
    size_t               input_data_length = 0;

    /* Read the header */
    if (read(socket_data->pipe_fd, &input_header, sizeof(input_header)) < 0) {
        g_warning("Failed to read from pipe %d: %s", socket_data->pipe_fd,
                  g_strerror(errno));
        return -1;
    }

    input_data_length = input_header.message_size - sizeof(input_header);

    /* Checking for non-header part */
    if (input_data_length > 0) {
        input_data = malloc(sizeof(uint8_t) * input_data_length);

        if (read(socket_data->pipe_fd, input_data, input_data_length) < 0) {
            g_warning("Failed to read from pipe %d: %s", socket_data->pipe_fd,
                      g_strerror(errno));
            return -1;
        }
    }

    switch (input_header.message_type) {
        case UI_PIPE_DISCONNECT_EVT:
            return socket_handle_disconnect_evt(socket_data);
        default:
            g_warning("[socket] Unhandled message type %u", input_header.message_type);
            g_assert_not_reached();
    }
    return 0;
}

void *socket_thread_fct(void *arg)
{
    int                 ret;
    struct sockaddr_in  si_me;
    socket_data_t      *socket_data;
    int                 retry = SOCKET_NB_CONNECT_RETRY;

    /* master file descriptor list */
    fd_set              master_fds;
    /* temp file descriptor list for select() */
    fd_set              read_fds;
    int                 fd_max = 0;
    struct timeval      tv;

    socket_data = (socket_data_t *)arg;

    g_assert(socket_data != NULL);

    /* Preparing the socket */
    if ((socket_data->sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        g_warning("socket failed: %s", g_strerror(errno));
        free(socket_data->ip_address);
        free(socket_data);
        pthread_exit(NULL);
    }
    memset((void *)&si_me, 0, sizeof(si_me));

    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(socket_data->port);
    if (inet_aton(socket_data->ip_address, &si_me.sin_addr) == 0) {
        g_warning("inet_aton() failed\n");
        free(socket_data->ip_address);
        free(socket_data);
        pthread_exit(NULL);
    }

    /* clear the master and temp sets */
    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);

    /* Add the GUI pipe to the list of sockets to monitor */
    FD_SET(socket_data->pipe_fd, &master_fds);

    /* Add the client socket to the list of sockets to monitor */
    FD_SET(socket_data->sd, &master_fds);

    /* Update the fd_max with the MAX of socket/pipe */
    fd_max = MAX(socket_data->pipe_fd, socket_data->sd);

    /* Setup the timeout for select.
     * When a timeout is caught, check for new notifications to send to GUI.
     */
    tv.tv_sec = 0;
    tv.tv_usec = 1000 * SOCKET_MS_BEFORE_SIGNALLING;

    do {
        /* Connecting to remote peer */
        ret = connect(socket_data->sd, (struct sockaddr *) &si_me, sizeof(struct sockaddr_in));
        if (ret < 0) {
            if ((socket_abort_connection) || (retry < 0)) {
                if (retry < 0) {
                    g_warning("Failed to connect to peer %s:%d", socket_data->ip_address, socket_data->port);
                    ui_pipe_write_message(socket_data->pipe_fd, UI_PIPE_CONNECTION_FAILED, NULL, 0);
                }
                free(socket_data->ip_address);
                free(socket_data);
                socket_abort_connection = FALSE;
                /* Quit the thread */
                pthread_exit(NULL);
            }
            usleep(SOCKET_US_BEFORE_CONNECT_RETRY);
            retry--;
        }
    } while (ret < 0);

    /* Set the socket as non-blocking */
    fcntl(socket_data->sd, F_SETFL, O_NONBLOCK);

    while (1) {
        memcpy(&read_fds, &master_fds, sizeof(master_fds));

        ret = select(fd_max + 1, &read_fds, NULL, NULL, &tv);
        if (ret < 0) {
            g_warning("Error in select: %s", g_strerror(errno));
            free(socket_data->ip_address);
            free(socket_data);
            /* Quit the thread */
            pthread_exit(NULL);
        } else if (ret == 0) {
            /* Timeout for select: check if there is new incoming messages
             * since last GUI update
             */
            if (socket_data->nb_signals_since_last_update > 0) {
                g_debug("Timout on select and data new signal in list");
                g_debug("-> notify GUI");
                socket_notify_gui_update(socket_data);
            }

            /* Reset the timeval to the max value */
            tv.tv_usec = 1000 * SOCKET_MS_BEFORE_SIGNALLING;
        }

        /* Checking if there is data to read from the pipe */
        if (FD_ISSET(socket_data->pipe_fd, &read_fds)) {
            FD_CLR(socket_data->pipe_fd, &read_fds);
            pipe_read_message(socket_data);
        }

        /* Checking if there is data to read from the socket */
        if (FD_ISSET(socket_data->sd, &read_fds)) {
            FD_CLR(socket_data->sd, &read_fds);
            socket_read(socket_data);

            /* Update the timeout of select if there is data not notify to GUI */
            if (socket_data->nb_signals_since_last_update > 0) {
                gint64 current_time;

                current_time = g_get_monotonic_time();

                if ((current_time - socket_data->last_data_notification) > SOCKET_MS_BEFORE_SIGNALLING) {
                    socket_notify_gui_update(socket_data);
                    tv.tv_usec = 1000 * SOCKET_MS_BEFORE_SIGNALLING;
                } else {
                    /* Update tv */
                    tv.tv_usec = (1000 * SOCKET_MS_BEFORE_SIGNALLING) - (current_time - socket_data->last_data_notification);
                }
            }
        }
    }

    return NULL;
}

int socket_connect_to_remote_host(const char *remote_ip, const uint16_t port,
                                  int pipe_fd)
{
    socket_data_t *socket_data;

    socket_data = calloc(1, sizeof(*socket_data));

    socket_data->ip_address = strdup(remote_ip);

    socket_data->pipe_fd = pipe_fd;
    socket_data->port    = port;
    socket_data->sd      = -1;

    if (pthread_create(&socket_data->thread, NULL, socket_thread_fct, socket_data) != 0) {
        g_warning("Failed to create thread %d:%s", errno, strerror(errno));
        free(socket_data->ip_address);
        free(socket_data);
        return RC_FAIL;
    }

    return RC_OK;
}
