/* Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
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

/*! \file RRC/LITE/defs_NR.h
* \brief NR RRC struct definitions and function prototypes
* \author Navid Nikaein, Raymond Knopp and WEI-TAI CHEN
* \date 2010 - 2014, 2018
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr, raymond.knopp@eurecom.fr, kroempa@gmail.com.tw
*/

#ifndef __OPENAIR_RRC_DEFS_NR_H__
#define __OPENAIR_RRC_DEFS_NR_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "collection/tree.h"
#include "rrc_types_NR.h"
#include "COMMON/platform_constants.h"
#include "COMMON/platform_types.h"

//#include "COMMON/mac_rrc_primitives.h"
#if defined(Rel15)
#include "SIB1.h"
//#include "SystemInformation.h"
//#include "RRCConnectionReconfiguration.h"
//#include "RRCConnectionReconfigurationComplete.h"
//#include "RRCConnectionSetup.h"
//#include "RRCConnectionSetupComplete.h"
//#include "RRCConnectionRequest.h"
//#include "RRCConnectionReestablishmentRequest.h"
//#include "BCCH-DL-SCH-Message.h"
#include "BCCH-BCH-Message.h"
//#include "MCCH-Message.h"
//#include "MBSFNAreaConfiguration-r9.h"
//#include "SCellToAddMod-r10.h"
//#include "AS-Config.h"
//#include "AS-Context.h"
#include "UE-NR-Capability.h"
#include "MeasResults.h"
#endif
//-------------------

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

/* TODO: be sure this include is correct.
 * It solves a problem of compilation of the RRH GW,
 * issue #186.
 */
#if !defined(ENABLE_ITTI)
# include "as_message.h"
#endif

#if defined(ENABLE_USE_MME)
# include "commonDef.h"
#endif

#if ENABLE_RAL
# include "collection/hashtable/obj_hashtable.h"
#endif



/*I will change the name of the structure for compile purposes--> hope not to undo this process*/

typedef unsigned int uid_NR_t;
#define UID_LINEAR_ALLOCATOR_BITMAP_SIZE_NR (((NUMBER_OF_UE_MAX_NR/8)/sizeof(unsigned int)) + 1)

typedef struct uid_linear_allocator_NR_s {
  unsigned int   bitmap[UID_LINEAR_ALLOCATOR_BITMAP_SIZE_NR];
} uid_allocator_NR_t;


#define PROTOCOL_RRC_CTXT_UE_FMT                PROTOCOL_CTXT_FMT
#define PROTOCOL_RRC_CTXT_UE_ARGS(CTXT_Pp)      PROTOCOL_CTXT_ARGS(CTXT_Pp)

#define PROTOCOL_RRC_CTXT_FMT                   PROTOCOL_CTXT_FMT
#define PROTOCOL_RRC_CTXT_ARGS(CTXT_Pp)         PROTOCOL_CTXT_ARGS(CTXT_Pp)


#define UE_MODULE_INVALID ((module_id_t) ~0) // FIXME attention! depends on type uint8_t!!!
#define UE_INDEX_INVALID  ((module_id_t) ~0) // FIXME attention! depends on type uint8_t!!! used to be -1

typedef enum {
  RRC_OK=0,
  RRC_ConnSetup_failed,
  RRC_PHY_RESYNCH,
  RRC_Handover_failed,
  RRC_HO_STARTED
} RRC_status_NR_t;

typedef enum UE_STATE_NR_e {
  RRC_INACTIVE=0,
  RRC_IDLE,
  RRC_SI_RECEIVED,
  RRC_CONNECTED,
  RRC_RECONFIGURED,
  RRC_HO_EXECUTION
} UE_STATE_NR_t;

typedef enum HO_STATE_NR_e {
  HO_IDLE=0,
  HO_MEASURMENT,
  HO_PREPARE,
  HO_CMD, // initiated by the src eNB
  HO_COMPLETE // initiated by the target eNB
} HO_STATE_NR_t;

//#define NUMBER_OF_UE_MAX MAX_MOBILES_PER_RG
#define RRM_FREE(p)       if ( (p) != NULL) { free(p) ; p=NULL ; }
#define RRM_MALLOC(t,n)   (t *) malloc16( sizeof(t) * n )
#define RRM_CALLOC(t,n)   (t *) malloc16( sizeof(t) * n)
#define RRM_CALLOC2(t,s)  (t *) malloc16( s )

#define MAX_MEAS_OBJ                                  6
#define MAX_MEAS_CONFIG                               6
#define MAX_MEAS_ID                                   6

#define PAYLOAD_SIZE_MAX                              1024
#define RRC_BUF_SIZE                                  255
#define UNDEF_SECURITY_MODE                           0xff
#define NO_SECURITY_MODE                              0x20

/* TS 36.331: RRC-TransactionIdentifier ::= INTEGER (0..3) */
#define RRC_TRANSACTION_IDENTIFIER_NUMBER             3

typedef struct {
  unsigned short                                      transport_block_size;      /*!< \brief Minimum PDU size in bytes provided by RLC to MAC layer interface */
  unsigned short                                      max_transport_blocks;      /*!< \brief Maximum PDU size in bytes provided by RLC to MAC layer interface */
  unsigned long                                       Guaranteed_bit_rate;       /*!< \brief Guaranteed Bit Rate (average) to be offered by MAC layer scheduling*/
  unsigned long                                       Max_bit_rate;              /*!< \brief Maximum Bit Rate that can be offered by MAC layer scheduling*/
  uint8_t                                             Delay_class;               /*!< \brief Delay class offered by MAC layer scheduling*/
  uint8_t                                             Target_bler;               /*!< \brief Target Average Transport Block Error rate*/
  uint8_t                                             Lchan_t;                   /*!< \brief Logical Channel Type (BCCH,CCCH,DCCH,DTCH_B,DTCH,MRBCH)*/
} __attribute__ ((__packed__))  LCHAN_DESC_NR;

typedef struct UE_RRC_INFO_NR_s {
  UE_STATE_NR_t                                       State;
  uint8_t                                             SIB1systemInfoValueTag;
  uint32_t                                            SIStatus;
  uint32_t                                            SIcnt;
#if defined(Rel10) || defined(Rel14)
  uint8_t                                             MCCHStatus[8];             // MAX_MBSFN_AREA
#endif
  uint8_t                                             SIwindowsize;              //!< Corresponds to the SIB1 si-WindowLength parameter. The unit is ms. Possible values are (final): 1,2,5,10,15,20,40
  uint8_t                                             handoverTarget;
  //HO_STATE_t ho_state;
  uint16_t                                            SIperiod;                  //!< Corresponds to the SIB1 si-Periodicity parameter (multiplied by 10). Possible values are (final): 80,160,320,640,1280,2560,5120
  unsigned short                                      UE_index;
  uint32_t                                            T300_active;
  uint32_t                                            T300_cnt;
  uint32_t                                            T304_active;
  uint32_t                                            T304_cnt;
  uint32_t                                            T310_active;
  uint32_t                                            T310_cnt;
  uint32_t                                            N310_cnt;
  uint32_t                                            N311_cnt;
  rnti_t                                              rnti;
} __attribute__ ((__packed__)) UE_RRC_INFO_NR;

typedef struct UE_S_TMSI_NR_s {
  boolean_t                                           presence;
  mme_code_t                                          mme_code;
  m_tmsi_t                                            m_tmsi;
} __attribute__ ((__packed__)) UE_S_TMSI_NR;


typedef enum e_rab_satus_NR_e {
  E_RAB_STATUS_NEW_NR,
  E_RAB_STATUS_DONE_NR,           // from the eNB perspective
  E_RAB_STATUS_ESTABLISHED_NR,    // get the reconfigurationcomplete form UE
  E_RAB_STATUS_FAILED_NR,
} e_rab_status_NR_t;

typedef struct e_rab_param_NR_s {
  e_rab_t param;
  uint8_t status;
  uint8_t xid; // transaction_id
} __attribute__ ((__packed__)) e_rab_param_NR_t;


typedef struct HANDOVER_INFO_NR_s {
  uint8_t                                             ho_prepare;
  uint8_t                                             ho_complete;
  uint8_t                                             modid_s;            //module_idP of serving cell
  uint8_t                                             modid_t;            //module_idP of target cell
  uint8_t                                             ueid_s;             //UE index in serving cell
  uint8_t                                             ueid_t;             //UE index in target cell

  // NR not define at this moment
  //AS_Config_t                                       as_config;          /* these two parameters are taken from 36.331 section 10.2.2: HandoverPreparationInformation-r8-IEs */
  //AS_Context_t                                      as_context;         /* They are mandatory for HO */

  uint8_t                                             buf[RRC_BUF_SIZE];  /* ASN.1 encoded handoverCommandMessage */
  int                                                 size;               /* size of above message in bytes */
} HANDOVER_INFO_NR;


#define RRC_HEADER_SIZE_MAX 64
#define RRC_BUFFER_SIZE_MAX 1024

typedef struct {
  char                                                Payload[RRC_BUFFER_SIZE_MAX];
  char                                                Header[RRC_HEADER_SIZE_MAX];
  char                                                payload_size;
} RRC_BUFFER_NR;

#define RRC_BUFFER_SIZE_NR                            sizeof(RRC_BUFFER_NR)


typedef struct RB_INFO_NR_s {
  uint16_t                                            Rb_id;  //=Lchan_id
  LCHAN_DESC Lchan_desc[2]; 
  //MAC_MEAS_REQ_ENTRY *Meas_entry; //may not needed for NB-IoT
} RB_INFO_NR;

typedef struct SRB_INFO_NR_s {
  uint16_t                                            Srb_id;         //=Lchan_id
  RRC_BUFFER                                          Rx_buffer;
  RRC_BUFFER                                          Tx_buffer;
  LCHAN_DESC                                          Lchan_desc[2];
  unsigned int                                        Trans_id;
  uint8_t                                             Active;
} SRB_INFO_NR;


typedef struct RB_INFO_TABLE_ENTRY_NR_s {
  RB_INFO_NR                                          Rb_info;
  uint8_t                                             Active;
  uint32_t                                            Next_check_frame;
  uint8_t                                             Status;
} RB_INFO_TABLE_ENTRY_NR;

typedef struct SRB_INFO_TABLE_ENTRY_NR_s {
  SRB_INFO_NR                                         Srb_info;
  uint8_t                                             Active;
  uint8_t                                             Status;
  uint32_t                                            Next_check_frame;
} SRB_INFO_TABLE_ENTRY_NR;

typedef struct MEAS_REPORT_LIST_NR_s {
  MeasId_t                                            measId;
  //CellsTriggeredList  cellsTriggeredList;//OPTIONAL
  uint32_t                                            numberOfReportsSent;
} MEAS_REPORT_LIST_NR;

typedef struct HANDOVER_INFO_UE_NR_s {
  PhysCellId_t                                        targetCellId;
  uint8_t                                             measFlag;
} HANDOVER_INFO_UE_NR;

//NB-IoT eNB_RRC_UE_NB_IoT_s--(used as a context in eNB --> ue_context in rrc_eNB_ue_context)------
typedef struct gNB_RRC_UE_s {

  uint8_t                                             primaryCC_id;
  
  //SCellToAddMod_t                               sCell_config[2];

  SRB_ToAddModList_t*                                 SRB_configList;//for SRB1 and SRB1bis
  SRB_ToAddModList_t*                                 SRB_configList2[RRC_TRANSACTION_IDENTIFIER_NUMBER];
  DRB_ToAddModList_t*                                 DRB_configList; //for all the DRBs
  DRB_ToAddModList_t*                                 DRB_configList2[RRC_TRANSACTION_IDENTIFIER_NUMBER]; //for the configured DRBs of a xid
  uint8_t                                             DRB_active[8];//in LTE was 8 

  // NR not define at this moment
  //struct PhysicalConfigDedicated*                   physicalConfigDedicated_NR;
  
  struct SPS_Config*                                  sps_Config;
  MeasObjectToAddMod_t*                               MeasObj[MAX_MEAS_OBJ];
  struct ReportConfigToAddMod*                        ReportConfig[MAX_MEAS_CONFIG];
  struct QuantityConfig*                              QuantityConfig;
  struct MeasIdToAddMod*                              MeasId[MAX_MEAS_ID];

  // NR not define at this moment
  //MAC_MainConfig_t*                                 mac_MainConfig_NR;

  MeasGapConfig_t*                                    measGapConfig;

  SRB_INFO_NR                                         SI;
  SRB_INFO_NR                                         Srb0;
  SRB_INFO_TABLE_ENTRY_NR                             Srb1;
  SRB_INFO_TABLE_ENTRY_NR                             Srb2;

  MeasConfig_t*                                       measConfig;
  HANDOVER_INFO_NR*                                   handover_info;


#if defined(ENABLE_SECURITY)
  /* KeNB as derived from KASME received from EPC */
  uint8_t                                             kenb[32];
#endif

  /* Used integrity/ciphering algorithms */
  //Specs. TS 38.331 V15.1.0 pag 432 Change position of chipering enumerative w.r.t previous version
  e_CipheringAlgorithm                                ciphering_algorithm; 
  e_IntegrityProtAlgorithm                            integrity_algorithm;

  uint8_t                                             Status;
  rnti_t                                              rnti;
  uint64_t                                            random_ue_identity;



  /* Information from UE RRC ConnectionRequest */
  UE_S_TMSI_NR                                        Initialue_identity_s_TMSI;
  
  /* NR not define at this moment
  EstablishmentCause_t                             establishment_cause_NR; //different set for NB-IoT
  
  /* NR not define at this moment
  /* Information from UE RRC ConnectionReestablishmentRequest  */
  //ReestablishmentCause_t                           reestablishment_cause_NR; //different set for NB_IoT

  /* UE id for initial connection to S1AP */
  uint16_t                                            ue_initial_id;

  /* Information from S1AP initial_context_setup_req */
  uint32_t                                            eNB_ue_s1ap_id :24;

  security_capabilities_t                             security_capabilities;

  /* Total number of e_rab already setup in the list */ //NAS list?
  uint8_t                                             setup_e_rabs;
  /* Number of e_rab to be setup in the list */ //NAS list?
  uint8_t                                             nb_of_e_rabs;
  /* list of e_rab to be setup by RRC layers */
  e_rab_param_NR_t                                    e_rab[NB_RB_MAX_NB_IOT];//[S1AP_MAX_E_RAB];

  // LG: For GTPV1 TUNNELS
  uint32_t                                            enb_gtp_teid[S1AP_MAX_E_RAB];
  transport_layer_addr_t                              enb_gtp_addrs[S1AP_MAX_E_RAB];
  rb_id_t                                             enb_gtp_ebi[S1AP_MAX_E_RAB];

 //Which timers are referring to?
  uint32_t                                            ul_failure_timer;
  uint32_t                                            ue_release_timer;
  //threshold of the release timer--> set in RRCConnectionRelease
  uint32_t                                            ue_release_timer_thres;
} gNB_RRC_UE_t;
//--------------------------------------------------------------------------------

typedef uid_NR_t ue_uid_t;


//generally variable called: ue_context_pP
typedef struct rrc_gNB_ue_context_s {

  /* Tree related data */
  RB_ENTRY(rrc_gNB_ue_context_s)         entries;

  /* Uniquely identifies the UE between MME and eNB within the eNB.
   * This id is encoded on 24bits.
   */
  rnti_t                                    ue_id_rnti;

  // another key for protocol layers but should not be used as a key for RB tree
  ue_uid_t                                  local_uid;

  /* UE id for initial connection to S1AP */
  struct gNB_RRC_UE_s                       ue_context; //context of ue in the e-nB

} rrc_gNB_ue_context_t;


//called "carrier"--> data from PHY layer
typedef struct {

  // buffer that contains the encoded messages
  uint8_t							                      *MIB_NR;
  uint8_t							                      sizeof_MIB_NR;
/*
  uint8_t                                   *SIB1_NB_IoT;
  uint8_t                                   sizeof_SIB1_NB_IoT;
  uint8_t                         	        *SIB23_NB_IoT;
  uint8_t                        	          sizeof_SIB23_NB_IoT;
*/

/*
  //not actually implemented in OAI
  uint8_t                                   *SIB4_NB_IoT;
  uint8_t                                   sizeof_SIB4_NB_IoT;
  uint8_t                                   *SIB5_NB_IoT;
  uint8_t                                   sizeof_SIB5_NB_IoT;
  uint8_t                                   *SIB14_NB_IoT;
  uint8_t                                   sizeof_SIB14_NB_IoT;
  uint8_t                                   *SIB16_NB_IoT;
  uint8_t                                   sizeof_SIB16_NB_IoT;
*/
  //TS 36.331 V14.2.1
//  uint8_t                                 *SIB15_NB;
//  uint8_t                                 sizeof_SIB15_NB;
//  uint8_t                                 *SIB20_NB;
//  uint8_t                                 sizeof_SIB20_NB;
//  uint8_t                                 *SIB22_NB;
//  uint8_t                                 sizeof_SIB22_NB;

  //implicit parameters needed
  int                                       Ncp; //cyclic prefix for DL
  int								                        Ncp_UL; //cyclic prefix for UL
  int                                       p_eNB; //number of tx antenna port
  int								                        p_rx_eNB; //number of receiving antenna ports
  uint32_t                                  dl_CarrierFreq; //detected by the UE
  uint32_t                                  ul_CarrierFreq; //detected by the UE
  uint16_t                                  physCellId; //not stored in the MIB-NB but is getting through NPSS/NSSS

  //are the only static one (memory has been already allocated)
  BCCH_BCH_Message_t                        mib_NR;
  
  /*
  BCCH_DL_SCH_Message_NR_t                  siblock1_NB_IoT; //SIB1-NB
  BCCH_DL_SCH_Message_NR_t                  systemInformation_NB_IoT; //SI
  */
  SIB1_t     		                            *sib1_NR;
  /*
  SIB2_t   	                                *sib2_NR;
  SIB3_t   	                                *sib3_NR;
  //not implemented yet
  SIB4_t    	                              *sib4_NR;
  SIB5_t     	                              *sib5_NR;
  */


  SRB_INFO_NR                               SI;
  SRB_INFO_NR                               Srb0;

  uint8_t                                   **MCCH_MESSAGE; //  probably not needed , but added to remove errors
  uint8_t                                   sizeof_MCCH_MESSAGE[8];// but added to remove errors
  SRB_INFO_NR                               MCCH_MESS[8];// MAX_MBSFN_AREA

  /*future implementation TS 36.331 V14.2.1
  SystemInformationBlockType15_NB_r14_t     *sib15;
  SystemInformationBlockType20_NB_r14_t     *sib20;
  SystemInformationBlockType22_NB_r14_t     *sib22;

  uint8_t							                      SCPTM_flag;
  uint8_t							                      sizeof_SC_MCHH_MESS[];
  SC_MCCH_Message_NR_t				              scptm;*/


} rrc_gNB_carrier_data_t;
//---------------------------------------------------



//---NR---(completely change)---------------------
typedef struct gNB_RRC_INST_s {

  eth_params_t                                        eth_params_s;
  rrc_gNB_carrier_data_t                              carrier[MAX_NUM_CCs];
  uid_allocator_NR_t                                  uid_allocator; // for rrc_ue_head
  RB_HEAD(rrc_ue_tree_NR_s, rrc_gNB_ue_context_s)     rrc_ue_head; // ue_context tree key search by rnti
  
  uint8_t                                             HO_flag;
  uint8_t                                             Nb_ue;

  hash_table_t                                        *initial_id2_s1ap_ids; // key is    content is rrc_ue_s1ap_ids_t
  hash_table_t                                        *s1ap_id2_s1ap_ids   ; // key is    content is rrc_ue_s1ap_ids_t

  //RRC configuration
#if defined(ENABLE_ITTI)
  gNB_RrcConfigurationReq                             configuration;//rrc_messages_types.h
#endif
  // other PLMN parameters
  /// Mobile country code
  int mcc;
  /// Mobile network code
  int mnc;
  /// number of mnc digits
  int mnc_digit_length;

  // other RAN parameters
  int srb1_timer_poll_retransmit;
  int srb1_poll_pdu;
  int srb1_poll_byte;
  int srb1_max_retx_threshold;
  int srb1_timer_reordering;
  int srb1_timer_status_prohibit;
  int srs_enable[MAX_NUM_CCs];

} gNB_RRC_INST;

//#define RRC_HEADER_SIZE_MAX_NR 64
#define MAX_UE_CAPABILITY_SIZE_NR 255

//not needed for the moment
typedef struct OAI_UECapability_NR_s {
 uint8_t sdu[MAX_UE_CAPABILITY_SIZE_NR];
 uint8_t sdu_size;
////NR------
  UE_NR_Capability_t  UE_Capability_NR; //replace the UE_EUTRA_Capability of LTE
} OAI_UECapability_NR_t;


typedef struct UE_RRC_INST_NR_s {
  Rrc_State_NR_t     RrcState;
  Rrc_Sub_State_NR_t RrcSubState;
# if defined(ENABLE_USE_MME)
  plmn_t          plmnID;
  Byte_t          rat;
  as_nas_info_t   initialNasMsg;
# endif
  OAI_UECapability_NR_t *UECap;
  uint8_t *UECapability;
  uint8_t UECapability_size;

  UE_RRC_INFO_NR              Info[NB_SIG_CNX_UE];
  
  SRB_INFO_NR                 Srb0[NB_SIG_CNX_UE];
  SRB_INFO_TABLE_ENTRY_NR     Srb1[NB_CNX_UE];
  SRB_INFO_TABLE_ENTRY_NR     Srb2[NB_CNX_UE];
  HANDOVER_INFO_UE_NR         HandoverInfoUe;
  /*
  uint8_t *SIB1[NB_CNX_UE];
  uint8_t sizeof_SIB1[NB_CNX_UE];
  uint8_t *SI[NB_CNX_UE];
  uint8_t sizeof_SI[NB_CNX_UE];
  uint8_t SIB1Status[NB_CNX_UE];
  uint8_t SIStatus[NB_CNX_UE];
  SIB1_t *sib1[NB_CNX_UE];
  SystemInformation_t *si[NB_CNX_UE]; //!< Temporary storage for an SI message. Decoding happens in decode_SI().
  
  SystemInformationBlockType2_t *sib2[NB_CNX_UE];
  /*
  SystemInformationBlockType3_t *sib3[NB_CNX_UE];
  SystemInformationBlockType4_t *sib4[NB_CNX_UE];
  SystemInformationBlockType5_t *sib5[NB_CNX_UE];
  SystemInformationBlockType6_t *sib6[NB_CNX_UE];
  SystemInformationBlockType7_t *sib7[NB_CNX_UE];
  SystemInformationBlockType8_t *sib8[NB_CNX_UE];
  SystemInformationBlockType9_t *sib9[NB_CNX_UE];
  SystemInformationBlockType10_t *sib10[NB_CNX_UE];
  SystemInformationBlockType11_t *sib11[NB_CNX_UE];

#if defined(Rel10) || defined(Rel14)
  uint8_t                           MBMS_flag;
  uint8_t *MCCH_MESSAGE[NB_CNX_UE];
  uint8_t sizeof_MCCH_MESSAGE[NB_CNX_UE];
  uint8_t MCCH_MESSAGEStatus[NB_CNX_UE];
  MBSFNAreaConfiguration_r9_t       *mcch_message[NB_CNX_UE];
  SystemInformationBlockType12_r9_t *sib12[NB_CNX_UE];
  SystemInformationBlockType13_r9_t *sib13[NB_CNX_UE];
#endif
#ifdef CBA
  uint8_t                         num_active_cba_groups;
  uint16_t                        cba_rnti[NUM_MAX_CBA_GROUP];
#endif
  uint8_t                         num_srb;
  struct SRB_ToAddMod             *SRB1_config[NB_CNX_UE];
  struct SRB_ToAddMod             *SRB2_config[NB_CNX_UE];
  struct DRB_ToAddMod             *DRB_config[NB_CNX_UE][8];
  rb_id_t                         *defaultDRB; // remember the ID of the default DRB
  MeasObjectToAddMod_t            *MeasObj[NB_CNX_UE][MAX_MEAS_OBJ];
  struct ReportConfigToAddMod     *ReportConfig[NB_CNX_UE][MAX_MEAS_CONFIG];
  */
  struct QuantityConfig           *QuantityConfig[NB_CNX_UE];
  /*
  struct MeasIdToAddMod           *MeasId[NB_CNX_UE][MAX_MEAS_ID];
  MEAS_REPORT_LIST      *measReportList[NB_CNX_UE][MAX_MEAS_ID];
  uint32_t           measTimer[NB_CNX_UE][MAX_MEAS_ID][6]; // 6 neighboring cells
  RSRP_Range_t                    s_measure;
  struct MeasConfig__speedStatePars *speedStatePars;
  struct PhysicalConfigDedicated  *physicalConfigDedicated[NB_CNX_UE];
  struct SPS_Config               *sps_Config[NB_CNX_UE];
  MAC_MainConfig_t                *mac_MainConfig[NB_CNX_UE];
  MeasGapConfig_t                 *measGapConfig[NB_CNX_UE];
  double                          filter_coeff_rsrp; // [7] ???
  double                          filter_coeff_rsrq; // [7] ???
  float                           rsrp_db[7];
  float                           rsrq_db[7];
  float                           rsrp_db_filtered[7];
  float                           rsrq_db_filtered[7];
#if ENABLE_RAL
  obj_hash_table_t               *ral_meas_thresholds;
  ral_transaction_id_t            scan_transaction_id;
#endif
#if defined(ENABLE_SECURITY)
  // KeNB as computed from parameters within USIM card //
  uint8_t kenb[32];
#endif

  // Used integrity/ciphering algorithms //
  CipheringAlgorithm_r12_t                          ciphering_algorithm;
  e_SecurityAlgorithmConfig__integrityProtAlgorithm integrity_algorithm;
  */
}UE_RRC_INST_NR;




#include "proto_NR.h" //should be put here otherwise compilation error

#endif
/** @} */
