/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*!
* \brief Header file for fapi_vnf_p7.c
 */

#ifndef OPENAIRINTERFACE_FAPI_VNF_P7_H
#define OPENAIRINTERFACE_FAPI_VNF_P7_H

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "nfapi_interface.h"
#include "nfapi_nr_interface_scf.h"
#include "nfapi_nr_vnf_interface.h"
#include "fapi_nvIPC.h"
#include "openair2/PHY_INTERFACE/queue_t.h"
#include "vnf_p7_nr.h"
uint8_t aerial_unpack_nr_rx_data_indication(uint8_t **ppReadPackedMsg,
                                            uint8_t *end,
                                            uint8_t **pDataMsg,
                                            uint8_t *data_end,
                                            nfapi_nr_rx_data_indication_t *msg,
                                            nfapi_p7_codec_config_t *config);

uint8_t aerial_unpack_nr_srs_indication(uint8_t **ppReadPackedMsg, uint8_t *end, uint8_t **pDataMsg, uint8_t *data_end, void *msg);
bool aerial_nr_send_p7_message(vnf_p7_t *vnf_p7, nfapi_nr_p7_message_header_t *header);
#endif // OPENAIRINTERFACE_FAPI_VNF_P7_H
