/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2017 Cisco Systems, Inc.
 */

#ifndef _VNF_NR_H_
#define _VNF_NR_H_

#include "vnf_common.h"

void vnf_nr_handle_p4_p5_message(void *pRecvMsg, int recvMsgLen, int p5_idx, nfapi_vnf_config_t* config);

#endif // _VNF_NR_H_
