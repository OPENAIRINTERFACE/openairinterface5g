/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>

#include "vnf.h"

nfapi_vnf_config_t *nfapi_vnf_config_create()
{
  vnf_t *_this = (vnf_t *)calloc(1, sizeof(vnf_t));

  if (_this == 0)
    return 0;

  _this->sctp = 1;

  _this->next_phy_id = 1;

  // Set the default P5 port
  _this->_public.vnf_p5_port = NFAPI_P5_SCTP_PORT;

  // set the default memory allocation
  _this->_public.malloc = &malloc;
  _this->_public.free = &free;

  // set the default memory allocation
  _this->_public.codec_config.allocate = &malloc;
  _this->_public.codec_config.deallocate = &free;

  return (nfapi_vnf_config_t *)_this;
}

void nfapi_vnf_config_destory(nfapi_vnf_config_t *config)
{
  free(config);
}

int nfapi_vnf_start(nfapi_vnf_config_t *config)
{
  assert(config != 0);
  NFAPI_TRACE(NFAPI_TRACE_INFO, "%s()\n", __FUNCTION__);

  int p5ListenSock, p5Sock;

  struct sockaddr_in addr;
  socklen_t addrSize;

  struct sockaddr_in6 addr6;

  struct sctp_event_subscribe events;
  struct sctp_initmsg initMsg;
  int noDelay;

  (void)memset(&addr, 0, sizeof(struct sockaddr_in));
  (void)memset(&addr6, 0, sizeof(struct sockaddr_in6));
  (void)memset(&events, 0, sizeof(struct sctp_event_subscribe));
  (void)memset(&initMsg, 0, sizeof(struct sctp_initmsg));

  vnf_t *vnf = (vnf_t *)(config);

  NFAPI_TRACE(NFAPI_TRACE_INFO, "Starting P5 VNF connection on port %u\n", config->vnf_p5_port);

  {
    int protocol;
    int domain;

    if (vnf->sctp)
      protocol = IPPROTO_SCTP;
    else
      protocol = IPPROTO_IP;

    if (config->vnf_ipv6) {
      domain = PF_INET6;
    } else {
      domain = AF_INET;
    }

    // open the SCTP socket
    if ((p5ListenSock = socket(domain, SOCK_STREAM, protocol)) < 0) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "After P5 socket errno: %d\n", errno);
      return 0;
    }
    NFAPI_TRACE(NFAPI_TRACE_INFO, "P5 socket created... %d\n", p5ListenSock);
  }

  if (vnf->sctp) {
    // configure for MSG_NOTIFICATION
    if (setsockopt(p5ListenSock, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(struct sctp_event_subscribe)) < 0) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt (SCTP_EVENTS) errno: %d\n", errno);
      close(p5ListenSock);
      return 0;
    }
    NFAPI_TRACE(NFAPI_TRACE_NOTE, "VNF Setting the SCTP_INITMSG\n");
    // configure the SCTP socket options
    initMsg.sinit_num_ostreams = 5;
    initMsg.sinit_max_instreams = 5;
    if (setsockopt(p5ListenSock, IPPROTO_SCTP, SCTP_INITMSG, &initMsg, sizeof(initMsg)) < 0) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt (SCTP_INITMSG) errno: %d\n", errno);
      close(p5ListenSock);
      return 0;
    }
    noDelay = 1;
    if (setsockopt(p5ListenSock, IPPROTO_SCTP, SCTP_NODELAY, &noDelay, sizeof(noDelay)) < 0) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt (STCP_NODELAY) errno: %d\n", errno);
      close(p5ListenSock);
      return 0;
    }
    struct sctp_event_subscribe sctp_events;
    memset((void *)&sctp_events, 0, sizeof(sctp_events));
    sctp_events.sctp_data_io_event = 1;

    if (setsockopt(p5ListenSock, SOL_SCTP, SCTP_EVENTS, (const void *)&sctp_events, sizeof(sctp_events)) < 0) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "After setsockopt errno: %d\n", errno);
      close(p5ListenSock);
      return -1;
    }
  }

  if (config->vnf_ipv6) {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "IPV6 binding to port %d %d\n", config->vnf_p5_port, p5ListenSock);
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(config->vnf_p5_port);
    addr6.sin6_addr = in6addr_any;

    // bind to the configured address and port
    if (bind(p5ListenSock, (struct sockaddr *)&addr6, sizeof(struct sockaddr_in6)) < 0) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "After bind errno: %d\n", errno);
      close(p5ListenSock);
      return 0;
    }
  } else if (config->vnf_ipv4) {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "IPV4 binding to port %d\n", config->vnf_p5_port);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config->vnf_p5_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    // bind to the configured address and port
    if (bind(p5ListenSock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "After bind errno: %d\n", errno);
      close(p5ListenSock);
      return 0;
    }
  }

  NFAPI_TRACE(NFAPI_TRACE_INFO, "bind succeeded..%d.\n", p5ListenSock);

  // put the socket into listen mode
  if (listen(p5ListenSock, 2) < 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "After listen errno: %d\n", errno);
    close(p5ListenSock);
    return 0;
  }

  NFAPI_TRACE(NFAPI_TRACE_INFO, "listen succeeded...\n");

  struct timeval tv;
  fd_set read_fd_set;

  int p5_idx = 0;
  while (vnf->terminate == 0) {
    FD_ZERO(&read_fd_set);

    FD_SET(p5ListenSock, &read_fd_set);
    int max_fd = p5ListenSock;

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    nfapi_vnf_pnf_info_t *pnf = config->pnf_list;
    while (pnf != 0) {
      if (pnf->connected) {
        FD_SET(pnf->p5_sock, &read_fd_set);
        if (pnf->p5_sock > max_fd) {
          max_fd = pnf->p5_sock;
        }
      }

      pnf = pnf->next;
    }

    int select_result = select(max_fd + 1, &read_fd_set, 0, 0, &tv);

    if (select_result == -1) {
      NFAPI_TRACE(NFAPI_TRACE_INFO, "select result %d errno %d\n", select_result, errno);
      close(p5ListenSock);
      return 0;
    } else if (select_result) {
      if (FD_ISSET(p5ListenSock, &read_fd_set)) {
        addrSize = sizeof(struct sockaddr_in);
        NFAPI_TRACE(NFAPI_TRACE_INFO, "Accepting connection from PNF...\n");

        p5Sock = accept(p5ListenSock, (struct sockaddr *)&addr, &addrSize);

        if (p5Sock < 0) {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to accept PNF connection reason:%d\n", errno);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_INFO,
                      "PNF connection (fd:%d) accepted from %s:%d \n",
                      p5Sock,
                      inet_ntoa(addr.sin_addr),
                      ntohs(addr.sin_port));
          nfapi_vnf_pnf_info_t *new_pnf = (nfapi_vnf_pnf_info_t *)malloc(sizeof(nfapi_vnf_pnf_info_t));
          NFAPI_TRACE(NFAPI_TRACE_INFO, "MALLOC nfapi_vnf_pnf_info_t for pnf_list pnf:%p\n", new_pnf);
          memset(new_pnf, 0, sizeof(nfapi_vnf_pnf_info_t));
          new_pnf->p5_sock = p5Sock;
          new_pnf->p5_idx = p5_idx++;
          new_pnf->p5_pnf_sockaddr = addr;
          new_pnf->connected = 1;

          nfapi_vnf_pnf_list_add(config, new_pnf);

          if (config->pnf_connection_indication != 0) {
            (config->pnf_connection_indication)(config, new_pnf->p5_idx);
          }

          // check the connection status
          {
            struct sctp_status status;
            (void)memset(&status, 0, sizeof(struct sctp_status));
            socklen_t optLen = (socklen_t)sizeof(struct sctp_status);
            if (getsockopt(p5Sock, IPPROTO_SCTP, SCTP_STATUS, &status, &optLen) < 0) {
              NFAPI_TRACE(NFAPI_TRACE_ERROR, "After getsockopt errno: %d\n", errno);
              return -1;
            } else {
              NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Association ID = %d\n", status.sstat_assoc_id);
              NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Receiver window size = %d\n", status.sstat_rwnd);
              NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF In Streams = %d\n", status.sstat_instrms);
              NFAPI_TRACE(NFAPI_TRACE_INFO, "VNF Out Streams = %d\n", status.sstat_outstrms);
            }
          }
        }
      } else {
        uint8_t delete_pnfs = 0;

        nfapi_vnf_pnf_info_t *pnf_iter = config->pnf_list;
        while (pnf_iter != 0) {
          if (FD_ISSET(pnf_iter->p5_sock, &read_fd_set)) {
            if (vnf_read_dispatch_message(config, pnf_iter) == 0) {
              if (config->pnf_disconnect_indication != 0) {
                (config->pnf_disconnect_indication)(config, pnf_iter->p5_idx);
              }

              close(pnf_iter->p5_sock);

              pnf_iter->to_delete = 1;
              delete_pnfs = 1;
            }
          }

          pnf_iter = pnf_iter->next;
        }

        if (delete_pnfs) {
          nfapi_vnf_pnf_info_t *pnf_cur = config->pnf_list;
          nfapi_vnf_pnf_info_t *prev = 0;
          while (pnf_cur != 0) {
            nfapi_vnf_pnf_info_t *curr = pnf_cur;

            if (pnf_cur->to_delete == 1) {
              if (prev == 0) {
                config->pnf_list = pnf_cur->next;
              } else {
                prev->next = pnf_cur->next;
              }

              pnf_cur = pnf_cur->next;

              free(curr);
            } else {
              prev = pnf_cur;
              pnf_cur = pnf_cur->next;
            }
          }
        }
      }

      continue;
    } else {
      // timeout
      continue;
    }
  }

  NFAPI_TRACE(NFAPI_TRACE_INFO, "Closing p5Sock socket's\n");
  {
    nfapi_vnf_pnf_info_t *curr = config->pnf_list;
    while (curr != NULL) {
      if (config->pnf_disconnect_indication) {
        (config->pnf_disconnect_indication)(config, curr->p5_idx);
      }

      close(curr->p5_sock);
      curr = curr->next;
    }
  }

  NFAPI_TRACE(NFAPI_TRACE_INFO, "Closing p5Listen socket\n");
  close(p5ListenSock);

  return 0;
}

int nfapi_vnf_stop(nfapi_vnf_config_t *config)
{
  if (config == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);
  _this->terminate = 1;
  return 0;
}

int nfapi_vnf_allocate_phy(nfapi_vnf_config_t *config, int p5_idx, uint16_t *phy_id)
{
  vnf_t *vnf = (vnf_t *)config;

  nfapi_vnf_phy_info_t *info = (nfapi_vnf_phy_info_t *)calloc(1, sizeof(nfapi_vnf_phy_info_t));
  info->p5_idx = p5_idx;
  info->phy_id = vnf->next_phy_id++;

  info->timing_window = 30;
  info->timing_info_mode = 0x03;
  info->timing_info_period = 10;

  nfapi_vnf_phy_info_list_add(config, info);

  (*phy_id) = info->phy_id;

  return 0;
}
