/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "vnf_p7.h"

nfapi_vnf_p7_config_t *nfapi_vnf_p7_config_create()
{
  vnf_p7_t *_this = (vnf_p7_t *)calloc(1, sizeof(vnf_p7_t));

  if (_this == 0)
    return 0;

  _this->_public.segment_size = 65000;
  _this->_public.max_num_segments = 8;
  _this->_public.checksum_enabled = 1;

  _this->_public.malloc = &malloc;
  _this->_public.free = &free;

  _this->_public.codec_config.allocate = &malloc;
  _this->_public.codec_config.deallocate = &free;

  return (nfapi_vnf_p7_config_t *)_this;
}

void nfapi_vnf_p7_config_destory(nfapi_vnf_p7_config_t *config)
{
  free(config);
}

static struct timespec timespec_add(struct timespec lhs, struct timespec rhs)
{
  struct timespec result;

  result.tv_sec = lhs.tv_sec + rhs.tv_sec;
  result.tv_nsec = lhs.tv_nsec + rhs.tv_nsec;

  if (result.tv_nsec > 1e9) {
    result.tv_sec++;
    result.tv_nsec -= 1e9;
  }

  return result;
}

static struct timespec timespec_sub(struct timespec lhs, struct timespec rhs)
{
  struct timespec result;
  if ((lhs.tv_nsec - rhs.tv_nsec) < 0) {
    result.tv_sec = lhs.tv_sec - rhs.tv_sec - 1;
    result.tv_nsec = 1000000000 + lhs.tv_nsec - rhs.tv_nsec;
  } else {
    result.tv_sec = lhs.tv_sec - rhs.tv_sec;
    result.tv_nsec = lhs.tv_nsec - rhs.tv_nsec;
  }
  return result;
}

int nfapi_vnf_p7_start(nfapi_vnf_p7_config_t *config)
{
  if (config == 0)
    return -1;

  NFAPI_TRACE(NFAPI_TRACE_INFO, "%s()\n", __FUNCTION__);

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;

  NFAPI_TRACE(NFAPI_TRACE_INFO, "Initialising VNF P7 port:%u\n", config->port);

  // open the UDP socket
  if ((vnf_p7->socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "After P7 socket errno: %d\n", errno);
    return -1;
  }

  NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF P7 socket created...\n");

  // configure the UDP socket options
  int iptos_value = 0;
  if (setsockopt(vnf_p7->socket, IPPROTO_IP, IP_TOS, &iptos_value, sizeof(iptos_value)) < 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt (IP_TOS) errno: %d\n", errno);
    return -1;
  }

  NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF P7 setsockopt succeeded...\n");

  // Create the address structure
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(config->port);
  addr.sin_addr.s_addr = INADDR_ANY;

  // bind to the configured port
  NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF P7 binding too %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
  if (bind(vnf_p7->socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "After bind errno: %d\n", errno);
    return -1;
  }

  NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF P7 bind succeeded...\n");

  struct timespec pselect_timeout;
  pselect_timeout.tv_sec = 0;
  pselect_timeout.tv_nsec = 1000000;

  struct timespec pselect_start;
  struct timespec pselect_stop;

  long last_millisecond = -1;

  struct timespec sf_duration;
  sf_duration.tv_sec = 0;
  sf_duration.tv_nsec = 1e6;

  struct timespec sf_start;
  clock_gettime(CLOCK_MONOTONIC, &sf_start);
  long millisecond = sf_start.tv_nsec / 1e6;
  sf_start = timespec_add(sf_start, sf_duration);
  NFAPI_TRACE(NFAPI_TRACE_INFO, "next subframe will start at %ld.%ld\n", sf_start.tv_sec, sf_start.tv_nsec);

  while (vnf_p7->terminate == 0) {
    fd_set rfds;
    int maxSock = 0;
    FD_ZERO(&rfds);
    int selectRetval = 0;

    // Add the p7 socket
    FD_SET(vnf_p7->socket, &rfds);
    maxSock = vnf_p7->socket;

    clock_gettime(CLOCK_MONOTONIC, &pselect_start);

    if ((last_millisecond == -1) || (millisecond == last_millisecond) || (millisecond == (last_millisecond + 1) % 1000)) {
      if ((pselect_start.tv_sec > sf_start.tv_sec)
          || ((pselect_start.tv_sec == sf_start.tv_sec) && (pselect_start.tv_nsec > sf_start.tv_nsec))) {
        // overran the end of the subframe we do not want to wait
        pselect_timeout.tv_sec = 0;
        pselect_timeout.tv_nsec = 0;
      } else {
        // still time before the end of the subframe wait
        pselect_timeout = timespec_sub(sf_start, pselect_start);
      }

      selectRetval = pselect(maxSock + 1, &rfds, NULL, NULL, &pselect_timeout, NULL);

      clock_gettime(CLOCK_MONOTONIC, &pselect_stop);
      (void)pselect_stop;

      nfapi_vnf_p7_connection_info_t *phy = vnf_p7->p7_connections;

      if (selectRetval == -1 && errno == 22) {
        NFAPI_TRACE(NFAPI_TRACE_ERROR,
                    "INVAL: pselect_timeout:%ld.%ld adj[dur:%d adj:%d], sf_dur:%ld.%ld\n",
                    pselect_timeout.tv_sec,
                    pselect_timeout.tv_nsec,
                    phy->insync_minor_adjustment_duration,
                    phy->insync_minor_adjustment,
                    sf_duration.tv_sec,
                    sf_duration.tv_nsec);
      }
      if (selectRetval == 0) {
        // calculate the start of the next subframe
        sf_start = timespec_add(sf_start, sf_duration);

        if (phy && phy->in_sync && phy->insync_minor_adjustment != 0 && phy->insync_minor_adjustment_duration > 0) {
          long insync_minor_adjustment_ns = (phy->insync_minor_adjustment * 1000);

          sf_start.tv_nsec -= insync_minor_adjustment_ns;

          if (sf_start.tv_nsec > 1e9) {
            sf_start.tv_sec++;
            sf_start.tv_nsec -= 1e9;
          } else if (sf_start.tv_nsec < 0) {
            sf_start.tv_sec--;
            sf_start.tv_nsec += 1e9;
          }

          phy->insync_minor_adjustment_duration--;

          NFAPI_TRACE(NFAPI_TRACE_NOTE,
                      "[VNF] AFTER adjustment - Subframe minor adjustment %dus sf_start.tv_nsec:%ld duration:%u\n",
                      phy->insync_minor_adjustment,
                      sf_start.tv_nsec,
                      phy->insync_minor_adjustment_duration);

          if (phy->insync_minor_adjustment_duration == 0) {
            phy->insync_minor_adjustment = 0;
          }
        }

        millisecond++;
      }
    } else {
      // we have overrun the subframe advance to go and collect $200
      if ((millisecond - last_millisecond) > 3)
        NFAPI_TRACE(NFAPI_TRACE_WARN,
                    "subframe overrun %ld %ld (%ld)\n",
                    millisecond,
                    last_millisecond,
                    millisecond - last_millisecond + 1);

      last_millisecond = (last_millisecond + 1) % 1000;
      selectRetval = 0;
    }

    if (selectRetval == 0) {
      vnf_p7->sf_start_time_hr = vnf_get_current_time_hr();

      // pselect timed out
      nfapi_vnf_p7_connection_info_t *curr = vnf_p7->p7_connections;

      while (curr != 0) {
        curr->sfn_sf = increment_sfn_sf(curr->sfn_sf);
        vnf_sync(vnf_p7, curr);
        curr = curr->next;
      }

      send_mac_subframe_indications(vnf_p7);

    } else if (selectRetval > 0) {
      // have a p7 message
      if (FD_ISSET(vnf_p7->socket, &rfds)) {
        vnf_p7_read_dispatch_message(vnf_p7);
      }
    } else {
      // pselect error
      if (selectRetval == -1 && errno == EINTR) {
        // a signal was received
      } else {
        NFAPI_TRACE(NFAPI_TRACE_INFO,
                    "P7 select failed result %d errno %d timeout:%ld.%ld last_ms:%ld ms:%ld\n",
                    selectRetval,
                    errno,
                    pselect_timeout.tv_sec,
                    pselect_timeout.tv_nsec,
                    last_millisecond,
                    millisecond);
        if (selectRetval == -1 && errno == 22) {
          usleep(100000);
        }
      }
    }
  }

  NFAPI_TRACE(NFAPI_TRACE_INFO, "Closing p7 socket\n");
  close(vnf_p7->socket);

  NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() returning\n", __FUNCTION__);

  return 0;
}

int nfapi_vnf_p7_stop(nfapi_vnf_p7_config_t *config)
{
  if (config == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  vnf_p7->terminate = 1;
  return 0;
}

int nfapi_vnf_p7_add_pnf(nfapi_vnf_p7_config_t *config, const char *pnf_p7_addr, int pnf_p7_port, int phy_id, int mu)
{
  NFAPI_TRACE(NFAPI_TRACE_INFO,
              "%s(config:%p phy_id:%d pnf_addr:%s pnf_p7_port:%d)\n",
              __FUNCTION__,
              config,
              phy_id,
              pnf_p7_addr,
              pnf_p7_port);

  if (config == 0) {
    return -1;
  }

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;

  nfapi_vnf_p7_connection_info_t *node = (nfapi_vnf_p7_connection_info_t *)malloc(sizeof(nfapi_vnf_p7_connection_info_t));

  memset(node, 0, sizeof(nfapi_vnf_p7_connection_info_t));
  node->phy_id = phy_id;
  node->in_sync = 0;
  node->dl_out_sync_offset = 30;
  node->dl_out_sync_period = 10;
  node->dl_in_sync_offset = 30;
  node->dl_in_sync_period = 512;
  node->sfn = 0;
  node->slot = 0;
  node->min_sync_cycle_count = 8;
  node->mu = mu;
#ifndef ENABLE_AERIAL
  // save the remote endpoint information
  node->remote_addr.sin_family = AF_INET;
  node->remote_addr.sin_port = pnf_p7_port;
  node->remote_addr.sin_addr.s_addr = inet_addr(pnf_p7_addr);
#endif
  vnf_p7_connection_info_list_add(vnf_p7, node);

  return 0;
}

int nfapi_vnf_p7_del_pnf(nfapi_vnf_p7_config_t *config, int phy_id)
{
  NFAPI_TRACE(NFAPI_TRACE_INFO, "%s(phy_id:%d)\n", __FUNCTION__, phy_id);

  if (config == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;

  nfapi_vnf_p7_connection_info_t *to_delete = vnf_p7_connection_info_list_delete(vnf_p7, phy_id);

  if (to_delete) {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "%s(phy_id:%d) deleting connection info\n", __FUNCTION__, phy_id);
    free(to_delete);
  }

  return 0;
}

int nfapi_vnf_p7_release_msg(nfapi_vnf_p7_config_t *config, nfapi_p7_message_header_t *header)
{
  if (config == 0 || header == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  vnf_p7_release_msg(vnf_p7, header);

  return 0;
}

int nfapi_vnf_p7_release_pdu(nfapi_vnf_p7_config_t *config, void *pdu)
{
  if (config == 0 || pdu == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  vnf_p7_release_pdu(vnf_p7, pdu);

  return 0;
}
