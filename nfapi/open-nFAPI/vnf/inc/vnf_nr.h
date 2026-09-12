/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#ifndef _VNF_NR_H_
#define _VNF_NR_H_

#include "nfapi_nr_vnf_interface.h"

typedef struct vnf_nr_s {
  nfapi_nr_vnf_config_t _public;

  uint8_t terminate;
  uint8_t sctp;

  uint8_t tx_message_buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
  uint16_t next_phy_id;

} vnf_nr_t;

void nfapi_nr_vnf_phy_info_list_add(nfapi_nr_vnf_config_t* config, nfapi_vnf_phy_info_t* info);
nfapi_vnf_phy_info_t* nfapi_nr_vnf_phy_info_list_find(nfapi_nr_vnf_config_t* config, uint16_t phy_id);
void nfapi_nr_vnf_pnf_list_add(nfapi_nr_vnf_config_t* config, nfapi_vnf_pnf_info_t* node);
nfapi_vnf_pnf_info_t* nfapi_nr_vnf_pnf_list_find(nfapi_nr_vnf_config_t* config, int p5_idx);

void vnf_nr_handle_vendor_extension(void *pRecvMsg, int recvMsgLen, nfapi_nr_vnf_config_t *config, int p5_idx, uint16_t message_id);

void vnf_nr_handle_p4_p5_message(void *pRecvMsg, int recvMsgLen, int p5_idx, nfapi_nr_vnf_config_t* config);

#endif // _VNF_NR_H_
