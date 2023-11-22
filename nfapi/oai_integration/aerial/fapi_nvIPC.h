/*
* Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
* contributor license agreements.  See the NOTICE file distributed with
* this work for additional information regarding copyright ownership.
* The OpenAirInterface Software Alliance licenses this file to You under
* the OAI Public License, Version 1.1  (the "License"); you may not use this file
* except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.openairinterface.org/?page_id=698
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*-------------------------------------------------------------------------------
* For more information about the OpenAirInterface (OAI) Software Alliance:
*      contact@openairinterface.org
 */

/*! \file fapi/oai-integration/fapi_nvIPC.h
* \brief Header file for fapi_nvIPC.c
* \author Ruben S. Silva
* \date 2023
* \version 0.1
* \company OpenAirInterface Software Alliance
* \email: contact@openairinterface.org, rsilva@allbesmart.pt
* \note
* \warning
 */
#ifdef ENABLE_AERIAL
#ifndef OPENAIRINTERFACE_FAPI_NVIPC_H
#define OPENAIRINTERFACE_FAPI_NVIPC_H

#include "nv_ipc.h"
#include "nv_ipc_utils.h"
#include "nvlog.h"
#include "nfapi/open-nFAPI/vnf/public_inc/nfapi_vnf_interface.h"
#include "openair1/PHY/defs_gNB.h"
#include "debug.h"

typedef struct {
  uint8_t num_msg;
  uint8_t opaque_handle;
  uint16_t message_id;
  uint32_t message_length;
} fapi_phy_api_msg;

int aerial_send_P5_msg(void *packedBuf, uint32_t packedMsgLength, nfapi_p4_p5_message_header_t *header);
int aerial_send_P7_msg(void *packedBuf, uint32_t packedMsgLength, nfapi_p7_message_header_t *header);
int aerial_send_P7_msg_with_data(void *packedBuf,
                                      uint32_t packedMsgLength,
                                      void *dataBuf,
                                      uint32_t dataLength,
                                      nfapi_p7_message_header_t *header);
void set_config(nfapi_vnf_config_t *conf);
int nvIPC_Init();

#endif // OPENAIRINTERFACE_FAPI_NVIPC_H
#endif
