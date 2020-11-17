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
* FILENAME    :  pbch_test.c
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

typedef int nfapi_nr_pfcch_commonSearchSpaces_t;
#include "../nfapi/open-nFAPI/nfapi/public_inc/nfapi_nr_interface.h"

#include "PHY/defs_nr_UE.h"
#include "PHY/phy_extern_nr_ue.h"
#include "PHY/INIT/init_extern.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"

#include "PHY/NR_REFSIG/ss_pbch_nr.h"
#include "PHY/NR_REFSIG/pss_nr.h"
#include "PHY/NR_REFSIG/sss_nr.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"

#include "../../unit_tests/src/pss_util_test.h"

/************** EXTERNAL ******************************************/

extern int pbch_detection(PHY_VARS_NR_UE *ue, runmode_t mode);

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

int test_synchro_pss_sss_nr(PHY_VARS_NR_UE *PHY_vars_UE, int position_symbol, int pss_sequence_number)
{
  NR_DL_FRAME_PARMS *frame_parms = &(PHY_vars_UE->frame_parms);
  UE_nr_rxtx_proc_t proc = {0};
  int synchro_position;
  int offset;
  int16_t *tmp;
  int32_t metric_fdd_ncp=0;
  uint8_t phase_fdd_ncp;
  int rate_change = SYNCHRO_RATE_CHANGE_FACTOR;
  int decoded_pbch = -1;

  set_sequence_pss(PHY_vars_UE, position_symbol, pss_sequence_number);

  synchro_position = pss_synchro_nr(PHY_vars_UE, rate_change);

  synchro_position = synchro_position * rate_change;

  if (abs(synchro_position - position_symbol) > PSS_DETECTION_MARGIN_MAX)
  {
    printf("NR Pss has been detected at position %d instead of %d \n", synchro_position, position_symbol);
  }

  /* symbols order for synchronisation block is   PSS - PBCH - SSS */
  /* knowing that synchronisation position gives pss ofdm start one can deduce start of sss symbol */
  offset = (position_symbol + (SSS_SYMBOL_NB - PSS_SYMBOL_NB)*(frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples));

  tmp = (int16_t *)&PHY_vars_UE->common_vars.rxdata[0][offset];

  insert_sss_nr(tmp, frame_parms);

  //PHY_vars_UE->rx_offset = position_symbol; /* offset is given for samples of type int16 */
  PHY_vars_UE->rx_offset = synchro_position; /* offset is given for samples of type int16 */

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

  if (phase_fdd_ncp != INDEX_NO_PHASE_DIFFERENCE) {
    printf("Error: detected phase on sss is not correct: expected index phase is %d, detected index phase is %d !\n", INDEX_NO_PHASE_DIFFERENCE, phase_fdd_ncp);
    return -1;
  }

  generate_dmrs_pbch(PHY_vars_UE->dmrs_pbch_bitmap_nr, frame_parms->Nid_cell);

  decoded_pbch = pbch_detection(PHY_vars_UE, PHY_vars_UE->mode);

  return decoded_pbch;
}

/*******************************************************************
*
* NAME :         set_sequence_sss
*
* PARAMETERS :   UE context
*                position of sss in the received UE buffer
*
* RETURN :       none
*
* DESCRIPTION :  write reference sss sequence at a specified sample position in the received UE buffer
*
*********************************************************************/
void set_sequence_sss(PHY_VARS_NR_UE *PHY_vars_UE, int offset, int slot_offset)
{
  NR_DL_FRAME_PARMS *frame_parms = &(PHY_vars_UE->frame_parms);
  int length = frame_parms->ofdm_symbol_size;
  int size = length * IQ_SIZE; /* i & q */
  nfapi_config_request_t nfapi_config;
  int16_t data_sss[LENGTH_SSS_NR];

  nfapi_config.sch_config.physical_cell_id.value = PHY_vars_UE->frame_parms.Nid_cell;

  /* reinitialise random for always getting same data */
  srand(0);
  int random;
  for (int i=0; i< size/2; i++) {
    random = ((rand() % AMP) - AMP/2);        /* AMP allows to get a scaling of samples */
    synchroF_tmp[i] = random;
  }

  bzero(synchro_tmp, size);

  /* Computation of below offset has been added due to function generate_sss which is used for eNodeB to process transmission */
  /* so it is based on a transmitted buffer which has been dimensioned for a full frame */
  /* here one uses a smaller buffer according to the size of an ofdm symbol so it can not properly work in this case. */
  /* so in order to only write in this buffer, pointer of the buffer has been modified in such a way that function */
  /* generate_sss writes correct data at the right buffer location */
  /* this address offset is applied for SSS which is in the second part of the frame */
  int Nsymb = (frame_parms->Ncp==NORMAL)?14:12;
  int index_offset = slot_offset*Nsymb/2*frame_parms->ofdm_symbol_size;
  int16_t *tmp = synchroF_tmp - (2*index_offset);

#if 0

  nr_generate_sss(data_sss,
		          (int32_t **)&tmp,
                  AMP,
                  0,                        // symbol number
				  &nfapi_config,
				  frame_parms);

  /* get sss in the frequency domain by applying an inverse FFT */
  idft2048(synchroF_tmp,        /* complex input */
           synchro_tmp,         /* complex output */
           1);               /* scaling factor */

  for (int aa=0;aa<PHY_vars_UE->frame_parms.nb_antennas_rx;aa++) {

    /* copy sss in rx buffer */
    for (int i=0; i<length; i++) {
      ((int32_t*)PHY_vars_UE->common_vars.rxdata[aa])[i + offset] = ((int32_t *)synchro_tmp)[i];
    }
  }
#else

 insert_sss_nr(tmp, frame_parms);

#endif
}

/*******************************************************************
*
* NAME :         insert_sequence_sss
*
* PARAMETERS :   UE context
*                position of sss in the received UE buffer
*
* RETURN :       none
*
* DESCRIPTION :  write reference sss sequence at a specified sample position in the received UE buffer
*
*********************************************************************/

void insert_sequence_sss(PHY_VARS_NR_UE *PHY_vars_UE, int offset)
{
  NR_DL_FRAME_PARMS *frame_parms = &(PHY_vars_UE->frame_parms);
  int samples_for_half_frame = frame_parms->samples_per_frame / 2;

  set_sequence_sss(PHY_vars_UE, offset, 0);

  set_sequence_sss(PHY_vars_UE, offset + samples_for_half_frame, 10);
}

/********************************************************************
*
* NAME :         test_synchro_pss_sss
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
*
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
*                     -------------                       -------------
*     <-------------->| first pss |<--------------------->| second pss |
*      position       -------------        (5 ms)         -------------
*          ----------                          ----------
*         | first sss|                        |second sss|
*         ------------                        -----------
* As specified by the standard:
*  - first and second sss sequence are different -> allow to get frame slot timing
*  - first sss sequence is located in slot 0 and second sss sequence is in slot 10
*  - ofdm symbol of sss is just before ofdm symbol of pss
*
*********************************************************************/

int test_synchro_pss_sss(PHY_VARS_NR_UE *PHY_vars_UE, int position_symbol, int sequence_number, test_t *test)
{
  VOID_PARAMETER test;
  NR_DL_FRAME_PARMS *frame_parms = &(PHY_vars_UE->frame_parms);
  int synchro_position;
  int offset;
  int32_t metric_fdd_ncp=0;
  uint8_t flip_fdd_ncp;
  uint8_t phase_fdd_ncp;
  int rate_change = SYNCHRO_RATE_CHANGE_FACTOR;  /* by default there is no sampling rate change */
  int decoded_pbch = -1;

  offset = position_symbol - frame_parms->ofdm_symbol_size - frame_parms->nb_prefix_samples;

  if (offset < 0)
  {
    printf("Test aborted because sss position is outside the window\n");
    exit(0);
  }

  set_sequence_pss(PHY_vars_UE, position_symbol, sequence_number);

  synchro_position = pss_synchro_nr(PHY_vars_UE, rate_change);

  printf("Pss has been detected at position %d with Nid2 %d \n", synchro_position, PHY_vars_UE->common_vars.eNb_id );

  insert_sequence_sss(PHY_vars_UE, offset);

  if ((frame_parms->Ncp != NORMAL) || (frame_parms->frame_type != FDD)) {
    printf("This configuration is not supported. Please use call to function ""initial_sync"" for covering others configurations.\n");
    exit(0);
  }

  int sync_pos2 = synchro_position - frame_parms->nb_prefix_samples;

  int sync_pos_slot = (frame_parms->samples_per_slot>>1) - frame_parms->ofdm_symbol_size - frame_parms->nb_prefix_samples;

  if (sync_pos2 >= sync_pos_slot)
    PHY_vars_UE->rx_offset = sync_pos2 - sync_pos_slot;
  else
    PHY_vars_UE->rx_offset = FRAME_LENGTH_COMPLEX_SAMPLES + sync_pos2 - sync_pos_slot;

  rx_sss(PHY_vars_UE, &metric_fdd_ncp, &flip_fdd_ncp, &phase_fdd_ncp);

  if (phase_fdd_ncp != INDEX_NO_PHASE_DIFFERENCE) {
    printf("Error: detected phase on sss is not correct: expected index phase is %d, detected index phase is %d !\n", INDEX_NO_PHASE_DIFFERENCE, phase_fdd_ncp);
    return -1;
  }

  init_frame_parms(&PHY_vars_UE->frame_parms,1);

  //lte_gold_new(frame_parms, PHY_vars_UE->lte_gold_table[0],frame_parms->Nid_cell);

  decoded_pbch = pbch_detection(PHY_vars_UE, PHY_vars_UE->mode);

  return decoded_pbch;
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
  int decoded_pbch = -1;
  int Nid1, Nid2;
  const char *name_test = " PBCH NR ";
  test_t test = { name_test, 0, 0, 0, 0};

#if 1
  int test_position[] = { 20000 }; /* this is the sample number in the frame */
#else
  int test_position[] = { -1 };    /* to get samples from a buffer initialised from an external file */
#endif

  int size_test_position;

  /* this is a pointer to the function in charge of the test */
  int (*p_test_synchro_pss_sss)(PHY_VARS_NR_UE *PHY_vars_UE, int position_symbol, int sequence_number) = test_synchro_pss_sss_nr;

#if 0
  int Nid_cell[] = { (3*0+0), (3*71+0), (3*21+2), (3*21+2), (3*55+1), (3*111+2) };
#else
  int Nid_cell[] = { ((3*1+2)) };
#endif

  printf(" PBCH TEST \n");
  printf("----------\n");

  if (init_test(nb_antennas_tx, nb_antennas_rx, transmission_mode, extended_prefix_flag, frame_type, Nid_cell[0], N_RB_DL) != 0) {
    printf("Initialisation problem for synchronisation test \n");
    exit(-1);;
  }

  p_test_synchro_pss_sss = test_synchro_pss_sss_nr;
  size_test_position = sizeof(test_position)/sizeof(int);

  printf("***********************************\n");
  printf("    %s Test NR synchroisation \n", test.test_current);
  printf("***********************************\n");

  for (unsigned int index = 0; index < (sizeof(Nid_cell)/sizeof(int)); index++) {

    Nid2 = GET_NID2(Nid_cell[index]);
    Nid1 = GET_NID1(Nid_cell[index]);

    for (int position = 0; position < size_test_position; position++) {

      PHY_vars_UE->frame_parms.Nid_cell = (3 * N_ID_1_NUMBER) + N_ID_2_NUMBER; /* set to unvalid value */

      decoded_pbch = (*p_test_synchro_pss_sss)(PHY_vars_UE, test_position[position], Nid2); /* return phase index which gives phase error from an array */

      test.number_of_tests++;
      printf("\n%s ", test.test_current);

      if (PHY_vars_UE->frame_parms.Nid_cell == (3 * Nid1) + Nid2) {
        if (decoded_pbch != -1) {
          printf("Test is pass with offset %d Nid1 %d Nid2 %d \n", test_position[position], Nid1, Nid2);
          test.number_of_pass++;
        }
        else {
          printf("Test is fail because pbch has not been decoded \n");
          test.number_of_fail++;
        }
      }
      else {
        printf("Test is fail with offset %d Nid1 %d Nid2 %d \n", test_position[position], Nid1, Nid2);
        test.number_of_fail++;
      }
      break;
    }
    break;
  }

  printf("\n %s Number of tests : %d  Pass : %d Pass with warning : %d Fail : %d \n", test.test_current, test.number_of_tests, test.number_of_pass, test.number_of_pass_warning, test.number_of_fail);

  printf("%s Synchronisaton test is terminated.\n\n", test.test_current);

  /* reset test statistics */
  test.number_of_tests = test.number_of_pass = test.number_of_pass_warning = test.number_of_fail = 0;

  free_context_synchro_nr();

  return(0);
}
