/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#ifndef _NFAPI_LTE_VNF_INTERFACE_H_
#define _NFAPI_LTE_VNF_INTERFACE_H_

#include "nfapi_vnf_interface_common.h"

#if defined(__cplusplus)
extern "C" {
#endif

// P5 Request functions
int nfapi_vnf_pnf_param_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_param_request_t* req);
int nfapi_vnf_pnf_config_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_config_request_t* req);
int nfapi_vnf_pnf_start_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_start_request_t* req);
int nfapi_vnf_pnf_stop_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_stop_request_t* req);
int nfapi_vnf_param_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_param_request_t* req);
int nfapi_vnf_config_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_config_request_t* req);
int nfapi_vnf_start_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_start_request_t* req);
int nfapi_vnf_stop_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_stop_request_t* req);
int nfapi_vnf_measurement_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_measurement_request_t* req);

// P4 Request functions
int nfapi_vnf_rssi_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_rssi_request_t* req);
int nfapi_vnf_cell_search_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_cell_search_request_t* req);
int nfapi_vnf_broadcast_detect_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_broadcast_detect_request_t* req);
int nfapi_vnf_system_information_schedule_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_schedule_request_t* req);
int nfapi_vnf_system_information_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_request_t* req);
int nfapi_vnf_nmm_stop_request(nfapi_vnf_config_t* config, int p5_idx, nfapi_nmm_stop_request_t* req);

// P7 functions
int nfapi_vnf_p7_start(nfapi_vnf_p7_config_t* config);
int nfapi_vnf_p7_release_msg(nfapi_vnf_p7_config_t* config, nfapi_p7_message_header_t*);
int nfapi_vnf_p7_dl_config_req(nfapi_vnf_p7_config_t* config, nfapi_dl_config_request_t* req);
int nfapi_vnf_p7_ul_config_req(nfapi_vnf_p7_config_t* config, nfapi_ul_config_request_t* req);
int nfapi_vnf_p7_hi_dci0_req(nfapi_vnf_p7_config_t* config, nfapi_hi_dci0_request_t* req);
int nfapi_vnf_p7_tx_req(nfapi_vnf_p7_config_t* config, nfapi_tx_request_t* req);
int nfapi_vnf_p7_lbt_dl_config_req(nfapi_vnf_p7_config_t* config, nfapi_lbt_dl_config_request_t* req);
int nfapi_vnf_p7_ue_release_req(nfapi_vnf_p7_config_t* config, nfapi_ue_release_request_t* req);

#if defined(__cplusplus)
}
#endif

#endif // _NFAPI_LTE_VNF_INTERFACE_H_
