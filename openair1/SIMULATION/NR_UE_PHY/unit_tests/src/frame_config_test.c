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
* FILENAME    :  frame_config_test.c
*
* MODULE      :  UE test bench for tdd configuration
*
* DESCRIPTION :  it allows unitary tests for tdd configuration
*
************************************************************************/

#include "../../unit_tests/src/pss_util_test.h"

#include "PHY/defs_nr_UE.h"
#include "PHY/INIT/init_extern.h"
#include "PHY/phy_extern_nr_ue.h"

#include "SCHED_NR_UE/phy_frame_config_nr.h"

/*******************************************************************
*
* NAME :         display_frame_configuration
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  display uplink & downlink configuration
*
*********************************************************************/

void display_frame_configuration(NR_DL_FRAME_PARMS *frame_parms) {

  printf("\nTdd configuration tti %d downlink %d uplink %d period %d \n", frame_parms->slots_per_subframe, frame_parms->p_tdd_UL_DL_Configuration->nrofDownlinkSlots,
      frame_parms->p_tdd_UL_DL_Configuration->nrofUplinkSlots, frame_parms->p_tdd_UL_DL_Configuration->dl_UL_TransmissionPeriodicity);

  int k = (TDD_CONFIG_NB_FRAMES * NR_NUMBER_OF_SUBFRAMES_PER_FRAME) - 1; //19;
  int tti = 0;
  for (int j = 0; j < TDD_CONFIG_NB_FRAMES * frame_parms->slots_per_frame; j++) {
    int frame = 0;
    if (j != 0) {
      frame = j / frame_parms->slots_per_frame;
      tti = j % frame_parms->slots_per_frame;
    }
    else {
      frame = 0;
      tti = 0;
    }

    if (slot_select_nr(frame_parms, frame, tti) & NR_DOWNLINK_SLOT) {
      printf(" [%3d] D", j);
    }
    else {
      if (slot_select_nr(frame_parms, frame, tti) & NR_UPLINK_SLOT) {
        printf(" [%3d] U", j);
      }
      else {
        printf("Error test_frame_configuration: unexpected value \n");
        assert(0);
      }
    }
    if (j == k) {
      printf("\n");
      k += (TDD_CONFIG_NB_FRAMES * NR_NUMBER_OF_SUBFRAMES_PER_FRAME); // 20
    }
  }
  printf("\n");
}

/*******************************************************************
*
* NAME :         set_tti_test
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  set tti
*
*********************************************************************/

void set_tti_test(NR_DL_FRAME_PARMS *frame_parms, int slots_per_subframe)
{
  frame_parms->slots_per_subframe = slots_per_subframe;
  frame_parms->slots_per_frame    = slots_per_subframe * NR_NUMBER_OF_SUBFRAMES_PER_FRAME;
}

/*******************************************************************
*
* NAME :         test_frame_configuration
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  test of frame configuration feature
*
*********************************************************************/

int test_frame_configuration(PHY_VARS_NR_UE *PHY_vars_UE)
{
  NR_DL_FRAME_PARMS *frame_parms = &(PHY_vars_UE->frame_parms);
  int v_return = 0;

  #define  NO_DOWNLINK_SYMBOL           (0)
  #define  NO_UPLINK_SYMBOL             (0)

  #define  TTI_DEFAULT                  (2)
  #define  DL_SLOT_DEFAULT              (4)
  #define  UL_SLOT_DEFAULT              (1)
  #define  TDD_PERIODICITY_DEFAULT      (ms2p5)

  #define  TDD_WRONG_CONFIG_TEST        (3)
  #define  TDD_GOOD_CONFIG_TEST         (18)
  #define  TDD_CONFIG_TEST              (TDD_WRONG_CONFIG_TEST + TDD_GOOD_CONFIG_TEST)

  /* this array allows setting different tdd configurations */
  /* it defines number of tti (slot) per subframe, number of downlink slots, number of uplink slots */
  /* and a period of the configuration */
  /* first configurations (wrong) shall be fired error because they are not correct */
  /* other configurations (good) are correct and they do not generate any error */
  /* there is a tdd configuration for each line */

  uint16_t TDD_configuration[TDD_CONFIG_TEST][4] = {
  /*       tti per subframe      SLOT_DL          SLOT_UL        period                    */
    {              2,               4+1,             1,          ms2p5                     },
    {              2,               4,             1+1,          ms2p5                     },
    {              2,               4,               1,          ms0p5                     },
    {              1,               1,               1,          ms2                       },
    {              1,               4,               1,          ms5                       },
    {              1,               9,               1,          ms10                      },
    {              2,               1,               1,          ms1                       },
    {              2,               3,               1,          ms2                       },
    {              2,               4,               1,          ms2p5                     },
    {              2,               9,               1,          ms5                       },
    {              2,              19,               1,          ms10                      },
    {              4,               1,               1,          ms0p5                     },
    {              4,               3,               1,          ms1                       },
    {              4,               4,               1,          ms1p25                    },
    {              4,               7,               1,          ms2                       },
    {              4,               9,               1,          ms2p5                     },
    {              4,              19,               1,          ms5                       },
    {              4,              38,               2,          ms10                      },
    {              8,               4,               1,          ms0p625                   },
    {             16,               7,               1,          ms0p5                     },
    {    TTI_DEFAULT, DL_SLOT_DEFAULT, UL_SLOT_DEFAULT,          TDD_PERIODICITY_DEFAULT   },
  };

  #define  TDD_WRONG_DEDICATED_CONFIG_TEST        (7)
  #define  TDD_GOOD_DEDICATED_CONFIG_TEST         (18)
  #define  TDD_DEDICATED_CONFIG_TEST              (TDD_WRONG_DEDICATED_CONFIG_TEST + TDD_GOOD_DEDICATED_CONFIG_TEST)

  /* this array allows setting different tdd dedicated configurations */
  /* tdd dedicated configuration overwrite current tdd confirguration for specified slot number */
  /* it defines slot number to modify, then with a bit map number of downlink symbol, number of uplink symbols */
  /* if next slot is set to 0 means that there is no additional slot to modify with current dedicated configuration */
  /* if next slot is set to 1 means that there is an additional slot to mdofy with current dedicated configuration */
  /* so tdd dedicated configuration can be composed of only 1 lines (for 1 slot) or several lines for several slots */
  /* first dedictaed configurations (wrong) shall be fired errors because they are not correct */
  /* other dedicated configurations (good) are correct and they do not generate any error */

  uint16_t TDD_dedicated_config[TDD_DEDICATED_CONFIG_TEST][4] = {
  /*      slot number  SLOT_DL                   SLOT_UL                 next slot */
    {        0,           1,                        1,                         0      },
    {        0,           0,                        1,                         0      },
    {       40,           0,                        NR_TDD_SET_ALL_SYMBOLS,    0      },
    {        0,           NR_TDD_SET_ALL_SYMBOLS,   NR_TDD_SET_ALL_SYMBOLS,    0      },
    {        0,           0,                        0,                         0      },
    {        0,           0,                        NR_TDD_SET_ALL_SYMBOLS,    1      },
    {        0,           0,                        0,                         0      },
    {        0,           NR_TDD_SET_ALL_SYMBOLS,   0,                         0      },
    {        0,           0,                        NR_TDD_SET_ALL_SYMBOLS,    1      },
    {        5,           0,                        NR_TDD_SET_ALL_SYMBOLS,    1      },
    {       10,           0,                        NR_TDD_SET_ALL_SYMBOLS,    1      },
    {       15,           0,                        NR_TDD_SET_ALL_SYMBOLS,    1      },
    {       20,           0,                        NR_TDD_SET_ALL_SYMBOLS,    1      },
    {       25,           0,                        NR_TDD_SET_ALL_SYMBOLS,    1      },
    {       30,           0,                        NR_TDD_SET_ALL_SYMBOLS,    1      },
    {       35,           0,                        NR_TDD_SET_ALL_SYMBOLS,    1      },
    {        4,           NR_TDD_SET_ALL_SYMBOLS,   0,                         1      },
    {        9,           NR_TDD_SET_ALL_SYMBOLS,   0,                         1      },
    {       14,           NR_TDD_SET_ALL_SYMBOLS,   0,                         1      },
    {       19,           NR_TDD_SET_ALL_SYMBOLS,   0,                         1      },
    {       19,           NR_TDD_SET_ALL_SYMBOLS,   0,                         1      },
    {       24,           NR_TDD_SET_ALL_SYMBOLS,   0,                         1      },
    {       29,           NR_TDD_SET_ALL_SYMBOLS,   0,                         1      },
    {       34,           NR_TDD_SET_ALL_SYMBOLS,   0,                         1      },
    {       39,           NR_TDD_SET_ALL_SYMBOLS,   0,                         0      },
  };

  /* TEST wrong and good tdd configurations starting by wrong ones */
  int return_status = -1;
  for (int i = 0; i < TDD_CONFIG_TEST; i++) {
    if (i == TDD_WRONG_CONFIG_TEST) {
      return_status = 0;  /* status should be fine for fine configurations */
    }

    set_tti_test(frame_parms, TDD_configuration[i][0]);

    if (set_tdd_config_nr(frame_parms, TDD_configuration[i][3], TDD_configuration[i][1], NO_DOWNLINK_SYMBOL,
                          TDD_configuration[i][2], NO_UPLINK_SYMBOL)  != return_status) {
      v_return = (-1);
    }

    display_frame_configuration(frame_parms);

    free_tdd_configuration_nr(frame_parms);
  }

  /* set default configuration */
  set_tti_test(frame_parms, TTI_DEFAULT);

  if (set_tdd_config_nr(frame_parms, TDD_PERIODICITY_DEFAULT, DL_SLOT_DEFAULT, NO_DOWNLINK_SYMBOL, UL_SLOT_DEFAULT, NO_UPLINK_SYMBOL) != 0) {
    v_return = (-1);
  }

  /* TEST wrong and good tdd dedicated configurations starting by wrong ones */
  return_status = (-1);
  for (int i = 0; i < TDD_DEDICATED_CONFIG_TEST; i++) {

    bool next = false;
    do {
      if (i == TDD_WRONG_DEDICATED_CONFIG_TEST) {
        return_status = 0;  /* status should be fine for fine configurations */
      }

      add_tdd_dedicated_configuration_nr( frame_parms, TDD_dedicated_config[i][0], TDD_dedicated_config[i][1], TDD_dedicated_config[i][2]);

      if (TDD_dedicated_config[i][3] == 1) {
        next = true;
        i = i + 1;
      }
      else {
        next = false;
      }
    } while (next);

    if (set_tdd_configuration_dedicated_nr(frame_parms) != return_status) {
      v_return = (-1);
    }

    if (return_status == 0) {
      display_frame_configuration(frame_parms);
    }

    free_tdd_configuration_dedicated_nr(frame_parms);
  }

  free_tdd_configuration_nr(frame_parms);

  return (v_return);
}

/*******************************************************************
*
* NAME :         main
*
* DESCRIPTION :  test of tdd configuration
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
  int Nid_cell[] = {(3*0+0)};
  VOID_PARAMETER argc;
  VOID_PARAMETER argv;

  printf(" FRAME CONFIGURATION TEST \n");
  printf("--------------------------\n");

  if (init_test(nb_antennas_tx, nb_antennas_rx, transmission_mode, extended_prefix_flag, frame_type, Nid_cell[0], N_RB_DL) != 0) {
    printf("Initialisation problem for frame configuration test \n");
    exit(-1);;
  }

  if (test_frame_configuration(PHY_vars_UE) != 0) {
    printf("Test NR FRAME CONFIGURATION is fail \n");
  }
  else {
    printf("Test NR FRAME CONFIGURATION is pass \n");
  }

  lte_sync_time_free();

  free_context_synchro_nr();

  return(0);
}
