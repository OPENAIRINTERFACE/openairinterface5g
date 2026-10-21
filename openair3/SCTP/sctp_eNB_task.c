/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <pthread.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/sctp.h>

#include <arpa/inet.h>

#include "assertions.h"
#include "common/utils/system.h"
#include "queue.h"

#include "intertask_interface.h"

#include "sctp_default_values.h"
#include "sctp_common.h"
#include "sctp_eNB_itti_messaging.h"

/* Used to format an uint32_t containing an ipv4 address */
#define IPV4_ADDR    "%u.%u.%u.%u"
#define IPV4_ADDR_FORMAT(aDDRESS)               \
    (uint8_t)((aDDRESS)  & 0x000000ff),         \
    (uint8_t)(((aDDRESS) & 0x0000ff00) >> 8 ),  \
    (uint8_t)(((aDDRESS) & 0x00ff0000) >> 16),  \
    (uint8_t)(((aDDRESS) & 0xff000000) >> 24)


enum sctp_connection_type_e {
    SCTP_TYPE_CLIENT,
    SCTP_TYPE_SERVER,
    SCTP_TYPE_MULTI_SERVER,
    SCTP_TYPE_MAX
};

typedef struct sctp_cnx_list_elm_s {
    STAILQ_ENTRY(sctp_cnx_list_elm_s) entries;

    /* Type of this association
     */
    enum sctp_connection_type_e connection_type;

    int        sd;              ///< Socket descriptor of connection */
    uint16_t   local_port;
    uint16_t   in_streams;      ///< Number of input streams negociated for this connection
    uint16_t   out_streams;     ///< Number of output streams negotiated for this connection
    uint16_t   ppid;            ///< Payload protocol Identifier
    sctp_assoc_t assoc_id;      ///< SCTP association id for the connection     (note4debug host byte order)
    uint32_t   messages_recv;   ///< Number of messages received on this connection
    uint32_t   messages_sent;   ///< Number of messages sent on this connection
    task_id_t  task_id;         ///< Task id of the task who asked for this connection
    instance_t instance;        ///< Instance
    uint16_t   cnx_id;          ///< Upper layer identifier

    struct   sockaddr *peer_addresses;///< A list of peer addresses for server socket
    int      nb_peer_addresses; ///< For server socket
} sctp_cnx_list_elm_t;


static STAILQ_HEAD(sctp_cnx_list_head, sctp_cnx_list_elm_s) sctp_cnx_list;
static uint16_t sctp_nb_cnx = 0;

//------------------------------------------------------------------------------
struct sctp_cnx_list_elm_s *sctp_get_cnx(sctp_assoc_t assoc_id, int sd)
{
    struct sctp_cnx_list_elm_s *elm;

    STAILQ_FOREACH(elm, &sctp_cnx_list, entries) {
        if (assoc_id != -1) {
            if (elm->assoc_id == assoc_id) {
                return elm;
            }
        } else {
            if (elm->sd == sd) {
                return elm;
            }
        }
    }

    return NULL;
}

//------------------------------------------------------------------------------
static inline
void
sctp_eNB_accept_associations_multi(
    struct sctp_cnx_list_elm_s *sctp_cnx)
{
    int                    ns;
    int                    n;
    int                    flags = 0;
    socklen_t              from_len;
    struct sctp_sndrcvinfo sinfo;

    struct sockaddr_in addr;
    uint8_t buffer[SCTP_RECV_BUFFER_SIZE];

    DevAssert(sctp_cnx != NULL);

    memset((void *)&addr, 0, sizeof(struct sockaddr_in));
    from_len = (socklen_t)sizeof(struct sockaddr_in);
    memset((void *)&sinfo, 0, sizeof(struct sctp_sndrcvinfo));

    n = sctp_recvmsg(sctp_cnx->sd, (void *)buffer, SCTP_RECV_BUFFER_SIZE,
                     (struct sockaddr *)&addr, &from_len,
                     &sinfo, &flags);

    if (n < 0) {
        if (errno == ENOTCONN) {
            SCTP_DEBUG("Received not connected for sd %d\n", sctp_cnx->sd);
            close(sctp_cnx->sd);
        } else {
            SCTP_DEBUG("An error occured during read\n");
            SCTP_ERROR("sctp_recvmsg (fd %d, len %d ): %s:%d\n", sctp_cnx->sd, n, strerror(errno), errno);
        }
        return;
    }

    if (flags & MSG_NOTIFICATION) {
        union sctp_notification *snp;
        snp = (union sctp_notification *)buffer;

        SCTP_DEBUG("Received notification for sd %d, type %u\n",
                   sctp_cnx->sd, snp->sn_header.sn_type);

        /* Association has changed. */
        if (SCTP_ASSOC_CHANGE == snp->sn_header.sn_type) {
            struct sctp_assoc_change *sctp_assoc_changed;
            sctp_assoc_changed = &snp->sn_assoc_change;

            SCTP_DEBUG("Client association changed: %d\n", sctp_assoc_changed->sac_state);

            /* New physical association requested by a peer */
            switch (sctp_assoc_changed->sac_state) {
            case SCTP_COMM_UP: {
                SCTP_DEBUG("Comm up notified for sd %d, assigned assoc_id %d\n",
                           sctp_cnx->sd, sctp_assoc_changed->sac_assoc_id);
                struct sctp_cnx_list_elm_s *new_cnx;

                new_cnx = calloc(1, sizeof(*sctp_cnx));

                DevAssert(new_cnx != NULL);

                new_cnx->connection_type = SCTP_TYPE_CLIENT;

                ns = sctp_peeloff(sctp_cnx->sd, sctp_assoc_changed->sac_assoc_id);

                new_cnx->sd         = ns;
                new_cnx->task_id    = sctp_cnx->task_id;
                new_cnx->cnx_id     = 0;
                new_cnx->ppid       = sctp_cnx->ppid;
                new_cnx->instance   = sctp_cnx->instance;
                new_cnx->local_port = sctp_cnx->local_port;
                new_cnx->assoc_id   = sctp_assoc_changed->sac_assoc_id;

                if (sctp_get_sockinfo(ns, &new_cnx->in_streams, &new_cnx->out_streams,
                                      &new_cnx->assoc_id) != 0) {
                    SCTP_ERROR("sctp_get_sockinfo failed\n");
                    close(ns);
                    free(new_cnx);
                    return;
                }

                /* Insert new element at end of list */
                STAILQ_INSERT_TAIL(&sctp_cnx_list, new_cnx, entries);
                sctp_nb_cnx++;

                /* Add the socket to list of fd monitored by ITTI */
                itti_subscribe_event_fd(TASK_SCTP, ns);

                sctp_itti_send_association_ind(new_cnx->task_id, new_cnx->instance,
                                               new_cnx->assoc_id, new_cnx->local_port,
                                               new_cnx->out_streams, new_cnx->in_streams);
            }
            break;

            default:
                break;
            }
        }
    } else {
        SCTP_DEBUG("No notification from SCTP\n");
    }
}

//------------------------------------------------------------------------------
static void
sctp_handle_new_association_req_multi(
    const instance_t instance,
    const task_id_t requestor,
    const sctp_new_association_req_multi_t * const sctp_new_association_req_p)
{
    int                           ns;
    int sd;

    sctp_assoc_t assoc_id = 0;

    struct sctp_cnx_list_elm_s   *sctp_cnx = NULL;
    enum sctp_connection_type_e   connection_type = SCTP_TYPE_CLIENT;

    /* Prepare a new SCTP association as requested by upper layer and try to connect
     * to remote host.
     */
    DevAssert(sctp_new_association_req_p != NULL);

    sd = sctp_new_association_req_p->multi_sd;

    /* Create new socket with IPv6 affinity */
//#warning "SCTP may Force IPv4 only, here"

    /* Mark the socket as non-blocking */
    //if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0) {
    //SCTP_ERROR("fcntl F_SETFL O_NONBLOCK failed: %s\n",
    //         strerror(errno));
    //close(sd);
    //return;
    //}

    /* SOCK_STREAM socket type requires an explicit connect to the remote host
     * address and port.
     * Only use IPv4 for the first connection attempt
     */
    if ((sctp_new_association_req_p->remote_address.ipv6 != 0) ||
            (sctp_new_association_req_p->remote_address.ipv4 != 0)) {
        uint8_t address_index = 0;
        const net_ip_address_t *remote = &sctp_new_association_req_p->remote_address;
        uint8_t used_address  = remote->ipv6 + remote->ipv4;
        struct sockaddr_storage addr[used_address];

        memset(addr, 0, used_address * sizeof(*addr));

        if (remote->ipv6 == 1) {
            struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)&addr[address_index];
            if (inet_pton(AF_INET6, remote->ipv6_address, &sa6->sin6_addr.s6_addr) != 1) {
                SCTP_ERROR("Failed to convert ipv6 address %*s to network type\n",
                           (int)strlen(remote->ipv6_address),
                           remote->ipv6_address);
                //close(sd);
                //return;
                exit_fun("sctp_handle_new_association_req_multi fatal: inet_pton error");
            }

            SCTP_DEBUG("Converted ipv6 address %*s to network type\n",
                       (int)strlen(remote->ipv6_address),
                       remote->ipv6_address);

            sa6->sin6_family = AF_INET6;
            sa6->sin6_port   = htons(sctp_new_association_req_p->port);
            address_index++;
        }

        if (remote->ipv4 == 1) {
            struct sockaddr_in *sa = (struct sockaddr_in *)&addr[address_index];
            if (inet_pton(AF_INET, remote->ipv4_address,
                          &sa->sin_addr.s_addr) != 1) {
                SCTP_ERROR("Failed to convert ipv4 address %*s to network type\n",
                           (int)strlen(remote->ipv4_address),
                           remote->ipv4_address);
                //close(sd);
                //return;
                exit_fun("sctp_handle_new_association_req_multi fatal: inet_pton error");
            }

            SCTP_DEBUG("Converted ipv4 address %*s to network type\n",
                       (int)strlen(remote->ipv4_address),
                       remote->ipv4_address);

            sa->sin_family = AF_INET;
            sa->sin_port   = htons(sctp_new_association_req_p->port);
            address_index++;
        }

        /* Connect to remote host and port */
        if (sctp_connectx(sd, (struct sockaddr *)addr, 1, &assoc_id) < 0) {
            /* sctp_connectx on non-blocking socket return EINPROGRESS */
            if (errno != EINPROGRESS) {
                SCTP_ERROR("Connect failed: %s\n", strerror(errno));
                sctp_itti_send_association_resp(
                    requestor, instance, -1, sctp_new_association_req_p->ulp_cnx_id,
                    SCTP_STATE_UNREACHABLE, 0, 0);
                /* Add the socket to list of fd monitored by ITTI */
                //itti_unsubscribe_event_fd(TASK_SCTP, sd);
                //close(sd);
                return;
            } else {
                SCTP_DEBUG("connectx assoc_id  %d in progress..., used %d addresses\n",
                           assoc_id, used_address);
            }
        } else {
            SCTP_DEBUG("sctp_connectx SUCCESS, socket %d used %d addresses assoc_id %d\n",
		       sd,
                       used_address,
                       assoc_id);
        }
    }

    ns = sctp_peeloff(sd, assoc_id);
    if (ns == -1) {
      perror("sctp_peeloff");
      printf("sctp_peeloff: sd=%d assoc_id=%d\n", sd, assoc_id);
      exit_fun("sctp_handle_new_association_req_multi fatal: sctp_peeloff error");
    }

    sctp_cnx = calloc(1, sizeof(*sctp_cnx));

    sctp_cnx->connection_type = connection_type;

    sctp_cnx->sd       = ns;
    sctp_cnx->task_id  = requestor;
    sctp_cnx->cnx_id   = sctp_new_association_req_p->ulp_cnx_id;
    sctp_cnx->ppid     = sctp_new_association_req_p->ppid;
    sctp_cnx->instance = instance;
    sctp_cnx->assoc_id = assoc_id;

    /* Add the socket to list of fd monitored by ITTI */
    itti_subscribe_event_fd(TASK_SCTP, ns);

    /* Insert new element at end of list */
    STAILQ_INSERT_TAIL(&sctp_cnx_list, sctp_cnx, entries);
    sctp_nb_cnx++;

    SCTP_DEBUG("Inserted new descriptor for sd %d in list, nb elements %u, assoc_id %d\n",
               ns, sctp_nb_cnx, assoc_id);
}

static const char *print_ip(struct addrinfo *p, char *buf, size_t buf_len)
{
  void *addr;
  if (p->ai_family == AF_INET) {
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
    addr = &ipv4->sin_addr;
  } else {
    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
    addr = &ipv6->sin6_addr;
  }

  // convert the IP to a string and print it:
  return inet_ntop(p->ai_family, addr, buf, buf_len);
}

static void sctp_handle_new_association_req(const instance_t instance,
                                            const task_id_t requestor,
                                            const sctp_new_association_req_t *const req)
{
  enum sctp_connection_type_e connection_type = SCTP_TYPE_CLIENT;

  /* local address: IPv4 has priority, but can also handle IPv6 */
  const char *local = NULL;
  if (req->local_address.ipv6) {
    local = req->local_address.ipv6_address;
    SCTP_WARN("please specify IPv6 addresses in the IPv4 field, IPv4 handles both\n");
  }
  if (req->local_address.ipv4)
    local = req->local_address.ipv4_address;

  /* Prepare a new SCTP association as requested by upper layer and try to connect
   * to remote host.
   */
  DevAssert(req != NULL);
  struct addrinfo hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM, .ai_protocol = IPPROTO_SCTP};
  if (local == NULL)
    hints.ai_flags = AI_PASSIVE;
  struct addrinfo *serv;

  int status = getaddrinfo(local, NULL, &hints, &serv);
  AssertFatal(status == 0, "getaddrinfo(%s) failed: %s\n", local, gai_strerror(status));

  int sd;
  struct addrinfo *p = NULL;
  for (p = serv; p != NULL; p = p->ai_next) {
    char buf[512];
    const char *ip = print_ip(p, buf, sizeof(buf));
    SCTP_DEBUG("Trying %s for client socket creation\n", ip);

    if ((sd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) == -1) {
      SCTP_WARN("Socket creation failed: %s\n", strerror(errno));
      continue;
    }

    /* we assume we can set options, or something is likely broken; the
     * connection won't operate properly */
    int ret = sctp_set_init_opt(sd, req->in_streams, req->out_streams, SCTP_MAX_ATTEMPTS, SCTP_TIMEOUT);
    AssertFatal(ret == 0, "sctp_set_init_opt() failed\n");

    /* Subscribe to all events */
    struct sctp_event_subscribe events = {0};
    events.sctp_data_io_event = 1;
    events.sctp_association_event = 1;
    events.sctp_address_event = 1;
    events.sctp_send_failure_event = 1;
    events.sctp_peer_error_event = 1;
    events.sctp_shutdown_event = 1;
    events.sctp_partial_delivery_event = 1;

    /* as above */
    ret = setsockopt(sd, serv->ai_protocol, SCTP_EVENTS, &events, 8);
    AssertFatal(ret == 0, "setsockopt() IPPROTO_SCTP_EVENTS failed: %s\n", strerror(errno));

    /* if that fails, we will try the next address */
    ret = sctp_bindx(sd, p->ai_addr, 1, SCTP_BINDX_ADD_ADDR);
    if (ret != 0) {
      SCTP_WARN("sctp_bindx() SCTP_BINDX_ADD_ADDR failed: errno %d %s\n", errno, strerror(errno));
      close(sd);
      continue;
    }

    SCTP_DEBUG("sctp_bindx() SCTP_BINDX_ADD_ADDR: socket bound to %s/%s\n", local, ip);
    break;
  }

  freeaddrinfo(serv);

  if (p == NULL) {
    SCTP_ERROR("could not open socket, no SCTP connection established\n");
    return;
  }

  /* SOCK_STREAM socket type requires an explicit connect to the remote host
   * address and port. */
  /* remote address: IPv4 has priority, but can also handle IPv6 */
  const char *remote = NULL;
  if (req->remote_address.ipv6) {
    remote = req->remote_address.ipv6_address;
    SCTP_WARN("please specify IPv6 addresses in the IPv4 field, IPv4 handles both\n");
  }
  if (req->remote_address.ipv4)
    remote = req->remote_address.ipv4_address;
  sctp_assoc_t assoc_id = 0;
  if (remote != NULL) {
    struct addrinfo hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM, .ai_protocol = IPPROTO_SCTP};
    struct addrinfo *serv;

    char port[12];
    snprintf(port, sizeof(port), "%d", req->port);
    int status = getaddrinfo(remote, port, &hints, &serv);
    AssertFatal(status == 0, "getaddrinfo(node %s service %s) failed: %s\n", remote, port, gai_strerror(status));

    struct addrinfo *p = NULL;
    for (p = serv; p != NULL; p = p->ai_next) {
      char buf[512];
      const char *ip = print_ip(p, buf, sizeof(buf));
      SCTP_DEBUG("Trying to connect to %s for remote end %s\n", ip, remote);

      if (sctp_connectx(sd, p->ai_addr, 1, &assoc_id) < 0) {
        /* sctp_connectx on non-blocking socket return EINPROGRESS */
        if (errno != EINPROGRESS) {
          SCTP_ERROR("Connect failed: %s\n", strerror(errno));
          sctp_itti_send_association_resp(requestor, instance, -1, req->ulp_cnx_id, SCTP_STATE_UNREACHABLE, 0, 0);
          freeaddrinfo(serv);
          close(sd);
          return;
        } else {
          SCTP_DEBUG("sctp_connectx(): assoc_id %d in progress...\n", assoc_id);
        }
      } else {
        SCTP_DEBUG("sctp_connectx() SUCCESS: used assoc_id %d\n", assoc_id);
      }
      break;
    }

    freeaddrinfo(serv);
  } else {
    /* I am not sure that this is relevant; we already did sctp_bindx() above */
    connection_type = SCTP_TYPE_SERVER;

    /* No remote address provided -> only bind the socket for now.
     * Connection will be accepted in the main event loop
     */
    /*
    struct sockaddr_in6 addr6;


    addr6.sin6_family = AF_INET6;
    addr6.sin6_addr = in6addr_any;
    addr6.sin6_port = htons(req->port);
    addr6.sin6_flowinfo = 0;

    if (bind(sd, (struct sockaddr *)&addr6, sizeof(addr6)) < 0) {
      SCTP_ERROR("Failed to bind the socket to address any (v4/v6): %s\n", strerror(errno));
      close(sd);
      return;
    }
    */
  }

  struct sctp_cnx_list_elm_s *sctp_cnx = calloc(1, sizeof(*sctp_cnx));
  AssertFatal(sctp_cnx != NULL, "out of memory\n");
  sctp_cnx->connection_type = connection_type;

  sctp_cnx->sd = sd;
  sctp_cnx->task_id = requestor;
  sctp_cnx->cnx_id = req->ulp_cnx_id;
  sctp_cnx->ppid = req->ppid;
  sctp_cnx->instance = instance;
  sctp_cnx->assoc_id = assoc_id;

  /* Insert new element at end of list */
  STAILQ_INSERT_TAIL(&sctp_cnx_list, sctp_cnx, entries);
  sctp_nb_cnx++;

  /* Add the socket to list of fd monitored by ITTI */
  itti_subscribe_event_fd(TASK_SCTP, sd);

  SCTP_DEBUG("Inserted new descriptor for sd %d in list, nb elements %u, assoc_id %d\n", sd, sctp_nb_cnx, assoc_id);
}

//------------------------------------------------------------------------------
static void sctp_send_data(sctp_data_req_t *sctp_data_req_p)
{
    struct sctp_cnx_list_elm_s *sctp_cnx = NULL;

    DevAssert(sctp_data_req_p != NULL);
    DevAssert(sctp_data_req_p->buffer != NULL);
    DevAssert(sctp_data_req_p->buffer_length > 0);

    sctp_cnx = sctp_get_cnx(sctp_data_req_p->assoc_id, 0);

    if (sctp_cnx == NULL) {
        SCTP_ERROR("Failed to find SCTP description for assoc_id %d\n",
                   sctp_data_req_p->assoc_id);
        /* TODO: notify upper layer */
        return;
    }

    if (sctp_data_req_p->stream >= sctp_cnx->out_streams) {
        SCTP_ERROR("Requested stream (%"PRIu16") >= nb out streams (%"PRIu16")\n",
                   sctp_data_req_p->stream, sctp_cnx->out_streams);
        return;
    }

    /* Send message on specified stream of the sd association
     * NOTE: PPID should be defined in network order
     */
    if (sctp_sendmsg(sctp_cnx->sd, sctp_data_req_p->buffer,
                     sctp_data_req_p->buffer_length, NULL, 0,
                     htonl(sctp_cnx->ppid), 0, sctp_data_req_p->stream, 0, 0) < 0) {
        SCTP_ERROR("Sctp_sendmsg failed: %s\n", strerror(errno));
        /* TODO: notify upper layer */
        return;
    }
    free(sctp_data_req_p->buffer); // assuming it has been malloced
    SCTP_DEBUG("Successfully sent %u bytes on stream %d for assoc_id %d\n",
               sctp_data_req_p->buffer_length, sctp_data_req_p->stream,
               sctp_cnx->assoc_id);
}

//------------------------------------------------------------------------------
static int sctp_close_association(sctp_close_association_t *close_association_p)
{

    struct sctp_cnx_list_elm_s *sctp_cnx = NULL;

    DevAssert(close_association_p != NULL);
    sctp_cnx = sctp_get_cnx(close_association_p->assoc_id, 0);

    if (sctp_cnx == NULL) {
        SCTP_ERROR("Failed to find SCTP description for assoc_id %d\n",
                   close_association_p->assoc_id);
        /* TODO: notify upper layer */
        return -1;
    } else {
        close(sctp_cnx->sd);
        STAILQ_REMOVE(&sctp_cnx_list, sctp_cnx, sctp_cnx_list_elm_s, entries);
        SCTP_DEBUG("Removed assoc_id %d (closed socket %u)\n",
                   sctp_cnx->assoc_id, (unsigned int)sctp_cnx->sd);
    }

    return 0;
}

static int sctp_create_new_listener(const instance_t instance, const task_id_t requestor, sctp_init_t *init_p, int server_type)
{
  DevAssert(init_p != NULL);
  DevAssert(init_p->bind_address != NULL);
  int in_streams = 1;
  int out_streams = 1;

  /* local address: IPv4 has priority, but can also handle IPv6 */
  const char *local = init_p->bind_address;

  struct addrinfo hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM, .ai_protocol = IPPROTO_SCTP};
  struct addrinfo *serv;

  char port[12];
  snprintf(port, sizeof(port), "%d", init_p->port);
  int status = getaddrinfo(local, port, &hints, &serv);
  AssertFatal(status == 0, "getaddrinfo() failed: %s\n", gai_strerror(status));

  int sd;
  struct addrinfo *p = NULL;
  for (p = serv; p != NULL; p = p->ai_next) {
    char buf[512];
    const char *ip = print_ip(p, buf, sizeof(buf));
    SCTP_DEBUG("Trying %s for server socket creation\n", ip);

    /* SOCK_SEQPACKET to be able to reuse(?) socket through SCTP_INIT_MSG_MULTI_REQ */
    int socktype = server_type ? SOCK_SEQPACKET : SOCK_STREAM;
    if ((sd = socket(serv->ai_family, socktype, serv->ai_protocol)) == -1) {
      SCTP_WARN("Socket creation failed: %s\n", strerror(errno));
      continue;
    }

    /* we assume we can set options, or something is likely broken; the
     * connection won't operate properly */
    int ret = sctp_set_init_opt(sd, in_streams, out_streams, 0, 0);
    AssertFatal(ret == 0, "sctp_set_init_opt() failed\n");

    struct sctp_event_subscribe event = {0};
    event.sctp_data_io_event = 1;
    event.sctp_association_event = 1;
    event.sctp_address_event = 1;
    event.sctp_send_failure_event = 1;
    event.sctp_peer_error_event = 1;
    event.sctp_shutdown_event = 1;
    event.sctp_partial_delivery_event = 1;

    /* as above */
    ret = setsockopt(sd, serv->ai_protocol, SCTP_EVENTS, &event, 8);
    AssertFatal(ret == 0, "setsockopt() IPPROTO_SCTP_EVENTS failed: %s\n", strerror(errno));

    /* if that fails, we will try the next address */
    ret = sctp_bindx(sd, p->ai_addr, 1, SCTP_BINDX_ADD_ADDR);
    if (ret != 0) {
      SCTP_WARN("sctp_bindx() SCTP_BINDX_ADD_ADDR failed: errno %d %s\n", errno, strerror(errno));
      close(sd);
      continue;
    }

    if (listen(sd, 5) < 0) {
      SCTP_WARN("listen() failed: %s:%d\n", strerror(errno), errno);
      close(sd);
      return -1;
    }

    SCTP_DEBUG("Created listen socket %d on %s:%s local %s\n", sd, ip, port, local);

    break;
  }

  freeaddrinfo(serv);

  if (p == NULL) {
    SCTP_ERROR("could not open server socket, no SCTP listener active\n");
    return -1;
  }

  struct sctp_cnx_list_elm_s *sctp_cnx = calloc(1, sizeof(*sctp_cnx));
  AssertFatal(sctp_cnx != NULL, "out of memory\n");

  sctp_cnx->connection_type = server_type ? SCTP_TYPE_MULTI_SERVER : SCTP_TYPE_SERVER;
  sctp_cnx->sd = sd;
  sctp_cnx->local_port = init_p->port;
  sctp_cnx->in_streams = in_streams;
  sctp_cnx->out_streams = out_streams;
  sctp_cnx->ppid = init_p->ppid;
  sctp_cnx->task_id = requestor;
  sctp_cnx->instance = instance;
  /* Insert new element at end of list */
  STAILQ_INSERT_TAIL(&sctp_cnx_list, sctp_cnx, entries);
  sctp_nb_cnx++;

  /* Add the socket to list of fd monitored by ITTI */
  itti_subscribe_event_fd(TASK_SCTP, sd);

  return sd;
}

//------------------------------------------------------------------------------
static inline
void
sctp_eNB_accept_associations(
    struct sctp_cnx_list_elm_s *sctp_cnx)
{
    int             client_sd;
    struct sockaddr_in6 saddr;
    socklen_t       saddr_size;

    DevAssert(sctp_cnx != NULL);

    saddr_size = sizeof(saddr);

    /* There is a new client connecting. Accept it...
     */
    if ((client_sd = accept(sctp_cnx->sd, (struct sockaddr*)&saddr, &saddr_size)) < 0) {
        SCTP_ERROR("[%d] accept failed: %s:%d\n", sctp_cnx->sd, strerror(errno), errno);
    } else {
        struct sctp_cnx_list_elm_s *new_cnx;
        uint16_t port;

        /* This is an ipv6 socket */
        port = saddr.sin6_port;

        /* Contrary to BSD, client socket does not inherit O_NONBLOCK option */
        if (fcntl(client_sd, F_SETFL, O_NONBLOCK) < 0) {
            SCTP_ERROR("fcntl F_SETFL O_NONBLOCK failed: %s\n",
                       strerror(errno));
            close(client_sd);
            return;
        }

        new_cnx = calloc(1, sizeof(*sctp_cnx));

        DevAssert(new_cnx != NULL);

        new_cnx->connection_type = SCTP_TYPE_CLIENT;

        new_cnx->sd         = client_sd;
        new_cnx->task_id    = sctp_cnx->task_id;
        new_cnx->cnx_id     = 0;
        new_cnx->ppid       = sctp_cnx->ppid;
        new_cnx->instance   = sctp_cnx->instance;
        new_cnx->local_port = sctp_cnx->local_port;

        if (sctp_get_sockinfo(client_sd, &new_cnx->in_streams, &new_cnx->out_streams,
                              &new_cnx->assoc_id) != 0) {
            SCTP_ERROR("sctp_get_sockinfo failed\n");
            close(client_sd);
            free(new_cnx);
            return;
        }

        /* Insert new element at end of list */
        STAILQ_INSERT_TAIL(&sctp_cnx_list, new_cnx, entries);
        sctp_nb_cnx++;

        /* Add the socket to list of fd monitored by ITTI */
        itti_subscribe_event_fd(TASK_SCTP, client_sd);

        sctp_itti_send_association_ind(new_cnx->task_id, new_cnx->instance,
                                       new_cnx->assoc_id, port,
                                       new_cnx->out_streams, new_cnx->in_streams);
    }
}

//------------------------------------------------------------------------------
static inline
void
sctp_eNB_read_from_socket(
    struct sctp_cnx_list_elm_s *sctp_cnx)
{
    DevAssert(sctp_cnx != NULL);

    int    flags = 0;
    struct sctp_sndrcvinfo sinfo={0};
    uint8_t buffer[SCTP_RECV_BUFFER_SIZE];

    int n = sctp_recvmsg(sctp_cnx->sd, (void *)buffer, SCTP_RECV_BUFFER_SIZE,
                    NULL, NULL, 
                     &sinfo, &flags);

    if (n < 0) {
        if( (errno == ENOTCONN) || (errno == ECONNRESET) || (errno == ETIMEDOUT) || (errno == ECONNREFUSED) )
        {
            itti_unsubscribe_event_fd(TASK_SCTP, sctp_cnx->sd);

            SCTP_DEBUG("Received not connected for sd %d\n", sctp_cnx->sd);
            SCTP_ERROR("sctp_recvmsg (fd %d, len %d ): %s:%d\n", sctp_cnx->sd, n, strerror(errno), errno);

            sctp_itti_send_association_resp(
                sctp_cnx->task_id, sctp_cnx->instance, sctp_cnx->assoc_id,
                sctp_cnx->cnx_id, SCTP_STATE_UNREACHABLE, 0, 0);

            close(sctp_cnx->sd);
            STAILQ_REMOVE(&sctp_cnx_list, sctp_cnx, sctp_cnx_list_elm_s, entries);
            sctp_nb_cnx--;
            free(sctp_cnx);
        } else {
            SCTP_DEBUG("An error occured during read\n");
            SCTP_ERROR("sctp_recvmsg (fd %d, len %d ): %s:%d\n", sctp_cnx->sd, n, strerror(errno), errno);
        }

        return;
    } else if (n == 0) {
        SCTP_DEBUG("return of sctp_recvmsg is 0...\n");
        return;
    }

    if (!(flags & MSG_EOR)) {
      SCTP_ERROR("partial SCTP message received, discarding message\n");
      return;
    }

    if (flags & MSG_NOTIFICATION) {
        union sctp_notification *snp;
        snp = (union sctp_notification *)buffer;

        SCTP_DEBUG("Received notification for sd %d, type %u\n",
                   sctp_cnx->sd, snp->sn_header.sn_type);

        /* Client deconnection */
        if (SCTP_SHUTDOWN_EVENT == snp->sn_header.sn_type) {
            itti_unsubscribe_event_fd(TASK_SCTP, sctp_cnx->sd);

            SCTP_WARN("Received SCTP SHUTDOWN EVENT\n");
            close(sctp_cnx->sd);

            sctp_itti_send_association_resp(
                sctp_cnx->task_id, sctp_cnx->instance, sctp_cnx->assoc_id,
                sctp_cnx->cnx_id, SCTP_STATE_SHUTDOWN,
                0, 0);

            STAILQ_REMOVE(&sctp_cnx_list, sctp_cnx, sctp_cnx_list_elm_s, entries);
            sctp_nb_cnx--;

            free(sctp_cnx);
        }
        /* Association has changed. */
        else if (SCTP_ASSOC_CHANGE == snp->sn_header.sn_type) {
            struct sctp_assoc_change *sctp_assoc_changed;
            sctp_assoc_changed = &snp->sn_assoc_change;

            SCTP_DEBUG("Client association changed: %d\n", sctp_assoc_changed->sac_state);

            /* New physical association requested by a peer */
            switch (sctp_assoc_changed->sac_state) {
            case SCTP_COMM_UP: {
                if (sctp_get_peeraddresses(sctp_cnx->sd, NULL, NULL) != 0) {
                    /* TODO Failure -> notify upper layer */
                } else {
                    sctp_get_sockinfo(sctp_cnx->sd, &sctp_cnx->in_streams,
                                      &sctp_cnx->out_streams, &sctp_cnx->assoc_id);
                }

                SCTP_DEBUG("Comm up notified for sd %d, assigned assoc_id %d\n",
                           sctp_cnx->sd, sctp_cnx->assoc_id);

                sctp_itti_send_association_resp(
                    sctp_cnx->task_id, sctp_cnx->instance, sctp_cnx->assoc_id,
                    sctp_cnx->cnx_id, SCTP_STATE_ESTABLISHED,
                    sctp_cnx->out_streams, sctp_cnx->in_streams);

            }
            break;

            default:
                if ( sctp_assoc_changed->sac_state == SCTP_SHUTDOWN_COMP) 
                    SCTP_WARN("SCTP_ASSOC_CHANGE to SSCTP_SHUTDOWN_COMP\n");
                if ( sctp_assoc_changed->sac_state == SCTP_RESTART) 
                    SCTP_WARN("SCTP_ASSOC_CHANGE to SCTP_RESTART\n");
                if ( sctp_assoc_changed->sac_state == SCTP_CANT_STR_ASSOC) 
                    SCTP_ERROR("SCTP_ASSOC_CHANGE to SCTP_CANT_STR_ASSOC\n");
                if ( sctp_assoc_changed->sac_state == SCTP_COMM_LOST) 
                    SCTP_ERROR("SCTP_ASSOC_CHANGE to SCTP_COMM_LOST\n");
                break;
            }
        }
    } else {
        sctp_cnx->messages_recv++;

        if (ntohl(sinfo.sinfo_ppid) != sctp_cnx->ppid) {
            /* Mismatch in Payload Protocol Identifier,
             * may be we received unsollicited traffic from stack other than S1AP.
             */
            SCTP_ERROR("Received data from peer with unsollicited PPID %d, expecting %d\n",
                       ntohl(sinfo.sinfo_ppid),
                       sctp_cnx->ppid);
        }

        SCTP_DEBUG("[%d][%d] Msg of length %d received, on stream %d, PPID %d\n",
                   sinfo.sinfo_assoc_id, sctp_cnx->sd, n, 
                   sinfo.sinfo_stream, ntohl(sinfo.sinfo_ppid));

        sctp_itti_send_new_message_ind(sctp_cnx->task_id,
                                       sctp_cnx->instance,
                                       sinfo.sinfo_assoc_id,
                                       buffer,
                                       n,
                                       sinfo.sinfo_stream);
    }
}

//------------------------------------------------------------------------------
void
static sctp_eNB_flush_sockets(
    struct epoll_event *events, int nb_events)
{
    int i;
    struct sctp_cnx_list_elm_s *sctp_cnx = NULL;

    for (i = 0; i < nb_events; i++) {
        sctp_cnx = sctp_get_cnx(-1, events[i].data.fd);

        if (sctp_cnx == NULL) {
            continue;
        }

        SCTP_DEBUG("Found data for descriptor %d\n", events[i].data.fd);

        if (sctp_cnx->connection_type == SCTP_TYPE_CLIENT) {
            sctp_eNB_read_from_socket(sctp_cnx);
        }
        else if (sctp_cnx->connection_type == SCTP_TYPE_MULTI_SERVER) {
            sctp_eNB_accept_associations_multi(sctp_cnx);
        }
        else {
            sctp_eNB_accept_associations(sctp_cnx);
        }
    }
}

//------------------------------------------------------------------------------
static void sctp_eNB_init(void)
{
    SCTP_DEBUG("Starting SCTP layer\n");

    STAILQ_INIT(&sctp_cnx_list);

    itti_mark_task_ready(TASK_SCTP);
}

//------------------------------------------------------------------------------
static void sctp_eNB_process_itti_msg()
{
    int                 nb_events;
    MessageDef         *received_msg = NULL;
    int                 result;

    itti_receive_msg(TASK_SCTP, &received_msg);

    /* Check if there is a packet to handle */
    if (received_msg != NULL) {
      LOG_D(SCTP,"Received message %d:%s\n",
		 ITTI_MSG_ID(received_msg), ITTI_MSG_NAME(received_msg));
        switch (ITTI_MSG_ID(received_msg)) {
        case SCTP_INIT_MSG: {
            if (sctp_create_new_listener(
                        ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                        ITTI_MSG_ORIGIN_ID(received_msg),
                        &received_msg->ittiMsg.sctp_init,0) < 0) {
                /* SCTP socket creation or bind failed... */
                SCTP_ERROR("Failed to create new SCTP listener\n");
            }
        }
        break;

        case SCTP_INIT_MSG_MULTI_REQ: {
           int multi_sd = sctp_create_new_listener(
                           ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                           ITTI_MSG_ORIGIN_ID(received_msg),
                           &received_msg->ittiMsg.sctp_init_multi,1);
            /* We received a new connection request */
            if (multi_sd < 0) {
                /* SCTP socket creation or bind failed... */
                SCTP_ERROR("Failed to create new SCTP listener\n");
            }
            sctp_itti_send_init_msg_multi_cnf(
                ITTI_MSG_ORIGIN_ID(received_msg),
                ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                multi_sd);
        }
        break;

        case SCTP_NEW_ASSOCIATION_REQ: {
            sctp_handle_new_association_req(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                            ITTI_MSG_ORIGIN_ID(received_msg),
                                            &received_msg->ittiMsg.sctp_new_association_req);
        }
        break;

        case SCTP_NEW_ASSOCIATION_REQ_MULTI: {
            sctp_handle_new_association_req_multi(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                                  ITTI_MSG_ORIGIN_ID(received_msg),
                                                  &received_msg->ittiMsg.sctp_new_association_req_multi);
        }
        break;

        case SCTP_CLOSE_ASSOCIATION:
          sctp_close_association(&received_msg->ittiMsg.sctp_close_association);
          break;

        case TERMINATE_MESSAGE:
            SCTP_WARN("*** Exiting SCTP thread\n");
            itti_exit_task();
            break;

        case SCTP_DATA_REQ: {
          sctp_send_data(&received_msg->ittiMsg.sctp_data_req);
        }
        break;

        default:
            SCTP_ERROR("Received unhandled message %d:%s\n",
                       ITTI_MSG_ID(received_msg), ITTI_MSG_NAME(received_msg));
            break;
        }

        result = itti_free(ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
        AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
        received_msg = NULL;
    }
    struct epoll_event events[20];
    nb_events = itti_get_events(TASK_SCTP, events, 20);
    /* Now handle notifications for other sockets */
    sctp_eNB_flush_sockets(events, nb_events);

    return;
}

//------------------------------------------------------------------------------
void *sctp_eNB_task(void *arg)
{
  UNUSED(arg);
    sctp_eNB_init();

    while (1) {
        sctp_eNB_process_itti_msg();
    }

    return NULL;
}
