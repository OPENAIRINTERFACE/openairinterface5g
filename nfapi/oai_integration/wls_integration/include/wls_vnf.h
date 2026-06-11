/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef OPENAIRINTERFACE_WLS_VNF_H
#define OPENAIRINTERFACE_WLS_VNF_H
#include "nfapi_nr_vnf_interface.h"
#include "vnf_nr.h"
#include "vnf_p7_nr.h"
#include "wls_common.h"

void *wls_fapi_vnf_nr_start_thread(void *ptr);
int wls_fapi_nr_vnf_start();
bool wls_vnf_nr_send_p5_message(vnf_t *vnf,uint16_t p5_idx, nfapi_nr_p4_p5_message_header_t* msg, uint32_t msg_len);
bool wls_vnf_nr_send_p7_message(vnf_p7_t* vnf_p7,nfapi_nr_p7_message_header_t* msg);
void wls_vnf_send_stop_request();
void wls_vnf_stop();
#endif //OPENAIRINTERFACE_WLS_VNF_H
