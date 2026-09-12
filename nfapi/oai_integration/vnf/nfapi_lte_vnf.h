/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef NFAPI_LTE_VNF_H_
#define NFAPI_LTE_VNF_H_
#include "nfapi_lte_vnf_interface.h"
#include "nfapi_common_vnf.h"

void configure_nfapi_vnf(char *vnf_addr, int vnf_p5_port, char *pnf_ip_addr, int pnf_p7_port, int vnf_p7_port);
#endif /* NFAPI_LTE_VNF_H_ */
