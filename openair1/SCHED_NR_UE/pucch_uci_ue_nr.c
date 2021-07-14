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

/************************************************************************
*
* MODULE      :  PUCCH Packed Uplink Control Channel for UE NR
*                PUCCH is used to transmit Uplink Control Information UCI
*                which is composed of:
*                - SR Scheduling Request
*                - HARQ ACK/NACK
*                - CSI Channel State Information
*                UCI can also be transmitted on a PUSCH if it schedules.
*
* DESCRIPTION :  functions related to PUCCH UCI management
*                TS 38.213 9  UE procedure for reporting control information
*
**************************************************************************/

#include "executables/softmodem-common.h"
#include "PHY/NR_REFSIG/ss_pbch_nr.h"
#include "PHY/defs_nr_UE.h"
#include <openair1/SCHED/sched_common.h>
#include <openair1/PHY/NR_UE_TRANSPORT/pucch_nr.h>
#include "openair2/LAYER2/NR_MAC_UE/mac_proto.h"
#include "openair1/PHY/NR_UE_ESTIMATION/nr_estimation.h"
#include <openair1/PHY/impl_defs_nr.h>
#include <common/utils/nr/nr_common.h>

#ifndef NO_RAT_NR

#include "SCHED_NR_UE/defs.h"
#include "SCHED_NR_UE/harq_nr.h"
#include "SCHED_NR_UE/pucch_power_control_ue_nr.h"

#define DEFINE_VARIABLES_PUCCH_UE_NR_H
#include "SCHED_NR_UE/pucch_uci_ue_nr.h"
#undef DEFINE_VARIABLES_PUCCH_UE_NR_H

#endif


/* TS 36.213 Table 9.2.3-3: Mapping of values for one HARQ-ACK bit to sequences */
static const int sequence_cyclic_shift_1_harq_ack_bit[2]
/*        HARQ-ACK Value        0    1 */
/* Sequence cyclic shift */ = { 0,   6 }
;

/* TS 36.213 Table 9.2.5-2: Mapping of values for two HARQ-ACK bits and positive SR to sequences */
static const int sequence_cyclic_shift_2_harq_ack_bits_positive_sr[4]
/*        HARQ-ACK Value      (0,0)  (0,1)   (1,0)  (1,1) */
/* Sequence cyclic shift */ = {  1,     4,     10,     7 }
;


uint8_t nr_is_cqi_TXOp(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t gNB_id);
uint8_t nr_is_ri_TXOp(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t gNB_id);

static const uint16_t scheduling_request_periodicity[NB_SR_PERIOD]
= { 0, 0, 1, 2, 4, 5, 8, 10, 16, 20, 40, 80, 160, 320, 640 }
;

/* TS 38.213 9.2.5.2 UE procedure for multiplexing HARQ-ACK/SR and CSI in a PUCCH */
/* this is a counter of number of pucch format 4 per subframe */
static int nb_pucch_format_4_in_subframes[LTE_NUMBER_OF_SUBFRAMES_PER_FRAME] = { 0 } ;

/* TS 36.213 Table 9.2.5.2-1: Code rate  corresponding to higher layer parameter PUCCH-F2-maximum-coderate, */
/* or PUCCH-F3-maximum-coderate, or PUCCH-F4-maximum-coderate */
/* add one additional element set to 0 for parsing the array until this end */
/* stored values are code rates * 100 */
//static const int code_rate_r_time_100[8] = { (0.08 * 100), (0.15 * 100), (0.25*100), (0.35*100), (0.45*100), (0.60*100), (0.80*100), 0 } ;

/* TS 38.213 Table 9.2.3-4: Mapping of values for two HARQ-ACK bits to sequences */
static const int sequence_cyclic_shift_2_harq_ack_bits[4]
/*        HARQ-ACK Value       (0,0)  (0,1)  (1,0)  (1,1) */
/* Sequence cyclic shift */ = {   0,     3,     9,     6 }
;

/* TS 38.211 Table 6.4.1.3.3.2-1: DM-RS positions for PUCCH format 3 and 4 */
static const int nb_symbols_excluding_dmrs[NB_SYMBOL_MINUS_FOUR][2][2]
= {
/*                     No additional DMRS            Additional DMRS   */
/* PUCCH length      No hopping   hopping         No hopping   hopping */
/* index                  0          1                 0          1    */
/*    4     */    {{      3    ,     2   }   ,  {      3     ,    2    }},
/*    5     */    {{      3    ,     3   }   ,  {      3     ,    3    }},
/*    6     */    {{      4    ,     4   }   ,  {      4     ,    4    }},
/*    7     */    {{      5    ,     5   }   ,  {      5     ,    5    }},
/*    8     */    {{      6    ,     6   }   ,  {      6     ,    6    }},
/*    9     */    {{      7    ,     7   }   ,  {      7     ,    7    }},
/*   10     */    {{      8    ,     8   }   ,  {      6     ,    6    }},
/*   11     */    {{      9    ,     9   }   ,  {      7     ,    7    }},
/*   12     */    {{     10    ,    10   }   ,  {      8     ,    8    }},
/*   13     */    {{     11    ,    11   }   ,  {      9     ,    9    }},
/*   14     */    {{     12    ,    12   }   ,  {     10     ,   10    }},
}
;


/* TS 36.213 Table 9.2.5-1: Mapping of values for one HARQ-ACK bit and positive SR to sequences */
static const int sequence_cyclic_shift_1_harq_ack_bit_positive_sr[2]
/*        HARQ-ACK Value        0    1 */
/* Sequence cyclic shift */ = { 3,   9 }
;

static float RSRP_meas_mapping_nr[98]
= {
  -140,
    -139,
    -138,
    -137,
    -136,
    -135,
    -134,
    -133,
    -132,
    -131,
    -130,
    -129,
    -128,
    -127,
    -126,
    -125,
    -124,
    -123,
    -122,
    -121,
    -120,
    -119,
    -118,
    -117,
    -116,
    -115,
    -114,
    -113,
    -112,
    -111,
    -110,
    -109,
    -108,
    -107,
    -106,
    -105,
    -104,
    -103,
    -102,
    -101,
    -100,
    -99,
    -98,
    -97,
    -96,
    -95,
    -94,
    -93,
    -92,
    -91,
    -90,
    -89,
    -88,
    -87,
    -86,
    -85,
    -84,
    -83,
    -82,
    -81,
    -80,
    -79,
    -78,
    -77,
    -76,
    -75,
    -74,
    -73,
    -72,
    -71,
    -70,
    -69,
    -68,
    -67,
    -66,
    -65,
    -64,
    -63,
    -62,
    -61,
    -60,
    -59,
    -58,
    -57,
    -56,
    -55,
    -54,
    -53,
    -52,
    -51,
    -50,
    -49,
    -48,
    -47,
    -46,
    -45,
    -44,
    -43
  }
  ;
  
long
binary_search_float_nr(
  float elements[],
  long numElem,
  float value
)
//-----------------------------------------------------------------------------
{
  long first, last, middle;
  first = 0;
  last = numElem-1;
  middle = (first+last)/2;

  if(value < elements[0]) {
    return first;
  }

  if(value >= elements[last]) {
    return last;
  }

  while (last - first > 1) {
    if (elements[middle] > value) {
      last = middle;
    } else {
      first = middle;
    }

    middle = (first+last)/2;
  }

  if (first < 0 || first >= numElem) {
    LOG_E(RRC,"\n Error in binary search float!");
  }

  return first;
}
/*
void nr_generate_pucch0(int32_t **txdataF,
                        NR_DL_FRAME_PARMS *frame_parms,
                        PUCCH_CONFIG_DEDICATED *pucch_config_dedicated,
                        int16_t amp,
                        int nr_slot_tx,
                        uint8_t mcs,
                        uint8_t nrofSymbols,
                        uint8_t startingSymbolIndex,
                        uint16_t startingPRB);

void nr_generate_pucch1(int32_t **txdataF,
                        NR_DL_FRAME_PARMS *frame_parms,
                        PUCCH_CONFIG_DEDICATED *pucch_config_dedicated,
                        uint64_t payload,
                        int16_t amp,
                        int nr_slot_tx,
                        uint8_t nrofSymbols,
                        uint8_t startingSymbolIndex,
                        uint16_t startingPRB,
                        uint16_t startingPRB_intraSlotHopping,
                        uint8_t timeDomainOCC,
                        uint8_t nr_bit);

void nr_generate_pucch2(int32_t **txdataF,
                        NR_DL_FRAME_PARMS *frame_parms,
                        PUCCH_CONFIG_DEDICATED *pucch_config_dedicated,
                        uint64_t payload,
                        int16_t amp,
                        int nr_slot_tx,
                        uint8_t nrofSymbols,
                        uint8_t startingSymbolIndex,
                        uint8_t nrofPRB,
                        uint16_t startingPRB,
                        uint8_t nr_bit);

void nr_generate_pucch3_4(int32_t **txdataF,
                         NR_DL_FRAME_PARMS *frame_parms,
                         pucch_format_nr_t fmt,
                         PUCCH_CONFIG_DEDICATED *pucch_config_dedicated,
                         uint64_t payload,
                         int16_t amp,
                         int nr_slot_tx,
                         uint8_t nrofSymbols,
                         uint8_t startingSymbolIndex,
                         uint8_t nrofPRB,
                         uint16_t startingPRB,
                         uint8_t nr_bit,
                         uint8_t occ_length_format4,
                         uint8_t occ_index_format4);
*/
/**************** variables **************************************/


/**************** functions **************************************/

//extern uint8_t is_cqi_TXOp(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id);
//extern uint8_t is_ri_TXOp(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id);
/*******************************************************************
*
* NAME :         pucch_procedures_ue_nr
*
* PARAMETERS :   ue context
*                processing slots of reception/transmission
*                gNB_id identifier
*
* RETURN :       bool TRUE  PUCCH will be transmitted
*                     FALSE No PUCCH to transmit
*
* DESCRIPTION :  determines UCI (uplink Control Information) payload
*                and PUCCH format and its parameters.
*                PUCCH is no transmitted if:
*                - there is no valid data to transmit
*                - Pucch parameters are not valid
*
* Below information is scanned in order to know what information should be transmitted to network.
*
* (SR Scheduling Request)   (HARQ ACK/NACK)    (CSI Channel State Information)
*          |                        |               - CQI Channel Quality Indicator
*          |                        |                - RI  Rank Indicator
*          |                        |                - PMI Primary Matrux Indicator
*          |                        |                - LI Layer Indicator
*          |                        |                - L1-RSRP
*          |                        |                - CSI-RS resource idicator
*          |                        V                    |
*          +-------------------- -> + <------------------
*                                   |
*                   +--------------------------------+
*                   | UCI Uplink Control Information |
*                   +--------------------------------+
*                                   V                                            PUCCH Configuration
*               +----------------------------------------+                   +--------------------------+
*               | Determine PUCCH  payload and its       |                   |     PUCCH Resource Set   |
*               +----------------------------------------+                   |     PUCCH Resource       |
*                                   V                                        |     Format parameters    |
*               +-----------------------------------------+                  |                          |
*               | Select PUCCH format with its parameters | <----------------+--------------------------+
*               +-----------------------------------------+
*                                   V
*                          +-----------------+
*                          |  Generate PUCCH |
*                          +-----------------+
*
* TS 38.213 9  UE procedure for reporting control information
*
*********************************************************************/

/* TS 36.213 Table 9.2.1-1: PUCCH resource sets before dedicated PUCCH resource configuration */
const initial_pucch_resource_t initial_pucch_resource[NB_INITIAL_PUCCH_RESOURCE]
=
{
/*              format           first symbol     Number of symbols        PRB offset    nb index for       set of initial CS */
/*  0  */ {  pucch_format0_nr,      12,                  2,                   0,            2,       {    0,   3,    0,    0  }   },
/*  1  */ {  pucch_format0_nr,      12,                  2,                   0,            3,       {    0,   4,    8,    0  }   },
/*  2  */ {  pucch_format0_nr,      12,                  2,                   3,            3,       {    0,   4,    8,    0  }   },
/*  3  */ {  pucch_format1_nr,      10,                  4,                   0,            2,       {    0,   6,    0,    0  }   },
/*  4  */ {  pucch_format1_nr,      10,                  4,                   0,            4,       {    0,   3,    6,    9  }   },
/*  5  */ {  pucch_format1_nr,      10,                  4,                   2,            4,       {    0,   3,    6,    9  }   },
/*  6  */ {  pucch_format1_nr,      10,                  4,                   4,            4,       {    0,   3,    6,    9  }   },
/*  7  */ {  pucch_format1_nr,       4,                 10,                   0,            2,       {    0,   6,    0,    0  }   },
/*  8  */ {  pucch_format1_nr,       4,                 10,                   0,            4,       {    0,   3,    6,    9  }   },
/*  9  */ {  pucch_format1_nr,       4,                 10,                   2,            4,       {    0,   3,    6,    9  }   },
/* 10  */ {  pucch_format1_nr,       4,                 10,                   4,            4,       {    0,   3,    6,    9  }   },
/* 11  */ {  pucch_format1_nr,       0,                 14,                   0,            2,       {    0,   6,    0,    0  }   },
/* 12  */ {  pucch_format1_nr,       0,                 14,                   0,            4,       {    0,   3,    6,    9  }   },
/* 13  */ {  pucch_format1_nr,       0,                 14,                   2,            4,       {    0,   3,    6,    9  }   },
/* 14  */ {  pucch_format1_nr,       0,                 14,                   4,            4,       {    0,   3,    6,    9  }   },
/* 15  */ {  pucch_format1_nr,       0,                 14,                   0,            4,       {    0,   3,    6,    9  }   },
}
;

bool pucch_procedures_ue_nr(PHY_VARS_NR_UE *ue, uint8_t gNB_id, UE_nr_rxtx_proc_t *proc, bool reset_harq)
{
  uint8_t   sr_payload = 0;
  uint32_t  pucch_ack_payload = 0; /* maximum number of bits for pucch payload is supposed to be 32 */
  uint64_t  pucch_payload = 0;
  uint32_t  csi_payload = 0;
  int       frame_tx = proc->frame_tx;
  int       nr_slot_tx = proc->nr_slot_tx;
  int       Mod_id = ue->Mod_id;
  int       CC_id = ue->CC_id;

  int       O_SR = 0;
  int       O_ACK = 0;
  int       O_CSI = 0;      /* channel state information */
  int       N_UCI = 0;      /* size in bits for Uplink Control Information */
  int       cqi_status = 0;
  int       ri_status = 0;
  int       csi_status = 0;

  int       initial_pucch_id = NB_INITIAL_PUCCH_RESOURCE;
  int       pucch_resource_set = MAX_NB_OF_PUCCH_RESOURCE_SETS;
  int       pucch_resource_id = MAX_NB_OF_PUCCH_RESOURCES;
  int       pucch_resource_indicator = MAX_PUCCH_RESOURCE_INDICATOR;
  int       n_HARQ_ACK;

  int dmrs_scrambling_id=0,data_scrambling_id=0;

  NR_UE_MAC_INST_t *mac = get_mac_inst(0);
  NR_PUCCH_Resource_t *pucch_resource = NULL;
  uint16_t crnti = mac->crnti;
  NR_BWP_Id_t bwp_id = mac->UL_BWP_Id;

  /* update current context */

  int subframe_number = proc->nr_slot_rx / ue->frame_parms.slots_per_subframe;
  nb_pucch_format_4_in_subframes[subframe_number] = 0; /* reset pucch format 4 counter at current rx position */

  int dl_harq_pid = ue->dlsch[proc->thread_id][gNB_id][0]->current_harq_pid;

  if (dl_harq_pid < ue->dlsch[proc->thread_id][gNB_id][0]->number_harq_processes_for_pdsch) {
    /* pucch indicator can be reseted in function get_downlink_ack so it should be get now */
    pucch_resource_indicator = ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[dl_harq_pid]->harq_ack.pucch_resource_indicator;
  }

  LOG_D(PHY, "PUCCH: %d.%d bwp_id %ld dl_harq_pid = %d, pucch_resource_indicator = %d\n", frame_tx, nr_slot_tx, bwp_id,dl_harq_pid, pucch_resource_indicator);

  /* Part - I
   * Collect feedback that should be transmitted at this nr_slot_tx :
   * - ACK/NACK, SR, CSI (CQI, RI, ...)
   */

  sr_payload = 0;

  if (trigger_periodic_scheduling_request( ue, gNB_id, proc ) == 1) {
    O_SR = 1; /* sr should be transmitted */
    if (ue->mac_enabled == 1) {

      /* sr_payload = 1 means that this is a positive SR, sr_payload = 0 means that it is a negative SR */
      sr_payload = nr_ue_get_SR(Mod_id,
                                CC_id,
                                frame_tx,
                                gNB_id,
                                0,//ue->pdcch_vars[proc->thread_id][gNB_id]->crnti,
                                nr_slot_tx); // nr_slot_rx used for meas gap
    }
    else {
      sr_payload = 1;
    }
  }

  O_ACK = get_downlink_ack( ue, gNB_id, proc, &pucch_ack_payload,
                            &n_HARQ_ACK, reset_harq); // 1 to reset ACK/NACK status : 0 otherwise

  cqi_status = ((ue->cqi_report_config[gNB_id].CQI_ReportPeriodic.cqi_PMI_ConfigIndex>0) &&
                                                         (nr_is_cqi_TXOp(ue,proc,gNB_id) == 1));

  ri_status = ((ue->cqi_report_config[gNB_id].CQI_ReportPeriodic.ri_ConfigIndex>0) &&
                                                         (nr_is_ri_TXOp(ue,proc,gNB_id) == 1));


  if (mac->cg &&
      mac->cg->spCellConfig &&
      mac->cg->spCellConfig->spCellConfigDedicated &&
      mac->cg->spCellConfig->spCellConfigDedicated->csi_MeasConfig&&
      mac->cg->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup) {
    NR_CSI_MeasConfig_t *csi_MeasConfig = mac->cg->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup;

    uint16_t report_slot_csi =csi_MeasConfig->csi_ReportConfigToAddModList->list.array[0]->reportConfigType.choice.periodic->reportSlotConfig.choice.slots320;

    //if (mac->csirc->reportQuantity.choice.ssb_Index_RSRP){
    if (report_slot_csi == proc->nr_slot_tx)
      csi_status = get_csi_nr(mac, ue, gNB_id, &csi_payload);
    else
      csi_status = 0;
    //}

    O_CSI = cqi_status + ri_status + csi_status;

    /* Part - II */
    /* if payload is empty or only negative SR -> no pucch transmission */

  if(O_ACK == 0) {
    N_UCI = O_SR + O_CSI;
    if ((N_UCI == 0) || ((O_CSI == 0) && (sr_payload == 0))) {   /* TS 38.213 9.2.4 UE procedure for reporting SR */
      NR_TST_PHY_PRINTF("PUCCH No feedback AbsSubframe %d.%d \n", frame_tx%1024, nr_slot_tx);
      LOG_D(PHY,"PUCCH No feedback AbsSubframe %d.%d \n", frame_tx%1024, nr_slot_tx);
      return (FALSE);
    }
    else {
      /* a resource set and a resource should be find according to payload size */
      pucch_resource_set = find_pucch_resource_set( mac, gNB_id, N_UCI);
      if (pucch_resource_set != MAX_NB_OF_PUCCH_RESOURCE_SETS) {
        pucch_resource_indicator = 0;
        /* get the first resource of the set */
        pucch_resource_id = mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList->list.array[pucch_resource_set]->resourceList.list.array[pucch_resource_indicator][0];
      }
      else {
        LOG_W(PHY,"PUCCH no resource set found for CSI at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
        O_CSI = 0;
        csi_payload = 0;
      }

	if (O_CSI == 0) {
	  /* only SR has to be send */
	  /* in this case there is no DCI related to PUCCH parameters so pucch resource should be get from sr configuration */
	  /* TS 38.213 9.2.4 UE procedure for reporting SR */
	  pucch_resource_set = 0; /* force it to a valid value */
	  if (ue->scheduling_request_config_nr[gNB_id].sr_ResourceConfig[ue->scheduling_request_config_nr[gNB_id].active_sr_id] != NULL) {
	    pucch_resource_id = ue->scheduling_request_config_nr[gNB_id].sr_ResourceConfig[ue->scheduling_request_config_nr[gNB_id].active_sr_id]->resource;
	  }
	  else {
	    LOG_E(PHY,"PUCCH No scheduling request configuration : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
	    return(FALSE);
	  }
	}
      }
    }
  }

  N_UCI = O_SR + O_ACK + O_CSI;
  if (N_UCI ==0) return(TRUE);

  /* Part - III */
  /* Choice PUCCH format and its related parameters */
  pucch_format_nr_t format = pucch_format0_nr;
  uint8_t  starting_symbol_index=0;
  uint8_t nb_symbols_total = 0;
  uint8_t  nb_symbols = 0;
  uint16_t startingPRB = 0;;  /* it can be considered as first  hop on case of pucch hopping */
  uint16_t secondHopPRB = 0;     /* second part for pucch for hopping */
  uint8_t  nb_of_prbs = 0;
  int m_0 = 0;                 /* format 0 only */
  int m_CS = 0;                /* for all format except for format 0 */
  int index_additional_dmrs = I_PUCCH_NO_ADDITIONAL_DMRS;
  int index_hopping = I_PUCCH_NO_HOPPING;
  int time_domain_occ = 0;
  int occ_length = 0;
  int occ_Index = 0;
  int BWPsize = 0;
  int BWPstart = 0;

  NR_UE_HARQ_STATUS_t *harq_status = &ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[dl_harq_pid]->harq_ack;

  if (select_pucch_resource(ue, mac, gNB_id, N_UCI, pucch_resource_indicator, &initial_pucch_id, &pucch_resource_set,
                            &pucch_resource_id, harq_status) == TRUE) {
    /* use of initial pucch configuration provided by system information 1 */
    /***********************************************************************/
    if (initial_pucch_id != NB_INITIAL_PUCCH_RESOURCE) {
      LOG_D(MAC,"Selecting INITIAL PUCCH Resource\n");
      format = initial_pucch_resource[initial_pucch_id].format;
      starting_symbol_index = initial_pucch_resource[initial_pucch_id].startingSymbolIndex;
      nb_symbols_total = initial_pucch_resource[initial_pucch_id].nrofSymbols;

      int N_CS = initial_pucch_resource[initial_pucch_id].nb_CS_indexes;
      /* see TS 38213 Table 9.2.1-1: PUCCH resource sets before dedicated PUCCH resource configuration */
      BWPsize = NRRIV2BW(mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
      BWPstart = NRRIV2PRBOFFSET(mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
      int RB_BWP_offset;
      if (initial_pucch_id == 15) {
        RB_BWP_offset =BWPsize/4;
      }
      else
      {
        RB_BWP_offset = initial_pucch_resource[initial_pucch_id].PRB_offset;
      }
      if (initial_pucch_id/8 == 0) {
        startingPRB = RB_BWP_offset + (initial_pucch_id/N_CS);
        secondHopPRB = BWPsize - 1 - RB_BWP_offset - (initial_pucch_id/N_CS);
        m_0 = initial_pucch_resource[initial_pucch_id].initial_CS_indexes[initial_pucch_id%N_CS];
      }
      else if (initial_pucch_id/8 == 1)
      {
        startingPRB = RB_BWP_offset + (initial_pucch_id/N_CS);
        secondHopPRB = BWPsize - 1 - RB_BWP_offset - ((initial_pucch_id - 8)/N_CS);
        m_0 =  initial_pucch_resource[initial_pucch_id].initial_CS_indexes[(initial_pucch_id - 8)%N_CS];
      }
      if ((ue->UE_mode[gNB_id] != PUSCH) && (O_ACK > 1)) {
        O_ACK = 1;
        pucch_ack_payload &= 0x1; /* take only first ack */
        LOG_W(PHY,"PUCCH ue is not expected to generate more than one HARQ-ACK at AbsSubframe %d.%d \n", frame_tx%1024, nr_slot_tx);
      }
      NR_TST_PHY_PRINTF("PUCCH common configuration with index %d \n", initial_pucch_id);
      startingPRB += BWPstart;
      secondHopPRB += BWPstart;
    }
    /* use dedicated pucch resource configuration */
    /**********************************************/
    else if ((pucch_resource_set != MAX_NB_OF_PUCCH_RESOURCE_SETS) && (pucch_resource_id != MAX_NB_OF_PUCCH_RESOURCES)) {
      /* check that current configuration is supported */
      if (mac->cg &&
	        mac->cg->physicalCellGroupConfig &&
          (mac->cg->physicalCellGroupConfig->harq_ACK_SpatialBundlingPUCCH != NULL || mac->cg->physicalCellGroupConfig->pdsch_HARQ_ACK_Codebook != 1)) {
        LOG_E(PHY,"PUCCH Unsupported cell group configuration : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
        return(FALSE);
      }
      else if (mac->cg &&
               mac->cg->spCellConfig &&
               mac->cg->spCellConfig->spCellConfigDedicated &&
               mac->cg->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig &&
               mac->cg->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->choice.setup &&
               mac->cg->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->choice.setup->codeBlockGroupTransmission != NULL) {
        LOG_E(PHY,"PUCCH Unsupported code block group for serving cell config : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
        return(FALSE);
      }
      NR_PUCCH_Config_t *pucch_Config;
      if (bwp_id>0 &&
          mac->ULbwp[bwp_id-1] &&
          mac->ULbwp[bwp_id-1]->bwp_Dedicated &&
          mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config &&
          mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup)
          pucch_Config =  mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup;
      else if (bwp_id==0 &&
               mac->cg &&
               mac->cg->spCellConfig &&
               mac->cg->spCellConfig->spCellConfigDedicated &&
               mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
               mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP &&
               mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config &&
               mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup) {
        pucch_Config = mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup;
        BWPsize  =  NRRIV2BW(mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth,MAX_BWP_SIZE);
        BWPstart =  NRRIV2PRBOFFSET(mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth,MAX_BWP_SIZE);
      }

      else AssertFatal(1==0,"no pucch_Config\n");
      pucch_resource = select_resource_by_id(pucch_resource_id, pucch_Config);
      format = pucch_resource->format.present;
      nb_symbols_total = get_nb_symbols_pucch(pucch_resource, format);
      starting_symbol_index = get_starting_symb_idx(pucch_resource, format);
      startingPRB   = BWPstart + pucch_resource->startingPRB;
      secondHopPRB = pucch_resource->intraSlotFrequencyHopping ? (BWPstart+*pucch_resource->secondHopPRB) : startingPRB;
      if (format==pucch_format1_nr)
        time_domain_occ = pucch_resource->format.choice.format1->timeDomainOCC;
      if (format==pucch_format4_nr) {
        occ_length = pucch_resource->format.choice.format4->occ_Length;
        occ_Index  = pucch_resource->format.choice.format4->occ_Index;
      }

      m_0 = get_ics_pucch(pucch_resource, format);
      AssertFatal(m_0 >= 0, "Invalid m_0\n");
      if (format == pucch_format3_nr) {
        if (mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format3->choice.setup->additionalDMRS[0] == 1) {
          index_additional_dmrs = I_PUCCH_ADDITIONAL_DMRS;
        }
      }
      else if (format == pucch_format4_nr) {
        if (mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format4->choice.setup->additionalDMRS[0] == 1) {
          index_additional_dmrs = I_PUCCH_ADDITIONAL_DMRS;
        }
      }

      if ((format == pucch_format3_nr) || (format == pucch_format4_nr)) {
        if (pucch_resource->intraSlotFrequencyHopping[0] == 1) {
          index_hopping = I_PUCCH_HOPING;
        }
      }

      NR_TST_PHY_PRINTF("PUCCH dedicated configuration with resource index %d \n", pucch_resource_id);
    }
  }
  else {
    LOG_W(PHY,"PUCCH No PUCCH resource found at AbsSubframe %d.%d \n", frame_tx%1024, nr_slot_tx);
    return (FALSE);
  }

  //int max_code_rate = 0;
  //int Q_m = BITS_PER_SYMBOL_QPSK; /* default pucch modulation type is QPSK with 2 bits per symbol */
  int N_sc_ctrl_RB = 0;
  int O_CRC = 0;

  nb_symbols = nb_symbols_total; /* by default, it can be reduced due to symbols reserved for dmrs */

  switch(format) {
    case pucch_format0_nr:
    {
      nb_of_prbs = 1;
      N_sc_ctrl_RB = N_SC_RB;
      break;
    }
    case pucch_format1_nr:
    {
      nb_of_prbs = 1;
      N_sc_ctrl_RB = N_SC_RB;
      break;
    }
    case pucch_format2_nr:
    {
      nb_of_prbs = pucch_resource->format.choice.format2->nrofPRBs;
      N_sc_ctrl_RB = N_SC_RB - 4;
      break;
    }
    case pucch_format3_nr:
    {
      nb_of_prbs = pucch_resource->format.choice.format3->nrofPRBs;
      //if (mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format3->choice.setup->pi2BPSK[0] == 1) {
      //  Q_m = BITS_PER_SYMBOL_BPSK; /* set bpsk modulation type with 1 bit per modulation symbol */
      //}
      N_sc_ctrl_RB = N_SC_RB;
      nb_symbols = nb_symbols_excluding_dmrs[nb_symbols_total-4][index_additional_dmrs][index_hopping];
      break;
    }
    case pucch_format4_nr:
    {
      //if (mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format4->choice.setup->pi2BPSK[0] == 1) {
      //  Q_m = BITS_PER_SYMBOL_BPSK; /* set bpsk modulation type with 1 bit per modulation symbol */
      //}
      nb_symbols = nb_symbols_excluding_dmrs[nb_symbols_total-4][index_additional_dmrs][index_hopping];
      nb_of_prbs = 1;
      subframe_number = nr_slot_tx / ue->frame_parms.slots_per_subframe;
      nb_pucch_format_4_in_subframes[subframe_number]++; /* increment number of transmit pucch 4 in current subframe */
      NR_TST_PHY_PRINTF("PUCCH Number of pucch format 4 in subframe %d is %d \n", subframe_number, nb_pucch_format_4_in_subframes[subframe_number]);
      N_sc_ctrl_RB = N_SC_RB/(nb_pucch_format_4_in_subframes[subframe_number]);
      break;
    }
  }

  /* TS 38.213 9.2.5.2 UE procedure for multiplexing HARQ-ACK/SR and CSI */
  /* drop CSI report if simultaneous HARQ-ACK/SR and periodic/semi-periodic CSI cannot be transmitted at the same time */
  if (format !=  pucch_format0_nr) {

    if (mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format1 != NULL) {
      //max_code_rate = code_rate_r_time_100[mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format1->choice.setup->maxCodeRate[0]]; /* it is code rate * 10 */

      if ((O_ACK != 0) && (mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format1->choice.setup->simultaneousHARQ_ACK_CSI[0] == 0)) {
        N_UCI = N_UCI - O_CSI;
        O_CSI = cqi_status = ri_status = 0;
        csi_payload = 0; /* csi should be dropped in this case */
      }
    }

    /* TS 38.212 6.3.1.2  Code block segmentation and CRC attachment */
    /* crc attachment can be done depending of payload size */
//    if (N_UCI < 11) {
//      O_CRC = 0;  /* no additional crc bits */
//    }
//    else if ((N_UCI >= 12) && (N_UCI <= 19)) {
//      O_CRC = 6;  /* number of additional crc bits */
//    }
//   else if (N_UCI >= 20) {
//      O_CRC = 11; /* number of additional crc bits */
//    }

    N_UCI = N_UCI + O_CRC;

    /* for format 2 and 3, number of prb should be adjusted to minimum value which cope to information size */
    /*if (nb_of_prbs > 1 ) {
      int nb_prb_min = 0;
      int payload_in_bits;
      do {
        nb_prb_min++;
        payload_in_bits = (nb_prb_min * N_sc_ctrl_RB * nb_symbols * Q_m * max_code_rate)/100; */ /* code rate has been multiplied by 100 */
        
        /*NR_TST_PHY_PRINTF("PUCCH Adjust number of prb : (N_UCI : %d ) (payload_in_bits : %d) (N_sc_ctrl_RB : %d) (nb_symbols : %d) (Q_m : %d) (max_code_rate*100 : %d) \n",
                                               N_UCI,        payload_in_bits,       N_sc_ctrl_RB,       nb_symbols,       Q_m,       max_code_rate);
      } while (N_UCI > payload_in_bits);

      if (nb_prb_min > nb_of_prbs) {
        LOG_E(PHY,"PUCCH Number of prbs too small for current pucch bits to transmit : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
        return (FALSE);
      }
      else {
        nb_of_prbs = nb_prb_min;
      }
    }*/

    /* TS 38.213 9.2.4 for a positive SR transmission, payload b(0) = 0 */
    if ((O_SR == 1) && (format ==  pucch_format1_nr)) {
      sr_payload = 0;
    }
  }
  else {  /* only format 0 here */
    if ((O_SR == 0) && (O_CSI == 0)) {  /* only ack is transmitted TS 36.213 9.2.3 UE procedure for reporting HARQ-ACK */
      if (O_ACK == 1) {
        m_CS = sequence_cyclic_shift_1_harq_ack_bit[pucch_ack_payload & 0x1];   /* only harq of 1 bit */
      }
      else {
        m_CS = sequence_cyclic_shift_2_harq_ack_bits[pucch_ack_payload & 0x3];  /* only harq with 2 bits */
      }
    }
    else if ((O_SR == 1) && (O_CSI == 0)) { /* SR + eventually ack are transmitted TS 36.213 9.2.5.1 UE procedure for multiplexing HARQ-ACK or CSI and SR */
      if (sr_payload == 1) {                /* positive scheduling request */
        if (O_ACK == 1) {
          m_CS = sequence_cyclic_shift_1_harq_ack_bit_positive_sr[pucch_ack_payload & 0x1];   /* positive SR and harq of 1 bit */
        }
        else if (O_ACK == 2) {
          m_CS = sequence_cyclic_shift_2_harq_ack_bits_positive_sr[pucch_ack_payload & 0x3];  /* positive SR and harq with 2 bits */
        }
        else {
          m_CS = 0;  /* only positive SR */
        }
      }
    }
    N_UCI = O_SR = O_ACK = 0;
    pucch_payload = sr_payload = pucch_ack_payload = 0; /* no data for format 0 */
  }

  /* TS 38.212 6.3.1  Uplink control information on PUCCH                                       */
  /* information concatenation of payload                                                       */
  /*                                                   CSI           SR          HARQ-ACK       */
  /* bit order of payload of size n :           a(n)....................................a(0)    */
  /* a(0) is the LSB and a(n) the MSB   <--------><--------------><------------><---------->    */
  /*                                       O_CRC        O_CSI           O_SR         O_ACK      */
  /*                                                                                            */
  /* remark: crc is not part of payload, it is later added by block coding.                     */

  if (N_UCI > (sizeof(uint64_t)*8)) {
    LOG_E(PHY,"PUCCH number of UCI bits exceeds payload size : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
    return(0);
  }

  pucch_payload = pucch_payload | (csi_payload << (O_ACK + O_SR)) |  (sr_payload << O_ACK) | pucch_ack_payload;

  NR_TST_PHY_PRINTF("PUCCH ( AbsSubframe : %d.%d ) ( total payload size %d data 0x%02x ) ( ack length %d data 0x%02x ) ( sr length %d value %d ) ( csi length %d data : 0x%02x ) \n",
                         frame_tx%1024, nr_slot_tx, N_UCI,  pucch_payload, O_ACK, pucch_ack_payload, O_SR, sr_payload, csi_status, csi_payload);

  NR_TST_PHY_PRINTF("PUCCH ( format : %d ) ( modulation : %s ) ( nb prb : %d ) ( nb symbols total: %d ) ( nb symbols : %d ) ( max code rate*100 : %d ) ( starting_symbol_index : %d ) \n",
                             format, (Q_m == BITS_PER_SYMBOL_QPSK ? " QPSK " : " BPSK "), nb_of_prbs, nb_symbols_total, nb_symbols, max_code_rate, starting_symbol_index);

  NR_TST_PHY_PRINTF("PUCCH ( startingPRB : %d ) ( secondHopPRB : %d ) ( m_0 : %d ) ( m_CS : %d ) ( time_domain_occ %d ) (occ_length : %d ) ( occ_Index : %d ) \n",
		    startingPRB (absolute),         secondHopPRB (absolute),         m_0,         m_CS,         time_domain_occ,      occ_length,         occ_Index);

  /* Part - IV */
  /* Generate PUCCH signal according to its format and parameters */
  ue->generate_ul_signal[gNB_id] = 1;

  int16_t pucch_tx_power = get_pucch_tx_power_ue( ue, gNB_id, proc, format,
                                                  nb_of_prbs, N_sc_ctrl_RB, nb_symbols, N_UCI, O_SR, O_CSI, O_ACK,
                                                  O_CRC, n_HARQ_ACK);

  /* set tx power */
  ue->tx_power_dBm[nr_slot_tx] = pucch_tx_power;
  ue->tx_total_RE[nr_slot_tx] = nb_of_prbs*N_SC_RB;

  int tx_amp;

#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)

  tx_amp = nr_get_tx_amp(pucch_tx_power,
                      ue->tx_power_max_dBm,
                      ue->frame_parms.N_RB_UL,
                      nb_of_prbs);
#else
  tx_amp = AMP;
#endif

  switch(format) {
    case pucch_format0_nr:
    {
      int pucch_GroupHopping = mac->ULbwp[bwp_id-1] ?
            mac->ULbwp[bwp_id-1]->bwp_Common->pucch_ConfigCommon->choice.setup->pucch_GroupHopping:
            mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pucch_ConfigCommon->choice.setup->pucch_GroupHopping;
      int hoppingId = mac->ULbwp[bwp_id-1] ?
            mac->ULbwp[bwp_id-1]->bwp_Common->pucch_ConfigCommon->choice.setup->hoppingId[0]:
            (mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pucch_ConfigCommon->choice.setup->hoppingId?
             mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pucch_ConfigCommon->choice.setup->hoppingId[0]:
             mac->physCellId);
      nr_generate_pucch0(ue,ue->common_vars.txdataF,
                         &ue->frame_parms,
                         pucch_GroupHopping,
                         hoppingId,
                         tx_amp,
                         nr_slot_tx,
                         (uint8_t)m_0,
                         (uint8_t)m_CS,
                         nb_symbols_total,
                         starting_symbol_index,
                         startingPRB,
                         secondHopPRB
                         );
      break;
    }
    case pucch_format1_nr:
    {
      nr_generate_pucch1(ue,ue->common_vars.txdataF,
                         &ue->frame_parms,
                         &ue->pucch_config_dedicated[gNB_id],
                         pucch_payload,
                         tx_amp,
                         nr_slot_tx,
                         (uint8_t)m_0,
                         nb_symbols_total,
                         starting_symbol_index,
                         startingPRB,
                         secondHopPRB,
                         (uint8_t)time_domain_occ,
                         (uint8_t)N_UCI);
      break;
    }
    case pucch_format2_nr:
    {
      nr_generate_pucch2(ue,
                         crnti,
			 dmrs_scrambling_id,
			 data_scrambling_id,
                         ue->common_vars.txdataF,
                         &ue->frame_parms,
                         &ue->pucch_config_dedicated[gNB_id],
                         pucch_payload,
                         tx_amp,
                         nr_slot_tx,
                         nb_symbols_total,
                         starting_symbol_index,
                         nb_of_prbs,
                         startingPRB,
                         (uint8_t)N_UCI);
      break;
    }
    case pucch_format3_nr:
    case pucch_format4_nr:
    {
      nr_generate_pucch3_4(ue,
                           0,//ue->pdcch_vars[proc->thread_id][gNB_id]->crnti,
                           ue->common_vars.txdataF,
                           &ue->frame_parms,
                           format,
                           &ue->pucch_config_dedicated[gNB_id],
                           pucch_payload,
                           tx_amp,
                           nr_slot_tx,
                           nb_symbols_total,
                           starting_symbol_index,
                           nb_of_prbs,
                           startingPRB,
                           secondHopPRB,
                           (uint8_t)N_UCI,
                           (uint8_t)occ_length,
                           (uint8_t)occ_Index);
      break;
    }
  }
  return (TRUE);
}

/*******************************************************************
*
* NAME :         get_downlink_ack
*
* PARAMETERS :   ue context
*                processing slots of reception/transmission
*                gNB_id identifier
*
* RETURN :       o_ACK acknowledgment data
*                o_ACK_number_bits number of bits for acknowledgment
*
* DESCRIPTION :  return acknowledgment value
*                TS 38.213 9.1.3 Type-2 HARQ-ACK codebook determination
*
*          --+--------+-------+--------+-------+---  ---+-------+--
*            | PDCCH1 |       | PDCCH2 |PDCCH3 |        | PUCCH |
*          --+--------+-------+--------+-------+---  ---+-------+--
*    DAI_DL      1                 2       3              ACK for
*                V                 V       V        PDCCH1, PDDCH2 and PCCH3
*                |                 |       |               ^
*                +-----------------+-------+---------------+
*
*                PDCCH1, PDCCH2 and PDCCH3 are PDCCH monitoring occasions
*                M is the total of monitoring occasions
*
*********************************************************************/

uint8_t get_downlink_ack(PHY_VARS_NR_UE *ue, uint8_t gNB_id,  UE_nr_rxtx_proc_t *proc,
                         uint32_t *o_ACK, int *n_HARQ_ACK,
                         bool do_reset) // 1 to reset ACK/NACK status : 0 otherwise
{
  NR_UE_HARQ_STATUS_t *harq_status;
  uint32_t ack_data[NR_DL_MAX_NB_CW][NR_DL_MAX_DAI] = {{0},{0}};
  uint32_t dai[NR_DL_MAX_NB_CW][NR_DL_MAX_DAI] = {{0},{0}};       /* for serving cell */
  uint32_t dai_total[NR_DL_MAX_NB_CW][NR_DL_MAX_DAI] = {{0},{0}}; /* for multiple cells */
  int number_harq_feedback = 0;
  uint32_t dai_current = 0;
  uint32_t dai_max = 0;
  int number_pid_dl = ue->dlsch[proc->thread_id][gNB_id][0]->number_harq_processes_for_pdsch;
  bool two_transport_blocks = FALSE;
  int number_of_code_word = 1;
  int U_DAI_c = 0;
  int N_m_c_rx = 0;
  int V_DAI_m_DL = 0;
  NR_UE_MAC_INST_t *mac = get_mac_inst(0);

  if (mac->DLbwp[0] &&
      mac->DLbwp[0]->bwp_Dedicated &&
      mac->DLbwp[0]->bwp_Dedicated->pdsch_Config &&
      mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup &&
      mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->maxNrofCodeWordsScheduledByDCI[0] == 2) {
    two_transport_blocks = TRUE;
    number_of_code_word = 2;
  }
  else {
    number_of_code_word = 1;
  }

  if (ue->n_connected_gNB > 1) {
    LOG_E(PHY,"PUCCH ACK feedback is not implemented for mutiple gNB cells : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
    return (0);
  }

  /* look for dl acknowledgment which should be done on current uplink slot */
  for (int code_word = 0; code_word < number_of_code_word; code_word++) {

    for (int dl_harq_pid = 0; dl_harq_pid < number_pid_dl; dl_harq_pid++) {

      for (int thread_idx = 0; thread_idx < RX_NB_TH; thread_idx++) {

        harq_status = &ue->dlsch[thread_idx][gNB_id][code_word]->harq_processes[dl_harq_pid]->harq_ack;

        /* check if current tx slot should transmit downlink acknowlegment */
        if (harq_status->slot_for_feedback_ack == proc->nr_slot_tx) {

          if (harq_status->ack == DL_ACKNACK_NO_SET) {
            LOG_E(PHY,"PUCCH Downlink acknowledgment has not been set : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
          }
          else if (harq_status->vDAI_DL == DL_DAI_NO_SET) {
            LOG_E(PHY,"PUCCH Downlink DAI has not been set : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
          }
          else if (harq_status->vDAI_DL > NR_DL_MAX_DAI) {
            LOG_E(PHY,"PUCCH Downlink DAI has an invalid value : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
          }
          else if (harq_status->send_harq_status == 0) {
            LOG_E(PHY,"PUCCH Downlink ack can not be transmitted : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
          }
          else {

            dai_current = harq_status->vDAI_DL+1; // DCI DAI to counter DAI conversion

            if (dai_current == 0) {
              LOG_E(PHY,"PUCCH Downlink dai is invalid : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
              return(0);
            } else if (dai_current > dai_max) {
              dai_max = dai_current;
            }

            number_harq_feedback++;
            ack_data[code_word][dai_current - 1] = harq_status->ack;
            dai[code_word][dai_current - 1] = dai_current;
            harq_status->slot_for_feedback_ack = NR_MAX_SLOTS_PER_FRAME;
            harq_status->send_harq_status = 0;
          }
          if (do_reset == TRUE) {
            init_downlink_harq_status(ue->dlsch[thread_idx][gNB_id][code_word]->harq_processes[dl_harq_pid]);
          }
        }
      }
    }
  }

  /* no any ack to transmit */
  if (number_harq_feedback == 0) {
    *n_HARQ_ACK = 0;
    return(0);
  }
  else  if (number_harq_feedback > (sizeof(uint32_t)*8)) {
    LOG_E(PHY,"PUCCH number of ack bits exceeds payload size : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
    return(0);
  }

  /* for computing n_HARQ_ACK for power */
   V_DAI_m_DL = dai_max;
   U_DAI_c = number_harq_feedback/number_of_code_word;
   N_m_c_rx = number_harq_feedback;
   int N_SPS_c = 0; /* FFS TODO_NR multicells and SPS are not supported at the moment */
   if (mac->cg != NULL &&
       mac->cg->physicalCellGroupConfig != NULL &&
       mac->cg->physicalCellGroupConfig->harq_ACK_SpatialBundlingPUCCH != NULL) {
     int N_TB_max_DL = mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->maxNrofCodeWordsScheduledByDCI[0];
     *n_HARQ_ACK = (((V_DAI_m_DL - U_DAI_c)%4) * N_TB_max_DL) + N_m_c_rx + N_SPS_c;
     NR_TST_PHY_PRINTF("PUCCH power n(%d) = ( V(%d) - U(%d) )mod4 * N_TB(%d) + N(%d) \n", *n_HARQ_ACK, V_DAI_m_DL, U_DAI_c, N_TB_max_DL, N_m_c_rx);
   }

  /*
  * For a monitoring occasion of a PDCCH with DCI format 1_0 or DCI format 1_1 in at least one serving cell,
  * when a UE receives a PDSCH with one transport block and the value of higher layer parameter maxNrofCodeWordsScheduledByDCI is 2,
  * the HARQ-ACK response is associated with the first transport block and the UE generates a NACK for the second transport block
  * if spatial bundling is not applied (HARQ-ACK-spatial-bundling-PUCCH = FALSE) and generates HARQ-ACK value of ACK for the second
  * transport block if spatial bundling is applied.
  */

  for (int code_word = 0; code_word < number_of_code_word; code_word++) {
    for (uint32_t i = 0; i < dai_max ; i++ ) {
      if (dai[code_word][i] != i + 1) { /* fill table with consistent value for each dai */
        dai[code_word][i] = i + 1;      /* it covers case for which PDCCH DCI has not been successfully decoded and so it has been missed */
        ack_data[code_word][i] = 0;     /* nack data transport block which has been missed */
        number_harq_feedback++;
      }
      if (two_transport_blocks == TRUE) {
        dai_total[code_word][i] = dai[code_word][i]; /* for a single cell, dai_total is the same as dai of first cell */
      }
    }
  }

  int M = dai_max;
  int j = 0;
  uint32_t V_temp = 0;
  uint32_t V_temp2 = 0;
  int O_ACK = 0;
  int O_bit_number_cw0 = 0;
  int O_bit_number_cw1 = 0;

  for (int m = 0; m < M ; m++) {

    if (dai[0][m] <= V_temp) {
      j = j + 1;
    }

    V_temp = dai[0][m]; /* value of the counter DAI for format 1_0 and format 1_1 on serving cell c */

    if (dai_total[0][m] == 0) {
      V_temp2 = dai[0][m];
    } else {
      V_temp2 = dai[1][m];         /* second code word has been received */
      O_bit_number_cw1 = (8 * j) + 2*(V_temp - 1) + 1;
      *o_ACK = *o_ACK | (ack_data[1][m] << O_bit_number_cw1);
    }

    if (two_transport_blocks == TRUE) {
      O_bit_number_cw0 = (8 * j) + 2*(V_temp - 1);
    }
    else {
      O_bit_number_cw0 = (4 * j) + (V_temp - 1);
    }

    *o_ACK = *o_ACK | (ack_data[0][m] << O_bit_number_cw0);
  }

  if (V_temp2 < V_temp) {
    j = j + 1;
  }

  if (two_transport_blocks == TRUE) {
    O_ACK = 2 * ( 4 * j + V_temp2);  /* for two transport blocks */
  }
  else {
    O_ACK = 4 * j + V_temp2;         /* only one transport block */
  }

  if (number_harq_feedback != O_ACK) {
    LOG_E(PHY,"PUCCH Error for number of bits for acknowledgment : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
    return (0);
  }

  return(number_harq_feedback);
}

/*******************************************************************
*
* NAME :         select_pucch_format
*
* PARAMETERS :   ue context
*                processing slots of reception/transmission
*                gNB_id identifier
*
* RETURN :       TRUE a valid resource has been found
*
* DESCRIPTION :  return tx harq process identifier for given transmission slot
*                TS 38.213 9.2.1  PUCCH Resource Sets
*                TS 38.213 9.2.2  PUCCH Formats for UCI transmission
*                In the case of pucch for scheduling request only, resource is already get from scheduling request configuration
*
*********************************************************************/

boolean_t select_pucch_resource(PHY_VARS_NR_UE *ue, NR_UE_MAC_INST_t *mac, uint8_t gNB_id, int uci_size, int pucch_resource_indicator, 
                                int *initial_pucch_id, int *resource_set_id, int *resource_id, NR_UE_HARQ_STATUS_t *harq_status)
{
  boolean_t resource_set_found = FALSE;
  int nb_symbols_for_tx = 0;
  int current_resource_id = MAX_NB_OF_PUCCH_RESOURCES;
  pucch_format_nr_t format_pucch;
  int ready_pucch_resource_id = FALSE; /* in the case that it is already given */
  NR_PUCCH_Resource_t *pucch_resource = NULL;
  NR_BWP_Id_t bwp_id = mac->UL_BWP_Id;

  /* ini values to unset */
  *initial_pucch_id = NB_INITIAL_PUCCH_RESOURCE;
  //*resource_set_id = MAX_NB_OF_PUCCH_RESOURCE_SETS;
  //*resource_id = MAX_NB_OF_PUCCH_RESOURCES;

  if ((bwp_id ==0 &&
       mac->cg == NULL) ||
      (bwp_id == 0 &&
       mac->cg &&
       mac->cg->spCellConfig &&
       mac->cg->spCellConfig->spCellConfigDedicated &&
       mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
       mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP &&
       mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config &&
       mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup &&
       mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList &&
       mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList->list.array[0] == NULL) ||
      (mac->ULbwp[bwp_id-1] &&
       mac->ULbwp[bwp_id-1]->bwp_Dedicated &&
       mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config &&
       mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup &&
       mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList &&
       mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList->list.array[0] == NULL)
      ){

    /* No resource set has been already configured so pucch_configCommon from Sib1 should be used in this case */

    if (ue->UE_mode[gNB_id] != PUSCH) {
      *initial_pucch_id = *mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pucch_ConfigCommon->choice.setup->pucch_ResourceCommon;
      if (*initial_pucch_id >= NB_INITIAL_PUCCH_RESOURCE) {
        LOG_E(PHY,"PUCCH Invalid initial resource index : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
        *initial_pucch_id = NB_INITIAL_PUCCH_RESOURCE;
        return (FALSE);
      }
    }
    else  {
      /* see TS 38.213 9.2.1  PUCCH Resource Sets */
      int delta_PRI = harq_status->pucch_resource_indicator;
      // n_CCE can be obtained from ue->dci_ind.dci_list[i].n_CCE. FIXME!!!
      // N_CCE can be obtained from ue->dci_ind.dci_list[i].N_CCE. FIXME!!!
      //int n_CCE = ue->dci_ind.dci_list[0].n_CCE;
      //int N_CCE = ue->dci_ind.dci_list[0].N_CCE;
      int n_CCE_0 = harq_status->n_CCE;
      int N_CCE_0 = harq_status->N_CCE;
      if (N_CCE_0 == 0) {
        AssertFatal(1==0,"PUCCH No compatible pucch format found : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
      }
      int r_PUCCH = ((2 * n_CCE_0)/N_CCE_0) + (2 * delta_PRI);
      *initial_pucch_id = r_PUCCH;
    }
    nb_symbols_for_tx = initial_pucch_resource[*initial_pucch_id].nrofSymbols;
    format_pucch = initial_pucch_resource[*initial_pucch_id].format;
    if (check_pucch_format(mac, gNB_id, format_pucch, nb_symbols_for_tx, uci_size) == TRUE) {
      return (TRUE);
    }
    else {
      LOG_E(PHY,"PUCCH No compatible pucch format found : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
      return (FALSE);
    }
  }
  else {
    /* dedicated resources have been configured */
    int pucch_resource_set_id = 0;
    if (*resource_set_id == MAX_NB_OF_PUCCH_RESOURCE_SETS) {
      /* from TS 38.331 field maxPayloadMinus1
        -- Maximum number of payload bits minus 1 that the UE may transmit using this PUCCH resource set. In a PUCCH occurrence, the UE
        -- chooses the first of its PUCCH-ResourceSet which supports the number of bits that the UE wants to transmit.
        -- The field is not present in the first set (Set0) since the maximum Size of Set0 is specified to be 3 bit.
        -- The field is not present in the last configured set since the UE derives its maximum payload size as specified in 38.213.
        -- This field can take integer values that are multiples of 4. Corresponds to L1 parameter 'N_2' or 'N_3' (see 38.213, section 9.2)
      */
      /* look for the first resource set which supports uci_size number of bits for payload */
      pucch_resource_set_id = find_pucch_resource_set(mac, gNB_id, uci_size);
      if (pucch_resource_set_id != MAX_NB_OF_PUCCH_RESOURCE_SETS) {
        resource_set_found = TRUE;
      }
    }
    else {
      /* a valid resource has already be found outside this function */
      resource_set_found = TRUE;
      ready_pucch_resource_id = TRUE;
      //pucch_resource_indicator = pucch_resource_indicator;
    }

    if (resource_set_found == TRUE) {

      if (pucch_resource_indicator < MAX_PUCCH_RESOURCE_INDICATOR) {
        // Verify that the value of pucch_resource_indicator is valid
        struct NR_PUCCH_Config__resourceSetToAddModList *resourceSetToAddModList = NULL;
	      struct NR_PUCCH_Config__resourceToAddModList *resourceToAddModList = NULL;
        if (bwp_id > 0 && mac->ULbwp[bwp_id-1]) {
           AssertFatal(mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList!=NULL,"mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList is null\n");
           resourceSetToAddModList = mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList;
           resourceToAddModList = mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceToAddModList;
        } else if (bwp_id == 0 && mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList!=NULL) {
	        resourceSetToAddModList = mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList;
          resourceToAddModList = mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceToAddModList;
        }
        if (resourceSetToAddModList->list.array[pucch_resource_set_id]->resourceList.list.count <= pucch_resource_indicator)
        {
          LOG_E(PHY, "Value of pucch_resource_indicator is out of bounds! Possibly due to a false DCI. \n");
          return (FALSE);
        }
        /* check if resource indexing by pucch_resource_indicator of this set is compatible */
        if ((ready_pucch_resource_id == TRUE) || (resourceSetToAddModList->list.array[pucch_resource_set_id]->resourceList.list.array[pucch_resource_indicator][0] != MAX_NB_OF_PUCCH_RESOURCES)) {

          if (ready_pucch_resource_id == TRUE) {
            current_resource_id = *resource_id;
          }
          else {
            int R_PUCCH = resourceSetToAddModList->list.array[pucch_resource_set_id]->resourceList.list.count;
            /* is it the first resource and its size exceeds 8 */
            if ((pucch_resource_set_id == 0)
             && (R_PUCCH > MAX_NB_OF_PUCCH_RESOURCES_PER_SET_NOT_0)) {
              /* see TS 38.213 9.2.3  UE procedure for reporting HARQ-ACK */
              int delta_PRI = pucch_resource_indicator;
              int n_CCE_p = harq_status->n_CCE;
              int N_CCE_p = harq_status->N_CCE;
              int r_PUCCH;
              if (N_CCE_p == 0) {
                LOG_E(PHY,"PUCCH No compatible pucch format found : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
                return (FALSE);
              }
              if (pucch_resource_set_id < (R_PUCCH%8)) {
                r_PUCCH = ((n_CCE_p * (R_PUCCH/8))/N_CCE_p) + (delta_PRI*(R_PUCCH/8));
              }
              else {
                r_PUCCH = ((n_CCE_p * (R_PUCCH/8))/N_CCE_p) + (delta_PRI*(R_PUCCH/8)) + (R_PUCCH%8);
              }
              current_resource_id = r_PUCCH;
            }
            else {
		          current_resource_id = resourceSetToAddModList->list.array[pucch_resource_set_id]->resourceList.list.array[pucch_resource_indicator][0];
            }
          }

          uint8_t pucch_resource_count = resourceToAddModList->list.count;
          for (uint8_t i=0; i<pucch_resource_count; i++) {
            if (resourceToAddModList->list.array[i]->pucch_ResourceId == current_resource_id)
              pucch_resource = resourceToAddModList->list.array[i];
          }
          if (pucch_resource != NULL) {
            format_pucch = pucch_resource->format.present;
            nb_symbols_for_tx = get_nb_symbols_pucch(pucch_resource, format_pucch);
            if (check_pucch_format(mac, gNB_id, format_pucch, nb_symbols_for_tx, uci_size) == TRUE) {
              *resource_set_id = pucch_resource_set_id;
              *resource_id = current_resource_id;
              return (TRUE);
            }
            else {
              LOG_E(PHY,"PUCCH Found format no compatible with payload size and symbol length : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
              return (FALSE);
            }
          }
        }
        else {
          LOG_E(PHY,"PUCCH Undefined Resource related to pucch resource indicator: at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
          return (FALSE);
        }
      }
      else {
        LOG_E(PHY,"PUCCH Invalid pucch resource indicator: at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
        return (FALSE);
      }
    }

    /* check that a resource has been found */
    if (*resource_set_id == MAX_NB_OF_PUCCH_RESOURCES) {
      LOG_E(PHY,"PUCCH No compatible pucch format found : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
      return (FALSE);
    }
  }
  return (FALSE);
}

/*******************************************************************
*
* NAME :         find_pucch_resource_set
*
* PARAMETERS :   ue context
*                gNB_id identifier
*
*
* RETURN :       harq process identifier
*
* DESCRIPTION :  return tx harq process identifier for given transmission slot
*                YS 38.213 9.2.2  PUCCH Formats for UCI transmission
*
*********************************************************************/

int find_pucch_resource_set(NR_UE_MAC_INST_t *mac, uint8_t gNB_id, int uci_size)
{
  int pucch_resource_set_id = 0;
  NR_BWP_Id_t bwp_id = mac->DL_BWP_Id;

  //long *pucch_max_pl_bits = NULL;

  /* from TS 38.331 field maxPayloadMinus1
    -- Maximum number of payload bits minus 1 that the UE may transmit using this PUCCH resource set. In a PUCCH occurrence, the UE
    -- chooses the first of its PUCCH-ResourceSet which supports the number of bits that the UE wants to transmit.
    -- The field is not present in the first set (Set0) since the maximum Size of Set0 is specified to be 3 bit.
    -- The field is not present in the last configured set since the UE derives its maximum payload size as specified in 38.213.
    -- This field can take integer values that are multiples of 4. Corresponds to L1 parameter 'N_2' or 'N_3' (see 38.213, section 9.2)
  */
  /* look for the first resource set which supports uci_size number of bits for payload */
  while (pucch_resource_set_id < MAX_NB_OF_PUCCH_RESOURCE_SETS) {
    if ((bwp_id>0 &&
         mac->ULbwp[bwp_id-1] &&
         mac->ULbwp[bwp_id-1]->bwp_Dedicated &&
         mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config &&
         mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup &&
         mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList &&
         mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->resourceSetToAddModList->list.array[pucch_resource_set_id] != NULL) ||
        (bwp_id==0 &&
         mac->cg &&
         mac->cg->spCellConfig &&
         mac->cg->spCellConfig->spCellConfigDedicated &&
         mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
         mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP &&
         mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config &&
         mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup &&
         mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList &&
         mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup->resourceSetToAddModList->list.array[pucch_resource_set_id] != NULL)) {
      if (uci_size <= 2) {
        pucch_resource_set_id = 0;
        return (pucch_resource_set_id);
        break;
      }
      else {
        pucch_resource_set_id = 1;
        return (pucch_resource_set_id);
        break;
      }
    }
    pucch_resource_set_id++;
  }

  pucch_resource_set_id = MAX_NB_OF_PUCCH_RESOURCE_SETS;

  return (pucch_resource_set_id);
}

/*******************************************************************
*
* NAME :         check_pucch_format
*
* PARAMETERS :   ue context
*                processing slots of reception/transmission
*                gNB_id identifier
*
* RETURN :       harq process identifier
*
* DESCRIPTION :  return tx harq process identifier for given transmission slot
*                YS 38.213 9.2.2  PUCCH Formats for UCI transmission
*
*********************************************************************/

boolean_t check_pucch_format(NR_UE_MAC_INST_t *mac, uint8_t gNB_id, pucch_format_nr_t format_pucch, int nb_symbols_for_tx, int uci_size)
{
  pucch_format_nr_t selected_pucch_format;
  pucch_format_nr_t selected_pucch_format_second;
  /*NR_SetupRelease_PUCCH_FormatConfig_t *identified_format = NULL;

  switch (format_pucch) {
    case pucch_format1_nr:
    if (mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format1 != NULL)
      identified_format = mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format1;
    break;

    case pucch_format2_nr:
    if (mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format2 != NULL)
      identified_format = mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format2;
    break;

    case pucch_format3_nr:
    if (mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format3 != NULL)
      identified_format = mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format3;
    break;

    case pucch_format4_nr:
    if (mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format4 != NULL)
      identified_format = mac->ULbwp[bwp_id-1]->bwp_Dedicated->pucch_Config->choice.setup->format4;
    break;

    default:
    break;
  }*/

 /* if ((identified_format != NULL) && (identified_format->choice.setup->nrofSlots[0] != 1)) {
    LOG_E(PHY,"PUCCH not implemented multislots transmission : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
    return (FALSE);
  }*/

  if (nb_symbols_for_tx <= 2) {
    if (uci_size <= 2) {
      selected_pucch_format = pucch_format0_nr;
      selected_pucch_format_second = selected_pucch_format;
    }
    else {
      selected_pucch_format = pucch_format2_nr;
      selected_pucch_format_second = selected_pucch_format;
    }
  }
  else {
    if (nb_symbols_for_tx >= 4) {
      if (uci_size <= 2) {
        selected_pucch_format = pucch_format1_nr;
        selected_pucch_format_second = selected_pucch_format;
      }
      else {
        selected_pucch_format = pucch_format3_nr;  /* in this case choice can be done between two formats */
        selected_pucch_format_second = pucch_format4_nr;
      }
    }
    else {
      LOG_D(PHY,"PUCCH Undefined PUCCH format : set PUCCH to format 4 : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
      return (FALSE);
    }
  }

  NR_TST_PHY_PRINTF("PUCCH format %d nb symbols total %d uci size %d selected format %d \n", format_pucch, nb_symbols_for_tx, uci_size, selected_pucch_format);

  if (format_pucch != selected_pucch_format) {
    if (format_pucch != selected_pucch_format_second) {
      NR_TST_PHY_PRINTF("PUCCH mismatched of selected format: at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
      LOG_D(PHY,"PUCCH format mismatched of selected format: at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
      return (FALSE);
    }
    else {
      return (TRUE);
    }
  }
  else {
    return (TRUE);
  }
}

/*******************************************************************
*
* NAME :         trigger_periodic_scheduling_request
*
* PARAMETERS :   pointer to resource set
*
* RETURN :       1 if peridic scheduling request is triggered
*                0 no periodic scheduling request
*
* DESCRIPTION :  TS 38.213 9.2.4 UE procedure for reporting SR
*
*********************************************************************/

int trigger_periodic_scheduling_request(PHY_VARS_NR_UE *ue, uint8_t gNB_id, UE_nr_rxtx_proc_t *proc)
{
  const int max_sr_periodicity[NB_NUMEROLOGIES_NR] = { 80, 160, 320, 640, 640 };

  int active_scheduling_request = ue->scheduling_request_config_nr[gNB_id].active_sr_id;

  /* is there any valid scheduling request configuration */
  if (ue->scheduling_request_config_nr[gNB_id].sr_ResourceConfig[active_scheduling_request] == NULL) {
    return (0);
  }

  if (ue->scheduling_request_config_nr[gNB_id].sr_ResourceConfig[active_scheduling_request]->periodicity < 2) {
    LOG_W(PHY,"PUCCH Not supported scheduling request period smaller than 1 slot : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
    return (0);
  }

  int16_t SR_periodicity = scheduling_request_periodicity[ue->scheduling_request_config_nr[gNB_id].sr_ResourceConfig[active_scheduling_request]->periodicity];
  uint16_t SR_offset = ue->scheduling_request_config_nr[gNB_id].sr_ResourceConfig[active_scheduling_request]->offset;

  if (SR_periodicity > max_sr_periodicity[ue->frame_parms.numerology_index]) {
    LOG_W(PHY,"PUCCH Invalid scheduling request period : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
    return (0);
  }

  if (SR_offset > SR_periodicity) {
    LOG_E(PHY,"PUCCH SR offset %d is greater than SR periodicity %d : at line %d in function %s of file %s \n", SR_offset, SR_periodicity, LINE_FILE , __func__, FILE_NAME);
    return (0);
  }
  else if (SR_periodicity == 1) {
    return (1); /* period is slot */
  }

  int16_t N_slot_frame = ue->frame_parms.slots_per_frame;
  if (((proc->frame_tx * N_slot_frame) + proc->nr_slot_tx - SR_offset)%SR_periodicity == 0) {
    return (1);
  }
  else {
    return (0);
  }
}

/*******************************************************************
*
* NAME :         get_csi_nr
* PARAMETERS :   ue context
*                processing slots of reception/transmission
*                gNB_id identifier
*
* RETURN :       size of csi payload
*
* DESCRIPTION :  CSI management is not already implemented
*                so it has been simulated thank to two functions:
*                - set_csi_nr
*                - get_csi_nr
*
*********************************************************************/

int      dummy_csi_status = 0;
uint32_t dummy_csi_payload = 0;

/* FFS TODO_NR code that should be developed */

uint16_t get_nr_csi_bitlen(NR_UE_MAC_INST_t *mac) {

  uint16_t csi_bitlen =0;
  uint16_t rsrp_bitlen = 0;
  uint16_t diff_rsrp_bitlen = 0;
  uint16_t nb_ssbri_cri = 0; 
  uint16_t cri_ssbri_bitlen = 0;
  
  NR_CSI_MeasConfig_t *csi_MeasConfig = mac->cg->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup;
  struct NR_CSI_ResourceConfig__csi_RS_ResourceSetList__nzp_CSI_RS_SSB * nzp_CSI_RS_SSB = csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[0]->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB;

  uint16_t nb_csi_ssb_report = nzp_CSI_RS_SSB->csi_SSB_ResourceSetList!=NULL ? nzp_CSI_RS_SSB->csi_SSB_ResourceSetList->list.count:0;
  
  if (0 != nb_csi_ssb_report){
	  uint8_t nb_ssb_resources =0;
	  
  if (NULL != csi_MeasConfig->csi_ReportConfigToAddModList->list.array[0]->groupBasedBeamReporting.choice.disabled->nrofReportedRS)
      nb_ssbri_cri = *(csi_MeasConfig->csi_ReportConfigToAddModList->list.array[0]->groupBasedBeamReporting.choice.disabled->nrofReportedRS)+1;
  else
      nb_ssbri_cri = 1;
  
  nb_ssb_resources = csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[0]->csi_SSB_ResourceList.list.count;
  
  if (nb_ssb_resources){
	cri_ssbri_bitlen =ceil(log2 (nb_ssb_resources));
	rsrp_bitlen = 7;
	diff_rsrp_bitlen = 4;
	}
  else{
	cri_ssbri_bitlen =0;
	rsrp_bitlen = 0;
	diff_rsrp_bitlen = 0;
	}
  
  csi_bitlen = ((cri_ssbri_bitlen * nb_ssbri_cri) + rsrp_bitlen +(diff_rsrp_bitlen *(nb_ssbri_cri -1 ))) *nb_csi_ssb_report;
               
  //printf("get csi bitlen %d nb_ssbri_cri %d nb_csi_report %d nb_resources %d\n", csi_bitlen,nb_ssbri_cri ,nb_csi_ssb_report, nb_ssb_resources);
  }
  return csi_bitlen;
}

int get_csi_nr(NR_UE_MAC_INST_t *mac, PHY_VARS_NR_UE *ue, uint8_t gNB_id, uint32_t *csi_payload)
{
  VOID_PARAMETER ue;
  VOID_PARAMETER gNB_id;
  float rsrp_db[7];
  int nElem = 98;
  int rsrp_offset = 17;
  int csi_status = 0;
  
  csi_status = get_nr_csi_bitlen(mac);
  rsrp_db[0] = get_nr_RSRP(0,0,0);


  if (csi_status == 0) {
    *csi_payload = 0;
  }
  else {
    *csi_payload = binary_search_float_nr(RSRP_meas_mapping_nr,nElem, rsrp_db[0]) + rsrp_offset;
  }

  return (csi_status);
}

/* FFS TODO_NR code that should be removed */

void set_csi_nr(int csi_status, uint32_t csi_payload)
{
  dummy_csi_status = csi_status;

  if (dummy_csi_status == 0) {
    dummy_csi_payload = 0;
  }
  else {
    dummy_csi_payload = csi_payload;
  }
}

uint8_t get_nb_symbols_pucch(NR_PUCCH_Resource_t *pucch_resource, pucch_format_nr_t format_type)
{
  switch (format_type) {
    case pucch_format0_nr:
      return pucch_resource->format.choice.format0->nrofSymbols;

    case pucch_format1_nr:
      return pucch_resource->format.choice.format1->nrofSymbols;

    case pucch_format2_nr:
      return pucch_resource->format.choice.format2->nrofSymbols;

    case pucch_format3_nr:
      return pucch_resource->format.choice.format3->nrofSymbols;

    case pucch_format4_nr:
      return pucch_resource->format.choice.format4->nrofSymbols;
  }
  return 0;
}

uint16_t get_starting_symb_idx(NR_PUCCH_Resource_t *pucch_resource, pucch_format_nr_t format_type)
{
  switch (format_type) {
    case pucch_format0_nr:
      return pucch_resource->format.choice.format0->startingSymbolIndex;

    case pucch_format1_nr:
      return pucch_resource->format.choice.format1->startingSymbolIndex;

    case pucch_format2_nr:
      return pucch_resource->format.choice.format2->startingSymbolIndex;

    case pucch_format3_nr:
      return pucch_resource->format.choice.format3->startingSymbolIndex;

    case pucch_format4_nr:
      return pucch_resource->format.choice.format4->startingSymbolIndex;
  }
  return 0;
}

int get_ics_pucch(NR_PUCCH_Resource_t *pucch_resource, pucch_format_nr_t format_type)
{
  switch (format_type) {
    case pucch_format0_nr:
      return pucch_resource->format.choice.format0->initialCyclicShift;

    case pucch_format1_nr:
      return pucch_resource->format.choice.format1->initialCyclicShift;
      
    case pucch_format2_nr:
      return 0;

    default:
      return -1;
  }
  return -1;
}

NR_PUCCH_Resource_t *select_resource_by_id(int resource_id, NR_PUCCH_Config_t *pucch_config)
{
  int n_list = pucch_config->resourceToAddModList->list.count; 
  NR_PUCCH_Resource_t *pucchres;
  AssertFatal(n_list>0,"PUCCH resourceToAddModList is empty\n");

  for (int i=0; i<n_list; i++) {
    pucchres = pucch_config->resourceToAddModList->list.array[i];
    if (pucchres->pucch_ResourceId == resource_id)
      return pucchres;
  }
  return NULL;
}

