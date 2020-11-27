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
* FILENAME    :  sss_test.c
*
* MODULE      :  UE test bench for sss tests
*
* DESCRIPTION :  it allows unitary tests for SSS on host machine
*
************************************************************************/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "../../unit_tests/src/pss_util_test.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/INIT/init_extern.h"
#include "PHY/phy_extern_nr_ue.h"

#include "PHY/NR_REFSIG/ss_pbch_nr.h"
#include "PHY/NR_REFSIG/pss_nr.h"
#include "PHY/NR_REFSIG/sss_nr.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"


/*******************************************************************
*
* NAME :         test_synchro_pss_sss_nr
*
* INPUT :        UE context
*                for first and second pss sequence
*                - position of pss in the received UE buffer
*                - number of pss sequence
*
* RETURN :      0 if OK else error
*
* DESCRIPTION :  detect pss sequences in the received UE buffer
*                then detect sss sequence
*
*                Test procedure:
*                - reference pss sequences (first, second or both) are written at specific
*                positions
*                - correlation is done between the received buffer and the possible pss sequences
*                - results indicate pss sequence number and correlation result
*                - reference sss sequences are written at specific positions
*                - correlation is done with possible sss sequence
*
*                                      (10 ms)
*     <--------------------------------------------------------------------------->
*     -----------------------------------------------------------------------------
*     |        Received UE data buffer                                            |
*     ----------------------------------------------------------------------------
*                     ---------------------------------------------
*     <-------------->|   pss     |  pbch    |    sss    |  pbch   |
*                     ---------------------------------------------
*          offset
*
* SSS/PBCH block contains following symbols: pss + pbch + sss + pbch
*
*********************************************************************/

int test_synchro_pss_sss_nr(PHY_VARS_NR_UE *PHY_vars_UE, int position_symbol, NR_DL_FRAME_PARMS *frame_parms_gNB, test_t *test)
{
  UE_nr_rxtx_proc_t proc = {0};
  VOID_PARAMETER test;
  int synchro_position;
  int offset;
  int16_t *tmp;
  int32_t metric_fdd_ncp=0;
  uint8_t phase_fdd_ncp;
  int rate_change = SYNCHRO_RATE_CHANGE_FACTOR;
  int Nid2 = GET_NID2(frame_parms_gNB->Nid_cell);
  int Nid1 = GET_NID1(frame_parms_gNB->Nid_cell);

  display_test_configuration_pss(position_symbol, Nid2);

  set_sequence_pss(PHY_vars_UE, position_symbol, Nid2);

  synchro_position = pss_synchro_nr(PHY_vars_UE, rate_change);

  display_test_configuration_sss(Nid1);

  synchro_position = synchro_position * rate_change;

  if (abs(synchro_position - position_symbol) > PSS_DETECTION_MARGIN_MAX)
  {
    printf("NR Pss has been detected at position %d instead of %d \n", synchro_position, position_symbol);
  }

  /* symbols order for synchronisation block is   PSS - PBCH - SSS */
  /* knowing that synchronisation position gives pss ofdm start one can deduce start of sss symbol */
  offset = (position_symbol + (SSS_SYMBOL_NB - PSS_SYMBOL_NB)*(PHY_vars_UE->frame_parms.ofdm_symbol_size + PHY_vars_UE->frame_parms.nb_prefix_samples));

  tmp = (int16_t *)&PHY_vars_UE->common_vars.rxdata[0][offset];

  insert_sss_nr(tmp, frame_parms_gNB);

  PHY_vars_UE->rx_offset = position_symbol; /* offset is given for samples of type int16 */

#if 0

  int samples_for_frame = PHY_vars_UE->frame_parms.samples_per_frame;

  write_output("rxdata0_rand.m","rxd0_rand", &PHY_vars_UE->common_vars.rxdata[0][0], samples_for_frame, 1, 1);

#endif

#if TEST_SYNCHRO_TIMING_PSS

  start_meas(&generic_time[TIME_SSS]);

#endif

  rx_sss_nr(PHY_vars_UE, &proc, &metric_fdd_ncp, &phase_fdd_ncp);

#if TEST_SYNCHRO_TIMING_PSS

  stop_meas(&generic_time[TIME_SSS]);

  #ifndef NR_UNIT_TEST

    printf("PSS execution duration %5.2f \n", generic_time[TIME_PSS].p_time/(cpuf*1000.0));

    printf("SSS execution duration %5.2f \n", generic_time[TIME_SSS].p_time/(cpuf*1000.0));

  #endif

#endif

  return phase_fdd_ncp;
}

/*******************************************************************
*
* NAME :         main
*
* DESCRIPTION :  test bench for synchronisation sequence (pss/sss/pbch)
*                purpose of test is to check that synchronisation sequence can be
*                properly detected depending of input waveforms.
*
*                - First step of synchronisation consists in pss search (primary synchronisation signal).
*                pss sequence should be detected at the right position
*                in the received buffer whatever the position along the
*                received data buffer.
*                Detection of position is based on correlation in time domain between
*                received signal and pss sequence -> position is given by
*                peak of correlation result.
*                - Second step of synchronisation is sss decoding (secundary synchronisation signal).
*                sss detection is based on position of detected pss.
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
  uint8_t N_RB_DL=100;
  lte_prefix_type_t extended_prefix_flag = NORMAL;
  int phase;
  int Nid1, Nid2;
  NR_DL_FRAME_PARMS frame_parms_gNB;

  const char *name_test = "SSS NR";
  test_t test = { name_test, 0, 0, 0, 0};

#if 1
  int test_position[] = { 0, 492, 493, 56788, 111111, 222222 }; /* this is the sample number in the frame */
#else
  int test_position[] = {-1};   /* to get samples from an array */
#endif

  int size_test_position = sizeof(test_position)/sizeof(int);

  /* this is a pointer to the function in charge of the test */
  int (*p_test_synchro_pss_sss)(PHY_VARS_NR_UE *PHY_vars_UE, int position_symbol, NR_DL_FRAME_PARMS *frame_parms, test_t *test) = test_synchro_pss_sss_nr;

#if 1
  int Nid_cell[] = {  (3*0+0), (3*71+0), (3*21+2), (3*21+2), (3*55+1), (3*111+2) };
#else
  int Nid_cell[] = { ((3*1+2)) };
#endif

  printf(" SSS TEST \n");
  printf("----------\n");

  if (init_test(nb_antennas_tx, nb_antennas_rx, transmission_mode, extended_prefix_flag, frame_type, Nid_cell[0], N_RB_DL) != 0) {
    printf("Initialisation problem for synchronisation test \n");
    exit(-1);;
  }

  memcpy((void *)&frame_parms_gNB, (void*)&PHY_vars_UE->frame_parms, sizeof(NR_DL_FRAME_PARMS));

  printf("***********************************\n");
  printf("    %s Test synchronisation \n", test.test_current);
  printf("***********************************\n");

  for (unsigned int index = 0; index < (sizeof(Nid_cell)/sizeof(int)); index++) {

    frame_parms_gNB.Nid_cell = Nid_cell[index];

    Nid2 = GET_NID2(Nid_cell[index]);
    Nid1 = GET_NID1(Nid_cell[index]);

    for (int position = 0; position < size_test_position; position++) {

      PHY_vars_UE->frame_parms.Nid_cell = (3 * N_ID_1_NUMBER) + N_ID_2_NUMBER; /* set to invalid value */

      phase = (*p_test_synchro_pss_sss)(PHY_vars_UE, test_position[position], &frame_parms_gNB, &test); /* return phase index which gives phase error from an array */

      test.number_of_tests++;
      printf("%s ", test.test_current);

      if (PHY_vars_UE->frame_parms.Nid_cell == (3 * Nid1) + Nid2) {
        if (phase != INDEX_NO_PHASE_DIFFERENCE) {
          printf("Test is pass with warning due to phase difference %d (instead of %d) offset %d Nid1 %d Nid2 %d \n",
                                                             phase, INDEX_NO_PHASE_DIFFERENCE, test_position[position], Nid1, Nid2);
          test.number_of_pass_warning++;
        } else {
          printf("Test is pass with offset %d Nid1 %d Nid2 %d \n", test_position[position], Nid1, Nid2);
          test.number_of_pass++;
        }
      }
      else {
        printf("Test is fail with offset %d Nid1 %d Nid2 %d \n", test_position[position], Nid1, Nid2);
        test.number_of_fail++;
      }
      //break;
    }
    //break;
  }

  printf("\n%s Number of tests : %d  Pass : %d Pass with warning : %d Fail : %d \n", test.test_current, test.number_of_tests, test.number_of_pass, test.number_of_pass_warning, test.number_of_fail);

  printf("%s Synchronisaton test is terminated.\n\n", test.test_current);

  /* reset test statistics */
  test.number_of_tests = test.number_of_pass = test.number_of_pass_warning = test.number_of_fail = 0;

  free_context_synchro_nr();

  return(0);
}
