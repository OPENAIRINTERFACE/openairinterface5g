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

#include "vnf_lte.h"
#include "vnf_nr.h"

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

nfapi_nr_vnf_config_t *nfapi_nr_vnf_config_create()
{
  vnf_nr_t *_this = (vnf_nr_t *)calloc(1, sizeof(vnf_nr_t));

  if (_this == 0)
    return 0;

  _this->sctp = 1;
  _this->next_phy_id = 0;

  _this->_public.vnf_p5_port = NFAPI_P5_SCTP_PORT;
  _this->_public.malloc = &malloc;
  _this->_public.free = &free;
  _this->_public.codec_config.allocate = &malloc;
  _this->_public.codec_config.deallocate = &free;

  return (nfapi_nr_vnf_config_t *)_this;
}

void nfapi_nr_vnf_config_destory(nfapi_nr_vnf_config_t *config)
{
  free(config);
}

int nfapi_nr_vnf_stop(nfapi_nr_vnf_config_t *config)
{
  if (config == 0)
    return -1;

  vnf_nr_t *_this = (vnf_nr_t *)(config);
  _this->terminate = 1;
  return 0;
}

int nfapi_nr_vnf_allocate_phy(nfapi_nr_vnf_config_t *config, int p5_idx, uint16_t *phy_id)
{
  vnf_nr_t *vnf = (vnf_nr_t *)config;

  nfapi_vnf_phy_info_t *info = (nfapi_vnf_phy_info_t *)calloc(1, sizeof(nfapi_vnf_phy_info_t));
  info->p5_idx = p5_idx;
  info->phy_id = vnf->next_phy_id++;

  info->timing_window = 30;
  info->timing_info_mode = 0x03;
  info->timing_info_period = 10;

  nfapi_nr_vnf_phy_info_list_add(config, info);

  (*phy_id) = info->phy_id;

  return 0;
}
