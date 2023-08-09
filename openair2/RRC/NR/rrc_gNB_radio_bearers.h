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

#ifndef _RRC_GNB_DRBS_H_
#define _RRC_GNB_DRBS_H_

#include "nr_rrc_defs.h"
#include "NR_SDAP-Config.h"
#include "NR_DRB-ToAddMod.h"
#include "NR_SRB-ToAddMod.h"

#include "common/platform_constants.h"

#define DRB_ACTIVE_NONGBR       (2)   /* DRB is used for Non-GBR Flows */
#define DRB_ACTIVE              (1)
#define DRB_INACTIVE            (0)
#define GBR_FLOW                (1)
#define NONGBR_FLOW             (0)

/// @brief Generates an ASN1 DRB-ToAddMod, from the established_drbs in gNB_RRC_UE_t.
/// @param drb_t drb_asn1
/// @return Returns the ASN1 DRB-ToAddMod structs.
NR_DRB_ToAddMod_t *generateDRB_ASN1(const drb_t *drb_asn1);
/// @brief Creates and stores a DRB in the gNB_RRC_UE_t struct, it doesn't create the actual entity,
/// to create the actual entity use the generateDRB_ASN1.
/// @param ue The gNB_RRC_UE_t struct that holds information for the UEs
/// @param drb_id The Data Radio Bearer Identity to be created for the established DRB.
/// @param pduSession The PDU Session that the DRB is created for.
/// @param enable_sdap If true the SDAP header will be added to the packet, else it will not add or search for SDAP header.
/// @param do_drb_integrity
/// @param do_drb_ciphering
void generateDRB(gNB_RRC_UE_t *ue,
                 uint8_t drb_id,
                 rrc_pdu_session_param_t *pduSession,
                 bool enable_sdap,
                 int do_drb_integrity,
                 int do_drb_ciphering);
uint8_t next_available_drb(gNB_RRC_UE_t *ue, rrc_pdu_session_param_t *pdusession, bool is_gbr);
bool drb_is_active(gNB_RRC_UE_t *ue, uint8_t drb_id);

rrc_pdu_session_param_t *find_pduSession(gNB_RRC_UE_t *ue, int id, bool create);

#endif
