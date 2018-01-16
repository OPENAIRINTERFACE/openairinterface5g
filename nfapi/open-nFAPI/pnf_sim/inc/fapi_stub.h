/*
 * Copyright 2017 Cisco Systems, Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _FAPI_STUB_H_
#define _FAPI_STUB_H_

#include "fapi_interface.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct fapi fapi_t;

typedef struct fapi {
	void* user_data;
} fapi_t;

typedef struct {
	int (*fapi_param_response)(fapi_t* fapi, fapi_param_resp_t* resp);
	int (*fapi_config_response)(fapi_t* fapi, fapi_config_resp_t* resp);

	int (*fapi_subframe_ind)(fapi_t* fapi, fapi_subframe_ind_t* ind);

	int (*fapi_harq_ind)(fapi_t* fapi, fapi_harq_ind_t* ind);
	int (*fapi_crc_ind)(fapi_t* fapi, fapi_crc_ind_t* ind);
	int (*fapi_rx_ulsch_ind)(fapi_t* fapi, fapi_rx_ulsch_ind_t* ind);
	int (*fapi_rx_cqi_ind)(fapi_t* fapi, fapi_rx_cqi_ind_t* ind);
	int (*fapi_rx_sr_ind)(fapi_t* fapi, fapi_rx_sr_ind_t* ind);
	int (*fapi_rach_ind)(fapi_t* fapi, fapi_rach_ind_t* ind);
	int (*fapi_srs_ind)(fapi_t* fapi, fapi_srs_ind_t* ind);
	
	int (*fapi_lbt_dl_ind)(fapi_t* fapi, fapi_lbt_dl_ind_t* ind);
	int (*fapi_nb_harq_ind)(fapi_t* fapi, fapi_nb_harq_ind_t* ind);
	int (*fapi_nrach_ind)(fapi_t* fapi, fapi_nrach_ind_t* ind);

} fapi_cb_t;

typedef struct {

	uint16_t duplex_mode;
	uint16_t dl_channel_bw_support;
	uint16_t ul_channel_bw_support;
} fapi_config_t;

fapi_t* fapi_create(fapi_cb_t* callbacks, fapi_config_t* config);

void fapi_start_data(fapi_t* fapi, unsigned rx_port, const char* tx_address, unsigned tx_port);

int fapi_param_request(fapi_t* fapi, fapi_param_req_t* req);
int fapi_config_request(fapi_t* fapi, fapi_config_req_t* req);
int fapi_start_request(fapi_t* fapi, fapi_start_req_t* req);

int fapi_dl_config_request(fapi_t* fapi, fapi_dl_config_req_t* req);
int fapi_ul_config_request(fapi_t* fapi, fapi_ul_config_req_t* req);
int fapi_hi_dci0_request(fapi_t* fapi, fapi_hi_dci0_req_t* req);
int fapi_tx_request(fapi_t* fapi, fapi_tx_req_t* req);

#if defined(__cplusplus)
}
#endif

#endif
