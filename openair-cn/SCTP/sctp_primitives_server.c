/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file sctp_primitives_server.c
 *  \brief Main server primitives
 *  \author Sebastien ROUX
 *  \date 2013
 *  \version 1.0
 *  @ingroup _sctp
 */

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/sctp.h>

#include <arpa/inet.h>

#include "assertions.h"

#include "intertask_interface.h"
#include "sctp_primitives_server.h"
#include "mme_config.h"
#include "conversions.h"

#include "sctp_common.h"
#include "sctp_itti_messaging.h"
#include "msc.h"

#define SCTP_DUMP_LIST

#define SCTP_RC_ERROR       -1
#define SCTP_RC_NORMAL_READ  0
#define SCTP_RC_DISCONNECT   1

struct sctp_association_s {
  struct sctp_association_s *next_assoc;       ///< Next association in the list
  struct sctp_association_s *previous_assoc;   ///< Previous association in the list
  int      sd;            ///< Socket descriptor
  uint32_t ppid;          ///< Payload protocol Identifier
  uint16_t instreams;     ///< Number of input streams negociated for this connection
  uint16_t outstreams;    ///< Number of output strams negotiated for this connection
  int32_t  assoc_id;      ///< SCTP association id for the connection
  uint32_t messages_recv; ///< Number of messages received on this connection
  uint32_t messages_sent; ///< Number of messages sent on this connection

  struct   sockaddr *peer_addresses;   ///< A list of peer addresses
  int      nb_peer_addresses;
};

struct sctp_descriptor_s {
  // List of connected peers
  struct sctp_association_s *available_connections_head;
  struct sctp_association_s *available_connections_tail;

  uint32_t number_of_connections;
  uint16_t nb_instreams;
  uint16_t nb_outstreams;
};

struct sctp_arg_s {
  int      sd;
  uint32_t ppid;
};

static struct sctp_descriptor_s sctp_desc;

// Thread used to handle sctp messages
static pthread_t assoc_thread;

// LOCAL FUNCTIONS prototypes
void *sctp_receiver_thread(void *args_p);
static int sctp_send_msg(int32_t sctp_assoc_id, uint16_t stream,
                         const uint8_t *buffer, const uint32_t length);

// Association list related local functions prototypes
static struct sctp_association_s *sctp_is_assoc_in_list(int32_t assoc_id);
static struct sctp_association_s *sctp_add_new_peer(void);
static int sctp_handle_com_down(uint32_t assoc_id);
static void sctp_dump_list(void);

static struct sctp_association_s *sctp_add_new_peer(void)
{
  struct sctp_association_s *new_sctp_descriptor = NULL;

  new_sctp_descriptor = calloc(1, sizeof(struct sctp_association_s));

  if (new_sctp_descriptor == NULL) {
    SCTP_ERROR("Failed to allocate memory for new peer (%s:%d)\n",
               __FILE__, __LINE__);
    return NULL;
  }

  new_sctp_descriptor->next_assoc = NULL;
  new_sctp_descriptor->previous_assoc = NULL;

  if (sctp_desc.available_connections_tail == NULL) {
    sctp_desc.available_connections_head = new_sctp_descriptor;
    sctp_desc.available_connections_tail = sctp_desc.available_connections_head;
  } else {
    new_sctp_descriptor->previous_assoc = sctp_desc.available_connections_tail;
    sctp_desc.available_connections_tail->next_assoc = new_sctp_descriptor;
    sctp_desc.available_connections_tail = new_sctp_descriptor;
  }

  sctp_desc.number_of_connections++;

  sctp_dump_list();

  return new_sctp_descriptor;
}

static struct sctp_association_s *sctp_is_assoc_in_list(int32_t assoc_id)
{
  struct sctp_association_s *assoc_desc;

  if (assoc_id < 0) {
    return NULL;
  }

  for (assoc_desc = sctp_desc.available_connections_head;
       assoc_desc;
       assoc_desc = assoc_desc->next_assoc) {
    if (assoc_desc->assoc_id == assoc_id) {
      break;
    }
  }

  return assoc_desc;
}

static int sctp_remove_assoc_from_list(int32_t assoc_id)
{
  struct sctp_association_s *assoc_desc;

  /* Association not in the list */
  if ((assoc_desc = sctp_is_assoc_in_list(assoc_id)) == NULL) {
    return -1;
  }

  if (assoc_desc->next_assoc == NULL) {
    if (assoc_desc->previous_assoc == NULL) {
      /* Head and tail */
      sctp_desc.available_connections_head = sctp_desc.available_connections_tail = NULL;
    } else {
      /* Not head but tail */
      sctp_desc.available_connections_tail = assoc_desc->previous_assoc;
      assoc_desc->previous_assoc->next_assoc = NULL;
    }
  } else {
    if (assoc_desc->previous_assoc == NULL) {
      /* Head but not tail */
      sctp_desc.available_connections_head = assoc_desc->next_assoc;
      assoc_desc->next_assoc->previous_assoc = NULL;
    } else {
      /* Not head and not tail */
      assoc_desc->previous_assoc->next_assoc = assoc_desc->next_assoc;
      assoc_desc->next_assoc->previous_assoc = assoc_desc->previous_assoc;
    }
  }

  free(assoc_desc);
  assoc_desc = NULL;
  sctp_desc.number_of_connections --;
  return 0;
}

static void sctp_dump_assoc(struct sctp_association_s *sctp_assoc_p)
{
#if defined(SCTP_DUMP_LIST)
  int i;

  if (sctp_assoc_p == NULL) {
    return;
  }

  SCTP_DEBUG("sd           : %d\n", sctp_assoc_p->sd);
  SCTP_DEBUG("input streams: %d\n", sctp_assoc_p->instreams);
  SCTP_DEBUG("out streams  : %d\n", sctp_assoc_p->outstreams);
  SCTP_DEBUG("assoc_id     : %d\n", sctp_assoc_p->assoc_id);
  SCTP_DEBUG("peer address :\n");

  for (i = 0; i < sctp_assoc_p->nb_peer_addresses; i++) {
    char address[40];

    memset(address, 0, sizeof(address));

    if (inet_ntop(sctp_assoc_p->peer_addresses[i].sa_family,
                  sctp_assoc_p->peer_addresses[i].sa_data, address, sizeof(address)) != NULL) {
      SCTP_DEBUG("    - [%s]\n", address);
    }
  }

#else
  sctp_assoc_p = sctp_assoc_p;
#endif
}

static void sctp_dump_list(void)
{
#if defined(SCTP_DUMP_LIST)
  struct sctp_association_s *sctp_assoc_p;

  sctp_assoc_p = sctp_desc.available_connections_head;

  SCTP_DEBUG("SCTP list contains %d associations\n", sctp_desc.number_of_connections);

  while (sctp_assoc_p != NULL) {
    sctp_dump_assoc(sctp_assoc_p);
    sctp_assoc_p = sctp_assoc_p->next_assoc;
  }

#else
  sctp_dump_assoc(NULL);
#endif
}

static int sctp_send_msg(int32_t sctp_assoc_id, uint16_t stream,
                         const uint8_t *buffer, const uint32_t length)
{
  struct sctp_association_s *assoc_desc = NULL;

  DevAssert(buffer != NULL);

  if ((assoc_desc = sctp_is_assoc_in_list(sctp_assoc_id)) == NULL) {
    SCTP_DEBUG("This assoc id has not been fount in list (%d)\n",
               sctp_assoc_id);
    return -1;
  }

  if (assoc_desc->sd == -1) {
    /* The socket is invalid may be closed.
     */
    return -1;
  }

  SCTP_DEBUG("[%d][%d] Sending buffer %p of %d bytes on stream %d with ppid %d\n",
             assoc_desc->sd, sctp_assoc_id, buffer,
             length, stream, assoc_desc->ppid);

  /* Send message_p on specified stream of the sd association */
  if (sctp_sendmsg(assoc_desc->sd,
                   (const void *)buffer,
                   length,
                   NULL,
                   0,
                   assoc_desc->ppid, 0, stream, 0, 0) < 0) {
    SCTP_ERROR("send: %s:%d", strerror(errno), errno);
    return -1;
  }

  assoc_desc->messages_sent++;

  SCTP_DEBUG("Successfully sent %d bytes on stream %d\n", length, stream);

  return 0;
}

static int sctp_create_new_listener(SctpInit *init_p)
{
  struct sctp_event_subscribe event;
  struct sockaddr *addr;

  struct sctp_arg_s *sctp_arg_p;

  uint16_t i = 0, j;
  int sd;
  int used_addresses = 0;

  DevAssert(init_p != NULL);

  if (init_p->ipv4 == 0 && init_p->ipv6 == 0) {
    SCTP_ERROR("Illegal IP configuration upper layer should request at"
               "least ipv4 and/or ipv6 config\n");
    return -1;
  }

  if ((used_addresses = init_p->nb_ipv4_addr + init_p->nb_ipv6_addr) == 0) {
    SCTP_WARN("No address provided...\n");
    return -1;
  }

  addr = calloc(used_addresses, sizeof(struct sockaddr));

  SCTP_DEBUG("Creating new listen socket on port %u with\n", init_p->port);

  if (init_p->ipv4 == 1) {
    struct sockaddr_in *ip4_addr;

    SCTP_DEBUG("ipv4 addresses:\n");

    for (i = 0; i < init_p->nb_ipv4_addr; i++) {
      SCTP_DEBUG("\t- "IPV4_ADDR"\n",
                 IPV4_ADDR_FORMAT(init_p->ipv4_address[i]));
      ip4_addr = (struct sockaddr_in *)&addr[i];
      ip4_addr->sin_family = AF_INET;
      ip4_addr->sin_port   = htons(init_p->port);
      ip4_addr->sin_addr.s_addr = init_p->ipv4_address[i];
    }
  }

  if (init_p->ipv6 == 1) {
    struct sockaddr_in6 *ip6_addr;

    SCTP_DEBUG("ipv6 addresses:\n");

    for (j = 0; j < init_p->nb_ipv6_addr; j++) {
      SCTP_DEBUG("\t- %s\n", init_p->ipv6_address[j]);
      ip6_addr = (struct sockaddr_in6 *)&addr[i + j];
      ip6_addr->sin6_family = AF_INET6;
      ip6_addr->sin6_port  = htons(init_p->port);

      if (inet_pton(AF_INET6, init_p->ipv6_address[j],
                    ip6_addr->sin6_addr.s6_addr) <= 0) {
        SCTP_WARN("Provided ipv6 address %s is not valid\n",
                  init_p->ipv6_address[j]);
      }
    }
  }

  if ((sd = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
    SCTP_ERROR("socket: %s:%d\n", strerror(errno), errno);
    return -1;
  }

  memset((void *)&event, 1, sizeof(struct sctp_event_subscribe));

  if (setsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, &event,
                 sizeof(struct sctp_event_subscribe)) < 0) {
    SCTP_ERROR("setsockopt: %s:%d\n", strerror(errno), errno);
    return -1;
  }

  /* Some pre-bind socket configuration */
  if (sctp_set_init_opt(sd, sctp_desc.nb_instreams, sctp_desc.nb_outstreams,
                        0, 0) < 0) {
    goto err;
  }

  if (sctp_bindx(sd, addr, used_addresses, SCTP_BINDX_ADD_ADDR) != 0) {
    SCTP_ERROR("sctp_bindx: %s:%d\n", strerror(errno), errno);
    return -1;
  }

  if (listen(sd, 5) < 0) {
    SCTP_ERROR("listen: %s:%d\n", strerror(errno), errno);
    return -1;
  }

  if ((sctp_arg_p = malloc(sizeof(struct sctp_arg_s))) == NULL) {
    return -1;
  }

  sctp_arg_p->sd = sd;
  sctp_arg_p->ppid = init_p->ppid;

  if (pthread_create(&assoc_thread, NULL, &sctp_receiver_thread,
                     (void *)sctp_arg_p) < 0) {
    SCTP_ERROR("pthread_create: %s:%d\n", strerror(errno), errno);
    return -1;
  }

  return sd;
err:

  if (sd != -1) {
    close(sd);
    sd = -1;
  }

  return -1;
}

static
inline int sctp_read_from_socket(int sd, int ppid)
{
  int flags = 0, n;
  socklen_t from_len;
  struct sctp_sndrcvinfo sinfo;

  struct sockaddr_in addr;
  uint8_t buffer[SCTP_RECV_BUFFER_SIZE];

  if (sd < 0) {
    return -1;
  }

  memset((void *)&addr, 0, sizeof(struct sockaddr_in));
  from_len = (socklen_t)sizeof(struct sockaddr_in);
  memset((void *)&sinfo, 0, sizeof(struct sctp_sndrcvinfo));
  n = sctp_recvmsg(sd, (void *)buffer, SCTP_RECV_BUFFER_SIZE,
                   (struct sockaddr *)&addr, &from_len,
                   &sinfo, &flags);

  if (n < 0) {
    SCTP_DEBUG("An error occured during read\n");
    SCTP_ERROR("sctp_recvmsg: %s:%d\n", strerror(errno), errno);
    return SCTP_RC_ERROR;
  }

  if (flags & MSG_NOTIFICATION) {
    union sctp_notification *snp;
    snp = (union sctp_notification *)buffer;

    /* Client deconnection */
    if (SCTP_SHUTDOWN_EVENT == snp->sn_header.sn_type) {
      SCTP_DEBUG("SCTP_SHUTDOWN_EVENT received\n");
      return sctp_handle_com_down(snp->sn_shutdown_event.sse_assoc_id);
    }
    /* Association has changed. */
    else if (SCTP_ASSOC_CHANGE == snp->sn_header.sn_type) {
      struct sctp_assoc_change *sctp_assoc_changed;
      sctp_assoc_changed = &snp->sn_assoc_change;

      SCTP_DEBUG("Client association changed: %d\n", sctp_assoc_changed->sac_state);

      /* New physical association requested by a peer */
      switch (sctp_assoc_changed->sac_state) {
      case SCTP_COMM_UP: {
        struct sctp_association_s *new_association;

        sctp_get_sockinfo(sd, NULL, NULL, NULL);

        SCTP_DEBUG("New connection\n");

        if ((new_association = sctp_add_new_peer()) == NULL) {
          // TODO: handle this case
          DevMessage("Unexpected error...\n");
          return SCTP_RC_ERROR;
        } else {
          new_association->sd         = sd;
          new_association->ppid       = ppid;
          new_association->instreams  = sctp_assoc_changed->sac_inbound_streams;
          new_association->outstreams = sctp_assoc_changed->sac_outbound_streams;
          new_association->assoc_id   = sctp_assoc_changed->sac_assoc_id;

          sctp_get_localaddresses(sd, NULL, NULL);
          sctp_get_peeraddresses(sd, &new_association->peer_addresses,
                                 &new_association->nb_peer_addresses);

          if (sctp_itti_send_new_association(
                new_association->assoc_id, new_association->instreams,
                new_association->outstreams) < 0) {
            SCTP_ERROR("Failed to send message to S1AP\n");
            return SCTP_RC_ERROR;
          }
        }
      }
      break;

      default:
        break;
      }
    }
  } else {
    /* Data payload received */
    struct sctp_association_s *association;

    if ((association = sctp_is_assoc_in_list(sinfo.sinfo_assoc_id)) == NULL) {
      // TODO: handle this case
      return SCTP_RC_ERROR;
    }

    association->messages_recv++;

    if (ntohl(sinfo.sinfo_ppid) != association->ppid) {
      /* Mismatch in Payload Protocol Identifier,
       * may be we received unsollicited traffic from stack other than S1AP.
       */
      SCTP_ERROR("Received data from peer with unsollicited PPID %d, expecting %d\n",
                 ntohl(sinfo.sinfo_ppid), association->ppid);
      return SCTP_RC_ERROR;
    }

    SCTP_DEBUG("[%d][%d] Msg of length %d received from port %u, on stream %d, PPID %d\n",
               sinfo.sinfo_assoc_id, sd, n, ntohs(addr.sin_port),
               sinfo.sinfo_stream, ntohl(sinfo.sinfo_ppid));

    sctp_itti_send_new_message_ind(n, buffer, sinfo.sinfo_assoc_id, sinfo.sinfo_stream,
                                   association->instreams, association->outstreams);
  }

  return SCTP_RC_NORMAL_READ;
}

static int sctp_handle_com_down(uint32_t assoc_id)
{
  SCTP_DEBUG("Sending close connection for assoc_id %u\n", assoc_id);

  if (sctp_itti_send_com_down_ind(assoc_id) < 0) {
    SCTP_ERROR("Failed to send message to TASK_S1AP\n");
  }

  if (sctp_remove_assoc_from_list(assoc_id) < 0) {
    SCTP_ERROR("Failed to find client in list\n");
  }

  return SCTP_RC_DISCONNECT;
}

void *sctp_receiver_thread(void *args_p)
{
  struct sctp_arg_s *sctp_arg_p;

  /* maximum file descriptor number */
  int fdmax, clientsock, i;
  /* master file descriptor list */
  fd_set master;
  /* temp file descriptor list for select() */
  fd_set read_fds;

  if ((sctp_arg_p = (struct sctp_arg_s *)args_p) == NULL) {
    pthread_exit(NULL);
  }

  /* clear the master and temp sets */
  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  FD_SET(sctp_arg_p->sd, &master);
  fdmax = sctp_arg_p->sd; /* so far, it's this one*/
  MSC_START_USE();

  while(1) {

    memcpy(&read_fds, &master, sizeof(master));

    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
      SCTP_ERROR("[%d] Select() error: %s",
                 sctp_arg_p->sd, strerror(errno));
      free(args_p);
      args_p = NULL;
      pthread_exit(NULL);
    }

    for (i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == sctp_arg_p->sd) {
          /* There is data to read on listener socket. This means we have to accept
           * the connection.
           */
          if ((clientsock = accept(sctp_arg_p->sd, NULL, NULL)) < 0) {
            SCTP_ERROR("[%d] accept: %s:%d\n", sctp_arg_p->sd, strerror(errno), errno);
            free(args_p);
            args_p = NULL;
            pthread_exit(NULL);
          } else {
            FD_SET(clientsock, &master); /* add to master set */

            if(clientsock > fdmax) {
              /* keep track of the maximum */
              fdmax = clientsock;
            }
          }
        } else {
          int ret;

          /* Read from socket */
          ret = sctp_read_from_socket(i, sctp_arg_p->ppid);

          /* When the socket is disconnected we have to update
           * the fd_set.
           */
          if (ret == SCTP_RC_DISCONNECT) {
            /* Remove the socket from the FD set and update the max sd */
            FD_CLR(i, &master);

            if (i == fdmax) {
              while (FD_ISSET(fdmax, &master) == FALSE)
                fdmax -= 1;
            }
          }
        }
      }
    }
  }

  free(args_p);
  args_p = NULL;
  return NULL;
}

static void *sctp_intertask_interface(void *args_p)
{
  itti_mark_task_ready(TASK_SCTP);
  MSC_START_USE();

  while(1) {
    MessageDef *received_message_p;
    itti_receive_msg(TASK_SCTP, &received_message_p);

    switch (ITTI_MSG_ID(received_message_p)) {
    case SCTP_INIT_MSG: {
      SCTP_DEBUG("Received SCTP_INIT_MSG\n");

      /* We received a new connection request */
      if (sctp_create_new_listener(&received_message_p->ittiMsg.sctpInit) < 0) {
        /* SCTP socket creation or bind failed... */
        SCTP_ERROR("Failed to create new SCTP listener\n");
      }
    }
    break;

    case SCTP_CLOSE_ASSOCIATION: {

    } break;

    case SCTP_DATA_REQ: {
      if (sctp_send_msg(SCTP_DATA_REQ(received_message_p).assocId,
                        SCTP_DATA_REQ(received_message_p).stream,
                        SCTP_DATA_REQ(received_message_p).buffer,
                        SCTP_DATA_REQ(received_message_p).bufLen) < 0) {
        SCTP_ERROR("Failed to send message over SCTP\n");
      }
    }
    break;

    case MESSAGE_TEST: {
      //                 int i = 10000;
      //                 while(i--);
    } break;

    case TERMINATE_MESSAGE: {
      itti_exit_task();
    }
    break;

    default: {
      SCTP_DEBUG("Unkwnon message ID %d:%s\n",
                 ITTI_MSG_ID(received_message_p),
                 ITTI_MSG_NAME(received_message_p));
    }
    break;
    }

    itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), received_message_p);
    received_message_p = NULL;
  }

  return NULL;
}

int sctp_init(const mme_config_t *mme_config_p)
{
  SCTP_DEBUG("Initializing SCTP task interface\n");

  memset(&sctp_desc, 0, sizeof(struct sctp_descriptor_s));

  /* Number of streams from configuration */
  sctp_desc.nb_instreams  = mme_config_p->sctp_config.in_streams;
  sctp_desc.nb_outstreams = mme_config_p->sctp_config.out_streams;

  if (itti_create_task(TASK_SCTP, &sctp_intertask_interface,
                       NULL) < 0) {
    SCTP_ERROR("create task failed");
    SCTP_DEBUG("Initializing SCTP task interface: FAILED\n");
    return -1;
  }

  SCTP_DEBUG("Initializing SCTP task interface: DONE\n");
  return 0;
}
