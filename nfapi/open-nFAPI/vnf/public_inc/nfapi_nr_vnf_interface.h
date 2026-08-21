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

typedef struct vnf_nr_s vnf_nr_t;

typedef struct nfapi_nr_vnf_config nfapi_nr_vnf_config_t;

/*! NR VNF configuration — common fields mirror nfapi_vnf_config_t for safe cast of the shared list helpers */
typedef struct nfapi_nr_vnf_config
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

  int (*pnf_nr_connection_indication)(nfapi_nr_vnf_config_t* config, int p5_idx);
  int (*pnf_disconnect_indication)(nfapi_nr_vnf_config_t* config, int p5_idx);

  int (*pnf_nr_param_resp)(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_param_response_t* resp);
  int (*pnf_nr_config_resp)(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_config_response_t* resp);
  int (*pnf_nr_start_resp)(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_start_response_t* resp);
  int (*nr_stop_ind)(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_stop_indication_scf_t* ind);
  int (*nr_param_resp)(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_param_response_scf_t* resp);
  int (*nr_config_resp)(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_config_response_scf_t* resp);
  int (*nr_start_resp)(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_start_response_scf_t* resp);
  int (*nr_error_ind)(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_error_indication_scf_t* ind);

  int (*vendor_ext)(nfapi_nr_vnf_config_t* config, int p5_idx, void* msg);

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

  bool (*send_p5_msg)(vnf_nr_t* vnf, uint16_t p5_idx, nfapi_nr_p4_p5_message_header_t* msg, uint32_t msg_len);

} nfapi_nr_vnf_config_t;

nfapi_nr_vnf_config_t* nfapi_nr_vnf_config_create(void);
void nfapi_nr_vnf_config_destory(nfapi_nr_vnf_config_t* config);
int nfapi_nr_vnf_stop(nfapi_nr_vnf_config_t* config);
int nfapi_nr_vnf_allocate_phy(nfapi_nr_vnf_config_t* config, int p5_idx, uint16_t* phy_id);

// P5 Request functions
int nfapi_nr_vnf_pnf_param_req(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_param_request_t* req);
int nfapi_nr_vnf_pnf_config_req(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_config_request_t* req);
int nfapi_nr_vnf_pnf_start_req(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_pnf_start_request_t* req);
int nfapi_nr_vnf_param_req(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_param_request_scf_t* req);
int nfapi_nr_vnf_config_req(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_config_request_scf_t* req);
int nfapi_nr_vnf_start_req(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_start_request_scf_t* req);
int nfapi_nr_vnf_stop_req(nfapi_nr_vnf_config_t* config, int p5_idx, nfapi_nr_stop_request_scf_t* req);

// P7 functions (nfapi_nr_vnf_p7_config_t is typedef nfapi_vnf_p7_config_t)
bool nfapi_vnf_p7_dl_tti_req(nfapi_nr_vnf_p7_config_t* config, nfapi_nr_dl_tti_request_t* req);
bool nfapi_vnf_p7_ul_tti_req(nfapi_nr_vnf_p7_config_t* config, nfapi_nr_ul_tti_request_t* req);
bool nfapi_vnf_p7_ul_dci_req(nfapi_nr_vnf_p7_config_t* config, nfapi_nr_ul_dci_request_t* req);
bool nfapi_vnf_p7_tx_data_req(nfapi_nr_vnf_p7_config_t* config, nfapi_nr_tx_data_request_t* req);

#if defined(__cplusplus)
}
#endif

#endif // _NFAPI_NR_VNF_INTERFACE_H_
