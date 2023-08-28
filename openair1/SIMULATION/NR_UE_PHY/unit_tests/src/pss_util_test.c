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

/*********************************************************************
*
* FILENAME    :  pss_util_test.c
*
* MODULE      :  UE test bench for unit tests
*
* DESCRIPTION :  it allows unitary tests of NR UE
*
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <time.h>

#include "PHY/defs_nr_UE.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/phy_extern_nr_ue.h"

/* to declare ue variables */
#include "PHY/phy_vars_nr_ue.h"
#include "PHY/LTE_TRANSPORT/transport_vars.h"

#include "PHY/NR_REFSIG/pss_nr.h"
#include "PHY/NR_REFSIG/sss_nr.h"
#include "PHY/NR_UE_TRANSPORT/cic_filter_nr.h"

#define DEFINE_VARIABLES_PSS_NR_H
#include "../../unit_tests/src/pss_util_test.h"
#undef DEFINE_VARIABLES_PSS_NR_H

#define DEFINE_VARIABLES_INPUT_BUFFER_TEST_H
#include "../../unit_tests/src/input_buffer_test.h"
#undef DEFINE_VARIABLES_INPUT_BUFFER_TEST_H

#include "common/utils/LOG/log.h"

/************** DEFINE *******************************************/

/*************** LOCAL VARIABLES***********************************/

static  nfapi_config_request_t  config_t;
static  nfapi_config_request_t *config =&config_t;

/*************** FUNCTIONS ****************************************/

//void phy_init_nr_top(NR_DL_FRAME_PARMS *frame_parms);
int init_nr_ue_signal(PHY_VARS_NR_UE *ue, int nb_connected_eNB);

/*******************************************************************
*
* NAME :         apply phase shift
*
* PARAMETERS :   pointer to buffer in frequency
*                length of sample
*
* RETURN :       none
*
* DESCRIPTION :  apply phase shift to samples
*
*********************************************************************/

void phase_shift_samples(int16_t *samples, int length, int16_t phase_shift_re, int16_t phase_shift_im) {
  int16_t sample_re, sample_im;

  for (int i = 0; i < length; i++) {
    sample_re = (((phase_shift_re*samples[2*i])>>SCALING_METRIC_SSS_NR) - ((phase_shift_im*samples[2*i+1])>>SCALING_METRIC_SSS_NR));
    sample_im = (((phase_shift_im*samples[2*i])>>SCALING_METRIC_SSS_NR) + ((phase_shift_re*samples[2*i+1])>>SCALING_METRIC_SSS_NR));
    samples[2*i] = sample_re;
    samples[2*i+1] = sample_im;
  }
}

/*******************************************************************
*
* NAME :         display_data
*
* PARAMETERS :   pointer to buffer
*                position in the buffer
*
* RETURN :       none
*
* DESCRIPTION :  display pss sequence and data of a buffer
*
*
*********************************************************************/

void display_data(int pss_sequence_number, int16_t *rxdata, int position) {
#ifdef DEBUG_TEST_PSS
  int pss_sequence = get_softmodem_params()->sl_mode == 0 ? NUMBER_PSS_SEQUENCE : NUMBER_PSS_SEQUENCE_SL;
  int16_t *pss_sequence[pss_sequence] = {primary_synch0_time, primary_synch1_time, primary_synch2_time};
  int16_t *pss_sequence_sl[pss_sequence] = {primary_synch0_time, primary_synch1_time};
  int16_t *pss_sequence_time = pss_sequence[pss_sequence_number];;
  if (get_softmodem_params()->sl_mode != 0) {
    pss_sequence_time = pss_sequence_sl[pss_sequence_number];
  printf("   pss %6d             data \n", pss_sequence_number);
  for (int i = 0; i < 4; i++) {
    if (pss_sequence_number < pss_sequence) {
      printf("[i %6d] : %4d       [i %6d] : %8i     at address : %p \n", i, pss_sequence_time[2*i], (i + position), rxdata[2*i + (position*2)],  &(rxdata[2*i + (position*2)]));
      printf("[q %6d] : %4d       [q %6d] : %8i     at address : %p \n", i, pss_sequence_time[2*i+1], (i + position), rxdata[2*i + 1 + (position*2)],  &(rxdata[2*i + 1 + (position*2)]));
    } else {
      printf("[i %6d] : Undef      [i %6d] : %8i     at address : %p \n", i, (i + position), rxdata[2*i + (position*2)], &(rxdata[2*i + (position*2)]));
      printf("[q %6d] : Undef      [q %6d] : %8i     at address : %p \n", i, (i + position), rxdata[2*i + 1 + (position*2)], &(rxdata[2*i + 1 + (position*2)]));
    }
  }

  nr_init_frame_parms
  printf("    ...             ... \n");
#else
  (void) pss_sequence_number;
  (void) rxdata;
  (void) position;
#endif
}

/*******************************************************************
*
* NAME :         display_test_configuration_pss
*
* PARAMETERS :
*
*
* RETURN :       none
*
* DESCRIPTION :  display test configuration of pss
*
*********************************************************************/

void display_test_configuration_pss(int position, int pss_sequence_number) {
  const char next_test_text[]  = "------------------------------------------------\n";
  const char test_text_pss[]       = "Test nr pss with Nid2 %i at position %i \n";
  printf(next_test_text);
  printf(test_text_pss, pss_sequence_number, position);
}

/*******************************************************************
*
* NAME :         display_test_configuration_sss
*
* PARAMETERS :
*
*
* RETURN :       none
*
* DESCRIPTION :  display test configuration of pss
*
*********************************************************************/

void display_test_configuration_sss(int sss_sequence_number) {
  const char test_text_sss[]       = "Test nr sss with Nid1 %i \n";
  printf(test_text_sss, sss_sequence_number);
}

/*******************************************************************
*
* NAME :         undefinedFunction
*
* PARAMETERS :   function name
*
* RETURN :       none
*
* DESCRIPTION :  these functions allow to solve undefined functions at link
*                function name is printed at execution time for checking that
*                each function is not called
*
*********************************************************************/

void undefined_function(const char *function) {
  printf("%s undefined \n", function);
  printf("Warning: function \"%s\" has been replaced by an empty function for avoiding undefined function error at build \n", function);
}

/*******************************************************************
*
* NAME :         init_synchro_test
*
* PARAMETERS :   configuration for UE and eNB
*
* RETURN :       0 if OK else error
*
* DESCRIPTION :  initialisation of UE and eNode contexts
*
*********************************************************************/

int init_test(unsigned char N_tx, unsigned char N_rx, unsigned char transmission_mode,
              unsigned char extended_prefix_flag, uint8_t frame_type, uint16_t Nid_cell, int N_RB_DL) {
  (void) transmission_mode;
  NR_DL_FRAME_PARMS *frame_parms;
  int log_level = OAILOG_TRACE;
  logInit();
  // enable these lines if you need debug info
  //set_comp_log(PHY,LOG_DEBUG,LOG_HIGH,1);
  set_glog(log_level);
#ifndef NR_UNIT_TEST
  cpuf = get_cpu_freq_GHz();
  //LOG_I(PHY, "[CONFIG] Test of UE synchronisation \n");
  set_component_filelog(USIM);  // file located in /tmp/testSynchroue.txt
#endif
  //randominit(0);
  //set_taus_seed(0);
  printf("Start lte_param_init, frame_type %d, extended_prefix %d\n",frame_type,extended_prefix_flag);
  PHY_vars_UE = malloc(sizeof(PHY_VARS_NR_UE));
  bzero(PHY_vars_UE, sizeof(PHY_VARS_NR_UE));

  if (PHY_vars_UE == NULL)
    return(-1);

  frame_parms = &(PHY_vars_UE->frame_parms);
  frame_parms->N_RB_DL              = N_RB_DL;   //50 for 10MHz and 25 for 5 MHz
  frame_parms->N_RB_UL              = N_RB_DL;
  frame_parms->Ncp                  = extended_prefix_flag;
  frame_parms->Nid_cell = Nid_cell;
  frame_parms->nb_antennas_tx       = N_tx;
  frame_parms->nb_antennas_rx       = N_rx;
  frame_parms->frame_type           = frame_type;
  frame_parms->nb_antenna_ports_gNB = 1;
  frame_parms->threequarter_fs      = 0;
  frame_parms->numerology_index     = NUMEROLOGY_INDEX_MAX_NR;
  int mu = 1;
  int n_ssb_crb = 0;
  int ssb_subcarrier_offset = 0;
  nr_init_frame_parms_ue(frame_parms, mu, extended_prefix_flag, N_RB_DL, n_ssb_crb, ssb_subcarrier_offset);
  int nid_2_num = get_softmodem_params()->sl_mode == 0 ? N_ID_2_NUMBER : N_ID_2_NUMBER_SL;
  PHY_vars_UE->frame_parms.Nid_cell = (3 * N_ID_1_NUMBER) + nid_2_num; /* set to unvalid value */

  //phy_init_nr_top(frame_parms);

  if (init_nr_ue_signal(PHY_vars_UE, 1) != 0) {
    LOG_E(PHY,"Error at UE NR initialisation : at line %d in function %s of file %s \n", LINE_FILE , __func__, FILE_NAME);
    return (0);
  }

  /* dummy initialisation of global structure PHY_vars_UE_g */
  uint16_t NB_UE_INST=1;
  PHY_vars_UE_g = (PHY_VARS_NR_UE ** *)calloc( NB_UE_INST, sizeof(PHY_VARS_NR_UE **));

  for (int UE_id=0; UE_id<NB_UE_INST; UE_id++) {
    PHY_vars_UE_g[UE_id] = (PHY_VARS_NR_UE **) calloc( MAX_NUM_CCs, sizeof(PHY_VARS_NR_UE *));

    for (int CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      //(frame_parms[CC_id])->nb_antennas_tx     = 1;
      //(frame_parms[CC_id])->nb_antennas_rx     = nb_antennas_rx_ue;
      // PHY_vars_UE_g[UE_id][CC_id] = init_lte_UE(frame_parms[CC_id], UE_id,abstraction_flag);
      PHY_vars_UE_g[UE_id][CC_id] = calloc(1, sizeof(PHY_VARS_NR_UE));
      PHY_vars_UE_g[UE_id][CC_id]->Mod_id=UE_id;
      PHY_vars_UE_g[UE_id][CC_id]->CC_id=CC_id;
    }
  }

  return (0);
}

/*******************************************************************
*
* NAME :         set_random_rx_buffer
*
* PARAMETERS :   Ue context
*
* RETURN :       0 if OK else error
*
* DESCRIPTION :  write random data in received UE buffer of the synchronisation window
*
*********************************************************************/

typedef enum {
  ZERO_DATA,
  SINUSOIDAL_DATA,
  RANDOM_DATA,
  RANDOM_MAX_DATA
} random_data_format_t;

/* data is a pure cosinus wave */
#define  SAMPLING_RATE           (30720000L)
#define  SCALING_SINUSOIDAL_DATA (4)                  /* 16 is max value without decimation */
#define  FREQUENCY_15_MHZ        (15360000L)
#define  FREQUENCY               (FREQUENCY_15_MHZ)    /* to generate a frequency with a sampling of 30,72 MHz  5 gives 770 KHz, 20 gives 1,5 MHz, 40 gives 3 MHz */

void set_random_rx_buffer(PHY_VARS_NR_UE *PHY_vars_UE, int amp) {
  NR_DL_FRAME_PARMS *frame_parms = &(PHY_vars_UE->frame_parms);
  int samples_for_frame = frame_parms->samples_per_frame;
  int16_t random;
  int16_t *data_p;
  random_data_format_t data_format = SINUSOIDAL_DATA;
  /* reinitialise random for always getting same data */
  srand(0);
  double n = 0;

  for (int aa=0; aa<PHY_vars_UE->frame_parms.nb_antennas_rx; aa++) {
    data_p = (int16_t *) &(PHY_vars_UE->common_vars.rxdata[aa][0]);
    int frequency_switch = samples_for_frame/LTE_NUMBER_OF_SUBFRAMES_PER_FRAME;
    int frequency_step = 0;
    double beat = (2*M_PI*FREQUENCY_15_MHZ)/(SAMPLING_RATE);

    for (int i=0; i< samples_for_frame; i++) {
      switch(data_format) {
        case ZERO_DATA: {
          /* all data are forced to zero */
          random = 0;
          break;
        }

        case SINUSOIDAL_DATA: {
          /* sinusoidal signal */
          n = cos(beat*i);
          random =  n * (amp * SCALING_SINUSOIDAL_DATA);
          frequency_step++;

          if (frequency_step == frequency_switch) {
            beat = beat/2;  /* frequency is divided by 2 */
            //printf("frequency %f at %d\n", (beat/2*M_PI), i);
            frequency_step = 0;
          }

          //printf("%d : cos %d %d \n", i, n, random);
          break;
        }

        case RANDOM_DATA: {
          /* random data can take any value between -SHRT_MAX and SHRT_MAX */
          /* in this case one can use maxim value for uint16 because there is no saturation */
#define SCALING_RANDOM_DATA       (24)      /* 48 is max value without decimation */
#define RANDOM_MAX_AMP            (amp * SCALING_RANDOM_DATA)
          random = ((rand() % RANDOM_MAX_AMP) - RANDOM_MAX_AMP/2);
          break;
        }

        case RANDOM_MAX_DATA: {
          /* random data can take only two value (-RANDOM_MAX) or RANDOM_MAX */
          /* In this case saturation can occur with value of scaling_value greater than 23 */
#define SCALING_RANDOM_MAX_DATA   (8)
#define RANDOM_VALUE              (amp * SCALING_RANDOM_DATA)
          const int random_number[2] = {-1,+1};
          random = random_number[rand()%2] * RANDOM_VALUE;
          break;
        }

        default: {
          printf("Format of data is undefined \n");
          assert(0);
          break;
        }
      }

      data_p[2*i] = random;
      data_p[2*i+1] = random;
#if 0

      if (i < 10) {
        printf("random %d \n", random);
        printf("data[%d] : %d  %d at address %p \n", i, data_p[2*i], data_p[2*i+1], &data_p[2*i]);
      }

#endif
    }
  }
}

/*******************************************************************
*
* NAME :         set_pss_in_rx_buffer_from_external_buffer
*
* PARAMETERS :   UE context
*                position of pss in the received UE buffer
*                number of pss sequence*
*
* RETURN :      0 if OK else error
*
* DESCRIPTION : set received buffer according to an external buffer
*
*********************************************************************/

int set_pss_in_rx_buffer_from_external_buffer(PHY_VARS_NR_UE *PHY_vars_UE, short *input_buffer) {
  NR_DL_FRAME_PARMS *frame_parms = &(PHY_vars_UE->frame_parms);
  int samples_for_frame = frame_parms->samples_per_frame; /* both i and q */

  for (int aa=0; aa<PHY_vars_UE->frame_parms.nb_antennas_rx; aa++) {
    for (int i = 0; i < samples_for_frame; i++) {
      ((int16_t *)PHY_vars_UE->common_vars.rxdata[aa])[2*i] = input_buffer[2*i];     /* real part */
      ((int16_t *)PHY_vars_UE->common_vars.rxdata[aa])[2*i+1] = input_buffer[2*i+1]; /* imaginary part */
    }
  }

  /* check that sequence has been properly copied */
  for (int aa=0; aa<PHY_vars_UE->frame_parms.nb_antennas_rx; aa++) {
    for (int i=0; i<samples_for_frame; i++) {
      if ((input_buffer[2*i] != ((int16_t *)PHY_vars_UE->common_vars.rxdata[aa])[2*i])
          || (input_buffer[2*i+1] != ((int16_t *)PHY_vars_UE->common_vars.rxdata[aa])[2*i+1])) {
        printf("Sequence pss was not properly copied into received buffer at index %d \n", i);
        exit(-1);
      }
    }
  }

  return (0);
}

/*******************************************************************
*
* NAME :         set_pss_in_rx_buffer
*
* PARAMETERS :   UE context
*                position of pss in the received UE buffer
*                number of pss sequence*
*
* RETURN :      0 if OK else error
*
* DESCRIPTION :  write reference pss sequence at a specified sample position in the received UE buffer
*
*                first pss sequence is written into the buffer only if:
*                - pss sequence is valid
*                - there is enough room for written the complete sequence
*
*                second sequence is written at a position of 5 ms after first pss sequence if:
*                - pss sequence is valid
*                - there is enough room for written the complete sequence*
*
*********************************************************************/

int set_pss_in_rx_buffer(PHY_VARS_NR_UE *PHY_vars_UE, int position_symbol, int pss_sequence_number) {
  NR_DL_FRAME_PARMS *frame_parms = &(PHY_vars_UE->frame_parms);
  int samples_for_frame = frame_parms->samples_per_frame;
  int16_t *pss_sequence_time;

  if ((position_symbol > samples_for_frame)
      || ((position_symbol + frame_parms->ofdm_symbol_size) > samples_for_frame)) {
    printf("This pss sequence can not be fully written in the received window \n");
    return (-1);
  }
  int pss_sequence = get_softmodem_params()->sl_mode == 0 ? NUMBER_PSS_SEQUENCE : NUMBER_PSS_SEQUENCE_SL;
  if ((pss_sequence_number >= pss_sequence) && (pss_sequence_number < 0)) {
    printf("Unknow pss sequence %d \n", pss_sequence_number);
    return (-1);
  }

  pss_sequence_time = primary_synchro_time_nr[pss_sequence_number];

  for (int aa=0; aa<PHY_vars_UE->frame_parms.nb_antennas_rx; aa++) {
    for (int i = 0; i < frame_parms->ofdm_symbol_size; i++) {
      ((int16_t *)PHY_vars_UE->common_vars.rxdata[aa])[(position_symbol*2) + (2*i)] = pss_sequence_time[2*i];     /* real part */
      ((int16_t *)PHY_vars_UE->common_vars.rxdata[aa])[(position_symbol*2) + (2*i+1)] = pss_sequence_time[2*i+1]; /* imaginary part */
    }
  }

  /* check that sequence has been properly copied */
  for (int aa=0; aa<PHY_vars_UE->frame_parms.nb_antennas_rx; aa++) {
    for (int i=0; i<(frame_parms->ofdm_symbol_size); i++) {
      if ((pss_sequence_time[2*i] != ((int16_t *)PHY_vars_UE->common_vars.rxdata[aa])[(position_symbol*2) + (2*i)])
          || (pss_sequence_time[2*i+1] != ((int16_t *)PHY_vars_UE->common_vars.rxdata[aa])[(position_symbol*2) + (2*i+1)])) {
        printf("Sequence pss was not properly copied into received buffer at index %d \n", i);
        exit(-1);
      }
    }
  }

  return (0);
}

/*******************************************************************
*
* NAME :         set_sequence_pss
*
* PARAMETERS :   UE context
*                position of pss in the received UE buffer
*                number of pss sequence*
*
* RETURN :      0 if OK else error
*
* DESCRIPTION :  write reference pss sequence at a specified sample position in the received UE buffer
*
*                pss sequence is written into the buffer only if:
*                - pss sequence is valid
*                - there is enough room for written the complete sequence*
*
*********************************************************************/

void set_sequence_pss(PHY_VARS_NR_UE *PHY_vars_UE, int position_symbol, int pss_sequence_number) {
  NR_DL_FRAME_PARMS *frame_parms = &(PHY_vars_UE->frame_parms);
  int samples_for_frame = frame_parms->samples_per_frame;
  /* initialise received ue data with random */
  set_random_rx_buffer(PHY_vars_UE, AMP);

  /* in this case input buffer is provided by an external buffer */
  if (position_symbol < 0) {
    set_pss_in_rx_buffer_from_external_buffer(PHY_vars_UE, input_buffer);
  }
  int pss_sequence = get_softmodem_params()->sl_mode == 0 ? NUMBER_PSS_SEQUENCE : NUMBER_PSS_SEQUENCE_SL;
  /* write pss sequence in received ue buffer */
  else if (pss_sequence_number < pss_sequence) {
    if (position_symbol > (samples_for_frame - frame_parms->ofdm_symbol_size)) {
      printf("This position for pss sequence %d is not supported because it exceeds the frame length %d!\n", position_symbol, samples_for_frame);
      exit(0);
    }

    if (set_pss_in_rx_buffer(PHY_vars_UE, position_symbol, pss_sequence_number) != 0)
      printf("Warning: pss sequence can not be properly written into received buffer !\n");
  }

  display_data(pss_sequence_number, (int16_t *)&(PHY_vars_UE->common_vars.rxdata[0][0]), position_symbol);
}
