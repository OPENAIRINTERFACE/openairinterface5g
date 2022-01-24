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
* \author Raymond Knopp, Navid Nikaein and Michele Paffetti
* \date 2011, 2017
* \version 1.0
* \company Eurecom
* \email: raymond.knopp@eurecom.fr ,navid.nikaein@eurecom.fr, michele.paffetti@studio.unibo.it
*/

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h> /* for atoi(3) */
#include <unistd.h> /* for getopt(3) */
#include <string.h> /* for strerror(3) */
#include <sysexits.h> /* for EX_* exit codes */
#include <errno.h>  /* for errno */

#include <asn_application.h>
#include <asn_internal.h> /* for _ASN_DEFAULT_STACK_MAX */


#include "RRC/LTE/defs_NB_IoT.h"

/*
 * The variant of the above function which dumps the BASIC-XER (XER_F_BASIC)
 * output into the chosen string buffer.
 * RETURN VALUES:
 *       0: The structure is printed.
 *      -1: Problem printing the structure.
 * WARNING: No sensible error value is returned.
 */


/**
\brief Generate configuration for SIB1 (eNB).
@param carrier pointer to Carrier information
@param N_RB_DL Number of downlink PRBs
@param frame radio frame number
@return size of encoded bit stream in bytes*/
uint8_t do_MIB_NB_IoT(
		rrc_eNB_carrier_data_NB_IoT_t *carrier,
		uint32_t N_RB_DL,
		uint32_t frame,
    uint32_t hyper_frame);


/**
\brief Generate a default configuration for SIB1-NB (eNB).
@param Mod_id Instance of eNB
@param CC_id Component carrier to configure
@param carrier pointer to Carrier information
@param configuration Pointer Configuration Request structure
@return size of encoded bit stream in bytes*/

uint8_t do_SIB1_NB_IoT(uint8_t Mod_id, 
                       int CC_id,
				               rrc_eNB_carrier_data_NB_IoT_t *carrier,
                       NbIoTRrcConfigurationReq *configuration,
				               uint32_t frame
                      );

/**
\brief Generate a default configuration for SIB2/SIB3-NB in one System Information PDU (eNB).
@param Mod_id Index of eNB (used to derive some parameters)
@param buffer Pointer to PER-encoded ASN.1 description of SI-NB PDU
@param systemInformation_NB_IoT Pointer to asn1c C representation of SI-NB PDU
@param sib2_NB Pointer (returned) to sib2_NB component withing SI-NB PDU
@param sib3_NB Pointer (returned) to sib3_NB component withing SI-NB PDU
@return size of encoded bit stream in bytes*/

uint8_t do_SIB23_NB_IoT(uint8_t Mod_id,
                        int CC_id,
                        rrc_eNB_carrier_data_NB_IoT_t *carrier,
                        NbIoTRrcConfigurationReq *configuration
                        );

/**(UE-SIDE)
\brief Generate an RRCConnectionRequest-NB UL-CCCH-Message (UE) based on random string or S-TMSI.  This
routine only generates an mo-data establishment cause.
@param buffer Pointer to PER-encoded ASN.1 description of UL-DCCH-Message PDU
@param rv 5 byte random string or S-TMSI
@param Mod_id
@returns Size of encoded bit stream in bytes*/

uint8_t do_RRCConnectionRequest_NB_IoT(uint8_t Mod_id, uint8_t *buffer,uint8_t *rv);


/**(UE -SIDE)
\brief Generate an RRCConnectionSetupComplete-NB UL-DCCH-Message (UE)
@param Mod_id
@param Transaction_id
@param dedicatedInfoNASLength
@param dedicatedInfoNAS
@param buffer Pointer to PER-encoded ASN.1 description of UL-DCCH-Message PDU
@returns Size of encoded bit stream in bytes*/

uint8_t do_RRCConnectionSetupComplete_NB_IoT(uint8_t Mod_id, uint8_t* buffer, const uint8_t Transaction_id, const int dedicatedInfoNASLength,
                                             const char* dedicatedInfoNAS);

/** (UE-SIDE)
\brief Generate an RRCConnectionReconfigurationComplete-NB UL-DCCH-Message (UE)
@param buffer Pointer to PER-encoded ASN.1 description of UL-DCCH-Message PDU
@param ctxt_pP
@param Transaction_id
@returns Size of encoded bit stream in bytes*/

size_t do_RRCConnectionReconfigurationComplete_NB_IoT(
  const protocol_ctxt_t* const ctxt_pP,
  uint8_t* buffer,
  size_t buffer_size,
  const uint8_t Transaction_id,
  OCTET_STRING_t *str
);

/**
\brief Generate an RRCConnectionSetup-NB DL-CCCH-Message (eNB).  This routine configures SRB_ToAddMod (SRB1/SRB1bis-NB) and
PhysicalConfigDedicated-NB IEs.
@param ctxt_pP Running context
@param ue_context_pP UE context
@param CC_id         Component Carrier ID
@param buffer Pointer to PER-encoded ASN.1 description of DL-CCCH-Message PDU
@param transmission_mode Transmission mode for UE (1-9)
@param UE_id UE index for this message
@param Transaction_id Transaction_ID for this message
@param SRB_configList Pointer (returned) to SRB1_config/SRB1bis_config(later) IEs for this UE
@param physicalConfigDedicated_NB Pointer (returned) to PhysicalConfigDedicated-NB IE for this UE
@returns Size of encoded bit stream in bytes*/

uint8_t do_RRCConnectionSetup_NB_IoT(
  const protocol_ctxt_t*     const ctxt_pP,
  rrc_eNB_ue_context_NB_IoT_t*      const ue_context_pP,
  int                              CC_id,
  uint8_t*                   const buffer, //carrier[CC_id].Srb0.Tx_buffer.Payload
  const uint8_t                    Transaction_id,
  const NB_IoT_DL_FRAME_PARMS* const frame_parms, //to be changed but not deleted
  SRB_ToAddModList_NB_r13_t**             SRB_configList_NB_IoT, //in order to be configured--> stanno puntando alla SRB_configlist dell ue_context
  struct PhysicalConfigDedicated_NB_r13** physicalConfigDedicated_NB_IoT //in order to be configured--> stanno puntando alla physicalConfigDedicated dell ue_context
);


/**
 * For which SRB is used in NB-IoT??
\brief Generate an RRCConnectionReconfiguration-NB DL-DCCH-Message (eNB).  This routine configures SRBToAddMod-NB (SRB1) and one DRBToAddMod-NB
(DRB3).  PhysicalConfigDedicated-NB is not updated.
@param ctxt_pP Running context
@param buffer Pointer to PER-encoded ASN.1 description of DL-CCCH-Message PDU
@param Transaction_id Transaction_ID for this message
@param SRB_list_NB Pointer to SRB List to be added/modified (NULL if no additions/modifications)
@param DRB_list_NB Pointer to DRB List to be added/modified (NULL if no additions/modifications)
@param DRB_list2_NB Pointer to DRB List to be released      (NULL if none to be released)
//sps not supported by NB-IoT
@param physicalConfigDedicated_NB Pointer to PhysicalConfigDedicated-NB to be modified (NULL if no modifications)
//measurement not supported by NB-IoT
@param mac_MainConfig Pointer to Mac_MainConfig(NULL if no modifications)
//no CBA functionalities
@returns Size of encoded bit stream in bytes*/

uint16_t
do_RRCConnectionReconfiguration_NB_IoT(
  const protocol_ctxt_t*        const ctxt_pP,
    uint8_t                            *buffer,
    size_t                              buffer_size,
    uint8_t                             Transaction_id,
    SRB_ToAddModList_NB_r13_t          *SRB_list_NB_IoT,
    DRB_ToAddModList_NB_r13_t          *DRB_list_NB_IoT,
    DRB_ToReleaseList_NB_r13_t         *DRB_list2_NB_IoT,
    struct PhysicalConfigDedicated_NB_r13     *physicalConfigDedicated,
    MAC_MainConfig_t                   *mac_MainConfig,
  struct RRCConnectionReconfiguration_NB_r13_IEs__dedicatedInfoNASList_r13* dedicatedInfoNASList_NB_IoT);

/**
 * E-UTRAN applies the procedure as follows: when only for NB-IoT SRB1 and SRB1bis is established
 \brief Generate a SecurityModeCommand
 @param ctxt_pP Running context
 @param buffer Pointer to PER-encoded ASN.1 description
 @param Transaction_id Transaction_ID for this message
 @param cipheringAlgorithm
 @param integrityProtAlgorithm
 */

uint8_t do_SecurityModeCommand_NB_IoT(
  const protocol_ctxt_t* const ctxt_pP,
  uint8_t* const buffer,
  const uint8_t Transaction_id,
  const uint8_t cipheringAlgorithm,
  const uint8_t integrityProtAlgorithm);

/**
 * E-UTRAN applies the procedure as follows: when only for NB-IoT SRB1 and SRB1bis is established
 \brief Generate a SecurityModeCommand
 @param ctxt_pP Running context
 @param buffer Pointer to PER-encoded ASN.1 description
 @param Transaction_id Transaction_ID for this message
 */

uint8_t do_UECapabilityEnquiry_NB_IoT(
  const protocol_ctxt_t* const ctxt_pP,
  uint8_t*               const buffer,
  const uint8_t                Transaction_id
);


/**
 * There is nothing new in this type of message for NB-IoT (only change in some nomenclature)
\brief Generate an RRCConnectionReestablishmentReject DL-CCCH-Message (eNB).
@param Mod_id Module ID of eNB
@param buffer Pointer to PER-encoded ASN.1 description of DL-CCCH-Message PDU
@returns Size of encoded bit stream in bytes*/

uint8_t
do_RRCConnectionReestablishmentReject_NB_IoT(
    uint8_t                    Mod_id,
    uint8_t*                   const buffer);


/**
\brief Generate an RRCConnectionReject-NB DL-CCCH-Message (eNB).
@param Mod_id Module ID of eNB
@param buffer Pointer to PER-encoded ASN.1 description of DL-CCCH-Message PDU
@returns Size of encoded bit stream in bytes*/
uint8_t
do_RRCConnectionReject_NB_IoT(
    uint8_t                    Mod_id,
    uint8_t*                   const buffer);


/**
\brief Generate an RRCConnectionRelease-NB DL-DCCH-Message
@param Mod_id Module ID of eNB
@param buffer Pointer to PER-encoded ASN.1 description
@param transaction_id Transaction index
@returns Size of encoded bit stream in bytes*/

uint8_t do_RRCConnectionRelease_NB_IoT(uint8_t Mod_id, uint8_t *buffer, size_t buffer_size,
                                       int Transaction_id);

uint8_t do_DLInformationTransfer_NB_IoT(
		uint8_t Mod_id,
		uint8_t **buffer,
		uint8_t transaction_id,
		uint32_t pdu_length,
		uint8_t *pdu_buffer);

//for now not implemented since UE side
//uint8_t do_ULInformationTransfer(uint8_t **buffer, uint32_t pdu_length, uint8_t *pdu_buffer);

//for now not implemented since UE side???
//OAI_UECapability_t *fill_ue_capability(char *UE_EUTRA_Capability_xer_fname)

/**
\brief Generate an RRCConnectionReestablishment-NB DL-CCCH Message
 *@param
 *
 */

uint8_t do_RRCConnectionReestablishment_NB_IoT(
		uint8_t Mod_id,
		uint8_t* buffer,
		size_t buffer_size,
		const uint8_t     Transaction_id,
		const NB_IoT_DL_FRAME_PARMS* const frame_parms, //to be changed
		SRB_ToAddModList_NB_r13_t**             SRB_configList_NB_IoT
		 );

/**
\brief Generate an RRCConnectionRelease-NB DL-DCCH-Message (eNB)
@param Mod_id Module ID of eNB
@param buffer Pointer to PER-encoded ASN.1 description of DL-DCCH-Message PDU
@param transaction_id Transaction index
@returns Size of encoded bit stream in bytes*/

//uint8_t do_RRCConnectionRelease_NB_IoT(uint8_t Mod_id, uint8_t *buffer,int Transaction_id);
