/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#include "vnf_p7.h"

int nfapi_vnf_p7_dl_config_req(nfapi_vnf_p7_config_t *config, nfapi_dl_config_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  return vnf_p7_pack_and_send_p7_msg(vnf_p7, &req->header);
}

int nfapi_vnf_p7_ul_config_req(nfapi_vnf_p7_config_t *config, nfapi_ul_config_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  return vnf_p7_pack_and_send_p7_msg(vnf_p7, &req->header);
}

int nfapi_vnf_p7_hi_dci0_req(nfapi_vnf_p7_config_t *config, nfapi_hi_dci0_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  return vnf_p7_pack_and_send_p7_msg(vnf_p7, &req->header);
}

int nfapi_vnf_p7_tx_req(nfapi_vnf_p7_config_t *config, nfapi_tx_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  return vnf_p7_pack_and_send_p7_msg(vnf_p7, &req->header);
}

int nfapi_vnf_p7_lbt_dl_config_req(nfapi_vnf_p7_config_t *config, nfapi_lbt_dl_config_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  return vnf_p7_pack_and_send_p7_msg(vnf_p7, &req->header);
}

int nfapi_vnf_p7_vendor_extension(nfapi_vnf_p7_config_t *config, nfapi_p7_message_header_t *header)
{
  if (config == 0 || header == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  return vnf_p7_pack_and_send_p7_msg(vnf_p7, header);
}

int nfapi_vnf_p7_ue_release_req(nfapi_vnf_p7_config_t *config, nfapi_ue_release_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  return vnf_p7_pack_and_send_p7_msg(vnf_p7, &req->header);
}
