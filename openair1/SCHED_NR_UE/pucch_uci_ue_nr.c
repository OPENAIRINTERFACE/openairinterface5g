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

#define DEFINE_VARIABLES_PUCCH_UE_NR_H
#include "SCHED_NR_UE/pucch_uci_ue_nr.h"
#undef DEFINE_VARIABLES_PUCCH_UE_NR_H

#endif


uint8_t nr_is_cqi_TXOp(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t gNB_id);
uint8_t nr_is_ri_TXOp(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t gNB_id);

/* TS 36.213 Table 9.2.5.2-1: Code rate  corresponding to higher layer parameter PUCCH-F2-maximum-coderate, */
/* or PUCCH-F3-maximum-coderate, or PUCCH-F4-maximum-coderate */
/* add one additional element set to 0 for parsing the array until this end */
/* stored values are code rates * 100 */
//static const int code_rate_r_time_100[8] = { (0.08 * 100), (0.15 * 100), (0.25*100), (0.35*100), (0.45*100), (0.60*100), (0.80*100), 0 } ;

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

void pucch_procedures_ue_nr(PHY_VARS_NR_UE *ue, 
                            uint8_t gNB_id,
                            UE_nr_rxtx_proc_t *proc) {

  int       nr_slot_tx = proc->nr_slot_tx;
  fapi_nr_ul_config_pucch_pdu *pucch_pdu;
  NR_UE_PUCCH *pucch_vars = ue->pucch_vars[proc->thread_id][gNB_id];

  for (int i=0; i<2; i++) {
    if(pucch_vars->active[i]) {
      pucch_pdu = &pucch_vars->pucch_pdu[i];
      uint16_t nb_of_prbs = pucch_pdu->prb_size;
      /* Generate PUCCH signal according to its format and parameters */
      ue->generate_ul_signal[gNB_id] = 1;

      int16_t PL = get_nr_PL(ue->Mod_id, ue->CC_id, gNB_id); /* LTE function because NR path loss not yet implemented FFS TODO NR */
      int contributor = (10 * log10((double)(pow(2,(ue->frame_parms.numerology_index)) * nb_of_prbs)));

      int16_t pucch_tx_power = pucch_pdu->pucch_tx_power + contributor + PL;

      if (pucch_tx_power > ue->tx_power_max_dBm)
        pucch_tx_power = ue->tx_power_max_dBm;

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

      switch(pucch_pdu->format_type) {
        case 0:
          nr_generate_pucch0(ue,
                             ue->common_vars.txdataF,
                             &ue->frame_parms,
                             tx_amp,
                             nr_slot_tx,
                             pucch_pdu);
          break;
        case 1:
          nr_generate_pucch1(ue,
                             ue->common_vars.txdataF,
                             &ue->frame_parms,
                             tx_amp,
                             nr_slot_tx,
                             pucch_pdu);
          break;
        case 2:
          nr_generate_pucch2(ue,
                             ue->common_vars.txdataF,
                             &ue->frame_parms,
                             tx_amp,
                             nr_slot_tx,
                             pucch_pdu);
          break;
        case 3:
        case 4:
          nr_generate_pucch3_4(ue,
                               ue->common_vars.txdataF,
                               &ue->frame_parms,
                               tx_amp,
                               nr_slot_tx,
                               pucch_pdu);
          break;
      }
    }
    pucch_vars->active[i] = false;
  }
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

