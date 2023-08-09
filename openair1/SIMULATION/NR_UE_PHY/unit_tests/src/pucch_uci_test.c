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

/**********************************************************************
*
* FILENAME    :  pucch_uci_test.c
*
* MODULE      :  UE test bench for pucch tests
*
* DESCRIPTION :  it allows unitary tests of PUCCH UCI on host machine
*
************************************************************************/

#include "../../unit_tests/src/pss_util_test.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/INIT/init_extern.h"
#include "PHY/phy_extern_nr_ue.h"

#include "SCHED_NR_UE/defs.h"
#include "SCHED_NR/extern.h"

#include "SCHED_NR_UE/harq_nr.h"
#include "SCHED_NR_UE/pucch_uci_ue_nr.h"


/**************** define **************************************/

#define TST_GNB_ID_0                       (0)         /* first index of gNB */
#define TST_THREAD_ID                      (0)

#define TST_N_BWP_SIZE                     (100)       /* band width for pucch */

#define TST_PUCCH_COMMON_CONFIG_INDEX_KO   (6)
#define TST_PUCCH_COMMON_CONFIG_INDEX_OK   (1)

#define TST_DL_HARQ_PID_FIRST              (1)
#define TST_DL_HARQ_PID_SECOND             (3)
#define TST_DL_HARQ_PID_THIRD              (4)
#define TST_DL_HARQ_PID_FOURTH             (0)

#define TST_NB_RESOURCE_SET                (MAX_NB_OF_PUCCH_RESOURCE_SETS)
#define TST_NB_RESOURCES                   (32)
#define TST_NB_RESOURCES_PER_SET           (8)

/* pucch power control */
#define TST_P0_NOMINAL                     (3)    /* -202..24 */
#define TST_P0_PUCCH_VALUE                 (0)    /* -16..15  */
#define TST_DELTA_F                        (1)    /* -16..15  */
#define TST_TX_POWER_MAX_DBM               (24)   /* -202..24 */


/**************** extern *****************************************/

extern uint32_t (*p_nr_ue_get_SR)(module_id_t module_idP,int CC_id,frame_t frameP,uint8_t eNB_id,uint16_t rnti, sub_frame_t subframe);

/**************** variables **************************************/

uint32_t tst_scheduling_request_payload;

const char tst_separator[255] = "\n-------------------------------------------------------------------------------------------\n";

/*******************************************************************
*
* NAME :         init_pucch_power_configuration
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  set power onfiguration
*
*********************************************************************/

void init_pucch_power_configuration(PHY_VARS_NR_UE *ue, uint8_t gNB_id) {

  ue->pucch_config_common_nr[gNB_id].p0_nominal = TST_P0_NOMINAL;

  PUCCH_PowerControl_t *power_config = &ue->pucch_config_dedicated_nr[gNB_id].pucch_PowerControl;

  P0_PUCCH_t *p_p0_pucch = calloc( 1, sizeof(P0_PUCCH_t));

  p_p0_pucch->p0_PUCCH_Value = TST_P0_PUCCH_VALUE;
  p_p0_pucch->p0_PUCCH_Id = 1;

  power_config->p0_Set[0] = p_p0_pucch;

  for (int i = 0; i < NUMBER_PUCCH_FORMAT_NR; i++) {
    power_config->deltaF_PUCCH_f[i] = TST_DELTA_F + i;
  }

  ue->tx_power_max_dBm = TST_TX_POWER_MAX_DBM;
}

/*******************************************************************
*
* NAME :         init_pucch_configuration
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  set srs default configuration
*
*********************************************************************/

void common_pucch_configuration(PHY_VARS_NR_UE *ue, uint8_t gNB_id, int pucch_index_config)
{
  printf("\nSet pucch common configuration \n");

  for (int i = 0; i < MAX_NB_OF_PUCCH_RESOURCES; i++) {
    ue->pucch_config_dedicated_nr[gNB_id].PUCCH_Resource[i] = NULL;
  }

  for (int i = 0; i < MAX_NB_OF_PUCCH_RESOURCE_SETS; i++) {
    ue->pucch_config_dedicated_nr[gNB_id].PUCCH_ResourceSet[i] = NULL;
  }

  for (int i = 0; i < NUMBER_PUCCH_FORMAT_NR-1; i++) {
    ue->pucch_config_dedicated_nr[gNB_id].formatConfig[i] = NULL;
  }

  ue->pucch_config_common_nr[gNB_id].pucch_ResourceCommon = pucch_index_config;

  /* default number of pdsch code words */
  ue->PDSCH_Config.maxNrofCodeWordsScheduledByDCI = nb_code_n1;

  /* init of band width part for pucch */
  ue->systemInformationBlockType1_nr.N_BWP_SIZE = TST_N_BWP_SIZE;

  /* init default state */
  ue->UE_mode[gNB_id] = PRACH;
}

/*******************************************************************
*
* NAME :         test_pucch_basic_error
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  test of PUCCH error cases
*                in this case no transmission occurs
*
*********************************************************************/

int test_pucch_basic_error(PHY_VARS_NR_UE *ue, int gNB_id, UE_nr_rxtx_proc_t *proc)
{
  int v_return = 0;
  bool reset_harq = false;

  printf("%s", tst_separator);

  ue->dlsch[proc->thread_id][gNB_id][0]->current_harq_pid = TST_DL_HARQ_PID_FIRST;

  printf("\n  => Test : No harq feedback on going\n");

  common_pucch_configuration(ue, gNB_id, TST_PUCCH_COMMON_CONFIG_INDEX_KO);

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != false) {
    v_return = -1;
  }

  /* set a tx slot with no ack */
  NR_UE_HARQ_STATUS_t *harq_status = &ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[TST_DL_HARQ_PID_FIRST].harq_ack;

  harq_status->slot_for_feedback_ack  = proc->nr_slot_tx;

  printf("\n  => Test : Error due to acknownlegment not set \n");

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != false) {
    v_return = -1;
  }

  harq_status->ack = DL_ACK;

  printf("\n  => Test : Error due to DAI not set \n");

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != false) {
    v_return = -1;
  }

  printf("\n  => Test : Error due to DAI at wrong value \n");

  harq_status->vDAI_DL = NR_DL_MAX_DAI + 1;

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != false) {
    v_return = -1;
  }

  printf("\n  => Test : Error due to ACK not ready to be send \n");

  harq_status->vDAI_DL = 0;

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != false) {
    v_return = -1;
  }

  printf("\n  => Test : Error due to invalid DAI value \n");

  harq_status->send_harq_status = 1;  /* ack ready to be send */

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != false) {
    v_return = -1;
  }

  common_pucch_configuration(ue, gNB_id, NB_INITIAL_PUCCH_RESOURCE);

  printf("\n  => Test : Error due to undefined PUCCH format from common config  \n");

  harq_status->vDAI_DL = 1;

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != false) {
    v_return = -1;
  }

  ue->n_connected_gNB = 2;

  printf("\n  => Test : Error due to PUCCH format with multiple cells not already implemented \n");

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != false) {
    v_return = -1;
  }

  ue->n_connected_gNB = 1;

  return (v_return);
}

/*******************************************************************
*
* NAME :         test_pucch_common_config_single_transport_block
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  test of PUCCH from common configuration*
*
*********************************************************************/

int test_pucch_common_config_single_transport_block(PHY_VARS_NR_UE *ue, int gNB_id, UE_nr_rxtx_proc_t *proc)
{
  int v_return = 0;
  bool reset_harq = false;

  printf("%s", tst_separator);

  common_pucch_configuration(ue, gNB_id, TST_PUCCH_COMMON_CONFIG_INDEX_OK);

  /* set a tx slot with no ack */
  NR_UE_HARQ_STATUS_t *harq_status = &ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[TST_DL_HARQ_PID_FIRST].harq_ack;

  harq_status->ack = DL_ACK;

  printf("\n  => Test : PUCCH format from common config in prach mode: one positive downlink ACKnowledgment \n");

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
   v_return = -1;
  }

  harq_status->ack = DL_NACK;

  printf("\n  => Test : PUCCH format from common config in prach mode: one negative downlink ACKnowledgment \n");

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
   v_return = -1;
  }

  /* switch ue mode after PRACH phase */
  ue->UE_mode[gNB_id] = PUSCH;

  /* set a second ack on a different downlink harq process */

  ue->dlsch[proc->thread_id][gNB_id][0]->current_harq_pid = TST_DL_HARQ_PID_SECOND;

  harq_status = &ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[TST_DL_HARQ_PID_SECOND].harq_ack;

  harq_status->n_CCE = 2;
  harq_status->N_CCE = 4;
  harq_status->pucch_resource_indicator = 1;
  harq_status->send_harq_status = 1;
  harq_status->vDAI_DL = 2;
  harq_status->ack = DL_ACK;
  harq_status->slot_for_feedback_ack  = proc->nr_slot_tx;

  printf("\n  => Test : PUCCH format from common config in dedicated mode: two positive downlink ACKnowledgments \n");

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    v_return = -1;
  }

  harq_status->ack = DL_NACK;

  printf("\n  => Test : PUCCH format from common config in dedicated mode: one positive and one negative downlink ACKnowledgments \n");

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    v_return = -1;
  }

  ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[TST_DL_HARQ_PID_FIRST].harq_ack.ack = DL_NACK;

  printf("\n  => Test : PUCCH format from common config in dedicated mode: two negative downlink ACKnowledgments \n");

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    v_return = -1;
  }

  harq_status->ack = DL_ACK;
  reset_harq = true;

  printf("\n  => Test : PUCCH format from common config in dedicated mode: no resource is found \n");

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != false) {
    v_return = -1;
  }

  printf("\n  => Test : PUCCH format from common config in dedicated mode: no PUCCH after reset of pending harq ACKnowledgments \n");

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != false) {
    v_return = -1;
  }

  return (v_return);
}

/*******************************************************************
*
* NAME :         init_pucch_dedicated_configuration
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  initialisation of PUCCH from dedicated configuration
*
*********************************************************************/

void init_pucch_dedicated_configuration(PHY_VARS_NR_UE *ue, uint8_t gNB_id)
{
  PUCCH_FormatConfig_t *p_format;
  PUCCH_Resource_t     *p_resource;
  PUCCH_ResourceSet_t  *p_resource_set;
  int i,j;

  /* set cell group parameters which are supported */
  ue->cell_group_config.physicalCellGroupConfig.harq_ACK_SpatialBundlingPUCCH = false;
  ue->cell_group_config.physicalCellGroupConfig.pdsch_HARQ_ACK_Codebook = dynamic;

  ue->PDSCH_ServingCellConfig.codeBlockGroupTransmission = NULL;
  ue->PDSCH_ServingCellConfig.nrofHARQ_ProcessesForPDSCH = 8;

  printf("\nSet default pucch dedicated configuration \n");

  for (i = 0; i < NB_DL_DATA_TO_UL_ACK; i++) {
    ue->pucch_config_dedicated_nr[gNB_id].dl_DataToUL_ACK[i] = i + NR_UE_CAPABILITY_SLOT_RX_TO_TX;
  }

  /* set configuration for format 1 to 4 */
  for (i = 0; i < NUMBER_PUCCH_FORMAT_NR-1; i++) {

    p_format = calloc( 1, sizeof(PUCCH_FormatConfig_t));

    if (p_format == NULL) {
     printf("Error of memory allocation : at line %d in function %s of file %s \n", LINE_FILE , __func__, __FILE__);
     assert(0);
    }
    else {
      //printf("Allocation %p \n", p_format);
      p_format->interslotFrequencyHopping = feature_disabled;
      p_format->additionalDMRS = disable_feature;
      p_format->maxCodeRate = i;
      p_format->nrofSlots = 1;
      p_format->pi2PBSK = feature_disabled;
      p_format->simultaneousHARQ_ACK_CSI = disable_feature;
    }

    ue->pucch_config_dedicated_nr[gNB_id].formatConfig[i] = p_format;
  }

  /* 4 resource set with 8 resources */
  /* for set 0 only format 0 and 1 can be used */
  const pucch_format_nr_t tst_pucch_format[TST_NB_RESOURCE_SET][TST_NB_RESOURCES_PER_SET] = {
/* set 0 */ { pucch_format0_nr, pucch_format1_nr, pucch_format0_nr, pucch_format1_nr, pucch_format0_nr, pucch_format1_nr, pucch_format0_nr, pucch_format1_nr},
/* set 1 */ { pucch_format2_nr, pucch_format3_nr, pucch_format4_nr, pucch_format2_nr, pucch_format3_nr, pucch_format4_nr, pucch_format2_nr, pucch_format3_nr},
/* set 2 */ { pucch_format2_nr, pucch_format3_nr, pucch_format4_nr, pucch_format2_nr, pucch_format3_nr, pucch_format4_nr, pucch_format2_nr, pucch_format3_nr},
/* set 3 */ { pucch_format3_nr, pucch_format3_nr, pucch_format4_nr, pucch_format3_nr, pucch_format3_nr, pucch_format4_nr, pucch_format3_nr, pucch_format3_nr},
  };

  /* from TS 38.331 The field is not present in the first set (Set0) since the maximum Size of Set0 is specified to be 3 bit. */
  const int tst_max_payload[TST_NB_RESOURCE_SET] = { 1, 2, 4, 9 };

  int counter_resource_id = 0;

  /* create TST_NB_RESOURCE_SET resources set with resources according to test tables */
  /* each resource set has TST_NB_RESOURCES_PER_SET resources */
  for (i = 0; i < TST_NB_RESOURCE_SET; i++) {

    p_resource_set = calloc( 1, sizeof(PUCCH_ResourceSet_t));

    if (p_resource_set == NULL) {
     printf("Error of memory allocation : at line %d in function %s of file %s \n", LINE_FILE , __func__, __FILE__);
     assert(0);
    }
    else {
      //printf("Allocation %p \n", p_resource_set);
      p_resource_set->pucch_ResourceSetId = i;
      p_resource_set->maxPayloadMinus1 = tst_max_payload[i];

      /* create  resources for format 0 to 4 */
        for (j = 0; j < TST_NB_RESOURCES_PER_SET; j++) {

         p_resource_set->pucch_resource_id[j] = counter_resource_id;

         p_resource = calloc( 1, sizeof(PUCCH_Resource_t));

         if (p_resource == NULL) {
          //printf("Error of memory allocation : at line %d in function %s of file %s \n", LINE_FILE , __func__, __FILE__);
          assert(0);
         }
         else {
           //printf("Allocation %p \n", p_resource);
           p_resource->format_parameters.format = tst_pucch_format[i][j];
           p_resource->pucch_ResourceId = counter_resource_id;
           p_resource->startingPRB = i+1;
           p_resource->intraSlotFrequencyHopping = 0;
           p_resource->secondHopPRB = 0;
           p_resource->format_parameters.initialCyclicShift = i+2;
           p_resource->format_parameters.timeDomainOCC = 0;
           p_resource->format_parameters.nrofPRBs = j + i;
           p_resource->format_parameters.occ_length = 0;
           p_resource->format_parameters.occ_Index = 0;

           switch(p_resource->format_parameters.format) {
             case 0:
               p_resource->format_parameters.nrofSymbols = 1;
               break;
             case 1:
               p_resource->format_parameters.timeDomainOCC = 10+j;
               p_resource->format_parameters.nrofSymbols = 4+j;
               p_resource->format_parameters.timeDomainOCC = j%6;
               break;
             case 2:
               p_resource->format_parameters.nrofSymbols = 2;
               break;
             case 3:
               p_resource->format_parameters.nrofSymbols = 4+j;
               break;
             case 4:
               p_resource->format_parameters.nrofSymbols = 4+j;
               p_resource->format_parameters.occ_length = 10+j;
//               p_resource->format_parameters.occ_Index = 11+j;
               p_resource->format_parameters.occ_Index = 0;
               break;
             default:
               LOG_E(PHY,"Unknown format : at line %d in function %s of file %s \n", LINE_FILE , __func__, __FILE__);
           }
           p_resource->format_parameters.startingSymbolIndex = i;
         }

         ue->pucch_config_dedicated_nr[gNB_id].PUCCH_Resource[counter_resource_id] = p_resource;
         counter_resource_id++; /* next resource id */
        }
      }
      if (i == 0) { /* this is the first resource set */
        p_resource_set->first_resources_set_R_PUCCH = j;
      }
      for (; j < MAX_NB_OF_PUCCH_RESOURCES_PER_SET; j++) {
         p_resource_set->pucch_resource_id[j] = MAX_NB_OF_PUCCH_RESOURCES;
       ue->pucch_config_dedicated_nr[gNB_id].PUCCH_ResourceSet[i] = p_resource_set;
    }
  }
}

/*******************************************************************
*
* NAME :         relase_pucch_dedicated_configuration
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  release of PUCCH dedicated configuration
*
*********************************************************************/

void release_pucch_dedicated_configuration(PHY_VARS_NR_UE *ue, uint8_t gNB_id)
{
  int i;

  printf("\nRelease default pucch dedicated configuration \n");

  /* release of resources set */
  for (i = 0; i < MAX_NB_OF_PUCCH_RESOURCE_SETS; i++) {
    if (ue->pucch_config_dedicated_nr[gNB_id].PUCCH_ResourceSet[i] != NULL) {
      //printf("Free %p \n", ue->pucch_config_dedicated_nr[gNB_id].PUCCH_ResourceSet[i]);
      free(ue->pucch_config_dedicated_nr[gNB_id].PUCCH_ResourceSet[i]);
    }
  }

  /* release of resources */
  for (i = 0; i < MAX_NB_OF_PUCCH_RESOURCES; i++) {
    if (ue->pucch_config_dedicated_nr[gNB_id].PUCCH_Resource[i] != NULL) {
      //printf("Free %p \n", ue->pucch_config_dedicated_nr[gNB_id].PUCCH_Resource[i]);
      free(ue->pucch_config_dedicated_nr[gNB_id].PUCCH_Resource[i]);
    }
  }

  /* release of format */
  for (i = 0; i < NUMBER_PUCCH_FORMAT_NR-1; i++) {
    if (ue->pucch_config_dedicated_nr[gNB_id].formatConfig[i] != NULL) {
      //printf("Free %p \n", ue->pucch_config_dedicated_nr[gNB_id].formatConfig[i]);
      free(ue->pucch_config_dedicated_nr[gNB_id].formatConfig[i]);
    }
  }
}

/*******************************************************************
*
* NAME :         test_pucch_dedicated_single_transport_block
*
* PARAMETERS :   ue context
*                gNB identity
*                current rx and tx time frame
*
* RETURN :       none
*
* DESCRIPTION :  test of PUCCH from dedicated configuration
*                get DAI values from downlink harq and the generates
*                acknownledgment with a PUCCH
*
*********************************************************************/

int test_pucch_dedicated_single_transport_block(PHY_VARS_NR_UE *ue, int gNB_id, UE_nr_rxtx_proc_t *proc)
{
#define   TST_NB_STEP_SINGLE_TRANSPORT_BLOCK      (4)

  int v_return = 0;
  int reset_harq = false;
  int dl_harq_pid[TST_NB_STEP_SINGLE_TRANSPORT_BLOCK] = {TST_DL_HARQ_PID_FIRST, TST_DL_HARQ_PID_SECOND, TST_DL_HARQ_PID_THIRD, TST_DL_HARQ_PID_FOURTH };
  int pucch_resource_indicator[TST_NB_STEP_SINGLE_TRANSPORT_BLOCK][2] = { { 0, 4 }, { 1, 0 } , { 1, 3 } , { 5, 7 } };
  NR_UE_HARQ_STATUS_t *harq_status;

  ue->PDSCH_Config.maxNrofCodeWordsScheduledByDCI = nb_code_n1;

  printf("%s", tst_separator);

  ue->dlsch[proc->thread_id][gNB_id][0]->current_harq_pid = TST_DL_HARQ_PID_FIRST;

  harq_status = &ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[TST_DL_HARQ_PID_FIRST].harq_ack;

  harq_status->slot_for_feedback_ack  = proc->nr_slot_tx;
  harq_status->send_harq_status = 1;
  harq_status->vDAI_DL = 1;
  harq_status->ack = DL_ACK;
  harq_status->pucch_resource_indicator = MAX_PUCCH_RESOURCE_INDICATOR;

  printf("\n  => Test : PUCCH format from dedicated config : pucch resource indicator is invalid \n");

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != false) {
    v_return = -1;
  }

  for (int i = 0; i < TST_NB_STEP_SINGLE_TRANSPORT_BLOCK; i++) {

    /* set a tx slot with no ack */
    ue->dlsch[proc->thread_id][gNB_id][0]->current_harq_pid = dl_harq_pid[i];

    NR_UE_HARQ_STATUS_t *harq_status = &ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[dl_harq_pid[i]].harq_ack;

    harq_status->slot_for_feedback_ack  = proc->nr_slot_tx;
    harq_status->send_harq_status = 1;
    harq_status->vDAI_DL = i+1;
    harq_status->ack = DL_ACK;

    for (int j = 0 ; j < 2; j++) {

      /* reset ack context on last test */
      if ((i == (TST_NB_STEP_SINGLE_TRANSPORT_BLOCK-1)) && (j == 1)) {
        reset_harq = true;
      }

      harq_status->pucch_resource_indicator = pucch_resource_indicator[i][j];

      printf("\n  => Test : PUCCH format from dedicated config with 1 transport block : with %d downlink ACKnowledgments and pucch resource indicator %d \n", i+1, pucch_resource_indicator[i][j]);

      if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
        v_return = -1;
      }
    }
  }

  /* Test with missed PDSCH */
  /* some transport blocks have been missed and they need to be nack by UE in order to be retransmitted by the network */

  int i;

  for (i = 1; i < TST_NB_STEP_SINGLE_TRANSPORT_BLOCK; i = i+2) {

    /* set a tx slot with no ack */
    ue->dlsch[proc->thread_id][gNB_id][0]->current_harq_pid = dl_harq_pid[i];

    NR_UE_HARQ_STATUS_t *harq_status = &ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[dl_harq_pid[i]].harq_ack;

    harq_status->slot_for_feedback_ack  = proc->nr_slot_tx;
    harq_status->send_harq_status = 1;
    harq_status->vDAI_DL = i+1;
    harq_status->ack = DL_ACK;
    harq_status->pucch_resource_indicator = pucch_resource_indicator[i][0];
  }

  reset_harq = true;

  printf("\n  => Test : PUCCH format from dedicated config with 1 transport block and missed PDSCH : with %d downlink ACKnowledgments and pucch resource indicator %d \n", i+1, pucch_resource_indicator[3][0]);

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    v_return = -1;
  }

  return (v_return);
}

/*******************************************************************
*
* NAME :         test_pucch_dedicated_config_two_transport_blocks
*
* PARAMETERS :   ue context
*                gNB identity
*                current rx and tx time frame
*
* RETURN :       none
*
* DESCRIPTION :  test of PUCCH from dedicated configuration
*                get DAI values from downlink harq and the generates
*                acknownledgment with a PUCCH
*
*********************************************************************/

int test_pucch_dedicated_two_transport_blocks(PHY_VARS_NR_UE *ue, int gNB_id, UE_nr_rxtx_proc_t *proc)
{
#define   TST_NB_STEP_TWO_TRANSPORT_BLOCKS      (4)

  int v_return = 0;
  int reset_harq = false;
  int dl_harq_pid[TST_NB_STEP_TWO_TRANSPORT_BLOCKS] = {TST_DL_HARQ_PID_FIRST, TST_DL_HARQ_PID_SECOND, TST_DL_HARQ_PID_THIRD, TST_DL_HARQ_PID_FOURTH };
  int pucch_resource_indicator[TST_NB_STEP_TWO_TRANSPORT_BLOCKS][2] = { { 0, 1 }, { 3, 7 } , { 2 , 4 } , { 4 , 6 } };
  NR_UE_HARQ_STATUS_t *harq_status;
  int TB_identifier = 1;
  int thread_number = TST_THREAD_ID;

  ue->PDSCH_Config.maxNrofCodeWordsScheduledByDCI = nb_code_n2;

  printf("%s", tst_separator);

  /* create context for second transport block */
  config_downlink_harq_process(ue , gNB_id, TB_identifier, thread_number, NR_DEFAULT_DLSCH_HARQ_PROCESSES);

  for (int i = 0; i < TST_NB_STEP_TWO_TRANSPORT_BLOCKS; i++) {

    for (int code_word = 0; code_word < 2; code_word++) {

      ue->dlsch[proc->thread_id][gNB_id][code_word]->Number_MCS_HARQ_DL_DCI = 2;

      /* set a tx slot with no ack */
      ue->dlsch[proc->thread_id][gNB_id][code_word]->current_harq_pid = dl_harq_pid[i];

      NR_UE_HARQ_STATUS_t *harq_status = &ue->dlsch[proc->thread_id][gNB_id][code_word]->harq_processes[dl_harq_pid[i]].harq_ack;

      harq_status->slot_for_feedback_ack  = proc->nr_slot_tx;
      harq_status->send_harq_status = 1;
      harq_status->vDAI_DL = i+1;
      harq_status->ack = DL_ACK;
    }

    for (int j = 0 ; j < 2; j++) {

      /* reset ack context on last test */
      if ((i == (TST_NB_STEP_TWO_TRANSPORT_BLOCKS-1)) && (j == 1)) {
        reset_harq = true;
      }

      harq_status = &ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[dl_harq_pid[i]].harq_ack;

      harq_status->pucch_resource_indicator = pucch_resource_indicator[i][j];

      printf("\n  => Test : PUCCH format from dedicated config with 2 transport blocks : with %d downlink ACKnowledgments and pucch resource indicator %d \n", i+1, pucch_resource_indicator[i][j]);

      if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
        printf("Test fail \n");
        v_return = -1;
      }
    }
  }

  /* Test with missed PDSCH */
  /* some transport blocks have been missed and they need to be nack by UE in order to be retransmitted by the network */
  /* here first and third block receptions have been missed */

  reset_harq = true;

  for (int i = 1; i < TST_NB_STEP_TWO_TRANSPORT_BLOCKS; i = i + 1) {

    for (int code_word = 0; code_word < 2; code_word++) {

      ue->dlsch[proc->thread_id][gNB_id][code_word]->Number_MCS_HARQ_DL_DCI = 2;

      /* set a tx slot with no ack */
      ue->dlsch[proc->thread_id][gNB_id][code_word]->current_harq_pid = dl_harq_pid[i];

      NR_UE_HARQ_STATUS_t *harq_status = &ue->dlsch[proc->thread_id][gNB_id][code_word]->harq_processes[dl_harq_pid[i]].harq_ack;

      harq_status->slot_for_feedback_ack  = proc->nr_slot_tx;
      harq_status->send_harq_status = 1;
      harq_status->vDAI_DL = i+1;
      harq_status->ack = DL_ACK;

      for (int j = 0 ; j < 2; j++) {

        harq_status = &ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[dl_harq_pid[i]].harq_ack;

        harq_status->pucch_resource_indicator = pucch_resource_indicator[i][j];
      }
    }
  }

  printf("\n  => Test : PUCCH format from dedicated config with 2 transport blocks and 1 missed PDSCH : with %d downlink ACKnowledgments and pucch resource indicator %d \n", 4, pucch_resource_indicator[3][0]);

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    printf("Test fail \n");
    v_return = -1;
  }

  printf("\n  => Test : PUCCH format from dedicated config with 2 transport blocks and 2 missed PDSCH : with %d downlink ACKnowledgments and pucch resource indicator %d \n", 4, pucch_resource_indicator[3][0]);

  for (int i = 1; i < TST_NB_STEP_TWO_TRANSPORT_BLOCKS; i = i + 2) {

    for (int code_word = 0; code_word < 2; code_word++) {

      ue->dlsch[proc->thread_id][gNB_id][code_word]->Number_MCS_HARQ_DL_DCI = 2;

      /* set a tx slot with no ack */
      ue->dlsch[proc->thread_id][gNB_id][code_word]->current_harq_pid = dl_harq_pid[i];

      NR_UE_HARQ_STATUS_t *harq_status = &ue->dlsch[proc->thread_id][gNB_id][code_word]->harq_processes[dl_harq_pid[i]].harq_ack;

      harq_status->slot_for_feedback_ack  = proc->nr_slot_tx;
      harq_status->send_harq_status = 1;
      harq_status->vDAI_DL = i+1;
      harq_status->ack = DL_ACK;

      for (int j = 0 ; j < 2; j++) {

        harq_status = &ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[dl_harq_pid[i]].harq_ack;

        harq_status->pucch_resource_indicator = pucch_resource_indicator[i][j];
      }
    }
  }

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
   printf("Test fail \n");
   v_return = -1;
  }

  ue->PDSCH_Config.maxNrofCodeWordsScheduledByDCI = nb_code_n1;  /* set to default value */

  return (v_return);
}

/*******************************************************************
*
* NAME :         tst_set_scheduling_request_configuration
*
* PARAMETERS :   resource identity
*                period and offset
*
* RETURN :       none
*
* DESCRIPTION :  set configuration of scheduling request
*
*********************************************************************/

void tst_set_scheduling_request_configuration (PHY_VARS_NR_UE *ue, int gNB_id, int sr_config_index, int resource_id, sr_periodicity_t periodicity, int offset)
{
  SchedulingRequestResourceConfig_t *p_scheduling_request_config = calloc( 1, sizeof(SchedulingRequestResourceConfig_t));

  p_scheduling_request_config->schedulingRequestResourceId = 0;
  p_scheduling_request_config->schedulingRequestID = 0;
  p_scheduling_request_config->resource = resource_id;
  p_scheduling_request_config->periodicity = periodicity;
  p_scheduling_request_config->offset = offset;

  ue->scheduling_request_config_nr[gNB_id].sr_ResourceConfig[sr_config_index] = p_scheduling_request_config;
  ue->scheduling_request_config_nr[gNB_id].active_sr_id = sr_config_index;
}

/*******************************************************************
*
* NAME :         tst_free_scheduling_request_configuration
*
* PARAMETERS :   resource identity
*                period and offset
*
* RETURN :       none
*
* DESCRIPTION :  set configuration of scheduling request
*
*********************************************************************/

void tst_free_scheduling_request_configuration (PHY_VARS_NR_UE *ue, int gNB_id, int sr_config_index)
{
  free(ue->scheduling_request_config_nr[gNB_id].sr_ResourceConfig[sr_config_index]);

  ue->scheduling_request_config_nr[gNB_id].sr_ResourceConfig[sr_config_index] = NULL;
}

/*******************************************************************
*
* NAME :         tst_ue_get_SR
*
* PARAMETERS :   frameP is the sr payload
*
* RETURN :       frameP is the sr payload
*
* DESCRIPTION :  return payload of scheduling request
*
*********************************************************************/

uint32_t tst_ue_get_SR(module_id_t module_idP,int CC_id,frame_t frameP,uint8_t eNB_id,uint16_t rnti, sub_frame_t subframe)
{
  VOID_PARAMETER module_idP;
  VOID_PARAMETER CC_id;
  VOID_PARAMETER frameP;
  VOID_PARAMETER eNB_id;
  VOID_PARAMETER subframe;
  VOID_PARAMETER rnti;
  return (tst_scheduling_request_payload);
}

/*******************************************************************
*
* NAME :         tst_set_sr_config
*
* PARAMETERS :   ue context
*                gNB_id identity
*
* RETURN :       none
*
* DESCRIPTION :  set sr configuration
*
*********************************************************************/

void tst_set_sr_config(PHY_VARS_NR_UE *ue, int gNB_id) {
  ue->scheduling_request_config[gNB_id].sr_ConfigIndex = 0;
  p_nr_ue_get_SR = tst_ue_get_SR;
}

/*******************************************************************
*
* NAME :         test_sr_dedicated
*
* PARAMETERS :   ue context
*                gNB identity
*                current rx and tx time frame
*
* RETURN :       none
*
* DESCRIPTION :  test of PUCCH from dedicated configuration
*                with scheduling request
*
*********************************************************************/

int test_sr_alone_dedicated(PHY_VARS_NR_UE *ue, int gNB_id, UE_nr_rxtx_proc_t *proc)
{
  int v_return = 0;
  int reset_harq = 0;
  int sr_config_id = 0;
  int sr_resource_id = 0;
  int sr_offset = 0;

  printf("%s", tst_separator);

  tst_set_scheduling_request_configuration( ue, gNB_id, sr_config_id, sr_resource_id, sr_sl1, sr_offset);

  tst_set_sr_config( ue, gNB_id);

  printf("\n  => Test : PUCCH format from dedicated config with only a negative scheduling request : no PUCCH should be generated \n");

  tst_scheduling_request_payload = 0;

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != false) {
   printf("Test fail \n");
   v_return = -1;
  }

  printf("\n  => Test : PUCCH format 0 from dedicated config with only a positive scheduling request \n");

  tst_scheduling_request_payload = 1;

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    printf("Test fail \n");
    v_return = -1;
  }

  printf("\n  => Test : PUCCH format 1 from dedicated config with only a positive scheduling request \n");

  sr_resource_id = 1;

  tst_set_scheduling_request_configuration( ue, gNB_id, sr_config_id, sr_resource_id, sr_sl1, sr_offset );

  tst_scheduling_request_payload = 1;

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    printf("Test fail \n");
    v_return = -1;
  }

  /* restore single transport block */
  ue->dlsch[proc->thread_id][gNB_id][0]->Number_MCS_HARQ_DL_DCI = 1;

  return (v_return);
}

/*******************************************************************
*
* NAME :         test_sr_ack_dedicated
*
* PARAMETERS :   ue context
*                gNB identity
*                current rx and tx time frame
*
* RETURN :       none
*
* DESCRIPTION :  test of PUCCH from dedicated configuration
*                with scheduling request and acknowledgment
*
*********************************************************************/

int test_sr_ack_dedicated(PHY_VARS_NR_UE *ue, int gNB_id, UE_nr_rxtx_proc_t *proc)
{
  int v_return = 0;
  bool reset_harq = false;
  int sr_config_id = 0;
  int sr_resource_id = 0;
  int sr_offset = 0;
  int pucch_resource_indicator[5] = { 0, 1, 2, 3, 4 };

  printf("%s", tst_separator);

  tst_set_scheduling_request_configuration( ue, gNB_id, sr_config_id, sr_resource_id, sr_sl1, sr_offset);

  tst_set_sr_config( ue, gNB_id);

  printf("\n  => Test : PUCCH format from dedicated config with a negative scheduling request and a positive ack \n");

  /* set a first harq ack context */

  NR_UE_HARQ_STATUS_t *harq_status;

  ue->dlsch[proc->thread_id][gNB_id][0]->current_harq_pid = TST_DL_HARQ_PID_FIRST;

  harq_status = &ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[TST_DL_HARQ_PID_FIRST].harq_ack;

  harq_status->slot_for_feedback_ack  = proc->nr_slot_tx;
  harq_status->send_harq_status = 1;
  harq_status->vDAI_DL = 1;
  harq_status->ack = DL_ACK;
  harq_status->pucch_resource_indicator = pucch_resource_indicator[0];

  tst_scheduling_request_payload = 0;  /* set sr payload */

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    printf("Test fail \n");
    v_return = -1;
  }

  printf("\n  => Test : PUCCH format from dedicated config with a positive scheduling request and a positive ack \n");

  tst_scheduling_request_payload = 1;  /* set sr payload */

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
   printf("Test fail \n");
   v_return = -1;
  }

  printf("\n  => Test : PUCCH format from dedicated config with a positive scheduling request and a positive ack \n");

  harq_status->pucch_resource_indicator = pucch_resource_indicator[1];

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    printf("Test fail \n");
    v_return = -1;
  }

  printf("\n  => Test : PUCCH format from dedicated config with a positive scheduling request and two positive acks \n");

  /* set a second harq ack context */

  ue->dlsch[proc->thread_id][gNB_id][0]->current_harq_pid = TST_DL_HARQ_PID_SECOND;

  harq_status = &ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[TST_DL_HARQ_PID_SECOND].harq_ack;

  harq_status->slot_for_feedback_ack  = proc->nr_slot_tx;
  harq_status->send_harq_status = 1;
  harq_status->vDAI_DL = 2;
  harq_status->ack = DL_ACK;
  harq_status->pucch_resource_indicator = pucch_resource_indicator[0];

  tst_scheduling_request_payload = 1;  /* set sr payload */

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    printf("Test fail \n");
    v_return = -1;
  }

  printf("\n  => Test : PUCCH format from dedicated config with a positive scheduling request and two positive acks \n");

  harq_status->pucch_resource_indicator = pucch_resource_indicator[1];
  reset_harq = true;

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
   printf("Test fail \n");
   v_return = -1;
  }

  tst_free_scheduling_request_configuration(ue, gNB_id, sr_config_id);

  return (v_return);
}

/*******************************************************************
*
* NAME :         test_csi_dedicated
*
* PARAMETERS :   ue context
*                gNB identity
*                current rx and tx time frame
*
* RETURN :       none
*
* DESCRIPTION :  test of PUCCH from dedicated configuration
*                with channel state information CSI
*
*********************************************************************/

int test_csi_dedicated(PHY_VARS_NR_UE *ue, int gNB_id, UE_nr_rxtx_proc_t *proc)
{
  int v_return = 0;
  int reset_harq = false;
  int sr_config_id = 0;
  int sr_resource_id = 0;
  int sr_offset = 0;
  int pucch_resource_indicator[5] = { 0, 1, 2, 3, 4 };

  printf("%s", tst_separator);

  ue->dlsch[proc->thread_id][gNB_id][0]->current_harq_pid = NR_MAX_DLSCH_HARQ_PROCESSES;

  set_csi_nr( 4, 0x0E );

  printf("\n  => Test : PUCCH format from dedicated config with CSI alone \n");

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    printf("Test fail \n");
    v_return = -1;
  }

  printf("\n  => Test : PUCCH format from dedicated config with ACK and CSI \n");

  /* set a first harq ack context */

  NR_UE_HARQ_STATUS_t *harq_status;

  ue->dlsch[proc->thread_id][gNB_id][0]->current_harq_pid = TST_DL_HARQ_PID_FIRST;

  harq_status = &ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[TST_DL_HARQ_PID_FIRST].harq_ack;

  harq_status->slot_for_feedback_ack  = proc->nr_slot_tx;
  harq_status->send_harq_status = 1;
  harq_status->vDAI_DL = 1;
  harq_status->ack = DL_ACK;
  harq_status->pucch_resource_indicator = pucch_resource_indicator[3];

  ue->pucch_config_dedicated_nr[gNB_id].formatConfig[2-1]->simultaneousHARQ_ACK_CSI = enable_feature; /* format 2 */

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    printf("Test fail \n");
    v_return = -1;
  }

  printf("\n  => Test : PUCCH format from dedicated config with ACK, negative SR and CSI \n");

  tst_set_scheduling_request_configuration( ue, gNB_id, sr_config_id, sr_resource_id, sr_sl1, sr_offset);

  tst_set_sr_config( ue, gNB_id);

  tst_scheduling_request_payload = 0;  /* set sr payload */

  reset_harq = true;

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    printf("Test fail \n");
    v_return = -1;
  }

  printf("\n  => Test : PUCCH format from dedicated config with negative SR and CSI \n");

  harq_status->pucch_resource_indicator = pucch_resource_indicator[4];

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    printf("Test fail \n");
    v_return = -1;
  }

  tst_scheduling_request_payload = 1;

  printf("\n  => Test : PUCCH format from dedicated config with positive SR and CSI \n");

  if (pucch_procedures_ue_nr(ue, gNB_id, proc, reset_harq) != true) {
    printf("Test fail \n");
    v_return = -1;
  }

  return (v_return);
}

/*******************************************************************
*
* NAME :         test_pucch
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  test of PUCCH feature
*
*********************************************************************/

int test_pucch(PHY_VARS_NR_UE *ue)
{
  int gNB_id = TST_GNB_ID_0;
  int thread_number = TST_THREAD_ID;
  int TB_identifier = 0;
  int v_return = 0;

  UE_nr_rxtx_proc_t *proc = calloc( 1, sizeof(UE_nr_rxtx_proc_t));
  if (proc == NULL) {
    printf("Error test_srs: memory allocation problem p_rxtx_proc \n");
    assert(0);
  }

  proc->frame_rx   = 0;
  proc->nr_slot_rx = 0;

  proc->frame_tx   = proc->frame_rx + (proc->nr_slot_rx + NR_UE_CAPABILITY_SLOT_RX_TO_TX) / ue->frame_parms.slots_per_frame;
  proc->nr_slot_tx = (proc->nr_slot_rx + NR_UE_CAPABILITY_SLOT_RX_TO_TX) % ue->frame_parms.slots_per_frame;

  init_pucch_power_configuration( ue, gNB_id);

  /* init and clear downlink harq */
  config_downlink_harq_process(ue , gNB_id, TB_identifier, thread_number, NR_DEFAULT_DLSCH_HARQ_PROCESSES);

  if (test_pucch_basic_error(ue, gNB_id, proc) != 0) {
    v_return = -1;
    return (v_return);
  }

  if (test_pucch_common_config_single_transport_block(ue, gNB_id, proc) != 0) {
    v_return = -1;
    return (v_return);
  }

  init_pucch_dedicated_configuration(ue , gNB_id);

  if (test_pucch_dedicated_single_transport_block(ue, gNB_id, proc) != 0) {
    v_return = -1;
    return (v_return);
  }

  if (test_pucch_dedicated_two_transport_blocks(ue, gNB_id, proc) != 0) {
    v_return = -1;
    return (v_return);
  }

  if (test_sr_alone_dedicated(ue, gNB_id, proc) != 0) {
    v_return = -1;
    return (v_return);
  }

  if (test_sr_ack_dedicated(ue, gNB_id, proc) != 0) {
    v_return = -1;
    return (v_return);
  }

  if (test_csi_dedicated(ue, gNB_id, proc) != 0) {
    v_return = -1;
    return (v_return);
  }

  release_downlink_harq_process(ue, gNB_id, TB_identifier, thread_number);

  release_pucch_dedicated_configuration(ue , gNB_id);

  free(proc);

  return (v_return);
}

/*******************************************************************
*
* NAME :         main
*
* DESCRIPTION :  test of uplink sounding reference signal aka SRS feature
*
*********************************************************************/

int main(int argc, char *argv[])
{
  uint8_t transmission_mode = 1;
  uint8_t nb_antennas_tx = 1;
  uint8_t nb_antennas_rx = 1;
  uint8_t frame_type = FDD;
  int N_RB_DL=100;
  lte_prefix_type_t extended_prefix_flag = NORMAL;
  int Nid_cell[] = {(3*1+3)};
  VOID_PARAMETER argc;
  VOID_PARAMETER argv;

  printf(" PUCCH TEST \n");
  printf("-----------\n");

  if (init_test(nb_antennas_tx, nb_antennas_rx, transmission_mode, extended_prefix_flag, frame_type, Nid_cell[0], N_RB_DL) != 0) {
    printf("Initialisation problem for test \n");
    exit(-1);;
  }

  if (test_pucch(PHY_vars_UE) != 0) {
    printf("\nTest PUCCH is fail \n");
  }
  else {
    printf("\nTest PUCCH is pass \n");
  }

  free_context_synchro_nr();

  return(0);
}
