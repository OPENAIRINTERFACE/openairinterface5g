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

/*! \file udp_eNB_task.c
* \brief
* \author Sebastien ROUX, Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#include "queue.h"
#include "intertask_interface.h"
#include "assertions.h"
#include "udp_eNB_task.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "msc.h"


#define IPV4_ADDR    "%u.%u.%u.%u"
#define IPV4_ADDR_FORMAT(aDDRESS)               \
    (uint8_t)((aDDRESS)  & 0x000000ff),         \
    (uint8_t)(((aDDRESS) & 0x0000ff00) >> 8 ),  \
    (uint8_t)(((aDDRESS) & 0x00ff0000) >> 16),  \
    (uint8_t)(((aDDRESS) & 0xff000000) >> 24)


struct udp_socket_desc_s {
  int       sd;              /* Socket descriptor to use */

  pthread_t listener_thread; /* Thread affected to recv */

  char     *local_address;   /* Local ipv4 address to use */
  uint16_t  local_port;      /* Local port to use */

  task_id_t task_id;         /* Task who has requested the new endpoint */

  STAILQ_ENTRY(udp_socket_desc_s) entries;
};

static STAILQ_HEAD(udp_socket_list_s, udp_socket_desc_s) udp_socket_list;
static pthread_mutex_t udp_socket_list_mutex = PTHREAD_MUTEX_INITIALIZER;


static
struct udp_socket_desc_s *
udp_eNB_get_socket_desc(task_id_t task_id);

void udp_eNB_process_file_descriptors(
  struct epoll_event *events,
  int nb_events);

static
int
udp_eNB_create_socket(
  int port,
  char *ip_addr,
  task_id_t task_id);

int
udp_eNB_send_to(
  int sd,
  uint16_t port,
  uint32_t address,
  const uint8_t *buffer,
  uint32_t length);

void udp_eNB_receiver(struct udp_socket_desc_s *udp_sock_pP);

void *udp_eNB_task(void *args_p);

int udp_enb_init(void);
/* @brief Retrieve the descriptor associated with the task_id
 */
static
struct udp_socket_desc_s *udp_eNB_get_socket_desc(task_id_t task_id)
{
  struct udp_socket_desc_s *udp_sock_p = NULL;

#if defined(LOG_UDP) && LOG_UDP > 0
  LOG_T(UDP_, "Looking for task %d\n", task_id);
#endif

  STAILQ_FOREACH(udp_sock_p, &udp_socket_list, entries) {
    if (udp_sock_p->task_id == task_id) {
#if defined(LOG_UDP) && LOG_UDP > 0
      LOG_T(UDP_, "Found matching task desc\n");
#endif
      break;
    }
  }
  return udp_sock_p;
}

void udp_eNB_process_file_descriptors(struct epoll_event *events, int nb_events)
{
  int                       i;
  struct udp_socket_desc_s *udp_sock_p = NULL;

  if (events == NULL) {
    return;
  }

  for (i = 0; i < nb_events; i++) {
    STAILQ_FOREACH(udp_sock_p, &udp_socket_list, entries) {
      if (udp_sock_p->sd == events[i].data.fd) {
#if defined(LOG_UDP) && LOG_UDP > 0
        LOG_D(UDP_, "Found matching task desc\n");
#endif
        udp_eNB_receiver(udp_sock_p);
        break;
      }
    }
  }
}

static
int udp_eNB_create_socket(int port, char *ip_addr, task_id_t task_id)
{

  struct udp_socket_desc_s  *udp_socket_desc_p = NULL;
  int                       sd, rc;
  struct sockaddr_in        sin;

  LOG_I(UDP_, "Initializing UDP for local address %s with port %d\n", ip_addr, port);

  sd = socket(AF_INET, SOCK_DGRAM, 0);
  AssertFatal(sd > 0, "UDP: Failed to create new socket: (%s:%d)\n", strerror(errno), errno);

  memset(&sin, 0, sizeof(struct sockaddr_in));
  sin.sin_family      = AF_INET;
  sin.sin_port        = htons(port);

  if (ip_addr == NULL) {
    sin.sin_addr.s_addr = inet_addr(INADDR_ANY);
  } else {
    sin.sin_addr.s_addr = inet_addr(ip_addr);
  }

  if ((rc = bind(sd, (struct sockaddr *)&sin, sizeof(struct sockaddr_in))) < 0) {
    close(sd);
    AssertFatal(rc >= 0, "UDP: Failed to bind socket: (%s:%d) address %s port %d\n",
                strerror(errno), errno, ip_addr, port);
  }

  /* Create a new descriptor for this connection */
  udp_socket_desc_p = calloc(1, sizeof(struct udp_socket_desc_s));

  DevAssert(udp_socket_desc_p != NULL);

  udp_socket_desc_p->sd            = sd;
  udp_socket_desc_p->local_address = ip_addr;
  udp_socket_desc_p->local_port    = port;
  udp_socket_desc_p->task_id       = task_id;

  LOG_I(UDP_, "Inserting new descriptor for task %d, sd %d\n", udp_socket_desc_p->task_id, udp_socket_desc_p->sd);
  pthread_mutex_lock(&udp_socket_list_mutex);
  STAILQ_INSERT_TAIL(&udp_socket_list, udp_socket_desc_p, entries);
  pthread_mutex_unlock(&udp_socket_list_mutex);

  itti_subscribe_event_fd(TASK_UDP, sd);
  LOG_I(UDP_, "Initializing UDP for local address %s with port %d: DONE\n", ip_addr, port);
  return sd;
}

int
udp_eNB_send_to(
  int sd,
  uint16_t port,
  uint32_t address,
  const uint8_t *buffer,
  uint32_t length)
{
  struct sockaddr_in to;
  socklen_t          to_length;

  if (sd <= 0 || ((buffer == NULL) && (length > 0))) {
    LOG_E(UDP_, "udp_send_to: bad param\n");
    return -1;
  }

  memset(&to, 0, sizeof(struct sockaddr_in));
  to_length = sizeof(to);

  to.sin_family      = AF_INET;
  to.sin_port        = htons(port);
  to.sin_addr.s_addr = address;

  if (sendto(sd, (void *)buffer, (size_t)length, 0, (struct sockaddr *)&to,
             to_length) < 0) {
    LOG_E(UDP_,
          "[SD %d] Failed to send data to "IPV4_ADDR" on port %d, buffer size %u\n",
          sd, IPV4_ADDR_FORMAT(address), port, length);
    return -1;
  }

#if defined(LOG_UDP) && LOG_UDP > 0
  LOG_I(UDP_, "[SD %d] Successfully sent to "IPV4_ADDR
        " on port %d, buffer size %u, buffer address %x\n",
        sd, IPV4_ADDR_FORMAT(address), port, length, buffer);
#endif
  return 0;
}


void udp_eNB_receiver(struct udp_socket_desc_s *udp_sock_pP)
{
  uint8_t                   l_buffer[2048];
  int                n;
  socklen_t          from_len;
  struct sockaddr_in addr;
  MessageDef               *message_p        = NULL;
  udp_data_ind_t           *udp_data_ind_p   = NULL;
  uint8_t                  *forwarded_buffer = NULL;

  if (1) {
    from_len = (socklen_t)sizeof(struct sockaddr_in);

    if ((n = recvfrom(udp_sock_pP->sd, l_buffer, sizeof(l_buffer), 0,
                      (struct sockaddr *)&addr, &from_len)) < 0) {
      LOG_E(UDP_, "Recvfrom failed %s\n", strerror(errno));
      return;
    } else if (n == 0) {
      LOG_W(UDP_, "Recvfrom returned 0\n");
      return;
    } else {
      forwarded_buffer = itti_malloc(TASK_UDP, udp_sock_pP->task_id, n*sizeof(uint8_t));
      DevAssert(forwarded_buffer != NULL);
      memcpy(forwarded_buffer, l_buffer, n);
      message_p = itti_alloc_new_message(TASK_UDP, UDP_DATA_IND);
      DevAssert(message_p != NULL);
      udp_data_ind_p = &message_p->ittiMsg.udp_data_ind;
      udp_data_ind_p->buffer        = forwarded_buffer;
      udp_data_ind_p->buffer_length = n;
      udp_data_ind_p->peer_port     = htons(addr.sin_port);
      udp_data_ind_p->peer_address  = addr.sin_addr.s_addr;

#if defined(LOG_UDP) && LOG_UDP > 0
      LOG_I(UDP_, "Msg of length %d received from %s:%u\n",
            n, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
#endif

      /* TODO: this is a hack. Let's accept failures and do nothing when
       * it happens. Since itti_send_msg_to_task crashes when the message
       * queue is full we wrote itti_try_send_msg_to_task that returns -1
       * if the queue is full.
       */
      /* look for HACK_RLC_UM_LIMIT for others places related to the hack. Please do not remove this comment. */
      if (itti_send_msg_to_task(udp_sock_pP->task_id, INSTANCE_DEFAULT, message_p) < 0) {
        LOG_E(UDP_, "Failed to send message %d to task %d\n",
              UDP_DATA_IND,
              udp_sock_pP->task_id);
        itti_free(TASK_UDP, message_p);
        itti_free(TASK_UDP, forwarded_buffer);
        return;
      }
    }
  }

  //close(udp_sock_p->sd);
  //udp_sock_p->sd = -1;

  //pthread_mutex_lock(&udp_socket_list_mutex);
  //STAILQ_REMOVE(&udp_socket_list, udp_sock_p, udp_socket_desc_s, entries);
  //pthread_mutex_unlock(&udp_socket_list_mutex);
}


void *udp_eNB_task(void *args_p)
{
  int                 nb_events;
  struct epoll_event *events;
  MessageDef         *received_message_p    = NULL;
  //const char         *msg_name = NULL;
  //instance_t          instance  = 0;
  udp_enb_init();

  itti_mark_task_ready(TASK_UDP);
  MSC_START_USE();

  while(1) {
    itti_receive_msg(TASK_UDP, &received_message_p);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UDP_ENB_TASK, VCD_FUNCTION_IN);
#if defined(LOG_UDP) && LOG_UDP > 0
    LOG_D(UDP_, "Got message %p\n", &received_message_p);
#endif

    if (received_message_p != NULL) {

      //msg_name = ITTI_MSG_NAME (received_message_p);
      //instance = ITTI_MSG_INSTANCE (received_message_p);

      switch (ITTI_MSG_ID(received_message_p)) {
      case UDP_INIT: {
        LOG_D(UDP_, "Received UDP_INIT\n");
        udp_init_t *udp_init_p;
        udp_init_p = &received_message_p->ittiMsg.udp_init;
        udp_eNB_create_socket(
          udp_init_p->port,
          udp_init_p->address,
          ITTI_MSG_ORIGIN_ID(received_message_p));
      }
      break;

      case UDP_DATA_REQ: {
#if defined(LOG_UDP) && LOG_UDP > 0
        LOG_D(UDP_, "Received UDP_DATA_REQ\n");
#endif
        int     udp_sd = -1;
        ssize_t bytes_written;

        struct udp_socket_desc_s *udp_sock_p = NULL;
        udp_data_req_t           *udp_data_req_p;
        struct sockaddr_in        peer_addr;

        udp_data_req_p = &received_message_p->ittiMsg.udp_data_req;

        memset(&peer_addr, 0, sizeof(struct sockaddr_in));

        peer_addr.sin_family       = AF_INET;
        peer_addr.sin_port         = htons(udp_data_req_p->peer_port);
        peer_addr.sin_addr.s_addr  = udp_data_req_p->peer_address;

        pthread_mutex_lock(&udp_socket_list_mutex);
        udp_sock_p = udp_eNB_get_socket_desc(ITTI_MSG_ORIGIN_ID(received_message_p));

        if (udp_sock_p == NULL) {
          LOG_E(UDP_,
                "Failed to retrieve the udp socket descriptor "
                "associated with task %d\n",
                ITTI_MSG_ORIGIN_ID(received_message_p));
          pthread_mutex_unlock(&udp_socket_list_mutex);

          if (udp_data_req_p->buffer) {
            itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), udp_data_req_p->buffer);
          }

          goto on_error;
        }

        udp_sd = udp_sock_p->sd;
        pthread_mutex_unlock(&udp_socket_list_mutex);

#if defined(LOG_UDP) && LOG_UDP > 0
        LOG_D(UDP_, "[%d] Sending message of size %u to "IPV4_ADDR" and port %u\n",
              udp_sd,
              udp_data_req_p->buffer_length,
              IPV4_ADDR_FORMAT(udp_data_req_p->peer_address),
              udp_data_req_p->peer_port);
#endif
        bytes_written = sendto(
                          udp_sd,
                          &udp_data_req_p->buffer[udp_data_req_p->buffer_offset],
                          udp_data_req_p->buffer_length,
                          0,
                          (struct sockaddr *)&peer_addr,
                          sizeof(struct sockaddr_in));

        if (bytes_written != udp_data_req_p->buffer_length) {
          LOG_E(UDP_, "There was an error while writing to socket %d rc %zd"
                "(%d:%s) May be normal if GTPU kernel module loaded on same host (may NF_DROP IP packet)\n",
                udp_sd, bytes_written, errno, strerror(errno));
        }

        itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), udp_data_req_p->buffer);
      }
      break;

      case TERMINATE_MESSAGE: {
        LOG_W(UDP_, " *** Exiting UDP thread\n");
        itti_exit_task();
      }
      break;

      case MESSAGE_TEST: {
      } break;

      default: {
        LOG_W(UDP_, "Unkwnon message ID %d:%s\n",
              ITTI_MSG_ID(received_message_p),
              ITTI_MSG_NAME(received_message_p));
      }
      break;
      }

on_error:
      itti_free (ITTI_MSG_ORIGIN_ID(received_message_p), received_message_p);
      received_message_p = NULL;
    }

    nb_events = itti_get_events(TASK_UDP, &events);

    /* Now handle notifications for other sockets */
    if (nb_events > 0) {
#if defined(LOG_UDP) && LOG_UDP > 0
      LOG_D(UDP_, "UDP task Process %d events\n",nb_events);
#endif
      udp_eNB_process_file_descriptors(events, nb_events);
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UDP_ENB_TASK, VCD_FUNCTION_OUT);
  }

  LOG_I(UDP_, "Task UDP eNB exiting\n");
  return NULL;
}

int udp_enb_init(void)
{
  LOG_I(UDP_, "Initializing UDP task interface\n");
  STAILQ_INIT(&udp_socket_list);
  LOG_I(UDP_, "Initializing UDP task interface: DONE\n");
  return 0;
}
