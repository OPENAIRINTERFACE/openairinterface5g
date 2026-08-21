/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#include "assertions.h"
#include "vnf_nr.h"

int nfapi_nr_vnf_pnf_param_req(nfapi_nr_vnf_config_t *config, int p5_idx, nfapi_nr_pnf_param_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_nr_t *_this = (vnf_nr_t *)(config);
  AssertFatal(config->send_p5_msg, "Function pointer must be configured\n");
  return config->send_p5_msg(_this, p5_idx, &req->header, sizeof(nfapi_nr_pnf_param_request_t));
}

int nfapi_nr_vnf_pnf_config_req(nfapi_nr_vnf_config_t *config, int p5_idx, nfapi_nr_pnf_config_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_nr_t *_this = (vnf_nr_t *)(config);
  AssertFatal(config->send_p5_msg, "Function pointer must be configured\n");
  return config->send_p5_msg(_this, p5_idx, &req->header, sizeof(nfapi_nr_pnf_config_request_t));
}

int nfapi_nr_vnf_pnf_start_req(nfapi_nr_vnf_config_t *config, int p5_idx, nfapi_nr_pnf_start_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_nr_t *_this = (vnf_nr_t *)(config);
  AssertFatal(config->send_p5_msg, "Function pointer must be configured\n");
  return config->send_p5_msg(_this, p5_idx, &req->header, sizeof(nfapi_nr_pnf_start_request_t));
}

int nfapi_nr_vnf_param_req(nfapi_nr_vnf_config_t *config, int p5_idx, nfapi_nr_param_request_scf_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_nr_t *_this = (vnf_nr_t *)(config);
  AssertFatal(config->send_p5_msg, "Function pointer must be configured\n");
  return config->send_p5_msg(_this, p5_idx, &req->header, sizeof(nfapi_nr_param_request_scf_t));
}

int nfapi_nr_vnf_config_req(nfapi_nr_vnf_config_t *config, int p5_idx, nfapi_nr_config_request_scf_t *req)
{
  if (config == 0 || req == 0)
    return -1;
  AssertFatal(config->send_p5_msg, "Function pointer must be configured\n");
  vnf_nr_t *_this = (vnf_nr_t *)(config);
#if !defined(ENABLE_WLS) && !defined(ENABLE_AERIAL)
  nfapi_vnf_phy_info_t *phy = nfapi_nr_vnf_phy_info_list_find(config, req->header.phy_id);

  if (phy == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_WARN, "%s failed to find phy information phy_id:%d\n", __FUNCTION__, req->header.phy_id);
    return -1;
  }

  // set the timing parameters
  req->nfapi_config.timing_window.tl.tag = NFAPI_NR_NFAPI_TIMING_WINDOW_TAG;
  req->nfapi_config.timing_window.value = phy->timing_window;
  req->num_tlv++;

  req->nfapi_config.timing_info_mode.tl.tag = NFAPI_NR_NFAPI_TIMING_INFO_MODE_TAG;
  req->nfapi_config.timing_info_mode.value = phy->timing_info_mode;
  req->num_tlv++;

  req->nfapi_config.timing_info_period.tl.tag = NFAPI_NR_NFAPI_TIMING_INFO_PERIOD_TAG;
  req->nfapi_config.timing_info_period.value = phy->timing_info_period;
  req->num_tlv++;
#endif
  return config->send_p5_msg(_this, p5_idx, &req->header, sizeof(nfapi_nr_config_request_scf_t));
}

int nfapi_nr_vnf_start_req(nfapi_nr_vnf_config_t *config, int p5_idx, nfapi_nr_start_request_scf_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_nr_t *_this = (vnf_nr_t *)(config);
  AssertFatal(config->send_p5_msg, "Function pointer must be configured\n");
  return config->send_p5_msg(_this, p5_idx, &req->header, sizeof(nfapi_nr_start_request_scf_t));
}

int nfapi_nr_vnf_stop_req(nfapi_nr_vnf_config_t *config, int p5_idx, nfapi_nr_stop_request_scf_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_nr_t *_this = (vnf_nr_t *)(config);
  AssertFatal(config->send_p5_msg, "Function pointer must be configured\n");
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] Sending NFAPI STOP.request\n");
  return config->send_p5_msg(_this, p5_idx, &req->header, sizeof(nfapi_nr_stop_request_scf_t));
}
