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

/*! \file asn1_msg.h
* \brief primitives to build the asn1 messages
* \author Raymond Knopp and Navid Nikaein, WIE-TAI CHEN
* \date 2011, 2018
* \version 1.0
* \company Eurecom, NTUST
* \email: raymond.knopp@eurecom.fr and  navid.nikaein@eurecom.fr, kroempa@gmail.com
*/

#pragma once

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h> /* for atoi(3) */
#include <unistd.h> /* for getopt(3) */
#include <string.h> /* for strerror(3) */
#include <sysexits.h> /* for EX_* exit codes */
#include <errno.h>  /* for errno */

#include <asn_application.h>
#include <asn_internal.h> /* for _ASN_DEFAULT_STACK_MAX */

#include "RRC/NR/nr_rrc_defs.h"
#include "RRC/NR/nr_rrc_config.h"


/*
 * The variant of the above function which dumps the BASIC-XER (XER_F_BASIC)
 * output into the chosen string buffer.
 * RETURN VALUES:
 *       0: The structure is printed.
 *      -1: Problem printing the structure.
 * WARNING: No sensible errno value is returned.
 */
int xer_sprint_NR(char *string, size_t string_size, struct asn_TYPE_descriptor_s *td, void *sptr);


#define asn1cCallocOne(VaR, VaLue) \
  VaR = calloc(1,sizeof(*VaR)); *VaR=VaLue;
#define asn1cCalloc(VaR, lOcPtr) \
  typeof(VaR) lOcPtr = VaR = calloc(1,sizeof(*VaR));
#define asn1cSequenceAdd(VaR, TyPe, lOcPtr) \
  TyPe *lOcPtr= calloc(1,sizeof(TyPe)); \
  ASN_SEQUENCE_ADD(&VaR,lOcPtr);

uint8_t do_MIB_NR(gNB_RRC_INST *rrc,
                  uint32_t frame);

/**
\brief Generate configuration for SIB1 (gNB).
@param carrier pointer to Carrier information
@param configuration Pointer Configuration Request structure  
@return size of encoded bit stream in bytes*/
uint8_t do_SIB1_NR(rrc_gNB_carrier_data_t *carrier,
  gNB_RrcConfigurationReq *configuration
                  );

uint8_t do_SIB23_NR(rrc_gNB_carrier_data_t *carrier,
                    gNB_RrcConfigurationReq *configuration);

void do_RLC_BEARER(uint8_t Mod_id,
                    int CC_id,
                    struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_BearerToAddModList,
                    rlc_bearer_config_t  *rlc_config);
void do_MAC_CELLGROUP(uint8_t Mod_id,
                      int CC_id,
                      NR_MAC_CellGroupConfig_t *mac_CellGroupConfig,
                      mac_cellgroup_t *mac_cellgroup_config);

void do_PHYSICALCELLGROUP(uint8_t Mod_id,
                          int CC_id,
                          NR_PhysicalCellGroupConfig_t *physicalCellGroupConfig,
                          physicalcellgroup_t *physicalcellgroup_config);


void do_SpCellConfig(gNB_RRC_INST *rrc,
                      struct NR_SpCellConfig  *spconfig);

uint8_t do_RRCReject(uint8_t Mod_id,
                     uint8_t *const buffer);

void fill_initial_SpCellConfig(rnti_t rnti,
                               int uid,
                               NR_SpCellConfig_t *SpCellConfig,
                               NR_ServingCellConfigCommon_t *scc,
                               NR_ServingCellConfig_t *servingcellconfigdedicated,
                               rrc_gNB_carrier_data_t *carrier);

void fill_initial_cellGroupConfig(rnti_t rnti,
                                  int uid,
                                  NR_CellGroupConfig_t *cellGroupConfig,
                                  NR_ServingCellConfigCommon_t *scc,
                                  NR_ServingCellConfig_t *servingcellconfigdedicated,
                                  rrc_gNB_carrier_data_t *carrier) ;

void fill_mastercellGroupConfig(NR_CellGroupConfig_t *cellGroupConfig, NR_CellGroupConfig_t *ue_context_mastercellGroup);

uint8_t do_RRCSetup(rrc_gNB_ue_context_t         *const ue_context_pP,
                    uint8_t                      *const buffer,
                    const uint8_t                transaction_id,
                    OCTET_STRING_t               *masterCellGroup_from_DU,
                    NR_ServingCellConfigCommon_t *scc,
                    NR_ServingCellConfig_t       *servingcellconfigdedicated,
                    rrc_gNB_carrier_data_t       *carrier);

uint8_t do_NR_SecurityModeCommand(
                    const protocol_ctxt_t *const ctxt_pP,
                    uint8_t *const buffer,
                    const uint8_t Transaction_id,
                    const uint8_t cipheringAlgorithm,
                    NR_IntegrityProtAlgorithm_t *integrityProtAlgorithm);

uint8_t do_NR_SA_UECapabilityEnquiry( const protocol_ctxt_t *const ctxt_pP,
                                   uint8_t               *const buffer,
                                   const uint8_t                Transaction_id);

uint8_t do_NR_RRCRelease(uint8_t *buffer, size_t buffer_size,
                         uint8_t Transaction_id);

int16_t do_RRCReconfiguration(
    const protocol_ctxt_t        *const ctxt_pP,
    uint8_t                      *buffer,
    size_t                        buffer_size,
    uint8_t                       Transaction_id,
    NR_SRB_ToAddModList_t        *SRB_configList,
    NR_DRB_ToAddModList_t        *DRB_configList,
    NR_DRB_ToReleaseList_t       *DRB_releaseList,
    NR_SecurityConfig_t          *security_config,
    NR_SDAP_Config_t             *sdap_config,
    NR_MeasConfig_t              *meas_config,
    struct NR_RRCReconfiguration_v1530_IEs__dedicatedNAS_MessageList *dedicatedNAS_MessageList,
    NR_MAC_CellGroupConfig_t     *mac_CellGroupConfig,
    NR_CellGroupConfig_t         *cellGroupConfig);

uint8_t do_RRCSetupComplete(uint8_t Mod_id,
                            uint8_t *buffer,
                            size_t buffer_size,
                            const uint8_t Transaction_id,
                            uint8_t sel_plmn_id,
                            const int dedicatedInfoNASLength,
                            const char *dedicatedInfoNAS);

uint8_t do_RRCSetupRequest(uint8_t Mod_id, uint8_t *buffer, size_t buffer_size, uint8_t *rv);

uint8_t do_NR_RRCReconfigurationComplete_for_nsa(uint8_t *buffer, size_t buffer_size, NR_RRC_TransactionIdentifier_t Transaction_id);

uint8_t do_NR_RRCReconfigurationComplete(
                        const protocol_ctxt_t *const ctxt_pP,
                        uint8_t *buffer,
                        size_t buffer_size,
                        const uint8_t Transaction_id
                      );

uint8_t 
do_NR_DLInformationTransfer(
    uint8_t Mod_id,
    uint8_t **buffer,
    uint8_t transaction_id,
    uint32_t pdu_length,
    uint8_t *pdu_buffer
);

uint8_t do_NR_ULInformationTransfer(uint8_t **buffer, 
                        uint32_t pdu_length,
                        uint8_t *pdu_buffer);

uint8_t do_RRCReestablishmentRequest(uint8_t Mod_id, uint8_t *buffer, uint16_t c_rnti);

uint8_t
do_RRCReestablishment(
  const protocol_ctxt_t     *const ctxt_pP,
  rrc_gNB_ue_context_t      *const ue_context_pP,
  int                              CC_id,
  uint8_t                   *const buffer,
  size_t                           buffer_size,
  //const uint8_t                    transmission_mode,
  const uint8_t                    Transaction_id,
  NR_SRB_ToAddModList_t               **SRB_configList
);

uint8_t 
do_RRCReestablishmentComplete(
    uint8_t *buffer, size_t buffer_size,
    int64_t rrc_TransactionIdentifier);

