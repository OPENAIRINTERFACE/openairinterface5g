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

#include <arpa/inet.h>
#include <netinet/in.h>

#ifndef SOCKET_H_
#define SOCKET_H_

typedef struct {
    pthread_t thread;
    int       sd;
    char     *ip_address;
    uint16_t  port;

    /* The pipe used between main thread (running GTK) and the socket thread */
    int       pipe_fd;

    /* Time used to avoid refreshing UI every time a new signal is incoming */
    gint64    last_data_notification;
    uint8_t   nb_signals_since_last_update;

    /* The last signals received which are not yet been updated in GUI */
    GList    *signal_list;
} socket_data_t;

gboolean socket_abort_connection;

int socket_connect_to_remote_host(const char *remote_ip, const uint16_t port,
                                  int pipe_fd);

int socket_disconnect_from_remote_host(void);

#endif /* SOCKET_H_ */
