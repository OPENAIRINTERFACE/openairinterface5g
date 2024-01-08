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
* FILENAME    :  harq_test.c
*
* MODULE      :  UE test bench for hard (hybrid repeat request acknowledgment)
*
* DESCRIPTION :  it allows unitary tests for downlink harq
*
************************************************************************/

#include "../../unit_tests/src/pss_util_test.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/INIT/init_extern.h"
#include "PHY/phy_extern_nr_ue.h"

#include "SCHED_NR_UE/harq_nr.h"

//#include "PHY/vars.h"
//#include "LAYER2/MAC/vars.h"

/*******************************************************************
*
* NAME :         test_harq_downlink
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  test of HARQ downlink feature
*
*********************************************************************/

int test_harq_downlink(PHY_VARS_NR_UE *phy_vars_ue)
{
  int gNB_id = 0;
  int harq_pid = 0;
  int ndi = 1;
  uint8_t rnti_type = TYPE_C_RNTI_;
  int number_steps = 5;
  int thread_number = 0;
  int TB_identifier = 0;

  printf("\nHARQ Downlink \n");

  config_downlink_harq_process(phy_vars_ue , gNB_id, TB_identifier, thread_number, NR_DEFAULT_DLSCH_HARQ_PROCESSES);

  NR_DL_UE_HARQ_t *dlsch_harq = phy_vars_ue->dlsch[thread_number][gNB_id][TB_identifier]->harq_processes[harq_pid];

  /* reach maximum number of retransmissions */
  printf("First sequence ndi does not toggle \n");
  for (int i = 0 ; i < number_steps; i++) {
    downlink_harq_process(dlsch_harq, harq_pid, ndi, rnti_type);
  }

  harq_pid++; /* next harq */
  dlsch_harq = phy_vars_ue->dlsch[thread_number][gNB_id][TB_identifier]->harq_processes[harq_pid];

  /* new grant so new transmission each try */
  printf("Second sequence ndi toggle each transmission \n");
  for (int i = 0 ; i < number_steps; i++) {
    downlink_harq_process(dlsch_harq, harq_pid, ndi, rnti_type);
    ndi ^= 1; /* toogle ndi each try */
  }

  harq_pid++; /* next harq */
  dlsch_harq = phy_vars_ue->dlsch[thread_number][gNB_id][TB_identifier]->harq_processes[harq_pid];

  /* new grant so new transmission each try */
  printf("Third sequence ndi toggle each two transmissions \n");
  for (int i = 0 ; i < number_steps; i++) {
    downlink_harq_process(dlsch_harq, harq_pid, ndi, rnti_type);
    if (i & 0x1) {
      ndi ^= 1; /* toogle ndi each try */
    }
  }

  release_downlink_harq_process(phy_vars_ue , gNB_id, TB_identifier, thread_number);

  return 0;
}

/*******************************************************************
*
* NAME :         test_harq
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  test of HARQ feature
*
*********************************************************************/

int test_harq(PHY_VARS_NR_UE *phy_vars_ue)
{
  int result = -1;

  result = test_harq_downlink(phy_vars_ue);

  return (result);
}

/*******************************************************************
*
* NAME :         main
*
* DESCRIPTION :  test of downlink harq entities
*
*********************************************************************/

int main(int argc, char *argv[])
{
  VOID_PARAMETER argc;
  VOID_PARAMETER argv;
  uint8_t transmission_mode = 1;
  uint8_t nb_antennas_tx = 1;
  uint8_t nb_antennas_rx = 1;
  uint8_t frame_type = FDD;
  int N_RB_DL=100;
  lte_prefix_type_t extended_prefix_flag = NORMAL;
  int Nid_cell[] = {(3*0+0)};
  VOID_PARAMETER argc;
  VOID_PARAMETER argv;

  printf(" HARQ TEST \n");
  printf("----------\n");

  if (init_test(nb_antennas_tx, nb_antennas_rx, transmission_mode, extended_prefix_flag, frame_type, Nid_cell[0], N_RB_DL) != 0) {
    printf("Initialisation problem for harq test \n");
    exit(-1);;
  }

  if (test_harq(PHY_vars_UE) != 0) {
    printf("Test NR HARQ is fail \n");
  }
  else {
    printf("Test NR HARQ is pass \n");
  }

  free_context_synchro_nr();

  return(0);
}
