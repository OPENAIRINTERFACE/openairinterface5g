/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#include "assertions.h"
#include "vnf_p7_nr.h"

bool nfapi_vnf_p7_dl_tti_req(nfapi_vnf_p7_config_t *config, nfapi_nr_dl_tti_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  AssertFatal(config->send_p7_msg, "Function pointer must be configured");
  return config->send_p7_msg(vnf_p7, &req->header);
}

bool nfapi_vnf_p7_ul_tti_req(nfapi_vnf_p7_config_t *config, nfapi_nr_ul_tti_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  AssertFatal(config->send_p7_msg, "Function pointer must be configured");
  return config->send_p7_msg(vnf_p7, &req->header);
}

bool nfapi_vnf_p7_ul_dci_req(nfapi_vnf_p7_config_t *config, nfapi_nr_ul_dci_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  AssertFatal(config->send_p7_msg, "Function pointer must be configured");
  return config->send_p7_msg(vnf_p7, &req->header);
}

bool nfapi_vnf_p7_tx_data_req(nfapi_vnf_p7_config_t *config, nfapi_nr_tx_data_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_p7_t *vnf_p7 = (vnf_p7_t *)config;
  AssertFatal(config->send_p7_msg, "Function pointer must be configured");
  return config->send_p7_msg(vnf_p7, &req->header);
}
