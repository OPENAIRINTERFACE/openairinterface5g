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

/*! \file rrc_UE.c
 * \brief rrc procedures for UE / rrc procedures for FeMBMS UE
 * \author Navid Nikaein, Raymond Knopp and Javier Morgade
 * \date 2011 - 2014 / FeMBMS 2019
 * \version 1.0
 * \company Eurecom, Vicomtech
 * \email: navid.nikaein@eurecom.fr, raymond.knopp@eurecom.fr and javier.morgade@ieee.org
 */

#define RRC_UE
#define RRC_UE_C
#define _GNU_SOURCE

#include "assertions.h"
#include "hashtable.h"
#include "asn1_conversions.h"
#include "rrc_defs.h"
#include "rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "openair1/PHY/LTE_ESTIMATION/lte_estimation.h"
#include "LAYER2/RLC/rlc.h"
#include "COMMON/mac_rrc_primitives.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#ifndef CELLULAR
  #include "RRC/LTE/MESSAGES/asn1_msg.h"
#endif
#include "LTE_RRCConnectionRequest.h"
#include "LTE_RRCConnectionReconfiguration.h"
#include "LTE_UL-CCCH-Message.h"
#include "LTE_DL-CCCH-Message.h"
#include "LTE_UL-DCCH-Message.h"
#include "LTE_DL-DCCH-Message.h"
#include "LTE_BCCH-DL-SCH-Message.h"
#include "LTE_PCCH-Message.h"
#include "LTE_MCCH-Message.h"
#include "LTE_MeasConfig.h"
#include "LTE_MeasGapConfig.h"
#include "LTE_MeasObjectEUTRA.h"
#include "LTE_TDD-Config.h"
#include "LTE_UECapabilityEnquiry.h"
#include "LTE_UE-CapabilityRequest.h"
#include "RRC/NAS/nas_config.h"
#include "RRC/NAS/rb_config.h"
#if ENABLE_RAL
  #include "rrc_UE_ral.h"
#endif

#include "UTIL/OSA/osa_defs.h"

#include "pdcp.h"
#include "plmn_data.h"
#include "msc.h"
#include <common/utils/system.h>

#include "intertask_interface.h"
#include "targets/RT/USER/lte-softmodem.h"

#include "SIMULATION/TOOLS/sim.h" // for taus

#include "openair2/LAYER2/MAC/mac_extern.h"

#include "LTE_BCCH-BCH-Message-MBMS.h"
#include "LTE_BCCH-DL-SCH-Message-MBMS.h"
#include "LTE_SystemInformation-MBMS-r14.h"
#include "LTE_SystemInformationBlockType1-MBMS-r14.h"
#include "LTE_NonMBSFN-SubframeConfig-r14.h"


#include "LTE_SL-Preconfiguration-r12.h"

//for D2D
int ctrl_sock_fd;
struct sockaddr_in prose_app_addr;
static const char nsa_ipaddr[] = "127.0.0.1";
static int from_nr_ue_fd = -1;
static int to_nr_ue_fd = -1;
int slrb_id;
int send_ue_information = 0;

// for malloc_clear
#include "PHY/defs_UE.h"

extern void pdcp_config_set_security(
  const protocol_ctxt_t *const  ctxt_pP,
  pdcp_t          *const pdcp_pP,
  const rb_id_t         rb_idP,
  const uint16_t        lc_idP,
  const uint8_t         security_modeP,
  uint8_t         *const kRRCenc,
  uint8_t         *const kRRCint,
  uint8_t         *const  kUPenc);

// internal prototypes

void rrc_ue_process_securityModeCommand( const protocol_ctxt_t *const ctxt_pP, LTE_SecurityModeCommand_t *const securityModeCommand, const uint8_t eNB_index );

static int decode_SI( const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index );

static int decode_SI_MBMS( const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index );


static int decode_SIB1( const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index, const uint8_t rsrq, const uint8_t rsrp );

static int decode_SIB1_MBMS( const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index, const uint8_t rsrq, const uint8_t rsrp );

typedef struct rrc_dcch_data_copy_t
{
    LTE_DL_DCCH_Message_t *dl_dcch_msg;
} rrc_dcch_data_copy_t;

typedef struct rrc_nrue_cap_info_t
{
    uint8_t mesg[RRC_BUF_SIZE];
    size_t mesg_len;
    LTE_DL_DCCH_Message_t *dl_dcch_msg;
} rrc_nrue_cap_info_t;

static void rrc_ue_process_ueCapabilityEnquiry(const protocol_ctxt_t *const ctxt_pP,
                                               LTE_UECapabilityEnquiry_t *UECapabilityEnquiry,
                                               uint8_t eNB_index);

static void rrc_ue_process_nrueCapabilityEnquiry(const protocol_ctxt_t *const ctxt_pP,
                                                 LTE_UECapabilityEnquiry_t *UECapabilityEnquiry,
                                                 rrc_nrue_cap_info_t *nrue_cap_info,
                                                 uint8_t eNB_index);

/** \brief Generates/Encodes RRCConnnectionSetupComplete message at UE
 *  \param ctxt_pP Running context
 *  \param eNB_index Index of corresponding eNB/CH
 *  \param Transaction_id Transaction identifier
 *  \param sel_plmn_id selected PLMN Identity
 */
static void rrc_ue_generate_RRCConnectionSetupComplete(
    const protocol_ctxt_t *const ctxt_pP,
    const uint8_t eNB_index,
    const uint8_t Transaction_id,
    uint8_t sel_plmn_id);

/** \brief Generates/Encodes RRCConnectionReconfigurationComplete message at UE
 *  \param ctxt_pP Running context
 *  \param eNB_index Index of corresponding eNB/CH
 *  \param Transaction_id RRC transaction identifier
 */
static void rrc_ue_generate_RRCConnectionReconfigurationComplete(const protocol_ctxt_t *const ctxt_pP,
                                                                 const uint8_t eNB_index,
                                                                 const uint8_t Transaction_id,
                                                                 OCTET_STRING_t *str);

static void rrc_ue_generate_MeasurementReport(protocol_ctxt_t *const ctxt_pP, uint8_t eNB_index );

static void rrc_ue_generate_nrMeasurementReport(protocol_ctxt_t *const ctxt_pP, uint8_t eNB_index );

static uint8_t check_trigger_meas_event(
  module_id_t module_idP,
  frame_t frameP,
  uint8_t eNB_index,
  uint8_t ue_cnx_index,
  uint8_t meas_index,
  LTE_Q_OffsetRange_t ofn, LTE_Q_OffsetRange_t ocn, LTE_Hysteresis_t hys,
  LTE_Q_OffsetRange_t ofs, LTE_Q_OffsetRange_t ocs, long a3_offset, LTE_TimeToTrigger_t ttt);

static void decode_MBSFNAreaConfiguration(module_id_t module_idP, uint8_t eNB_index, frame_t frameP,uint8_t mbsfn_sync_area);
static void decode_MBMSCountingRequest(module_id_t module_idP, uint8_t eNB_index, frame_t frameP,uint8_t mbsfn_sync_area);

uint8_t rrc_ue_generate_SidelinkUEInformation( const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index,LTE_SL_DestinationInfoList_r12_t  *destinationInfoList, long *discTxResourceReq,
    SL_TRIGGER_t mode);

void
rrc_ue_process_MBMSCountingRequest(
  const protocol_ctxt_t *const ctxt_pP,
  LTE_MBMSCountingRequest_r10_t *MBMSCountingRequest,
  uint8_t eNB_index
		);

static void process_nr_nsa_msg(nsa_msg_t *msg, int msg_len);
static void nsa_sendmsg_to_nrue(const void *message, size_t msg_len, Rrc_Msg_Type_t msg_type);
protocol_ctxt_t ctxt_pP_local;


/*------------------------------------------------------------------------------*/

static Rrc_State_t rrc_get_state (module_id_t ue_mod_idP) {
  return UE_rrc_inst[ue_mod_idP].RrcState;
}


static Rrc_Sub_State_t rrc_get_sub_state (module_id_t ue_mod_idP) {
  return UE_rrc_inst[ue_mod_idP].RrcSubState;
}

static int rrc_set_state (module_id_t ue_mod_idP, Rrc_State_t state) {
  AssertFatal ((RRC_STATE_FIRST <= state) && (state <= RRC_STATE_LAST),
               "Invalid state %d!\n", state);

  if (UE_rrc_inst[ue_mod_idP].RrcState != state) {
    UE_rrc_inst[ue_mod_idP].RrcState = state;
    return (1);
  }

  return (0);
}

//-----------------------------------------------------------------------------
static int rrc_set_sub_state( module_id_t ue_mod_idP, Rrc_Sub_State_t subState ) {
  if (EPC_MODE_ENABLED) {
    switch (UE_rrc_inst[ue_mod_idP].RrcState) {
      case RRC_STATE_INACTIVE:
        AssertFatal ((RRC_SUB_STATE_INACTIVE_FIRST <= subState) && (subState <= RRC_SUB_STATE_INACTIVE_LAST),
                     "Invalid sub state %d for state %d!\n", subState, UE_rrc_inst[ue_mod_idP].RrcState);
        break;

      case RRC_STATE_IDLE:
        AssertFatal ((RRC_SUB_STATE_IDLE_FIRST <= subState) && (subState <= RRC_SUB_STATE_IDLE_LAST),
                     "Invalid sub state %d for state %d!\n", subState, UE_rrc_inst[ue_mod_idP].RrcState);
        break;

      case RRC_STATE_CONNECTED:
        AssertFatal ((RRC_SUB_STATE_CONNECTED_FIRST <= subState) && (subState <= RRC_SUB_STATE_CONNECTED_LAST),
                     "Invalid sub state %d for state %d!\n", subState, UE_rrc_inst[ue_mod_idP].RrcState);
        break;
    }
  }

  if (UE_rrc_inst[ue_mod_idP].RrcSubState != subState) {
    UE_rrc_inst[ue_mod_idP].RrcSubState = subState;
    return (1);
  }

  return (0);
}

//-----------------------------------------------------------------------------
void
openair_rrc_on_ue(
  const protocol_ctxt_t *const ctxt_pP
)
//-----------------------------------------------------------------------------
{
  unsigned short i;
  LOG_I(RRC, PROTOCOL_RRC_CTXT_FMT" UE?:OPENAIR RRC IN....\n",
        PROTOCOL_RRC_CTXT_ARGS(ctxt_pP));

  for (i = 0; i < NB_eNB_INST; i++) {
    LOG_D(RRC, PROTOCOL_RRC_CTXT_FMT" Activating CCCH (eNB %d)\n",
          PROTOCOL_RRC_CTXT_ARGS(ctxt_pP), i);
    UE_rrc_inst[ctxt_pP->module_id].Srb0[i].Srb_id = CCCH;
    memcpy (&UE_rrc_inst[ctxt_pP->module_id].Srb0[i].Lchan_desc[0], &CCCH_LCHAN_DESC, LCHAN_DESC_SIZE);
    memcpy (&UE_rrc_inst[ctxt_pP->module_id].Srb0[i].Lchan_desc[1], &CCCH_LCHAN_DESC, LCHAN_DESC_SIZE);
    rrc_config_buffer (&UE_rrc_inst[ctxt_pP->module_id].Srb0[i], CCCH, 1);
    UE_rrc_inst[ctxt_pP->module_id].Srb0[i].Active = 1;
  }
}

//-----------------------------------------------------------------------------
static void init_SI_UE(  protocol_ctxt_t const *ctxt_pP, const uint8_t eNB_index ) {
  UE_rrc_inst[ctxt_pP->module_id].sizeof_SIB1[eNB_index] = 0;
  UE_rrc_inst[ctxt_pP->module_id].sizeof_SI[eNB_index] = 0;
  UE_rrc_inst[ctxt_pP->module_id].SIB1[eNB_index] = (uint8_t *)malloc16_clear( 32 );
  UE_rrc_inst[ctxt_pP->module_id].sib1[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType1_t) );
  UE_rrc_inst[ctxt_pP->module_id].sizeof_SIB1_MBMS[eNB_index] = 0;
  UE_rrc_inst[ctxt_pP->module_id].sizeof_SI_MBMS[eNB_index] = 0;
  UE_rrc_inst[ctxt_pP->module_id].SIB1_MBMS[eNB_index] = (uint8_t *)malloc16_clear( 32 );
  UE_rrc_inst[ctxt_pP->module_id].sib1_MBMS[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType1_MBMS_r14_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib2[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType2_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib3[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType3_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib4[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType4_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib5[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType5_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib6[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType6_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib7[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType7_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib8[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType8_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib9[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType9_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib10[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType10_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib11[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType11_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib12[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType12_r9_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib13[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType13_r9_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib18[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType18_r12_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib19[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType19_r12_t) );
  UE_rrc_inst[ctxt_pP->module_id].sib21[eNB_index] = malloc16_clear( sizeof(LTE_SystemInformationBlockType21_r14_t) );
  UE_rrc_inst[ctxt_pP->module_id].SI[eNB_index] = (uint8_t *)malloc16_clear( 64 );
  UE_rrc_inst[ctxt_pP->module_id].si[eNB_index] = (LTE_SystemInformation_t *)malloc16_clear( sizeof(LTE_SystemInformation_t) );
  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus = 0;
  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIcnt    = 0;
  UE_rrc_inst[ctxt_pP->module_id].SI_MBMS[eNB_index] = (uint8_t *)malloc16_clear( 64 );
  UE_rrc_inst[ctxt_pP->module_id].si_MBMS[eNB_index] = (LTE_SystemInformation_MBMS_r14_t *)malloc16_clear( sizeof(LTE_SystemInformation_MBMS_r14_t) );
  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus_MBMS = 0;
  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIcnt_MBMS    = 0;
}

void init_SL_preconfig(UE_RRC_INST *UE, const uint8_t eNB_index ) {
  LOG_I(RRC,"Initializing Sidelink Pre-configuration for UE\n");
  UE->SL_Preconfiguration[eNB_index] = malloc16_clear( sizeof(struct LTE_SL_Preconfiguration_r12) );
  UE->SL_Preconfiguration[eNB_index]->preconfigGeneral_r12.rohc_Profiles_r12.profile0x0001_r12       = true;
  UE->SL_Preconfiguration[eNB_index]->preconfigGeneral_r12.carrierFreq_r12                           = 3350;
  UE->SL_Preconfiguration[eNB_index]->preconfigGeneral_r12.maxTxPower_r12                            = 0;
  UE->SL_Preconfiguration[eNB_index]->preconfigGeneral_r12.additionalSpectrumEmission_r12            = 0;
  UE->SL_Preconfiguration[eNB_index]->preconfigGeneral_r12.sl_bandwidth_r12                          = LTE_SL_PreconfigGeneral_r12__sl_bandwidth_r12_n50;
  UE->SL_Preconfiguration[eNB_index]->preconfigGeneral_r12.tdd_ConfigSL_r12.subframeAssignmentSL_r12 = LTE_TDD_ConfigSL_r12__subframeAssignmentSL_r12_none;
  UE->SL_Preconfiguration[eNB_index]->preconfigSync_r12.syncCP_Len_r12            = LTE_SL_CP_Len_r12_normal;
  UE->SL_Preconfiguration[eNB_index]->preconfigSync_r12.syncOffsetIndicator1_r12  = 0;
  UE->SL_Preconfiguration[eNB_index]->preconfigSync_r12.syncOffsetIndicator2_r12  = 0;
  UE->SL_Preconfiguration[eNB_index]->preconfigSync_r12.syncTxParameters_r12      = 0;
  UE->SL_Preconfiguration[eNB_index]->preconfigSync_r12.syncTxThreshOoC_r12       = 0;
  UE->SL_Preconfiguration[eNB_index]->preconfigSync_r12.filterCoefficient_r12     = LTE_FilterCoefficient_fc0;
  UE->SL_Preconfiguration[eNB_index]->preconfigSync_r12.syncRefMinHyst_r12        = LTE_SL_PreconfigSync_r12__syncRefMinHyst_r12_dB0;
  UE->SL_Preconfiguration[eNB_index]->preconfigSync_r12.syncRefDiffHyst_r12       = LTE_SL_PreconfigSync_r12__syncRefDiffHyst_r12_dB0;
  UE->SL_Preconfiguration[eNB_index]->preconfigSync_r12.ext1                      = malloc16_clear(sizeof(struct LTE_SL_PreconfigSync_r12__ext1));
  UE->SL_Preconfiguration[eNB_index]->preconfigSync_r12.ext1->syncTxPeriodic_r13  = NULL;
  struct LTE_SL_PreconfigCommPool_r12 *preconfigpool = malloc16_clear(sizeof(struct LTE_SL_PreconfigCommPool_r12));
  preconfigpool->sc_CP_Len_r12                                                    = LTE_SL_CP_Len_r12_normal;
  preconfigpool->sc_Period_r12                                                    = LTE_SL_PeriodComm_r12_sf40;
  // 20 PRBs for SL communications
  preconfigpool->sc_TF_ResourceConfig_r12.prb_Num_r12                             = 20;
  preconfigpool->sc_TF_ResourceConfig_r12.prb_Start_r12                           = 5;
  preconfigpool->sc_TF_ResourceConfig_r12.prb_End_r12                             = 44;
  // Offset set to 0 subframes
  preconfigpool->sc_TF_ResourceConfig_r12.offsetIndicator_r12.present             = LTE_SL_OffsetIndicator_r12_PR_small_r12;
  preconfigpool->sc_TF_ResourceConfig_r12.offsetIndicator_r12.choice.small_r12    = 0;
  // 40 ms SL Period
  preconfigpool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.present              = LTE_SubframeBitmapSL_r12_PR_bs40_r12;
  preconfigpool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf         = CALLOC(1,5);
  preconfigpool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.size        = 5;
  preconfigpool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.bits_unused = 0;
  // 1st 4 subframes for PSCCH
  preconfigpool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf[0]      = 0xF;
  preconfigpool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf[1]      = 0;
  preconfigpool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf[2]      = 0;
  preconfigpool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf[3]      = 0;
  preconfigpool->sc_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf[4]      = 0;
  preconfigpool->sc_TxParameters_r12                                              = 0;
  preconfigpool->data_CP_Len_r12                                                  = LTE_SL_CP_Len_r12_normal;
  // 20 PRBs for SL communications
  preconfigpool->data_TF_ResourceConfig_r12.prb_Num_r12                             = 20;
  preconfigpool->data_TF_ResourceConfig_r12.prb_Start_r12                           = 5;
  preconfigpool->data_TF_ResourceConfig_r12.prb_End_r12                             = 44;
  // Offset set to 0 subframes
  preconfigpool->data_TF_ResourceConfig_r12.offsetIndicator_r12.present             = LTE_SL_OffsetIndicator_r12_PR_small_r12;
  preconfigpool->data_TF_ResourceConfig_r12.offsetIndicator_r12.choice.small_r12    = 0;
  // 40 ms SL Period
  preconfigpool->data_TF_ResourceConfig_r12.subframeBitmap_r12.present              = LTE_SubframeBitmapSL_r12_PR_bs40_r12;
  preconfigpool->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf         = CALLOC(5,1);
  preconfigpool->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.size        = 5;
  preconfigpool->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.bits_unused = 0;
  // last 36 subframes for PSCCH
  preconfigpool->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf[0]      = 0xF0;
  preconfigpool->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf[1]      = 0xFF;
  preconfigpool->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf[2]      = 0xFF;
  preconfigpool->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf[3]      = 0xFF;
  preconfigpool->data_TF_ResourceConfig_r12.subframeBitmap_r12.choice.bs40_r12.buf[4]      = 0xFF;
  preconfigpool->dataHoppingConfig_r12.hoppingParameter_r12                         = 0;
  preconfigpool->dataHoppingConfig_r12.numSubbands_r12                              = LTE_SL_HoppingConfigComm_r12__numSubbands_r12_ns1;
  preconfigpool->dataHoppingConfig_r12.rb_Offset_r12                                = 0;
  preconfigpool->dataTxParameters_r12                                               = 0;
  ASN_SEQUENCE_ADD(&UE->SL_Preconfiguration[eNB_index]->preconfigComm_r12.list,preconfigpool);
  // Rel13 extensions
  UE->SL_Preconfiguration[eNB_index]->ext1 = NULL;
}



//-----------------------------------------------------------------------------
void openair_rrc_ue_init_security( const protocol_ctxt_t *const ctxt_pP ) {
  //    uint8_t *kRRCenc;
  //    uint8_t *kRRCint;
  char ascii_buffer[65];
  uint8_t i;
  memset(UE_rrc_inst[ctxt_pP->module_id].kenb, ctxt_pP->module_id, 32);

  for (i = 0; i < 32; i++) {
    sprintf(&ascii_buffer[2 * i], "%02X", UE_rrc_inst[ctxt_pP->module_id].kenb[i]);
  }

  LOG_T(RRC, PROTOCOL_RRC_CTXT_FMT"[OSA] kenb    = %s\n",
        PROTOCOL_RRC_CTXT_ARGS(ctxt_pP),
        ascii_buffer);
}

//-----------------------------------------------------------------------------
char openair_rrc_ue_init( const module_id_t ue_mod_idP, const unsigned char eNB_index ) {
  protocol_ctxt_t ctxt;
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_idP, ENB_FLAG_NO, NOT_A_RNTI, 0, 0,eNB_index);
  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_FMT" Init...\n",
        PROTOCOL_RRC_CTXT_ARGS(&ctxt));
  rrc_set_state (ue_mod_idP, RRC_STATE_INACTIVE);
  rrc_set_sub_state (ue_mod_idP, RRC_SUB_STATE_INACTIVE);
  LOG_I(RRC,"[UE %d] INIT State = RRC_IDLE (eNB %d)\n",ctxt.module_id,eNB_index);
  UE_rrc_inst[ctxt.module_id].selected_plmn_identity = 1;
  UE_rrc_inst[ctxt.module_id].Info[eNB_index].State=RRC_IDLE;
  UE_rrc_inst[ctxt.module_id].Info[eNB_index].T300_active = 0;
  UE_rrc_inst[ctxt.module_id].Info[eNB_index].T304_active = 0;
  UE_rrc_inst[ctxt.module_id].Info[eNB_index].T310_active = 0;
  UE_rrc_inst[ctxt.module_id].Info[eNB_index].UE_index=0xffff;
  UE_rrc_inst[ctxt.module_id].Srb0[eNB_index].Active=0;
  UE_rrc_inst[ctxt.module_id].Srb1[eNB_index].Active=0;
  UE_rrc_inst[ctxt.module_id].Srb2[eNB_index].Active=0;
  UE_rrc_inst[ctxt.module_id].HandoverInfoUe.measFlag=1;
  UE_rrc_inst[ctxt.module_id].ciphering_algorithm = LTE_CipheringAlgorithm_r12_eea0;
  UE_rrc_inst[ctxt.module_id].integrity_algorithm = LTE_SecurityAlgorithmConfig__integrityProtAlgorithm_eia0_v920;
  openair_rrc_ue_init_security(&ctxt);
  init_SI_UE(&ctxt,eNB_index);
  LOG_D(RRC,PROTOCOL_RRC_CTXT_FMT"  INIT: phy_sync_2_ch_ind\n",
        PROTOCOL_RRC_CTXT_ARGS(&ctxt));
#ifndef NO_RRM
  send_msg(&S_rrc,msg_rrc_phy_synch_to_CH_ind(ctxt.module_id,eNB_index,UE_rrc_inst[ctxt.module_id].Mac_id));
#endif
#ifndef NO_RRM
  send_msg(&S_rrc,msg_rrc_phy_synch_to_CH_ind(ctxt.module_id,eNB_index,UE_rrc_inst[ctxt.module_id].Mac_id));
#endif
#ifdef NO_RRM //init ch SRB0, SRB1 & BDTCH
  openair_rrc_on_ue(&ctxt);
#endif
  return 0;
}

//-----------------------------------------------------------------------------
void rrc_ue_generate_RRCConnectionRequest( const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index ) {
  uint8_t rv[6];

  if(UE_rrc_inst[ctxt_pP->module_id].Srb0[eNB_index].Tx_buffer.payload_size ==0) {
    // Get RRCConnectionRequest, fill random for now
    // Generate random byte stream for contention resolution
    for (int i=0; i<6; i++) {
#ifdef SMBV
      // if SMBV is configured the contention resolution needs to be fix for the connection procedure to succeed
      rv[i]=i;
#else
      rv[i]=taus()&0xff;
#endif
      LOG_T(RRC,"%x.",rv[i]);
    }

    LOG_T(RRC,"\n");
    UE_rrc_inst[ctxt_pP->module_id].Srb0[eNB_index].Tx_buffer.payload_size =
      do_RRCConnectionRequest(
        ctxt_pP->module_id,
        (uint8_t *)UE_rrc_inst[ctxt_pP->module_id].Srb0[eNB_index].Tx_buffer.Payload,
        sizeof(UE_rrc_inst[ctxt_pP->module_id].Srb0[eNB_index].Tx_buffer.Payload),
        rv);
    LOG_I(RRC,"[UE %d] : Frame %d, Logical Channel UL-CCCH (SRB0), Generating RRCConnectionRequest (bytes %d, eNB %d)\n",
          ctxt_pP->module_id, ctxt_pP->frame, UE_rrc_inst[ctxt_pP->module_id].Srb0[eNB_index].Tx_buffer.payload_size, eNB_index);

    for (int i=0; i<UE_rrc_inst[ctxt_pP->module_id].Srb0[eNB_index].Tx_buffer.payload_size; i++) {
      LOG_T(RRC,"%x.\n",UE_rrc_inst[ctxt_pP->module_id].Srb0[eNB_index].Tx_buffer.Payload[i]);
    }

    LOG_T(RRC,"\n");
    /*UE_rrc_inst[ue_mod_idP].Srb0[Idx].Tx_buffer.Payload[i] = taus()&0xff;
    UE_rrc_inst[ue_mod_idP].Srb0[Idx].Tx_buffer.payload_size =i; */
  }
}


mui_t rrc_mui=0;


/* NAS Attach request with IMSI */
static const char  nas_attach_req_imsi[] = {
  0x07, 0x41,
  /* EPS Mobile identity = IMSI */
  0x71, 0x08, 0x29, 0x80, 0x43, 0x21, 0x43, 0x65, 0x87,
  0xF9,
  /* End of EPS Mobile Identity */
  0x02, 0xE0, 0xE0, 0x00, 0x20, 0x02, 0x03,
  0xD0, 0x11, 0x27, 0x1A, 0x80, 0x80, 0x21, 0x10, 0x01, 0x00, 0x00,
  0x10, 0x81, 0x06, 0x00, 0x00, 0x00, 0x00, 0x83, 0x06, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x0A, 0x00, 0x52, 0x12, 0xF2,
  0x01, 0x27, 0x11,
};


//-----------------------------------------------------------------------------
void
rrc_t310_expiration(
  const protocol_ctxt_t *const ctxt_pP,
  const uint8_t                 eNB_index
)
//-----------------------------------------------------------------------------
{
  if (UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].State != RRC_CONNECTED) {
    LOG_D(RRC, "Timer 310 expired, going to RRC_IDLE\n");
    UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].State = RRC_IDLE;
    UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].UE_index = 0xffff;
    UE_rrc_inst[ctxt_pP->module_id].Srb0[eNB_index].Rx_buffer.payload_size = 0;
    UE_rrc_inst[ctxt_pP->module_id].Srb0[eNB_index].Tx_buffer.payload_size = 0;
    UE_rrc_inst[ctxt_pP->module_id].Srb1[eNB_index].Srb_info.Rx_buffer.payload_size = 0;
    UE_rrc_inst[ctxt_pP->module_id].Srb1[eNB_index].Srb_info.Tx_buffer.payload_size = 0;

    if (UE_rrc_inst[ctxt_pP->module_id].Srb2[eNB_index].Active == 1) {
      LOG_D (RRC,"[Inst %d] eNB_index %d, Remove RB %d\n ", ctxt_pP->module_id, eNB_index,
             UE_rrc_inst[ctxt_pP->module_id].Srb2[eNB_index].Srb_info.Srb_id);
      rrc_pdcp_config_req (ctxt_pP,
                           SRB_FLAG_YES,
                           CONFIG_ACTION_REMOVE,
                           UE_rrc_inst[ctxt_pP->module_id].Srb2[eNB_index].Srb_info.Srb_id,
                           0);
      rrc_rlc_config_req (ctxt_pP,
                          SRB_FLAG_YES,
                          MBMS_FLAG_NO,
                          CONFIG_ACTION_REMOVE,
                          UE_rrc_inst[ctxt_pP->module_id].Srb2[eNB_index].Srb_info.Srb_id,
                          Rlc_info_um);
      UE_rrc_inst[ctxt_pP->module_id].Srb2[eNB_index].Active = 0;
      UE_rrc_inst[ctxt_pP->module_id].Srb2[eNB_index].StatusSrb = IDLE;
      UE_rrc_inst[ctxt_pP->module_id].Srb2[eNB_index].Next_check_frame = 0;
    }
  } else { // Restablishment procedure
    LOG_D(RRC, "Timer 310 expired, trying RRCRestablishment ...\n");
  }
}

//-----------------------------------------------------------------------------
static void rrc_ue_generate_RRCConnectionSetupComplete(
    const protocol_ctxt_t *const ctxt_pP,
    const uint8_t eNB_index,
    const uint8_t Transaction_id,
    uint8_t sel_plmn_id) {
  uint8_t    buffer[100];
  uint8_t    size;
  const char *nas_msg;
  int   nas_msg_length;

  if (EPC_MODE_ENABLED) {
    nas_msg         = (char *) UE_rrc_inst[ctxt_pP->module_id].initialNasMsg.data;
    nas_msg_length  = UE_rrc_inst[ctxt_pP->module_id].initialNasMsg.length;
  } else {
    nas_msg         = nas_attach_req_imsi;
    nas_msg_length  = sizeof(nas_attach_req_imsi);
  }

  size = do_RRCConnectionSetupComplete(ctxt_pP->module_id, buffer, Transaction_id, sel_plmn_id, nas_msg_length, nas_msg);
  LOG_I(RRC,"[UE %d][RAPROC] Frame %d : Logical Channel UL-DCCH (SRB1), Generating RRCConnectionSetupComplete (bytes%d, eNB %d)\n",
        ctxt_pP->module_id,ctxt_pP->frame, size, eNB_index);
  LOG_D(RLC,
        "[FRAME %05d][RRC_UE][MOD %02d][][--- PDCP_DATA_REQ/%d Bytes (RRCConnectionSetupComplete to eNB %d MUI %d) --->][PDCP][MOD %02d][RB %02d]\n",
        ctxt_pP->frame, ctxt_pP->module_id+NB_eNB_INST, size, eNB_index, rrc_mui, ctxt_pP->module_id+NB_eNB_INST, DCCH);
  ctxt_pP_local.rnti = ctxt_pP->rnti;
  rrc_data_req_ue (
    ctxt_pP,
    DCCH,
    rrc_mui++,
    SDU_CONFIRM_NO,
    size,
    buffer,
    PDCP_TRANSMISSION_MODE_CONTROL);
}

//-----------------------------------------------------------------------------
void rrc_ue_generate_RRCConnectionReconfigurationComplete(const protocol_ctxt_t *const ctxt_pP,
                                                          const uint8_t eNB_index,
                                                          const uint8_t Transaction_id,
                                                          OCTET_STRING_t *str) {
  uint8_t buffer[RRC_BUF_SIZE];
  size_t size = do_RRCConnectionReconfigurationComplete(ctxt_pP, buffer, sizeof(buffer), Transaction_id, str);
  LOG_I(RRC,PROTOCOL_RRC_CTXT_UE_FMT" Logical Channel UL-DCCH (SRB1), Generating RRCConnectionReconfigurationComplete (bytes %zu, eNB_index %d)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP), size, eNB_index);
  LOG_D(RLC,
        "[FRAME %05d][RRC_UE][INST %02d][][--- PDCP_DATA_REQ/%zu Bytes (RRCConnectionReconfigurationComplete to eNB %d MUI %d) --->][PDCP][INST %02d][RB %02d]\n",
        ctxt_pP->frame,
        UE_MODULE_ID_TO_INSTANCE(ctxt_pP->module_id),
        size,
        eNB_index,
        rrc_mui,
        UE_MODULE_ID_TO_INSTANCE(ctxt_pP->module_id),
        DCCH);
  rrc_data_req_ue (
    ctxt_pP,
    DCCH,
    rrc_mui++,
    SDU_CONFIRM_NO,
    size,
    buffer,
    PDCP_TRANSMISSION_MODE_CONTROL);
}


//-----------------------------------------------------------------------------
// Called by L2 interface (MAC)
int rrc_ue_decode_ccch( const protocol_ctxt_t *const ctxt_pP, const SRB_INFO *const Srb_info, const uint8_t eNB_index ) {
  LTE_DL_CCCH_Message_t *dl_ccch_msg=NULL;
  asn_dec_rval_t dec_rval;
  int rval=0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_CCCH, VCD_FUNCTION_IN);
  //  LOG_D(RRC,"[UE %d] Decoding DL-CCCH message (%d bytes), State %d\n",ue_mod_idP,Srb_info->Rx_buffer.payload_size,
  //  UE_rrc_inst[ue_mod_idP].Info[eNB_index].State);
  dec_rval = uper_decode(NULL,
                         &asn_DEF_LTE_DL_CCCH_Message,
                         (void **)&dl_ccch_msg,
                         (uint8_t *)Srb_info->Rx_buffer.Payload,
                         100,0,0);

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout,&asn_DEF_LTE_DL_CCCH_Message,(void *)dl_ccch_msg);
  }

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed==0)) {
    LOG_E(RRC,"[UE %d] Frame %d : Failed to decode DL-CCCH-Message (%zu bytes)\n",ctxt_pP->module_id,ctxt_pP->frame,dec_rval.consumed);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_CCCH, VCD_FUNCTION_OUT);
    return -1;
  }

  if (dl_ccch_msg->message.present == LTE_DL_CCCH_MessageType_PR_c1) {
    if (UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].State == RRC_SI_RECEIVED) {
      switch (dl_ccch_msg->message.choice.c1.present) {
        case LTE_DL_CCCH_MessageType__c1_PR_NOTHING:
          LOG_I(RRC, "[UE%d] Frame %d : Received PR_NOTHING on DL-CCCH-Message\n",
                ctxt_pP->module_id,
                ctxt_pP->frame);
          rval = 0;
          break;

        case LTE_DL_CCCH_MessageType__c1_PR_rrcConnectionReestablishment:
          LOG_I(RRC,
                "[UE%d] Frame %d : Logical Channel DL-CCCH (SRB0), Received RRCConnectionReestablishment\n",
                ctxt_pP->module_id,
                ctxt_pP->frame);
          rval = 0;
          break;

        case LTE_DL_CCCH_MessageType__c1_PR_rrcConnectionReestablishmentReject:
          LOG_I(RRC,
                "[UE%d] Frame %d : Logical Channel DL-CCCH (SRB0), Received RRCConnectionReestablishmentReject\n",
                ctxt_pP->module_id,
                ctxt_pP->frame);
          rval = 0;
          break;

        case LTE_DL_CCCH_MessageType__c1_PR_rrcConnectionReject:
          LOG_I(RRC,
                "[UE%d] Frame %d : Logical Channel DL-CCCH (SRB0), Received RRCConnectionReject \n",
                ctxt_pP->module_id,
                ctxt_pP->frame);
          rval = 0;
          break;

        case LTE_DL_CCCH_MessageType__c1_PR_rrcConnectionSetup:
          LOG_A(RRC,
                "[UE%d][RAPROC] Frame %d : Logical Channel DL-CCCH (SRB0), Received RRCConnectionSetup RNTI %x\n",
                ctxt_pP->module_id,
                ctxt_pP->frame,
                ctxt_pP->rnti);
          // Get configuration
          // Release T300 timer
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].T300_active = 0;
          rrc_ue_process_radioResourceConfigDedicated(
            ctxt_pP,
            eNB_index,
            &dl_ccch_msg->message.choice.c1.choice.rrcConnectionSetup.criticalExtensions.choice.c1.choice.rrcConnectionSetup_r8.radioResourceConfigDedicated);
          rrc_set_state (ctxt_pP->module_id, RRC_STATE_CONNECTED);
          rrc_set_sub_state (ctxt_pP->module_id, RRC_SUB_STATE_CONNECTED);
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].rnti = ctxt_pP->rnti;
          rrc_ue_generate_RRCConnectionSetupComplete(
            ctxt_pP,
            eNB_index,
            dl_ccch_msg->message.choice.c1.choice.rrcConnectionSetup.rrc_TransactionIdentifier,
            UE_rrc_inst[ctxt_pP->module_id].selected_plmn_identity);
          rval = 0;
          break;

        default:
          LOG_E(RRC, "[UE%d] Frame %d : Unknown message\n",
                ctxt_pP->module_id,
                ctxt_pP->frame);
          rval = -1;
          break;
      }
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_CCCH, VCD_FUNCTION_OUT);
  return rval;
}

//-----------------------------------------------------------------------------
int32_t
rrc_ue_establish_srb1(
  module_id_t ue_mod_idP,
  frame_t frameP,
  uint8_t eNB_index,
  struct LTE_SRB_ToAddMod *SRB_config
)
//-----------------------------------------------------------------------------
{
  // add descriptor from RRC PDU
  UE_rrc_inst[ue_mod_idP].Srb1[eNB_index].Active = 1;
  UE_rrc_inst[ue_mod_idP].Srb1[eNB_index].StatusSrb = RADIO_CONFIG_OK;//RADIO CFG
  UE_rrc_inst[ue_mod_idP].Srb1[eNB_index].Srb_info.Srb_id = 1;
  // copy default configuration for now
  //  memcpy(&UE_rrc_inst[ue_mod_idP].Srb1[eNB_index].Srb_info.Lchan_desc[0],&DCCH_LCHAN_DESC,LCHAN_DESC_SIZE);
  //  memcpy(&UE_rrc_inst[ue_mod_idP].Srb1[eNB_index].Srb_info.Lchan_desc[1],&DCCH_LCHAN_DESC,LCHAN_DESC_SIZE);
  LOG_I(RRC,"[UE %d], CONFIG_SRB1 %d corresponding to eNB_index %d\n", ue_mod_idP,DCCH,eNB_index);
  //rrc_pdcp_config_req (ue_mod_idP+NB_eNB_INST, frameP, 0, CONFIG_ACTION_ADD, lchan_id,UNDEF_SECURITY_MODE);
  //  rrc_rlc_config_req(ue_mod_idP+NB_eNB_INST,frameP,0,CONFIG_ACTION_ADD,lchan_id,SIGNALLING_RADIO_BEARER,Rlc_info_am_config);
  //  UE_rrc_inst[ue_mod_idP].Srb1[eNB_index].Srb_info.Tx_buffer.payload_size=DEFAULT_MEAS_IND_SIZE+1;
  return(0);
}

//-----------------------------------------------------------------------------
int32_t
rrc_ue_establish_srb2(
  module_id_t ue_mod_idP,
  frame_t frameP,
  uint8_t eNB_index,
  struct LTE_SRB_ToAddMod *SRB_config
)
//-----------------------------------------------------------------------------
{
  // add descriptor from RRC PDU
  UE_rrc_inst[ue_mod_idP].Srb2[eNB_index].Active = 1;
  UE_rrc_inst[ue_mod_idP].Srb2[eNB_index].StatusSrb = RADIO_CONFIG_OK;//RADIO CFG
  UE_rrc_inst[ue_mod_idP].Srb2[eNB_index].Srb_info.Srb_id = 2;
  // copy default configuration for now
  //  memcpy(&UE_rrc_inst[ue_mod_idP].Srb2[eNB_index].Srb_info.Lchan_desc[0],&DCCH_LCHAN_DESC,LCHAN_DESC_SIZE);
  //  memcpy(&UE_rrc_inst[ue_mod_idP].Srb2[eNB_index].Srb_info.Lchan_desc[1],&DCCH_LCHAN_DESC,LCHAN_DESC_SIZE);
  LOG_I(RRC,"[UE %d], CONFIG_SRB2 %d corresponding to eNB_index %d\n",ue_mod_idP,DCCH1,eNB_index);
  //rrc_pdcp_config_req (ue_mod_idP+NB_eNB_INST, frameP, 0, CONFIG_ACTION_ADD, lchan_id, UNDEF_SECURITY_MODE);
  //  rrc_rlc_config_req(ue_mod_idP+NB_eNB_INST,frameP,0,CONFIG_ACTION_ADD,lchan_id,SIGNALLING_RADIO_BEARER,Rlc_info_am_config);
  //  UE_rrc_inst[ue_mod_idP].Srb1[eNB_index].Srb_info.Tx_buffer.payload_size=DEFAULT_MEAS_IND_SIZE+1;
  return(0);
}

//-----------------------------------------------------------------------------
int32_t
rrc_ue_establish_drb(
  module_id_t ue_mod_idP,
  frame_t frameP,
  uint8_t eNB_index,
  struct LTE_DRB_ToAddMod *DRB_config
)
//-----------------------------------------------------------------------------
{
  // add descriptor from RRC PDU
  int oip_ifup=0,ip_addr_offset3=0,ip_addr_offset4=0;
  /* avoid gcc warnings */
  (void)oip_ifup;
  (void)ip_addr_offset3;
  (void)ip_addr_offset4;
  LOG_I(RRC,"[UE %d] Frame %d: processing RRCConnectionReconfiguration: reconfiguring DRB %ld/LCID %d\n",
        ue_mod_idP, frameP, DRB_config->drb_Identity, (int)*DRB_config->logicalChannelIdentity);

  if(!EPC_MODE_ENABLED) {
    ip_addr_offset3 = 0;
    ip_addr_offset4 = 1;
    LOG_I(OIP,"[UE %d] trying to bring up the OAI interface %d, IP X.Y.%d.%d\n", ue_mod_idP, ip_addr_offset3+ue_mod_idP,
          ip_addr_offset3+ue_mod_idP+1,ip_addr_offset4+ue_mod_idP+1);
    oip_ifup=nas_config(ip_addr_offset3+ue_mod_idP+1,   // interface_id
                        UE_NAS_USE_TUN?1:(ip_addr_offset3+ue_mod_idP+1), // third_octet
                        ip_addr_offset4+ue_mod_idP+1, // fourth_octet
                        "oip");                        // interface suffix (when using kernel module)

    if (oip_ifup == 0 && (!UE_NAS_USE_TUN)) { // interface is up --> send a config the DRB
      LOG_I(OIP,"[UE %d] Config the ue net interface %d to send/receive pkt on DRB %ld to/from the protocol stack\n",
            ue_mod_idP,
            ip_addr_offset3+ue_mod_idP,
            (long int)((eNB_index * LTE_maxDRB) + DRB_config->drb_Identity));
      rb_conf_ipv4(0,//add
                   ue_mod_idP,//cx align with the UE index
                   ip_addr_offset3+ue_mod_idP,//inst num_enb+ue_index
                   (eNB_index * LTE_maxDRB) + DRB_config->drb_Identity,//rb
                   0,//dscp
                   ipv4_address(ip_addr_offset3+ue_mod_idP+1,ip_addr_offset4+ue_mod_idP+1),//saddr
                   ipv4_address(ip_addr_offset3+ue_mod_idP+1,eNB_index+1));//daddr
      LOG_D(RRC,"[UE %d] State = Attached (eNB %d)\n",ue_mod_idP,eNB_index);
    }
  } // !EPC_MODE_ENABLED

  return(0);
}

//-----------------------------------------------------------------------------
void
rrc_ue_process_measConfig(
  const protocol_ctxt_t *const       ctxt_pP,
  const uint8_t                      eNB_index,
  LTE_MeasConfig_t *const               measConfig
)
//-----------------------------------------------------------------------------
{
  // This is the procedure described in 36.331 Section 5.5.2.1
  int i;
  long ind;
  LTE_MeasObjectToAddMod_t *measObj;
  UE_RRC_INST *ue = &UE_rrc_inst[ctxt_pP->module_id];

  if (measConfig->measObjectToRemoveList != NULL) {
    for (i = 0; i < measConfig->measObjectToRemoveList->list.count; i++) {
      ind = *measConfig->measObjectToRemoveList->list.array[i];
      free(ue->MeasObj[eNB_index][ind-1]);
      ue->MeasObj[eNB_index][ind-1] = NULL;
    }
  }

  if (measConfig->measObjectToAddModList != NULL) {
    LOG_I(RRC,"Measurement Object List is present\n");

    for (i = 0; i < measConfig->measObjectToAddModList->list.count; i++) {
      measObj = measConfig->measObjectToAddModList->list.array[i];
      ind = measConfig->measObjectToAddModList->list.array[i]->measObjectId;
      AssertFatal(ind > 0 && ind <= MAX_MEAS_OBJ && eNB_index >= 0 && eNB_index < NB_CNX_UE,
                 "measObjectId is out of bounds. ind = %ld, eNB_index = %d, i = %d.\n", ind, eNB_index, i);
      if (ue->MeasObj[eNB_index][ind-1]) {
        LOG_D(RRC, "Modifying measurement object [%d][%ld]\n", eNB_index, ind);
        memcpy((char *)ue->MeasObj[eNB_index][ind-1],
               (char *)measObj,
               sizeof(LTE_MeasObjectToAddMod_t));
      } else {
          if (measObj->measObject.present == LTE_MeasObjectToAddMod__measObject_PR_measObjectEUTRA) {
            LOG_I(RRC,"EUTRA Measurement : carrierFreq %ld, allowedMeasBandwidth %ld,presenceAntennaPort1 %d, neighCellConfig %d\n",
                  measObj->measObject.choice.measObjectEUTRA.carrierFreq,
                  measObj->measObject.choice.measObjectEUTRA.allowedMeasBandwidth,
                  measObj->measObject.choice.measObjectEUTRA.presenceAntennaPort1,
                  measObj->measObject.choice.measObjectEUTRA.neighCellConfig.buf[0]);
          } else if (measObj->measObject.present == LTE_MeasObjectToAddMod__measObject_PR_measObjectNR_r15) {
            ue->subframeCount = 0;
            LOG_I(RRC, "NR_r15 Measurement: carrierFreq: %ld\n",
                  measObj->measObject.choice.measObjectNR_r15.carrierFreq_r15);
            if (!get_softmodem_params()->nsa) {
              LOG_E(RRC, "Not in NSA mode but attempting to send measurement request to NR-UE\n");
              return;
            }
            uint8_t buffer[RRC_BUF_SIZE];
            asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_MeasObjectToAddMod,
                                                            NULL,
                                                            measObj,
                                                            buffer,
                                                            sizeof(buffer));
            AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %zu)!\n",
                          enc_rval.failed_type->name, enc_rval.encoded);
            nsa_sendmsg_to_nrue(buffer, (enc_rval.encoded + 7)/8, RRC_MEASUREMENT_PROCEDURE);
            LOG_A(RRC, "Encoded measurement object %zu bits (%zu bytes) and sent to NR UE\n",
                  enc_rval.encoded, (enc_rval.encoded + 7)/8);
          }
          LOG_D(RRC, "Adding measurement object [%d][%ld]\n", eNB_index, ind);
          ue->MeasObj[eNB_index][ind-1]=measObj;
        }
        measConfig->measObjectToAddModList->list.array[i] = NULL;
    }

    LOG_I(RRC,"call rrc_mac_config_req \n");
    rrc_mac_config_req_ue(ctxt_pP->module_id,0,eNB_index,
                          (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                          (struct LTE_PhysicalConfigDedicated *)NULL,
                          (LTE_SCellToAddMod_r10_t *)NULL,
                          ue->MeasObj[eNB_index],
                          (LTE_MAC_MainConfig_t *)NULL,
                          0,
                          (struct LTE_LogicalChannelConfig *)NULL,
                          (LTE_MeasGapConfig_t *)NULL,
                          (LTE_TDD_Config_t *)NULL,
                          (LTE_MobilityControlInfo_t *)NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          0,
                          (LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                          (LTE_PMCH_InfoList_r9_t *)NULL,
                          0,
                          NULL,
                          NULL,
                          0,
                          (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                          (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                         );
  }

  if (measConfig->reportConfigToRemoveList != NULL) {
    for (i=0; i<measConfig->reportConfigToRemoveList->list.count; i++) {
      ind   = *measConfig->reportConfigToRemoveList->list.array[i];
      free(ue->ReportConfig[eNB_index][ind-1]);
      ue->ReportConfig[eNB_index][ind-1] = NULL;
    }
  }

  if (measConfig->reportConfigToAddModList != NULL) {
    LOG_I(RRC,"Report Configuration List is present\n");

    for (i=0; i < measConfig->reportConfigToAddModList->list.count; i++) {
      ind = measConfig->reportConfigToAddModList->list.array[i]->reportConfigId;
      AssertFatal(ind > 0 && ind <= MAX_MEAS_CONFIG && eNB_index >= 0 && eNB_index < NB_CNX_UE,
                 "ReportConfigId is out of bounds. ind = %ld, eNB_index = %d, i = %d.\n", ind, eNB_index, i);
      if (ue->ReportConfig[eNB_index][ind-1]) {
        LOG_D(RRC,"Modifying ReportConfig [%d][%ld]\n", eNB_index, ind-1);
        memcpy((char *)ue->ReportConfig[eNB_index][ind-1],
               (char *)measConfig->reportConfigToAddModList->list.array[i],
               sizeof(LTE_ReportConfigToAddMod_t));
      } else {
        LOG_D(RRC,"Adding ReportConfig [%d][%ld]\n", eNB_index, ind-1);
        ue->ReportConfig[eNB_index][ind-1] = measConfig->reportConfigToAddModList->list.array[i];
      }
      measConfig->reportConfigToAddModList->list.array[i] = NULL;
    }
  }

  if (measConfig->quantityConfig != NULL) {
    if (ue->QuantityConfig[eNB_index]) {
      LOG_D(RRC,"Modifying Quantity Configuration \n");
      memcpy((char *)ue->QuantityConfig[eNB_index],
             (char *)measConfig->quantityConfig,
             sizeof(LTE_QuantityConfig_t));
    } else {
      LOG_D(RRC,"Adding Quantity configuration\n");
      ue->QuantityConfig[eNB_index] = measConfig->quantityConfig;
    }
    measConfig->quantityConfig = NULL;
  }

  if (measConfig->measIdToRemoveList != NULL) {
    for (i=0; i<measConfig->measIdToRemoveList->list.count; i++) {
      ind   = *measConfig->measIdToRemoveList->list.array[i];
      free(ue->MeasId[eNB_index][ind-1]);
      ue->MeasId[eNB_index][ind-1] = NULL;
    }
  }

  if (measConfig->measIdToAddModList != NULL) {
    for (i=0; i<measConfig->measIdToAddModList->list.count; i++) {
      ind   = measConfig->measIdToAddModList->list.array[i]->measId;
      AssertFatal(ind > 0 && ind <= MAX_MEAS_ID && eNB_index >= 0 && eNB_index < NB_CNX_UE,
                 "measId is out of bounds. ind = %ld, eNB_index = %d, i = %d.\n", ind, eNB_index, i);
      if (ue->MeasId[eNB_index][ind-1]) {
        LOG_I(RRC,"Modifying Measurement ID [%d][%ld]\n", eNB_index, ind-1);
        memcpy((char *)ue->MeasId[eNB_index][ind-1],
               (char *)measConfig->measIdToAddModList->list.array[i],
               sizeof(LTE_MeasIdToAddMod_t));
      } else {
        LOG_I(RRC,"Adding Measurement ID [%d][%ld]\n", eNB_index, ind-1);
        ue->MeasId[eNB_index][ind-1] = measConfig->measIdToAddModList->list.array[i];
      }
      measConfig->measIdToAddModList->list.array[i] = NULL;
    }
  }

  if (measConfig->measGapConfig !=NULL) {
    if (ue->measGapConfig[eNB_index]) {
      memcpy((char *)ue->measGapConfig[eNB_index],
             (char *)measConfig->measGapConfig,
             sizeof(LTE_MeasGapConfig_t));
    } else {
      ue->measGapConfig[eNB_index] = measConfig->measGapConfig;
    }
    measConfig->measGapConfig = NULL;
  }

  if (measConfig->quantityConfig != NULL) {
    if (ue->QuantityConfig[eNB_index]) {
      LOG_I(RRC,"Modifying Quantity Configuration \n");
      memcpy((char *)ue->QuantityConfig[eNB_index],
             (char *)measConfig->quantityConfig,
             sizeof(LTE_QuantityConfig_t));
    } else {
      LOG_I(RRC,"Adding Quantity configuration\n");
      ue->QuantityConfig[eNB_index] = measConfig->quantityConfig;
    }
    measConfig->quantityConfig = NULL;

    ue->filter_coeff_rsrp = 1./pow(2,
        (*ue->QuantityConfig[eNB_index]->quantityConfigEUTRA->filterCoefficientRSRP)/4);
    ue->filter_coeff_rsrq = 1./pow(2,
        (*ue->QuantityConfig[eNB_index]->quantityConfigEUTRA->filterCoefficientRSRQ)/4);
    LOG_I(RRC,"[UE %d] set rsrp-coeff for eNB %d: %ld rsrq-coeff: %ld rsrp_factor: %f rsrq_factor: %f \n",
          ctxt_pP->module_id, eNB_index, // UE_rrc_inst[ue_mod_idP].Info[eNB_index].UE_index,
          *ue->QuantityConfig[eNB_index]->quantityConfigEUTRA->filterCoefficientRSRP,
          *ue->QuantityConfig[eNB_index]->quantityConfigEUTRA->filterCoefficientRSRQ,
          ue->filter_coeff_rsrp,
          ue->filter_coeff_rsrq);
  }

  if (measConfig->s_Measure != NULL) {
    ue->s_measure = *measConfig->s_Measure;
  }
  measConfig->s_Measure = NULL;

  if (measConfig->speedStatePars != NULL) {
    if (ue->speedStatePars) {
      memcpy((char *)ue->speedStatePars,(char *)measConfig->speedStatePars,sizeof(struct LTE_MeasConfig__speedStatePars));
    } else {
      ue->speedStatePars = measConfig->speedStatePars;
    }
    measConfig->speedStatePars = NULL;

    LOG_I(RRC,"[UE %d] Configuring mobility optimization params for UE %d \n",
          ctxt_pP->module_id,ue->Info[0].UE_index);
  }
}


void
rrc_ue_update_radioResourceConfigDedicated(LTE_RadioResourceConfigDedicated_t *radioResourceConfigDedicated,
    const protocol_ctxt_t *const ctxt_pP,
    uint8_t eNB_index) {
  LTE_PhysicalConfigDedicated_t *physicalConfigDedicated2 = NULL;
  physicalConfigDedicated2 = CALLOC(1,sizeof(*physicalConfigDedicated2));
  physicalConfigDedicated2->pdsch_ConfigDedicated         = CALLOC(1,sizeof(*physicalConfigDedicated2->pdsch_ConfigDedicated));
  physicalConfigDedicated2->pusch_ConfigDedicated         = CALLOC(1,sizeof(*physicalConfigDedicated2->pusch_ConfigDedicated));
  physicalConfigDedicated2->pucch_ConfigDedicated         = CALLOC(1,sizeof(*physicalConfigDedicated2->pucch_ConfigDedicated));
  physicalConfigDedicated2->cqi_ReportConfig              = CALLOC(1,sizeof(*physicalConfigDedicated2->cqi_ReportConfig));
  physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic
    = CALLOC(1,sizeof(*physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic));
  physicalConfigDedicated2->soundingRS_UL_ConfigDedicated = CALLOC(1,sizeof(*physicalConfigDedicated2->soundingRS_UL_ConfigDedicated));
  physicalConfigDedicated2->schedulingRequestConfig       = CALLOC(1,sizeof(*physicalConfigDedicated2->schedulingRequestConfig));
  physicalConfigDedicated2->antennaInfo                   = CALLOC(1,sizeof(*physicalConfigDedicated2->antennaInfo));
  physicalConfigDedicated2->uplinkPowerControlDedicated   = CALLOC(1,sizeof(*physicalConfigDedicated2->uplinkPowerControlDedicated));
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH         = CALLOC(1,sizeof(*physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH));
  physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH         = CALLOC(1,sizeof(*physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH));

  // Update pdsch_ConfigDedicated
  if(radioResourceConfigDedicated->physicalConfigDedicated->pdsch_ConfigDedicated != NULL) {
    LOG_I(RRC,"Update pdsch_ConfigDedicated config \n");

    if(UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->pdsch_ConfigDedicated == NULL)
      UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->pdsch_ConfigDedicated = CALLOC(1,sizeof(LTE_PDSCH_ConfigDedicated_t));

    memcpy((char *)UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->pdsch_ConfigDedicated,
           (char *)radioResourceConfigDedicated->physicalConfigDedicated->pdsch_ConfigDedicated,
           sizeof(physicalConfigDedicated2->pdsch_ConfigDedicated));
  } else {
    LOG_I(RRC,"Keep old config for pdsch_ConfigDedicated\n");
  }

  // Update pusch_ConfigDedicated
  if(radioResourceConfigDedicated->physicalConfigDedicated->pusch_ConfigDedicated != NULL) {
    LOG_I(RRC,"Update pusch_ConfigDedicated config \n");

    if(UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->pusch_ConfigDedicated == NULL)
      UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->pusch_ConfigDedicated = CALLOC(1,sizeof(LTE_PUSCH_ConfigDedicated_t));

    memcpy((char *)UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->pusch_ConfigDedicated,
           (char *)radioResourceConfigDedicated->physicalConfigDedicated->pusch_ConfigDedicated,
           sizeof(physicalConfigDedicated2->pusch_ConfigDedicated));
  } else {
    LOG_I(RRC,"Keep old config for pusch_ConfigDedicated\n");
  }

  // Update pucch_ConfigDedicated
  if(radioResourceConfigDedicated->physicalConfigDedicated->pucch_ConfigDedicated != NULL) {
    LOG_I(RRC,"Update pucch_ConfigDedicated config \n");

    if(UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->pucch_ConfigDedicated == NULL)
      UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->pucch_ConfigDedicated = CALLOC(1,sizeof(LTE_PUCCH_ConfigDedicated_t));

    memcpy((char *)UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->pucch_ConfigDedicated,
           (char *)radioResourceConfigDedicated->physicalConfigDedicated->pucch_ConfigDedicated,
           sizeof(physicalConfigDedicated2->pucch_ConfigDedicated));
  } else {
    LOG_I(RRC,"Keep old config for pucch_ConfigDedicated\n");
  }

  // Update cqi_ReportConfig
  if(radioResourceConfigDedicated->physicalConfigDedicated->cqi_ReportConfig != NULL) {
    LOG_I(RRC,"Update cqi_ReportConfig config (size=%zu,%zu)\n", sizeof(*physicalConfigDedicated2->cqi_ReportConfig), sizeof(LTE_CQI_ReportConfig_t));

    if(UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->cqi_ReportConfig == NULL)
      UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->cqi_ReportConfig = CALLOC(1,sizeof(LTE_CQI_ReportConfig_t));

    memcpy((char *)UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->cqi_ReportConfig,
           (char *)radioResourceConfigDedicated->physicalConfigDedicated->cqi_ReportConfig,
           sizeof(*physicalConfigDedicated2->cqi_ReportConfig));

    if (radioResourceConfigDedicated->physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic != NULL) {
      LOG_I(RRC,"Update cqi_ReportPeriodic config (size=%zu,%zu)\n", sizeof(*physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic), sizeof(LTE_CQI_ReportPeriodic_t));

      if(UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->cqi_ReportConfig->cqi_ReportPeriodic == NULL)
        UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->cqi_ReportConfig->cqi_ReportPeriodic = CALLOC(1,sizeof(LTE_CQI_ReportPeriodic_t));

      memcpy((char *)UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->cqi_ReportConfig->cqi_ReportPeriodic,
             (char *)radioResourceConfigDedicated->physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic,
             sizeof(*physicalConfigDedicated2->cqi_ReportConfig->cqi_ReportPeriodic));
    }
  } else {
    LOG_I(RRC,"Keep old config for cqi_ReportConfig\n");
  }

  // Update schedulingRequestConfig
  if(radioResourceConfigDedicated->physicalConfigDedicated->schedulingRequestConfig != NULL) {
    LOG_I(RRC,"Update schedulingRequestConfig config \n");

    if(UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->schedulingRequestConfig == NULL)
      UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->schedulingRequestConfig = CALLOC(1,sizeof(LTE_SchedulingRequestConfig_t));

    memcpy((char *)UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->schedulingRequestConfig,
           (char *)radioResourceConfigDedicated->physicalConfigDedicated->schedulingRequestConfig,
           sizeof(physicalConfigDedicated2->schedulingRequestConfig));
  } else {
    LOG_I(RRC,"Keep old config for schedulingRequestConfig\n");
  }

  // Update soundingRS_UL_ConfigDedicated
  if(radioResourceConfigDedicated->physicalConfigDedicated->soundingRS_UL_ConfigDedicated != NULL) {
    LOG_I(RRC,"Update soundingRS_UL_ConfigDedicated config \n");

    if(UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->soundingRS_UL_ConfigDedicated == NULL)
      UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->soundingRS_UL_ConfigDedicated = CALLOC(1,sizeof(LTE_SoundingRS_UL_ConfigDedicated_t));

    memcpy((char *)UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->soundingRS_UL_ConfigDedicated,
           (char *)radioResourceConfigDedicated->physicalConfigDedicated->soundingRS_UL_ConfigDedicated,
           sizeof(physicalConfigDedicated2->soundingRS_UL_ConfigDedicated));
  } else {
    LOG_I(RRC,"Keep old config for soundingRS_UL_ConfigDedicated\n");
  }

  // Update antennaInfo
  if(radioResourceConfigDedicated->physicalConfigDedicated->antennaInfo != NULL) {
    LOG_I(RRC,"Update antennaInfo config \n");

    if(UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->antennaInfo == NULL)
      UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->antennaInfo = CALLOC(1,sizeof(struct LTE_PhysicalConfigDedicated__antennaInfo));

    memcpy((char *)UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->antennaInfo,
           (char *)radioResourceConfigDedicated->physicalConfigDedicated->antennaInfo,
           sizeof(physicalConfigDedicated2->antennaInfo));
    UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->antennaInfo->choice.explicitValue.transmissionMode =
      radioResourceConfigDedicated->physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode;
    UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->antennaInfo->choice.explicitValue.codebookSubsetRestriction =
      radioResourceConfigDedicated->physicalConfigDedicated->antennaInfo->choice.explicitValue.codebookSubsetRestriction;
    UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->antennaInfo->choice.explicitValue.ue_TransmitAntennaSelection =
      radioResourceConfigDedicated->physicalConfigDedicated->antennaInfo->choice.explicitValue.ue_TransmitAntennaSelection;
    LOG_I(PHY,"New Transmission Mode %ld \n",radioResourceConfigDedicated->physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode);
    LOG_I(PHY,"Configured Transmission Mode %ld \n",UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->antennaInfo->choice.explicitValue.transmissionMode);
  } else {
    LOG_I(RRC,"Keep old config for antennaInfo\n");
  }

  // Update uplinkPowerControlDedicated
  if(radioResourceConfigDedicated->physicalConfigDedicated->uplinkPowerControlDedicated != NULL) {
    LOG_I(RRC,"Update uplinkPowerControlDedicated config \n");

    if(UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->uplinkPowerControlDedicated == NULL)
      UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->uplinkPowerControlDedicated = CALLOC(1,sizeof(LTE_UplinkPowerControlDedicated_t));

    memcpy((char *)UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->uplinkPowerControlDedicated,
           (char *)radioResourceConfigDedicated->physicalConfigDedicated->uplinkPowerControlDedicated,
           sizeof(physicalConfigDedicated2->uplinkPowerControlDedicated));
  } else {
    LOG_I(RRC,"Keep old config for uplinkPowerControlDedicated\n");
  }

  // Update tpc_PDCCH_ConfigPUCCH
  if(radioResourceConfigDedicated->physicalConfigDedicated->tpc_PDCCH_ConfigPUCCH != NULL) {
    LOG_I(RRC,"Update tpc_PDCCH_ConfigPUCCH config \n");

    if(UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->tpc_PDCCH_ConfigPUCCH == NULL)
      UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->tpc_PDCCH_ConfigPUCCH = CALLOC(1,sizeof(LTE_TPC_PDCCH_Config_t));

    memcpy((char *)UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->tpc_PDCCH_ConfigPUCCH,
           (char *)radioResourceConfigDedicated->physicalConfigDedicated->tpc_PDCCH_ConfigPUCCH,
           sizeof(physicalConfigDedicated2->tpc_PDCCH_ConfigPUCCH));
  } else {
    LOG_I(RRC,"Keep old config for tpc_PDCCH_ConfigPUCCH\n");
  }

  // Update tpc_PDCCH_ConfigPUSCH
  if(radioResourceConfigDedicated->physicalConfigDedicated->tpc_PDCCH_ConfigPUSCH != NULL) {
    LOG_I(RRC,"Update tpc_PDCCH_ConfigPUSCH config \n");

    if(UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->tpc_PDCCH_ConfigPUSCH == NULL)
      UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->tpc_PDCCH_ConfigPUSCH = CALLOC(1,sizeof(LTE_TPC_PDCCH_Config_t));

    memcpy((char *)UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]->tpc_PDCCH_ConfigPUSCH,
           (char *)radioResourceConfigDedicated->physicalConfigDedicated->tpc_PDCCH_ConfigPUSCH,
           sizeof(physicalConfigDedicated2->tpc_PDCCH_ConfigPUSCH));
  } else {
    LOG_I(RRC,"Keep old config for tpc_PDCCH_ConfigPUSCH\n");
  }
}
//-----------------------------------------------------------------------------
void
rrc_ue_process_radioResourceConfigDedicated(
  const protocol_ctxt_t *const ctxt_pP,
  uint8_t eNB_index,
  LTE_RadioResourceConfigDedicated_t *radioResourceConfigDedicated
)
//-----------------------------------------------------------------------------
{
  long SRB_id,DRB_id;
  int i,cnt;
  LTE_LogicalChannelConfig_t *SRB1_logicalChannelConfig,*SRB2_logicalChannelConfig;

  // Save physicalConfigDedicated if present
  if (radioResourceConfigDedicated->physicalConfigDedicated) {
    LOG_I(RRC,"Save physicalConfigDedicated if present \n");

    if (UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index]) {
#if 1
      rrc_ue_update_radioResourceConfigDedicated(radioResourceConfigDedicated, ctxt_pP, eNB_index);
#else
      memcpy((char *)UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index],(char *)radioResourceConfigDedicated->physicalConfigDedicated,
             sizeof(struct PhysicalConfigDedicated));
#endif
    } else {
      LOG_I(RRC,"Init physicalConfigDedicated UE_rrc_inst to radioResourceConfigDedicated->physicalConfigDedicated\n");
      UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index] = radioResourceConfigDedicated->physicalConfigDedicated;
    }
    radioResourceConfigDedicated->physicalConfigDedicated = NULL;
  }

  // Apply macMainConfig if present
  if (radioResourceConfigDedicated->mac_MainConfig) {
    if (radioResourceConfigDedicated->mac_MainConfig->present == LTE_RadioResourceConfigDedicated__mac_MainConfig_PR_explicitValue) {
      if (UE_rrc_inst[ctxt_pP->module_id].mac_MainConfig[eNB_index]) {
        memcpy((char *)UE_rrc_inst[ctxt_pP->module_id].mac_MainConfig[eNB_index],(char *)&radioResourceConfigDedicated->mac_MainConfig->choice.explicitValue,
               sizeof(LTE_MAC_MainConfig_t));
      } else {
        UE_rrc_inst[ctxt_pP->module_id].mac_MainConfig[eNB_index] = &radioResourceConfigDedicated->mac_MainConfig->choice.explicitValue;
      }
    }
  }

  // Apply spsConfig if present
  if (radioResourceConfigDedicated->sps_Config) {
    if (UE_rrc_inst[ctxt_pP->module_id].sps_Config[eNB_index]) {
      memcpy(UE_rrc_inst[ctxt_pP->module_id].sps_Config[eNB_index],radioResourceConfigDedicated->sps_Config,
             sizeof(struct LTE_SPS_Config));
    } else {
      UE_rrc_inst[ctxt_pP->module_id].sps_Config[eNB_index] = radioResourceConfigDedicated->sps_Config;
    }
    radioResourceConfigDedicated->sps_Config = NULL;
  }

  // Establish SRBs if present
  // loop through SRBToAddModList
  if (radioResourceConfigDedicated->srb_ToAddModList) {
    uint8_t *kRRCenc = NULL;
    uint8_t *kRRCint = NULL;
    derive_key_rrc_enc(UE_rrc_inst[ctxt_pP->module_id].ciphering_algorithm,
                       UE_rrc_inst[ctxt_pP->module_id].kenb, &kRRCenc);
    derive_key_rrc_int(UE_rrc_inst[ctxt_pP->module_id].integrity_algorithm,
                       UE_rrc_inst[ctxt_pP->module_id].kenb, &kRRCint);
    // Refresh SRBs
    rrc_pdcp_config_asn1_req(ctxt_pP,
                             radioResourceConfigDedicated->srb_ToAddModList,
                             (LTE_DRB_ToAddModList_t *)NULL,
                             (LTE_DRB_ToReleaseList_t *)NULL,
                             UE_rrc_inst[ctxt_pP->module_id].ciphering_algorithm |
                             (UE_rrc_inst[ctxt_pP->module_id].integrity_algorithm << 4),
                             kRRCenc,
                             kRRCint,
                             NULL,
                             (LTE_PMCH_InfoList_r9_t *)NULL,
                             NULL);
    // Refresh SRBs
    rrc_rlc_config_asn1_req(ctxt_pP,
                            radioResourceConfigDedicated->srb_ToAddModList,
                            (LTE_DRB_ToAddModList_t *)NULL,
                            (LTE_DRB_ToReleaseList_t *)NULL,
                            (LTE_PMCH_InfoList_r9_t *)NULL,
                            0, 0
                           );
#if ENABLE_RAL
    // first msg that includes srb config
    UE_rrc_inst[ctxt_pP->module_id].num_srb=radioResourceConfigDedicated->srb_ToAddModList->list.count;
#endif

    for (cnt=0; cnt<radioResourceConfigDedicated->srb_ToAddModList->list.count; cnt++) {
      //  connection_reestablishment_ind.num_srb+=1;
      SRB_id = radioResourceConfigDedicated->srb_ToAddModList->list.array[cnt]->srb_Identity;
      LOG_D(RRC,"[UE %d]: Frame %d SRB config cnt %d (SRB%ld)\n",ctxt_pP->module_id,ctxt_pP->frame,cnt,SRB_id);

      if (SRB_id == 1) {
        if (UE_rrc_inst[ctxt_pP->module_id].SRB1_config[eNB_index]) {
          memcpy(UE_rrc_inst[ctxt_pP->module_id].SRB1_config[eNB_index],radioResourceConfigDedicated->srb_ToAddModList->list.array[cnt],
                 sizeof(struct LTE_SRB_ToAddMod));
        } else {
          UE_rrc_inst[ctxt_pP->module_id].SRB1_config[eNB_index] = radioResourceConfigDedicated->srb_ToAddModList->list.array[cnt];
          rrc_ue_establish_srb1(ctxt_pP->module_id,ctxt_pP->frame,eNB_index,radioResourceConfigDedicated->srb_ToAddModList->list.array[cnt]);
          radioResourceConfigDedicated->srb_ToAddModList->list.array[cnt] = NULL;
          if (UE_rrc_inst[ctxt_pP->module_id].SRB1_config[eNB_index]->logicalChannelConfig) {
            if (UE_rrc_inst[ctxt_pP->module_id].SRB1_config[eNB_index]->logicalChannelConfig->present == LTE_SRB_ToAddMod__logicalChannelConfig_PR_explicitValue) {
              SRB1_logicalChannelConfig = &UE_rrc_inst[ctxt_pP->module_id].SRB1_config[eNB_index]->logicalChannelConfig->choice.explicitValue;
            } else {
              SRB1_logicalChannelConfig = &SRB1_logicalChannelConfig_defaultValue;
            }
          } else {
            SRB1_logicalChannelConfig = &SRB1_logicalChannelConfig_defaultValue;
          }

          LOG_I(RRC, "[FRAME %05d][RRC_UE][MOD %02d][][--- MAC_CONFIG_REQ  (SRB1 eNB %d) --->][MAC_UE][MOD %02d][]\n",
                ctxt_pP->frame, ctxt_pP->module_id, eNB_index, ctxt_pP->module_id);
          rrc_mac_config_req_ue(ctxt_pP->module_id,0,eNB_index,
                                (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                                UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index],
                                (LTE_SCellToAddMod_r10_t *)NULL,
                                (LTE_MeasObjectToAddMod_t **)NULL,
                                UE_rrc_inst[ctxt_pP->module_id].mac_MainConfig[eNB_index],
                                1,
                                SRB1_logicalChannelConfig,
                                (LTE_MeasGapConfig_t *)NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                0,
                                (LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                                (LTE_PMCH_InfoList_r9_t *)NULL,
                                0,
                                NULL,
                                NULL,
                                0,
                                (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                                (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                               );
        }
      } else {
        if (UE_rrc_inst[ctxt_pP->module_id].SRB2_config[eNB_index]) {
          memcpy(UE_rrc_inst[ctxt_pP->module_id].SRB2_config[eNB_index],radioResourceConfigDedicated->srb_ToAddModList->list.array[cnt],
                 sizeof(struct LTE_SRB_ToAddMod));
        } else {
          UE_rrc_inst[ctxt_pP->module_id].SRB2_config[eNB_index] = radioResourceConfigDedicated->srb_ToAddModList->list.array[cnt];
          rrc_ue_establish_srb2(ctxt_pP->module_id,ctxt_pP->frame,eNB_index,radioResourceConfigDedicated->srb_ToAddModList->list.array[cnt]);
          radioResourceConfigDedicated->srb_ToAddModList->list.array[cnt] = NULL;
          if (UE_rrc_inst[ctxt_pP->module_id].SRB2_config[eNB_index]->logicalChannelConfig) {
            if (UE_rrc_inst[ctxt_pP->module_id].SRB2_config[eNB_index]->logicalChannelConfig->present == LTE_SRB_ToAddMod__logicalChannelConfig_PR_explicitValue) {
              LOG_I(RRC,"Applying Explicit SRB2 logicalChannelConfig\n");
              SRB2_logicalChannelConfig = &UE_rrc_inst[ctxt_pP->module_id].SRB2_config[eNB_index]->logicalChannelConfig->choice.explicitValue;
            } else {
              LOG_I(RRC,"Applying default SRB2 logicalChannelConfig\n");
              SRB2_logicalChannelConfig = &SRB2_logicalChannelConfig_defaultValue;
            }
          } else {
            SRB2_logicalChannelConfig = &SRB2_logicalChannelConfig_defaultValue;
          }

          LOG_I(RRC, "[FRAME %05d][RRC_UE][MOD %02d][][--- MAC_CONFIG_REQ  (SRB2 eNB %d) --->][MAC_UE][MOD %02d][]\n",
                ctxt_pP->frame,
                ctxt_pP->module_id,
                eNB_index,
                ctxt_pP->module_id);
          rrc_mac_config_req_ue(ctxt_pP->module_id,0,eNB_index,
                                (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                                UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index],
                                (LTE_SCellToAddMod_r10_t *)NULL,
                                (LTE_MeasObjectToAddMod_t **)NULL,
                                UE_rrc_inst[ctxt_pP->module_id].mac_MainConfig[eNB_index],
                                2,
                                SRB2_logicalChannelConfig,
                                UE_rrc_inst[ctxt_pP->module_id].measGapConfig[eNB_index],
                                (LTE_TDD_Config_t *)NULL,
                                (LTE_MobilityControlInfo_t *)NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                0,
                                (LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                                (LTE_PMCH_InfoList_r9_t *)NULL,
                                0,
                                NULL,
                                NULL,
                                0,
                                (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                                (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                               );
        }
      }
    }
  }

  // Establish DRBs if present
  if (radioResourceConfigDedicated->drb_ToAddModList) {
    if ( (UE_rrc_inst[ctxt_pP->module_id].defaultDRB == NULL) &&
         (radioResourceConfigDedicated->drb_ToAddModList->list.count >= 1) ) {
      // configure the first DRB ID as the default DRB ID
      UE_rrc_inst[ctxt_pP->module_id].defaultDRB = malloc(sizeof(rb_id_t));
      *UE_rrc_inst[ctxt_pP->module_id].defaultDRB = radioResourceConfigDedicated->drb_ToAddModList->list.array[0]->drb_Identity;
      LOG_I(RRC,"[UE %d] default DRB = %ld\n",ctxt_pP->module_id, *UE_rrc_inst[ctxt_pP->module_id].defaultDRB);
    }

    uint8_t *kUPenc = NULL;
    derive_key_up_enc(UE_rrc_inst[ctxt_pP->module_id].integrity_algorithm,
                      UE_rrc_inst[ctxt_pP->module_id].kenb, &kUPenc);
    MSC_LOG_TX_MESSAGE(
      MSC_RRC_UE,
      MSC_PDCP_UE,
      NULL,
      0,
      MSC_AS_TIME_FMT" CONFIG_REQ UE %x DRB (security %X)",
      MSC_AS_TIME_ARGS(ctxt_pP),
      ctxt_pP->rnti,
      UE_rrc_inst[ctxt_pP->module_id].ciphering_algorithm |
      (UE_rrc_inst[ctxt_pP->module_id].integrity_algorithm << 4));
    // Refresh DRBs
    rrc_pdcp_config_asn1_req(ctxt_pP,
                             (LTE_SRB_ToAddModList_t *)NULL,
                             radioResourceConfigDedicated->drb_ToAddModList,
                             (LTE_DRB_ToReleaseList_t *)NULL,
                             UE_rrc_inst[ctxt_pP->module_id].ciphering_algorithm |
                             (UE_rrc_inst[ctxt_pP->module_id].integrity_algorithm << 4),
                             NULL,
                             NULL,
                             kUPenc,
                             (LTE_PMCH_InfoList_r9_t *)NULL,
                             UE_rrc_inst[ctxt_pP->module_id].defaultDRB);
    // Refresh DRBs
    rrc_rlc_config_asn1_req(ctxt_pP,
                            (LTE_SRB_ToAddModList_t *)NULL,
                            radioResourceConfigDedicated->drb_ToAddModList,
                            (LTE_DRB_ToReleaseList_t *)NULL,
                            (LTE_PMCH_InfoList_r9_t *)NULL, 0, 0
                           );

    for (i=0; i<radioResourceConfigDedicated->drb_ToAddModList->list.count; i++) {
      DRB_id   = radioResourceConfigDedicated->drb_ToAddModList->list.array[i]->drb_Identity-1;

      if (UE_rrc_inst[ctxt_pP->module_id].DRB_config[eNB_index][DRB_id]) {
        memcpy(UE_rrc_inst[ctxt_pP->module_id].DRB_config[eNB_index][DRB_id],
               radioResourceConfigDedicated->drb_ToAddModList->list.array[i],
               sizeof(struct LTE_DRB_ToAddMod));
      } else {
        UE_rrc_inst[ctxt_pP->module_id].DRB_config[eNB_index][DRB_id] = radioResourceConfigDedicated->drb_ToAddModList->list.array[i];
        rrc_ue_establish_drb(ctxt_pP->module_id,ctxt_pP->frame,eNB_index,radioResourceConfigDedicated->drb_ToAddModList->list.array[i]);
        // MAC/PHY Configuration
        LOG_I(RRC, "[FRAME %05d][RRC_UE][MOD %02d][][--- MAC_CONFIG_REQ (DRB %ld eNB %d) --->][MAC_UE][MOD %02d][]\n",
              ctxt_pP->frame, ctxt_pP->module_id,
              radioResourceConfigDedicated->drb_ToAddModList->list.array[i]->drb_Identity,
              eNB_index,
              ctxt_pP->module_id);
        radioResourceConfigDedicated->drb_ToAddModList->list.array[i] = NULL;
        rrc_mac_config_req_ue(ctxt_pP->module_id,0,eNB_index,
                              (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                              UE_rrc_inst[ctxt_pP->module_id].physicalConfigDedicated[eNB_index],
                              (LTE_SCellToAddMod_r10_t *)NULL,
                              (LTE_MeasObjectToAddMod_t **)NULL,
                              UE_rrc_inst[ctxt_pP->module_id].mac_MainConfig[eNB_index],
                              *UE_rrc_inst[ctxt_pP->module_id].DRB_config[eNB_index][DRB_id]->logicalChannelIdentity,
                              UE_rrc_inst[ctxt_pP->module_id].DRB_config[eNB_index][DRB_id]->logicalChannelConfig,
                              UE_rrc_inst[ctxt_pP->module_id].measGapConfig[eNB_index],
                              (LTE_TDD_Config_t *)NULL,
                              (LTE_MobilityControlInfo_t *)NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              0,
                              (LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                              (LTE_PMCH_InfoList_r9_t *)NULL,
                              0,
                              NULL,
                              NULL,
                              0,
                              (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                              (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                             );
      }
    }
  }

  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].State = RRC_CONNECTED;
  LOG_I(RRC,"[UE %d] State = RRC_CONNECTED (eNB %d)\n",ctxt_pP->module_id,eNB_index);
}


//-----------------------------------------------------------------------------
void
rrc_ue_process_securityModeCommand(
  const protocol_ctxt_t *const ctxt_pP,
  LTE_SecurityModeCommand_t *const securityModeCommand,
  const uint8_t                eNB_index
)
//-----------------------------------------------------------------------------
{
  asn_enc_rval_t enc_rval;
  LTE_UL_DCCH_Message_t ul_dcch_msg;
  // SecurityModeCommand_t SecurityModeCommand;
  uint8_t buffer[200];
  int i, securityMode;
  LOG_I(RRC,"[UE %d] SFN/SF %d/%d: Receiving from SRB1 (DL-DCCH), Processing securityModeCommand (eNB %d)\n",
        ctxt_pP->module_id,ctxt_pP->frame, ctxt_pP->subframe, eNB_index);

  switch (securityModeCommand->criticalExtensions.choice.c1.choice.securityModeCommand_r8.securityConfigSMC.securityAlgorithmConfig.cipheringAlgorithm) {
    case LTE_CipheringAlgorithm_r12_eea0:
      LOG_I(RRC,"[UE %d] Security algorithm is set to eea0\n",
            ctxt_pP->module_id);
      securityMode= LTE_CipheringAlgorithm_r12_eea0;
      break;

    case LTE_CipheringAlgorithm_r12_eea1:
      LOG_I(RRC,"[UE %d] Security algorithm is set to eea1\n",ctxt_pP->module_id);
      securityMode= LTE_CipheringAlgorithm_r12_eea1;
      break;

    case LTE_CipheringAlgorithm_r12_eea2:
      LOG_I(RRC,"[UE %d] Security algorithm is set to eea2\n",
            ctxt_pP->module_id);
      securityMode = LTE_CipheringAlgorithm_r12_eea2;
      break;

    default:
      LOG_I(RRC,"[UE %d] Security algorithm is set to none\n",ctxt_pP->module_id);
      securityMode = LTE_CipheringAlgorithm_r12_spare1;
      break;
  }

  switch (securityModeCommand->criticalExtensions.choice.c1.choice.securityModeCommand_r8.securityConfigSMC.securityAlgorithmConfig.integrityProtAlgorithm) {
    case LTE_SecurityAlgorithmConfig__integrityProtAlgorithm_eia1:
      LOG_I(RRC,"[UE %d] Integrity protection algorithm is set to eia1\n",ctxt_pP->module_id);
      securityMode |= 1 << 5;
      break;

    case LTE_SecurityAlgorithmConfig__integrityProtAlgorithm_eia2:
      LOG_I(RRC,"[UE %d] Integrity protection algorithm is set to eia2\n",ctxt_pP->module_id);
      securityMode |= 1 << 6;
      break;

    default:
      LOG_I(RRC,"[UE %d] Integrity protection algorithm is set to none\n",ctxt_pP->module_id);
      securityMode |= 0x70 ;
      break;
  }

  LOG_D(RRC,"[UE %d] security mode is %x \n",ctxt_pP->module_id, securityMode);
  /* Store the parameters received */
  UE_rrc_inst[ctxt_pP->module_id].ciphering_algorithm =
    securityModeCommand->criticalExtensions.choice.c1.choice.securityModeCommand_r8.securityConfigSMC.securityAlgorithmConfig.cipheringAlgorithm;
  UE_rrc_inst[ctxt_pP->module_id].integrity_algorithm =
    securityModeCommand->criticalExtensions.choice.c1.choice.securityModeCommand_r8.securityConfigSMC.securityAlgorithmConfig.integrityProtAlgorithm;
  memset((void *)&ul_dcch_msg,0,sizeof(LTE_UL_DCCH_Message_t));
  //memset((void *)&SecurityModeCommand,0,sizeof(SecurityModeCommand_t));
  ul_dcch_msg.message.present           = LTE_UL_DCCH_MessageType_PR_c1;

  if (securityMode >= NO_SECURITY_MODE) {
    LOG_I(RRC, "rrc_ue_process_securityModeCommand, security mode complete case \n");
    ul_dcch_msg.message.choice.c1.present = LTE_UL_DCCH_MessageType__c1_PR_securityModeComplete;
  } else {
    LOG_I(RRC, "rrc_ue_process_securityModeCommand, security mode failure case \n");
    ul_dcch_msg.message.choice.c1.present = LTE_UL_DCCH_MessageType__c1_PR_securityModeFailure;
  }

  uint8_t *kRRCenc = NULL;
  uint8_t *kUPenc = NULL;
  uint8_t *kRRCint = NULL;
  pdcp_t *pdcp_p = NULL;
  hash_key_t key = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t h_rc;
  key = PDCP_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti,
                            ctxt_pP->enb_flag, DCCH, SRB_FLAG_YES);
  h_rc = hashtable_get(pdcp_coll_p, key, (void **) &pdcp_p);

  if (h_rc == HASH_TABLE_OK) {
    LOG_D(RRC, "PDCP_COLL_KEY_VALUE() returns valid key = %ld\n", key);
    LOG_D(RRC, "driving kRRCenc, kRRCint and kUPenc from KeNB="
          "%02x%02x%02x%02x"
          "%02x%02x%02x%02x"
          "%02x%02x%02x%02x"
          "%02x%02x%02x%02x"
          "%02x%02x%02x%02x"
          "%02x%02x%02x%02x"
          "%02x%02x%02x%02x"
          "%02x%02x%02x%02x\n",
          UE_rrc_inst[ctxt_pP->module_id].kenb[0],  UE_rrc_inst[ctxt_pP->module_id].kenb[1],  UE_rrc_inst[ctxt_pP->module_id].kenb[2],  UE_rrc_inst[ctxt_pP->module_id].kenb[3],
          UE_rrc_inst[ctxt_pP->module_id].kenb[4],  UE_rrc_inst[ctxt_pP->module_id].kenb[5],  UE_rrc_inst[ctxt_pP->module_id].kenb[6],  UE_rrc_inst[ctxt_pP->module_id].kenb[7],
          UE_rrc_inst[ctxt_pP->module_id].kenb[8],  UE_rrc_inst[ctxt_pP->module_id].kenb[9],  UE_rrc_inst[ctxt_pP->module_id].kenb[10], UE_rrc_inst[ctxt_pP->module_id].kenb[11],
          UE_rrc_inst[ctxt_pP->module_id].kenb[12], UE_rrc_inst[ctxt_pP->module_id].kenb[13], UE_rrc_inst[ctxt_pP->module_id].kenb[14], UE_rrc_inst[ctxt_pP->module_id].kenb[15],
          UE_rrc_inst[ctxt_pP->module_id].kenb[16], UE_rrc_inst[ctxt_pP->module_id].kenb[17], UE_rrc_inst[ctxt_pP->module_id].kenb[18], UE_rrc_inst[ctxt_pP->module_id].kenb[19],
          UE_rrc_inst[ctxt_pP->module_id].kenb[20], UE_rrc_inst[ctxt_pP->module_id].kenb[21], UE_rrc_inst[ctxt_pP->module_id].kenb[22], UE_rrc_inst[ctxt_pP->module_id].kenb[23],
          UE_rrc_inst[ctxt_pP->module_id].kenb[24], UE_rrc_inst[ctxt_pP->module_id].kenb[25], UE_rrc_inst[ctxt_pP->module_id].kenb[26], UE_rrc_inst[ctxt_pP->module_id].kenb[27],
          UE_rrc_inst[ctxt_pP->module_id].kenb[28], UE_rrc_inst[ctxt_pP->module_id].kenb[29], UE_rrc_inst[ctxt_pP->module_id].kenb[30], UE_rrc_inst[ctxt_pP->module_id].kenb[31]);
    derive_key_rrc_enc(UE_rrc_inst[ctxt_pP->module_id].ciphering_algorithm,
                       UE_rrc_inst[ctxt_pP->module_id].kenb, &kRRCenc);
    derive_key_rrc_int(UE_rrc_inst[ctxt_pP->module_id].integrity_algorithm,
                       UE_rrc_inst[ctxt_pP->module_id].kenb, &kRRCint);
    derive_key_up_enc(UE_rrc_inst[ctxt_pP->module_id].ciphering_algorithm,
                      UE_rrc_inst[ctxt_pP->module_id].kenb, &kUPenc);

    if (securityMode != 0xff) {
      pdcp_config_set_security(ctxt_pP, pdcp_p, 0, 0,
                               UE_rrc_inst[ctxt_pP->module_id].ciphering_algorithm
                               | (UE_rrc_inst[ctxt_pP->module_id].integrity_algorithm << 4),
                               kRRCenc, kRRCint, kUPenc);
    } else {
      LOG_I(RRC, "skipped pdcp_config_set_security() as securityMode == 0x%02x",
            securityMode);
    }
  } else {
    LOG_I(RRC, "Could not get PDCP instance where key=0x%ld\n", key);
  }

  if (securityModeCommand->criticalExtensions.present == LTE_SecurityModeCommand__criticalExtensions_PR_c1) {
    if (securityModeCommand->criticalExtensions.choice.c1.present != LTE_SecurityModeCommand__criticalExtensions__c1_PR_securityModeCommand_r8)
      LOG_W(RRC,"securityModeCommand->criticalExtensions.choice.c1.present (%d) != SecurityModeCommand__criticalExtensions__c1_PR_securityModeCommand_r8\n",
            securityModeCommand->criticalExtensions.choice.c1.present);

    ul_dcch_msg.message.choice.c1.choice.securityModeComplete.rrc_TransactionIdentifier = securityModeCommand->rrc_TransactionIdentifier;
    ul_dcch_msg.message.choice.c1.choice.securityModeComplete.criticalExtensions.present = LTE_SecurityModeCommand__criticalExtensions_PR_c1;
    ul_dcch_msg.message.choice.c1.choice.securityModeComplete.criticalExtensions.choice.securityModeComplete_r8.nonCriticalExtension =NULL;
    LOG_I(RRC,"[UE %d] SFN/SF %d/%d: Receiving from SRB1 (DL-DCCH), encoding securityModeComplete (eNB %d), rrc_TransactionIdentifier: %ld\n",
          ctxt_pP->module_id,ctxt_pP->frame, ctxt_pP->subframe, eNB_index, securityModeCommand->rrc_TransactionIdentifier);
    enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_UL_DCCH_Message,
                                     NULL,
                                     (void *)&ul_dcch_msg,
                                     buffer,
                                     100);
    AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
                 enc_rval.failed_type->name, enc_rval.encoded);

    if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
      xer_fprint(stdout, &asn_DEF_LTE_UL_DCCH_Message, (void *)&ul_dcch_msg);
    }

    LOG_D(RRC, "securityModeComplete Encoded %zd bits (%zd bytes)\n", enc_rval.encoded, (enc_rval.encoded+7)/8);

    for (i = 0; i < (enc_rval.encoded + 7) / 8; i++) {
      LOG_T(RRC, "%02x.", buffer[i]);
    }

    LOG_T(RRC, "\n");
    rrc_data_req (
      ctxt_pP,
      DCCH,
      rrc_mui++,
      SDU_CONFIRM_NO,
      (enc_rval.encoded + 7) / 8,
      buffer,
      PDCP_TRANSMISSION_MODE_CONTROL);
  } else LOG_W(RRC,"securityModeCommand->criticalExtensions.present (%d) != SecurityModeCommand__criticalExtensions_PR_c1\n",
                 securityModeCommand->criticalExtensions.present);
}

void
rrc_ue_process_MBMSCountingRequest(
  const protocol_ctxt_t *const ctxt_pP,
  LTE_MBMSCountingRequest_r10_t *MBMSCountingRequest,
  uint8_t eNB_index
)
{
  asn_enc_rval_t enc_rval;
  LTE_UL_DCCH_Message_t ul_dcch_msg;
  struct LTE_CountingResponseInfo_r10 CountingResponse;
  uint8_t buffer[200];
  //int i;
  LOG_I(RRC,"[UE %d] Frame %d: Receiving from (MCCH), Processing MBMSCoutingRequest (eNB %d)\n",
        ctxt_pP->module_id,
        ctxt_pP->frame,
        eNB_index);
  memset((void *)&ul_dcch_msg,0,sizeof(LTE_UL_DCCH_Message_t));
  memset((void *)&CountingResponse,0,sizeof(struct LTE_CountingResponseInfo_r10));


  ul_dcch_msg.message.present           = LTE_UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1.present = LTE_UL_DCCH_MessageType__c1_PR_mbmsCountingResponse_r10;
  LTE_MBMSCountingResponse_r10_t *MBMSCountingResponse = &ul_dcch_msg.message.choice.c1.choice.mbmsCountingResponse_r10;

  MBMSCountingResponse->criticalExtensions.present = LTE_MBMSCountingResponse_r10__criticalExtensions_PR_c1;
  MBMSCountingResponse->criticalExtensions.choice.c1.present = LTE_MBMSCountingResponse_r10__criticalExtensions__c1_PR_countingResponse_r10; 
  LTE_MBMSCountingResponse_r10_IEs_t *MBMSCountingResponse_r10_IEs = &MBMSCountingResponse->criticalExtensions.choice.c1.choice.countingResponse_r10;

  MBMSCountingResponse_r10_IEs->mbsfn_AreaIndex_r10 = calloc(1,sizeof(long));
 // MBMSCountingResponse_r10_IEs->countingResponseList_r10 = calloc(1,sizeof(struct LTE_CountingResponseList_r10));
//
//  ASN_SEQUENCE_ADD(
//        &MBMSCountingResponse->criticalExtensions.choice.c1.choice.countingResponse_r10.countingResponseList_r10.list,
//        &CountingResponse);
//

  enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_UL_DCCH_Message, NULL, (void *) &ul_dcch_msg, buffer, 100);
      AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
                   enc_rval.failed_type->name, enc_rval.encoded);

      if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
        xer_fprint(stdout, &asn_DEF_LTE_UL_DCCH_Message, (void *)&ul_dcch_msg);
      }
        xer_fprint(stdout, &asn_DEF_LTE_UL_DCCH_Message, (void *)&ul_dcch_msg);


 LOG_I(RRC,"MBMSCountingResponse Encoded %zd bits (%zd bytes)\n",enc_rval.encoded,(enc_rval.encoded+7)/8);
      rrc_data_req_ue (
        &ctxt_pP_local,
        DCCH,
        rrc_mui++,
        SDU_CONFIRM_NO,
        (enc_rval.encoded + 7) / 8,
        buffer,
        PDCP_TRANSMISSION_MODE_CONTROL);

}


//-----------------------------------------------------------------------------
void
rrc_ue_process_nrueCapabilityEnquiry(
  const protocol_ctxt_t *const ctxt_pP,
  LTE_UECapabilityEnquiry_t *UECapabilityEnquiry,
  rrc_nrue_cap_info_t *nrue_cap_info,
  uint8_t eNB_index
)
//-----------------------------------------------------------------------------
{
  asn_enc_rval_t enc_rval;
  uint8_t buffer[RRC_BUF_SIZE];
  LOG_I(RRC,"[UE %d] Frame %d: Receiving from SRB1 (DL-DCCH), Processing NRUECapabilityEnquiry (eNB %d)\n",
        ctxt_pP->module_id,
        ctxt_pP->frame,
        eNB_index);
  LTE_UL_DCCH_Message_t ul_dcch_msg;
  memset(&ul_dcch_msg, 0, sizeof(ul_dcch_msg));
  LTE_UECapabilityInformation_t *ue_cap = &ul_dcch_msg.message.choice.c1.choice.ueCapabilityInformation;
  ul_dcch_msg.message.present = LTE_UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1.present = LTE_UL_DCCH_MessageType__c1_PR_ueCapabilityInformation;
  ue_cap->rrc_TransactionIdentifier = UECapabilityEnquiry->rrc_TransactionIdentifier;

  NR_UE_CapabilityRAT_Container_t ue_CapabilityRAT_Container;
  memset(&ue_CapabilityRAT_Container, 0, sizeof(ue_CapabilityRAT_Container));
  ue_CapabilityRAT_Container.rat_Type = NR_RAT_Type_nr;
  OCTET_STRING_fromBuf(&ue_CapabilityRAT_Container.ue_CapabilityRAT_Container,
                       (const char *)nrue_cap_info->mesg,
                       nrue_cap_info->mesg_len);
  # if(1) //Melissa: This is a hack. The MRDC capabilites should be filled in the NR UE
  NR_UE_CapabilityRAT_Container_t ue_CapabilityRAT_Container_mrdc;
  memset(&ue_CapabilityRAT_Container_mrdc, 0, sizeof(ue_CapabilityRAT_Container_mrdc));
  uint8_t buffer_mrdc[RRC_BUF_SIZE];
  NR_UE_MRDC_Capability_t *UE_Capability_MRDC = CALLOC(1, sizeof(NR_UE_MRDC_Capability_t));
  asn_enc_rval_t enc_rval_mrdc = uper_encode_to_buffer(&asn_DEF_NR_UE_MRDC_Capability,
                                   NULL,
                                   (void *)UE_Capability_MRDC,
                                   &buffer_mrdc,
                                   sizeof(buffer_mrdc));
  AssertFatal (enc_rval_mrdc.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
               enc_rval_mrdc.failed_type->name, enc_rval_mrdc.encoded);
  LOG_I(RRC, "[NR_RRC] NRUE MRDC Capability encoded, %ld bytes (%ld bits)\n",
        (enc_rval_mrdc.encoded + 7) / 8, enc_rval_mrdc.encoded + 7);

  ue_CapabilityRAT_Container_mrdc.rat_Type = NR_RAT_Type_eutra_nr;
  OCTET_STRING_fromBuf(&ue_CapabilityRAT_Container_mrdc.ue_CapabilityRAT_Container,
                       (const char *)buffer_mrdc,
                       (enc_rval_mrdc.encoded + 7) / 8);
  #endif

  ue_cap->criticalExtensions.present           = LTE_UECapabilityInformation__criticalExtensions_PR_c1;
  ue_cap->criticalExtensions.choice.c1.present = LTE_UECapabilityInformation__criticalExtensions__c1_PR_ueCapabilityInformation_r8;
  ue_cap->criticalExtensions.choice.c1.choice.ueCapabilityInformation_r8.ue_CapabilityRAT_ContainerList.list.count = 0;
  int count = UECapabilityEnquiry->criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8.ue_CapabilityRequest.list.count;
  xer_fprint(stdout, &asn_DEF_NR_UE_CapabilityRAT_Container, (void *)&ue_CapabilityRAT_Container);
  LTE_UE_CapabilityRequest_t *cap_req = &UECapabilityEnquiry->criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8.ue_CapabilityRequest;
  for (int i = 0; i < count; i++) {
    enc_rval.encoded = 0;
    if (*cap_req->list.array[i] == LTE_RAT_Type_nr) {
        ASN_SEQUENCE_ADD(&ue_cap->criticalExtensions.choice.c1.choice.ueCapabilityInformation_r8.ue_CapabilityRAT_ContainerList.list,
                         &ue_CapabilityRAT_Container);
        ue_cap->criticalExtensions.choice.c1.choice.ueCapabilityInformation_r8.ue_CapabilityRAT_ContainerList.list.array[i]->rat_Type = LTE_RAT_Type_nr;
        asn_enc_rval_t enc_rval_nr = uper_encode_to_buffer(&asn_DEF_LTE_UL_DCCH_Message, NULL, (void *) &ul_dcch_msg, buffer, sizeof(buffer));
        AssertFatal (enc_rval_nr.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
                     enc_rval_nr.failed_type->name, enc_rval_nr.encoded);
        enc_rval.encoded = enc_rval.encoded + enc_rval_nr.encoded;
        xer_fprint(stdout, &asn_DEF_LTE_UL_DCCH_Message, (void *)&ul_dcch_msg);
        LOG_A(RRC, "%s: NR_UECapInfo LTE_RAT_Type_nr Encoded %zd bits (%zd bytes)\n",
              __FUNCTION__, enc_rval.encoded, (enc_rval.encoded+7)/8);
    }
    else if (*cap_req->list.array[i] == LTE_RAT_Type_eutra_nr) {
        ASN_SEQUENCE_ADD(&ue_cap->criticalExtensions.choice.c1.choice.ueCapabilityInformation_r8.ue_CapabilityRAT_ContainerList.list,
                         &ue_CapabilityRAT_Container_mrdc);
        ue_cap->criticalExtensions.choice.c1.choice.ueCapabilityInformation_r8.ue_CapabilityRAT_ContainerList.list.array[i]->rat_Type = LTE_RAT_Type_eutra_nr;
        asn_enc_rval_t enc_rval_eutra_nr = uper_encode_to_buffer(&asn_DEF_LTE_UL_DCCH_Message, NULL, (void *) &ul_dcch_msg, buffer, sizeof(buffer));
        AssertFatal (enc_rval_eutra_nr.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
                     enc_rval_eutra_nr.failed_type->name, enc_rval_eutra_nr.encoded);
        enc_rval.encoded = enc_rval.encoded + enc_rval_eutra_nr.encoded;
        xer_fprint(stdout, &asn_DEF_LTE_UL_DCCH_Message, (void *)&ul_dcch_msg);
        LOG_A(RRC, "%s: NR_UECapInfo LTE_RAT_Type_eutra_nr Encoded %zd bits (%zd bytes)\n",
              __FUNCTION__, enc_rval.encoded, (enc_rval.encoded+7)/8);
    }
    rrc_data_req_ue (
      ctxt_pP,
      DCCH,
      rrc_mui++,
      SDU_CONFIRM_NO,
      (enc_rval.encoded + 7) / 8,
      buffer,
      PDCP_TRANSMISSION_MODE_CONTROL);
  }
}



//-----------------------------------------------------------------------------
void
rrc_ue_process_ueCapabilityEnquiry(
  const protocol_ctxt_t *const ctxt_pP,
  LTE_UECapabilityEnquiry_t *UECapabilityEnquiry,
  uint8_t eNB_index
)
//-----------------------------------------------------------------------------
{
  asn_enc_rval_t enc_rval;
  LTE_UL_DCCH_Message_t ul_dcch_msg;
  LTE_UE_CapabilityRAT_Container_t ue_CapabilityRAT_Container;
  uint8_t buffer[200];
  int i;
  LOG_I(RRC,"[UE %d] Frame %d: Receiving from SRB1 (DL-DCCH), Processing UECapabilityEnquiry (eNB %d)\n",
        ctxt_pP->module_id,
        ctxt_pP->frame,
        eNB_index);
  memset((void *)&ul_dcch_msg,0,sizeof(LTE_UL_DCCH_Message_t));
  memset((void *)&ue_CapabilityRAT_Container,0,sizeof(LTE_UE_CapabilityRAT_Container_t));
  ul_dcch_msg.message.present           = LTE_UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1.present = LTE_UL_DCCH_MessageType__c1_PR_ueCapabilityInformation;
  ul_dcch_msg.message.choice.c1.choice.ueCapabilityInformation.rrc_TransactionIdentifier = UECapabilityEnquiry->rrc_TransactionIdentifier;
  ue_CapabilityRAT_Container.rat_Type = LTE_RAT_Type_eutra;
  OCTET_STRING_fromBuf(&ue_CapabilityRAT_Container.ueCapabilityRAT_Container,
                       (const char *)UE_rrc_inst[ctxt_pP->module_id].UECapability,
                       UE_rrc_inst[ctxt_pP->module_id].UECapability_size);
  //  ue_CapabilityRAT_Container.ueCapabilityRAT_Container.buf  = UE_rrc_inst[ue_mod_idP].UECapability;
  // ue_CapabilityRAT_Container.ueCapabilityRAT_Container.size = UE_rrc_inst[ue_mod_idP].UECapability_size;
  AssertFatal(UECapabilityEnquiry->criticalExtensions.present == LTE_UECapabilityEnquiry__criticalExtensions_PR_c1,
              "UECapabilityEnquiry->criticalExtensions.present (%d) != UECapabilityEnquiry__criticalExtensions_PR_c1 (%d)\n",
              UECapabilityEnquiry->criticalExtensions.present,LTE_UECapabilityEnquiry__criticalExtensions_PR_c1);

  if (UECapabilityEnquiry->criticalExtensions.choice.c1.present != LTE_UECapabilityEnquiry__criticalExtensions__c1_PR_ueCapabilityEnquiry_r8)
    LOG_I(RRC,"UECapabilityEnquiry->criticalExtensions.choice.c1.present (%d) != UECapabilityEnquiry__criticalExtensions__c1_PR_ueCapabilityEnquiry_r8)\n",
          UECapabilityEnquiry->criticalExtensions.choice.c1.present);

  ul_dcch_msg.message.choice.c1.choice.ueCapabilityInformation.criticalExtensions.present           = LTE_UECapabilityInformation__criticalExtensions_PR_c1;
  ul_dcch_msg.message.choice.c1.choice.ueCapabilityInformation.criticalExtensions.choice.c1.present =
    LTE_UECapabilityInformation__criticalExtensions__c1_PR_ueCapabilityInformation_r8;
  ul_dcch_msg.message.choice.c1.choice.ueCapabilityInformation.criticalExtensions.choice.c1.choice.ueCapabilityInformation_r8.ue_CapabilityRAT_ContainerList.list.count
    =0;

  for (i=0; i<UECapabilityEnquiry->criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8.ue_CapabilityRequest.list.count; i++) {
    if (*UECapabilityEnquiry->criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8.ue_CapabilityRequest.list.array[i]
        == LTE_RAT_Type_eutra) {
      ASN_SEQUENCE_ADD(
        &ul_dcch_msg.message.choice.c1.choice.ueCapabilityInformation.criticalExtensions.choice.c1.choice.ueCapabilityInformation_r8.ue_CapabilityRAT_ContainerList.list,
        &ue_CapabilityRAT_Container);
      enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_UL_DCCH_Message, NULL, (void *) &ul_dcch_msg, buffer, 100);
      AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
                   enc_rval.failed_type->name, enc_rval.encoded);

      if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
        xer_fprint(stdout, &asn_DEF_LTE_UL_DCCH_Message, (void *)&ul_dcch_msg);
      }

      LOG_A(RRC, "%s: UECapabilityInformation Encoded %zd bits (%zd bytes)\n",
            __FUNCTION__, enc_rval.encoded,(enc_rval.encoded+7)/8);
      rrc_data_req_ue (
        ctxt_pP,
        DCCH,
        rrc_mui++,
        SDU_CONFIRM_NO,
        (enc_rval.encoded + 7) / 8,
        buffer,
        PDCP_TRANSMISSION_MODE_CONTROL);
    }
    else if (*UECapabilityEnquiry->criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8.ue_CapabilityRequest.list.array[i]
            == LTE_RAT_Type_nr) {
        ASN_SEQUENCE_ADD(
          &ul_dcch_msg.message.choice.c1.choice.ueCapabilityInformation.criticalExtensions.choice.c1.choice.ueCapabilityInformation_r8.ue_CapabilityRAT_ContainerList.list,
          &ue_CapabilityRAT_Container);
        enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_UL_DCCH_Message, NULL, (void *) &ul_dcch_msg, buffer, 100);
        AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
                    enc_rval.failed_type->name, enc_rval.encoded);

        if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
          xer_fprint(stdout, &asn_DEF_LTE_UL_DCCH_Message, (void *)&ul_dcch_msg);
        }

        LOG_A(RRC, "%s: NR_UECapabilityInformation Encoded %zd bits (%zd bytes)\n",
              __FUNCTION__, enc_rval.encoded,(enc_rval.encoded+7)/8);
        rrc_data_req_ue (
          ctxt_pP,
          DCCH,
          rrc_mui++,
          SDU_CONFIRM_NO,
          (enc_rval.encoded + 7) / 8,
          buffer,
          PDCP_TRANSMISSION_MODE_CONTROL);
        }
  }
}

static bool is_nr_r15_config_present(LTE_RRCConnectionReconfiguration_r8_IEs_t *c)
{
#define NCE nonCriticalExtension
#define chk(x) do { \
  if ((x) == NULL) { \
    LOG_I(RRC, "NULL at %d\n", __LINE__); \
    return false; \
  } \
} while(0)
  chk(c);
  chk(c->NCE);
  chk(c->NCE->NCE);
  chk(c->NCE->NCE->NCE);
  chk(c->NCE->NCE->NCE->NCE);
  chk(c->NCE->NCE->NCE->NCE->NCE);
  chk(c->NCE->NCE->NCE->NCE->NCE->NCE);
  chk(c->NCE->NCE->NCE->NCE->NCE->NCE->NCE);
  chk(c->NCE->NCE->NCE->NCE->NCE->NCE->NCE->NCE);
  return c->NCE->NCE->NCE->NCE->NCE->NCE->NCE->NCE->nr_Config_r15->present ==
         LTE_RRCConnectionReconfiguration_v1510_IEs__nr_Config_r15_PR_setup;

#undef NCE
#undef chk
}

//-----------------------------------------------------------------------------
void
rrc_ue_process_rrcConnectionReconfiguration(
  const protocol_ctxt_t *const       ctxt_pP,
  LTE_RRCConnectionReconfiguration_t *rrcConnectionReconfiguration,
  uint8_t eNB_index
)
//-----------------------------------------------------------------------------
{
  LOG_I(RRC,"[UE %d] Frame %d: Receiving from SRB1 (DL-DCCH), Processing RRCConnectionReconfiguration (eNB %d)\n",
        ctxt_pP->module_id,ctxt_pP->frame,eNB_index);

  if (rrcConnectionReconfiguration->criticalExtensions.present ==
      LTE_RRCConnectionReconfiguration__criticalExtensions_PR_c1) {
    if (rrcConnectionReconfiguration->criticalExtensions.choice.c1.present ==
        LTE_RRCConnectionReconfiguration__criticalExtensions__c1_PR_rrcConnectionReconfiguration_r8) {
      LTE_RRCConnectionReconfiguration_r8_IEs_t *r_r8 = &rrcConnectionReconfiguration->
                                                         criticalExtensions.choice.c1.
                                                         choice.rrcConnectionReconfiguration_r8;

      if (is_nr_r15_config_present(r_r8)) {
          OCTET_STRING_t *nr_RadioBearer = r_r8->nonCriticalExtension->nonCriticalExtension->
                                            nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->
                                            nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->
                                            nr_RadioBearerConfig1_r15;
          OCTET_STRING_t *nr_SecondaryCellGroup = r_r8->nonCriticalExtension->nonCriticalExtension->
                                            nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->
                                            nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->
                                            nr_Config_r15->choice.setup.nr_SecondaryCellGroupConfig_r15;
          uint32_t total_size = nr_RadioBearer->size + nr_SecondaryCellGroup->size;
          struct msg {
                uint32_t RadioBearer_size;
                uint32_t SecondaryCellGroup_size;
                uint8_t trans_id;
                uint8_t padding[3];
                uint8_t buffer[total_size];
          } msg;

          msg.RadioBearer_size = nr_RadioBearer->size;
          msg.SecondaryCellGroup_size = nr_SecondaryCellGroup->size;
          msg.trans_id = rrcConnectionReconfiguration->rrc_TransactionIdentifier;
          memcpy(msg.buffer, nr_RadioBearer->buf, nr_RadioBearer->size);
          memcpy(msg.buffer + nr_RadioBearer->size, nr_SecondaryCellGroup->buf, nr_SecondaryCellGroup->size);

          LOG_D(RRC, "nr_RadioBearerConfig1_r15 size %ld nr_SecondaryCellGroupConfig_r15 size %ld, sizeof(msg) = %zu\n",
                      nr_RadioBearer->size,
                      nr_SecondaryCellGroup->size,
                      sizeof(msg));

          nsa_sendmsg_to_nrue(&msg, sizeof(msg), RRC_CONFIG_COMPLETE_REQ);
          LOG_A(RRC, "Sent RRC_CONFIG_COMPLETE_REQ to the NR UE\n");
          #if 0 //Melissa, this is a hack. We need the transaction_id from latest dl_dcch_msg the LTE UE received. (Ln 6658)
          LTE_RRCConnectionReconfiguration_t *rrc = &UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].dl_dcch_msg->message.
                                                    choice.c1.choice.rrcConnectionReconfiguration;
          if (rrc != NULL) {
            SEQUENCE_free(&asn_DEF_LTE_RRCConnectionReconfiguration, rrc, ASFM_FREE_EVERYTHING);
          }
          rrc->rrc_TransactionIdentifier = rrcConnectionReconfiguration->rrc_TransactionIdentifier;
          rrcConnectionReconfiguration = NULL;
          #endif
      }

      if (r_r8->mobilityControlInfo) {
        LOG_I(RRC,"Mobility Control Information is present\n");
        rrc_ue_process_mobilityControlInfo(
          ctxt_pP,
          eNB_index,
          r_r8->mobilityControlInfo);
      }

      if (r_r8->measConfig != NULL) {
        LOG_I(RRC,"Measurement Configuration is present\n");
        rrc_ue_process_measConfig(ctxt_pP,
                                  eNB_index,
                                  r_r8->measConfig);
        r_r8->measConfig = NULL;
      }

      if (r_r8->radioResourceConfigDedicated) {
        LOG_I(RRC,"Radio Resource Configuration is present\n");
        rrc_ue_process_radioResourceConfigDedicated(ctxt_pP,
                                                    eNB_index,
                                                    r_r8->radioResourceConfigDedicated);
        r_r8->radioResourceConfigDedicated = NULL;
      }

      //TTN for D2D
      //if RRCConnectionReconfiguration message includes the sl-CommConfig
      if ((r_r8->nonCriticalExtension != NULL)
          && (r_r8->nonCriticalExtension->nonCriticalExtension != NULL)
          && (r_r8->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension != NULL)
          && (r_r8->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension != NULL)
          && (r_r8->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
              != NULL)
          && (r_r8->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->sl_CommConfig_r12
              != NULL)) {
        if (r_r8->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->sl_CommConfig_r12->commTxResources_r12->present !=
            LTE_SL_CommConfig_r12__commTxResources_r12_PR_NOTHING) {
          LOG_I(RRC,"sl-CommConfig is present\n");
          //process sl-CommConfig
          rrc_ue_process_sidelink_radioResourceConfig(ctxt_pP->module_id,eNB_index,
              (LTE_SystemInformationBlockType18_r12_t *)NULL,
              (LTE_SystemInformationBlockType19_r12_t *)NULL,
              r_r8->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->sl_CommConfig_r12,
              (LTE_SL_DiscConfig_r12_t *)NULL
                                                     );
        }
      }

      /*
            //if RRCConnectionReconfiguration message includes the sl-DiscConfig
            if (r_r8->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->sl_DiscConfig_r12->discTxResources_r12->present != SL_DiscConfig_r12__discTxResources_r12_PR_NOTHING ){
               LOG_I(RRC,"sl-DiscConfig is present\n");
               //process sl-DiscConfig
               rrc_ue_process_sidelink_radioResourceConfig(ctxt_pP->module_id,eNB_index,
                     (SystemInformationBlockType18_r12_t *)NULL,
                     (SystemInformationBlockType19_r12_t *)NULL,
                     (SL_CommConfig_r12_t* )NULL,
                     r_r8->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->sl_DiscConfig_r12
                     );
            }
      */

      /* Check if there is dedicated NAS information to forward to NAS */
      if (r_r8->dedicatedInfoNASList != NULL) {
        int list_count;
        uint32_t pdu_length;
        uint8_t *pdu_buffer;
        MessageDef *msg_p;

        for (list_count = 0; list_count < r_r8->dedicatedInfoNASList->list.count; list_count++) {
          pdu_length = r_r8->dedicatedInfoNASList->list.array[list_count]->size;
          pdu_buffer = r_r8->dedicatedInfoNASList->list.array[list_count]->buf;
          msg_p = itti_alloc_new_message(TASK_RRC_UE, 0, NAS_CONN_ESTABLI_CNF);
          NAS_CONN_ESTABLI_CNF(msg_p).errCode = AS_SUCCESS;
          NAS_CONN_ESTABLI_CNF(msg_p).nasMsg.length = pdu_length;
          NAS_CONN_ESTABLI_CNF(msg_p).nasMsg.data = pdu_buffer;
          itti_send_msg_to_task(TASK_NAS_UE, ctxt_pP->instance, msg_p);
        }
        LOG_I(RRC, "Melissa Elkadi, we have sent NAS_CONN_ESTABLI_CNF to NAS layer via itti!\n");

        free (r_r8->dedicatedInfoNASList);
      }

#if ENABLE_RAL
      {
        MessageDef                                 *message_ral_p = NULL;
        rrc_ral_connection_reestablishment_ind_t    connection_reestablishment_ind;
        int                                         i;
        message_ral_p = itti_alloc_new_message (TASK_RRC_UE, 0, RRC_RAL_CONNECTION_REESTABLISHMENT_IND);
        memset(&connection_reestablishment_ind, 0, sizeof(rrc_ral_connection_reestablishment_ind_t));
        // TO DO ral_si_ind.plmn_id        = 0;
        connection_reestablishment_ind.ue_id            = ctxt_pP->rnti;

        if (rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList != NULL) {
          connection_reestablishment_ind.num_drb      =
            rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList->list.count;

          for (i=0; (
                 i<rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList->list.count)
               && (i < LTE_maxDRB); i++) {
            // why minus 1 in RRC code for drb_identity ?
            connection_reestablishment_ind.drb_id[i]   =
              rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList->list.array[i]->drb_Identity;
          }
        } else {
          connection_reestablishment_ind.num_drb      = 0;
        }

        if (rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->srb_ToAddModList != NULL) {
          connection_reestablishment_ind.num_srb      =
            rrcConnectionReconfiguration->criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->srb_ToAddModList->list.count +
            UE_rrc_inst[ctxt_pP->module_id].num_srb;
        } else {
          connection_reestablishment_ind.num_srb      += UE_rrc_inst[ctxt_pP->module_id].num_srb;
        }

        if (connection_reestablishment_ind.num_srb > 2) { // fixme: only 2 srbs can exist, adjust the value
          connection_reestablishment_ind.num_srb =2;
        }

        memcpy (&message_ral_p->ittiMsg, (void *) &connection_reestablishment_ind, sizeof(rrc_ral_connection_reestablishment_ind_t));
        //#warning "ue_mod_idP ? for instance ? => YES"
        LOG_I(RRC, "Sending RRC_RAL_CONNECTION_REESTABLISHMENT_IND to mRAL\n");
        itti_send_msg_to_task (TASK_RAL_UE, ctxt_pP->instance, message_ral_p);
      }
#endif
    } // c1 present
  } // critical extensions present
}

/* 36.331, 5.3.5.4      Reception of an RRCConnectionReconfiguration including the mobilityControlInfo by the UE (handover) */
//-----------------------------------------------------------------------------
void
rrc_ue_process_mobilityControlInfo(
  const protocol_ctxt_t *const       ctxt_pP,
  const uint8_t                      eNB_index,
  struct LTE_MobilityControlInfo *const mobilityControlInfo
)
//-----------------------------------------------------------------------------
{
  /*
  DRB_ToReleaseList_t*  drb2release_list;
  DRB_Identity_t *lcid;
   */
  LOG_I(RRC,"Note: This function needs some updates\n");

  if(UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].T310_active == 1) {
    UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].T310_active = 0;
  }

  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].T304_active = 1;
  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].T304_cnt = T304[mobilityControlInfo->t304];
  /*
  drb2release_list = CALLOC (1, sizeof (*drb2release_list));
  lcid= CALLOC (1, sizeof (DRB_Identity_t)); // long
  for (*lcid=0;*lcid<NB_RB_MAX;*lcid++)
  {
    ASN_SEQUENCE_ADD (&(drb2release_list)->list,lcid);
  }
   */
  //Removing SRB1 and SRB2 and DRB0
  LOG_I(RRC,"[UE %d] : Update needed for rrc_pdcp_config_req (deprecated) and rrc_rlc_config_req commands(deprecated)\n", ctxt_pP->module_id);
  rrc_pdcp_config_req (ctxt_pP, SRB_FLAG_YES, CONFIG_ACTION_REMOVE, DCCH,UNDEF_SECURITY_MODE);
  rrc_rlc_config_req(ctxt_pP, SRB_FLAG_YES, MBMS_FLAG_NO, CONFIG_ACTION_REMOVE,ctxt_pP->module_id+DCCH,Rlc_info_am_config);
  rrc_pdcp_config_req (ctxt_pP, SRB_FLAG_YES, CONFIG_ACTION_REMOVE, DCCH1,UNDEF_SECURITY_MODE);
  rrc_rlc_config_req(ctxt_pP, SRB_FLAG_YES,CONFIG_ACTION_REMOVE, MBMS_FLAG_NO,ctxt_pP->module_id+DCCH1,Rlc_info_am_config);
  rrc_pdcp_config_req (ctxt_pP, SRB_FLAG_NO, CONFIG_ACTION_REMOVE, DTCH,UNDEF_SECURITY_MODE);
  rrc_rlc_config_req(ctxt_pP, SRB_FLAG_NO,CONFIG_ACTION_REMOVE, MBMS_FLAG_NO,ctxt_pP->module_id+DTCH,Rlc_info_um);
  //Synchronisation to DL of target cell
  LOG_I(RRC,
        "HO: Reset PDCP and RLC for configured RBs.. \n[FRAME %05d][RRC_UE][MOD %02d][][--- MAC_CONFIG_REQ  (SRB2 eNB %d) --->][MAC_UE][MOD %02d][]\n",
        ctxt_pP->frame, ctxt_pP->module_id, eNB_index, ctxt_pP->module_id);
  // Reset MAC and configure PHY
  rrc_mac_config_req_ue(ctxt_pP->module_id,
                        0,
                        eNB_index,
                        (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                        (struct LTE_PhysicalConfigDedicated *)NULL,
                        (LTE_SCellToAddMod_r10_t *)NULL,
                        (LTE_MeasObjectToAddMod_t **)NULL,
                        (LTE_MAC_MainConfig_t *)NULL,
                        0,
                        (struct LTE_LogicalChannelConfig *)NULL,
                        (LTE_MeasGapConfig_t *)NULL,
                        (LTE_TDD_Config_t *)NULL,
                        mobilityControlInfo,
                        (uint8_t *)NULL,
                        (uint16_t *)NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        0,
                        (LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                        (LTE_PMCH_InfoList_r9_t *)NULL,
                        0,
                        NULL,
                        NULL,
                        0,
                        (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                        (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                       );
  // Re-establish PDCP for all RBs that are established
  // rrc_pdcp_config_req (ue_mod_idP+NB_eNB_INST, frameP, 0, CONFIG_ACTION_ADD, ue_mod_idP+DCCH);
  // rrc_pdcp_config_req (ue_mod_idP+NB_eNB_INST, frameP, 0, CONFIG_ACTION_ADD, ue_mod_idP+DCCH1);
  // rrc_pdcp_config_req (ue_mod_idP+NB_eNB_INST, frameP, 0, CONFIG_ACTION_ADD, ue_mod_idP+DTCH);
  // Re-establish RLC for all RBs that are established
  // rrc_rlc_config_req(ue_mod_idP+NB_eNB_INST,frameP,0,CONFIG_ACTION_ADD,ue_mod_idP+DCCH,SIGNALLING_RADIO_BEARER,Rlc_info_am_config);
  // rrc_rlc_config_req(ue_mod_idP+NB_eNB_INST,frameP,0,CONFIG_ACTION_ADD,ue_mod_idP+DCCH1,SIGNALLING_RADIO_BEARER,Rlc_info_am_config);
  // rrc_rlc_config_req(ue_mod_idP+NB_eNB_INST,frameP,0,CONFIG_ACTION_ADD,ue_mod_idP+DTCH,RADIO_ACCESS_BEARER,Rlc_info_um);
  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].State = RRC_SI_RECEIVED;
}

//-----------------------------------------------------------------------------
void
rrc_detach_from_eNB(
  module_id_t ue_mod_idP,
  uint8_t eNB_index
)
//-----------------------------------------------------------------------------
{
  //UE_rrc_inst[ue_mod_idP].DRB_config[eNB_index]
}

//-----------------------------------------------------------------------------
void
rrc_ue_decode_dcch(
  const protocol_ctxt_t *const ctxt_pP,
  const rb_id_t                Srb_id,
  const uint8_t         *const Buffer,
  const uint32_t               Buffer_size,
  const uint8_t                eNB_indexP
)
//-----------------------------------------------------------------------------
{
  //DL_DCCH_Message_t dldcchmsg;
  LTE_DL_DCCH_Message_t *dl_dcch_msg=NULL;//&dldcchmsg;
  //  asn_dec_rval_t dec_rval;
  // int i;
  uint8_t target_eNB_index=0xFF;
  MessageDef *msg_p;

  if (Srb_id != 1) {
    LOG_E(RRC,"[UE %d] Frame %d: Received message on DL-DCCH (SRB%ld), should not have ...\n",
          ctxt_pP->module_id, ctxt_pP->frame, Srb_id);
    return;
  }

  asn_dec_rval_t dec_rval = uper_decode(NULL,
              &asn_DEF_LTE_DL_DCCH_Message,
              (void **)&dl_dcch_msg,
              (uint8_t *)Buffer,
              Buffer_size, 0, 0);

  if (dec_rval.code != RC_OK && dec_rval.consumed == 0)
  {
    LOG_E(RRC, "%s: Failed to decode LTE_DL_DCC_Msg\n", __FUNCTION__);
    SEQUENCE_free(&asn_DEF_LTE_DL_DCCH_Message, dl_dcch_msg, ASFM_FREE_EVERYTHING);
    return;
  }

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout,&asn_DEF_LTE_DL_DCCH_Message,(void *)dl_dcch_msg);
  }

  if (dl_dcch_msg->message.present == LTE_DL_DCCH_MessageType_PR_c1) {
    if (UE_rrc_inst[ctxt_pP->module_id].Info[eNB_indexP].State >= RRC_CONNECTED) {
      switch (dl_dcch_msg->message.choice.c1.present) {
        case LTE_DL_DCCH_MessageType__c1_PR_NOTHING:
          LOG_I(RRC, "[UE %d] Frame %d : Received PR_NOTHING on DL-DCCH-Message\n",
                ctxt_pP->module_id, ctxt_pP->frame);
          return;

        case LTE_DL_DCCH_MessageType__c1_PR_csfbParametersResponseCDMA2000:
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_dlInformationTransfer: {
          LTE_DLInformationTransfer_t *dlInformationTransfer = &dl_dcch_msg->message.choice.c1.choice.dlInformationTransfer;

          if ((dlInformationTransfer->criticalExtensions.present == LTE_DLInformationTransfer__criticalExtensions_PR_c1)
              && (dlInformationTransfer->criticalExtensions.choice.c1.present
                  == LTE_DLInformationTransfer__criticalExtensions__c1_PR_dlInformationTransfer_r8)
              && (dlInformationTransfer->criticalExtensions.choice.c1.choice.dlInformationTransfer_r8.dedicatedInfoType.present
                  == LTE_DLInformationTransfer_r8_IEs__dedicatedInfoType_PR_dedicatedInfoNAS)) {
            /* This message hold a dedicated info NAS payload, forward it to NAS */
            struct LTE_DLInformationTransfer_r8_IEs__dedicatedInfoType *dedicatedInfoType =
                &dlInformationTransfer->criticalExtensions.choice.c1.choice.dlInformationTransfer_r8.dedicatedInfoType;
            uint32_t pdu_length;
            uint8_t *pdu_buffer;
            MessageDef *msg_p;
            pdu_length = dedicatedInfoType->choice.dedicatedInfoNAS.size;
            pdu_buffer = dedicatedInfoType->choice.dedicatedInfoNAS.buf;
            msg_p = itti_alloc_new_message(TASK_RRC_UE, 0, NAS_DOWNLINK_DATA_IND);
            NAS_DOWNLINK_DATA_IND(msg_p).UEid = ctxt_pP->module_id; // TODO set the UEid to something else ?
            NAS_DOWNLINK_DATA_IND(msg_p).nasMsg.length = pdu_length;
            NAS_DOWNLINK_DATA_IND(msg_p).nasMsg.data = pdu_buffer;
            itti_send_msg_to_task(TASK_NAS_UE, ctxt_pP->instance, msg_p);
          }

          break;
        }

        case LTE_DL_DCCH_MessageType__c1_PR_handoverFromEUTRAPreparationRequest:
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_mobilityFromEUTRACommand:
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_rrcConnectionReconfiguration:

          // first check if mobilityControlInfo  is present
          if (dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.mobilityControlInfo
              != NULL) {
            /* 36.331, 5.3.5.4 Reception of an RRCConnectionReconfiguration including the mobilityControlInfo by the UE (handover)*/
            if (UE_rrc_inst[ctxt_pP->module_id].HandoverInfoUe.targetCellId
                != dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.mobilityControlInfo->targetPhysCellId) {
              LOG_W(RRC,
                    "[UE %d] Frame %d: Handover target (%ld) is different from RSRP measured target (%ld)..\n",
                    ctxt_pP->module_id,
                    ctxt_pP->frame,
                    dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.mobilityControlInfo->targetPhysCellId,
                    UE_rrc_inst[ctxt_pP->module_id].HandoverInfoUe.targetCellId);
              return;
            } else if ((target_eNB_index = get_adjacent_cell_mod_id(UE_rrc_inst[ctxt_pP->module_id].HandoverInfoUe.targetCellId))
                       == 0xFF) {
              LOG_W(RRC,
                    "[UE %d] Frame %d: ue_mod_idP of the target eNB not found, check the network topology\n",
                    ctxt_pP->module_id,
                    ctxt_pP->frame);
              return;
            } else {
              LOG_I(RRC,
                    "[UE% d] Frame %d: Received rrcConnectionReconfiguration with mobilityControlInfo \n",
                    ctxt_pP->module_id,
                    ctxt_pP->frame);
              UE_rrc_inst[ctxt_pP->module_id].HandoverInfoUe.measFlag = 1; // Ready to send more MeasReports if required
            }
          }

          rrc_ue_process_rrcConnectionReconfiguration(
            ctxt_pP,
            &dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration,
            eNB_indexP);

          if (target_eNB_index != 0xFF) {
            rrc_ue_generate_RRCConnectionReconfigurationComplete(
              ctxt_pP,
              target_eNB_index,
              dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.rrc_TransactionIdentifier,
              NULL);
            UE_rrc_inst[ctxt_pP->module_id].Info[eNB_indexP].State = RRC_HO_EXECUTION;
            UE_rrc_inst[ctxt_pP->module_id].Info[target_eNB_index].State = RRC_RECONFIGURED;
            LOG_I(RRC, "[UE %d] State = RRC_RECONFIGURED during HO (eNB %d)\n",
                  ctxt_pP->module_id, target_eNB_index);
#if ENABLE_RAL
            {
              MessageDef                                 *message_ral_p = NULL;
              rrc_ral_connection_reconfiguration_ho_ind_t connection_reconfiguration_ho_ind;
              int                                         i;
              message_ral_p = itti_alloc_new_message (TASK_RRC_UE, 0, RRC_RAL_CONNECTION_RECONFIGURATION_HO_IND);
              memset(&connection_reconfiguration_ho_ind, 0, sizeof(rrc_ral_connection_reconfiguration_ho_ind_t));
              connection_reconfiguration_ho_ind.ue_id = ctxt_pP->module_id;

              if (dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList
                  != NULL) {
                connection_reconfiguration_ho_ind.num_drb      =
                  dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList->list.count;

                for (i=0; (
                       i<dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList->list.count)
                     && (i < LTE_maxDRB); i++) {
                  // why minus 1 in RRC code for drb_identity ?
                  connection_reconfiguration_ho_ind.drb_id[i]   =
                    dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList->list.array[i]->drb_Identity;
                }
              } else {
                connection_reconfiguration_ho_ind.num_drb      = 0;
              }

              if (dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->srb_ToAddModList
                  != NULL) {
                connection_reconfiguration_ho_ind.num_srb      =
                  dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->srb_ToAddModList->list.count
                  +
                  UE_rrc_inst[ctxt_pP->module_id].num_srb;
              } else {
                connection_reconfiguration_ho_ind.num_srb      += UE_rrc_inst[ctxt_pP->module_id].num_srb;
              }

              if (connection_reconfiguration_ho_ind.num_srb > 2 ) {
                connection_reconfiguration_ho_ind.num_srb =2;
              }

              memcpy (&message_ral_p->ittiMsg, (void *) &connection_reconfiguration_ho_ind, sizeof(rrc_ral_connection_reconfiguration_ho_ind_t));
              //#warning "ue_mod_idP ? for instance ? => YES"
              LOG_I(RRC, "Sending RRC_RAL_CONNECTION_RECONFIGURATION_HO_IND to mRAL\n");
              itti_send_msg_to_task (TASK_RAL_UE, ctxt_pP->instance, message_ral_p);
            }
#endif
          } else {
            rrc_ue_generate_RRCConnectionReconfigurationComplete(
              ctxt_pP,
              eNB_indexP,
              dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.rrc_TransactionIdentifier,
              NULL);
            UE_rrc_inst[ctxt_pP->module_id].Info[eNB_indexP].State = RRC_RECONFIGURED;
            LOG_I(RRC, "[UE %d] State = RRC_RECONFIGURED (eNB %d)\n",
                  ctxt_pP->module_id,
                  eNB_indexP);
#if ENABLE_RAL
            {
              MessageDef                                 *message_ral_p = NULL;
              rrc_ral_connection_reconfiguration_ind_t    connection_reconfiguration_ind;
              int                                         i;
              message_ral_p = itti_alloc_new_message (TASK_RRC_UE, 0, RRC_RAL_CONNECTION_RECONFIGURATION_IND);
              memset(&connection_reconfiguration_ind, 0, sizeof(rrc_ral_connection_reconfiguration_ind_t));
              connection_reconfiguration_ind.ue_id = ctxt_pP->module_id;

              if (dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList
                  != NULL) {
                connection_reconfiguration_ind.num_drb      =
                  dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList->list.count;

                for (i=0; (
                       i<dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList->list.count)
                     && (i < LTE_maxDRB); i++) {
                  // why minus 1 in RRC code for drb_identity ?
                  connection_reconfiguration_ind.drb_id[i]   =
                    dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->drb_ToAddModList->list.array[i]->drb_Identity;
                }
              } else {
                connection_reconfiguration_ind.num_drb      = 0;
              }

              if (dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->srb_ToAddModList
                  != NULL) {
                connection_reconfiguration_ind.num_srb      =
                  dl_dcch_msg->message.choice.c1.choice.rrcConnectionReconfiguration.criticalExtensions.choice.c1.choice.rrcConnectionReconfiguration_r8.radioResourceConfigDedicated->srb_ToAddModList->list.count
                  +
                  UE_rrc_inst[ctxt_pP->module_id].num_srb;
              } else {
                connection_reconfiguration_ind.num_srb      +=UE_rrc_inst[ctxt_pP->module_id].num_srb;
              }

              if (connection_reconfiguration_ind.num_srb > 2 ) {
                connection_reconfiguration_ind.num_srb =2;
              }

              memcpy (&message_ral_p->ittiMsg, (void *) &connection_reconfiguration_ind, sizeof(rrc_ral_connection_reconfiguration_ind_t));
              //#warning "ue_mod_idP ? for instance ? => YES"
              LOG_I(RRC, "Sending RRC_RAL_CONNECTION_RECONFIGURATION_IND to mRAL\n");
              itti_send_msg_to_task (TASK_RAL_UE, ctxt_pP->instance, message_ral_p);
            }
#endif
          }

          //TTN test D2D (should not be here - in reality, this message will be triggered from ProSeApp)
          if (send_ue_information == 0) {
            LOG_I(RRC, "TEST SidelinkUEInformation [UE %d] Received  (eNB %d)\n",
                  ctxt_pP->module_id, eNB_indexP);
            LTE_SL_DestinationInfoList_r12_t *destinationInfoList = CALLOC(1, sizeof(LTE_SL_DestinationInfoList_r12_t));
            LTE_SL_DestinationIdentity_r12_t *sl_destination_identity = CALLOC(1, sizeof(LTE_SL_DestinationIdentity_r12_t));
            sl_destination_identity->size = 3;
            sl_destination_identity->buf = CALLOC(1,3);
            sl_destination_identity->buf[0] = 0x00;
            sl_destination_identity->buf[1] = 0x00;
            sl_destination_identity->buf[2] = 0x01;
            sl_destination_identity->bits_unused = 0;
            ASN_SEQUENCE_ADD(&destinationInfoList->list,sl_destination_identity);
            rrc_ue_generate_SidelinkUEInformation(ctxt_pP, eNB_indexP, destinationInfoList, NULL, SL_TRANSMIT_NON_RELAY_ONE_TO_ONE);
            send_ue_information ++;
          }

          UE_RRC_INFO *info = &UE_rrc_inst[ctxt_pP->module_id].Info[eNB_indexP];
          if (info->dl_dcch_msg != NULL) {
              SEQUENCE_free(&asn_DEF_LTE_DL_DCCH_Message, info->dl_dcch_msg, ASFM_FREE_EVERYTHING);
          }
          info->dl_dcch_msg = dl_dcch_msg;
          dl_dcch_msg = NULL;
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_rrcConnectionRelease:
          msg_p = itti_alloc_new_message(TASK_RRC_UE, 0, NAS_CONN_RELEASE_IND);

          if ((dl_dcch_msg->message.choice.c1.choice.rrcConnectionRelease.criticalExtensions.present
               == LTE_RRCConnectionRelease__criticalExtensions_PR_c1)
              && (dl_dcch_msg->message.choice.c1.choice.rrcConnectionRelease.criticalExtensions.choice.c1.present
                  == LTE_RRCConnectionRelease__criticalExtensions__c1_PR_rrcConnectionRelease_r8)) {
            NAS_CONN_RELEASE_IND(msg_p).cause =
              dl_dcch_msg->message.choice.c1.choice.rrcConnectionRelease.criticalExtensions.choice.c1.choice.rrcConnectionRelease_r8.releaseCause;
          }

          itti_send_msg_to_task(TASK_NAS_UE, ctxt_pP->instance, msg_p);
#if ENABLE_RAL
          msg_p = itti_alloc_new_message(TASK_RRC_UE, 0, RRC_RAL_CONNECTION_RELEASE_IND);
          RRC_RAL_CONNECTION_RELEASE_IND(msg_p).ue_id = ctxt_pP->module_id;
          itti_send_msg_to_task(TASK_RAL_UE, ctxt_pP->instance, msg_p);
#endif
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_securityModeCommand:
          LOG_I(RRC, "[UE %d] Received securityModeCommand (eNB %d)\n",
                ctxt_pP->module_id, eNB_indexP);
          rrc_ue_process_securityModeCommand(
            ctxt_pP,
            &dl_dcch_msg->message.choice.c1.choice.securityModeCommand,
            eNB_indexP);
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry:
          LOG_I(RRC, "[UE %d] Received Capability Enquiry (eNB %d)\n",
                ctxt_pP->module_id,
                eNB_indexP);

          LTE_UE_CapabilityRequest_t *ue_cap = &dl_dcch_msg->message.choice.c1.choice.ueCapabilityEnquiry.criticalExtensions.
                                                choice.c1.choice.ueCapabilityEnquiry_r8.ue_CapabilityRequest;
          bool have_received_nrue_cap = false;
          for (int i = 0; i < ue_cap->list.count; i++) {
            if (*ue_cap->list.array[i] == LTE_RAT_Type_nr || *ue_cap->list.array[i] == LTE_RAT_Type_eutra_nr) {
                have_received_nrue_cap = true;
                break;
            }
          }
          if (have_received_nrue_cap) {
                LTE_UECapabilityEnquiry_t *ueCapabilityEnquiry_nsa = &dl_dcch_msg->message.choice.c1.choice.ueCapabilityEnquiry;
                OCTET_STRING_t * requestedFreqBandsNR = ueCapabilityEnquiry_nsa->
                                      criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8.nonCriticalExtension->
                                      nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->
                                      nonCriticalExtension->requestedFreqBandsNR_MRDC_r15;
                nsa_sendmsg_to_nrue(requestedFreqBandsNR->buf, requestedFreqBandsNR->size, NRUE_CAPABILITY_ENQUIRY);
                LOG_A(RRC, "Second ueCapabilityEnquiry (request for NR capabilities) sent to NR UE with size %zu\n",
                      requestedFreqBandsNR->size);
                // Save ueCapabilityEnquiry so we can use in nsa mode after nrUE response is received
                UE_RRC_INFO *info = &UE_rrc_inst[ctxt_pP->module_id].Info[eNB_indexP];
                if (info->dl_dcch_msg != NULL) {
                  info->dl_dcch_msg = NULL;
                  //SEQUENCE_free(&asn_DEF_LTE_DL_DCCH_Message, info->dl_dcch_msg, ASFM_FREE_EVERYTHING); Melissa
                }
                info->dl_dcch_msg = dl_dcch_msg;
                dl_dcch_msg = NULL;
          } else if (get_softmodem_params()->nsa && !have_received_nrue_cap) {
              LTE_UECapabilityEnquiry_t *ueCapabilityEnquiry_nsa = &dl_dcch_msg->message.choice.c1.choice.ueCapabilityEnquiry;
              OCTET_STRING_t * requestedFreqBandsNR = ueCapabilityEnquiry_nsa->
                                    criticalExtensions.choice.c1.choice.ueCapabilityEnquiry_r8.nonCriticalExtension->
                                    nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->
                                    nonCriticalExtension->requestedFreqBandsNR_MRDC_r15;
              nsa_sendmsg_to_nrue(requestedFreqBandsNR->buf, requestedFreqBandsNR->size, UE_CAPABILITY_ENQUIRY);
              LOG_A(RRC, "Initial ueCapabilityEnquiry sent to NR UE with size %zu\n", requestedFreqBandsNR->size);
              // Save ueCapabilityEnquiry so we can use in nsa mode after nrUE response is received
              UE_RRC_INFO *info = &UE_rrc_inst[ctxt_pP->module_id].Info[eNB_indexP];
              if (info->dl_dcch_msg != NULL) {
                info->dl_dcch_msg = NULL;
                //SEQUENCE_free(&asn_DEF_LTE_DL_DCCH_Message, info->dl_dcch_msg, ASFM_FREE_EVERYTHING); Melissa
              }
              info->dl_dcch_msg = dl_dcch_msg;
              dl_dcch_msg = NULL;
          } else {
            rrc_ue_process_ueCapabilityEnquiry(
              ctxt_pP,
              &dl_dcch_msg->message.choice.c1.choice.ueCapabilityEnquiry,
              eNB_indexP);
          }
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_counterCheck:
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_ueInformationRequest_r9:
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_loggedMeasurementConfiguration_r10:
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_rnReconfiguration_r10:
          break;

        case LTE_DL_DCCH_MessageType__c1_PR_spare1:
        case LTE_DL_DCCH_MessageType__c1_PR_spare2:
        case LTE_DL_DCCH_MessageType__c1_PR_spare3:
          break;

        default:
          break;
      }
    }
  }

#ifndef NO_RRM
  send_msg(&S_rrc,msg_rrc_end_scan_req(ctxt_pP->module_id,eNB_indexP));
#endif
  if (0) //Melissa hack: Were not done with underlying members of dl_dcch_msg (Use after free)
  {
    SEQUENCE_free(&asn_DEF_LTE_DL_DCCH_Message, dl_dcch_msg, ASFM_FREE_EVERYTHING);
  }
}

const char siWindowLength[9][5] = {"1ms","2ms","5ms","10ms","15ms","20ms","40ms","80ms","ERR"};
const char siWindowLength_int[8] = {1,2,5,10,15,20,40,80};

const char SIBType[12][6] = {"SIB3","SIB4","SIB5","SIB6","SIB7","SIB8","SIB9","SIB10","SIB11","SIB12","SIB13","Spare"};
const char SIBPeriod[8][6]= {"rf8","rf16","rf32","rf64","rf128","rf256","rf512","ERR"};
int siPeriod_int[7] = {80,160,320,640,1280,2560,5120};

const char *SIBreserved( long value ) {
  if (value < 0 || value > 1)
    return "ERR";

  if (value)
    return "notReserved";

  return "reserved";
}
const char *SIBbarred( long value ) {
  if (value < 0 || value > 1)
    return "ERR";

  if (value)
    return "notBarred";

  return "barred";
}
const char *SIBallowed( long value ) {
  if (value < 0 || value > 1)
    return "ERR";

  if (value)
    return "notAllowed";

  return "allowed";
}
const char *SIB2SoundingPresent( int value ) {
  switch (value) {
    case LTE_SoundingRS_UL_ConfigCommon_PR_NOTHING:
      return "NOTHING";

    case LTE_SoundingRS_UL_ConfigCommon_PR_release:
      return "release";

    case LTE_SoundingRS_UL_ConfigCommon_PR_setup:
      return "setup";
  }

  return "ERR";
}
const char *SIB2numberOfRA_Preambles( long value ) {
  static char temp[4] = {0};

  if (value < 0 || value > 15)
    return "ERR";

  snprintf( temp, sizeof(temp), "n%ld", value*4 + 4 );
  temp[3] = 0; // terminate string
  return temp;
}
const char *SIB2powerRampingStep( long value ) {
  if (value < 0 || value > 3)
    return "ERR";

  static const char str[4][4] = {"dB0","dB2","dB4","dB6"};
  return str[value];
}
const char *SIB2preambleInitialReceivedTargetPower( long value ) {
  static char temp[8] = {0};

  if (value < 0 || value > 15)
    return "ERR";

  snprintf( temp, sizeof(temp), "dBm-%ld", 120 - value*2 );
  temp[7] = 0; // terminate string
  return temp;
}
const char *SIB2preambleTransMax( long value ) {
  static char temp[5] = {0};

  if (value < 0 || value > 10)
    return "ERR";

  if (value <= 5) {
    snprintf( temp, sizeof(temp), "n%ld", value+3 );
    return temp;
  }

  switch (value) {
    case 6:
      return "n10";

    case 7:
      return "n20";

    case 8:
      return "n50";

    case 9:
      return "n100";

    case 10:
      return "n200";
  }

  /* unreachable but gcc warns... */
  return "ERR";
}
const char *SIB2ra_ResponseWindowSize( long value ) {
  static char temp[4] = {0};

  if (value < 0 || value > 7)
    return "ERR";

  if (value == 7)
    return "sf10";

  snprintf( temp, sizeof(temp), "sf%ld", value+2 );
  return temp;
}
const char *SIB2mac_ContentionResolutionTimer( long value ) {
  static char temp[5] = {0};

  if (value < 0 || value > 7)
    return "ERR";

  snprintf( temp, sizeof(temp), "sf%ld", 8 + value*8 );
  return temp;
}
const char *SIB2modificationPeriodCoeff( long value ) {
  static char temp[32] = {0};

  if (value < 0 || value > 3)
    return "ERR";

  snprintf( temp, sizeof(temp), "n%d", (int)pow(2,value+1) );
  return temp;
}
const char *SIB2defaultPagingCycle( long value ) {
  static char temp[32] = {0};

  if (value < 0 || value > 3)
    return "ERR";

  snprintf( temp, sizeof(temp), "rf%d", (int)pow(2,value+4) );
  return temp;
}
const char *SIB2nB( long value ) {
  if (value < 0 || value > 7)
    return "ERR";

  static const char str[8][17] = {"fourT","twoT","oneT","halfT","quarterT","oneEigthT","oneSixteenthT","oneThirtySecondT"};
  return str[value];
}

int decode_BCCH_MBMS_DLSCH_Message(
  const protocol_ctxt_t *const ctxt_pP,
  const uint8_t                eNB_index,
  uint8_t               *const Sdu,
  const uint8_t                Sdu_len,
  const uint8_t                rsrq,
  const uint8_t                rsrp ) {
  LTE_BCCH_DL_SCH_Message_MBMS_t *bcch_message = NULL;
  //LTE_SystemInformationBlockType1_MBMS_r14_t *sib1_mbms = UE_rrc_inst[ctxt_pP->module_id].sib1_MBMS[eNB_index];
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_IN );
  /*if (((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus_MBMS&1) == 1) &&  // SIB1 received
      (UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIcnt_MBMS == sib1->schedulingInfoList_MBMS_r14.list.count)) {
    // Avoid decoding  SystemInformationBlockType1_t* sib1_MBMS = UE_rrc_inst[ctxt_pP->module_id].sib1_MBMS[eNB_index];
    // to prevent memory bloating
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_OUT );
    return 0;
  }*/
  rrc_set_sub_state( ctxt_pP->module_id, RRC_SUB_STATE_IDLE_RECEIVING_SIB );
  //if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
  //xer_fprint(stdout, &asn_DEF_LTE_BCCH_DL_SCH_Message_MBMS,(void *)bcch_message );
  //}
  asn_dec_rval_t dec_rval = uper_decode_complete( NULL,
                            &asn_DEF_LTE_BCCH_DL_SCH_Message_MBMS,
                            (void **)&bcch_message,
                            (const void *)Sdu,
                            Sdu_len );

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    LOG_E( RRC, "[UE %"PRIu8"] Failed to decode BCCH_DLSCH_MBMS_MESSAGE (%zu bits)\n",
           ctxt_pP->module_id,
           dec_rval.consumed );
    log_dump(RRC, Sdu, Sdu_len, LOG_DUMP_CHAR,"   Received bytes:\n" );
    // free the memory
    SEQUENCE_free( &asn_DEF_LTE_BCCH_DL_SCH_Message_MBMS, (void *)bcch_message, 1 );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_OUT );
    return -1;
  }

  //if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
  //xer_fprint(stdout, &asn_DEF_LTE_BCCH_DL_SCH_Message_MBMS,(void *)bcch_message );
  //}

  if (bcch_message->message.present == LTE_BCCH_DL_SCH_MessageType_MBMS_r14_PR_c1) {
    switch (bcch_message->message.choice.c1.present) {
      case LTE_BCCH_DL_SCH_MessageType_MBMS_r14__c1_PR_systemInformationBlockType1_MBMS_r14:
        if ((ctxt_pP->frame % 4) == 0) {
          // every 4 frame
          if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus_MBMS&1) == 0) {
            LTE_SystemInformationBlockType1_MBMS_r14_t *sib1_mbms = UE_rrc_inst[ctxt_pP->module_id].sib1_MBMS[eNB_index];
            memcpy( (void *)sib1_mbms,
                    (void *)&bcch_message->message.choice.c1.choice.systemInformationBlockType1_MBMS_r14,
                    sizeof(LTE_SystemInformationBlockType1_MBMS_r14_t) );
            LOG_D( RRC, "[UE %"PRIu8"] Decoding \"First\" SIB1-MBMS\n", ctxt_pP->module_id );
            decode_SIB1_MBMS( ctxt_pP, eNB_index, rsrq, rsrp );
          }
        }

        break;

      case LTE_BCCH_DL_SCH_MessageType_MBMS_r14__c1_PR_systemInformation_MBMS_r14:
        if ((ctxt_pP->frame % 4) == 0) {
          //if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&1) == 1) {
          // SIB1 with schedulingInfoList is available
          // you miss this on the eNB!!!!
          LTE_SystemInformation_MBMS_r14_t *si_mbms = UE_rrc_inst[ctxt_pP->module_id].si_MBMS[eNB_index];
          memcpy( si_mbms,
                  &bcch_message->message.choice.c1.choice.systemInformation_MBMS_r14,
                  sizeof(LTE_SystemInformation_MBMS_r14_t) );
          LOG_W( RRC, "[UE %"PRIu8"] Decoding MBMS SI for frameP %"PRIu32"\n",
                 ctxt_pP->module_id,
                 ctxt_pP->frame );
          decode_SI_MBMS( ctxt_pP, eNB_index ); //TODO
          //UE_mac_inst[ctxt_pP->module_id].SI_Decoded = 1;
          //}
        }

        break;

      case LTE_BCCH_DL_SCH_MessageType__c1_PR_NOTHING:
      default:
        break;
    }
  }

  /*if (bcch_message->message.present == LTE_BCCH_DL_SCH_MessageType_PR_c1) {
    switch (bcch_message->message.choice.c1.present) {
      case LTE_BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1:
        if ((ctxt_pP->frame % 2) == 0) {
          // even frame
          if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&1) == 0) {
            LTE_SystemInformationBlockType1_t *sib1 = UE_rrc_inst[ctxt_pP->module_id].sib1[eNB_index];
            memcpy( (void *)sib1,
                    (void *)&bcch_message->message.choice.c1.choice.systemInformationBlockType1,
                    sizeof(LTE_SystemInformationBlockType1_t) );
            LOG_D( RRC, "[UE %"PRIu8"] Decoding First SIB1\n", ctxt_pP->module_id );
            decode_SIB1( ctxt_pP, eNB_index, rsrq, rsrp );
          }
        }

        break;

      case LTE_BCCH_DL_SCH_MessageType__c1_PR_systemInformation:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&1) == 1) {
          // SIB1 with schedulingInfoList is available
          LTE_SystemInformation_t *si = UE_rrc_inst[ctxt_pP->module_id].si[eNB_index];
          memcpy( si,
                  &bcch_message->message.choice.c1.choice.systemInformation,
                  sizeof(LTE_SystemInformation_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Decoding SI for frameP %"PRIu32"\n",
                 ctxt_pP->module_id,
                 ctxt_pP->frame );
          decode_SI( ctxt_pP, eNB_index );
          //if (nfapi_mode == 3)
          UE_mac_inst[ctxt_pP->module_id].SI_Decoded = 1;
        }

        break;

      case LTE_BCCH_DL_SCH_MessageType__c1_PR_NOTHING:
      default:
        break;
    }
  }*/
  /*if ((rrc_get_sub_state(ctxt_pP->module_id) == RRC_SUB_STATE_IDLE_SIB_COMPLETE)
  #if defined(ENABLE_USE_MME)
      && (UE_rrc_inst[ctxt_pP->module_id].initialNasMsg.data != NULL)
  #endif
     ) {
    rrc_ue_generate_RRCConnectionRequest(ctxt_pP, 0);
    rrc_set_sub_state( ctxt_pP->module_id, RRC_SUB_STATE_IDLE_CONNECTING );
  }*/
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_OUT );
  return 0;
}



//-----------------------------------------------------------------------------
int decode_BCCH_DLSCH_Message(
  const protocol_ctxt_t *const ctxt_pP,
  const uint8_t                eNB_index,
  uint8_t               *const Sdu,
  const uint8_t                Sdu_len,
  const uint8_t                rsrq,
  const uint8_t                rsrp ) {
  LTE_BCCH_DL_SCH_Message_t *bcch_message = NULL;
  LTE_SystemInformationBlockType1_t *sib1 = UE_rrc_inst[ctxt_pP->module_id].sib1[eNB_index];
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_IN );

  if (((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&1) == 1) &&  // SIB1 received
      (UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIcnt == sib1->schedulingInfoList.list.count)) {
    // Avoid decoding  SystemInformationBlockType1_t* sib1 = UE_rrc_inst[ctxt_pP->module_id].sib1[eNB_index];
    // to prevent memory bloating
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_OUT );
    return 0;
  }

  rrc_set_sub_state( ctxt_pP->module_id, RRC_SUB_STATE_IDLE_RECEIVING_SIB );

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_LTE_BCCH_DL_SCH_Message,(void *)bcch_message );
  }

  asn_dec_rval_t dec_rval = uper_decode_complete( NULL,
                            &asn_DEF_LTE_BCCH_DL_SCH_Message,
                            (void **)&bcch_message,
                            (const void *)Sdu,
                            Sdu_len );

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    LOG_E( RRC, "[UE %"PRIu8"] Failed to decode BCCH_DLSCH_MESSAGE (%zu bits)\n",
           ctxt_pP->module_id,
           dec_rval.consumed );
    log_dump(RRC, Sdu, Sdu_len, LOG_DUMP_CHAR,"   Received bytes:\n" );
    // free the memory
    SEQUENCE_free( &asn_DEF_LTE_BCCH_DL_SCH_Message, (void *)bcch_message, 1 );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_OUT );
    return -1;
  }

  if (bcch_message->message.present == LTE_BCCH_DL_SCH_MessageType_PR_c1) {
    switch (bcch_message->message.choice.c1.present) {
      case LTE_BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1:
        if ((ctxt_pP->frame % 2) == 0) {
          // even frame
          if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&1) == 0) {
            LTE_SystemInformationBlockType1_t *sib1 = UE_rrc_inst[ctxt_pP->module_id].sib1[eNB_index];
            memcpy( (void *)sib1,
                    (void *)&bcch_message->message.choice.c1.choice.systemInformationBlockType1,
                    sizeof(LTE_SystemInformationBlockType1_t) );
            LOG_D( RRC, "[UE %"PRIu8"] Decoding First SIB1\n", ctxt_pP->module_id );
            decode_SIB1( ctxt_pP, eNB_index, rsrq, rsrp );
          }
        }

        break;

      case LTE_BCCH_DL_SCH_MessageType__c1_PR_systemInformation:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&1) == 1) {
          // SIB1 with schedulingInfoList is available
          LTE_SystemInformation_t *si = UE_rrc_inst[ctxt_pP->module_id].si[eNB_index];
          memcpy( si,
                  &bcch_message->message.choice.c1.choice.systemInformation,
                  sizeof(LTE_SystemInformation_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Decoding SI for frameP %"PRIu32"\n",
                 ctxt_pP->module_id,
                 ctxt_pP->frame );
          decode_SI( ctxt_pP, eNB_index );
          //if (nfapi_mode == 3)
          UE_mac_inst[ctxt_pP->module_id].SI_Decoded = 1;
        }

        break;

      case LTE_BCCH_DL_SCH_MessageType__c1_PR_NOTHING:
      default:
        break;
    }
  }

  if (rrc_get_sub_state(ctxt_pP->module_id) == RRC_SUB_STATE_IDLE_SIB_COMPLETE) {
    if ( (UE_rrc_inst[ctxt_pP->module_id].initialNasMsg.data != NULL) || (!EPC_MODE_ENABLED)) {
      rrc_ue_generate_RRCConnectionRequest(ctxt_pP, 0);
      rrc_set_sub_state( ctxt_pP->module_id, RRC_SUB_STATE_IDLE_CONNECTING );
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_BCCH, VCD_FUNCTION_OUT );
  return 0;
}

//-----------------------------------------------------------------------------
int decode_PCCH_DLSCH_Message(
  const protocol_ctxt_t *const ctxt_pP,
  const uint8_t                eNB_index,
  uint8_t               *const Sdu,
  const uint8_t                Sdu_len) {
  LTE_PCCH_Message_t *pcch_message = NULL;
  int i;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_PCCH, VCD_FUNCTION_IN );
  asn_dec_rval_t dec_rval = uper_decode_complete( NULL,
                            &asn_DEF_LTE_PCCH_Message,
                            (void **)&pcch_message,
                            (const void *)Sdu,
                            Sdu_len );

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    LOG_E( RRC, "[UE %"PRIu8"] Failed to decode PCCH_MESSAGE (%zu bits)\n",
           ctxt_pP->module_id,
           dec_rval.consumed );

    for (i=0; i<Sdu_len; i++)
      printf("%02x ",Sdu[i]);

    printf("\n");
    // free the memory
    SEQUENCE_free( &asn_DEF_LTE_PCCH_Message, (void *)pcch_message, 1 );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_PCCH, VCD_FUNCTION_OUT );
    return -1;
  }

  return(0);
}

int decode_SIB1_MBMS( const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index, const uint8_t rsrq, const uint8_t rsrp ) {
  LTE_SystemInformationBlockType1_MBMS_r14_t *sib1_MBMS = UE_rrc_inst[ctxt_pP->module_id].sib1_MBMS[eNB_index];
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_UE_DECODE_SIB1, VCD_FUNCTION_IN );
  LOG_I( RRC, "[UE %d] : Dumping SIB 1 SIB1-MBMS\n", ctxt_pP->module_id );
  //cannot parse ! ... TODO
  /*LTE_PLMN_Identity_t *PLMN_identity = ((LTE_PLMN_Identity_t*)(&sib1_MBMS->cellAccessRelatedInfo_r14.plmn_IdentityList_r14.list.array[0]))->plmn_Identity;
  int mccdigits = PLMN_identity->mcc->list.count;
  int mncdigits = PLMN_identity->mnc.list.count;
  int mcc;

  if (mccdigits == 2) {
    mcc = *PLMN_identity->mcc->list.array[0]*10 + *PLMN_identity->mcc->list.array[1];
  } else {
    mcc = *PLMN_identity->mcc->list.array[0]*100 + *PLMN_identity->mcc->list.array[1]*10 + *PLMN_identity->mcc->list.array[2];
  }

  int mnc;

  if (mncdigits == 2) {
    mnc = *PLMN_identity->mnc.list.array[0]*10 + *PLMN_identity->mnc.list.array[1];
  } else {
    mnc = *PLMN_identity->mnc.list.array[0]*100 + *PLMN_identity->mnc.list.array[1]*10 + *PLMN_identity->mnc.list.array[2];
  }

  LOG_I( RRC, "PLMN MCC %0*d, MNC %0*d, TAC 0x%04x\n", mccdigits, mcc, mncdigits, mnc,
         ((sib1_MBMS->cellAccessRelatedInfo_r14.trackingAreaCode_r14.size == 2)?((sib1_MBMS->cellAccessRelatedInfo_r14.trackingAreaCode_r14.buf[0]<<8) + sib1_MBMS->cellAccessRelatedInfo_r14.trackingAreaCode_r14.buf[1]):0));
  LOG_I( RRC, "cellReservedForOperatorUse                 : raw:%ld decoded:%s\n", ((LTE_PLMN_IndentityInfo_t*)sib1_MBMS->cellAccessRelatedInfo_r14.plmn_IdentityList_r14.list.array[0])->cellReservedForOperatorUse,
         SIBreserved(((LTE_PLMN_IdentityInfo_t*)sib1_MBMS->cellAccessRelatedInfo_r14.plmn_IdentityList_r14.list.array[0])->cellReservedForOperatorUse) );
  // search internal table for provider name
  int plmn_ind = 0;

  while (plmn_data[plmn_ind].mcc > 0) {
    if ((plmn_data[plmn_ind].mcc == mcc) && (plmn_data[plmn_ind].mnc == mnc)) {
      LOG_I( RRC, "Found %s (name from internal table)\n", plmn_data[plmn_ind].oper_short );
      break;
    }

    plmn_ind++;
  }

  if (plmn_data[plmn_ind].mcc < 0) {
    LOG_I( RRC, "Found Unknown operator (no entry in internal table)\n" );
  }
  */
  LOG_I( RRC, "cellAccessRelatedInfo.cellIdentity         : raw:%"PRIu32" decoded:%02x.%02x.%02x.%02x\n",
         BIT_STRING_to_uint32( &sib1_MBMS->cellAccessRelatedInfo_r14.cellIdentity_r14 ),
         sib1_MBMS->cellAccessRelatedInfo_r14.cellIdentity_r14.buf[0],
         sib1_MBMS->cellAccessRelatedInfo_r14.cellIdentity_r14.buf[1],
         sib1_MBMS->cellAccessRelatedInfo_r14.cellIdentity_r14.buf[2],
         sib1_MBMS->cellAccessRelatedInfo_r14.cellIdentity_r14.buf[3] >> sib1_MBMS->cellAccessRelatedInfo_r14.cellIdentity_r14.bits_unused);
  //LOG_I( RRC, "cellAccessRelatedInfo.cellBarred           : raw:%ld decoded:%s\n", sib1_MBMS->cellAccessRelatedInfo_r14.cellBarred, SIBbarred(sib1_MBMS->cellAccessRelatedInfo_r14.cellBarred) );
  //LOG_I( RRC, "cellAccessRelatedInfo.intraFreqReselection : raw:%ld decoded:%s\n", sib1->cellAccessRelatedInfo.intraFreqReselection, SIBallowed(sib1->cellAccessRelatedInfo.intraFreqReselection) );
  //LOG_I( RRC, "cellAccessRelatedInfo.csg_Indication       : %d\n", sib1->cellAccessRelatedInfo.csg_Indication );
  //if (sib1->cellAccessRelatedInfo.csg_Identity)
  //LOG_I( RRC, "cellAccessRelatedInfo.csg_Identity         : %"PRIu32"\n", BIT_STRING_to_uint32(sib1->cellAccessRelatedInfo.csg_Identity) );
  //else
  //LOG_I( RRC, "cellAccessRelatedInfo.csg_Identity         : not defined\n" );
  //
  //LOG_I( RRC, "cellSelectionInfo.q_RxLevMin               : %ld\n", sib1->cellSelectionInfo.q_RxLevMin );
  //if (sib1->cellSelectionInfo.q_RxLevMinOffset)
  //LOG_I( RRC, "cellSelectionInfo.q_RxLevMinOffset         : %ld\n", *sib1->cellSelectionInfo.q_RxLevMinOffset );
  //else
  //LOG_I( RRC, "cellSelectionInfo.q_RxLevMinOffset         : not defined\n" );
  //if (sib1->p_Max)
  //LOG_I( RRC, "p_Max                                      : %ld\n", *sib1->p_Max );
  //else
  //LOG_I( RRC, "p_Max                                      : not defined\n" );
  LOG_I( RRC, "freqBandIndicator                          : %ld\n", sib1_MBMS->freqBandIndicator_r14 );

  if (sib1_MBMS->schedulingInfoList_MBMS_r14.list.count > 0) {
    for (int i=0; i<sib1_MBMS->schedulingInfoList_MBMS_r14.list.count; i++) {
      LOG_I( RRC, "si_Periodicity[%d]                          : %s\n", i, SIBPeriod[min(sib1_MBMS->schedulingInfoList_MBMS_r14.list.array[i]->si_Periodicity_r14,7)]);

      if (sib1_MBMS->schedulingInfoList_MBMS_r14.list.array[i]->sib_MappingInfo_r14.list.count > 0) {
        char temp[32 * sizeof(SIBType[0])] = {0}; // maxSIB==32

        for (int j=0; j<sib1_MBMS->schedulingInfoList_MBMS_r14.list.array[i]->sib_MappingInfo_r14.list.count; j++) {
          sprintf( temp + j*sizeof(SIBType[0]), "%*s ", (int)sizeof(SIBType[0])-1, SIBType[min(*sib1_MBMS->schedulingInfoList_MBMS_r14.list.array[i]->sib_MappingInfo_r14.list.array[0],11)] );
        }

        LOG_I( RRC, "siSchedulingInfoSIBType[%d]                 : %s\n", i, temp );
      } else {
        LOG_I( RRC, "mapping list %d is null\n", i );
      }
    }
  } else {
    LOG_E( RRC, "siSchedulingInfoPeriod[0]                  : PROBLEM!!!\n" );
    return -1;
  }

  //if (sib1_MBMS->tdd_Config) {
  //LOG_I( RRC, "TDD subframeAssignment                     : %ld\n", sib1_MBMS->tdd_Config->subframeAssignment );
  //LOG_I( RRC, "TDD specialSubframePatterns                : %ld\n", sib1_MBMS->tdd_Config->specialSubframePatterns );
  //}
  LOG_I( RRC, "siWindowLength                             : %s\n", siWindowLength[min(sib1_MBMS->si_WindowLength_r14,8)] );
  LOG_I( RRC, "systemInfoValueTag                         : %ld\n", sib1_MBMS->systemInfoValueTag_r14 );
  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIperiod_MBMS     = siPeriod_int[sib1_MBMS->schedulingInfoList_MBMS_r14.list.array[0]->si_Periodicity_r14];
  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIwindowsize_MBMS = siWindowLength_int[sib1_MBMS->si_WindowLength_r14];
  LOG_I( RRC, "[FRAME unknown][RRC_UE][MOD %02"PRIu8"][][--- MAC_CONFIG_REQ (SIB1-MBMS params eNB %"PRIu8") --->][MAC_UE][MOD %02"PRIu8"][]\n",
         ctxt_pP->module_id, eNB_index, ctxt_pP->module_id );
  rrc_mac_config_req_ue(ctxt_pP->module_id, 0, eNB_index,
                        (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                        (struct LTE_PhysicalConfigDedicated *)NULL,
                        (LTE_SCellToAddMod_r10_t *)NULL,
                        (LTE_MeasObjectToAddMod_t **)NULL,
                        (LTE_MAC_MainConfig_t *)NULL,
                        0,
                        (struct LTE_LogicalChannelConfig *)NULL,
                        (LTE_MeasGapConfig_t *)NULL,
                        (LTE_TDD_Config_t *)NULL,//UE_rrc_inst[ctxt_pP->module_id].sib1[eNB_index]->tdd_Config,
                        (LTE_MobilityControlInfo_t *) NULL,
                        NULL,//&UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIwindowsize,
                        NULL,//&UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIperiod,
                        NULL,
                        NULL,
                        NULL,
                        (LTE_MBSFN_SubframeConfigList_t *)NULL,0,
                        (sib1_MBMS->systemInformationBlockType13_r14==NULL?(LTE_MBSFN_AreaInfoList_r9_t *)NULL:&(sib1_MBMS->systemInformationBlockType13_r14)->mbsfn_AreaInfoList_r9),//(LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                        (LTE_PMCH_InfoList_r9_t *)NULL,
                        0,
                        NULL,
                        NULL,
                        0,
                        (sib1_MBMS->nonMBSFN_SubframeConfig_r14==NULL?(struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL:sib1_MBMS->nonMBSFN_SubframeConfig_r14),//(struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                        (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                       );
  LOG_I(RRC,"Setting SIStatus bit 0 to 1\n");
  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus_MBMS = 1;
  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIB1systemInfoValueTag_MBMS = sib1_MBMS->systemInfoValueTag_r14;

  /*
  if (EPC_MODE_ENABLED) 
    {
      int cell_valid = 0;

      if (sib1->cellAccessRelatedInfo.cellBarred == LTE_SystemInformationBlockType1__cellAccessRelatedInfo__cellBarred_notBarred) {
        int plmn;
        int plmn_number;
        plmn_number = sib1->cellAccessRelatedInfo.plmn_IdentityList.list.count;

        for (plmn = 0; plmn < plmn_number; plmn++) {
          LTE_PLMN_Identity_t *plmn_Identity;
          plmn_Identity = &sib1->cellAccessRelatedInfo.plmn_IdentityList.list.array[plmn]->plmn_Identity;

          if (
            (
              (plmn_Identity->mcc == NULL)
              ||
              (
                (UE_rrc_inst[ctxt_pP->module_id].plmnID.MCCdigit1 == *(plmn_Identity->mcc->list.array[0])) &&
                (UE_rrc_inst[ctxt_pP->module_id].plmnID.MCCdigit2 == *(plmn_Identity->mcc->list.array[1])) &&
                (UE_rrc_inst[ctxt_pP->module_id].plmnID.MCCdigit3 == *(plmn_Identity->mcc->list.array[2]))
              )
            )
            &&
            (UE_rrc_inst[ctxt_pP->module_id].plmnID.MNCdigit1 == *(plmn_Identity->mnc.list.array[0]))
            &&
            (UE_rrc_inst[ctxt_pP->module_id].plmnID.MNCdigit2 == *(plmn_Identity->mnc.list.array[1]))
            &&
            (
              ((UE_rrc_inst[ctxt_pP->module_id].plmnID.MNCdigit3 == 0xf) && (plmn_Identity->mnc.list.count == 2))
              ||
              (UE_rrc_inst[ctxt_pP->module_id].plmnID.MNCdigit3 == *(plmn_Identity->mnc.list.array[2]))
            )
          ) {
            MessageDef  *msg_p;
            msg_p = itti_alloc_new_message(TASK_RRC_UE, 0, NAS_CELL_SELECTION_CNF);
            NAS_CELL_SELECTION_CNF (msg_p).errCode = AS_SUCCESS;
            NAS_CELL_SELECTION_CNF (msg_p).cellID = BIT_STRING_to_uint32(&sib1->cellAccessRelatedInfo.cellIdentity);
            NAS_CELL_SELECTION_CNF (msg_p).tac = BIT_STRING_to_uint16(&sib1->cellAccessRelatedInfo.trackingAreaCode);
            NAS_CELL_SELECTION_CNF (msg_p).rat = 0xFF;
            NAS_CELL_SELECTION_CNF (msg_p).rsrq = rsrq;
            NAS_CELL_SELECTION_CNF (msg_p).rsrp = rsrp;
            itti_send_msg_to_task(TASK_NAS_UE, ctxt_pP->instance, msg_p);
            cell_valid = 1;
            break;
          }
        }
      }

      if (cell_valid == 0) {
        MessageDef  *msg_p;
        msg_p = itti_alloc_new_message(TASK_RRC_UE, 0, PHY_FIND_NEXT_CELL_REQ);
        itti_send_msg_to_task(TASK_PHY_UE, ctxt_pP->instance, msg_p);
        LOG_E(RRC, "Synched with a cell, but PLMN doesn't match our SIM, the message PHY_FIND_NEXT_CELL_REQ is sent but lost in current UE implementation! \n");
      }
    }
  }
  */
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_UE_DECODE_SIB1, VCD_FUNCTION_OUT );
  return 0;
}


//-----------------------------------------------------------------------------
int decode_SIB1( const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index, const uint8_t rsrq, const uint8_t rsrp ) {
  LTE_SystemInformationBlockType1_t *sib1 = UE_rrc_inst[ctxt_pP->module_id].sib1[eNB_index];
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_UE_DECODE_SIB1, VCD_FUNCTION_IN );
  LOG_I( RRC, "[UE %d] : Dumping SIB 1\n", ctxt_pP->module_id );
  const int n = sib1->cellAccessRelatedInfo.plmn_IdentityList.list.count;
  for (int i = 0; i < n; ++i) {
    LTE_PLMN_Identity_t *PLMN_identity = &sib1->cellAccessRelatedInfo.plmn_IdentityList.list.array[i]->plmn_Identity;
    int mccdigits = PLMN_identity->mcc->list.count;
    int mncdigits = PLMN_identity->mnc.list.count;

    int mcc;
    if (mccdigits == 2) {
      mcc = *PLMN_identity->mcc->list.array[0]*10 + *PLMN_identity->mcc->list.array[1];
    } else {
      mcc = *PLMN_identity->mcc->list.array[0]*100 + *PLMN_identity->mcc->list.array[1]*10 + *PLMN_identity->mcc->list.array[2];
    }

    int mnc;
    if (mncdigits == 2) {
      mnc = *PLMN_identity->mnc.list.array[0]*10 + *PLMN_identity->mnc.list.array[1];
    } else {
      mnc = *PLMN_identity->mnc.list.array[0]*100 + *PLMN_identity->mnc.list.array[1]*10 + *PLMN_identity->mnc.list.array[2];
    }

    LOG_I( RRC, "PLMN %d MCC %0*d, MNC %0*d\n", i + 1, mccdigits, mcc, mncdigits, mnc);

    // search internal table for provider name
    const size_t num_plmn_data = sizeof(plmn_data) / sizeof(plmn_data[0]);
    for (size_t plmn_ind = 0;; ++plmn_ind) {
      if (plmn_ind == num_plmn_data) {
        LOG_E( RRC, "Did not find name from internal table for %u %u\n", mcc, mnc);
        break;
      }
      if ((plmn_data[plmn_ind].mcc == mcc) && (plmn_data[plmn_ind].mnc == mnc)) {
        LOG_I( RRC, "Found %s (name from internal table)\n", plmn_data[plmn_ind].oper_short );
        break;
      }
    }
  }
  LOG_I( RRC, "TAC 0x%04x\n",
         ((sib1->cellAccessRelatedInfo.trackingAreaCode.size == 2)?((sib1->cellAccessRelatedInfo.trackingAreaCode.buf[0]<<8) + sib1->cellAccessRelatedInfo.trackingAreaCode.buf[1]):0));
  LOG_I( RRC, "cellReservedForOperatorUse                 : raw:%ld decoded:%s\n", sib1->cellAccessRelatedInfo.plmn_IdentityList.list.array[0]->cellReservedForOperatorUse,
         SIBreserved(sib1->cellAccessRelatedInfo.plmn_IdentityList.list.array[0]->cellReservedForOperatorUse) );

  LOG_I( RRC, "cellAccessRelatedInfo.cellIdentity         : raw:%"PRIu32" decoded:%02x.%02x.%02x.%02x\n",
         BIT_STRING_to_uint32( &sib1->cellAccessRelatedInfo.cellIdentity ),
         sib1->cellAccessRelatedInfo.cellIdentity.buf[0],
         sib1->cellAccessRelatedInfo.cellIdentity.buf[1],
         sib1->cellAccessRelatedInfo.cellIdentity.buf[2],
         sib1->cellAccessRelatedInfo.cellIdentity.buf[3] >> sib1->cellAccessRelatedInfo.cellIdentity.bits_unused);
  LOG_I( RRC, "cellAccessRelatedInfo.cellBarred           : raw:%ld decoded:%s\n", sib1->cellAccessRelatedInfo.cellBarred, SIBbarred(sib1->cellAccessRelatedInfo.cellBarred) );
  LOG_I( RRC, "cellAccessRelatedInfo.intraFreqReselection : raw:%ld decoded:%s\n", sib1->cellAccessRelatedInfo.intraFreqReselection, SIBallowed(sib1->cellAccessRelatedInfo.intraFreqReselection) );
  LOG_I( RRC, "cellAccessRelatedInfo.csg_Indication       : %d\n", sib1->cellAccessRelatedInfo.csg_Indication );

  if (sib1->cellAccessRelatedInfo.csg_Identity)
    LOG_I( RRC, "cellAccessRelatedInfo.csg_Identity         : %"PRIu32"\n", BIT_STRING_to_uint32(sib1->cellAccessRelatedInfo.csg_Identity) );
  else
    LOG_I( RRC, "cellAccessRelatedInfo.csg_Identity         : not defined\n" );

  LOG_I( RRC, "cellSelectionInfo.q_RxLevMin               : %ld\n", sib1->cellSelectionInfo.q_RxLevMin );

  if (sib1->cellSelectionInfo.q_RxLevMinOffset)
    LOG_I( RRC, "cellSelectionInfo.q_RxLevMinOffset         : %ld\n", *sib1->cellSelectionInfo.q_RxLevMinOffset );
  else
    LOG_I( RRC, "cellSelectionInfo.q_RxLevMinOffset         : not defined\n" );

  if (sib1->p_Max)
    LOG_I( RRC, "p_Max                                      : %ld\n", *sib1->p_Max );
  else
    LOG_I( RRC, "p_Max                                      : not defined\n" );

  LOG_I( RRC, "freqBandIndicator                          : %ld\n", sib1->freqBandIndicator );

  if (sib1->schedulingInfoList.list.count > 0) {
    for (int i=0; i<sib1->schedulingInfoList.list.count; i++) {
      LOG_I( RRC, "si_Periodicity[%d]                          : %s\n", i, SIBPeriod[min(sib1->schedulingInfoList.list.array[i]->si_Periodicity,7)]);

      if (sib1->schedulingInfoList.list.array[i]->sib_MappingInfo.list.count > 0) {
        char temp[32 * sizeof(SIBType[0])] = {0}; // maxSIB==32

        for (int j=0; j<sib1->schedulingInfoList.list.array[i]->sib_MappingInfo.list.count; j++) {
          sprintf( temp + j*sizeof(SIBType[0]), "%*s ", (int)sizeof(SIBType[0])-1, SIBType[min(*sib1->schedulingInfoList.list.array[i]->sib_MappingInfo.list.array[0],11)] );
        }

        LOG_I( RRC, "siSchedulingInfoSIBType[%d]                 : %s\n", i, temp );
      } else {
        LOG_I( RRC, "mapping list %d is null\n", i );
      }
    }
  } else {
    LOG_E( RRC, "siSchedulingInfoPeriod[0]                  : PROBLEM!!!\n" );
    return -1;
  }

  if (sib1->tdd_Config) {
    LOG_I( RRC, "TDD subframeAssignment                     : %ld\n", sib1->tdd_Config->subframeAssignment );
    LOG_I( RRC, "TDD specialSubframePatterns                : %ld\n", sib1->tdd_Config->specialSubframePatterns );
  }

  LOG_I( RRC, "siWindowLength                             : %s\n", siWindowLength[min(sib1->si_WindowLength,8)] );
  LOG_I( RRC, "systemInfoValueTag                         : %ld\n", sib1->systemInfoValueTag );
  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIperiod     = siPeriod_int[sib1->schedulingInfoList.list.array[0]->si_Periodicity];
  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIwindowsize = siWindowLength_int[sib1->si_WindowLength];
  LOG_I( RRC, "[FRAME unknown][RRC_UE][MOD %02"PRIu8"][][--- MAC_CONFIG_REQ (SIB1 params eNB %"PRIu8") --->][MAC_UE][MOD %02"PRIu8"][]\n",
         ctxt_pP->module_id, eNB_index, ctxt_pP->module_id );
  rrc_mac_config_req_ue(ctxt_pP->module_id, 0, eNB_index,
                        (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                        (struct LTE_PhysicalConfigDedicated *)NULL,
                        (LTE_SCellToAddMod_r10_t *)NULL,
                        (LTE_MeasObjectToAddMod_t **)NULL,
                        (LTE_MAC_MainConfig_t *)NULL,
                        0,
                        (struct LTE_LogicalChannelConfig *)NULL,
                        (LTE_MeasGapConfig_t *)NULL,
                        UE_rrc_inst[ctxt_pP->module_id].sib1[eNB_index]->tdd_Config,
                        (LTE_MobilityControlInfo_t *) NULL,
                        &UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIwindowsize,
                        &UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIperiod,
                        NULL,
                        NULL,
                        NULL,
                        (LTE_MBSFN_SubframeConfigList_t *)NULL,0,
                        (LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                        (LTE_PMCH_InfoList_r9_t *)NULL,
                        0,
                        NULL,
                        NULL,
                        0,
                        (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                        (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                       );
  LOG_I(RRC,"Setting SIStatus bit 0 to 1\n");
  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus = 1;
  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIB1systemInfoValueTag = sib1->systemInfoValueTag;

  if (EPC_MODE_ENABLED) {
    int cell_valid = 0;

    if (sib1->cellAccessRelatedInfo.cellBarred == LTE_SystemInformationBlockType1__cellAccessRelatedInfo__cellBarred_notBarred) {
      /* Cell is not barred */
      int plmn;
      int plmn_number;
      plmn_number = sib1->cellAccessRelatedInfo.plmn_IdentityList.list.count;

      /* Compare requested PLMN and PLMNs from SIB1*/
      for (plmn = 0; plmn < plmn_number; plmn++) {
        LTE_PLMN_Identity_t *plmn_Identity;
        plmn_Identity = &sib1->cellAccessRelatedInfo.plmn_IdentityList.list.array[plmn]->plmn_Identity;

        if (
          (
            (plmn_Identity->mcc == NULL)
            ||
            (
              (UE_rrc_inst[ctxt_pP->module_id].plmnID.MCCdigit1 == *(plmn_Identity->mcc->list.array[0])) &&
              (UE_rrc_inst[ctxt_pP->module_id].plmnID.MCCdigit2 == *(plmn_Identity->mcc->list.array[1])) &&
              (UE_rrc_inst[ctxt_pP->module_id].plmnID.MCCdigit3 == *(plmn_Identity->mcc->list.array[2]))
            )
          )
          &&
          (UE_rrc_inst[ctxt_pP->module_id].plmnID.MNCdigit1 == *(plmn_Identity->mnc.list.array[0]))
          &&
          (UE_rrc_inst[ctxt_pP->module_id].plmnID.MNCdigit2 == *(plmn_Identity->mnc.list.array[1]))
          &&
          (
            ((UE_rrc_inst[ctxt_pP->module_id].plmnID.MNCdigit3 == 0xf) && (plmn_Identity->mnc.list.count == 2))
            ||
            (UE_rrc_inst[ctxt_pP->module_id].plmnID.MNCdigit3 == *(plmn_Identity->mnc.list.array[2]))
          )
        ) {
          /* PLMN match, send a confirmation to NAS */
          MessageDef  *msg_p;
          msg_p = itti_alloc_new_message(TASK_RRC_UE, 0, NAS_CELL_SELECTION_CNF);
          NAS_CELL_SELECTION_CNF (msg_p).errCode = AS_SUCCESS;
          NAS_CELL_SELECTION_CNF (msg_p).cellID = BIT_STRING_to_uint32(&sib1->cellAccessRelatedInfo.cellIdentity);
          NAS_CELL_SELECTION_CNF (msg_p).tac = BIT_STRING_to_uint16(&sib1->cellAccessRelatedInfo.trackingAreaCode);
          NAS_CELL_SELECTION_CNF (msg_p).rat = 0xFF;
          NAS_CELL_SELECTION_CNF (msg_p).rsrq = rsrq;
          NAS_CELL_SELECTION_CNF (msg_p).rsrp = rsrp;
          itti_send_msg_to_task(TASK_NAS_UE, ctxt_pP->instance, msg_p);
          cell_valid = 1;
          UE_rrc_inst[ctxt_pP->module_id].selected_plmn_identity = plmn + 1;
          break;
        }
      }
    }

    if (cell_valid == 0) {
      /* Cell can not be used, ask PHY to try the next one */
      MessageDef  *msg_p;
      msg_p = itti_alloc_new_message(TASK_RRC_UE, 0, PHY_FIND_NEXT_CELL_REQ);
      itti_send_msg_to_task(TASK_PHY_UE, ctxt_pP->instance, msg_p);
      LOG_E(RRC,
            "Synched with a cell, but PLMN doesn't match our SIM "
            "(selected_plmn_identity %d), the message PHY_FIND_NEXT_CELL_REQ "
            "is sent but lost in current UE implementation!\n",
            UE_rrc_inst[ctxt_pP->module_id].selected_plmn_identity);
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_UE_DECODE_SIB1, VCD_FUNCTION_OUT );
  return 0;
}


//-----------------------------------------------------------------------------
void dump_sib2( LTE_SystemInformationBlockType2_t *sib2 ) {
  // ac_BarringInfo
  if (sib2->ac_BarringInfo) {
    LOG_I( RRC, "ac_BarringInfo->ac_BarringForEmergency : %d\n",
           sib2->ac_BarringInfo->ac_BarringForEmergency );

    if (sib2->ac_BarringInfo->ac_BarringForMO_Signalling) {
      LOG_I( RRC, "ac_BarringInfo->ac_BarringForMO_Signalling->ac_BarringFactor       : %ld\n",
             sib2->ac_BarringInfo->ac_BarringForMO_Signalling->ac_BarringFactor );
      LOG_I( RRC, "ac_BarringInfo->ac_BarringForMO_Signalling->ac_BarringTime         : %ld\n",
             sib2->ac_BarringInfo->ac_BarringForMO_Signalling->ac_BarringTime );
      LOG_I( RRC, "ac_BarringInfo->ac_BarringForMO_Signalling->ac_BarringForSpecialAC : %"PRIu32"\n",
             BIT_STRING_to_uint32(&sib2->ac_BarringInfo->ac_BarringForMO_Signalling->ac_BarringForSpecialAC) );
    } else
      LOG_I( RRC, "ac_BarringInfo->ac_BarringForMO_Signalling : not defined\n" );

    if (sib2->ac_BarringInfo->ac_BarringForMO_Data) {
      LOG_I( RRC, "ac_BarringInfo->ac_BarringForMO_Data->ac_BarringFactor       : %ld\n",
             sib2->ac_BarringInfo->ac_BarringForMO_Data->ac_BarringFactor );
      LOG_I( RRC, "ac_BarringInfo->ac_BarringForMO_Data->ac_BarringTime         : %ld\n",
             sib2->ac_BarringInfo->ac_BarringForMO_Data->ac_BarringTime );
      LOG_I( RRC, "ac_BarringInfo->ac_BarringForMO_Data->ac_BarringForSpecialAC : %"PRIu32"\n",
             BIT_STRING_to_uint32(&sib2->ac_BarringInfo->ac_BarringForMO_Data->ac_BarringForSpecialAC) );
    } else
      LOG_I( RRC, "ac_BarringInfo->ac_BarringForMO_Data : not defined\n" );
  } else
    LOG_I( RRC, "ac_BarringInfo : not defined\n" );

  // RACH
  LOG_I( RRC, "radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.numberOfRA_Preambles  : raw:%ld decoded:%s\n",
         sib2->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.numberOfRA_Preambles,
         SIB2numberOfRA_Preambles(sib2->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.numberOfRA_Preambles) );

  if (sib2->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig) {
    LOG_I( RRC, "radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig->sizeOfRA_PreamblesGroupA : %ld\n",
           sib2->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig->sizeOfRA_PreamblesGroupA );
    LOG_I( RRC, "radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig->messageSizeGroupA        : %ld\n",
           sib2->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig->messageSizeGroupA );
    LOG_I( RRC, "radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig->messagePowerOffsetGroupB : %ld\n",
           sib2->radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig->messagePowerOffsetGroupB );
  } else {
    LOG_I( RRC, "radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig : not defined\n" );
  }

  LOG_I( RRC, "radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.powerRampingStep                   : raw:%ld decoded:%s\n",
         sib2->radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.powerRampingStep,
         SIB2powerRampingStep(sib2->radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.powerRampingStep) );
  LOG_I( RRC, "radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.preambleInitialReceivedTargetPower : raw:%ld decoded:%s\n",
         sib2->radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.preambleInitialReceivedTargetPower,
         SIB2preambleInitialReceivedTargetPower(sib2->radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.preambleInitialReceivedTargetPower) );
  LOG_I( RRC, "radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.preambleTransMax              : raw:%ld decoded:%s\n",
         sib2->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.preambleTransMax,
         SIB2preambleTransMax(sib2->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.preambleTransMax) );
  LOG_I( RRC, "radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.ra_ResponseWindowSize         : raw:%ld decoded:%s\n",
         sib2->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.ra_ResponseWindowSize,
         SIB2ra_ResponseWindowSize(sib2->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.ra_ResponseWindowSize) );
  LOG_I( RRC, "radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.mac_ContentionResolutionTimer : raw:%ld decoded:%s\n",
         sib2->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.mac_ContentionResolutionTimer,
         SIB2mac_ContentionResolutionTimer(sib2->radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.mac_ContentionResolutionTimer) );
  LOG_I( RRC, "radioResourceConfigCommon.rach_ConfigCommon.maxHARQ_Msg3Tx : %ld\n",
         sib2->radioResourceConfigCommon.rach_ConfigCommon.maxHARQ_Msg3Tx );
  // BCCH
  LOG_I( RRC, "radioResourceConfigCommon.bcch_Config.modificationPeriodCoeff : raw:%ld decoded:%s\n",
         sib2->radioResourceConfigCommon.bcch_Config.modificationPeriodCoeff,
         SIB2modificationPeriodCoeff(sib2->radioResourceConfigCommon.bcch_Config.modificationPeriodCoeff) );
  // PCCH
  LOG_I( RRC, "radioResourceConfigCommon.pcch_Config.defaultPagingCycle : raw:%ld decoded:%s\n",
         sib2->radioResourceConfigCommon.pcch_Config.defaultPagingCycle,
         SIB2defaultPagingCycle(sib2->radioResourceConfigCommon.pcch_Config.defaultPagingCycle) );
  LOG_I( RRC, "radioResourceConfigCommon.pcch_Config.nB                 : raw:%ld decoded:%s\n",
         sib2->radioResourceConfigCommon.pcch_Config.nB,
         SIB2nB(sib2->radioResourceConfigCommon.pcch_Config.nB) );
  // PRACH
  LOG_I( RRC, "radioResourceConfigCommon.prach_Config.rootSequenceIndex                          : %ld\n",
         sib2->radioResourceConfigCommon.prach_Config.rootSequenceIndex );
  LOG_I( RRC, "radioResourceConfigCommon.prach_Config.prach_ConfigInfo.prach_ConfigIndex         : %ld\n",
         sib2->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.prach_ConfigIndex );
  LOG_I( RRC, "radioResourceConfigCommon.prach_Config.prach_ConfigInfo.highSpeedFlag             : %d\n",
         sib2->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.highSpeedFlag );
  LOG_I( RRC, "radioResourceConfigCommon.prach_Config.prach_ConfigInfo.zeroCorrelationZoneConfig : %ld\n",
         sib2->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.zeroCorrelationZoneConfig );
  LOG_I( RRC, "radioResourceConfigCommon.prach_Config.prach_ConfigInfo.prach_FreqOffset          : %ld\n",
         sib2->radioResourceConfigCommon.prach_Config.prach_ConfigInfo.prach_FreqOffset );
  // PDSCH-Config
  LOG_I( RRC, "radioResourceConfigCommon.pdsch_ConfigCommon.referenceSignalPower : %ld\n",
         sib2->radioResourceConfigCommon.pdsch_ConfigCommon.referenceSignalPower );
  LOG_I( RRC, "radioResourceConfigCommon.pdsch_ConfigCommon.p_b                  : %ld\n",
         sib2->radioResourceConfigCommon.pdsch_ConfigCommon.p_b );
  // PUSCH-Config
  LOG_I( RRC, "radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.n_SB                : %ld\n",
         sib2->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.n_SB );
  LOG_I( RRC, "radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode         : %ld\n",
         sib2->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode );
  LOG_I( RRC, "radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset : %ld\n",
         sib2->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset );
  LOG_I( RRC, "radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM         : %d\n",
         sib2->radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM );
  LOG_I( RRC, "radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled    : %d\n",
         sib2->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled );
  LOG_I( RRC, "radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH   : %ld\n",
         sib2->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH );
  LOG_I( RRC, "radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled : %d\n",
         sib2->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled );
  LOG_I( RRC, "radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift            : %ld\n",
         sib2->radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift );
  // PUCCH-Config
  LOG_I( RRC, "radioResourceConfigCommon.pucch_ConfigCommon.deltaPUCCH_Shift : %ld\n",
         sib2->radioResourceConfigCommon.pucch_ConfigCommon.deltaPUCCH_Shift );
  LOG_I( RRC, "radioResourceConfigCommon.pucch_ConfigCommon.nRB_CQI          : %ld\n",
         sib2->radioResourceConfigCommon.pucch_ConfigCommon.nRB_CQI );
  LOG_I( RRC, "radioResourceConfigCommon.pucch_ConfigCommon.nCS_AN           : %ld\n",
         sib2->radioResourceConfigCommon.pucch_ConfigCommon.nCS_AN );
  LOG_I( RRC, "radioResourceConfigCommon.pucch_ConfigCommon.n1PUCCH_AN       : %ld\n",
         sib2->radioResourceConfigCommon.pucch_ConfigCommon.n1PUCCH_AN );
  // SoundingRS_UL_Config
  LOG_I( RRC, "radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present : raw:%d decoded:%s\n",
         sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present,
         SIB2SoundingPresent(sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present) );

  if (sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present == LTE_SoundingRS_UL_ConfigCommon_PR_setup) {
    LOG_I( RRC, "radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_BandwidthConfig                 : %ld\n",
           sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_BandwidthConfig );
    LOG_I( RRC, "radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig                  : %ld\n",
           sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig );
    LOG_I( RRC, "radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.ackNackSRS_SimultaneousTransmission : %d\n",
           sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.ackNackSRS_SimultaneousTransmission );

    if(sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_MaxUpPts) {
      LOG_I( RRC, "radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_MaxUpPts                        : %ld\n",
             /* TODO: check that it's okay to access [0] */
             sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.srs_MaxUpPts[0] );
    }
  }

  // uplinkPowerControlCommon
  LOG_I( RRC, "radioResourceConfigCommon.uplinkPowerControlCommon.p0_NominalPUSCH   : %ld\n",
         sib2->radioResourceConfigCommon.uplinkPowerControlCommon.p0_NominalPUSCH );
  LOG_I( RRC, "radioResourceConfigCommon.uplinkPowerControlCommon.alpha             : %ld\n",
         sib2->radioResourceConfigCommon.uplinkPowerControlCommon.alpha );
  LOG_I( RRC, "radioResourceConfigCommon.uplinkPowerControlCommon.p0_NominalPUCCH   : %ld\n",
         sib2->radioResourceConfigCommon.uplinkPowerControlCommon.p0_NominalPUCCH );
  LOG_I( RRC, "radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1  : %ld\n",
         sib2->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1 );
  LOG_I( RRC, "radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1b : %ld\n",
         sib2->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1b );
  LOG_I( RRC, "radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2  : %ld\n",
         sib2->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2 );
  LOG_I( RRC, "radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2a : %ld\n",
         sib2->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2a );
  LOG_I( RRC, "radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2b : %ld\n",
         sib2->radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2b );
  LOG_I( RRC, "radioResourceConfigCommon.uplinkPowerControlCommon.deltaPreambleMsg3 : %ld\n",
         sib2->radioResourceConfigCommon.uplinkPowerControlCommon.deltaPreambleMsg3 );
  LOG_I( RRC, "radioResourceConfigCommon.ul_CyclicPrefixLength : %ld\n",
         sib2->radioResourceConfigCommon.ul_CyclicPrefixLength );
  LOG_I( RRC, "ue_TimersAndConstants.t300 : %ld\n", sib2->ue_TimersAndConstants.t300 );
  LOG_I( RRC, "ue_TimersAndConstants.t301 : %ld\n", sib2->ue_TimersAndConstants.t301 );
  LOG_I( RRC, "ue_TimersAndConstants.t310 : %ld\n", sib2->ue_TimersAndConstants.t310 );
  LOG_I( RRC, "ue_TimersAndConstants.n310 : %ld\n", sib2->ue_TimersAndConstants.n310 );
  LOG_I( RRC, "ue_TimersAndConstants.t311 : %ld\n", sib2->ue_TimersAndConstants.t311 );
  LOG_I( RRC, "ue_TimersAndConstants.n311 : %ld\n", sib2->ue_TimersAndConstants.n311 );

  if (sib2->freqInfo.ul_CarrierFreq)
    LOG_I( RRC, "freqInfo.ul_CarrierFreq             : %ld\n", *sib2->freqInfo.ul_CarrierFreq );
  else
    LOG_I( RRC, "freqInfo.ul_CarrierFreq             : not defined\n" );

  if (sib2->freqInfo.ul_Bandwidth)
    LOG_I( RRC, "freqInfo.ul_Bandwidth               : %ld\n", *sib2->freqInfo.ul_Bandwidth );
  else
    LOG_I( RRC, "freqInfo.ul_Bandwidth               : not defined\n" );

  LOG_I( RRC, "freqInfo.additionalSpectrumEmission : %ld\n", sib2->freqInfo.additionalSpectrumEmission );

  if (sib2->mbsfn_SubframeConfigList) {
    LOG_I( RRC, "mbsfn_SubframeConfigList : %p\n", sib2->mbsfn_SubframeConfigList );
    // FIXME
  } else
    LOG_I( RRC, "mbsfn_SubframeConfigList : not defined\n" );

  LOG_I( RRC, "timeAlignmentTimerCommon : %ld\n", sib2->timeAlignmentTimerCommon );

  if (sib2->lateNonCriticalExtension) {
    LOG_I( RRC, "lateNonCriticalExtension : %p\n", sib2->lateNonCriticalExtension );
  } else
    LOG_I( RRC, "lateNonCriticalExtension : not defined\n" );

  if (sib2->ext1 && sib2->ext1->ssac_BarringForMMTEL_Voice_r9) {
    LOG_I( RRC, "ssac_BarringForMMTEL_Voice_r9->ac_BarringFactor       : %ld\n",
           sib2->ext1->ssac_BarringForMMTEL_Voice_r9->ac_BarringFactor );
    LOG_I( RRC, "ssac_BarringForMMTEL_Voice_r9->ac_BarringTime         : %ld\n",
           sib2->ext1->ssac_BarringForMMTEL_Voice_r9->ac_BarringTime );
    LOG_I( RRC, "ssac_BarringForMMTEL_Voice_r9->ac_BarringForSpecialAC : %"PRIu32"\n",
           BIT_STRING_to_uint32(&sib2->ext1->ssac_BarringForMMTEL_Voice_r9->ac_BarringForSpecialAC) );
  } else
    LOG_I( RRC, "ssac_BarringForMMTEL_Voice_r9 : not defined\n" );

  if (sib2->ext1 && sib2->ext1->ssac_BarringForMMTEL_Video_r9) {
    LOG_I( RRC, "ssac_BarringForMMTEL_Video_r9->ac_BarringFactor       : %ld\n",
           sib2->ext1->ssac_BarringForMMTEL_Video_r9->ac_BarringFactor );
    LOG_I( RRC, "ssac_BarringForMMTEL_Video_r9->ac_BarringTime         : %ld\n",
           sib2->ext1->ssac_BarringForMMTEL_Video_r9->ac_BarringTime );
    LOG_I( RRC, "ssac_BarringForMMTEL_Video_r9->ac_BarringForSpecialAC : %"PRIu32"\n",
           BIT_STRING_to_uint32(&sib2->ext1->ssac_BarringForMMTEL_Video_r9->ac_BarringForSpecialAC) );
  } else
    LOG_I( RRC, "ssac_BarringForMMTEL_Video_r9 : not defined\n" );

  if (sib2->ext2 && sib2->ext2->ac_BarringForCSFB_r10) {
    LOG_I( RRC, "ac_BarringForCSFB_r10->ac_BarringFactor       : %ld\n",
           sib2->ext2->ac_BarringForCSFB_r10->ac_BarringFactor );
    LOG_I( RRC, "ac_BarringForCSFB_r10->ac_BarringTime         : %ld\n",
           sib2->ext2->ac_BarringForCSFB_r10->ac_BarringTime );
    LOG_I( RRC, "ac_BarringForCSFB_r10->ac_BarringForSpecialAC : %"PRIu32"\n",
           BIT_STRING_to_uint32(&sib2->ext2->ac_BarringForCSFB_r10->ac_BarringForSpecialAC) );
  } else
    LOG_I( RRC, "ac_BarringForCSFB_r10 : not defined\n" );
}

//-----------------------------------------------------------------------------
void dump_sib3( LTE_SystemInformationBlockType3_t *sib3 ) {
  LOG_I( RRC, "Dumping SIB3 (see TS36.331 V8.21.0)\n" );
  int q_Hyst_dB = sib3->cellReselectionInfoCommon.q_Hyst; // sib3->cellReselectionInfoCommon.q_Hyst is a enumerated value

  if (q_Hyst_dB > 6)
    q_Hyst_dB = (q_Hyst_dB-6) * 2 + 6;

  LOG_I( RRC, "cellReselectionInfoCommon.q_Hyst : raw:%ld  decoded:%d dB\n", sib3->cellReselectionInfoCommon.q_Hyst, q_Hyst_dB );

  if (sib3->cellReselectionInfoCommon.speedStateReselectionPars) {
    LOG_I( RRC, "cellReselectionInfoCommon.speedStateReselectionPars->mobilityStateParameters.t_Evaluation : %ld\n",
           sib3->cellReselectionInfoCommon.speedStateReselectionPars->mobilityStateParameters.t_Evaluation );
    LOG_I( RRC, "cellReselectionInfoCommon.speedStateReselectionPars->mobilityStateParameters.t_HystNormal : %ld\n",
           sib3->cellReselectionInfoCommon.speedStateReselectionPars->mobilityStateParameters.t_HystNormal );
    LOG_I( RRC, "cellReselectionInfoCommon.speedStateReselectionPars->mobilityStateParameters.n_CellChangeMedium : %ld\n",
           sib3->cellReselectionInfoCommon.speedStateReselectionPars->mobilityStateParameters.n_CellChangeMedium );
    LOG_I( RRC, "cellReselectionInfoCommon.speedStateReselectionPars->mobilityStateParameters.n_CellChangeHigh : %ld\n",
           sib3->cellReselectionInfoCommon.speedStateReselectionPars->mobilityStateParameters.n_CellChangeHigh );
    LOG_I( RRC, "cellReselectionInfoCommon.speedStateReselectionPars->q_HystSF.sf_Medium : raw:%ld decoded:%ld dB\n", sib3->cellReselectionInfoCommon.speedStateReselectionPars->q_HystSF.sf_Medium,
           6 - 2 * sib3->cellReselectionInfoCommon.speedStateReselectionPars->q_HystSF.sf_Medium );
    LOG_I( RRC, "cellReselectionInfoCommon.speedStateReselectionPars->q_HystSF.sf_High : raw:%ld decoded:%ld dB\n", sib3->cellReselectionInfoCommon.speedStateReselectionPars->q_HystSF.sf_High,
           6 - 2 * sib3->cellReselectionInfoCommon.speedStateReselectionPars->q_HystSF.sf_High );
  } else {
    LOG_I( RRC, "cellReselectionInfoCommon.speedStateReselectionPars : not defined\n" );
  }

  if (sib3->cellReselectionServingFreqInfo.s_NonIntraSearch) {
    LOG_I( RRC, "cellReselectionServingFreqInfo.s_NonIntraSearch : %ld\n", *(sib3->cellReselectionServingFreqInfo.s_NonIntraSearch) );
  } else {
    LOG_I( RRC, "cellReselectionServingFreqInfo.s_NonIntraSearch : not defined\n" );
  }

  LOG_I( RRC, "cellReselectionServingFreqInfo.threshServingLow : %ld\n", sib3->cellReselectionServingFreqInfo.threshServingLow );
  LOG_I( RRC, "cellReselectionServingFreqInfo.cellReselectionPriority : %ld\n", sib3->cellReselectionServingFreqInfo.cellReselectionPriority );
  LOG_I( RRC, "intraFreqCellReselectionInfo.q_RxLevMin : %ld\n", sib3->intraFreqCellReselectionInfo.q_RxLevMin );

  if (sib3->intraFreqCellReselectionInfo.p_Max) {
    LOG_I( RRC, "intraFreqCellReselectionInfo.p_Max : %ld\n", *(sib3->intraFreqCellReselectionInfo.p_Max) );
  } else {
    LOG_I( RRC, "intraFreqCellReselectionInfo.p_Max : not defined\n" );
  }

  if (sib3->intraFreqCellReselectionInfo.s_IntraSearch) {
    LOG_I( RRC, "intraFreqCellReselectionInfo.s_IntraSearch : %ld\n", *(sib3->intraFreqCellReselectionInfo.s_IntraSearch) );
  } else {
    LOG_I( RRC, "intraFreqCellReselectionInfo.s_IntraSearch : not defined\n" );
  }

  if (sib3->intraFreqCellReselectionInfo.allowedMeasBandwidth) {
    LOG_I( RRC, "intraFreqCellReselectionInfo.allowedMeasBandwidth : %ld\n", *(sib3->intraFreqCellReselectionInfo.allowedMeasBandwidth) );
  } else {
    LOG_I( RRC, "intraFreqCellReselectionInfo.allowedMeasBandwidth : not defined\n" );
  }

  LOG_I( RRC, "intraFreqCellReselectionInfo.presenceAntennaPort1 : %d\n", sib3->intraFreqCellReselectionInfo.presenceAntennaPort1 );
  LOG_I( RRC, "intraFreqCellReselectionInfo.neighCellConfig : %"PRIu8"\n", BIT_STRING_to_uint8( &(sib3->intraFreqCellReselectionInfo.neighCellConfig) ) );
  LOG_I( RRC, "intraFreqCellReselectionInfo.t_ReselectionEUTRA : %ld\n", sib3->intraFreqCellReselectionInfo.t_ReselectionEUTRA );

  if (sib3->intraFreqCellReselectionInfo.t_ReselectionEUTRA_SF) {
    LOG_I( RRC, "intraFreqCellReselectionInfo.t_ReselectionEUTRA_SF->sf_Medium : %ld\n", sib3->intraFreqCellReselectionInfo.t_ReselectionEUTRA_SF->sf_Medium );
    LOG_I( RRC, "intraFreqCellReselectionInfo.t_ReselectionEUTRA_SF->sf_High : %ld\n", sib3->intraFreqCellReselectionInfo.t_ReselectionEUTRA_SF->sf_High );
  } else {
    LOG_I( RRC, "intraFreqCellReselectionInfo.t_ReselectionEUTRA_SF : not defined\n" );
  }
}

int Qoffsettab[31] = {-24,-22,-20,-18,-16,-14,-12,-10,-8,-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6,8,10,12,14,16,18,20,22,24};
int PhysCellIdRange[16] = {4,8,12,16,24,32,48,64,84,96,128,168,252,504,0,0};

uint64_t arfcn_to_freq(long arfcn) {
  if (arfcn < 600)  // Band 1
    return((uint64_t)2110000000 + (arfcn*100000));
  else if (arfcn <1200) // Band 2
    return((uint64_t)1930000000 + ((arfcn-600)*100000));
  else if (arfcn <1950) // Band 3
    return((uint64_t)1805000000 + ((arfcn-1200)*100000));
  else if (arfcn <2400) // Band 4
    return((uint64_t)2110000000 + ((arfcn-1950)*100000));
  else if (arfcn <2650) // Band 5
    return((uint64_t)869000000 + ((arfcn-2400)*100000));
  else if (arfcn <2750) // Band 6
    return((uint64_t)875000000 + ((arfcn-2650)*100000));
  else if (arfcn <3450) // Band 7
    return((uint64_t)2620000000 + ((arfcn-2750)*100000));
  else if (arfcn <3800) // Band 8
    return((uint64_t)925000000 + ((arfcn-3450)*100000));
  else if (arfcn <4150) // Band 9
    return((uint64_t)1844900000 + ((arfcn-3800)*100000));
  else if (arfcn <4650) // Band 10
    return((uint64_t)2110000000 + ((arfcn-4150)*100000));
  else if (arfcn <5010) // Band 11
    return((uint64_t)1475900000 + ((arfcn-4750)*100000));
  else if (arfcn <5180) // Band 12
    return((uint64_t)729000000 + ((arfcn-5010)*100000));
  else if (arfcn <5280) // Band 13
    return((uint64_t)746000000 + ((arfcn-5180)*100000));
  else if (arfcn <5730) // Band 14
    return((uint64_t)758000000 + ((arfcn-5280)*100000));
  else if (arfcn <5850) // Band 17
    return((uint64_t)734000000 + ((arfcn-5730)*100000));
  else if (arfcn <6000) // Band 18
    return((uint64_t)860000000 + ((arfcn-5850)*100000));
  else if (arfcn <6150) // Band 19
    return((uint64_t)875000000 + ((arfcn-6000)*100000));
  else if (arfcn <6450) // Band 20
    return((uint64_t)791000000 + ((arfcn-6150)*100000));
  else if (arfcn <6600) // Band 21
    return((uint64_t)1495900000 + ((arfcn-6450)*100000));
  else if (arfcn <7500) // Band 22
    return((uint64_t)351000000 + ((arfcn-6600)*100000));
  else if (arfcn <7700) // Band 23
    return((uint64_t)2180000000 + ((arfcn-7500)*100000));
  else if (arfcn <8040) // Band 24
    return((uint64_t)1525000000 + ((arfcn-7700)*100000));
  else if (arfcn <8690) // Band 25
    return((uint64_t)1930000000 + ((arfcn-8040)*100000));
  else if (arfcn <36200) // Band 33
    return((uint64_t)1900000000 + ((arfcn-36000)*100000));
  else if (arfcn <36350) // Band 34
    return((uint64_t)2010000000 + ((arfcn-36200)*100000));
  else if (arfcn <36950) // Band 35
    return((uint64_t)1850000000 + ((arfcn-36350)*100000));
  else if (arfcn <37550) // Band 36
    return((uint64_t)1930000000 + ((arfcn-36950)*100000));
  else if (arfcn <37750) // Band 37
    return((uint64_t)1910000000 + ((arfcn-37550)*100000));
  else if (arfcn <38250) // Band 38
    return((uint64_t)2570000000 + ((arfcn-37750)*100000));
  else if (arfcn <38650) // Band 39
    return((uint64_t)1880000000 + ((arfcn-38250)*100000));
  else if (arfcn <39650) // Band 40
    return((uint64_t)2300000000 + ((arfcn-38650)*100000));
  else if (arfcn <41590) // Band 41
    return((uint64_t)2496000000 + ((arfcn-39650)*100000));
  else if (arfcn <43590) // Band 42
    return((uint64_t)3400000000 + ((arfcn-41590)*100000));
  else if (arfcn <45590) // Band 43
    return((uint64_t)3600000000 + ((arfcn-43950)*100000));
  else {
    LOG_E(RRC,"Unknown EARFCN %ld\n",arfcn);
    exit(1);
  }
}
void dump_sib5( LTE_SystemInformationBlockType5_t *sib5 ) {
  LTE_InterFreqCarrierFreqList_t interFreqCarrierFreqList = sib5->interFreqCarrierFreqList;
  int i,j;
  LTE_InterFreqCarrierFreqInfo_t *ifcfInfo;
  LOG_I( RRC, "Dumping SIB5 (see TS36.331 V8.21.0)\n" );

  for (i=0; i<interFreqCarrierFreqList.list.count; i++) {
    LOG_I(RRC, "SIB5 InterFreqCarrierFreq element %d/%d\n",i,interFreqCarrierFreqList.list.count);
    ifcfInfo = interFreqCarrierFreqList.list.array[i];
    LOG_I(RRC, "   DL Carrier Frequency/ARFCN : %ld/%ld\n",
          arfcn_to_freq(ifcfInfo->dl_CarrierFreq),
          ifcfInfo->dl_CarrierFreq);
    LOG_I(RRC,"   Q_RXLevMin : %ld\n", ifcfInfo->q_RxLevMin);

    if (ifcfInfo->p_Max != NULL)
      LOG_I(RRC,"   P_max : %ld\n", *ifcfInfo->p_Max);

    LOG_I(RRC,"   T_ReselectionEUTRA : %ld\n",ifcfInfo->t_ReselectionEUTRA);

    if (ifcfInfo->t_ReselectionEUTRA_SF) {
      LOG_I(RRC,"   t_ReselectionEUTRA_SF.sf_Medium %ld, t_ReselectionEUTRA_SF.sf_High %ld",
            ifcfInfo->t_ReselectionEUTRA_SF->sf_Medium,
            ifcfInfo->t_ReselectionEUTRA_SF->sf_High);
    }

    LOG_I(RRC,"   threshX_High : %ld\n",ifcfInfo->threshX_High);
    LOG_I(RRC,"   threshX_Low : %ld\n",ifcfInfo->threshX_Low);

    switch(ifcfInfo->allowedMeasBandwidth) {
      case LTE_AllowedMeasBandwidth_mbw6:
        LOG_I(RRC,"   AllowedMeasBandwidth : 6\n");
        break;

      case LTE_AllowedMeasBandwidth_mbw15:
        LOG_I(RRC,"   AllowedMeasBandwidth : 15\n");
        break;

      case LTE_AllowedMeasBandwidth_mbw25:
        LOG_I(RRC,"   AllowedMeasBandwidth : 25\n");
        break;

      case LTE_AllowedMeasBandwidth_mbw50:
        LOG_I(RRC,"   AllowedMeasBandwidth : 50\n");
        break;

      case LTE_AllowedMeasBandwidth_mbw75:
        LOG_I(RRC,"   AllowedMeasBandwidth : 75\n");
        break;

      case LTE_AllowedMeasBandwidth_mbw100:
        LOG_I(RRC,"   AllowedMeasBandwidth : 100\n");
        break;
    }

    if (ifcfInfo->presenceAntennaPort1)
      LOG_I(RRC,"   PresenceAntennaPort1 : True\n");
    else
      LOG_I(RRC,"   PresenceAntennaPort1 : False\n");

    if (ifcfInfo->cellReselectionPriority) {
      LOG_I(RRC,"   CellReselectionPriority : %ld\n",
            *ifcfInfo->cellReselectionPriority);
    }

    LOG_I(RRC,"   NeighCellConfig  : ");

    for (j=0; j<ifcfInfo->neighCellConfig.size; j++) {
      printf("%2x ",ifcfInfo->neighCellConfig.buf[j]);
    }

    printf("\n");

    if (ifcfInfo->q_OffsetFreq)
      LOG_I(RRC,"   Q_OffsetFreq : %d\n",Qoffsettab[*ifcfInfo->q_OffsetFreq]);

    if (ifcfInfo->interFreqNeighCellList) {
      for (j=0; j<ifcfInfo->interFreqNeighCellList->list.count; j++) {
        LOG_I(RRC,"   Cell %d\n", j);
        LOG_I(RRC,"      PhysCellId : %ld\n",ifcfInfo->interFreqNeighCellList->list.array[j]->physCellId);
        LOG_I(RRC,"      Q_OffsetRange : %ld\n",ifcfInfo->interFreqNeighCellList->list.array[j]->q_OffsetCell);
      }
    }

    if (ifcfInfo->interFreqBlackCellList) {
      for (j=0; j<ifcfInfo->interFreqBlackCellList->list.count; j++) {
        LOG_I(RRC,"   Cell %d\n", j);
        LOG_I(RRC,"      PhysCellId start: %ld\n",ifcfInfo->interFreqBlackCellList->list.array[j]->start);

        if (ifcfInfo->interFreqBlackCellList->list.array[i]->range) {
          LOG_I(RRC,"      PhysCellId Range : %ld\n",*ifcfInfo->interFreqBlackCellList->list.array[j]->range);
        }
      }
    }

    if (ifcfInfo->ext1 && ifcfInfo->ext1->q_QualMin_r9)
      LOG_I(RRC,"   Q_QualMin_r9 : %ld\n",*ifcfInfo->ext1->q_QualMin_r9);

    if (ifcfInfo->ext1 && ifcfInfo->ext1->threshX_Q_r9) {
      LOG_I(RRC,"   threshX_HighQ_r9 : %ld\n",ifcfInfo->ext1->threshX_Q_r9->threshX_HighQ_r9);
      LOG_I(RRC,"   threshX_LowQ_r9: %ld\n",ifcfInfo->ext1->threshX_Q_r9->threshX_LowQ_r9);
    }
  }
}


void dump_sib13( LTE_SystemInformationBlockType13_r9_t *sib13 ) {
  LOG_I( RRC, "[UE] Dumping SIB13\n" );
  LOG_I( RRC, "[UE] dumping sib13 second time\n" );
  LOG_I( RRC, "[UE] NotificationRepetitionCoeff-r9 : %ld\n", sib13->notificationConfig_r9.notificationRepetitionCoeff_r9 );
  LOG_I( RRC, "[UE] NotificationOffset-r9 : %d\n", (int)sib13->notificationConfig_r9.notificationOffset_r9 );
  LOG_I( RRC, "[UE] NotificationSF-Index-r9 : %d\n", (int)sib13->notificationConfig_r9.notificationSF_Index_r9 );
}


//TTN - SIB18
//-----------------------------------------------------------------------------
void dump_sib18(LTE_SystemInformationBlockType18_r12_t *sib18) {
  LOG_I( RRC, "[UE] Dumping SIB18\n" );

  for (int i = 0; i < sib18->commConfig_r12->commRxPool_r12.list.count; i++) {
    LOG_I(RRC, " Contents of SIB18 %d/%d \n", i+1, sib18->commConfig_r12->commRxPool_r12.list.count);
    LOG_I(RRC, " SIB18 rxPool_sc_CP_Len: %ld \n", sib18->commConfig_r12->commRxPool_r12.list.array[i]->sc_CP_Len_r12);
    LOG_I(RRC, " SIB18 sc_Period_r12: %ld \n", sib18->commConfig_r12->commRxPool_r12.list.array[i]->sc_Period_r12);
    LOG_I(RRC, " SIB18 data_CP_Len_r12: %ld \n", sib18->commConfig_r12->commRxPool_r12.list.array[i]->data_CP_Len_r12);
    LOG_I(RRC, " SIB18 prb_Num_r12: %ld \n", sib18->commConfig_r12->commRxPool_r12.list.array[i]->sc_TF_ResourceConfig_r12.prb_Num_r12);
    LOG_I(RRC, " SIB18 prb_Start_r12: %ld \n", sib18->commConfig_r12->commRxPool_r12.list.array[i]->sc_TF_ResourceConfig_r12.prb_Start_r12);
    LOG_I(RRC, " SIB18 prb_End_r12: %ld \n", sib18->commConfig_r12->commRxPool_r12.list.array[i]->sc_TF_ResourceConfig_r12.prb_End_r12);
    //to add more log
  }
}

//TTN -  SIB19
//-----------------------------------------------------------------------------
void dump_sib19(LTE_SystemInformationBlockType19_r12_t *sib19) {
  LOG_I( RRC, "[UE] Dumping SIB19\n" );

  for (int i = 0; i < sib19->discConfig_r12->discRxPool_r12.list.count; i++) {
    LOG_I(RRC, " Contents of SIB19 %d/%d \n", i+1, sib19->discConfig_r12->discRxPool_r12.list.count);
    LOG_I(RRC, " SIB19 cp_Len_r12: %ld \n", sib19->discConfig_r12->discRxPool_r12.list.array[i]->cp_Len_r12);
    LOG_I(RRC, " SIB19 discPeriod_r12: %ld \n", sib19->discConfig_r12->discRxPool_r12.list.array[i]->discPeriod_r12);
    LOG_I(RRC, " SIB19 numRetx_r12: %ld \n", sib19->discConfig_r12->discRxPool_r12.list.array[i]->numRetx_r12);
    LOG_I(RRC, " SIB19 numRepetition_r12: %ld \n", sib19->discConfig_r12->discRxPool_r12.list.array[i]->numRepetition_r12);
    LOG_I(RRC, " SIB19 prb_Num_r12: %ld \n", sib19->discConfig_r12->discRxPool_r12.list.array[i]->tf_ResourceConfig_r12.prb_Num_r12);
    LOG_I(RRC, " SIB19 prb_Start_r12: %ld \n", sib19->discConfig_r12->discRxPool_r12.list.array[i]->tf_ResourceConfig_r12.prb_Start_r12);
    LOG_I(RRC, " SIB19 prb_End_r12: %ld \n", sib19->discConfig_r12->discRxPool_r12.list.array[i]->tf_ResourceConfig_r12.prb_End_r12);
    //to add more log
  }
}

void dump_sib21(LTE_SystemInformationBlockType21_r14_t *sib21) {
  if ((sib21->sl_V2X_ConfigCommon_r14 != NULL) && (sib21->sl_V2X_ConfigCommon_r14->v2x_CommRxPool_r14 !=NULL) ) {
    for (int i = 0; i < sib21->sl_V2X_ConfigCommon_r14->v2x_CommRxPool_r14->list.count; i++) {
      LOG_I(RRC, " Contents of SIB21 %d/%d \n", i+1, sib21->sl_V2X_ConfigCommon_r14->v2x_CommRxPool_r14->list.count);
      LOG_I(RRC, " SIB21 sl_Subframe_r14: %d \n", sib21->sl_V2X_ConfigCommon_r14->v2x_CommRxPool_r14->list.array[i]->sl_Subframe_r14.present);
      LOG_I(RRC, " SIB21 adjacencyPSCCH_PSSCH_r14: %d \n", sib21->sl_V2X_ConfigCommon_r14->v2x_CommRxPool_r14->list.array[i]->adjacencyPSCCH_PSSCH_r14);
      LOG_I(RRC, " SIB21 sizeSubchannel_r14: %ld \n", sib21->sl_V2X_ConfigCommon_r14->v2x_CommRxPool_r14->list.array[i]->sizeSubchannel_r14);
      LOG_I(RRC, " SIB21 numSubchannel_r14: %ld \n", sib21->sl_V2X_ConfigCommon_r14->v2x_CommRxPool_r14->list.array[i]->numSubchannel_r14);
      LOG_I(RRC, " SIB21 startRB_Subchannel_r14: %ld \n", sib21->sl_V2X_ConfigCommon_r14->v2x_CommRxPool_r14->list.array[i]->startRB_Subchannel_r14);
      //to add more log
    }
  }
}

//-----------------------------------------------------------------------------
int decode_SI_MBMS( const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index ) {
  return 0;
}


//-----------------------------------------------------------------------------
int decode_SI( const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index ) {
  LTE_SystemInformation_t **si = &UE_rrc_inst[ctxt_pP->module_id].si[eNB_index];
  int new_sib = 0;
  LTE_SystemInformationBlockType1_t *sib1 = UE_rrc_inst[ctxt_pP->module_id].sib1[eNB_index];
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_UE_DECODE_SI, VCD_FUNCTION_IN );

  // Dump contents
  if ((*si)->criticalExtensions.present == LTE_SystemInformation__criticalExtensions_PR_systemInformation_r8 ||
      (*si)->criticalExtensions.present == LTE_SystemInformation__criticalExtensions_PR_criticalExtensionsFuture_r15) {
    LOG_D( RRC, "[UE] (*si)->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count %d\n",
           (*si)->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count );
  } else {
    LOG_D( RRC, "[UE] Unknown criticalExtension version (not Rel8)\n" );
    return -1;
  }

  for (int i=0; i<(*si)->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count; i++) {
    struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member *typeandinfo;
    typeandinfo = (*si)->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.array[i];

    switch(typeandinfo->present) {
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib2:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&2) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=2;
          //new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib2[eNB_index], &typeandinfo->choice.sib2, sizeof(LTE_SystemInformationBlockType2_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB2 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
          dump_sib2( UE_rrc_inst[ctxt_pP->module_id].sib2[eNB_index] );
          LOG_I( RRC, "[FRAME %05"PRIu32"][RRC_UE][MOD %02"PRIu8"][][--- MAC_CONFIG_REQ (SIB2 params  eNB %"PRIu8") --->][MAC_UE][MOD %02"PRIu8"][]\n",
                 ctxt_pP->frame, ctxt_pP->module_id, eNB_index, ctxt_pP->module_id );
          rrc_mac_config_req_ue(ctxt_pP->module_id, 0, eNB_index,
                                &UE_rrc_inst[ctxt_pP->module_id].sib2[eNB_index]->radioResourceConfigCommon,
                                (struct LTE_PhysicalConfigDedicated *)NULL,
                                (LTE_SCellToAddMod_r10_t *)NULL,
                                (LTE_MeasObjectToAddMod_t **)NULL,
                                (LTE_MAC_MainConfig_t *)NULL,
                                0,
                                (struct LTE_LogicalChannelConfig *)NULL,
                                (LTE_MeasGapConfig_t *)NULL,
                                (LTE_TDD_Config_t *)NULL,
                                (LTE_MobilityControlInfo_t *)NULL,
                                NULL,
                                NULL,
                                UE_rrc_inst[ctxt_pP->module_id].sib2[eNB_index]->freqInfo.ul_CarrierFreq,
                                UE_rrc_inst[ctxt_pP->module_id].sib2[eNB_index]->freqInfo.ul_Bandwidth,
                                &UE_rrc_inst[ctxt_pP->module_id].sib2[eNB_index]->freqInfo.additionalSpectrumEmission,
                                UE_rrc_inst[ctxt_pP->module_id].sib2[eNB_index]->mbsfn_SubframeConfigList,0,
                                (LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                                (LTE_PMCH_InfoList_r9_t *)NULL,
                                0,
                                NULL,
                                NULL,
                                0,
                                (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                                (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                               );

          // After SI is received, prepare RRCConnectionRequest
          if (UE_rrc_inst[ctxt_pP->module_id].MBMS_flag < 3) // see -Q option
            if (EPC_MODE_ENABLED) {
              rrc_ue_generate_RRCConnectionRequest( ctxt_pP, eNB_index );
            }

          if (UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].State == RRC_IDLE) {
            LOG_I( RRC, "[UE %d] Received SIB1/SIB2/SIB3 Switching to RRC_SI_RECEIVED\n", ctxt_pP->module_id );
            UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].State = RRC_SI_RECEIVED;
#if ENABLE_RAL
            {
              MessageDef                            *message_ral_p = NULL;
              rrc_ral_system_information_ind_t       ral_si_ind;
              message_ral_p = itti_alloc_new_message (TASK_RRC_UE, 0, RRC_RAL_SYSTEM_INFORMATION_IND);
              memset(&ral_si_ind, 0, sizeof(rrc_ral_system_information_ind_t));
              ral_si_ind.plmn_id.MCCdigit2 = '0';
              ral_si_ind.plmn_id.MCCdigit1 = '2';
              ral_si_ind.plmn_id.MNCdigit3 = '0';
              ral_si_ind.plmn_id.MCCdigit3 = '8';
              ral_si_ind.plmn_id.MNCdigit2 = '9';
              ral_si_ind.plmn_id.MNCdigit1 = '9';
              ral_si_ind.cell_id        = 1;
              ral_si_ind.dbm            = 0;
              //ral_si_ind.dbm            = fifo_dump_emos_UE.PHY_measurements->rx_rssi_dBm[eNB_index];
              // TO DO
              ral_si_ind.sinr           = 0;
              //ral_si_ind.sinr           = fifo_dump_emos_UE.PHY_measurements->subband_cqi_dB[eNB_index][phy_vars_ue->lte_frame_parms.nb_antennas_rx][0];
              // TO DO
              ral_si_ind.link_data_rate = 0;
              memcpy (&message_ral_p->ittiMsg, (void *) &ral_si_ind, sizeof(rrc_ral_system_information_ind_t));
#warning "ue_mod_idP ? for instance ?"
              itti_send_msg_to_task (TASK_RAL_UE, UE_MODULE_ID_TO_INSTANCE(ctxt_pP->module_id), message_ral_p);
            }
#endif
          }
        }else{
                       //LOG_W( RRC, "[UE %d] Received new SIB1/SIB2/SIB3 with MBMSs %d\n", ctxt_pP->module_id, ((&typeandinfo->choice.sib2)->mbsfn_SubframeConfigList == NULL ? 0:1) );
               if((&typeandinfo->choice.sib2)->mbsfn_SubframeConfigList != NULL && (UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&4096) == 0){
                       LOG_W( RRC, "[UE %d] Received SIB2 with MBSFN SF Config\n", ctxt_pP->module_id );

                       memcpy( UE_rrc_inst[ctxt_pP->module_id].sib2[eNB_index], &typeandinfo->choice.sib2, sizeof(LTE_SystemInformationBlockType2_t) );
                       LOG_I( RRC, "[FRAME %05"PRIu32"][RRC_UE][MOD %02"PRIu8"][][--- MAC_CONFIG_REQ (SIB2 params  eNB %"PRIu8") --->][MAC_UE][MOD %02"PRIu8"][]\n",
                               ctxt_pP->frame, ctxt_pP->module_id, eNB_index, ctxt_pP->module_id );
                       rrc_mac_config_req_ue(ctxt_pP->module_id, 0, eNB_index,
                               (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                                (struct LTE_PhysicalConfigDedicated *)NULL,
                                (LTE_SCellToAddMod_r10_t *)NULL,
                                (LTE_MeasObjectToAddMod_t **)NULL,
                                (LTE_MAC_MainConfig_t *)NULL,
                                0,
                                (struct LTE_LogicalChannelConfig *)NULL,
                                (LTE_MeasGapConfig_t *)NULL,
                                (LTE_TDD_Config_t *)NULL,
                                (LTE_MobilityControlInfo_t *)NULL,
                                NULL,
                                NULL,
                               NULL,
                               NULL,
                               NULL,
                                UE_rrc_inst[ctxt_pP->module_id].sib2[eNB_index]->mbsfn_SubframeConfigList
                                ,0,
                                (LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                                (LTE_PMCH_InfoList_r9_t *)NULL,
#ifdef CBA
                                0,0,
#endif
                                0,
                                NULL,
                                NULL,
                        0,
                        (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                        (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                               );

               }
       }

        break; // case SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib2

      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib3:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&4) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=4;
          new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib3[eNB_index], &typeandinfo->choice.sib3, sizeof(LTE_SystemInformationBlockType3_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB3 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
          dump_sib3( UE_rrc_inst[ctxt_pP->module_id].sib3[eNB_index] );
        }

        break;

      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib4:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&8) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=8;
          new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib4[eNB_index], &typeandinfo->choice.sib4, sizeof(LTE_SystemInformationBlockType4_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB4 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
        }

        break;

      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib5:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&16) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=16;
          new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib5[eNB_index], &typeandinfo->choice.sib5, sizeof(LTE_SystemInformationBlockType5_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB5 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
          dump_sib5(UE_rrc_inst[ctxt_pP->module_id].sib5[eNB_index]);
        }

        break;

      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib6:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&32) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=32;
          new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib6[eNB_index], &typeandinfo->choice.sib6, sizeof(LTE_SystemInformationBlockType6_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB6 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
        }

        break;

      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib7:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&64) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=64;
          new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib7[eNB_index], &typeandinfo->choice.sib7, sizeof(LTE_SystemInformationBlockType7_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB7 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
        }

        break;

      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib8:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&128) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=128;
          new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib8[eNB_index], &typeandinfo->choice.sib8, sizeof(LTE_SystemInformationBlockType8_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB8 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
        }

        break;

      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib9:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&256) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=256;
          new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib9[eNB_index], &typeandinfo->choice.sib9, sizeof(LTE_SystemInformationBlockType9_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB9 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
        }

        break;

      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib10:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&512) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=512;
          new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib10[eNB_index], &typeandinfo->choice.sib10, sizeof(LTE_SystemInformationBlockType10_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB10 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
        }

        break;

      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib11:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&1024) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=1024;
          new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib11[eNB_index], &typeandinfo->choice.sib11, sizeof(LTE_SystemInformationBlockType11_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB11 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
        }

        break;

      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib12_v920:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&2048) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=2048;
          new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib12[eNB_index], &typeandinfo->choice.sib12_v920, sizeof(LTE_SystemInformationBlockType12_r9_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB12 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
        }

        break;

      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib13_v920:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&4096) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=4096;
          new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib13[eNB_index], &typeandinfo->choice.sib13_v920, sizeof(LTE_SystemInformationBlockType13_r9_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB13 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
          dump_sib13( UE_rrc_inst[ctxt_pP->module_id].sib13[eNB_index] );
          // adding here function to store necessary parameters for using in decode_MCCH_Message + maybe transfer to PHY layer
          LOG_I( RRC, "[FRAME %05"PRIu32"][RRC_UE][MOD %02"PRIu8"][][--- MAC_CONFIG_REQ (SIB13 params eNB %"PRIu8") --->][MAC_UE][MOD %02"PRIu8"][]\n",
                 ctxt_pP->frame, ctxt_pP->module_id, eNB_index, ctxt_pP->module_id);
          rrc_mac_config_req_ue(ctxt_pP->module_id,0,eNB_index,
                                (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                                (struct LTE_PhysicalConfigDedicated *)NULL,
                                (LTE_SCellToAddMod_r10_t *)NULL,
                                (LTE_MeasObjectToAddMod_t **)NULL,
                                (LTE_MAC_MainConfig_t *)NULL,
                                0,
                                (struct LTE_LogicalChannelConfig *)NULL,
                                (LTE_MeasGapConfig_t *)NULL,
                                (LTE_TDD_Config_t *)NULL,
                                (LTE_MobilityControlInfo_t *)NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                (LTE_MBSFN_SubframeConfigList_t *)NULL,
                                0,
                                &UE_rrc_inst[ctxt_pP->module_id].sib13[eNB_index]->mbsfn_AreaInfoList_r9,
                                (LTE_PMCH_InfoList_r9_t *)NULL,
                                0,
                                NULL,
                                NULL,
                                0,
                                (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                                (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                               );
        }
        break;

      //SIB18
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib18_v1250:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&8192) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=8192;
          new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib18[eNB_index], &typeandinfo->choice.sib18_v1250, sizeof(LTE_SystemInformationBlockType18_r12_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB18 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
          dump_sib18( UE_rrc_inst[ctxt_pP->module_id].sib18[eNB_index] );
          // adding here function to store necessary parameters to transfer to PHY layer
          LOG_I( RRC, "[FRAME %05"PRIu32"][RRC_UE][MOD %02"PRIu8"][][--- MAC_CONFIG_REQ (SIB18 params eNB %"PRIu8") --->][MAC_UE][MOD %02"PRIu8"][]\n",
                 ctxt_pP->frame, ctxt_pP->module_id, eNB_index, ctxt_pP->module_id);
          //process SIB18 to transfer SL-related parameters to PHY
          rrc_ue_process_sidelink_radioResourceConfig(ctxt_pP->module_id,eNB_index,
              UE_rrc_inst[ctxt_pP->module_id].sib18[eNB_index],
              (LTE_SystemInformationBlockType19_r12_t *)NULL,
              (LTE_SL_CommConfig_r12_t *)NULL,
              (LTE_SL_DiscConfig_r12_t *)NULL
                                                     );
        }

        break;

      //SIB19
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib19_v1250:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&16384) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=16384;
          new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib19[eNB_index], &typeandinfo->choice.sib19_v1250, sizeof(LTE_SystemInformationBlockType19_r12_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB19 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
          dump_sib19( UE_rrc_inst[ctxt_pP->module_id].sib19[eNB_index] );
          // adding here function to store necessary parameters to transfer to PHY layer
          LOG_I( RRC, "[FRAME %05"PRIu32"][RRC_UE][MOD %02"PRIu8"][][--- MAC_CONFIG_REQ (SIB19 params eNB %"PRIu8") --->][MAC_UE][MOD %02"PRIu8"][]\n",
                 ctxt_pP->frame, ctxt_pP->module_id, eNB_index, ctxt_pP->module_id);
          //process SIB19 to transfer SL-related parameters to PHY
          rrc_ue_process_sidelink_radioResourceConfig(ctxt_pP->module_id,eNB_index,
              (LTE_SystemInformationBlockType18_r12_t *)NULL,
              UE_rrc_inst[ctxt_pP->module_id].sib19[eNB_index],
              (LTE_SL_CommConfig_r12_t *)NULL,
              (LTE_SL_DiscConfig_r12_t *)NULL
                                                     );
        }

        break;

      //SIB21
      case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib21_v1430:
        if ((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&32768) == 0) {
          UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus|=32768;
          new_sib=1;
          memcpy( UE_rrc_inst[ctxt_pP->module_id].sib21[eNB_index], &typeandinfo->choice.sib21_v1430, sizeof(LTE_SystemInformationBlockType21_r14_t) );
          LOG_I( RRC, "[UE %"PRIu8"] Frame %"PRIu32" Found SIB21 from eNB %"PRIu8"\n", ctxt_pP->module_id, ctxt_pP->frame, eNB_index );
          dump_sib21( UE_rrc_inst[ctxt_pP->module_id].sib21[eNB_index] );
          // adding here function to store necessary parameters to transfer to PHY layer
          LOG_I( RRC, "[FRAME %05"PRIu32"][RRC_UE][MOD %02"PRIu8"][][--- MAC_CONFIG_REQ (SIB21 params eNB %"PRIu8") --->][MAC_UE][MOD %02"PRIu8"][]\n",
                 ctxt_pP->frame, ctxt_pP->module_id, eNB_index, ctxt_pP->module_id);
          //process SIB21
          //TODO
        }

        break;

      default:
        break;
    }
    if (new_sib == 1) {
      UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIcnt++;
  
      if (UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIcnt == sib1->schedulingInfoList.list.count)
        rrc_set_sub_state( ctxt_pP->module_id, RRC_SUB_STATE_IDLE_SIB_COMPLETE );
  
      LOG_I(RRC,"SIStatus %x, SIcnt %d/%d\n",
            UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus,
            UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIcnt,
            sib1->schedulingInfoList.list.count);
    }
  }

  //if (new_sib == 1) {
  //  UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIcnt++;

  //  if (UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIcnt == sib1->schedulingInfoList.list.count)
  //    rrc_set_sub_state( ctxt_pP->module_id, RRC_SUB_STATE_IDLE_SIB_COMPLETE );

  //  LOG_I(RRC,"SIStatus %x, SIcnt %d/%d\n",
  //        UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus,
  //        UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIcnt,
  //        sib1->schedulingInfoList.list.count);
  //}

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_UE_DECODE_SI, VCD_FUNCTION_OUT);
  return 0;
}

// layer 3 filtering of RSRP (EUTRA) measurements: 36.331, Sec. 5.5.3.2
//-----------------------------------------------------------------------------
void ue_meas_filtering( const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index ) {
  float a  = UE_rrc_inst[ctxt_pP->module_id].filter_coeff_rsrp; // 'a' in 36.331 Sec. 5.5.3.2
  float a1 = UE_rrc_inst[ctxt_pP->module_id].filter_coeff_rsrq;
  //float rsrp_db, rsrq_db;
  uint8_t    eNB_offset;

  if(UE_rrc_inst[ctxt_pP->module_id].QuantityConfig[0] != NULL) { // Only consider 1 serving cell (index: 0)
    if (UE_rrc_inst[ctxt_pP->module_id].QuantityConfig[0]->quantityConfigEUTRA != NULL) {
      if(UE_rrc_inst[ctxt_pP->module_id].QuantityConfig[0]->quantityConfigEUTRA->filterCoefficientRSRP != NULL) {
        for (eNB_offset = 0; eNB_offset<1+get_n_adj_cells(ctxt_pP->module_id,0); eNB_offset++) {
          UE_rrc_inst[ctxt_pP->module_id].rsrp_db[eNB_offset] = get_RSRP(ctxt_pP->module_id,0,eNB_offset);
          /*
          (dB_fixed_times10(get_RSRP(ctxt_pP->module_id,0,eNB_offset))/10.0) -
          get_rx_total_gain_dB(ctxt_pP->module_id,0) -
          get_bw_gain_dB(ctxt_pP->module_id);
          */
          UE_rrc_inst[ctxt_pP->module_id].rsrp_db_filtered[eNB_offset] =
            (1.0-a)*UE_rrc_inst[ctxt_pP->module_id].rsrp_db_filtered[eNB_offset] +
            a*UE_rrc_inst[ctxt_pP->module_id].rsrp_db[eNB_offset];
          LOG_D(RRC,"RSRP_dBm: %3.2f \n",get_RSRP(ctxt_pP->module_id,0,eNB_offset));;
          /*          LOG_D(RRC,"gain_loss_dB: %d \n",get_rx_total_gain_dB(ctxt_pP->module_id,0));
                LOG_D(RRC,"gain_fixed_dB: %d \n",dB_fixed(frame_parms->N_RB_DL*12));*/
          LOG_D(PHY,"[UE %d] Frame %d, RRC Measurements => rssi %3.1f dBm (digital: %3.1f dB)\n",
                ctxt_pP->module_id,
                ctxt_pP->frame,
                10*log10(get_RSSI(ctxt_pP->module_id,0))-get_rx_total_gain_dB(ctxt_pP->module_id,0),
                10*log10(get_RSSI(ctxt_pP->module_id,0)));
          LOG_D(RRC,"[UE %d] Frame %d: Meas RSRP: eNB_offset: %d rsrp_coef: %3.1f filter_coef: %ld before L3 filtering: rsrp: %3.1f after L3 filtering: rsrp: %3.1f \n ",
                ctxt_pP->module_id,
                ctxt_pP->frame, eNB_offset,a,
                *UE_rrc_inst->QuantityConfig[0]->quantityConfigEUTRA->filterCoefficientRSRP,
                UE_rrc_inst[ctxt_pP->module_id].rsrp_db[eNB_offset],
                UE_rrc_inst[ctxt_pP->module_id].rsrp_db_filtered[eNB_offset]);
        }
      }
    } else {
      for (eNB_offset = 0; eNB_offset<1+get_n_adj_cells(ctxt_pP->module_id,0); eNB_offset++) {
        UE_rrc_inst[ctxt_pP->module_id].rsrp_db_filtered[eNB_offset]= get_RSRP(ctxt_pP->module_id,0,eNB_offset);
      }
    }

    if (UE_rrc_inst[ctxt_pP->module_id].QuantityConfig[0]->quantityConfigEUTRA != NULL) {
      if(UE_rrc_inst[ctxt_pP->module_id].QuantityConfig[0]->quantityConfigEUTRA->filterCoefficientRSRQ != NULL) {
        for (eNB_offset = 0; eNB_offset<1+get_n_adj_cells(ctxt_pP->module_id,0); eNB_offset++) {
          UE_rrc_inst[ctxt_pP->module_id].rsrq_db[eNB_offset] = (10*log10(get_RSRQ(ctxt_pP->module_id,0,eNB_offset)))-20;
          UE_rrc_inst[ctxt_pP->module_id].rsrq_db_filtered[eNB_offset]=(1-a1)*UE_rrc_inst[ctxt_pP->module_id].rsrq_db_filtered[eNB_offset] +
              a1 *UE_rrc_inst[ctxt_pP->module_id].rsrq_db[eNB_offset];
        }
      }
    } else {
      for (eNB_offset = 0; eNB_offset<1+get_n_adj_cells(ctxt_pP->module_id,0); eNB_offset++) {
        UE_rrc_inst[ctxt_pP->module_id].rsrq_db_filtered[eNB_offset]= get_RSRQ(ctxt_pP->module_id,0,eNB_offset);
      }
    }
  }
}
//Below routine implements Measurement Reporting procedure from 36.331 Section 5.5.5
//-----------------------------------------------------------------------------
void rrc_ue_generate_nrMeasurementReport(protocol_ctxt_t *const ctxt_pP, uint8_t eNB_index ) {
  uint8_t buffer[RRC_BUF_SIZE];
  UE_RRC_INST *ue = &UE_rrc_inst[ctxt_pP->module_id];
  uint8_t target_eNB_offset = ue->Info[0].handoverTarget;
  LTE_PhysCellId_t targetCellId = ue->HandoverInfoUe.targetCellId;

  for (int i = 0; i < MAX_MEAS_ID; i++) {
    if (ue->measReportList[eNB_index][i] != NULL) {
      LTE_MeasId_t measId = ue->measReportList[eNB_index][i]->measId;
      long rsrp_s = binary_search_float(RSRP_meas_mapping, 98, ue->rsrp_db_filtered[eNB_index]);
      long rsrq_s = binary_search_float(RSRQ_meas_mapping, 35, ue->rsrq_db_filtered[eNB_index]);
      long rsrp_tar = binary_search_float(RSRP_meas_mapping, 98, ue->rsrp_db_filtered[target_eNB_offset]);
      long rsrq_tar = binary_search_float(RSRQ_meas_mapping, 35, ue->rsrq_db_filtered[target_eNB_offset]);

      LOG_I(RRC,"[UE %d] Frame %d: source eNB: %d target eNB: %d servingCell(%d) targetCell(%ld)\n",
            ctxt_pP->module_id,
            ctxt_pP->frame,
            eNB_index,
            target_eNB_offset,
            get_adjacent_cell_id(ctxt_pP->module_id, eNB_index),
            targetCellId);

      if (ctxt_pP->frame != 0) {
        LOG_I(RRC, "measId %ld, targetCellId %ld, rsrp_s %ld, rsrq_s %ld, rsrp_t %ld, rsrq_t %ld\n",
                    measId, targetCellId, rsrp_s, rsrq_s, rsrp_tar, rsrq_tar);
        ssize_t size = do_nrMeasurementReport(buffer, sizeof(buffer), measId, targetCellId, rsrp_s, rsrq_s, rsrp_tar, rsrq_tar);
        AssertFatal(size >= 0, "do_nrMeasurementReport failed \n");
        LOG_I(RRC, "[UE %d] Frame %d : Generating Measurement Report for eNB %d\n",
              ctxt_pP->module_id, ctxt_pP->frame, eNB_index);
        int result = pdcp_data_req(ctxt_pP,  SRB_FLAG_YES, DCCH, rrc_mui++, 0, size, buffer, PDCP_TRANSMISSION_MODE_DATA,NULL, NULL);
        AssertFatal (result == TRUE, "PDCP data request failed!\n");
      }
    }
  }
}

//Below routine implements Measurement Reporting procedure from 36.331 Section 5.5.5
//-----------------------------------------------------------------------------
void rrc_ue_generate_MeasurementReport(protocol_ctxt_t *const ctxt_pP, uint8_t eNB_index ) {
  uint8_t             buffer[RRC_BUF_SIZE];
  uint8_t             target_eNB_offset;
  LTE_MeasId_t        measId;
  LTE_PhysCellId_t    targetCellId;
  long             rsrp_t,rsrq_t;
  long             rsrp_s,rsrq_s;
  long             nElem, nElem1;
  float            rsrp_filtered, rsrq_filtered;
  static frame_t   pframe=0;
  int              result;
  nElem = 98;
  nElem1 = 35;
  target_eNB_offset = UE_rrc_inst[ctxt_pP->module_id].Info[0].handoverTarget; // eNB_offset of target eNB: used to obtain the mod_id of target eNB

  for (int i = 0; i < MAX_MEAS_ID; i++) {
    if (UE_rrc_inst[ctxt_pP->module_id].measReportList[0][i] != NULL) {
      measId = UE_rrc_inst[ctxt_pP->module_id].measReportList[0][i]->measId;
      // Note: Values in the meas report have to be the mapped values...to implement binary search for LUT
      rsrp_filtered = UE_rrc_inst[ctxt_pP->module_id].rsrp_db_filtered[eNB_index];//nid_cell];
      rsrp_s = binary_search_float(RSRP_meas_mapping,nElem, rsrp_filtered);
      rsrq_filtered = UE_rrc_inst[ctxt_pP->module_id].rsrq_db_filtered[eNB_index];//nid_cell]; //RSRQ of serving cell
      rsrq_s = binary_search_float(RSRQ_meas_mapping,nElem1,rsrq_filtered);//mapped RSRQ of serving cell
      LOG_I(RRC,"[UE %d] Frame %d: source eNB %d :rsrp_s: %ld rsrq_s: %ld rsrp_filtered: %f rsrq_filtered: %f \n",
            ctxt_pP->module_id,
            ctxt_pP->frame,
            eNB_index,
            rsrp_s,
            rsrq_s,
            rsrp_filtered,
            rsrq_filtered);
      rsrp_t = binary_search_float(RSRP_meas_mapping,nElem,UE_rrc_inst[ctxt_pP->module_id].rsrp_db_filtered[target_eNB_offset]); //RSRP of target cell
      rsrq_t = binary_search_float(RSRQ_meas_mapping,nElem1,UE_rrc_inst[ctxt_pP->module_id].rsrq_db_filtered[target_eNB_offset]); //RSRQ of target cell
      LOG_I(RRC,"[UE %d] Frame %d: target eNB %d :rsrp_t: %ld rsrq_t: %ld rsrp_filtered: %f rsrq_filtered: %f \n",
            ctxt_pP->module_id,
            ctxt_pP->frame,
            target_eNB_offset,
            rsrp_t,
            rsrq_t,
            UE_rrc_inst[ctxt_pP->module_id].rsrp_db_filtered[target_eNB_offset],
            UE_rrc_inst[ctxt_pP->module_id].rsrq_db_filtered[target_eNB_offset]);
      //  if (measFlag == 1) {
      targetCellId = UE_rrc_inst[ctxt_pP->module_id].HandoverInfoUe.targetCellId ;//get_adjacent_cell_id(ue_mod_idP,target_eNB_offset); //PhycellId of target cell

      if (pframe!=ctxt_pP->frame) {
        pframe=ctxt_pP->frame;
        LOG_I(RRC, "[UE %d] Frame %ld: doing MeasReport: servingCell(%ld) targetCell(%ld) rsrp_s(%ld) rsrq_s(%ld) rsrp_t(%ld) rsrq_t(%ld) \n",
              ctxt_pP->module_id,
              (long int)ctxt_pP->frame,
              (long int)get_adjacent_cell_id(ctxt_pP->module_id, eNB_index),
              (long int)targetCellId,
              (long int)rsrp_s,
              (long int)rsrq_s,
              (long int)rsrp_t,
              (long int)rsrq_t);
        ssize_t size = do_MeasurementReport(ctxt_pP->module_id, buffer, sizeof(buffer),
                                            measId, targetCellId, rsrp_s, rsrq_s, rsrp_t, rsrq_t);
        AssertFatal(size >= 0, "do_MeasurementReport failed \n");
        LOG_I(RRC, "[UE %d] Frame %d : Generating Measurement Report for eNB %d. Size is %zu\n",
              ctxt_pP->module_id, ctxt_pP->frame, eNB_index, size);
        result = pdcp_data_req(ctxt_pP,  SRB_FLAG_YES, DCCH, rrc_mui++, 0, size, buffer, PDCP_TRANSMISSION_MODE_DATA,NULL, NULL);
        AssertFatal (result == TRUE, "PDCP data request failed!\n");
        //LOG_D(RRC, "[UE %d] Frame %d Sending MeasReport (%d bytes) through DCCH%d to PDCP \n",ue_mod_idP,frameP, size, DCCH);
      }

      //          measFlag = 0; //re-setting measFlag so that no more MeasReports are sent in this frameP
      //          }
    }
  }
}

static bool does_rrcConnReconfig_have_nr(const UE_RRC_INST *ue) {
  for (int i = 0; i < NB_CNX_UE; i++) {
    for (int j = 0; j < MAX_MEAS_ID; j++) {
      LTE_ReportConfigId_t reportConfigId = ue->MeasId[i][j]->reportConfigId;
      AssertFatal(reportConfigId >= 1 && reportConfigId <= MAX_MEAS_CONFIG, "Bad index\n");
      const LTE_ReportConfigToAddMod_t *rc = ue->ReportConfig[i][reportConfigId-1];
      if (rc == NULL) {
        LOG_D(RRC, "UE_rrc_inst[ctxt_pP->module_id]->ReportConfig[%d][%ld] = NULL\n", i, reportConfigId-1);
        continue;
      }
      if (rc->reportConfig.present != LTE_ReportConfigToAddMod__reportConfig_PR_reportConfigInterRAT) {
        LOG_D(RRC, "reportConfig.present = %d, not LTE_ReportConfigToAddMod__reportConfig_PR_reportConfigInterRAT\n",
              rc->reportConfig.present);
        continue;
      }
      LTE_ReportConfigInterRAT_t irat = rc->reportConfig.choice.reportConfigInterRAT;
      if (irat.triggerType.present != LTE_ReportConfigInterRAT__triggerType_PR_event) {
        LOG_D(RRC, "irat.triggerType.present = %d, not LTE_ReportConfigInterRAT__triggerType_PR_event\n",
              irat.triggerType.present);
        continue;
      }
      if (irat.triggerType.choice.event.eventId.present != LTE_ReportConfigInterRAT__triggerType__event__eventId_PR_eventB1_NR_r15) {
        LOG_D(RRC, "irat.triggerType.choice.event.eventId.present = %d\n",
              irat.triggerType.choice.event.eventId.present);
        continue;
      }
      return true;
    }
  }
  return false;
}


// Measurement report triggering, described in 36.331 Section 5.5.4.1: called periodically
//-----------------------------------------------------------------------------
void ue_measurement_report_triggering(protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index ) {
  uint8_t               i,j;
  LTE_Hysteresis_t     hys;
  LTE_TimeToTrigger_t  ttt_ms;
  LTE_Q_OffsetRange_t  ofn;
  LTE_Q_OffsetRange_t  ocn;
  LTE_Q_OffsetRange_t  ofs = 0;
  LTE_Q_OffsetRange_t  ocs = 0;
  long             a3_offset;
  LTE_MeasObjectId_t   measObjId;
  LTE_ReportConfigId_t reportConfigId;
  UE_RRC_INST *ue = &UE_rrc_inst[ctxt_pP->module_id];
  for(i=0 ; i<NB_CNX_UE ; i++) {
    for(j=0 ; j<MAX_MEAS_ID ; j++) {
      if(ue->MeasId[i][j] != NULL) {
        measObjId = ue->MeasId[i][j]->measObjectId;
        reportConfigId = ue->MeasId[i][j]->reportConfigId;

        if(ue->MeasObj[i][measObjId-1] != NULL) {
          if(ue->MeasObj[i][measObjId-1]->measObject.present == LTE_MeasObjectToAddMod__measObject_PR_measObjectEUTRA) {
            /* consider any neighboring cell detected on the associated frequency to be
             * applicable when the concerned cell is not included in the blackCellsToAddModList
             * defined within the VarMeasConfig for this measId */
            //    LOG_I(RRC,"event %d %d %p \n", measObjId,reportConfigId, ue->ReportConfig[i][reportConfigId-1]);
            if((ue->ReportConfig[i][reportConfigId-1] != NULL) &&
                (ue->ReportConfig[i][reportConfigId-1]->reportConfig.present == LTE_ReportConfigToAddMod__reportConfig_PR_reportConfigEUTRA) &&
                (ue->ReportConfig[i][reportConfigId-1]->reportConfig.choice.reportConfigEUTRA.triggerType.present ==
                 LTE_ReportConfigEUTRA__triggerType_PR_event)) {
              hys = ue->ReportConfig[i][reportConfigId-1]->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.hysteresis;
              ttt_ms = timeToTrigger_ms[ue->ReportConfig[i][reportConfigId
                                        -1]->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.timeToTrigger];
              // Freq specific offset of neighbor cell freq
              ofn = 5;//((ue->MeasObj[i][measObjId-1]->measObject.choice.measObjectEUTRA.offsetFreq != NULL) ?
              // *ue->MeasObj[i][measObjId-1]->measObject.choice.measObjectEUTRA.offsetFreq : 15); //  /* 15 is the Default */
              // cellIndividualOffset of neighbor cell - not defined yet
              ocn = 0;
              a3_offset = ue->ReportConfig[i][reportConfigId-1]->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.choice.eventA3.a3_Offset;

              switch (ue->ReportConfig[i][reportConfigId-1]->reportConfig.choice.reportConfigEUTRA.triggerType.choice.event.eventId.present) {
                case LTE_ReportConfigEUTRA__triggerType__event__eventId_PR_eventA1:
                  LOG_D(RRC,"[UE %d] Frame %d : A1 event: check if serving becomes better than threshold\n",
                        ctxt_pP->module_id, ctxt_pP->frame);
                  break;

                case LTE_ReportConfigEUTRA__triggerType__event__eventId_PR_eventA2:
                  LOG_D(RRC,"[UE %d] Frame %d : A2 event, check if serving becomes worse than a threshold\n",
                        ctxt_pP->module_id, ctxt_pP->frame);
                  break;

                case LTE_ReportConfigEUTRA__triggerType__event__eventId_PR_eventA3:
                  LOG_D(RRC,"[UE %d] Frame %d : A3 event: check if a neighboring cell becomes offset better than serving to trigger a measurement event \n",
                        ctxt_pP->module_id, ctxt_pP->frame);

                  if ((check_trigger_meas_event(
                         ctxt_pP->module_id,
                         ctxt_pP->frame,
                         eNB_index,
                         i,j,ofn,ocn,hys,ofs,ocs,a3_offset,ttt_ms)) &&
                      (ue->Info[0].State >= RRC_CONNECTED) &&
                      (ue->Info[0].T304_active == 0 )      &&
                      (ue->HandoverInfoUe.measFlag == 1)) {
                    //trigger measurement reporting procedure (36.331, section 5.5.5)
                    if (ue->measReportList[i][j] == NULL) {
                      ue->measReportList[i][j] = malloc(sizeof(MEAS_REPORT_LIST));
                    }

                    ue->measReportList[i][j]->measId = ue->MeasId[i][j]->measId;
                    ue->measReportList[i][j]->numberOfReportsSent = 0;
                    rrc_ue_generate_MeasurementReport(
                      ctxt_pP,
                      eNB_index);
                    ue->HandoverInfoUe.measFlag = 1;
                  } else {
                    if(ue->measReportList[i][j] != NULL) {
                      free(ue->measReportList[i][j]);
                    }
                    ue->measReportList[i][j] = NULL;
                  }

                  break;

                case LTE_ReportConfigEUTRA__triggerType__event__eventId_PR_eventA4:
                  LOG_D(RRC,"[UE %d] Frame %d : received an A4 event, neighbor becomes offset better than a threshold\n",
                        ctxt_pP->module_id, ctxt_pP->frame);
                  break;

                case LTE_ReportConfigEUTRA__triggerType__event__eventId_PR_eventA5:
                  LOG_D(RRC,"[UE %d] Frame %d: received an A5 event, serving becomes worse than threshold 1 and neighbor becomes better than threshold 2\n",
                        ctxt_pP->module_id, ctxt_pP->frame);
                  break;

                default:
                  LOG_D(RRC,"Invalid ReportConfigEUTRA__triggerType__event__eventId: %d",
                        ue->ReportConfig[i][j]->reportConfig.choice.reportConfigEUTRA.triggerType.present);
                  break;
              }
            }
          }

          if (ue->MeasObj[i][measObjId-1]->measObject.present == LTE_MeasObjectToAddMod__measObject_PR_measObjectNR_r15) {
            if (!does_rrcConnReconfig_have_nr(ue))
              break;
            LTE_ReportConfigInterRAT_t *rc = &ue->ReportConfig[i][reportConfigId-1]->reportConfig.choice.reportConfigInterRAT;
            LTE_TimeToTrigger_t trig_per = rc->triggerType.choice.event.timeToTrigger;
            ttt_ms = timeToTrigger_ms[trig_per];
            LOG_D(RRC, "[UE %d] Frame %d: B1_NR_r15 event. count %d, ttt %ld\n",
                  ctxt_pP->module_id, ctxt_pP->frame, ue->subframeCount, ttt_ms);
            if (ue->subframeCount < ttt_ms) {
              ++ue->subframeCount;
              break;
            }
            ue->subframeCount = 0;
            bool is_state_connected = false;
            bool is_t304_inactive = false;
            bool have_meas_flag = false;
            if (ue->Info[0].State >= RRC_CONNECTED)
              is_state_connected = true;
            if (ue->Info[0].T304_active == 0)
              is_t304_inactive = true;
            if (ue->HandoverInfoUe.measFlag == 1)
              have_meas_flag = true;

            if (is_state_connected && is_t304_inactive && have_meas_flag) {
              LOG_I(RRC,"[UE %d] Frame %d: Triggering generation of Meas Report for NR_r15. count = %d\n",
                    ctxt_pP->module_id, ctxt_pP->frame, ue->subframeCount);
              usleep(200000); //Melissa Elkadi come back for EPC
              if (ue->measReportList[i][j] == NULL) {
                ue->measReportList[i][j] = malloc(sizeof(MEAS_REPORT_LIST));
              }
              ue->measReportList[i][j]->measId = ue->MeasId[i][j]->measId;
              ue->measReportList[i][j]->numberOfReportsSent = 0;
              rrc_ue_generate_nrMeasurementReport(ctxt_pP, eNB_index);
              ue->HandoverInfoUe.measFlag = 1;
              LOG_I(RRC,"[UE %d] Frame %d: RSRB detected, state: %d \n",
                    ctxt_pP->module_id, ctxt_pP->frame, ue->Info[0].State);
            } else {
                if(ue->measReportList[i][j] != NULL) {
                  free(ue->measReportList[i][j]);
                }
                ue->measReportList[i][j] = NULL;
            }
          }
        }
      }
    }
  }
}

//check_trigger_meas_event(ue_mod_idP, frameP, eNB_index, i,j,ofn,ocn,hys,ofs,ocs,a3_offset,ttt_ms)
//-----------------------------------------------------------------------------

uint8_t check_trigger_meas_event(
  module_id_t     ue_mod_idP,
  frame_t         frameP,
  uint8_t         eNB_index,
  uint8_t         ue_cnx_index,
  uint8_t         meas_index,
  LTE_Q_OffsetRange_t ofn,
  LTE_Q_OffsetRange_t ocn,
  LTE_Hysteresis_t    hys,
  LTE_Q_OffsetRange_t ofs,
  LTE_Q_OffsetRange_t ocs,
  long            a3_offset,
  LTE_TimeToTrigger_t ttt ) {
  uint8_t eNB_offset;
  //  uint8_t currentCellIndex = frame_parms->Nid_cell;
  uint8_t tmp_offset;
  LOG_D(RRC,"[UE %d] ofn(%ld) ocn(%ld) hys(%ld) ofs(%ld) ocs(%ld) ttt(%ld) rssi %3.1f\n",
        ue_mod_idP,
        ofn,ocn,hys,ofs,ocs,ttt,
        10*log10(get_RSSI(ue_mod_idP,0))-get_rx_total_gain_dB(ue_mod_idP,0));
  LOG_D(RRC, "[UE %d] Frame %d: num_adj: %d eNB_idx: %d, NB_eNB_INST: %d\n",
        ue_mod_idP, frameP, get_n_adj_cells(ue_mod_idP,0), eNB_index, NB_eNB_INST);

  for (eNB_offset = 0; eNB_offset<1+get_n_adj_cells(ue_mod_idP,0); eNB_offset++) {
    /* RHS: Verify that idx 0 corresponds to currentCellIndex in rsrp array */
    if((eNB_offset!=eNB_index)&&(eNB_offset<NB_eNB_INST)) {
      if(eNB_offset<eNB_index) {
        tmp_offset = eNB_offset;
      } else {
        tmp_offset = eNB_offset-1;
      }

      if(UE_rrc_inst[ue_mod_idP].rsrp_db_filtered[eNB_offset]+ofn+ocn-hys > UE_rrc_inst[ue_mod_idP].rsrp_db_filtered[eNB_index]+ofs+ocs-1/*+a3_offset*/) {
        UE_rrc_inst->measTimer[ue_cnx_index][meas_index][tmp_offset] += 2; //Called every subframe = 2ms
        LOG_D(RRC,"[UE %d] Frame %d: Entry measTimer[%d][%d][%d]: %d currentCell: %d betterCell: %d \n",
              ue_mod_idP, frameP, ue_cnx_index,meas_index,tmp_offset,UE_rrc_inst->measTimer[ue_cnx_index][meas_index][tmp_offset],0,eNB_offset);
      } else {
        UE_rrc_inst->measTimer[ue_cnx_index][meas_index][tmp_offset] = 0; //Exit condition: Resetting the measurement timer
        LOG_D(RRC,"[UE %d] Frame %d: Exit measTimer[%d][%d][%d]: %d currentCell: %d betterCell: %d \n",
              ue_mod_idP, frameP, ue_cnx_index,meas_index,tmp_offset,UE_rrc_inst->measTimer[ue_cnx_index][meas_index][tmp_offset],0,eNB_offset);
      }

      if (UE_rrc_inst->measTimer[ue_cnx_index][meas_index][tmp_offset] >= ttt) {
        UE_rrc_inst->HandoverInfoUe.targetCellId = get_adjacent_cell_id(ue_mod_idP,tmp_offset); //WARNING!!!...check this!
        LOG_D(RRC,"[UE %d] Frame %d eNB %d: Handover triggered: targetCellId: %ld currentCellId: %d eNB_offset: %d rsrp source: %3.1f rsrp target: %3.1f\n",
              ue_mod_idP, frameP, eNB_index,
              UE_rrc_inst->HandoverInfoUe.targetCellId,ue_cnx_index,eNB_offset,
              get_RSRP(ue_mod_idP,0,0),
              get_RSRP(ue_mod_idP,0,1));
        UE_rrc_inst->Info[0].handoverTarget = eNB_offset;
        //LOG_D(RRC,"PHY_ID: %d \n",UE_rrc_inst->HandoverInfoUe.targetCellId);
        return 1;
      }

      // else{
      //  LOG_D(RRC,"Condition does not hold\n");
      // }
    }
  }

  return 0;
}

//-----------------------------------------------------------------------------
int decode_MCCH_Message( const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index, const uint8_t *const Sdu, const uint8_t Sdu_len, const uint8_t mbsfn_sync_area ) {
  LTE_MCCH_Message_t               *mcch=NULL;
  LTE_MBSFNAreaConfiguration_r9_t **mcch_message=&UE_rrc_inst[ctxt_pP->module_id].mcch_message[eNB_index];
  asn_dec_rval_t                dec_rval;

  if (UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].MCCHStatus[mbsfn_sync_area] == 1) {
    LOG_D(RRC,"[UE %d] Frame %d: MCCH MESSAGE for MBSFN sync area %d has been already received!\n",
          ctxt_pP->module_id,
          ctxt_pP->frame,
          mbsfn_sync_area);
    return 0; // avoid decoding to prevent memory bloating
  } else if(1/*UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].State >= RRC_CONNECTED*/ /*|| UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].State == RRC_RECONFIGURED*/){
    dec_rval = uper_decode_complete(NULL,
                                    &asn_DEF_LTE_MCCH_Message,
                                    (void **)&mcch,
                                    (const void *)Sdu,
                                    Sdu_len);

    if ((dec_rval.code != RC_OK) && (dec_rval.consumed==0)) {
      LOG_E(RRC,"[UE %d] Failed to decode MCCH__MESSAGE (%lu bits)\n",
            ctxt_pP->module_id,
            dec_rval.consumed);
      //free the memory
      SEQUENCE_free(&asn_DEF_LTE_MCCH_Message, (void *)mcch, 1);
      return -1;
    }

    if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
      xer_fprint(stdout, &asn_DEF_LTE_MCCH_Message, (void *)mcch);
    }
      xer_fprint(stdout, &asn_DEF_LTE_MCCH_Message, (void *)mcch);

    if (mcch->message.present == LTE_MCCH_MessageType_PR_c1) {
      LOG_D(RRC,"[UE %d] Found mcch message \n",
            ctxt_pP->module_id);

      if(mcch->message.choice.c1.present == LTE_MCCH_MessageType__c1_PR_mbsfnAreaConfiguration_r9) {
        /*
        memcpy((void*)*mcch_message,
         (void*)&mcch->message.choice.c1.choice.mbsfnAreaConfiguration_r9,
         sizeof(MBSFNAreaConfiguration_r9_t)); */
        *mcch_message = &mcch->message.choice.c1.choice.mbsfnAreaConfiguration_r9;
        LOG_I(RRC,"[UE %d] Frame %d : Found MBSFNAreaConfiguration from eNB %d \n",
              ctxt_pP->module_id,
              ctxt_pP->frame,
              eNB_index);
        decode_MBSFNAreaConfiguration(
          ctxt_pP->module_id,
          eNB_index,
          ctxt_pP->frame,
          mbsfn_sync_area);
      }
    }else if(mcch->message.present == LTE_MCCH_MessageType_PR_later){
      LOG_D(RRC,"[UE %d] Found mcch message \n",
            ctxt_pP->module_id);
        if(mcch->message.choice.later.present == LTE_MCCH_MessageType__later_PR_c2){
		if(mcch->message.choice.later.choice.c2.present == LTE_MCCH_MessageType__later__c2_PR_mbmsCountingRequest_r10){
        		LOG_I(RRC,"[UE %d] Frame %d : Found MBMSCountingRequest from eNB %d %p\n",
              			ctxt_pP->module_id,
              			ctxt_pP->frame,
              			eNB_index,&mcch->message.choice.later.choice.c2.choice.mbmsCountingRequest_r10);

  			rrc_ue_process_MBMSCountingRequest(ctxt_pP,&mcch->message.choice.later.choice.c2.choice.mbmsCountingRequest_r10,eNB_index);

			decode_MBMSCountingRequest(
          			ctxt_pP->module_id,
          			eNB_index,
          			ctxt_pP->frame,
          			mbsfn_sync_area);


		}
	}
    }

  }

  return 0;
}

//-----------------------------------------------------------------------------
void decode_MBSFNAreaConfiguration( module_id_t ue_mod_idP, uint8_t eNB_index, frame_t frameP, uint8_t mbsfn_sync_area ) {
  uint8_t i;
  protocol_ctxt_t               ctxt;
  LOG_I(RRC,"[UE %d] Frame %d : Number of MCH(s) in the MBSFN Sync Area %d  is %d\n",
        ue_mod_idP, frameP, mbsfn_sync_area, UE_rrc_inst[ue_mod_idP].mcch_message[eNB_index]->pmch_InfoList_r9.list.count);

  // Configure commonSF_Alloc
  for(i=0; i< UE_rrc_inst[ue_mod_idP].mcch_message[eNB_index]->commonSF_Alloc_r9.list.count; i++) {
    LOG_W(RRC,"[UE %d] Frame %d, commonSF_Alloc_r9: radioframeAllocationPeriod(%ldn),radioframeAllocationOffset(%ld), subframeAllocation(%x,%x,%x)\n",
          ue_mod_idP, frameP,
          UE_rrc_inst[ue_mod_idP].mcch_message[eNB_index]->commonSF_Alloc_r9.list.array[i]->radioframeAllocationPeriod<<1,
          UE_rrc_inst[ue_mod_idP].mcch_message[eNB_index]->commonSF_Alloc_r9.list.array[i]->radioframeAllocationOffset,
          UE_rrc_inst[ue_mod_idP].mcch_message[eNB_index]->commonSF_Alloc_r9.list.array[i]->subframeAllocation.choice.oneFrame.buf[0],
          UE_rrc_inst[ue_mod_idP].mcch_message[eNB_index]->commonSF_Alloc_r9.list.array[i]->subframeAllocation.choice.oneFrame.buf[1],
          UE_rrc_inst[ue_mod_idP].mcch_message[eNB_index]->commonSF_Alloc_r9.list.array[i]->subframeAllocation.choice.oneFrame.buf[2]);
    UE_mac_inst[ue_mod_idP].commonSF_Alloc_r9_mbsfn_SubframeConfig[i] = UE_rrc_inst[ue_mod_idP].mcch_message[eNB_index]->commonSF_Alloc_r9.list.array[i];
  }

  LOG_W(RRC,"[UE %d] Frame %d, commonSF_AllocPeriod_r9 %drf \n",
        ue_mod_idP, frameP,
        4<<UE_rrc_inst[ue_mod_idP].mcch_message[eNB_index]->commonSF_AllocPeriod_r9);
  // Configure commonSF_AllocPeriod
  UE_mac_inst[ue_mod_idP].commonSF_AllocPeriod_r9 = UE_rrc_inst[ue_mod_idP].mcch_message[eNB_index]->commonSF_AllocPeriod_r9;
  //  store to MAC/PHY necessary parameters for receiving MTCHs
  rrc_mac_config_req_ue(ue_mod_idP,0,eNB_index,
                        (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                        (struct LTE_PhysicalConfigDedicated *)NULL,
                        (LTE_SCellToAddMod_r10_t *)NULL,
                        (LTE_MeasObjectToAddMod_t **)NULL,
                        (LTE_MAC_MainConfig_t *)NULL,
                        0,
                        (struct LTE_LogicalChannelConfig *)NULL,
                        (LTE_MeasGapConfig_t *)NULL,
                        (LTE_TDD_Config_t *)NULL,
                        (LTE_MobilityControlInfo_t *)NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        (LTE_MBSFN_SubframeConfigList_t *)NULL,
                        0,
                        (LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                        &UE_rrc_inst[ue_mod_idP].mcch_message[eNB_index]->pmch_InfoList_r9,
                        0,
                        NULL,
                        NULL,
                        0,
                        (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                        (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                       );
  if(1/*UE_rrc_inst[ue_mod_idP].Info[eNB_index].State >= RRC_CONNECTED*/ /*|| UE_rrc_inst[ue_mod_idP].Info[eNB_index].State == RRC_RECONFIGURED*/)
  	UE_rrc_inst[ue_mod_idP].Info[eNB_index].MCCHStatus[mbsfn_sync_area] = 1;
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_idP, ENB_FLAG_NO, UE_rrc_inst[ue_mod_idP].Info[eNB_index].rnti, frameP, 0,eNB_index);
  // Config Radio Bearer for MBMS user data (similar way to configure for eNB side in init_MBMS function)
  rrc_pdcp_config_asn1_req(&ctxt,
                           NULL, // SRB_ToAddModList
                           NULL, // DRB_ToAddModList
                           (LTE_DRB_ToReleaseList_t *)NULL,
                           0, // security mode
                           NULL, // key rrc encryption
                           NULL, // key rrc integrity
                           NULL, // key encryption
                           &(UE_rrc_inst[ue_mod_idP].mcch_message[eNB_index]->pmch_InfoList_r9),
                           NULL
                          );
  rrc_rlc_config_asn1_req(&ctxt,
                          NULL,// SRB_ToAddModList
                          NULL,// DRB_ToAddModList
                          NULL,// DRB_ToReleaseList
                          &(UE_rrc_inst[ue_mod_idP].mcch_message[eNB_index]->pmch_InfoList_r9), 0, 0
                         );
  // */
}

void decode_MBMSCountingRequest( module_id_t ue_mod_idP, uint8_t eNB_index, frame_t frameP, uint8_t mbsfn_sync_area ) {
  //uint8_t i;
  //protocol_ctxt_t               ctxt;



}

//-----------------------------------------------------------------------------
void *rrc_ue_task( void *args_p ) {
  MessageDef   *msg_p;
  instance_t    instance;
  unsigned int  ue_mod_id;
  int           result;
  SRB_INFO     *srb_info_p;
  protocol_ctxt_t  ctxt;
  itti_mark_task_ready (TASK_RRC_UE);

  while(1) {
    // Wait for a message
    itti_receive_msg (TASK_RRC_UE, &msg_p);
    instance = ITTI_MSG_DESTINATION_INSTANCE (msg_p);
    ue_mod_id = UE_INSTANCE_TO_MODULE_ID(instance);

    /* TODO: Add case to handle nr-UE messages we want from nrUE RRC layer */
    switch (ITTI_MSG_ID(msg_p)) {
      case TERMINATE_MESSAGE:
        LOG_W(RRC, " *** Exiting RRC thread\n");
        itti_exit_task ();
        break;

      case MESSAGE_TEST:
        LOG_D(RRC, "[UE %d] Received %s\n", ue_mod_id, ITTI_MSG_NAME (msg_p));
        break;

      /* MAC messages */
      case RRC_MAC_IN_SYNC_IND:
        LOG_D(RRC, "[UE %d] Received %s: frameP %d, eNB %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p),
              RRC_MAC_IN_SYNC_IND (msg_p).frame, RRC_MAC_IN_SYNC_IND (msg_p).enb_index);
        UE_rrc_inst[ue_mod_id].Info[RRC_MAC_IN_SYNC_IND (msg_p).enb_index].N310_cnt = 0;

        if (UE_rrc_inst[ue_mod_id].Info[RRC_MAC_IN_SYNC_IND (msg_p).enb_index].T310_active == 1) {
          UE_rrc_inst[ue_mod_id].Info[RRC_MAC_IN_SYNC_IND (msg_p).enb_index].N311_cnt++;
        }

        break;

      case RRC_MAC_OUT_OF_SYNC_IND:
        LOG_D(RRC, "[UE %d] Received %s: frameP %d, eNB %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p),
              RRC_MAC_OUT_OF_SYNC_IND (msg_p).frame, RRC_MAC_OUT_OF_SYNC_IND (msg_p).enb_index);
        UE_rrc_inst[ue_mod_id].Info[RRC_MAC_OUT_OF_SYNC_IND (msg_p).enb_index].N310_cnt ++;
        break;

      case RRC_MAC_BCCH_DATA_IND:
        LOG_D(RRC, "[UE %d] Received %s: frameP %d, eNB %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p),
              RRC_MAC_BCCH_DATA_IND (msg_p).frame, RRC_MAC_BCCH_DATA_IND (msg_p).enb_index);
        //      PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, ENB_FLAG_NO, NOT_A_RNTI, RRC_MAC_BCCH_DATA_IND (msg_p).frame, 0);
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_id, ENB_FLAG_NO, NOT_A_RNTI, RRC_MAC_BCCH_DATA_IND (msg_p).frame, 0,RRC_MAC_BCCH_DATA_IND (msg_p).enb_index);
        decode_BCCH_DLSCH_Message (&ctxt,
                                   RRC_MAC_BCCH_DATA_IND (msg_p).enb_index,
                                   RRC_MAC_BCCH_DATA_IND (msg_p).sdu,
                                   RRC_MAC_BCCH_DATA_IND (msg_p).sdu_size,
                                   RRC_MAC_BCCH_DATA_IND (msg_p).rsrq,
                                   RRC_MAC_BCCH_DATA_IND (msg_p).rsrp);
        break;

      case RRC_MAC_BCCH_MBMS_DATA_IND:
        LOG_D(RRC, "[UE %d] Received %s: frameP %d, eNB %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p),
              RRC_MAC_BCCH_MBMS_DATA_IND (msg_p).frame, RRC_MAC_BCCH_MBMS_DATA_IND (msg_p).enb_index);
        //      PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, ENB_FLAG_NO, NOT_A_RNTI, RRC_MAC_BCCH_DATA_IND (msg_p).frame, 0);
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_id, ENB_FLAG_NO, NOT_A_RNTI, RRC_MAC_BCCH_MBMS_DATA_IND (msg_p).frame, 0,RRC_MAC_BCCH_MBMS_DATA_IND (msg_p).enb_index);
        decode_BCCH_MBMS_DLSCH_Message (&ctxt,
                                        RRC_MAC_BCCH_MBMS_DATA_IND (msg_p).enb_index,
                                        RRC_MAC_BCCH_MBMS_DATA_IND (msg_p).sdu,
                                        RRC_MAC_BCCH_MBMS_DATA_IND (msg_p).sdu_size,
                                        RRC_MAC_BCCH_MBMS_DATA_IND (msg_p).rsrq,
                                        RRC_MAC_BCCH_MBMS_DATA_IND (msg_p).rsrp);
        break;

      case RRC_MAC_CCCH_DATA_CNF:
        LOG_D(RRC, "[UE %d] Received %s: eNB %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p),
              RRC_MAC_CCCH_DATA_CNF (msg_p).enb_index);
        // reset the tx buffer to indicate RRC that ccch was successfully transmitted (for example if contention resolution succeeds)
        UE_rrc_inst[ue_mod_id].Srb0[RRC_MAC_CCCH_DATA_CNF (msg_p).enb_index].Tx_buffer.payload_size = 0;
        break;

      case RRC_MAC_CCCH_DATA_IND:
        LOG_D(RRC, "[UE %d] RNTI %x Received %s: frameP %d, eNB %d\n",
              ue_mod_id,
              RRC_MAC_CCCH_DATA_IND (msg_p).rnti,
              ITTI_MSG_NAME (msg_p),
              RRC_MAC_CCCH_DATA_IND (msg_p).frame,
              RRC_MAC_CCCH_DATA_IND (msg_p).enb_index);
        srb_info_p = &UE_rrc_inst[ue_mod_id].Srb0[RRC_MAC_CCCH_DATA_IND (msg_p).enb_index];
        memcpy (srb_info_p->Rx_buffer.Payload, RRC_MAC_CCCH_DATA_IND (msg_p).sdu,
                RRC_MAC_CCCH_DATA_IND (msg_p).sdu_size);
        srb_info_p->Rx_buffer.payload_size = RRC_MAC_CCCH_DATA_IND (msg_p).sdu_size;
        //      PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, ENB_FLAG_NO, RRC_MAC_CCCH_DATA_IND (msg_p).rnti, RRC_MAC_CCCH_DATA_IND (msg_p).frame, 0);
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_id, ENB_FLAG_NO, RRC_MAC_CCCH_DATA_IND (msg_p).rnti, RRC_MAC_CCCH_DATA_IND (msg_p).frame, 0, RRC_MAC_CCCH_DATA_IND (msg_p).enb_index);
        rrc_ue_decode_ccch (&ctxt,
                            srb_info_p,
                            RRC_MAC_CCCH_DATA_IND (msg_p).enb_index);
        break;

      case RRC_MAC_MCCH_DATA_IND:
        LOG_D(RRC, "[UE %d] Received %s: frameP %d, eNB %d, mbsfn SA %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p),
              RRC_MAC_MCCH_DATA_IND (msg_p).frame, RRC_MAC_MCCH_DATA_IND (msg_p).enb_index, RRC_MAC_MCCH_DATA_IND (msg_p).mbsfn_sync_area);
        //PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, ENB_FLAG_NO, M_RNTI, RRC_MAC_MCCH_DATA_IND (msg_p).frame, 0);
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_id, ENB_FLAG_NO, M_RNTI, RRC_MAC_MCCH_DATA_IND (msg_p).frame, 0,RRC_MAC_MCCH_DATA_IND (msg_p).enb_index);
        decode_MCCH_Message (
          &ctxt,
          RRC_MAC_MCCH_DATA_IND (msg_p).enb_index,
          RRC_MAC_MCCH_DATA_IND (msg_p).sdu,
          RRC_MAC_MCCH_DATA_IND (msg_p).sdu_size,
          RRC_MAC_MCCH_DATA_IND (msg_p).mbsfn_sync_area);
        break;

      /*  //TTN (for D2D)
        case RRC_MAC_SL_DISCOVERY_DATA_IND:
           LOG_D(RRC, "[UE %d] Received %s: frameP %d, eNB %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p),
                 RRC_MAC_SL_DISCOVERY_DATA_IND (msg_p).frame, RRC_MAC_SL_DISCOVERY_DATA_IND (msg_p).enb_index);
           PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_id, ENB_FLAG_NO, M_RNTI, RRC_MAC_SL_DISCOVERY_DATA_IND (msg_p).frame, 0,RRC_MAC_SL_DISCOVERY_DATA_IND (msg_p).enb_index);
           //send to ProSeApp
           break;
      */

      /* PDCP messages */
      case RRC_DCCH_DATA_IND:
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, RRC_DCCH_DATA_IND (msg_p).module_id, ENB_FLAG_NO, RRC_DCCH_DATA_IND (msg_p).rnti, RRC_DCCH_DATA_IND (msg_p).frame, 0,RRC_DCCH_DATA_IND (msg_p).eNB_index);
        LOG_D(RRC, "[UE %d] Received %s: frameP %d, DCCH %d, eNB %d\n",
              RRC_DCCH_DATA_IND (msg_p).module_id,
              ITTI_MSG_NAME (msg_p),
              RRC_DCCH_DATA_IND (msg_p).frame,
              RRC_DCCH_DATA_IND (msg_p).dcch_index,
              RRC_DCCH_DATA_IND (msg_p).eNB_index);
        LOG_D(RRC, PROTOCOL_RRC_CTXT_UE_FMT"Received %s DCCH %d, eNB %d\n",
              PROTOCOL_RRC_CTXT_UE_ARGS(&ctxt),
              ITTI_MSG_NAME (msg_p),
              RRC_DCCH_DATA_IND (msg_p).dcch_index,
              RRC_DCCH_DATA_IND (msg_p).eNB_index);
        rrc_ue_decode_dcch (
          &ctxt,
          RRC_DCCH_DATA_IND (msg_p).dcch_index,
          RRC_DCCH_DATA_IND (msg_p).sdu_p,
          RRC_DCCH_DATA_IND (msg_p).sdu_size,
          RRC_DCCH_DATA_IND (msg_p).eNB_index);
        // Message buffer has been processed, free it now.
        result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), RRC_DCCH_DATA_IND (msg_p).sdu_p);
        AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
        break;

      case RRC_DCCH_DATA_COPY_IND:
      {
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, RRC_DCCH_DATA_COPY_IND (msg_p).module_id,
                                       ENB_FLAG_NO, RRC_DCCH_DATA_COPY_IND (msg_p).rnti,
                                       RRC_DCCH_DATA_COPY_IND (msg_p).frame,
                                       0,
                                       RRC_DCCH_DATA_COPY_IND (msg_p).eNB_index);
        LOG_I(RRC, "[UE %d] Received %s. Now calling rrc_ue_process_ueCapabilityEnquiry\n",
              ue_mod_id, ITTI_MSG_NAME (msg_p));
        rrc_dcch_data_copy_t *dl_dcch_buffer = (void *)RRC_DCCH_DATA_COPY_IND (msg_p).sdu_p;
        AssertFatal(RRC_DCCH_DATA_COPY_IND (msg_p).sdu_size == sizeof(*dl_dcch_buffer), "Size of dl_dcch_buffer incorrect\n");
        rrc_ue_process_ueCapabilityEnquiry(
              &ctxt,
              &dl_dcch_buffer->dl_dcch_msg->message.choice.c1.choice.ueCapabilityEnquiry,
              RRC_DCCH_DATA_COPY_IND (msg_p).eNB_index);
        SEQUENCE_free(&asn_DEF_LTE_DL_DCCH_Message, dl_dcch_buffer->dl_dcch_msg, ASFM_FREE_EVERYTHING);
        result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), RRC_DCCH_DATA_COPY_IND (msg_p).sdu_p);
        AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
        break;
      }

      case RRC_NRUE_CAP_INFO_IND:
      {
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, RRC_NRUE_CAP_INFO_IND (msg_p).module_id,
                                       ENB_FLAG_NO, RRC_NRUE_CAP_INFO_IND (msg_p).rnti,
                                       RRC_NRUE_CAP_INFO_IND (msg_p).frame,
                                       0,
                                       RRC_NRUE_CAP_INFO_IND (msg_p).eNB_index);
        LOG_I(RRC, "[UE %d] Received %s. Now calling rrc_ue_process_nrueCapabilityEnquiry\n",
              ue_mod_id, ITTI_MSG_NAME (msg_p));
        rrc_nrue_cap_info_t *nrue_cap_info = (void *)RRC_NRUE_CAP_INFO_IND (msg_p).sdu_p;
        AssertFatal(RRC_NRUE_CAP_INFO_IND (msg_p).sdu_size == sizeof(*nrue_cap_info), "Size of nrue_cap_info incorrect\n");
        rrc_ue_process_nrueCapabilityEnquiry(
              &ctxt,
              &nrue_cap_info->dl_dcch_msg->message.choice.c1.choice.ueCapabilityEnquiry,
              nrue_cap_info,
              RRC_NRUE_CAP_INFO_IND (msg_p).eNB_index);
        SEQUENCE_free(&asn_DEF_LTE_DL_DCCH_Message, nrue_cap_info->dl_dcch_msg, ASFM_FREE_EVERYTHING);
        result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), RRC_NRUE_CAP_INFO_IND (msg_p).sdu_p);
        AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
        break;
      }
      case NAS_OAI_TUN_NSA:
      {
        LOG_I(NAS, "Melissa Elkadi Received %s: length %lu. About to send this to the NR UE\n", ITTI_MSG_NAME (msg_p),
              sizeof(NAS_OAI_TUN_NSA (msg_p).buffer));
        char buffer[RRC_BUF_SIZE];
        memcpy(buffer, NAS_OAI_TUN_NSA(msg_p).buffer, sizeof(buffer));
        nsa_sendmsg_to_nrue(buffer, sizeof(buffer), OAI_TUN_IFACE_NSA);
        break;
      }


      case NAS_KENB_REFRESH_REQ:
        memcpy((void *)UE_rrc_inst[ue_mod_id].kenb, (void *)NAS_KENB_REFRESH_REQ(msg_p).kenb, sizeof(UE_rrc_inst[ue_mod_id].kenb));
        LOG_D(RRC, "[UE %d] Received %s: refreshed RRC::KeNB = "
              "%02x%02x%02x%02x"
              "%02x%02x%02x%02x"
              "%02x%02x%02x%02x"
              "%02x%02x%02x%02x"
              "%02x%02x%02x%02x"
              "%02x%02x%02x%02x"
              "%02x%02x%02x%02x"
              "%02x%02x%02x%02x\n",
              ue_mod_id, ITTI_MSG_NAME (msg_p),
              UE_rrc_inst[ue_mod_id].kenb[0],  UE_rrc_inst[ue_mod_id].kenb[1],  UE_rrc_inst[ue_mod_id].kenb[2],  UE_rrc_inst[ue_mod_id].kenb[3],
              UE_rrc_inst[ue_mod_id].kenb[4],  UE_rrc_inst[ue_mod_id].kenb[5],  UE_rrc_inst[ue_mod_id].kenb[6],  UE_rrc_inst[ue_mod_id].kenb[7],
              UE_rrc_inst[ue_mod_id].kenb[8],  UE_rrc_inst[ue_mod_id].kenb[9],  UE_rrc_inst[ue_mod_id].kenb[10], UE_rrc_inst[ue_mod_id].kenb[11],
              UE_rrc_inst[ue_mod_id].kenb[12], UE_rrc_inst[ue_mod_id].kenb[13], UE_rrc_inst[ue_mod_id].kenb[14], UE_rrc_inst[ue_mod_id].kenb[15],
              UE_rrc_inst[ue_mod_id].kenb[16], UE_rrc_inst[ue_mod_id].kenb[17], UE_rrc_inst[ue_mod_id].kenb[18], UE_rrc_inst[ue_mod_id].kenb[19],
              UE_rrc_inst[ue_mod_id].kenb[20], UE_rrc_inst[ue_mod_id].kenb[21], UE_rrc_inst[ue_mod_id].kenb[22], UE_rrc_inst[ue_mod_id].kenb[23],
              UE_rrc_inst[ue_mod_id].kenb[24], UE_rrc_inst[ue_mod_id].kenb[25], UE_rrc_inst[ue_mod_id].kenb[26], UE_rrc_inst[ue_mod_id].kenb[27],
              UE_rrc_inst[ue_mod_id].kenb[28], UE_rrc_inst[ue_mod_id].kenb[29], UE_rrc_inst[ue_mod_id].kenb[30], UE_rrc_inst[ue_mod_id].kenb[31]);
        break;

      /* NAS messages */
      case NAS_CELL_SELECTION_REQ:
        LOG_D(RRC, "[UE %d] Received %s: state %d, plmnID (%d%d%d.%d%d%d), rat %x\n", ue_mod_id, ITTI_MSG_NAME (msg_p), rrc_get_state(ue_mod_id),
              NAS_CELL_SELECTION_REQ (msg_p).plmnID.MCCdigit1,
              NAS_CELL_SELECTION_REQ (msg_p).plmnID.MCCdigit2,
              NAS_CELL_SELECTION_REQ (msg_p).plmnID.MCCdigit3,
              NAS_CELL_SELECTION_REQ (msg_p).plmnID.MNCdigit1,
              NAS_CELL_SELECTION_REQ (msg_p).plmnID.MNCdigit2,
              NAS_CELL_SELECTION_REQ (msg_p).plmnID.MNCdigit3,
              NAS_CELL_SELECTION_REQ (msg_p).rat);

        if (rrc_get_state(ue_mod_id) == RRC_STATE_INACTIVE) {
          // have a look at MAC/main.c void dl_phy_sync_success(...)
          openair_rrc_ue_init(ue_mod_id,0);
        }

        /* Save cell selection criterion */
        {
          UE_rrc_inst[ue_mod_id].plmnID = NAS_CELL_SELECTION_REQ (msg_p).plmnID;
          UE_rrc_inst[ue_mod_id].rat = NAS_CELL_SELECTION_REQ (msg_p).rat;
          LOG_D(RRC, "[UE %d] Save cell selection criterion MCC %X%X%X MNC %X%X%X\n",
                ue_mod_id,
                UE_rrc_inst[ue_mod_id].plmnID.MCCdigit1,
                UE_rrc_inst[ue_mod_id].plmnID.MCCdigit2,
                UE_rrc_inst[ue_mod_id].plmnID.MCCdigit3,
                UE_rrc_inst[ue_mod_id].plmnID.MNCdigit1,
                UE_rrc_inst[ue_mod_id].plmnID.MNCdigit2,
                UE_rrc_inst[ue_mod_id].plmnID.MNCdigit3);
        }

        switch (rrc_get_state(ue_mod_id)) {
          case RRC_STATE_INACTIVE: {
            rrc_set_state (ue_mod_id, RRC_STATE_IDLE);
            /* Fall through to next case */
          }

          case RRC_STATE_IDLE: {
            /* Ask to layer 1 to find a cell matching the criterion */
            MessageDef *message_p;
            message_p = itti_alloc_new_message(TASK_RRC_UE, 0, PHY_FIND_CELL_REQ);
            PHY_FIND_CELL_REQ (message_p).earfcn_start = 1;
            PHY_FIND_CELL_REQ (message_p).earfcn_end = 1;
            itti_send_msg_to_task(TASK_PHY_UE, UE_MODULE_ID_TO_INSTANCE(ue_mod_id), message_p);
            rrc_set_sub_state (ue_mod_id, RRC_SUB_STATE_IDLE_SEARCHING);
            break;
          }

          case RRC_STATE_CONNECTED:
            /* should not happen */
            LOG_E(RRC, "[UE %d] request %s in RRC state %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p), rrc_get_state(ue_mod_id));
            break;

          default:
            LOG_E(RRC, "[UE %d] Invalid RRC state %d\n", ue_mod_id, rrc_get_state(ue_mod_id));
            break;
        }

        break;

      case NAS_CONN_ESTABLI_REQ:
        LOG_D(RRC, "[UE %d] Received %s: cause %d, type %d, s_tmsi (mme code %"PRIu8", m-tmsi %"PRIu32"), plmnID (%d%d%d.%d%d%d)\n", ue_mod_id, ITTI_MSG_NAME (msg_p), NAS_CONN_ESTABLI_REQ (msg_p).cause,
              NAS_CONN_ESTABLI_REQ (msg_p).type,
              NAS_CONN_ESTABLI_REQ (msg_p).s_tmsi.MMEcode,
              NAS_CONN_ESTABLI_REQ (msg_p).s_tmsi.m_tmsi,
              NAS_CONN_ESTABLI_REQ (msg_p).plmnID.MCCdigit1,
              NAS_CONN_ESTABLI_REQ (msg_p).plmnID.MCCdigit2,
              NAS_CONN_ESTABLI_REQ (msg_p).plmnID.MCCdigit3,
              NAS_CONN_ESTABLI_REQ (msg_p).plmnID.MNCdigit1,
              NAS_CONN_ESTABLI_REQ (msg_p).plmnID.MNCdigit2,
              NAS_CONN_ESTABLI_REQ (msg_p).plmnID.MNCdigit3);
        //PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, ENB_FLAG_NO, NOT_A_RNTI, 0, 0);
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_id, ENB_FLAG_NO, NOT_A_RNTI, 0, 0, 0);
        UE_rrc_inst[ue_mod_id].initialNasMsg = NAS_CONN_ESTABLI_REQ (msg_p).initialNasMsg;

        switch (rrc_get_state(ue_mod_id)) {
          case RRC_STATE_IDLE: {
            if (rrc_get_sub_state(ue_mod_id) == RRC_SUB_STATE_IDLE_SIB_COMPLETE) {
              rrc_ue_generate_RRCConnectionRequest(&ctxt, 0);
              LOG_D(RRC, "not sending connection request\n");
              rrc_set_sub_state (ue_mod_id, RRC_SUB_STATE_IDLE_CONNECTING);
            }

            break;
          }

          case RRC_STATE_INACTIVE:
          case RRC_STATE_CONNECTED:
            /* should not happen */
            LOG_E(RRC, "[UE %d] request %s in RRC state %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p), rrc_get_state(ue_mod_id));
            break;

          default:
            LOG_E(RRC, "[UE %d] Invalid RRC state %d\n", ue_mod_id, rrc_get_state(ue_mod_id));
            break;
        }

        break;

      case NAS_UPLINK_DATA_REQ: {
        uint32_t length;
        uint8_t *buffer;
        LOG_D(RRC, "[UE %d] Received %s: UEid %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p), NAS_UPLINK_DATA_REQ (msg_p).UEid);
        /* Create message for PDCP (ULInformationTransfer_t) */
        length = do_ULInformationTransfer(&buffer, NAS_UPLINK_DATA_REQ (msg_p).nasMsg.length, NAS_UPLINK_DATA_REQ (msg_p).nasMsg.data);
        /* Transfer data to PDCP */
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_id, ENB_FLAG_NO, UE_rrc_inst[ue_mod_id].Info[0].rnti, 0, 0,0);

        // check if SRB2 is created, if yes request data_req on DCCH1 (SRB2)
        if(UE_rrc_inst[ue_mod_id].SRB2_config[0] == NULL) {
          rrc_data_req_ue (&ctxt,
                           DCCH,
                           rrc_mui++,
                           SDU_CONFIRM_NO,
                           length, buffer,
                           PDCP_TRANSMISSION_MODE_CONTROL);
        } else {
          rrc_data_req_ue (&ctxt,
                           DCCH1,
                           rrc_mui++,
                           SDU_CONFIRM_NO,
                           length, buffer,
                           PDCP_TRANSMISSION_MODE_CONTROL);
        }

        break;
      }

# if ENABLE_RAL

      case RRC_RAL_SCAN_REQ:
        LOG_D(RRC, "[UE %d] Received %s: state %d\n", ue_mod_id,ITTI_MSG_NAME (msg_p) );

        switch (rrc_get_state(ue_mod_id)) {
          case RRC_STATE_INACTIVE: {
            rrc_set_state (ue_mod_id, RRC_STATE_IDLE);
            /* Fall through to next case */
          }

          case RRC_STATE_IDLE: {
            if (rrc_get_sub_state(ue_mod_id) != RRC_SUB_STATE_IDLE_SEARCHING) {
              /* Ask to layer 1 to find a cell matching the criterion */
              MessageDef *message_p;
              message_p = itti_alloc_new_message(TASK_RRC_UE, 0, PHY_FIND_CELL_REQ);
              rrc_set_sub_state (ue_mod_id, RRC_SUB_STATE_IDLE_SEARCHING);
              PHY_FIND_CELL_REQ (message_p).transaction_id = RRC_RAL_SCAN_REQ (msg_p).transaction_id;
              PHY_FIND_CELL_REQ (message_p).earfcn_start   = 1;
              PHY_FIND_CELL_REQ (message_p).earfcn_end     = 1; //44
              itti_send_msg_to_task(TASK_PHY_UE, instance, message_p);
            }

            break;
          }

          case RRC_STATE_CONNECTED:
            /* should not happen */
            LOG_E(RRC, "[UE %d] request %s in RRC state %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p), rrc_get_state(ue_mod_id));
            break;

          default:
            LOG_E(RRC, "[UE %d] Invalid RRC state %d\n", ue_mod_id, rrc_get_state(ue_mod_id));
            break;
        }

        break;

      case PHY_FIND_CELL_IND:
        LOG_D(RRC, "[UE %d] Received %s: state %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p), rrc_get_state(ue_mod_id));

        switch (rrc_get_state(ue_mod_id)) {
          case RRC_STATE_IDLE:
            switch (rrc_get_sub_state(ue_mod_id)) {
              case RRC_SUB_STATE_IDLE_SEARCHING: {
                MessageDef *message_p;
                int         i;
                message_p = itti_alloc_new_message(TASK_RRC_UE, 0, RRC_RAL_SCAN_CONF);
                RRC_RAL_SCAN_CONF (message_p).transaction_id = PHY_FIND_CELL_IND(msg_p).transaction_id;
                RRC_RAL_SCAN_CONF (message_p).num_scan_resp  = PHY_FIND_CELL_IND(msg_p).cell_nb;

                for (i = 0 ; i < PHY_FIND_CELL_IND(msg_p).cell_nb; i++) {
                  // TO DO
                  memset(&RRC_RAL_SCAN_CONF (message_p).link_scan_resp[i].link_addr,  0, sizeof(ral_link_addr_t));
                  // TO DO
                  memset(&RRC_RAL_SCAN_CONF (message_p).link_scan_resp[i].network_id, 0, sizeof(ral_network_id_t));
                  RRC_RAL_SCAN_CONF (message_p).link_scan_resp[i].sig_strength.choice     = RAL_SIG_STRENGTH_CHOICE_DBM;
                  RRC_RAL_SCAN_CONF (message_p).link_scan_resp[i].sig_strength._union.dbm = PHY_FIND_CELL_IND(msg_p).cells[i].rsrp;
                }

                rrc_set_sub_state (ue_mod_id, RRC_SUB_STATE_IDLE);
                itti_send_msg_to_task(TASK_RAL_UE, instance, message_p);
                break;
              }

              default:
                LOG_E(RRC, "[UE %d] Invalid RRC state %d substate %d\n",
                      ue_mod_id,
                      rrc_get_state(ue_mod_id),
                      rrc_get_sub_state(ue_mod_id));
            }

            break;

          case RRC_STATE_INACTIVE:
          case RRC_STATE_CONNECTED:
            /* should not happen */
            LOG_E(RRC, "[UE %d] indication %s in RRC state %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p), rrc_get_state(ue_mod_id));
            break;

          default:
            LOG_E(RRC, "[UE %d] Invalid RRC state %d\n", ue_mod_id, rrc_get_state(ue_mod_id));
            break;
        }

        break; // PHY_FIND_CELL_IND

      case PHY_MEAS_REPORT_IND: {
        LOG_D(RRC, "[UE %d] Received %s\n", ue_mod_id, ITTI_MSG_NAME (msg_p));
        MessageDef *message_p;
        message_p = itti_alloc_new_message(TASK_RRC_UE, 0, RRC_RAL_MEASUREMENT_REPORT_IND);
        memcpy(&RRC_RAL_MEASUREMENT_REPORT_IND (message_p).threshold,
               &PHY_MEAS_REPORT_IND(msg_p).threshold,
               sizeof(RRC_RAL_MEASUREMENT_REPORT_IND (message_p).threshold));
        memcpy(&RRC_RAL_MEASUREMENT_REPORT_IND (message_p).link_param,
               &PHY_MEAS_REPORT_IND(msg_p).link_param,
               sizeof(RRC_RAL_MEASUREMENT_REPORT_IND (message_p).link_param));
        LOG_D(RRC, "[UE %d] PHY_MEAS_REPORT_IN: sending msg %s to %s \n", ue_mod_id, "RRC_RAL_MEASUREMENT_REPORT_IND", "TASK_RAL_UE");
        itti_send_msg_to_task(TASK_RAL_UE, instance, message_p);
        break;
      }

      case RRC_RAL_CONFIGURE_THRESHOLD_REQ:
        LOG_D(RRC, "[UE %d] Received %s\n", ue_mod_id, ITTI_MSG_NAME (msg_p));
        rrc_ue_ral_handle_configure_threshold_request(ue_mod_id, msg_p);
        break;

      case RRC_RAL_CONNECTION_ESTABLISHMENT_REQ:
        LOG_D(RRC, "[UE %d] Received %s\n", ue_mod_id, ITTI_MSG_NAME (msg_p));

        switch (rrc_get_state(ue_mod_id)) {
          case RRC_STATE_IDLE: {
            if (rrc_get_sub_state(ue_mod_id) == RRC_SUB_STATE_IDLE_SIB_COMPLETE) {
              PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, ue_mod_id, ENB_FLAG_NO, UE_rrc_inst[ue_mod_id].Info[0].rnti, 0, 0, 0);
              rrc_ue_generate_RRCConnectionRequest(&ctxt, 0);
              LOG_D(RRC, "not sending connection request\n");
              rrc_set_sub_state (ue_mod_id, RRC_SUB_STATE_IDLE_CONNECTING);
            }

            break;
          }

          case RRC_STATE_INACTIVE:
          case RRC_STATE_CONNECTED:
            /* should not happen */
            LOG_E(RRC, "[UE %d] request %s in RRC state %d\n", ue_mod_id, ITTI_MSG_NAME (msg_p), rrc_get_state(ue_mod_id));
            break;

          default:
            LOG_E(RRC, "[UE %d] Invalid RRC state %d\n", ue_mod_id, rrc_get_state(ue_mod_id));
            break;
        }

        break;

      case RRC_RAL_CONNECTION_RELEASE_REQ:
        LOG_D(RRC, "[UE %d] Received %s\n", ue_mod_id, ITTI_MSG_NAME (msg_p));
        break;
#endif

      default:
        LOG_E(RRC, "[UE %d] Received unexpected message %s\n", ue_mod_id, ITTI_MSG_NAME (msg_p));
        break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    msg_p = NULL;
  }
}






/*------------------------------------------------------------------------------*/
void
openair_rrc_top_init_ue(
  int eMBMS_active,
  char *uecap_xer,
  uint8_t cba_group_active,
  uint8_t HO_active
)
//-----------------------------------------------------------------------------
{
  module_id_t         module_id;
  OAI_UECapability_t *UECap     = NULL;
  int                 CC_id;
  /* for no gcc warnings */
  (void)CC_id;
  LOG_D(RRC, "[OPENAIR][INIT] Init function start: NB_UE_INST=%d, NB_eNB_INST=%d\n", NB_UE_INST, NB_eNB_INST);

  if (NB_UE_INST > 0) {
    UE_rrc_inst = (UE_RRC_INST *) malloc16(NB_UE_INST*sizeof(UE_RRC_INST));
    memset (UE_rrc_inst, 0, NB_UE_INST * sizeof(UE_RRC_INST));
    LOG_D(RRC, "ALLOCATE %d Bytes for UE_RRC_INST @ %p\n", (unsigned int)(NB_UE_INST*sizeof(UE_RRC_INST)), UE_rrc_inst);
    // fill UE capability
    bool received_nr_msg = false;
    UECap = fill_ue_capability (uecap_xer, received_nr_msg);

    for (module_id = 0; module_id < NB_UE_INST; module_id++) {
      UE_rrc_inst[module_id].UECap = UECap;
      UE_rrc_inst[module_id].UECapability = UECap->sdu;
      UE_rrc_inst[module_id].UECapability_size = UECap->sdu_size;
    }

    LOG_I(RRC,"[UE] eMBMS active state is %d \n", eMBMS_active);

    for (module_id=0; module_id<NB_UE_INST; module_id++) {
      UE_rrc_inst[module_id].MBMS_flag = (uint8_t)eMBMS_active;
    }

    /* TODO: this is disabled for the moment because the standard UE
     * crashes when calling this function.
     */
    //init_SL_preconfig(&UE_rrc_inst[module_id],0);
  } else {
    UE_rrc_inst = NULL;
  }
}

//-----------------------------------------------------------------------------
void
rrc_top_cleanup_ue(
  void
)
//-----------------------------------------------------------------------------
{
  if (NB_UE_INST > 0) free (UE_rrc_inst);
}


//-----------------------------------------------------------------------------
uint8_t rrc_ue_generate_SidelinkUEInformation( const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index,LTE_SL_DestinationInfoList_r12_t  *destinationInfoList, long *discTxResourceReq,
    SL_TRIGGER_t mode) {
  uint8_t    size=0;
  uint8_t buffer[100];

  //Generate SidelinkUEInformation
  if (((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&8192) > 0) && (destinationInfoList != NULL)) {//if SIB18 is available
    size = do_SidelinkUEInformation(ctxt_pP->module_id, buffer, destinationInfoList, NULL, mode);
    LOG_I(RRC,"[UE %d][RRC_UE] Frame %d : Logical Channel UL-DCCH, Generating SidelinkUEInformation (bytes%d, eNB %d)\n",
          ctxt_pP->module_id,ctxt_pP->frame, size, eNB_index);
    //return size;
  }

  if (((UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].SIStatus&16384) > 0) && (discTxResourceReq != NULL)) {//if SIB19 is available
    size = do_SidelinkUEInformation(ctxt_pP->module_id, buffer, NULL, discTxResourceReq, mode);
    LOG_I(RRC,"[UE %d][RRC_UE] Frame %d : Logical Channel UL-DCCH, Generating SidelinkUEInformation (bytes%d, eNB %d)\n",
          ctxt_pP->module_id,ctxt_pP->frame, size, eNB_index);
    //return size;
  }

  rrc_data_req_ue (
    ctxt_pP,
    DCCH,
    rrc_mui++,
    SDU_CONFIRM_NO,
    size,
    buffer,
    PDCP_TRANSMISSION_MODE_CONTROL);
  return size;
}


// 3GPP 36.331 (Section 5.10.7.3)
uint8_t fill_SLSS(const protocol_ctxt_t *const ctxt_pP, const uint8_t eNB_index, LTE_SLSSID_r12_t *slss_id, uint8_t *subframe, uint8_t mode) {
  long syncOffsetIndicator = 0;

  switch(mode) {
    case 1: //if triggered by SL discovery announcement and in-coverage
      //discSyncConfig_r12 contains only one element
      *slss_id = UE_rrc_inst[ctxt_pP->module_id].sib19[eNB_index]->discConfig_r12->discSyncConfig_r12->list.array[0]->slssid_r12;
      syncOffsetIndicator = UE_rrc_inst[ctxt_pP->module_id].sib19[eNB_index]->discConfig_r12->discSyncConfig_r12->list.array[0]->syncOffsetIndicator_r12;
      //select subframe for SLSS
      break;

    case 2: //if triggered by SL communication and in-coverage
      if (UE_rrc_inst[ctxt_pP->module_id].sib18[eNB_index]->commConfig_r12->commSyncConfig_r12->list.array[0]->txParameters_r12) {
        *slss_id = UE_rrc_inst[ctxt_pP->module_id].sib18[eNB_index]->commConfig_r12->commSyncConfig_r12->list.array[0]->slssid_r12;
        syncOffsetIndicator = UE_rrc_inst[ctxt_pP->module_id].sib18[eNB_index]->commConfig_r12->commSyncConfig_r12->list.array[0]->syncOffsetIndicator_r12;

        //if RRC_CONNECTED (Todo: and if networkControlledSyncTx (RRCConnectionReconfiguration) is configured and set to On)
        if (UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].State == RRC_CONNECTED) {
          //select subframe(s) indicated by syncOffsetIndicator
          *subframe = syncOffsetIndicator;
        } else {
          //select subframe(s) indicated by syncOffsetIndicator within SC period
        }

        break;

      case 3: //if triggered by V2X communication and in coverage
        break;

      case 4: //if triggered by V2X communication and out-of-coverage
        break;

      case 5: //if triggered by V2X communication and UE has GNSS as the synchronization reference
      default:
        //if UE has a selected SyncRefUE
        //TODO
        //else (no SyncRefUE Selected)
        //Todo  if trigger by V2X
        //else randomly select an SLSSID from the set defined for out-of-coverage
        *slss_id = 170;//hardcoded
        //select the subframe according to syncOffsetIndicator1/2 from the preconfigured parameters
        break;
      }
  }

  return 0;
}


//-----------------------------------------------------------------------------
void
rrc_ue_process_sidelink_radioResourceConfig(
  module_id_t                      Mod_idP,
  uint8_t                          eNB_index,
  LTE_SystemInformationBlockType18_r12_t     *sib18,
  LTE_SystemInformationBlockType19_r12_t     *sib19,
  LTE_SL_CommConfig_r12_t *sl_CommConfig,
  LTE_SL_DiscConfig_r12_t *sl_DiscConfig
)
//-----------------------------------------------------------------------------
{
  //process SIB18, configure MAC/PHY for receiving SL communication (RRC_IDLE and RRC_CONNECTED), for transmitting SL communication (RRC_IDLE)
  if (sib18 != NULL) {
    if (sib18->commConfig_r12 != NULL) {
      //do not consider commTXPoolExceptional for the moment
      //configure PHY/MAC to receive SL communication by using the RPs indicated by commRxPool
      //sib18->commConfig_r12->commRxPool_r12
      //TODO
      if (sib18->commConfig_r12->commTxPoolNormalCommon_r12 !=NULL) { //commTxPoolNormalCommon - to transmit SL communication in RRC_IDLE
        //maybe we don't consider this case for the moment since UE will immediately establish a RRC connection after receiving SIB messages
        //configure PHY/MAC to transmit SL communication using the RPs indicated by the first entry in commTxPoolNormalCommon
        //SL_CommResourcePool_r12_t sl_CommResourcePool = sib18->commConfig_r12->commTxPoolNormalCommon_r12->list.array[0];
      }
    }
  }

  //process SIB19, configure MAC/PHY for receiving SL discovery (RRC_IDLE and RRC_CONNECTED), for transmitting SL discovery (RRC_IDLE)
  if (sib19 != NULL) {
    //to receive non-PS related discovery announcements (discRxPool)
    //sib19->discConfig_r12->discRxPool_r12;
    //to receive PS related discovery announcements (discRxPoolPS)
    //sib19->ext1->discConfigPS_13->discRxPoolPS_r13;
    //to transmit non-PS related discovery in RRC_IDLE
    //sib19->discConfig_r12->discTxPoolCommon_r12;
    //to transmit PS related discovery in RRC_IDLE
    //sib19->ext1->discConfigPS_13->discTxPoolPS_Common_r13;
  }

  //process sl_CommConfig, configure MAC/PHY for transmitting SL communication (RRC_CONNECTED)
  if (sl_CommConfig != NULL) {
    if (sl_CommConfig->commTxResources_r12 != NULL) {
      switch (sl_CommConfig->commTxResources_r12->present) {
        case LTE_SL_CommConfig_r12__commTxResources_r12_PR_setup:
          if (sl_CommConfig->commTxResources_r12->choice.setup.present == LTE_SL_CommConfig_r12__commTxResources_r12__setup_PR_scheduled_r12 ) {
            LOG_I(RRC,"[UE %d][RRC_UE] scheduled resource for SL, sl_RNTI size %lu  \n",
                  Mod_idP, sl_CommConfig->commTxResources_r12->choice.setup.choice.scheduled_r12.sl_RNTI_r12.size );
            LOG_I(RRC,"[UE %d][RRC_UE] scheduled resource for SL, sl_RNTI buf 0x%p \n",
                  Mod_idP, sl_CommConfig->commTxResources_r12->choice.setup.choice.scheduled_r12.sl_RNTI_r12.buf );
            LOG_I(RRC,"[UE %d][RRC_UE] scheduled resource for SL, Mac_MainConfig_r12.retx_BSR_TimerSL %ld \n",
                  Mod_idP, sl_CommConfig->commTxResources_r12->choice.setup.choice.scheduled_r12.mac_MainConfig_r12.retx_BSR_TimerSL );
            LOG_I(RRC,"[UE %d][RRC_UE] scheduled resource for SL, sc_CommTxConfig %ld \n",
                  Mod_idP, sl_CommConfig->commTxResources_r12->choice.setup.choice.scheduled_r12.mac_MainConfig_r12.retx_BSR_TimerSL );
            //configure scheduled resource for SL
            //TODO
          } else if (sl_CommConfig->commTxResources_r12->choice.setup.present == LTE_SL_CommConfig_r12__commTxResources_r12__setup_PR_ue_Selected_r12) {
            //configure dedicated resources (commTxPoolNormalDedicated) for SL from which UE can autonomously select
            //sl_CommConfig->commTxResources_r12->choice.setup.choice.ue_Selected_r12.commTxPoolNormalDedicated_r12;
            //for the moment, only pass the first entry (e.g., do not consider priorityList in commTxPoolNormalDedicated (3GPP 36.331 Section 5.10.4 1>2>3>4))
            //sl_CommConfig->commTxResources_r12->choice.setup.choice.ue_Selected_r12.commTxPoolNormalDedicated_r12.poolToAddModList_r12->list.array[0];
          } else {
            //SL_CommConfig_r12__commTxResources_r12__setup_PR_NOTHING /* No components present */
          }

          break;

        case LTE_SL_CommConfig_r12__commTxResources_r12_PR_release:
          //release dedicated resources for SL communication
          break;

        case LTE_SL_CommConfig_r12__commTxResources_r12_PR_NOTHING: /* No components present */
          break;

        default:
          break;
      }
    }
  }

  //process sl_DiscConfig, configure MAC/PHY for transmitting SL discovery announcements (RRC_CONNECTED)
  if (sl_DiscConfig != NULL) {
    //dedicated resources for transmitting non-PS related discovery
    if (sl_DiscConfig->discTxResources_r12 != NULL) {
      switch (sl_DiscConfig->discTxResources_r12->present) {
        case LTE_SL_DiscConfig_r12__discTxResources_r12_PR_setup:
          if (sl_DiscConfig->discTxResources_r12->choice.setup.present == LTE_SL_DiscConfig_r12__discTxResources_r12__setup_PR_scheduled_r12) {
            //sl_DiscConfig->discTxResources_r12->choice.setup.choice.scheduled_r12.discHoppingConfig_r12;
            //sl_DiscConfig->discTxResources_r12->choice.setup.choice.scheduled_r12.discTF_IndexList_r12;
            //sl_DiscConfig->discTxResources_r12->choice.setup.choice.scheduled_r12.discTxConfig_r12;
          } else if (sl_DiscConfig->discTxResources_r12->choice.setup.present == LTE_SL_DiscConfig_r12__discTxResources_r12__setup_PR_ue_Selected_r12) {
            //sl_DiscConfig->discTxResources_r12->choice.setup.choice.ue_Selected_r12.discTxPoolDedicated_r12;
          } else {
            //SL_DiscConfig_r12__discTxResources_r12__setup_PR_NOTHING,   /* No components present */
          }

          break;

        case LTE_SL_DiscConfig_r12__discTxResources_r12_PR_release:
          //release dedicated resources for SL discovery
          break;

        case LTE_SL_DiscConfig_r12__discTxResources_r12_PR_NOTHING: /* No components present */
          break;

        default:
          break;
      }
    }

    //dedicated resources for transmitting PS related discovery
    if (sl_DiscConfig->ext2->discTxResourcesPS_r13 != NULL) {
      switch (sl_DiscConfig->ext2->discTxResourcesPS_r13->present) {
        case LTE_SL_DiscConfig_r12__ext2__discTxResourcesPS_r13_PR_setup:
          if (sl_DiscConfig->ext2->discTxResourcesPS_r13->choice.setup.present == LTE_SL_DiscConfig_r12__ext2__discTxResourcesPS_r13__setup_PR_scheduled_r13) {
            //sl_DiscConfig->ext2->discTxResourcesPS_r13->choice.setup.choice.scheduled_r13.discHoppingConfig_r13;
            //sl_DiscConfig->ext2->discTxResourcesPS_r13->choice.setup.choice.scheduled_r13.discTxConfig_r13
          } else if (sl_DiscConfig->ext2->discTxResourcesPS_r13->choice.setup.present == LTE_SL_DiscConfig_r12__ext2__discTxResourcesPS_r13__setup_PR_ue_Selected_r13) {
            //sl_DiscConfig->ext2->discTxResourcesPS_r13->choice.setup.choice.ue_Selected_r13.discTxPoolPS_Dedicated_r13;
          } else {
            //SL_DiscConfig_r12__ext2__discTxResourcesPS_r13__setup_PR_NOTHING, /* No components present */
          }

          break;

        case LTE_SL_DiscConfig_r12__ext2__discTxResourcesPS_r13_PR_release:
          break;

        case LTE_SL_DiscConfig_r12__ext2__discTxResourcesPS_r13_PR_NOTHING:
          /* No components present */
          break;

        default:
          break;
      }
    }
  }
}


//-----------------------------------------------------------
void rrc_control_socket_init() {
  struct sockaddr_in rrc_ctrl_socket_addr;
  int optval; // flag value for setsockopt
  //int n; // message byte size
  // create the control socket
  ctrl_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

  if (ctrl_sock_fd == -1) {
    LOG_E(RRC,"[rrc_control_socket_init] :Error opening socket %d (%d:%s)\n", ctrl_sock_fd, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  //   if (ctrl_sock_fd < 0)
  //      error("ERROR: Failed on opening socket");
  optval = 1;
  setsockopt(ctrl_sock_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
  //build the server's address
  bzero((char *) &rrc_ctrl_socket_addr, sizeof(rrc_ctrl_socket_addr));
  rrc_ctrl_socket_addr.sin_family = AF_INET;
  rrc_ctrl_socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  rrc_ctrl_socket_addr.sin_port = htons(CONTROL_SOCKET_PORT_NO);

  // associate the parent socket with a port
  if (bind(ctrl_sock_fd, (struct sockaddr *) &rrc_ctrl_socket_addr, sizeof(rrc_ctrl_socket_addr)) < 0) {
    LOG_E(RRC,"[rrc_control_socket_init] ERROR: Failed on binding the socket\n");
    exit(1);
  }

  pthread_t rrc_control_socket_thread;
  threadCreate(&rrc_control_socket_thread, rrc_control_socket_thread_fct, NULL, "RRC/ProSeApp", -1, OAI_PRIORITY_RT);
}

//--------------------------------------------------------
void *rrc_control_socket_thread_fct(void *arg) {
  //int optval;
  int n;
  struct sidelink_ctrl_element *sl_ctrl_msg_recv = NULL;
  struct sidelink_ctrl_element *sl_ctrl_msg_send = NULL;
  uint32_t sourceL2Id, groupL2Id, destinationL2Id;
  module_id_t         module_id = 0; //hardcoded for testing only
  uint8_t type;
  UE_RRC_INST *UE  = NULL;
  protocol_ctxt_t ctxt;
  struct LTE_RLC_Config                  *DRB_rlc_config                   = NULL;
  struct LTE_PDCP_Config                 *DRB_pdcp_config                  = NULL;
  struct LTE_PDCP_Config__rlc_UM         *PDCP_rlc_UM                      = NULL;
  struct LTE_LogicalChannelConfig        *DRB_lchan_config                 = NULL;
  struct LTE_LogicalChannelConfig__ul_SpecificParameters  *DRB_ul_SpecificParameters = NULL;
  long                               *logicalchannelgroup_drb          = NULL;
  int j = 0;
  int i = 0;
  //from the main program, listen for the incoming messages from control socket (ProSe App)

  //int enable_notification = 1;
  while (1) {
    LOG_I(RRC,"Listening to incoming connection from ProSe App \n");
    // receive a message from ProSe App
    char receive_buf[MAX_MESSAGE_SIZE];
    memset(receive_buf, 0, sizeof(receive_buf));
    socklen_t prose_addr_len = sizeof(prose_app_addr);
    n = recvfrom(ctrl_sock_fd, receive_buf, sizeof(receive_buf), MSG_TRUNC,
                 (struct sockaddr *) &prose_app_addr, &prose_addr_len);

    if (n < 0) {
      LOG_E(RRC, "ERROR: Failed to receive from ProSe App\n");
      exit(EXIT_FAILURE);
    }
    if (n == 0) {
      LOG_E(RRC, "%s(%d). EOF for ctrl_sock_fd\n", __FUNCTION__, __LINE__);
    }
    if (n > MAX_MESSAGE_SIZE) {
      LOG_E(RRC, "%s(%d). Message truncated. %d\n", __FUNCTION__, __LINE__, n);
      exit(EXIT_FAILURE);
    }

    //TODO: should store the address of ProSeApp [UE_rrc_inst] to be able to send UE state notification to the App
    //sl_ctrl_msg_recv = (struct sidelink_ctrl_element *) receive_buf;
    sl_ctrl_msg_recv = calloc(1, sizeof(struct sidelink_ctrl_element));
    memcpy((void *)sl_ctrl_msg_recv, (void *)receive_buf, sizeof(struct sidelink_ctrl_element));

    //process the message
    char send_buf[MAX_MESSAGE_SIZE];
    switch (sl_ctrl_msg_recv->type) {
      case SESSION_INIT_REQ:
        if (LOG_DEBUGFLAG(DEBUG_CTRLSOCKET)) {
          LOG_UI(RRC,"Received SessionInitializationRequest on socket from ProSe App (msg type: %d)\n", sl_ctrl_msg_recv->type);
        }

        //TODO: get SL_UE_STATE from lower layer
        LOG_I(RRC,"Send UEStateInformation to ProSe App \n");
        memset(send_buf, 0, MAX_MESSAGE_SIZE);
        sl_ctrl_msg_send = calloc(1, sizeof(struct sidelink_ctrl_element));
        sl_ctrl_msg_send->type = UE_STATUS_INFO;
        sl_ctrl_msg_send->sidelinkPrimitive.ue_state = UE_STATE_OFF_NETWORK; //off-network
        memcpy((void *)send_buf, (void *)sl_ctrl_msg_send, sizeof(struct sidelink_ctrl_element));
        free(sl_ctrl_msg_send);
        n = sendto(ctrl_sock_fd, (char *)send_buf, sizeof(struct sidelink_ctrl_element), 0,
                   (struct sockaddr *)&prose_app_addr, sizeof(prose_app_addr));

        if (n < 0) {
          LOG_E(RRC, "ERROR: Failed to send to ProSe App\n");
          exit(EXIT_FAILURE);
        }

        if (LOG_DEBUGFLAG(DEBUG_CTRLSOCKET)) {
          struct sidelink_ctrl_element *ptr_ctrl_msg = NULL;
          ptr_ctrl_msg = (struct sidelink_ctrl_element *) send_buf;
          LOG_UI(RRC,"[UEStateInformation] msg type: %d\n",ptr_ctrl_msg->type);
          LOG_UI(RRC,"[UEStateInformation] UE state: %d\n",ptr_ctrl_msg->sidelinkPrimitive.ue_state);
        }

        /*  if (enable_notification > 0) {
             //create thread to send status notification (for testing purpose, status notification will be sent e.g., every 20 seconds)
             pthread_t notification_thread;
             if( pthread_create( &notification_thread , NULL ,  send_UE_status_notification , (void*) &sockfd) < 0)
                error("ERROR: could not create thread");
          }
          enable_notification = 0;
         */
        break;

      case GROUP_COMMUNICATION_ESTABLISH_REQ:
        sourceL2Id = sl_ctrl_msg_recv->sidelinkPrimitive.group_comm_establish_req.sourceL2Id;
        groupL2Id = sl_ctrl_msg_recv->sidelinkPrimitive.group_comm_establish_req.groupL2Id;

        if (LOG_DEBUGFLAG(DEBUG_CTRLSOCKET)) {
          LOG_UI(RRC,"[GroupCommunicationEstablishReq] Received on socket from ProSe App (msg type: %d)\n",sl_ctrl_msg_recv->type);
          LOG_UI(RRC,"[GroupCommunicationEstablishReq] source Id: 0x%08x\n",sl_ctrl_msg_recv->sidelinkPrimitive.group_comm_establish_req.sourceL2Id);
          LOG_UI(RRC,"[GroupCommunicationEstablishReq] group Id: 0x%08x\n",sl_ctrl_msg_recv->sidelinkPrimitive.group_comm_establish_req.groupL2Id);
          LOG_UI(RRC,"[GroupCommunicationEstablishReq] group IP Address: " IPV4_ADDR "\n",IPV4_ADDR_FORMAT(sl_ctrl_msg_recv->sidelinkPrimitive.group_comm_establish_req.groupIpAddress));
        }

        //store sourceL2Id/groupL2Id
        UE_rrc_inst[module_id].sourceL2Id = sourceL2Id;
        UE_rrc_inst[module_id].groupL2Id = groupL2Id;
        j = 0;
        i = 0;

        for (i=0; i< MAX_NUM_DEST; i++) {
          if ((UE_rrc_inst[module_id].destinationList[i] == 0) && (j == 0)) j = i+1;

          if (UE_rrc_inst[module_id].destinationList[i] == groupL2Id) break; //group already exists!
        }

        if ((i == MAX_NUM_DEST) && (j > 0))  UE_mac_inst[module_id].destinationList[j-1] = groupL2Id;

        // configure lower layers PDCP/MAC/PHY for this communication
        //Establish a new RBID/LCID for this communication
        // Establish a SLRB (using DRB 3 for now)
        UE  = &UE_rrc_inst[module_id];
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, 0, ENB_FLAG_NO, 0x1234, 0, 0,0);
        UE->DRB_config[0][0] = CALLOC(1,sizeof(struct LTE_DRB_ToAddMod));
        UE->DRB_config[0][0]->eps_BearerIdentity = CALLOC(1, sizeof(long));
        UE->DRB_config[0][0]->drb_Identity =  3;
        UE->DRB_config[0][0]->eps_BearerIdentity = CALLOC(1, sizeof(long));
        // allowed value 5..15, value : x+4
        *(UE->DRB_config[0][0]->eps_BearerIdentity) = 3;
        UE->DRB_config[0][0]->logicalChannelIdentity = CALLOC(1, sizeof(long));
        *(UE->DRB_config[0][0]->logicalChannelIdentity) = UE->DRB_config[0][0]->drb_Identity; //(long) (ue_context_pP->ue_context.e_rab[i].param.e_rab_id + 2); // value : x+2
        DRB_rlc_config                   = CALLOC(1,sizeof(struct LTE_RLC_Config));
        DRB_pdcp_config                  = CALLOC(1,sizeof(struct LTE_PDCP_Config));
        PDCP_rlc_UM                      = CALLOC(1,sizeof(struct LTE_PDCP_Config__rlc_UM));
        DRB_lchan_config                 = CALLOC(1,sizeof(struct LTE_LogicalChannelConfig));
        DRB_ul_SpecificParameters                                         = CALLOC(1, sizeof(struct LTE_LogicalChannelConfig__ul_SpecificParameters));
        logicalchannelgroup_drb          = CALLOC(1, sizeof(long));
        DRB_rlc_config->present = LTE_RLC_Config_PR_um_Bi_Directional;
        DRB_rlc_config->choice.um_Bi_Directional.ul_UM_RLC.sn_FieldLength = LTE_SN_FieldLength_size10;
        DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.sn_FieldLength = LTE_SN_FieldLength_size10;
        DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.t_Reordering = LTE_T_Reordering_ms35;
        UE->DRB_config[0][0]->rlc_Config = DRB_rlc_config;
        DRB_pdcp_config = CALLOC(1, sizeof(*DRB_pdcp_config));
        UE->DRB_config[0][0]->pdcp_Config = DRB_pdcp_config;
        DRB_pdcp_config->discardTimer = CALLOC(1, sizeof(long));
        *DRB_pdcp_config->discardTimer = LTE_PDCP_Config__discardTimer_infinity;
        DRB_pdcp_config->rlc_AM = NULL;
        DRB_pdcp_config->rlc_UM = NULL;
        /* avoid gcc warnings */
        (void)PDCP_rlc_UM;
        DRB_pdcp_config->rlc_UM = PDCP_rlc_UM;
        PDCP_rlc_UM->pdcp_SN_Size = LTE_PDCP_Config__rlc_UM__pdcp_SN_Size_len12bits;
        DRB_pdcp_config->headerCompression.present = LTE_PDCP_Config__headerCompression_PR_notUsed;
        UE->DRB_config[0][0]->logicalChannelConfig = DRB_lchan_config;
        DRB_ul_SpecificParameters = CALLOC(1, sizeof(*DRB_ul_SpecificParameters));
        DRB_lchan_config->ul_SpecificParameters = DRB_ul_SpecificParameters;
        DRB_ul_SpecificParameters->priority = 12;    // lower priority than srb1, srb2 and other dedicated bearer
        DRB_ul_SpecificParameters->prioritisedBitRate =LTE_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps8 ;
        //LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
        DRB_ul_SpecificParameters->bucketSizeDuration =
          LTE_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;
        // LCG for DTCH can take the value from 1 to 3 as defined in 36331: normally controlled by upper layers (like RRM)
        *logicalchannelgroup_drb = 1;
        DRB_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup_drb;
        UE->DRB_configList = CALLOC(1,sizeof(LTE_DRB_ToAddModList_t));
        ASN_SEQUENCE_ADD(&UE->DRB_configList->list,UE->DRB_config[0][0]);
        rrc_pdcp_config_asn1_req(&ctxt,
                                 (LTE_SRB_ToAddModList_t *) NULL,
                                 UE->DRB_configList,
                                 (LTE_DRB_ToReleaseList_t *) NULL,
                                 0xff, NULL, NULL, NULL,
                                 (LTE_PMCH_InfoList_r9_t *) NULL
                                 ,NULL);
        rrc_rlc_config_asn1_req(&ctxt,
                                (LTE_SRB_ToAddModList_t *)NULL,
                                UE->DRB_configList,
                                (LTE_DRB_ToReleaseList_t *)NULL,
                                (LTE_PMCH_InfoList_r9_t *)NULL
                                , 0, 0
                               );
        rrc_rlc_config_asn1_req(&ctxt,
                                (LTE_SRB_ToAddModList_t *)NULL,
                                UE->DRB_configList,
                                (LTE_DRB_ToReleaseList_t *)NULL,
                                (LTE_PMCH_InfoList_r9_t *)NULL,
                                sourceL2Id, groupL2Id
                               );
        //configure MAC with sourceL2Id/groupL2ID
        rrc_mac_config_req_ue(module_id,0,0, //eNB_index =0
                              (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                              (struct LTE_PhysicalConfigDedicated *)NULL,
                              (LTE_SCellToAddMod_r10_t *)NULL,
                              (LTE_MeasObjectToAddMod_t **)NULL,
                              (LTE_MAC_MainConfig_t *)NULL,
                              3, //LCID
                              (struct LTE_LogicalChannelConfig *)NULL,
                              (LTE_MeasGapConfig_t *)NULL,
                              (LTE_TDD_Config_t *)NULL,
                              (LTE_MobilityControlInfo_t *)NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,0,
                              (LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                              (LTE_PMCH_InfoList_r9_t *)NULL,
                              CONFIG_ACTION_ADD,
                              &sourceL2Id,
                              &groupL2Id,
                              0,
                              (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                              (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                             );
        LOG_I(RRC,"Send GroupCommunicationEstablishResp to ProSe App\n");
        memset(send_buf, 0, MAX_MESSAGE_SIZE);
        sl_ctrl_msg_send = calloc(1, sizeof(struct sidelink_ctrl_element));
        sl_ctrl_msg_send->type = GROUP_COMMUNICATION_ESTABLISH_RSP;
        sl_ctrl_msg_send->sidelinkPrimitive.slrb_id = 3; //slrb_id
        memcpy((void *)send_buf, (void *)sl_ctrl_msg_send, sizeof(struct sidelink_ctrl_element));
        free(sl_ctrl_msg_send);
        n = sendto(ctrl_sock_fd, (char *)send_buf, sizeof(struct sidelink_ctrl_element), 0,
                   (struct sockaddr *)&prose_app_addr, sizeof(prose_app_addr));

        if (n < 0) {
          LOG_E(RRC, "ERROR: Failed to send to ProSe App\n");
          exit(EXIT_FAILURE);
        }

        if (LOG_DEBUGFLAG(DEBUG_CTRLSOCKET)) {
          struct sidelink_ctrl_element *ptr_ctrl_msg = NULL;
          ptr_ctrl_msg = (struct sidelink_ctrl_element *) send_buf;
          LOG_UI(RRC,"[GroupCommunicationEstablishResponse]  msg type: %d\n",ptr_ctrl_msg->type);
          LOG_UI(RRC,"[GroupCommunicationEstablishResponse]  slrb_id: %d\n",ptr_ctrl_msg->sidelinkPrimitive.slrb_id);
        }

        break;

      case GROUP_COMMUNICATION_RELEASE_REQ:
        printf("-----------------------------------\n");

        if (LOG_DEBUGFLAG(DEBUG_CTRLSOCKET)) {
          LOG_UI(RRC,"[GroupCommunicationReleaseRequest] Received on socket from ProSe App (msg type: %d)\n",sl_ctrl_msg_recv->type);
          LOG_UI(RRC,"[GroupCommunicationReleaseRequest] Slrb Id: %i\n",sl_ctrl_msg_recv->sidelinkPrimitive.slrb_id);
        }

        //reset groupL2ID from MAC LAYER
        UE_rrc_inst[module_id].groupL2Id = 0x00000000;
        sourceL2Id = UE_rrc_inst[module_id].sourceL2Id;
        rrc_mac_config_req_ue(module_id,0,0, //eNB_index =0
                              (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                              (struct LTE_PhysicalConfigDedicated *)NULL,
                              (LTE_SCellToAddMod_r10_t *)NULL,
                              (LTE_MeasObjectToAddMod_t **)NULL,
                              (LTE_MAC_MainConfig_t *)NULL,
                              0,
                              (struct LTE_LogicalChannelConfig *)NULL,
                              (LTE_MeasGapConfig_t *)NULL,
                              (LTE_TDD_Config_t *)NULL,
                              (LTE_MobilityControlInfo_t *)NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,0,
                              (LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                              (LTE_PMCH_InfoList_r9_t *)NULL
                              ,CONFIG_ACTION_REMOVE,
                              &sourceL2Id,
                              &destinationL2Id,
                              0,
                              (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                              (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                             );
        LOG_I(RRC,"Send GroupCommunicationReleaseResponse to ProSe App \n");
        memset(send_buf, 0, MAX_MESSAGE_SIZE);
        sl_ctrl_msg_send = calloc(1, sizeof(struct sidelink_ctrl_element));
        sl_ctrl_msg_send->type = GROUP_COMMUNICATION_RELEASE_RSP;

        //if the requested id exists -> release this ID
        if (sl_ctrl_msg_recv->sidelinkPrimitive.slrb_id == slrb_id) {
          sl_ctrl_msg_send->sidelinkPrimitive.group_comm_release_rsp = GROUP_COMMUNICATION_RELEASE_OK;
          slrb_id = 0; //Reset slrb_id
        } else {
          sl_ctrl_msg_send->sidelinkPrimitive.group_comm_release_rsp = GROUP_COMMUNICATION_RELEASE_FAILURE;
        }

        memcpy((void *)send_buf, (void *)sl_ctrl_msg_send, sizeof(struct sidelink_ctrl_element));
        free(sl_ctrl_msg_send);
        n = sendto(ctrl_sock_fd, (char *)send_buf, sizeof(struct sidelink_ctrl_element), 0,
                   (struct sockaddr *)&prose_app_addr, sizeof(prose_app_addr));

        if (n < 0) {
          LOG_E(RRC, "ERROR: Failed to send to ProSe App\n");
          exit(EXIT_FAILURE);
        }

        break;

      case DIRECT_COMMUNICATION_ESTABLISH_REQ:
        sourceL2Id = sl_ctrl_msg_recv->sidelinkPrimitive.direct_comm_establish_req.sourceL2Id;
        destinationL2Id = sl_ctrl_msg_recv->sidelinkPrimitive.direct_comm_establish_req.destinationL2Id;

        if (LOG_DEBUGFLAG(DEBUG_CTRLSOCKET)) {
          LOG_UI(RRC,"[DirectCommunicationEstablishReq] Received on socket from ProSe App (msg type: %d)\n",sl_ctrl_msg_recv->type);
          LOG_UI(RRC,"[DirectCommunicationEstablishReq] source Id: 0x%08x\n",sl_ctrl_msg_recv->sidelinkPrimitive.group_comm_establish_req.sourceL2Id);
          LOG_UI(RRC,"[DirectCommunicationEstablishReq] destination Id: 0x%08x\n",sl_ctrl_msg_recv->sidelinkPrimitive.group_comm_establish_req.groupL2Id);
        }

        //store sourceL2Id/destinationL2Id
        UE_rrc_inst[module_id].sourceL2Id = sourceL2Id;
        i = 0;
        j = 0;

        for (i=0; i< MAX_NUM_DEST; i++) {
          if ((UE_rrc_inst[module_id].destinationList[i] == 0) && (j == 0)) j = i+1;

          if (UE_rrc_inst[module_id].destinationList[i] == destinationL2Id) break; //destination already exists!
        }

        if ((i == MAX_NUM_DEST) && (j > 0))  UE_mac_inst[module_id].destinationList[j-1] = destinationL2Id;

        // configure lower layers PDCP/MAC/PHY for this communication
        //Establish a new RBID/LCID for this communication
        // Establish a SLRB (using DRB 3 for now)
        UE  = &UE_rrc_inst[module_id];
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, 0, ENB_FLAG_NO, 0x1234, 0, 0,0);
        UE->DRB_config[0][0] = CALLOC(1,sizeof(struct LTE_DRB_ToAddMod));
        UE->DRB_config[0][0]->eps_BearerIdentity = CALLOC(1, sizeof(long));
        UE->DRB_config[0][0]->drb_Identity =  3;
        UE->DRB_config[0][0]->eps_BearerIdentity = CALLOC(1, sizeof(long));
        // allowed value 5..15, value : x+4
        *(UE->DRB_config[0][0]->eps_BearerIdentity) = 3;
        UE->DRB_config[0][0]->logicalChannelIdentity = CALLOC(1, sizeof(long));
        *(UE->DRB_config[0][0]->logicalChannelIdentity) = UE->DRB_config[0][0]->drb_Identity; //(long) (ue_context_pP->ue_context.e_rab[i].param.e_rab_id + 2); // value : x+2
        DRB_rlc_config                   = CALLOC(1,sizeof(struct LTE_RLC_Config));
        DRB_pdcp_config                  = CALLOC(1,sizeof(struct LTE_PDCP_Config));
        PDCP_rlc_UM                      = CALLOC(1,sizeof(struct LTE_PDCP_Config__rlc_UM));
        DRB_lchan_config                 = CALLOC(1,sizeof(struct LTE_LogicalChannelConfig));
        DRB_ul_SpecificParameters                                         = CALLOC(1, sizeof(struct LTE_LogicalChannelConfig__ul_SpecificParameters));
        logicalchannelgroup_drb          = CALLOC(1, sizeof(long));
        DRB_rlc_config->present = LTE_RLC_Config_PR_um_Bi_Directional;
        DRB_rlc_config->choice.um_Bi_Directional.ul_UM_RLC.sn_FieldLength = LTE_SN_FieldLength_size10;
        DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.sn_FieldLength = LTE_SN_FieldLength_size10;
        DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.t_Reordering = LTE_T_Reordering_ms35;
        UE->DRB_config[0][0]->rlc_Config = DRB_rlc_config;
        DRB_pdcp_config = CALLOC(1, sizeof(*DRB_pdcp_config));
        UE->DRB_config[0][0]->pdcp_Config = DRB_pdcp_config;
        DRB_pdcp_config->discardTimer = CALLOC(1, sizeof(long));
        *DRB_pdcp_config->discardTimer = LTE_PDCP_Config__discardTimer_infinity;
        DRB_pdcp_config->rlc_AM = NULL;
        DRB_pdcp_config->rlc_UM = NULL;
        /* avoid gcc warnings */
        (void)PDCP_rlc_UM;
        DRB_pdcp_config->rlc_UM = PDCP_rlc_UM;
        PDCP_rlc_UM->pdcp_SN_Size = LTE_PDCP_Config__rlc_UM__pdcp_SN_Size_len12bits;
        DRB_pdcp_config->headerCompression.present = LTE_PDCP_Config__headerCompression_PR_notUsed;
        UE->DRB_config[0][0]->logicalChannelConfig = DRB_lchan_config;
        DRB_ul_SpecificParameters = CALLOC(1, sizeof(*DRB_ul_SpecificParameters));
        DRB_lchan_config->ul_SpecificParameters = DRB_ul_SpecificParameters;
        DRB_ul_SpecificParameters->priority = 12;    // lower priority than srb1, srb2 and other dedicated bearer
        DRB_ul_SpecificParameters->prioritisedBitRate =LTE_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps8 ;
        //LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
        DRB_ul_SpecificParameters->bucketSizeDuration =
          LTE_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;
        // LCG for DTCH can take the value from 1 to 3 as defined in 36331: normally controlled by upper layers (like RRM)
        *logicalchannelgroup_drb = 1;
        DRB_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup_drb;
        UE->DRB_configList = CALLOC(1,sizeof(LTE_DRB_ToAddModList_t));
        ASN_SEQUENCE_ADD(&UE->DRB_configList->list,UE->DRB_config[0][0]);
        rrc_pdcp_config_asn1_req(&ctxt,
                                 (LTE_SRB_ToAddModList_t *) NULL,
                                 UE->DRB_configList,
                                 (LTE_DRB_ToReleaseList_t *) NULL,
                                 0xff, NULL, NULL, NULL,
                                 (LTE_PMCH_InfoList_r9_t *) NULL,NULL);
        rrc_rlc_config_asn1_req(&ctxt,
                                (LTE_SRB_ToAddModList_t *)NULL,
                                UE->DRB_configList,
                                (LTE_DRB_ToReleaseList_t *)NULL,
                                (LTE_PMCH_InfoList_r9_t *)NULL
                                , 0, 0
                               );
        rrc_rlc_config_asn1_req(&ctxt,
                                (LTE_SRB_ToAddModList_t *)NULL,
                                UE->DRB_configList,
                                (LTE_DRB_ToReleaseList_t *)NULL,
                                (LTE_PMCH_InfoList_r9_t *)NULL
                                , sourceL2Id, destinationL2Id
                               );
        //configure MAC with sourceL2Id/destinationL2Id
        rrc_mac_config_req_ue(module_id,0,0, //eNB_index =0
                              (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                              (struct LTE_PhysicalConfigDedicated *)NULL,
                              (LTE_SCellToAddMod_r10_t *)NULL,
                              (LTE_MeasObjectToAddMod_t **)NULL,
                              (LTE_MAC_MainConfig_t *)NULL,
                              3, //LCID
                              (struct LTE_LogicalChannelConfig *)NULL,
                              (LTE_MeasGapConfig_t *)NULL,
                              (LTE_TDD_Config_t *)NULL,
                              (LTE_MobilityControlInfo_t *)NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,0,
                              (LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                              (LTE_PMCH_InfoList_r9_t *)NULL,
                              CONFIG_ACTION_ADD,
                              &sourceL2Id,
                              &destinationL2Id,
                              0,
                              (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                              (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                             );
        LOG_I(RRC,"Send DirectCommunicationEstablishResp to ProSe App\n");
        memset(send_buf, 0, MAX_MESSAGE_SIZE);
        sl_ctrl_msg_send = calloc(1, sizeof(struct sidelink_ctrl_element));
        sl_ctrl_msg_send->type = DIRECT_COMMUNICATION_ESTABLISH_RSP;
        sl_ctrl_msg_send->sidelinkPrimitive.slrb_id = 3; //slrb_id
        memcpy((void *)send_buf, (void *)sl_ctrl_msg_send, sizeof(struct sidelink_ctrl_element));
        free(sl_ctrl_msg_send);
        n = sendto(ctrl_sock_fd, (char *)send_buf, sizeof(struct sidelink_ctrl_element), 0,
                   (struct sockaddr *)&prose_app_addr, sizeof(prose_app_addr));

        if (n < 0) {
          LOG_E(RRC, "ERROR: Failed to send to ProSe App\n");
          exit(EXIT_FAILURE);
        }

        if (LOG_DEBUGFLAG(DEBUG_CTRLSOCKET)) {
          struct sidelink_ctrl_element *ptr_ctrl_msg = NULL;
          ptr_ctrl_msg = (struct sidelink_ctrl_element *) send_buf;
          LOG_UI(RRC,"[DirectCommunicationEstablishResponse]  msg type: %d\n",ptr_ctrl_msg->type);
          LOG_UI(RRC,"[DirectCommunicationEstablishResponse]  slrb_id: %d\n",ptr_ctrl_msg->sidelinkPrimitive.slrb_id);
        }

        break;

      case PC5S_ESTABLISH_REQ:
        type =  sl_ctrl_msg_recv->sidelinkPrimitive.pc5s_establish_req.type;
        sourceL2Id = sl_ctrl_msg_recv->sidelinkPrimitive.pc5s_establish_req.sourceL2Id;

        if (LOG_DEBUGFLAG(DEBUG_CTRLSOCKET)) {
          LOG_UI(RRC,"[PC5EstablishReq] Received on socket from ProSe App (msg type: %d)\n",sl_ctrl_msg_recv->type);
          LOG_UI(RRC,"[PC5EstablishReq] type: %d\n",sl_ctrl_msg_recv->sidelinkPrimitive.pc5s_establish_req.type); //RX/TX
          LOG_UI(RRC,"[PC5EstablishReq] source Id: 0x%08x \n",sl_ctrl_msg_recv->sidelinkPrimitive.pc5s_establish_req.sourceL2Id);
        }

        if (type > 0) {
          destinationL2Id = sl_ctrl_msg_recv->sidelinkPrimitive.pc5s_establish_req.destinationL2Id;

          if (LOG_DEBUGFLAG(DEBUG_CTRLSOCKET)) {
            LOG_UI(RRC,"[PC5EstablishReq] destination Id: 0x%08x \n",sl_ctrl_msg_recv->sidelinkPrimitive.pc5s_establish_req.destinationL2Id);
          }
        }

        //store sourceL2Id/destinationL2Id
        if (type > 0) { //TX
          UE_rrc_inst[module_id].sourceL2Id = sourceL2Id;
          j = 0;
          i = 0;

          for (i=0; i< MAX_NUM_DEST; i++) {
            if ((UE_rrc_inst[module_id].destinationList[i] == 0) && (j == 0)) j = i+1;

            if (UE_rrc_inst[module_id].destinationList[i] == destinationL2Id) break; //group already exists!
          }

          if ((i == MAX_NUM_DEST) && (j > 0))  UE_mac_inst[module_id].destinationList[j-1] = destinationL2Id;
        } else {//RX
          UE_rrc_inst[module_id].sourceL2Id = sourceL2Id;
        }

        // configure lower layers PDCP/MAC/PHY for this communication
        //Establish a new RBID/LCID for this communication
        // Establish a SLRB (using DRB 10 for now)
        UE  = &UE_rrc_inst[module_id];
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, 0, ENB_FLAG_NO, 0x1234, 0, 0,0);
        UE->DRB_config[0][0] = CALLOC(1,sizeof(struct LTE_DRB_ToAddMod));
        UE->DRB_config[0][0]->eps_BearerIdentity = CALLOC(1, sizeof(long));
        UE->DRB_config[0][0]->drb_Identity =  10;
        UE->DRB_config[0][0]->eps_BearerIdentity = CALLOC(1, sizeof(long));
        // allowed value 5..15, value : x+4
        *(UE->DRB_config[0][0]->eps_BearerIdentity) = 10;
        UE->DRB_config[0][0]->logicalChannelIdentity = CALLOC(1, sizeof(long));
        *(UE->DRB_config[0][0]->logicalChannelIdentity) = UE->DRB_config[0][0]->drb_Identity; //(long) (ue_context_pP->ue_context.e_rab[i].param.e_rab_id + 2); // value : x+2
        DRB_rlc_config                   = CALLOC(1,sizeof(struct LTE_RLC_Config));
        DRB_pdcp_config                  = CALLOC(1,sizeof(struct LTE_PDCP_Config));
        PDCP_rlc_UM                      = CALLOC(1,sizeof(struct LTE_PDCP_Config__rlc_UM));
        DRB_lchan_config                 = CALLOC(1,sizeof(struct LTE_LogicalChannelConfig));
        DRB_ul_SpecificParameters                                         = CALLOC(1, sizeof(struct LTE_LogicalChannelConfig__ul_SpecificParameters));
        logicalchannelgroup_drb          = CALLOC(1, sizeof(long));
        DRB_rlc_config->present = LTE_RLC_Config_PR_um_Bi_Directional;
        DRB_rlc_config->choice.um_Bi_Directional.ul_UM_RLC.sn_FieldLength = LTE_SN_FieldLength_size10;
        DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.sn_FieldLength = LTE_SN_FieldLength_size10;
        DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.t_Reordering = LTE_T_Reordering_ms35;
        UE->DRB_config[0][0]->rlc_Config = DRB_rlc_config;
        DRB_pdcp_config = CALLOC(1, sizeof(*DRB_pdcp_config));
        UE->DRB_config[0][0]->pdcp_Config = DRB_pdcp_config;
        DRB_pdcp_config->discardTimer = CALLOC(1, sizeof(long));
        *DRB_pdcp_config->discardTimer = LTE_PDCP_Config__discardTimer_infinity;
        DRB_pdcp_config->rlc_AM = NULL;
        DRB_pdcp_config->rlc_UM = NULL;
        /* avoid gcc warnings */
        (void)PDCP_rlc_UM;
        DRB_pdcp_config->rlc_UM = PDCP_rlc_UM;
        PDCP_rlc_UM->pdcp_SN_Size = LTE_PDCP_Config__rlc_UM__pdcp_SN_Size_len12bits;
        DRB_pdcp_config->headerCompression.present = LTE_PDCP_Config__headerCompression_PR_notUsed;
        UE->DRB_config[0][0]->logicalChannelConfig = DRB_lchan_config;
        DRB_ul_SpecificParameters = CALLOC(1, sizeof(*DRB_ul_SpecificParameters));
        DRB_lchan_config->ul_SpecificParameters = DRB_ul_SpecificParameters;
        DRB_ul_SpecificParameters->priority = 12;    // lower priority than srb1, srb2 and other dedicated bearer
        DRB_ul_SpecificParameters->prioritisedBitRate = LTE_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps8 ;
        //LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
        DRB_ul_SpecificParameters->bucketSizeDuration =
          LTE_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;
        // LCG for DTCH can take the value from 1 to 3 as defined in 36331: normally controlled by upper layers (like RRM)
        *logicalchannelgroup_drb = 1;
        DRB_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup_drb;
        UE->DRB_configList = CALLOC(1,sizeof(LTE_DRB_ToAddModList_t));
        ASN_SEQUENCE_ADD(&UE->DRB_configList->list,UE->DRB_config[0][0]);
        rrc_pdcp_config_asn1_req(&ctxt,
                                 (LTE_SRB_ToAddModList_t *) NULL,
                                 UE->DRB_configList,
                                 (LTE_DRB_ToReleaseList_t *) NULL,
                                 0xff, NULL, NULL, NULL,
                                 (LTE_PMCH_InfoList_r9_t *) NULL,NULL);
        rrc_rlc_config_asn1_req(&ctxt,
                                (LTE_SRB_ToAddModList_t *)NULL,
                                UE->DRB_configList,
                                (LTE_DRB_ToReleaseList_t *)NULL,
                                (LTE_PMCH_InfoList_r9_t *)NULL, 0, 0
                               );

        //TX
        if (type > 0) {
          rrc_rlc_config_asn1_req(&ctxt,
                                  (LTE_SRB_ToAddModList_t *)NULL,
                                  UE->DRB_configList,
                                  (LTE_DRB_ToReleaseList_t *)NULL,
                                  (LTE_PMCH_InfoList_r9_t *)NULL,
                                  sourceL2Id, destinationL2Id
                                 );
          //configure MAC with sourceL2Id/groupL2ID
          rrc_mac_config_req_ue(module_id,0,0, //eNB_index =0
                                (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                                (struct LTE_PhysicalConfigDedicated *)NULL,
                                (LTE_SCellToAddMod_r10_t *)NULL,
                                (LTE_MeasObjectToAddMod_t **)NULL,
                                (LTE_MAC_MainConfig_t *)NULL,
                                10, //LCID
                                (struct LTE_LogicalChannelConfig *)NULL,
                                (LTE_MeasGapConfig_t *)NULL,
                                (LTE_TDD_Config_t *)NULL,
                                (LTE_MobilityControlInfo_t *)NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                0,
                                (LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                                (LTE_PMCH_InfoList_r9_t *)NULL,
                                CONFIG_ACTION_ADD,
                                &sourceL2Id,
                                &destinationL2Id,
                                0,
                                (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                                (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                               );
        } else {//RX
          //configure MAC with sourceL2Id/groupL2ID
          rrc_mac_config_req_ue(module_id,0,0, //eNB_index =0
                                (LTE_RadioResourceConfigCommonSIB_t *)NULL,
                                (struct LTE_PhysicalConfigDedicated *)NULL,
                                (LTE_SCellToAddMod_r10_t *)NULL,
                                (LTE_MeasObjectToAddMod_t **)NULL,
                                (LTE_MAC_MainConfig_t *)NULL,
                                10, //LCID
                                (struct LTE_LogicalChannelConfig *)NULL,
                                (LTE_MeasGapConfig_t *)NULL,
                                (LTE_TDD_Config_t *)NULL,
                                (LTE_MobilityControlInfo_t *)NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,0,
                                (LTE_MBSFN_AreaInfoList_r9_t *)NULL,
                                (LTE_PMCH_InfoList_r9_t *)NULL,
                                CONFIG_ACTION_ADD,
                                &sourceL2Id,
                                NULL,
                                0,
                                (struct LTE_NonMBSFN_SubframeConfig_r14 *)NULL,
                                (LTE_MBSFN_AreaInfoList_r9_t *)NULL
                               );
        }

        LOG_I(RRC,"Send PC5EstablishRsp to ProSe App\n");
        memset(send_buf, 0, MAX_MESSAGE_SIZE);
        sl_ctrl_msg_send = calloc(1, sizeof(struct sidelink_ctrl_element));
        sl_ctrl_msg_send->type = PC5S_ESTABLISH_RSP;
        sl_ctrl_msg_send->sidelinkPrimitive.pc5s_establish_rsp.slrbid_lcid28 = 10;
        sl_ctrl_msg_send->sidelinkPrimitive.pc5s_establish_rsp.slrbid_lcid29 = 10;
        sl_ctrl_msg_send->sidelinkPrimitive.pc5s_establish_rsp.slrbid_lcid30 = 10;
        memcpy((void *)send_buf, (void *)sl_ctrl_msg_send, sizeof(struct sidelink_ctrl_element));
        n = sendto(ctrl_sock_fd, (char *)send_buf, sizeof(struct sidelink_ctrl_element), 0,
                   (struct sockaddr *)&prose_app_addr, sizeof(prose_app_addr));

        //         free(sl_ctrl_msg_send);
        if (n < 0) {
          LOG_E(RRC, "ERROR: Failed to send to ProSe App\n");
          exit(EXIT_FAILURE);
        }

        break;

      case PC5_DISCOVERY_MESSAGE:
        if (LOG_DEBUGFLAG(DEBUG_CTRLSOCKET)) {
          LOG_UI(RRC,"[PC5DiscoveryMessage] Received on socket from ProSe App (msg type: %d)\n",sl_ctrl_msg_recv->type);
        }

        //prepare SL_Discovery buffer
        if (UE_rrc_inst) {
          memcpy((void *)&UE_rrc_inst[module_id].SL_Discovery[0].Tx_buffer.Payload[0], (void *)&sl_ctrl_msg_recv->sidelinkPrimitive.pc5_discovery_message.payload[0], PC5_DISCOVERY_PAYLOAD_SIZE);
          UE_rrc_inst[module_id].SL_Discovery[0].Tx_buffer.payload_size = PC5_DISCOVERY_PAYLOAD_SIZE;
          LOG_I(RRC,"[PC5DiscoveryMessage] Copied %d bytes\n",PC5_DISCOVERY_PAYLOAD_SIZE);
        }

        break;

      default:
        break;
    }
  }

  free (sl_ctrl_msg_recv);
  return 0;
}


//-----------------------------------------------------------------------------
int decode_SL_Discovery_Message(
  const protocol_ctxt_t *const ctxt_pP,
  const uint8_t                eNB_index,
  const uint8_t               *Sdu,
  const uint8_t                Sdu_len) {
  char send_buf[MAX_MESSAGE_SIZE];
  int n;
  struct sidelink_ctrl_element *sl_ctrl_msg_send = NULL;
  //from the main program, listen for the incoming messages from control socket (ProSe App)
  //Store in Rx_buffer
  memcpy((void *)&UE_rrc_inst[ctxt_pP->module_id].SL_Discovery[0].Rx_buffer.Payload[0], (void *)Sdu, Sdu_len);
  UE_rrc_inst[ctxt_pP->module_id].SL_Discovery[0].Rx_buffer.payload_size = Sdu_len;
  memset(send_buf, 0, MAX_MESSAGE_SIZE);
  //send to ProSeApp
  memcpy((void *)send_buf, (void *)Sdu, Sdu_len);
  sl_ctrl_msg_send = calloc(1, sizeof(struct sidelink_ctrl_element));
  sl_ctrl_msg_send->type = PC5_DISCOVERY_MESSAGE;
  // TODO:  Add a check for the SDU size.
  memcpy((void *)&sl_ctrl_msg_send->sidelinkPrimitive.pc5_discovery_message.payload[0], (void *) Sdu,  PC5_DISCOVERY_PAYLOAD_SIZE);
  memcpy((void *)send_buf, (void *)sl_ctrl_msg_send, sizeof(struct sidelink_ctrl_element));
  free(sl_ctrl_msg_send);
  n = sendto(ctrl_sock_fd, (char *)send_buf, sizeof(struct sidelink_ctrl_element), 0,
             (struct sockaddr *)&prose_app_addr, sizeof(prose_app_addr));

  if (n < 0) {
    // TODO:  We should not just exit if the Prose App has not yet attached.  It creates a race condition.
    LOG_I(RRC, "ERROR: Failed to send to ProSe App\n");
    //exit(EXIT_FAILURE);
  }

  return(0);
}


//-----------------------------------------------------------------------------
RRC_status_t
rrc_rx_tx_ue(
  protocol_ctxt_t *const ctxt_pP,
  const uint8_t      enb_indexP,
  const int          CC_id
)
//-----------------------------------------------------------------------------
{
#ifdef LOCALIZATION
  double                         estimated_distance;
  protocol_ctxt_t                ctxt;
#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_IN);

  // check timers

  if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T300_active == 1) {
    if ((UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T300_cnt % 10) == 0)
      LOG_D(RRC,
            "[UE %d][RAPROC] Frame %d T300 Count %d ms\n", ctxt_pP->module_id, ctxt_pP->frame, UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T300_cnt);

    if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T300_cnt
        == T300[UE_rrc_inst[ctxt_pP->module_id].sib2[enb_indexP]->ue_TimersAndConstants.t300]) {
      UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T300_active = 0;
      // ALLOW CCCH to be used
      UE_rrc_inst[ctxt_pP->module_id].Srb0[enb_indexP].Tx_buffer.payload_size = 0;
      rrc_ue_generate_RRCConnectionRequest (ctxt_pP, enb_indexP);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_OUT);
      return (RRC_ConnSetup_failed);
    }

    UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T300_cnt++;
  }

  if ((UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].SIStatus&2)>0) {
    if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].N310_cnt
        == N310[UE_rrc_inst[ctxt_pP->module_id].sib2[enb_indexP]->ue_TimersAndConstants.n310]) {
      LOG_I(RRC,"Activating T310\n");
      UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_active = 1;
    }
  } else { // in case we have not received SIB2 yet
    /*      if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].N310_cnt == 100) {
      UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].N310_cnt = 0;

    }*/
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_OUT);
    return RRC_OK;
  }

  if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_active == 1) {
    if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].N311_cnt
        == N311[UE_rrc_inst[ctxt_pP->module_id].sib2[enb_indexP]->ue_TimersAndConstants.n311]) {
      UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_active = 0;
      UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].N311_cnt = 0;
    }

    if ((UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_cnt % 10) == 0) {
      LOG_D(RRC, "[UE %d] Frame %d T310 Count %d ms\n", ctxt_pP->module_id, ctxt_pP->frame, UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_cnt);
    }

    if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_cnt    == T310[UE_rrc_inst[ctxt_pP->module_id].sib2[enb_indexP]->ue_TimersAndConstants.t310]) {
      UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_active = 0;
      rrc_t310_expiration (ctxt_pP, enb_indexP);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_OUT);
      LOG_I(RRC,"Returning RRC_PHY_RESYNCH: T310 expired\n");
      return RRC_PHY_RESYNCH;
    }

    UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_cnt++;
  }

  if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T304_active==1) {
    if ((UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T304_cnt % 10) == 0)
      LOG_D(RRC,"[UE %d][RAPROC] Frame %d T304 Count %d ms\n",ctxt_pP->module_id,ctxt_pP->frame,
            UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T304_cnt);

    if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T304_cnt == 0) {
      UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T304_active = 0;
      UE_rrc_inst[ctxt_pP->module_id].HandoverInfoUe.measFlag = 1;
      LOG_E(RRC,"[UE %d] Handover failure..initiating connection re-establishment procedure... \n",
            ctxt_pP->module_id);
      //Implement 36.331, section 5.3.5.6 here
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_OUT);
      return(RRC_Handover_failed);
    }

    UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T304_cnt--;
  }

  // Layer 3 filtering of RRC measurements
  if (UE_rrc_inst[ctxt_pP->module_id].QuantityConfig[0] != NULL) {
    ue_meas_filtering(ctxt_pP,enb_indexP);
  }

  ue_measurement_report_triggering(ctxt_pP,enb_indexP);

  if (UE_rrc_inst[ctxt_pP->module_id].Info[0].handoverTarget > 0) {
    LOG_I(RRC,"[UE %d] Frame %d : RRC handover initiated\n", ctxt_pP->module_id, ctxt_pP->frame);
  }

  if((UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].State == RRC_HO_EXECUTION)   &&
      (UE_rrc_inst[ctxt_pP->module_id].HandoverInfoUe.targetCellId != 0xFF)) {
    UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].State= RRC_IDLE;
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_OUT);
    return(RRC_HO_STARTED);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_OUT);
  return (RRC_OK);
}

void *recv_msgs_from_nr_ue(void *args_p)
{
    itti_mark_task_ready (TASK_RRC_NSA_UE);
    for (;;)
    {
        nsa_msg_t msg;
        int recvLen = recvfrom(from_nr_ue_fd, &msg, sizeof(msg),
                               MSG_WAITALL | MSG_TRUNC, NULL, NULL);
        if (recvLen == -1)
        {
            LOG_E(RRC, "%s: recvfrom: %s\n", __func__, strerror(errno));
            continue;
        }
        if (recvLen > sizeof(msg))
        {
            LOG_E(NR_RRC, "%s: Received a truncated message %d\n", __func__, recvLen);
            continue;
        }
        LOG_D(RRC, "We have received a %d msg (%d bytes). Calling process_nr_nsa_msg\n", msg.msg_type, recvLen);
        process_nr_nsa_msg(&msg, recvLen);
    }

}

void nsa_sendmsg_to_nrue(const void *message, size_t msg_len, Rrc_Msg_Type_t msg_type)
{
    LOG_I(RRC, "Entered %s \n", __FUNCTION__);
    nsa_msg_t n_msg;
    if (msg_len > sizeof(n_msg.msg_buffer))
    {
        LOG_E(RRC, "%s: message too big: %zu\n", __func__, msg_len);
        abort();
    }
    n_msg.msg_type = msg_type;
    memcpy(n_msg.msg_buffer, message, msg_len);
    size_t to_send = sizeof(n_msg.msg_type) + msg_len;

    struct sockaddr_in sa =
    {
        .sin_family = AF_INET,
        .sin_port = htons(6008 + ue_id_g * 2),
    };
    int sent = sendto(to_nr_ue_fd, &n_msg, to_send, 0,
                      (struct sockaddr *)&sa, sizeof(sa));
    if (sent == -1)
    {
        LOG_E(RRC, "%s: sendto: %s\n", __func__, strerror(errno));
        return;
    }
    if (sent != to_send)
    {
        LOG_E(RRC, "%s: Short send %d != %zu\n", __func__, sent, to_send);
        return;
    }
    LOG_I(RRC, "Sent a %d message to the nrUE (%d bytes) \n", msg_type, sent);
}

void init_connections_with_nr_ue()
{
    struct sockaddr_in sa =
    {
        .sin_family = AF_INET,
        .sin_port = htons(6007 + ue_id_g * 2),
    };
    AssertFatal(from_nr_ue_fd == -1, "from_nr_ue_fd was assigned already");
    from_nr_ue_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (from_nr_ue_fd == -1)
    {
        LOG_E(RRC, "%s: Error opening socket %d (%d:%s)\n", __FUNCTION__, from_nr_ue_fd, errno, strerror(errno));
        abort();
    }

    if (inet_aton(nsa_ipaddr, &sa.sin_addr) == 0)
    {
        LOG_E(RRC, "Bad nsa_ipaddr '%s'\n", nsa_ipaddr);
        abort();
    }

    if (bind(from_nr_ue_fd, (struct sockaddr *) &sa, sizeof(sa)) == -1)
    {
        LOG_E(RRC,"%s: Failed to bind the socket\n", __FUNCTION__);
        abort();
    }

    AssertFatal(to_nr_ue_fd == -1, "to_nr_ue_fd was assigned already");
    to_nr_ue_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (to_nr_ue_fd == -1)
    {
        LOG_E(RRC, "%s: Error opening socket %d (%d:%s)\n", __FUNCTION__, to_nr_ue_fd, errno, strerror(errno));
        abort();
    }

}

void process_nr_nsa_msg(nsa_msg_t *msg, int msg_len)
{
    if (msg_len < sizeof(msg->msg_type))
    {
        LOG_E(RRC, "Msg_len = %d\n", msg_len);
        return;
    }
    LOG_D(RRC, "We are processing an NSA message %d \n", msg->msg_type);
    Rrc_Msg_Type_t msg_type = msg->msg_type;
    uint8_t *const msg_buffer = msg->msg_buffer;
    msg_len -= sizeof(msg->msg_type);
    bool received_nr_msg = true;
    protocol_ctxt_t ctxt;
    module_id_t module_id = 0;
    eNB_index_t eNB_index = 0;

    switch (msg_type)
    {
        case NRUE_CAPABILITY_INFO:
        {
            LOG_I(RRC, "Create itti msg to send received NRUE_CAPABILITY_INFO to eNB\n");

            MessageDef *message_p;
            rrc_nrue_cap_info_t *nrue_cap_buf = itti_malloc (TASK_RRC_NSA_UE,
                                                             TASK_RRC_UE,
                                                             sizeof(rrc_nrue_cap_info_t));
            AssertFatal(msg_len <= sizeof(nrue_cap_buf->mesg), "msg_len = %d\n", msg_len);
            memcpy(nrue_cap_buf->mesg, msg_buffer, msg_len);
            nrue_cap_buf->mesg_len = msg_len;
            UE_RRC_INFO *info = &UE_rrc_inst[module_id].Info[eNB_index];
            nrue_cap_buf->dl_dcch_msg = info->dl_dcch_msg;
            info->dl_dcch_msg = NULL;
            message_p = itti_alloc_new_message (TASK_RRC_UE, 0, RRC_NRUE_CAP_INFO_IND);
            RRC_NRUE_CAP_INFO_IND (message_p).sdu_p = (void *)nrue_cap_buf;
            RRC_NRUE_CAP_INFO_IND (message_p).sdu_size = sizeof(*nrue_cap_buf);
            RRC_NRUE_CAP_INFO_IND (message_p).module_id = module_id;
            RRC_NRUE_CAP_INFO_IND (message_p).rnti = info->rnti;
            RRC_NRUE_CAP_INFO_IND (message_p).eNB_index = eNB_index;
            itti_send_msg_to_task (TASK_RRC_UE, 0, message_p);
            LOG_I(RRC, "Sent itti RRC_NRUE_CAP_INFO_IND\n");
            break;
        }
        case UE_CAPABILITY_DUMMY:
        {
            fill_ue_capability(NULL, received_nr_msg);
            UE_rrc_inst[module_id].UECap = UE_rrc_inst->UECap;
            UE_rrc_inst[module_id].UECapability = UE_rrc_inst->UECap->sdu;
            UE_rrc_inst[module_id].UECapability_size = UE_rrc_inst->UECap->sdu_size;

            if (!is_en_dc_supported(UE_rrc_inst->UECap->UE_EUTRA_Capability))
            {
              LOG_E(RRC, "en_dc is NOT supported! Not sending RRC_DCCH_DATA_COPY_IND to update UE_Capability_INFO\n");
              break;
            }

            LOG_I(RRC, "Send itti msg to trigger processing of capabilites b/c we have a UE_CAPABILITY_DUMMY\n");
            MessageDef *message_p;
            rrc_dcch_data_copy_t *dl_dcch_buffer = itti_malloc (TASK_RRC_NSA_UE,
                                                                TASK_RRC_UE,
                                                                sizeof(rrc_dcch_data_copy_t));
            UE_RRC_INFO *info = &UE_rrc_inst[module_id].Info[eNB_index];
            dl_dcch_buffer->dl_dcch_msg = info->dl_dcch_msg;
            info->dl_dcch_msg = NULL;
            message_p = itti_alloc_new_message (TASK_RRC_UE, 0, RRC_DCCH_DATA_COPY_IND);
            RRC_DCCH_DATA_COPY_IND (message_p).sdu_p = (void *)dl_dcch_buffer;
            RRC_DCCH_DATA_COPY_IND (message_p).sdu_size = sizeof(rrc_dcch_data_copy_t);
            RRC_DCCH_DATA_COPY_IND (message_p).module_id = module_id;
            RRC_DCCH_DATA_COPY_IND (message_p).rnti = info->rnti;
            RRC_DCCH_DATA_COPY_IND (message_p).eNB_index = eNB_index;
            itti_send_msg_to_task (TASK_RRC_UE, 0, message_p);
            LOG_I(RRC, "Sent itti RRC_DCCH_DATA_COPY_IND\n");
            break;
        }

        case NR_UE_RRC_MEASUREMENT:
        {
            nfapi_p7_message_header_t header;
            if (nfapi_p7_message_header_unpack((void *)msg_buffer, msg_len, &header, sizeof(header), NULL) < 0)
            {
                LOG_E(MAC, "Header unpack failed in %s \n", __FUNCTION__);
                break;
            }
            if (header.message_id != NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST)
            {
                LOG_E(MAC, "%s: Unexpected nfapi message type: %d\n", __FUNCTION__, header.message_id);
                break;
            }

            nfapi_nr_dl_tti_request_t dl_tti_request;
            int unpack_len = nfapi_nr_p7_message_unpack((void *)msg_buffer,
                                                         msg_len,
                                                         &dl_tti_request,
                                                         sizeof(nfapi_nr_dl_tti_request_t),
                                                         NULL);
            if (unpack_len < 0)
            {
                LOG_E(RRC, "%s: SSB PDU unpack failed \n", __FUNCTION__);
                break;
            }
            int num_pdus = dl_tti_request.dl_tti_request_body.nPDUs;
            if (num_pdus <= 0)
            {
                LOG_E(RRC, "%s: dl_tti_request number of PDUS <= 0\n", __FUNCTION__);
                abort();
            }
            for (int i = 0; i < num_pdus; i++)
            {
                nfapi_nr_dl_tti_request_pdu_t *pdu_list = &dl_tti_request.dl_tti_request_body.dl_tti_pdu_list[i];
                if (pdu_list->PDUType == NFAPI_NR_DL_TTI_SSB_PDU_TYPE)
                {
                    LOG_I(RRC, "Got an NR_UE_RRC_MEASUREMENT. pdulist[%d].ssbRsrp = %d\n",
                         i, pdu_list->ssb_pdu.ssb_pdu_rel15.ssbRsrp);
                }
            }
            break;
        }

        case NR_RRC_CONFIG_COMPLETE_REQ:
        {
            LOG_I(RRC, "Got an NR_RRC_CONFIG_COMPLETE_REQ. Now make octet string and call below!\n");
            OCTET_STRING_t rrcConfigurationComplete;
            memset(&rrcConfigurationComplete, 0, sizeof(rrcConfigurationComplete));
            OCTET_STRING_fromBuf(&rrcConfigurationComplete,
                                (const char *)msg_buffer,
                                msg_len);
            UE_RRC_INFO *info = &UE_rrc_inst[module_id].Info[eNB_index];
            uint8_t t_id = info->dl_dcch_msg->message.choice.c1.choice.
                           rrcConnectionReconfiguration.rrc_TransactionIdentifier;
            PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_id,
                                           ENB_FLAG_NO, info->rnti,
                                           0, 0, eNB_index);
            rrc_ue_generate_RRCConnectionReconfigurationComplete(&ctxt, ctxt.eNB_index, t_id, &rrcConfigurationComplete);
            break;
        }
        default:
            LOG_E(RRC, "No NSA Message Found\n");
    }
}
