/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef OPENAIRINTERFACE_NFAPI_NR_VNF_H
#define OPENAIRINTERFACE_NFAPI_NR_VNF_H

#include "nfapi_nr_vnf_interface.h"
#include "nfapi_common_vnf.h"

/* NR VNF entry point */
void configure_nr_nfapi_vnf(const char *vnf_addr, uint16_t vnf_p5_port, uint16_t vnf_p7_port);
void stop_nr_nfapi_vnf(void);
nfapi_vnf_config_t *get_nr_config();
vnf_p7_t *get_p7_nr_vnf();
nfapi_vnf_p7_config_t *get_p7_nr_vnf_config();

#endif /* OPENAIRINTERFACE_NFAPI_NR_VNF_H */
