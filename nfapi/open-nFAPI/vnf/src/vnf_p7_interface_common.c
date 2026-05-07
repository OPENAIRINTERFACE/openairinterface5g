/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

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

int nfapi_vnf_p7_release_pdu(nfapi_vnf_p7_config_t *config, void *pdu)
{
  if (config == 0 || pdu == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  vnf_p7_release_pdu(vnf_p7, pdu);

  return 0;
}
