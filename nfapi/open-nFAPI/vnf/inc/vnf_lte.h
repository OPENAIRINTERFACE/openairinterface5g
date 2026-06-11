/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#ifndef _VNF_LTE_H_
#define _VNF_LTE_H_

#include "nfapi_vnf_interface.h"

typedef struct vnf_s {
  nfapi_vnf_config_t _public;

  uint8_t terminate;
  uint8_t sctp;

  uint8_t tx_message_buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
  uint16_t next_phy_id;

} vnf_t;

int vnf_pack_and_send_p5_message(vnf_t* vnf, uint16_t p5_idx, nfapi_p4_p5_message_header_t* msg, uint16_t msg_len);
int vnf_pack_and_send_p4_message(vnf_t* vnf, uint16_t p5_idx, nfapi_p4_p5_message_header_t* msg, uint16_t msg_len);

void nfapi_vnf_phy_info_list_add(nfapi_vnf_config_t* config, nfapi_vnf_phy_info_t* info);
nfapi_vnf_phy_info_t* nfapi_vnf_phy_info_list_find(nfapi_vnf_config_t* config, uint16_t phy_id);
void nfapi_vnf_pnf_list_add(nfapi_vnf_config_t* config, nfapi_vnf_pnf_info_t* node);
nfapi_vnf_pnf_info_t* nfapi_vnf_pnf_list_find(nfapi_vnf_config_t* config, int p5_idx);

void vnf_handle_vendor_extension(void *pRecvMsg, int recvMsgLen, nfapi_vnf_config_t *config, int p5_idx, uint16_t message_id);

int vnf_read_dispatch_message(nfapi_vnf_config_t* config, nfapi_vnf_pnf_info_t* pnf);

#endif // _VNF_LTE_H_
