/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#ifndef _NFAPI_NR_VNF_INTERFACE_H_
#define _NFAPI_NR_VNF_INTERFACE_H_

#include "nfapi_vnf_interface_common.h"

#if defined(__cplusplus)
extern "C" {
#endif

// P5 Request functions
int nfapi_nr_vnf_pnf_param_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_param_request_t* req);
int nfapi_nr_vnf_pnf_config_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_config_request_t* req);
int nfapi_nr_vnf_pnf_start_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_start_request_t* req);
int nfapi_nr_vnf_param_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_param_request_scf_t* req);
int nfapi_nr_vnf_config_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_config_request_scf_t* req);
int nfapi_nr_vnf_start_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_start_request_scf_t* req);
int nfapi_nr_vnf_stop_req(nfapi_vnf_config_t* config, int p5_idx, nfapi_nr_stop_request_scf_t* req);

// P7 functions
bool nfapi_vnf_p7_dl_tti_req(nfapi_vnf_p7_config_t* config, nfapi_nr_dl_tti_request_t* req);
bool nfapi_vnf_p7_ul_tti_req(nfapi_vnf_p7_config_t* config, nfapi_nr_ul_tti_request_t* req);
bool nfapi_vnf_p7_ul_dci_req(nfapi_vnf_p7_config_t* config, nfapi_nr_ul_dci_request_t* req);
bool nfapi_vnf_p7_tx_data_req(nfapi_vnf_p7_config_t* config, nfapi_nr_tx_data_request_t* req);

#if defined(__cplusplus)
}
#endif

#endif // _NFAPI_NR_VNF_INTERFACE_H_
