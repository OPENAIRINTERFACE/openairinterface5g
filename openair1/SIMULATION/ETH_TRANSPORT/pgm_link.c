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

/*! \file pgm_link.c
 *  \brief implementation a warpper for openpgm for reliable multicast transmission
 *  \author Navid Nikaein and S. Roux
 *  \date 2013 - 2014
 *  \version 1.0
 *  \company Eurecom
 *  \email: navid.nikaein@eurecom.fr
 */

#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <arpa/inet.h>

#if defined(ENABLE_PGM_TRANSPORT)

#include <pgm/pgm.h>

#include "assertions.h"

#include "pgm_link.h"
#include "multicast_link.h"

#include "UTIL/OCG/OCG.h"
#include "UTIL/OCG/OCG_extern.h"

#include "UTIL/LOG/log.h"

// #define ENABLE_PGM_DEBUG

typedef struct {
  pgm_sock_t *sock;
  uint16_t port;
  uint8_t  rx_buffer[40000];
} pgm_multicast_group_t;

pgm_multicast_group_t pgm_multicast_group[MULTICAST_LINK_NUM_GROUPS];

static
int pgm_create_socket(int index, const char *if_addr);

unsigned int pgm_would_block = 1;

#if defined(ENABLE_PGM_DEBUG)
static void
log_handler (
  const int         log_level,
  const char*       message,
  void*             closure
)
{
  printf("%s\n", message);
}
#endif

int pgm_oai_init(char *if_addr)
{
  pgm_error_t* pgm_err = NULL;

  memset(pgm_multicast_group, 0,
         MULTICAST_LINK_NUM_GROUPS * sizeof(pgm_multicast_group_t));

#if defined(ENABLE_PGM_DEBUG)
  pgm_messages_init();
  pgm_min_log_level = PGM_LOG_LEVEL_DEBUG;
  pgm_log_mask = 0xFFF;

  pgm_log_set_handler(log_handler, NULL);
#endif

  if (!pgm_init(&pgm_err)) {
    LOG_E(EMU, "Unable to start PGM engine: %s\n", pgm_err->message);
    pgm_error_free (pgm_err);
    exit(EXIT_FAILURE);
  }

  return pgm_create_socket(oai_emulation.info.multicast_group, if_addr);
}

int pgm_recv_msg(int group, uint8_t *buffer, uint32_t length,
                 unsigned int frame, unsigned int next_slot)
{
  size_t                num_bytes = 0;
  int                   status    = 0;
  pgm_error_t*          pgm_err   = NULL;
  struct pgm_sockaddr_t from;
  socklen_t             fromlen   = sizeof(from);
  uint32_t              timeout   = 0;
  int                   flags     = 0;

  if (pgm_would_block == 0) {
    flags = MSG_DONTWAIT;
  }

  DevCheck((group <= MULTICAST_LINK_NUM_GROUPS) && (group >= 0),
           group, MULTICAST_LINK_NUM_GROUPS, 0);

#ifdef DEBUG_EMU
  LOG_I(EMU, "[PGM] Entering recv function for group %d\n", group);
#endif

  do {
    status = pgm_recvfrom(pgm_multicast_group[group].sock,
                          buffer,
                          length,
                          flags,
                          &num_bytes,
                          &from,
                          &fromlen,
                          &pgm_err);

    if (PGM_IO_STATUS_NORMAL == status) {
#ifdef DEBUG_EMU
      LOG_D(EMU, "[PGM] Received %d bytes for group %d\n", num_bytes, group);
#endif
      return num_bytes;
    } else if (PGM_IO_STATUS_TIMER_PENDING == status) {
      if (pgm_would_block == 0) {
        /* We sleep for 50 usec */
        usleep(50);

        timeout ++;

        if (timeout == (1000000 / 50)) {
          LOG_W(EMU, "[PGM] A packet has been lost -> ask for retransmit\n");
          /* If we do not receive a packet after 10000usec
          * -> send a NACK */
          bypass_tx_nack(frame, next_slot);
          timeout = 0;
        }
      }
    } else if (PGM_IO_STATUS_RESET == status) {
      LOG_W(EMU, "[PGM] Got session reset\n");
    } else {
#ifdef DEBUG_EMU
      LOG_D(EMU, "[PGM] Got status %d\n", status);
#endif

      if (pgm_err) {
        LOG_E(EMU, "[PGM] recvform failed: %s", pgm_err->message);
        pgm_error_free (pgm_err);
        pgm_err = NULL;
      }
    }
  } while(status != PGM_IO_STATUS_NORMAL);

  return -1;
}

int pgm_link_send_msg(int group, uint8_t *data, uint32_t len)
{
  int status;
  size_t bytes_written = 0;

  do {
    status = pgm_send(pgm_multicast_group[group].sock, data, len, &bytes_written);
  } while(status == PGM_IO_STATUS_WOULD_BLOCK);

  if (status != PGM_IO_STATUS_NORMAL) {
    return -1;
  }

  return bytes_written;
}

static
int pgm_create_socket(int index, const char *if_addr)
{
  struct pgm_addrinfo_t* res = NULL;
  pgm_error_t*          pgm_err        = NULL;
  sa_family_t           sa_family      = AF_INET;
  int                   udp_encap_port = 46014 + index;
  int                   max_tpdu       = 1500;
  int                   sqns           = 100;
  int                   port           = 0;
  struct pgm_sockaddr_t addr;
  int                   blocking       = 1;
  int                   multicast_loop = 0;
  int                   multicast_hops = 0;
  int                   dscp, i;

  port = udp_encap_port;

  /* Use PGM */
  udp_encap_port = 0;

  LOG_D(EMU, "[PGM] Preparing socket for group %d and address %s\n",
        index, if_addr);

  if (!pgm_getaddrinfo(if_addr, NULL, &res, &pgm_err)) {
    LOG_E(EMU, "Parsing network parameter: %s\n", pgm_err->message);
    goto err_abort;
  }

  if (udp_encap_port) {
    LOG_I(EMU, "[PGM] Creating PGM/UDP socket for encapsulated port %d\n",
          udp_encap_port);

    if (!pgm_socket (&pgm_multicast_group[index].sock, sa_family,
                     SOCK_SEQPACKET, IPPROTO_UDP, &pgm_err)) {
      LOG_E(EMU, "[PGM] Socket: %s\n", pgm_err->message);
      goto err_abort;
    }

    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_UDP_ENCAP_UCAST_PORT, &udp_encap_port,
                   sizeof(udp_encap_port));
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_UDP_ENCAP_MCAST_PORT, &udp_encap_port,
                   sizeof(udp_encap_port));
  } else {
    LOG_I(EMU, "[PGM] Creating PGM/IP socket\n");

    if (!pgm_socket(&pgm_multicast_group[index].sock, sa_family,
                    SOCK_SEQPACKET, IPPROTO_PGM, &pgm_err)) {
      LOG_E(EMU, "Creating PGM/IP socket: %s\n", pgm_err->message);
      goto err_abort;
    }
  }

  {
    /* Use RFC 2113 tagging for PGM Router Assist */
    const int no_router_assist = 0;
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_IP_ROUTER_ALERT, &no_router_assist,
                   sizeof(no_router_assist));
  }

  //     pgm_drop_superuser();

  {
    /* set PGM parameters */
    const int recv_only = 0,
              passive = 0,
              peer_expiry = pgm_secs (300),
              spmr_expiry = pgm_msecs (250),
              nak_bo_ivl = pgm_msecs (10),
              nak_rpt_ivl = pgm_secs (2),
              nak_rdata_ivl = pgm_secs (2),
              nak_data_retries = 50,
              nak_ncf_retries = 50,
              ambient_spm = pgm_secs(30);
    const int heartbeat_spm[] = {
      pgm_msecs (100),
      pgm_msecs (100),
      pgm_msecs (100),
      pgm_msecs (100),
      pgm_msecs (1300),
      pgm_secs  (7),
      pgm_secs  (16),
      pgm_secs  (25),
      pgm_secs  (30)
    };

    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_RECV_ONLY, &recv_only, sizeof(recv_only));
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_PASSIVE, &passive, sizeof(passive));
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_MTU, &max_tpdu, sizeof(max_tpdu));
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_RXW_SQNS, &sqns, sizeof(sqns));
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_PEER_EXPIRY, &peer_expiry, sizeof(peer_expiry));
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_SPMR_EXPIRY, &spmr_expiry, sizeof(spmr_expiry));
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_NAK_BO_IVL, &nak_bo_ivl, sizeof(nak_bo_ivl));
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_NAK_RPT_IVL, &nak_rpt_ivl, sizeof(nak_rpt_ivl));
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_NAK_RDATA_IVL, &nak_rdata_ivl, sizeof(nak_rdata_ivl));
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_NAK_DATA_RETRIES, &nak_data_retries, sizeof(nak_data_retries));
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_NAK_NCF_RETRIES, &nak_ncf_retries, sizeof(nak_ncf_retries));
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_AMBIENT_SPM, &ambient_spm, sizeof(ambient_spm));
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_HEARTBEAT_SPM, &heartbeat_spm, sizeof(heartbeat_spm));
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_TXW_SQNS, &sqns, sizeof(sqns));
  }

  /* create global session identifier */
  memset (&addr, 0, sizeof(addr));
  /* sa_port should be in host byte order */
  addr.sa_port = port;
  addr.sa_addr.sport = DEFAULT_DATA_SOURCE_PORT + index;

  if (!pgm_gsi_create_from_hostname(&addr.sa_addr.gsi, &pgm_err)) {
    LOG_E(EMU, "[PGM] Creating GSI: %s\n", pgm_err->message);
    goto err_abort;
  }

  LOG_D(EMU, "[PGM] Created GSI %s\n", pgm_tsi_print(&addr.sa_addr));

  /* assign socket to specified address */
  {
    struct pgm_interface_req_t if_req;
    memset (&if_req, 0, sizeof(if_req));
    if_req.ir_interface = res->ai_recv_addrs[0].gsr_interface;
    if_req.ir_scope_id  = 0;

    if (AF_INET6 == sa_family) {
      struct sockaddr_in6 sa6;
      memcpy (&sa6, &res->ai_recv_addrs[0].gsr_group, sizeof(sa6));
      if_req.ir_scope_id = sa6.sin6_scope_id;
    }

    if (!pgm_bind3(pgm_multicast_group[index].sock, &addr, sizeof(addr),
                   &if_req, sizeof(if_req),        /* tx interface */
                   &if_req, sizeof(if_req),        /* rx interface */
                   &pgm_err)) {
      LOG_E(EMU, "[PGM] Error: %s\n", pgm_err->message);
      goto err_abort;
    }
  }

  /* join IP multicast groups */
  {
    struct group_req req;
    struct sockaddr_in addr_in;

    memset(&req, 0, sizeof(req));

    /* Interface index */
    req.gr_interface = res->ai_recv_addrs[0].gsr_interface;

    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(port);

    for (i = 0; i < MULTICAST_LINK_NUM_GROUPS; i++) {
      addr_in.sin_addr.s_addr = inet_addr(multicast_group_list[i]);
      memcpy(&req.gr_group, &addr_in, sizeof(addr_in));

      pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                     PGM_JOIN_GROUP, &req,
                     sizeof(struct group_req));
    }

    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                   PGM_SEND_GROUP, &req,
                   sizeof(struct group_req));
  }

  pgm_freeaddrinfo(res);
  res = NULL;

  /* set IP parameters */
  multicast_hops = 64;
  dscp = 0x2e << 2;             /* Expedited Forwarding PHB for network elements, no ECN. */

  pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                 PGM_MULTICAST_LOOP, &multicast_loop, sizeof(multicast_loop));
  pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM,
                 PGM_MULTICAST_HOPS, &multicast_hops, sizeof(multicast_hops));

  if (AF_INET6 != sa_family)
    pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM, PGM_TOS,
                   &dscp, sizeof(dscp));

  pgm_setsockopt(pgm_multicast_group[index].sock, IPPROTO_PGM, PGM_NOBLOCK,
                 &blocking, sizeof(blocking));

  if (!pgm_connect(pgm_multicast_group[index].sock, &pgm_err)) {
    LOG_E(EMU, "[PGM] Connecting socket: %s\n", pgm_err->message);
    goto err_abort;
  }

  return 0;

err_abort:

  if (NULL != pgm_multicast_group[index].sock) {
    pgm_close(pgm_multicast_group[index].sock, FALSE);
    pgm_multicast_group[index].sock = NULL;
  }

  if (NULL != res) {
    pgm_freeaddrinfo(res);
    res = NULL;
  }

  if (NULL != pgm_err) {
    pgm_error_free(pgm_err);
    pgm_err = NULL;
  }

  exit(EXIT_FAILURE);
}

#endif
