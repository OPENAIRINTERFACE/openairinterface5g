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

/*! \file sctp_common.c
 *  \brief eNB/MME SCTP related common procedures
 *  \author Sebastien ROUX
 *  \date 2013
 *  \version 1.0
 *  @ingroup _sctp
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/sctp.h>

#include "sctp_common.h"

/* Pre-bind socket options configuration.
 * See http://linux.die.net/man/7/sctp for more informations on these options.
 */
int sctp_set_init_opt(int sd, uint16_t instreams, uint16_t outstreams,
                      uint16_t max_attempts, uint16_t init_timeout)
{
  int on = 1;
  struct sctp_initmsg init;

  memset((void *)&init, 0, sizeof(struct sctp_initmsg));

  /* Request a number of streams */
  init.sinit_num_ostreams   = outstreams;
  init.sinit_max_instreams  = instreams;
  init.sinit_max_attempts   = max_attempts;
  init.sinit_max_init_timeo = init_timeout;

  if (setsockopt(sd, IPPROTO_SCTP, SCTP_INITMSG, &init, sizeof(struct sctp_initmsg)) < 0) {
    SCTP_ERROR("setsockopt: %d:%s\n", errno, strerror(errno));
    close(sd);
    return -1;
  }

  /* Allow socket reuse */
  if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
    SCTP_ERROR("setsockopt SO_REUSEADDR failed (%d:%s)\n", errno, strerror(errno));
    close(sd);
    return -1;
  }

  return 0;
}

int sctp_get_sockinfo(int sock, uint16_t *instream, uint16_t *outstream,
                      int32_t *assoc_id)
{
  socklen_t i;
  struct sctp_status status;

  if (socket <= 0) {
    return -1;
  }

  memset(&status, 0, sizeof(struct sctp_status));
  i = sizeof(struct sctp_status);

  if (getsockopt(sock, IPPROTO_SCTP, SCTP_STATUS, &status, &i) < 0) {
    SCTP_ERROR("Getsockopt SCTP_STATUS failed: %s\n", strerror(errno));
    return -1;
  }

  SCTP_DEBUG("----------------------\n");
  SCTP_DEBUG("SCTP Status:\n");
  SCTP_DEBUG("assoc id .....: %u\n", status.sstat_assoc_id);
  SCTP_DEBUG("state ........: %d\n", status.sstat_state);
  SCTP_DEBUG("instrms ......: %u\n", status.sstat_instrms);
  SCTP_DEBUG("outstrms .....: %u\n", status.sstat_outstrms);
  SCTP_DEBUG("fragmentation : %u\n", status.sstat_fragmentation_point);
  SCTP_DEBUG("pending data .: %u\n", status.sstat_penddata);
  SCTP_DEBUG("unack data ...: %u\n", status.sstat_unackdata);
  SCTP_DEBUG("rwnd .........: %u\n", status.sstat_rwnd);
  SCTP_DEBUG("peer info     :\n");
  SCTP_DEBUG("    state ....: %u\n", status.sstat_primary.spinfo_state);
  SCTP_DEBUG("    cwnd .....: %u\n", status.sstat_primary.spinfo_cwnd);
  SCTP_DEBUG("    srtt .....: %u\n" , status.sstat_primary.spinfo_srtt);
  SCTP_DEBUG("    rto ......: %u\n" , status.sstat_primary.spinfo_rto);
  SCTP_DEBUG("    mtu ......: %u\n" , status.sstat_primary.spinfo_mtu);
  SCTP_DEBUG("----------------------\n");

  if (instream != NULL) {
    *instream = status.sstat_instrms;
  }

  if (outstream != NULL) {
    *outstream = status.sstat_outstrms;
  }

  if (assoc_id != NULL) {
    *assoc_id = status.sstat_assoc_id;
  }

  return 0;
}

int sctp_get_peeraddresses(int sock, struct sockaddr **remote_addr, int *nb_remote_addresses)
{
  int nb, j;
  struct sockaddr *temp_addr_p;

  if ((nb = sctp_getpaddrs(sock, -1, &temp_addr_p)) <= 0) {
    SCTP_ERROR("Failed to retrieve peer addresses\n");
    return -1;
  }

  SCTP_DEBUG("----------------------\n");
  SCTP_DEBUG("Peer addresses:\n");

  for (j = 0; j < nb; j++) {
    if (temp_addr_p[j].sa_family == AF_INET) {
      char address[16];
      struct sockaddr_in *addr;

      memset(&address, 0, sizeof(address));

      addr = (struct sockaddr_in*)&temp_addr_p[j];

      if (inet_ntop(AF_INET, &addr->sin_addr, address, sizeof(address)) != NULL) {
        SCTP_DEBUG("    - [%s]\n", address);
      }
    } else {
      struct sockaddr_in6 *addr;
      char address[40];

      addr = (struct sockaddr_in6*)&temp_addr_p[j];

      memset(&address, 0, sizeof(address));

      if (inet_ntop(AF_INET6, &addr->sin6_addr.s6_addr, address, sizeof(address)) != NULL) {
        SCTP_DEBUG("    - [%s]\n", address);
      }
    }
  }

  SCTP_DEBUG("----------------------\n");

  if (remote_addr != NULL && nb_remote_addresses != NULL) {
    *nb_remote_addresses = nb;
    *remote_addr = temp_addr_p;
  } else {
    /* We can destroy buffer */
    sctp_freepaddrs((struct sockaddr*)temp_addr_p);
  }

  return 0;
}

int sctp_get_localaddresses(int sock, struct sockaddr **local_addr, int *nb_local_addresses)
{
  int nb, j;
  struct sockaddr *temp_addr_p;

  if ((nb = sctp_getladdrs(sock, -1, &temp_addr_p)) <= 0) {
    SCTP_ERROR("Failed to retrieve local addresses\n");
    return -1;
  }

  SCTP_DEBUG("----------------------\n");
  SCTP_DEBUG("Local addresses:\n");

  for (j = 0; j < nb; j++) {
    if (temp_addr_p[j].sa_family == AF_INET) {
      char address[16];
      struct sockaddr_in *addr;

      memset(address, 0, sizeof(address));

      addr = (struct sockaddr_in*)&temp_addr_p[j];

      if (inet_ntop(AF_INET, &addr->sin_addr, address, sizeof(address)) != NULL) {
        SCTP_DEBUG("    - [%s]\n", address);
      }
    } else {
      struct sockaddr_in6 *addr;
      char address[40];

      addr = (struct sockaddr_in6*)&temp_addr_p[j];

      memset(address, 0, sizeof(address));

      if (inet_ntop(AF_INET6, &addr->sin6_addr.s6_addr, address, sizeof(address)) != NULL) {
        SCTP_DEBUG("    - [%s]\n", address);
      }
    }
  }

  SCTP_DEBUG("----------------------\n");

  if (local_addr != NULL && nb_local_addresses != NULL) {
    *nb_local_addresses = nb;
    *local_addr = temp_addr_p;
  } else {
    /* We can destroy buffer */
    sctp_freeladdrs((struct sockaddr*)temp_addr_p);
  }

  return 0;
}

