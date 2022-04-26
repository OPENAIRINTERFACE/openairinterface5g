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
* FILENAME    :  pss_util_test.h
*
* MODULE      :  UE utilities for test bench of synchronisation
*
* DESCRIPTION :  it allows unitary tests of UE synchronisation on host machine
*
************************************************************************/

#ifndef PSS_UTIL_TEST_H
#define PSS_UTIL_TEST_H

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "PHY/defs_nr_UE.h"
#include "PHY/INIT/init_extern.h"
#include "PHY/phy_extern_nr_ue.h"

#include "PHY/NR_REFSIG/pss_nr.h"

#ifdef DEFINE_VARIABLES_PSS_NR_H
#define EXTERN
#define INIT_VARIABLES_PSS_NR_H
#else
#define EXTERN extern
#undef INIT_VARIABLES_PSS_NR_H
#endif

/************** DEFINE *******************************************/

//#define DEBUG_TEST_PSS
#define PSS_DETECTION_MARGIN_MAX    (4)

#define NUMEROLOGY_INDEX_MAX_NR     (5)

/*************** TYPE*********************************************/

typedef struct {
  const char *test_current;
  int number_of_tests;
  int number_of_pass;
  int number_of_pass_warning;
  int number_of_fail;
} test_t;

/*************** GLOBAL VARIABLES***********************************/

EXTERN PHY_VARS_eNB *PHY_vars_eNB;
EXTERN PHY_VARS_NR_UE *PHY_vars_UE;

/*************** FUNCTIONS *****************************************/

void undefined_function(const char *function);

void display_data(int pss_sequence_number, int16_t *rxdata, int position);

void init_decimator_test(void);

void decimation_synchro_nr(PHY_VARS_NR_UE *PHY_vars_UE, int **rxdata);

void restore_context_frame(int **rxdata);

int set_pss_in_rx_buffer(PHY_VARS_NR_UE *PHY_vars_UE, int position_symbol, int pss_sequence_number);

int init_test(unsigned char N_tx, unsigned char N_rx, unsigned char transmission_mode,
                      unsigned char extended_prefix_flag, uint8_t frame_type, uint16_t Nid_cell,
                      int N_RB_DL);

void display_test_configuration_pss(int position, int pss_sequence_number);

void display_test_configuration_sss(int sss_sequence_number);

int set_pss_nr(int ofdm_symbol_size);

void set_random_rx_buffer(PHY_VARS_NR_UE *PHY_vars_UE, int amp);

void set_sequence_pss(PHY_VARS_NR_UE *PHY_vars_UE, int position_symbol, int pss_sequence_number);

int set_pss_in_rx_buffer_from_external_buffer(PHY_VARS_NR_UE *PHY_vars_UE, short *inputBuffer);

void phase_shift_samples(int16_t *samples, int length, int16_t phase_shift_re, int16_t phase_shift_im);

#endif /* PSS_UTIL_TEST_H */
