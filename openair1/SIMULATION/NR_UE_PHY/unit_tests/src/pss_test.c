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
* FILENAME    :  pss_test.c
*
* MODULE      :  UE test bench for synchronisation
*
* DESCRIPTION :  it allows unitary tests of UE synchronisation on host machine
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


/************** DEFINE *******************************************/

/*************** LOCAL VARIABLES***********************************/

/*************** FUNCTIONS ****************************************/

/********************************************************************
*
* NAME :         pss_test
*
* INPUT :        UE context
*                for a pss sequence
*                - position of pss in the received UE buffer
*                - number of pss sequence
*
* RETURN :      0 if OK else error
*
* DESCRIPTION :  detect pss sequences in the received UE buffer
*
*                Test procedure:
*
*                - reference pss sequences is written at a specific position
*                - correlation is done between the received buffer and the possible pss sequences
*                - results indicate pss sequence number and correlation result.
*
*                Test is pass if detection of pss position is the one which has been set.
*
*                                      (10 ms)
*     <--------------------------------------------------------------------------->
*     -----------------------------------------------------------------------------
*     |        Received UE data buffer                                            |
*     ----------------------------------------------------------------------------
*                +-----------+
*     <--------->|   pss     |
*      position  +-----------+
*                ^
*                |
*            peak position
*            given by maximum of correlation result
*            position matches beginning of ofdm symbol
*
*     Remark: memory position should be aligned on a multiple of 4 due to I & Q samples of int16
*             An OFDM symbol is composed of x number of received samples depending of Rf front end sample rate.
*
*     I & Q storage in memory
*
*             First samples       Second  samples
*     ------------------------- -------------------------  ...
*     |     I1     |     Q1    |     I2     |     Q2    |
*     ---------------------------------------------------  ...
*     ^    16  bits   16 bits  ^
*     |                        |
*     ---------------------------------------------------  ...
*     |        sample 1        |       sample 2         |
*    ----------------------------------------------------  ...
*     ^
*     |
*     OFDM in time
*
*********************************************************************/

void test_synchro_pss_nr(PHY_VARS_NR_UE *PHY_VARS_NR_UE, int position_symbol, int pss_sequence_number, test_t *test)
{
  int rate_change = SYNCHRO_RATE_CHANGE_FACTOR;
  int synchro_position;
  int NID2_value;

  display_test_configuration_pss(position_symbol, pss_sequence_number);

  set_sequence_pss(PHY_VARS_NR_UE, position_symbol, pss_sequence_number);

  /* search pss */
  synchro_position = pss_synchro_nr(PHY_VARS_NR_UE, rate_change);
  int pss_sequence = get_softmodem_params()->sl_mode == 0 ? NUMBER_PSS_SEQUENCE : NUMBER_PSS_SEQUENCE_SL;

  if (pss_sequence_number < pss_sequence) {
    NID2_value = pss_sequence_number;
  } else {
    NID2_value = pss_sequence;
  }

  if (NID2_value < pss_sequence) {
    test->number_of_tests++;
    /* position should be adjusted with i&q samples which are successively stored as int16 in the received buffer */
    int test_margin = PSS_DETECTION_MARGIN_MAX; /* warning correlation results give an offset position between 0 and 12 */
    int delta_position = abs(position_symbol - (synchro_position*rate_change));
    printf("%s ", test->test_current);
    if (PHY_VARS_NR_UE->common_vars.eNb_id == pss_sequence_number) {
      if ( delta_position !=  0) {
        if (delta_position > test_margin*rate_change) {
        printf("Test is fail due to wrong position %d instead of %d \n", (synchro_position*rate_change), position_symbol);
        printf("%s ", test->test_current);
        printf("Test is fail : pss detected with a shift of %d \n", delta_position);
        test->number_of_fail++;
        }
        else {
        printf("Test is pass with warning: pss detected with a shift of %d \n", delta_position);
        test->number_of_pass_warning++;
        }
      }
      else {
        printf("Test is pass: pss detected at right position \n");
        test->number_of_pass++;
      }
    }
    else {
      printf("Test is fail due to wrong NID2 detection %d instead of %d \n", PHY_VARS_NR_UE->common_vars.eNb_id, NID2_value);
      test->number_of_fail++;
    }
  }
}

/*******************************************************************
*
* NAME :         main
*
* DESCRIPTION :  test bench for synchronisation sequence (pss/sss/pbch)
*                purpose of test is to check that synchronisation sequence can be
*                properly detected depending of input waveforms.
*
*                - First step of synchronisation is pss sequence.
*                pss sequence should be detected at the right position
*                in the received buffer whatever the position along the
*                received data buffer.
*                Detection of position is based on correlation in time domain between
*                received signal and pss sequence -> position is given by
*                peak of correlation result.
*
*********************************************************************/

int main(int argc, char *argv[])
{
  VOID_PARAMETER argc;
  VOID_PARAMETER argv;
  uint8_t transmission_mode = 1;
  uint8_t nb_antennas_tx = 1;
  uint8_t nb_antennas_rx = 1;
  uint16_t Nid_cell=123;
  uint8_t frame_type=FDD;
  int N_RB_DL=100;
  lte_prefix_type_t extended_prefix_flag=NORMAL;
  const char *name_test = "PSS NR";
  test_t test = { name_test, 0, 0, 0, 0};
  /* this is a pointer to the function in charge of the test */
  void (*p_test_synchro_pss)(PHY_VARS_NR_UE *PHY_VARS_NR_UE, int position_symbol, int pss_sequence_number, test_t *test) = test_synchro_pss_nr;

  int test_position[] = { 0, 492, 493, 56788, 56888, 111111, 151234, 151500, 200000, 250004, (307200-2048) };
  int size_test_position = sizeof(test_position)/sizeof(int);

  printf(" PSS TEST \n");
  printf("----------\n");

  if (init_test(nb_antennas_tx, nb_antennas_rx, transmission_mode, extended_prefix_flag, frame_type, Nid_cell, N_RB_DL) != 0) {
    printf("Initialisation problem for synchronisation test \n");
    exit(-1);;
  }

  printf("***********************************\n");
  printf("    %s Test synchronisation \n", test.test_current);
  printf("***********************************\n");

#if 0

  printf("Synchronisation with random data\n");

  test_synchro_pss_nr(PHY_vars_UE, 0, INVALID_PSS_SEQUENCE, &test);

#endif
  int pss_sequence = get_softmodem_params()->sl_mode == 0 ? NUMBER_PSS_SEQUENCE : NUMBER_PSS_SEQUENCE_SL;

  for (int index_position = 0; index_position < size_test_position; index_position++) {
    for (int number_pss_sequence = 0; number_pss_sequence < pss_sequence; number_pss_sequence++) {

      p_test_synchro_pss(PHY_vars_UE, test_position[index_position], number_pss_sequence, &test);

      //break;
    }
    //break;
  }

  printf("\n%s Number of tests : %d  Pass : %d Pass with warning : %d Fail : %d \n\n", test.test_current, test.number_of_tests, test.number_of_pass, test.number_of_pass_warning, test.number_of_fail);

  printf("%s Synchronisaton test is terminated.\n\n", test.test_current);

  /* reset test statistics */
  test.number_of_tests = test.number_of_pass = test.number_of_pass_warning = test.number_of_fail = 0;

  free_context_synchro_nr();

  return(0);
}
