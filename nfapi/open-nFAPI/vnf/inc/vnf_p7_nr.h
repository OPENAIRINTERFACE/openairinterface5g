/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#ifndef _VNF_P7_NR_H_
#define _VNF_P7_NR_H_

#include "vnf_p7_common.h"

int vnf_nr_sync(vnf_p7_t* vnf_p7, nfapi_vnf_p7_connection_info_t* p7_info);
int send_mac_slot_indications(vnf_p7_t* config);
void vnf_nr_handle_p7_message(void *pRecvMsg, int recvMsgLen, vnf_p7_t* vnf_p7);

#endif // _VNF_P7_NR_H_
