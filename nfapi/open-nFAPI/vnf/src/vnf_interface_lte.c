/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#include "vnf.h"

int nfapi_vnf_pnf_param_req(nfapi_vnf_config_t *config, int p5_idx, nfapi_pnf_param_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_pnf_param_request_t));
}

int nfapi_vnf_pnf_config_req(nfapi_vnf_config_t *config, int p5_idx, nfapi_pnf_config_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_pnf_config_request_t));
}

int nfapi_vnf_pnf_start_req(nfapi_vnf_config_t *config, int p5_idx, nfapi_pnf_start_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_pnf_start_request_t));
}

int nfapi_vnf_pnf_stop_req(nfapi_vnf_config_t *config, int p5_idx, nfapi_pnf_stop_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_pnf_stop_request_t));
}

int nfapi_vnf_param_req(nfapi_vnf_config_t *config, int p5_idx, nfapi_param_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_param_request_t));
}

int nfapi_vnf_config_req(nfapi_vnf_config_t *config, int p5_idx, nfapi_config_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  nfapi_vnf_phy_info_t *phy = nfapi_vnf_phy_info_list_find(config, req->header.phy_id);

  if (phy == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_WARN, "%s failed to find phy inforation phy_id:%d\n", __FUNCTION__, req->header.phy_id);
    return -1;
  }

  // set the timing parameters
  req->nfapi_config.timing_window.tl.tag = NFAPI_NFAPI_TIMING_WINDOW_TAG;
  req->nfapi_config.timing_window.value = phy->timing_window;
  req->num_tlv++;

  req->nfapi_config.timing_info_mode.tl.tag = NFAPI_NFAPI_TIMING_INFO_MODE_TAG;
  req->nfapi_config.timing_info_mode.value = phy->timing_info_mode;
  req->num_tlv++;

  req->nfapi_config.timing_info_period.tl.tag = NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG;
  req->nfapi_config.timing_info_period.value = phy->timing_info_period;
  req->num_tlv++;

  return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_config_request_t));
}

int nfapi_vnf_start_req(nfapi_vnf_config_t *config, int p5_idx, nfapi_start_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_start_request_t));
}

int nfapi_vnf_stop_req(nfapi_vnf_config_t *config, int p5_idx, nfapi_stop_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_stop_request_t));
}

int nfapi_vnf_measurement_req(nfapi_vnf_config_t *config, int p5_idx, nfapi_measurement_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p5_message(_this, p5_idx, &req->header, sizeof(nfapi_measurement_request_t));
}

int nfapi_vnf_rssi_request(nfapi_vnf_config_t *config, int p5_idx, nfapi_rssi_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p4_message(_this, p5_idx, &req->header, sizeof(nfapi_rssi_request_t));
}

int nfapi_vnf_cell_search_request(nfapi_vnf_config_t *config, int p5_idx, nfapi_cell_search_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p4_message(_this, p5_idx, &req->header, sizeof(nfapi_cell_search_request_t));
}

int nfapi_vnf_broadcast_detect_request(nfapi_vnf_config_t *config, int p5_idx, nfapi_broadcast_detect_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p4_message(_this, p5_idx, &req->header, sizeof(nfapi_broadcast_detect_request_t));
}

int nfapi_vnf_system_information_schedule_request(nfapi_vnf_config_t *config,
                                                   int p5_idx,
                                                   nfapi_system_information_schedule_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p4_message(_this, p5_idx, &req->header, sizeof(nfapi_system_information_schedule_request_t));
}

int nfapi_vnf_system_information_request(nfapi_vnf_config_t *config, int p5_idx, nfapi_system_information_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p4_message(_this, p5_idx, &req->header, sizeof(nfapi_system_information_request_t));
}

int nfapi_vnf_nmm_stop_request(nfapi_vnf_config_t *config, int p5_idx, nfapi_nmm_stop_request_t *req)
{
  if (config == 0 || req == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p4_message(_this, p5_idx, &req->header, sizeof(nfapi_nmm_stop_request_t));
}

int nfapi_vnf_vendor_extension(nfapi_vnf_config_t *config, int p5_idx, nfapi_p4_p5_message_header_t *msg)
{
  if (config == 0 || msg == 0)
    return -1;

  vnf_t *_this = (vnf_t *)(config);

  return vnf_pack_and_send_p5_message(_this, p5_idx, msg, sizeof(nfapi_p4_p5_message_header_t));
}
