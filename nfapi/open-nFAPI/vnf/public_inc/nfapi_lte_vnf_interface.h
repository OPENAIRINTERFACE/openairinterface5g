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

typedef struct vnf_s vnf_t;

typedef struct nfapi_vnf_config nfapi_vnf_config_t;

/*! The nfapi VNF configuration information
 */
typedef struct nfapi_vnf_config
{
	void* (*malloc)(size_t size);
	void (*free)(void*);

	int vnf_p5_port;

	int vnf_ipv4;
	int vnf_ipv6;

	nfapi_vnf_pnf_info_t* pnf_list;
	nfapi_vnf_phy_info_t* phy_list;

	nfapi_p4_p5_codec_config_t codec_config;

	void* user_data;

	int (*pnf_connection_indication)(nfapi_vnf_config_t* config, int p5_idx);
	int (*pnf_disconnect_indication)(nfapi_vnf_config_t* config, int p5_idx);

	int (*pnf_param_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_param_response_t* resp);
	int (*pnf_config_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_config_response_t* resp);
	int (*pnf_start_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_start_response_t* resp);
	int (*pnf_stop_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_pnf_stop_response_t* resp);

	int (*param_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_param_response_t* resp);
	int (*config_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_config_response_t* resp);
	int (*start_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_start_response_t* resp);
	int (*stop_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_stop_response_t* resp);
	int (*measurement_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_measurement_response_t* resp);

	int (*rssi_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_rssi_response_t* resp);
	int (*rssi_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_rssi_indication_t* ind);
	int (*cell_search_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_cell_search_response_t* resp);
	int (*cell_search_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_cell_search_indication_t* ind);
	int (*broadcast_detect_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_broadcast_detect_response_t* resp);
	int (*broadcast_detect_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_broadcast_detect_indication_t* ind);
	int (*system_information_schedule_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_schedule_response_t* resp);
	int (*system_information_schedule_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_schedule_indication_t* ind);
	int (*system_information_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_response_t* resp);
	int (*system_information_ind)(nfapi_vnf_config_t* config, int p5_idx, nfapi_system_information_indication_t* ind);
	int (*nmm_stop_resp)(nfapi_vnf_config_t* config, int p5_idx, nfapi_nmm_stop_response_t* resp);

	int (*vendor_ext)(nfapi_vnf_config_t* config, int p5_idx, void* msg);

	void* (*allocate_p4_p5_vendor_ext)(uint16_t message_id, uint16_t* msg_size);
	void (*deallocate_p4_p5_vendor_ext)(void* header);

	int (*pack_func)(void* pMessageBuf,
	                 uint32_t messageBufLen,
	                 void* pPackedBuf,
	                 uint32_t packedBufLen,
	                 nfapi_p4_p5_codec_config_t* config);

	bool (*unpack_func)(void* pMessageBuf,
	                    uint32_t messageBufLen,
	                    void* pUnpackedBuf,
	                    uint32_t unpackedBufLen,
	                    nfapi_p4_p5_codec_config_t* config);

	bool (*hdr_unpack_func)(void* pMessageBuf,
	                        uint32_t messageBufLen,
	                        void* pUnpackedBuf,
	                        uint32_t unpackedBufLen,
	                        nfapi_p4_p5_codec_config_t* config);

	bool (*send_p5_msg)(vnf_t* vnf, uint16_t p5_idx, nfapi_nr_p4_p5_message_header_t* msg, uint32_t msg_len);

} nfapi_vnf_config_t;

typedef nfapi_vnf_config_t nfapi_lte_vnf_config_t;

nfapi_vnf_config_t* nfapi_vnf_config_create(void);
void nfapi_vnf_config_destory(nfapi_vnf_config_t* config);
int nfapi_vnf_start(nfapi_vnf_config_t* config);
int nfapi_vnf_stop(nfapi_vnf_config_t* config);
int nfapi_vnf_allocate_phy(nfapi_vnf_config_t* config, int p5_idx, uint16_t* phy_id);
int nfapi_vnf_vendor_extension(nfapi_vnf_config_t* config, int p5_idx, nfapi_p4_p5_message_header_t* msg);

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
