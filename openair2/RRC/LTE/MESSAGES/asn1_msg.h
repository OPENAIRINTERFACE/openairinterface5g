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
* \author Raymond Knopp and Navid Nikaein
* \date 2011
* \version 1.0
* \company Eurecom
* \email: raymond.knopp@eurecom.fr and  navid.nikaein@eurecom.fr
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

#include "RRC/LTE/rrc_defs.h"
#include "LTE_SL-DestinationInfoList-r12.h"

/*
 * The variant of the above function which dumps the BASIC-XER (XER_F_BASIC)
 * output into the chosen string buffer.
 * RETURN VALUES:
 *       0: The structure is printed.
 *      -1: Problem printing the structure.
 * WARNING: No sensible errno value is returned.
 */
int xer_sprint(char *string, size_t string_size, struct asn_TYPE_descriptor_s *td, void *sptr);

uint16_t get_adjacent_cell_id(uint8_t Mod_id,uint8_t index);

uint8_t get_adjacent_cell_mod_id(uint16_t phyCellId);

/**
\brief Generate configuration for MIB (eNB).
@param carrier pointer to Carrier information
@param N_RB_DL Number of downlink PRBs
@param phich_Resource PHICH resoure parameter
@param phich_duration PHICH duration parameter
@param frame radio frame number
@return size of encoded bit stream in bytes*/
uint8_t do_MIB(rrc_eNB_carrier_data_t *carrier, uint32_t N_RB_DL, uint32_t phich_Resource, uint32_t phich_duration, uint32_t frame
#if (LTE_RRC_VERSION >= MAKE_VERSION(13, 0, 0))
	       , uint32_t schedulingInfoSIB1
#endif
	       );

/**
<<<<<<< HEAD
\brief Generate configuration for MIB (eNB).
@param carrier pointer to Carrier information
@param N_RB_DL Number of downlink PRBs
@param additional Non MBSFN Subframes parameter
@param frame radio frame number
@return size of encoded bit stream in bytes*/
uint8_t do_MIB_FeMBMS(rrc_eNB_carrier_data_t *carrier, uint32_t N_RB_DL, uint32_t additionalNonMBSFNSubframes, uint32_t frame);
/**
\brief Generate configuration structure for DRX_Config
@param CC_id Id of component to configure
@param configuration Pointer Configuration Request structure
@param UEcap Pointer Configuration UE capablities
@return DRX_Config structure pointer or NULL => error */
LTE_DRX_Config_t *do_DrxConfig(int CC_id, RrcConfigurationReq *configuration, LTE_UE_EUTRA_Capability_t *UEcap);

/**
\brief Generate configuration for SIB1 (eNB).
@param carrier pointer to Carrier information
@param Mod_id Instance of eNB
@param Component carrier Component carrier to configure
@param configuration Pointer Configuration Request structure  
@param br_flag Do for BL/CE UE configuration
@return size of encoded bit stream in bytes*/

uint8_t do_SIB1(rrc_eNB_carrier_data_t *carrier,int Mod_id,int CC_id
#if (LTE_RRC_VERSION >= MAKE_VERSION(13, 0, 0))
                , BOOLEAN_t brOption
#endif
#if defined(ENABLE_ITTI)
                ,RrcConfigurationReq *configuration
#endif
               );

/**
\brief Generate configuration for SIB1 MBMS (eNB).
@param carrier pointer to Carrier information
@param Mod_id Instance of eNB
@param Component carrier Component carrier to configure
@param configuration Pointer Configuration Request structure
@return size of encoded bit stream in bytes*/
uint8_t do_SIB1_MBMS(rrc_eNB_carrier_data_t *carrier,int Mod_id,int CC_id, RrcConfigurationReq *configuration
               );



/**
\brief Generate a default configuration for SIB2/SIB3 in one System Information PDU (eNB).
@param Mod_id Index of eNB (used to derive some parameters)
@param buffer Pointer to PER-encoded ASN.1 description of SI PDU
@param systemInformation Pointer to asn1c C representation of SI PDU
@param sib2 Pointer (returned) to sib2 component withing SI PDU
@param sib3 Pointer (returned) to sib3 component withing SI PDU
@param sib13 Pointer (returned) to sib13 component withing SI PDU
@param MBMS_flag Indicates presence of MBMS system information (when 1)
@return size of encoded bit stream in bytes*/

uint8_t do_SIB23(uint8_t Mod_id,
                 int CC_id
#if (LTE_RRC_VERSION >= MAKE_VERSION(13, 0, 0))
		 , BOOLEAN_t brOption
#endif
#if defined(ENABLE_ITTI)
  , RrcConfigurationReq *configuration
#endif
                );

/**
\brief Generate an RRCConnectionRequest UL-CCCH-Message (UE) based on random string or S-TMSI.  This
routine only generates an mo-data establishment cause.
@param buffer Pointer to PER-encoded ASN.1 description of UL-DCCH-Message PDU
@param rv 5 byte random string or S-TMSI
@returns Size of encoded bit stream in bytes*/

uint8_t do_RRCConnectionRequest(uint8_t Mod_id, uint8_t *buffer,uint8_t *rv);

/**
\brief Generate an SidelinkUEInformation UL-DCCH-Message (UE).
@param destinationInfoList Pointer to a list of destination for which UE requests E-UTRAN to assign dedicated resources
@param discTxResourceReq Pointer to  number of discovery messages for discovery announcements for which  UE requests E-UTRAN to assign dedicated resources
@param mode Indicates different requests from upper layers
@returns Size of encoded bit stream in bytes*/
uint8_t do_SidelinkUEInformation(uint8_t Mod_id, uint8_t *buffer, LTE_SL_DestinationInfoList_r12_t  *destinationInfoList, long *discTxResourceReq, SL_TRIGGER_t mode);

/** \brief Generate an RRCConnectionSetupComplete UL-DCCH-Message (UE)
@param buffer Pointer to PER-encoded ASN.1 description of UL-DCCH-Message PDU
@returns Size of encoded bit stream in bytes*/
uint8_t do_RRCConnectionSetupComplete(uint8_t Mod_id, uint8_t *buffer, const uint8_t Transaction_id, const int dedicatedInfoNASLength,
                                      const char *dedicatedInfoNAS);

/** \brief Generate an RRCConnectionReconfigurationComplete UL-DCCH-Message (UE)
@param buffer Pointer to PER-encoded ASN.1 description of UL-DCCH-Message PDU
@returns Size of encoded bit stream in bytes*/
uint8_t
do_RRCConnectionReconfigurationComplete(
  const protocol_ctxt_t *const ctxt_pP,
  uint8_t *buffer,
  const uint8_t Transaction_id
);

/**
\brief Generate an RRCConnectionSetup DL-CCCH-Message (eNB).  This routine configures SRB_ToAddMod (SRB1/SRB2) and
PhysicalConfigDedicated IEs.  The latter does not enable periodic CQI reporting (PUCCH format 2/2a/2b) or SRS.
@param ctxt_pP Running context
@param ue_context_pP UE context
@param CC_id         Component Carrier ID
@param buffer Pointer to PER-encoded ASN.1 description of DL-CCCH-Message PDU
@param transmission_mode Transmission mode for UE (1-9)
@param UE_id UE index for this message
@param Transaction_id Transaction_ID for this message
@param SRB_configList Pointer (returned) to SRB1_config/SRB2_config(later) IEs for this UE
@param physicalConfigDedicated Pointer (returned) to PhysicalConfigDedicated IE for this UE
@returns Size of encoded bit stream in bytes*/
uint8_t
do_RRCConnectionSetup(
  const protocol_ctxt_t     *const ctxt_pP,
  rrc_eNB_ue_context_t      *const ue_context_pP,
  int                              CC_id,
  uint8_t                   *const buffer,
  const uint8_t                    transmission_mode,
  const uint8_t                    Transaction_id,
  LTE_SRB_ToAddModList_t             **SRB_configList,
  struct LTE_PhysicalConfigDedicated **physicalConfigDedicated
);
#if (LTE_RRC_VERSION >= MAKE_VERSION(13, 0, 0))
uint8_t
do_RRCConnectionSetup_BR(
	const protocol_ctxt_t*     const ctxt_pP,
	rrc_eNB_ue_context_t*      const ue_context_pP,
	int                              CC_id,
	uint8_t*                   const buffer,
	const uint8_t                    transmission_mode,
	const uint8_t                    Transaction_id,
	LTE_SRB_ToAddModList_t**             SRB_configList,
	struct LTE_PhysicalConfigDedicated** physicalConfigDedicated
);
#endif



#if (LTE_RRC_VERSION >= MAKE_VERSION(13, 0, 0))
uint16_t
do_RRCConnectionReconfiguration_BR(
                   const protocol_ctxt_t*        const ctxt_pP,
                   uint8_t                            *buffer,
                   uint8_t                             Transaction_id,
                   LTE_SRB_ToAddModList_t                 *SRB_list,
                   LTE_DRB_ToAddModList_t                 *DRB_list,
                   LTE_DRB_ToReleaseList_t                *DRB_list2,
                   struct LTE_SPS_Config                  *sps_Config,
                   struct LTE_PhysicalConfigDedicated     *physicalConfigDedicated,
                   LTE_MeasObjectToAddModList_t           *MeasObj_list,
                   LTE_ReportConfigToAddModList_t         *ReportConfig_list,
                   LTE_QuantityConfig_t                   *quantityConfig,
                   LTE_MeasIdToAddModList_t               *MeasId_list,
                   LTE_MAC_MainConfig_t                   *mac_MainConfig,
                   LTE_MeasGapConfig_t                    *measGapConfig,
                   LTE_MobilityControlInfo_t              *mobilityInfo,
                   struct LTE_MeasConfig__speedStatePars  *speedStatePars,
                   LTE_RSRP_Range_t                       *rsrp,
                   LTE_C_RNTI_t                           *cba_rnti,
                   struct LTE_RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList *dedicatedInfoNASList,
                   LTE_SCellToAddMod_r10_t  *SCell_config
                   );
#endif
/**
\brief Generate an RRCConnectionReconfiguration DL-DCCH-Message (eNB).  This routine configures SRBToAddMod (SRB2) and one DRBToAddMod
(DRB3).  PhysicalConfigDedicated is not updated.
@param ctxt_pP Running context
@param buffer Pointer to PER-encoded ASN.1 description of DL-CCCH-Message PDU
@param Transaction_id Transaction_ID for this message
@param SRB_list Pointer to SRB List to be added/modified (NULL if no additions/modifications)
@param DRB_list Pointer to DRB List to be added/modified (NULL if no additions/modifications)
@param DRB_list2 Pointer to DRB List to be released      (NULL if none to be released)
@param sps_Config Pointer to sps_Config to be modified (NULL if no modifications, or default if initial configuration)
@param physicalConfigDedicated Pointer to PhysicalConfigDedicated to be modified (NULL if no modifications)
@param MeasObj_list Pointer to MeasObj List to be added/modified (NULL if no additions/modifications)
@param ReportConfig_list Pointer to ReportConfig List (NULL if no additions/modifications)
@param QuantityConfig Pointer to QuantityConfig to be modified (NULL if no modifications)
@param MeasId_list Pointer to MeasID List (NULL if no additions/modifications)
@param mobilityInfo mobility control information for handover
@param speedStatePars speed state parameteres for handover
@param mac_MainConfig Pointer to Mac_MainConfig(NULL if no modifications)
@param measGapConfig Pointer to MeasGapConfig (NULL if no modifications)
@param cba_rnti RNTI for the cba transmission
@returns Size of encoded bit stream in bytes*/

uint16_t
do_RRCConnectionReconfiguration(
  const protocol_ctxt_t        *const ctxt_pP,
  uint8_t                            *buffer,
  uint8_t                             Transaction_id,
  LTE_SRB_ToAddModList_t                 *SRB_list,
  LTE_DRB_ToAddModList_t                 *DRB_list,
  LTE_DRB_ToReleaseList_t                *DRB_list2,
  struct LTE_SPS_Config                  *sps_Config,
  struct LTE_PhysicalConfigDedicated     *physicalConfigDedicated,
  LTE_MeasObjectToAddModList_t           *MeasObj_list,
  LTE_ReportConfigToAddModList_t         *ReportConfig_list,
  LTE_QuantityConfig_t                   *quantityConfig,
  LTE_MeasIdToAddModList_t               *MeasId_list,
  LTE_MAC_MainConfig_t                   *mac_MainConfig,
  LTE_MeasGapConfig_t                    *measGapConfig,
  LTE_MobilityControlInfo_t              *mobilityInfo,
  LTE_SecurityConfigHO_t                 *securityConfigHO,
  struct LTE_MeasConfig__speedStatePars  *speedStatePars,
  LTE_RSRP_Range_t                       *rsrp,
  LTE_C_RNTI_t                           *cba_rnti,
  struct LTE_RRCConnectionReconfiguration_r8_IEs__dedicatedInfoNASList *dedicatedInfoNASList,
  LTE_SL_CommConfig_r12_t                *sl_CommConfig,
  LTE_SL_DiscConfig_r12_t                *sl_DiscConfig
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
  , LTE_SCellToAddMod_r10_t  *SCell_config
#endif
);
/**
\brief Generate an RRCConnectionReestablishment DL-CCCH-Message (eNB).  This routine configures SRB_ToAddMod (SRB1/SRB2) and
PhysicalConfigDedicated IEs.  The latter does not enable periodic CQI reporting (PUCCH format 2/2a/2b) or SRS.
@param ctxt_pP Running context
@param ue_context_pP UE context
@param CC_id         Component Carrier ID
@param buffer Pointer to PER-encoded ASN.1 description of DL-CCCH-Message PDU
@param transmission_mode Transmission mode for UE (1-9)
@param Transaction_id Transaction_ID for this message
@param SRB_configList Pointer (returned) to SRB1_config/SRB2_config(later) IEs for this UE
@param physicalConfigDedicated Pointer (returned) to PhysicalConfigDedicated IE for this UE
@returns Size of encoded bit stream in bytes*/
uint8_t
do_RRCConnectionReestablishment(
  const protocol_ctxt_t     *const ctxt_pP,
  rrc_eNB_ue_context_t      *const ue_context_pP,
  int                              CC_id,
  uint8_t                   *const buffer,
  const uint8_t                    transmission_mode,
  const uint8_t                    Transaction_id,
  LTE_SRB_ToAddModList_t               **SRB_configList,
  struct LTE_PhysicalConfigDedicated   **physicalConfigDedicated);

/**
\brief Generate an RRCConnectionReestablishmentReject DL-CCCH-Message (eNB).
@param Mod_id Module ID of eNB
@param buffer Pointer to PER-encoded ASN.1 description of DL-CCCH-Message PDU
@returns Size of encoded bit stream in bytes*/
uint8_t
do_RRCConnectionReestablishmentReject(
  uint8_t                    Mod_id,
  uint8_t                   *const buffer);

/**
\brief Generate an RRCConnectionReject DL-CCCH-Message (eNB).
@param Mod_id Module ID of eNB
@param buffer Pointer to PER-encoded ASN.1 description of DL-CCCH-Message PDU
@returns Size of encoded bit stream in bytes*/
uint8_t
do_RRCConnectionReject(
  uint8_t                    Mod_id,
  uint8_t                   *const buffer);

/**
\brief Generate an RRCConnectionRequest UL-CCCH-Message (UE) based on random string or S-TMSI.  This
routine only generates an mo-data establishment cause.
@param Mod_id Module ID of eNB
@param buffer Pointer to PER-encoded ASN.1 description of UL-DCCH-Message PDU
@param transaction_id Transaction index
@returns Size of encoded bit stream in bytes*/

uint8_t do_RRCConnectionRelease(uint8_t Mod_id, uint8_t *buffer,int Transaction_id);

/***
 * \brief Generate an MCCH-Message (eNB). This routine configures MBSFNAreaConfiguration (PMCH-InfoList and Subframe Allocation for MBMS data)
 * @param buffer Pointer to PER-encoded ASN.1 description of MCCH-Message PDU
 * @returns Size of encoded bit stream in bytes
*/
uint8_t do_MCCHMessage(uint8_t *buffer);
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
/***
 * \brief Generate an MCCH-Message (eNB). This routine configures MBSFNAreaConfiguration (PMCH-InfoList and Subframe Allocation for MBMS data)
 * @param buffer Pointer to PER-encoded ASN.1 description of MCCH-Message PDU
 * @returns Size of encoded bit stream in bytes
*/
uint8_t do_MBSFNAreaConfig(uint8_t Mod_id,
                           uint8_t sync_area,
                           uint8_t *buffer,
                           LTE_MCCH_Message_t *mcch_message,
                           LTE_MBSFNAreaConfiguration_r9_t **mbsfnAreaConfiguration);
#endif

uint8_t do_MeasurementReport(uint8_t Mod_id, uint8_t *buffer,int measid,int phy_id,long rsrp_s,long rsrq_s,long rsrp_t,long rsrq_t);

uint8_t do_DLInformationTransfer(uint8_t Mod_id, uint8_t **buffer, uint8_t transaction_id, uint32_t pdu_length, uint8_t *pdu_buffer);

uint8_t do_Paging(uint8_t Mod_id, uint8_t *buffer, ue_paging_identity_t ue_paging_identity, cn_domain_t cn_domain);

uint8_t do_ULInformationTransfer(uint8_t **buffer, uint32_t pdu_length, uint8_t *pdu_buffer);

int do_HandoverPreparation(char *ho_buf, int ho_size, LTE_UE_EUTRA_Capability_t *ue_eutra_cap, int rrc_size);

int do_HandoverCommand(char *ho_buf, int ho_size, char *rrc_buf, int rrc_size);

OAI_UECapability_t *fill_ue_capability(char *LTE_UE_EUTRA_Capability_xer);

uint8_t
do_UECapabilityEnquiry(
  const protocol_ctxt_t *const ctxt_pP,
  uint8_t               *const buffer,
  const uint8_t                Transaction_id
);

uint8_t do_SecurityModeCommand(
  const protocol_ctxt_t *const ctxt_pP,
  uint8_t *const buffer,
  const uint8_t Transaction_id,
  const uint8_t cipheringAlgorithm,
  const uint8_t integrityProtAlgorithm);
