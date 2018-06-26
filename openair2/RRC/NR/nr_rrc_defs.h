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
* \author Navid Nikaein, Raymond Knopp 
* \date 2010 - 2014, 2018
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr, raymond.knopp@eurecom.fr
*/

#ifndef __OPENAIR_RRC_DEFS_NR_H__
#define __OPENAIR_RRC_DEFS_NR_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "collection/tree.h"
#include "nr_rrc_types.h"

#include "COMMON/platform_constants.h"
#include "COMMON/platform_types.h"
#include "RRC/LTE/rrc_defs.h"
//#include "LAYER2/RLC/rlc.h"

//#include "COMMON/mac_rrc_primitives.h"
#if defined(NR_Rel15)
#include "NR_SIB1.h"
//#include "SystemInformation.h"
//#include "RRCConnectionReconfiguration.h"
#include "NR_RRCReconfigurationComplete.h"
#include "NR_RRCReconfiguration.h"
//#include "RRCConnectionReconfigurationComplete.h"
//#include "RRCConnectionSetup.h"
//#include "RRCConnectionSetupComplete.h"
//#include "RRCConnectionRequest.h"
//#include "RRCConnectionReestablishmentRequest.h"
//#include "BCCH-DL-SCH-Message.h"
#include "NR_BCCH-BCH-Message.h"
//#include "MCCH-Message.h"
//#include "MBSFNAreaConfiguration-r9.h"
//#include "SCellToAddMod-r10.h"
//#include "AS-Config.h"
//#include "AS-Context.h"
#include "NR_UE-NR-Capability.h"
#include "NR_MeasResults.h"
#include "NR_ServingCellConfigCommon.h"
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


/*I will change the name of the structure for compile purposes--> hope not to undo this process*/

typedef unsigned int uid_nr_t;
#define NR_UID_LINEAR_ALLOCATOR_BITMAP_SIZE (((NUMBER_OF_NR_UE_MAX/8)/sizeof(unsigned int)) + 1)

/*typedef struct nr_uid_linear_allocator_s {
  unsigned int   bitmap[NR_UID_LINEAR_ALLOCATOR_BITMAP_SIZE];
} nr_uid_allocator_t;*/
    

#define PROTOCOL_NR_RRC_CTXT_UE_FMT                PROTOCOL_CTXT_FMT
#define PROTOCOL_NR_RRC_CTXT_UE_ARGS(CTXT_Pp)      PROTOCOL_CTXT_ARGS(CTXT_Pp)

#define PROTOCOL_NR_RRC_CTXT_FMT                   PROTOCOL_CTXT_FMT
#define PROTOCOL_NR_RRC_CTXT_ARGS(CTXT_Pp)         PROTOCOL_CTXT_ARGS(CTXT_Pp)


#define NR_UE_MODULE_INVALID ((module_id_t) ~0) // FIXME attention! depends on type uint8_t!!!
#define NR_UE_INDEX_INVALID  ((module_id_t) ~0) // FIXME attention! depends on type uint8_t!!! used to be -1

typedef enum {
  NR_RRC_OK=0,
  NR_RRC_ConnSetup_failed,
  NR_RRC_PHY_RESYNCH,
  NR_RRC_Handover_failed,
  NR_RRC_HO_STARTED
} NR_RRC_status_t;

typedef enum UE_STATE_NR_e {
  NR_RRC_INACTIVE=0,
  NR_RRC_IDLE,
  NR_RRC_SI_RECEIVED,
  NR_RRC_CONNECTED,
  NR_RRC_RECONFIGURED,
  NR_RRC_HO_EXECUTION
} NR_UE_STATE_t;


//#define NUMBER_OF_NR_UE_MAX MAX_MOBILES_PER_RG
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
#define NR_RRC_TRANSACTION_IDENTIFIER_NUMBER             3

typedef struct {
  unsigned short                                      transport_block_size;      /*!< \brief Minimum PDU size in bytes provided by RLC to MAC layer interface */
  unsigned short                                      max_transport_blocks;      /*!< \brief Maximum PDU size in bytes provided by RLC to MAC layer interface */
  unsigned long                                       Guaranteed_bit_rate;       /*!< \brief Guaranteed Bit Rate (average) to be offered by MAC layer scheduling*/
  unsigned long                                       Max_bit_rate;              /*!< \brief Maximum Bit Rate that can be offered by MAC layer scheduling*/
  uint8_t                                             Delay_class;               /*!< \brief Delay class offered by MAC layer scheduling*/
  uint8_t                                             Target_bler;               /*!< \brief Target Average Transport Block Error rate*/
  uint8_t                                             Lchan_t;                   /*!< \brief Logical Channel Type (BCCH,CCCH,DCCH,DTCH_B,DTCH,MRBCH)*/
} __attribute__ ((__packed__))  NR_LCHAN_DESC;

typedef struct UE_RRC_INFO_NR_s {
  NR_UE_STATE_t                                       State;
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
} __attribute__ ((__packed__)) NR_UE_RRC_INFO;

typedef struct UE_S_TMSI_NR_s {
  boolean_t                                           presence;
  mme_code_t                                          mme_code;
  m_tmsi_t                                            m_tmsi;
} __attribute__ ((__packed__)) NR_UE_S_TMSI;


typedef enum nr_e_rab_satus_e {
  NR_E_RAB_STATUS_NEW,
  NR_E_RAB_STATUS_DONE,           // from the gNB perspective
  NR_E_RAB_STATUS_ESTABLISHED,    // get the reconfigurationcomplete form UE
  NR_E_RAB_STATUS_FAILED,
} nr_e_rab_status_t;

typedef struct nr_e_rab_param_s {
  e_rab_t param;
  uint8_t status;
  uint8_t xid; // transaction_id
} __attribute__ ((__packed__)) nr_e_rab_param_t;


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
} NR_HANDOVER_INFO;


#define NR_RRC_HEADER_SIZE_MAX 64
#define NR_RRC_BUFFER_SIZE_MAX 1024

typedef struct {
  char                                                Payload[NR_RRC_BUFFER_SIZE_MAX];
  char                                                Header[NR_RRC_HEADER_SIZE_MAX];
  char                                                payload_size;
} NR_RRC_BUFFER;

#define NR_RRC_BUFFER_SIZE                            sizeof(RRC_BUFFER_NR)


typedef struct RB_INFO_NR_s {
  uint16_t                                            Rb_id;  //=Lchan_id
  NR_LCHAN_DESC Lchan_desc[2]; 
  //MAC_MEAS_REQ_ENTRY *Meas_entry; //may not needed for NB-IoT
} NR_RB_INFO;

typedef struct NR_SRB_INFO_s {
  uint16_t                                            Srb_id;         //=Lchan_id
  NR_RRC_BUFFER                                          Rx_buffer;
  NR_RRC_BUFFER                                          Tx_buffer;
  NR_LCHAN_DESC                                          Lchan_desc[2];
  unsigned int                                        Trans_id;
  uint8_t                                             Active;
} NR_SRB_INFO;


typedef struct RB_INFO_TABLE_ENTRY_NR_s {
  NR_RB_INFO                                          Rb_info;
  uint8_t                                             Active;
  uint32_t                                            Next_check_frame;
  uint8_t                                             Status;
} NR_RB_INFO_TABLE_ENTRY;

typedef struct SRB_INFO_TABLE_ENTRY_NR_s {
  NR_SRB_INFO                                         Srb_info;
  uint8_t                                             Active;
  uint8_t                                             Status;
  uint32_t                                            Next_check_frame;
} NR_SRB_INFO_TABLE_ENTRY;

//called "carrier"--> data from PHY layer
typedef struct {

  // buffer that contains the encoded messages
  uint8_t							                      *MIB;
  uint8_t							                      sizeof_MIB;

  uint8_t                                   *ServingCellConfigCommon;
  uint8_t                                   sizeof_servingcellconfigcommon;

  //implicit parameters needed
  int                                       physCellId;
  int                                       Ncp; //cyclic prefix for DL
  int								                        Ncp_UL; //cyclic prefix for UL
  int                                       p_gNB; //number of tx antenna port
  int								                        p_rx_gNB; //number of receiving antenna ports
  uint32_t                                  dl_CarrierFreq; //detected by the UE
  uint32_t                                  ul_CarrierFreq; //detected by the UE
  
  //are the only static one (memory has been already allocated)
  NR_BCCH_BCH_Message_t                     mib;
  
  NR_ServingCellConfigCommon_t              *servingcellconfigcommon;


  NR_SRB_INFO                               SI;
  NR_SRB_INFO                               Srb0;

} rrc_gNB_carrier_data_t;
//---------------------------------------------------



//---NR---(completely change)---------------------
typedef struct gNB_RRC_INST_s {

  eth_params_t                                        eth_params_s;
  rrc_gNB_carrier_data_t                              carrier[MAX_NUM_CCs];
  uid_allocator_t                                     uid_allocator; // for rrc_ue_head
  RB_HEAD(rrc_nr_ue_tree_s, rrc_gNB_ue_context_s)     rrc_ue_head; // ue_context tree key search by rnti
  
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


#include "nr_rrc_proto.h" //should be put here otherwise compilation error

#endif
/** @} */
