/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#ifndef _VNF_P7_LTE_H_
#define _VNF_P7_LTE_H_

#include "vnf_p7_common.h"

uint16_t increment_sfn_sf(uint16_t sfn_sf);
int vnf_sync(vnf_p7_t* vnf_p7, nfapi_vnf_p7_connection_info_t* p7_info);
int send_mac_subframe_indications(vnf_p7_t* config);
int vnf_p7_read_dispatch_message(vnf_p7_t* vnf_p7);
void vnf_p7_release_msg(vnf_p7_t* vnf_p7, nfapi_p7_message_header_t* header);

#endif // _VNF_P7_LTE_H_
