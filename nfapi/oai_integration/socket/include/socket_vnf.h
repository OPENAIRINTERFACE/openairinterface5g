/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef SOCKET_VNF_H
#define SOCKET_VNF_H
#include "socket_common.h"
#include "vnf_p7.h"
#include "vnf_nr.h"
#include "nr_fapi_p5.h"
#include "nr_nfapi_p7.h"
#include "nr_fapi_p7.h"
bool vnf_nr_send_p5_msg(vnf_t *vnf, uint16_t p5_idx, nfapi_nr_p4_p5_message_header_t* msg, uint32_t msg_len);
bool vnf_nr_send_p7_msg(vnf_p7_t *vnf_p7, nfapi_nr_p7_message_header_t* header);
void vnf_start_p5_thread(void* ptr);
void* vnf_nr_start_p7_thread(void* ptr);
void socket_nfapi_send_stop_request(vnf_t *vnf);
#endif // SOCKET_VNF_H
