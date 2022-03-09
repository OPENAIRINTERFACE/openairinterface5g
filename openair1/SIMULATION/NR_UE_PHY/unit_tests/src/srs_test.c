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

#include "../../unit_tests/src/pss_util_test.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/INIT/init_extern.h"
#include "PHY/phy_extern_nr_ue.h"

#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"
#include "PHY/NR_UE_TRANSPORT/srs_modulation_nr.h"


/*******************************************************************
*
* NAME :         generate_reference_signals
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  generate sequence of reference signals
*
*********************************************************************/
void generate_reference_signals(void)
{
  int number_differencies = 0;;
  int u, v, Msc_RS;

  generate_ul_reference_signal_sequences(SHRT_MAX);

  generate_ul_ref_sigs();

  for (Msc_RS = 2; Msc_RS < 33; Msc_RS++) {
    for (u=0; u < U_GROUP_NUMBER; u++) {
      for (v=0; v < V_BASE_SEQUENCE_NUMBER; v++) {
        int number_differencies = 0;
        /* find index of table which is for this srs length */
        unsigned int Msc_RS_index_nr = 0;
        while((ul_allocated_re[Msc_RS_index_nr] != dftsizes[Msc_RS]) && (Msc_RS_index_nr < SRS_SB_CONF)){
          Msc_RS_index_nr++;
        }
        if (Msc_RS_index_nr == SRS_SB_CONF) {
          printf("No nr index found for length %d \n", dftsizes[Msc_RS]);
          assert(0);
        }
        for (int n=0; n<ul_allocated_re[Msc_RS]; n++) {
          if ((ul_ref_sigs[u][v][Msc_RS][2*n] != rv_ul_ref_sig[u][v][Msc_RS_index_nr][2*n])
          || (ul_ref_sigs[u][v][Msc_RS][2*n+1] != rv_ul_ref_sig[u][v][Msc_RS_index_nr][2*n+1])) {
            number_differencies++;
          }
        }
      }
      if (number_differencies != 0) {
        printf("Differencies for u %d v %d Msc_RS %d differencies %d \n", u, v, Msc_RS, number_differencies);
        assert(0);
      }
      else {
        //printf("no differencies for u %d v %d Msc_RS %d of size %d differencies %d \n", u, v, Msc_RS, dftsizes[Msc_RS], number_differencies);
      }
    }
  }

  printf("Ok: No detected differences for uplink reference sequences \n");
}

/*******************************************************************
*
* NAME :         default_srs_configuration
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  set srs default configuration
*
*********************************************************************/
void default_srs_configuration(NR_DL_FRAME_PARMS *frame_parms)
{
  SRS_ResourceSet_t *p_srs_resource_set;
  SRS_Resource_t *p_srs_resource;

  p_srs_resource_set = calloc( 1, sizeof(SRS_ResourceSet_t));
  if (p_srs_resource_set == NULL) {
    printf("Error default_srs_configuration: memory allocation problem \n");
    assert(0);
  }
  else {
    frame_parms->srs_nr.active_srs_Resource_Set = 0;
    frame_parms->srs_nr.number_srs_Resource_Set++;
    frame_parms->srs_nr.p_SRS_ResourceSetList[0] = p_srs_resource_set;
  }

  p_srs_resource = calloc( 1, sizeof(SRS_Resource_t));
  if (p_srs_resource == NULL) {
    free(p_srs_resource_set);
    printf("Error default_srs_configuration: memory allocation problem \n");
    assert(0);
  }

  /* init resource */
  p_srs_resource->nrof_SrsPorts = port1;
  p_srs_resource->transmissionComb = 2; /* K_TC */
  p_srs_resource->resourceMapping_nrofSymbols = 1;
  p_srs_resource->resourceMapping_repetitionFactor = 1;
  p_srs_resource->SRS_Periodicity = sr_sl4;
  p_srs_resource->SRS_Offset = 3;

  /* init resource set with only 1 resource */
  p_srs_resource_set->resourceType = periodic;
  p_srs_resource_set->number_srs_Resource++;
  frame_parms->srs_nr.p_SRS_ResourceSetList[0]->p_srs_ResourceList[0] = p_srs_resource;
}

/*******************************************************************
*
* NAME :         test_srs_single
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  test of SRS feature
*                for various combinations of parameters
*
*********************************************************************/
int test_srs_single(NR_DL_FRAME_PARMS *frame_parms,
                    int32_t *txptr,
                    UE_nr_rxtx_proc_t *proc)
{
  SRS_Resource_t *p_SRS_Resource = frame_parms->srs_nr.p_SRS_ResourceSetList[0]->p_srs_ResourceList[0];

  if (generate_srs_nr(frame_parms->srs_nr.p_SRS_ResourceSetList[0],
                      frame_parms,


                      txptr,
                      AMP,
                      proc) != 0) {
    printf("Ok: test_srs_single for K_TC %d comboffset %d cyclicShift %d u %d B_SRS %d C_SRS %d n_shift %d n_RRC %d b_hop %d groupOrSequenceHopping %d "
           " sequenceId %d l_offset %d \n",
           p_SRS_Resource->transmissionComb,   p_SRS_Resource->combOffset,          p_SRS_Resource->cyclicShift,   p_SRS_Resource->sequenceId,
           p_SRS_Resource->freqHopping_b_SRS,  p_SRS_Resource->freqHopping_c_SRS,   p_SRS_Resource->freqDomainPosition,
           p_SRS_Resource->freqDomainShift,    p_SRS_Resource->freqHopping_b_hop,   p_SRS_Resource->groupOrSequenceHopping,
           p_SRS_Resource->sequenceId,         p_SRS_Resource->resourceMapping_startPosition);

    printf("Error: generate_srs_nr was not successfully executed ! \n");
    return (-1);
  }
  else {
#if 1
    printf("Ok: test_srs_single for K_TC %d comboffset %d cyclicShift %d u %d B_SRS %d C_SRS %d n_shift %d n_RRC %d b_hop %d groupOrSequenceHopping %d "
           " sequenceId %d l_offset %d \n",
           p_SRS_Resource->transmissionComb,   p_SRS_Resource->combOffset,          p_SRS_Resource->cyclicShift,   p_SRS_Resource->sequenceId,
           p_SRS_Resource->freqHopping_b_SRS,  p_SRS_Resource->freqHopping_c_SRS,   p_SRS_Resource->freqDomainPosition,
           p_SRS_Resource->freqDomainShift,    p_SRS_Resource->freqHopping_b_hop,   p_SRS_Resource->groupOrSequenceHopping,
           p_SRS_Resource->sequenceId,         p_SRS_Resource->resourceMapping_startPosition);
#endif

    return (0);
  }
}

/*******************************************************************
*
* NAME :         test_srs_periodicity
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  test of SRS feature
*                for various combinations of parameters
*
*********************************************************************/
int test_srs_periodicity(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc)
{
  NR_DL_FRAME_PARMS *frame_parms = &(ue->frame_parms);
  SRS_Resource_t *p_srs_resource = frame_parms->srs_nr.p_SRS_ResourceSetList[0]->p_srs_ResourceList[0];

  for (int test_index = 0; test_index < NB_SRS_PERIOD; test_index++) {
    p_srs_resource->SRS_Periodicity = test_index;
    int srs_current_period = srs_period[p_srs_resource->SRS_Periodicity];
    for (int offset = 0 ; offset < 2; offset++) {
      if (offset == 0) {
        p_srs_resource->SRS_Offset = 0;
      }
      else {
        p_srs_resource->SRS_Offset = srs_current_period - 1;
      }
      printf("srs period %d offset %d \n", srs_current_period, p_srs_resource->SRS_Offset);

      int duration = (10 * srs_current_period) / frame_parms->slots_per_frame;

      for (int frame_tx = 0; frame_tx < duration; frame_tx++) {
        for (int slot_tx = 0; slot_tx < frame_parms->slots_per_frame; slot_tx++) {
          proc->frame_tx   = frame_tx;
          proc->nr_slot_tx = slot_tx;
          if (ue_srs_procedures_nr( ue, proc, 0) == 0)  {
            printf("test_srs_periodicity srs at frame %d slot %d \n", frame_tx, slot_tx);
          }
        }
      }
    }
  }

  return(0);
}
/*******************************************************************
*
* NAME :         test_srs_single
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  test of SRS feature
*                for various combinations of parameters
*
*********************************************************************/
int test_srs_specific(NR_DL_FRAME_PARMS *frame_parms,
                      int32_t *txptr,
                      UE_nr_rxtx_proc_t *proc)
{
#define NUMBER_TEST_PARAMETERS        (12)

//#define SRS_TEST_HOPPING
#ifdef SRS_TEST_HOPPING
#define NUMBER_SRS_TEST               (5)
#else
#define NUMBER_SRS_TEST               (3)
#endif

  SRS_Resource_t *p_SRS_Resource = frame_parms->srs_nr.p_SRS_ResourceSetList[0]->p_srs_ResourceList[0];

  uint8_t test_parameters[NUMBER_SRS_TEST][NUMBER_TEST_PARAMETERS] = {
    /* K_TC comboffset cyclicShift  u  B_SRS C_SRS n_shift b_hop n_RRC groupOrSequenceHopping sequenceId l_offset*/
    {   2,      0,        0,        0,    1,   3,      0,    3,    0,      neitherHopping,          0,      0      },
    {   2,      0,        0,        0,    1,   3,      0,    3,    2,      neitherHopping,          0,      1      },
    {   2,      0,        0,        0,    3,   3,      0,    1,    0,      neitherHopping,          0,      0      },
#ifdef SRS_TEST_HOPPING
    {   2,      0,        0,        0,    1,   3,      0,    0,    0,      groupHopping,           30,      0      },
    {   2,      0,        0,        0,    0,  14,      0,    0,    0,      sequenceHopping,        55,      1      },
#endif
  };

  for (int test_index = 0; test_index < NUMBER_SRS_TEST; test_index++) {

    p_SRS_Resource->transmissionComb = test_parameters[test_index][0]; /* K_TC */
    p_SRS_Resource->combOffset = test_parameters[test_index][1];
    p_SRS_Resource->cyclicShift = test_parameters[test_index][2];
    p_SRS_Resource->sequenceId = test_parameters[test_index][3];
    p_SRS_Resource->freqHopping_b_SRS = test_parameters[test_index][4];
    p_SRS_Resource->freqHopping_c_SRS = test_parameters[test_index][5];
    p_SRS_Resource->freqDomainPosition = test_parameters[test_index][6];
    p_SRS_Resource->freqHopping_b_hop = test_parameters[test_index][7];
    p_SRS_Resource->freqDomainShift = (uint16_t)test_parameters[test_index][8];
    p_SRS_Resource->groupOrSequenceHopping = test_parameters[test_index][9];
    p_SRS_Resource->sequenceId = test_parameters[test_index][10];
    p_SRS_Resource->resourceMapping_startPosition = test_parameters[test_index][11];

    if (test_srs_single(frame_parms,
                        txptr,
                        proc) != 0) {
      return (-1);
    }
  }

  return (0);
}

/*******************************************************************
*
* NAME :         test_srs_group
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  test of SRS feature
*                for various combinations of parameters
*
*********************************************************************/
int test_srs_group(NR_DL_FRAME_PARMS *frame_parms,
                   int32_t *txptr,
                   UE_nr_rxtx_proc_t *proc)
{
  SRS_Resource_t *p_SRS_Resource = frame_parms->srs_nr.p_SRS_ResourceSetList[0]->p_srs_ResourceList[0];
  uint8_t n_SRS_cs_max[5] = { 0, 0, 8, 0, 12};
  int number_of_test = 0;

  for (int K_TC = 2; K_TC < 5; K_TC = K_TC + 2) {
    for (int comboffset = 0; comboffset < K_TC; comboffset++) {
      for (int cyclicShift = 0; cyclicShift < n_SRS_cs_max[K_TC]; cyclicShift++) {
        for (int u = 0; u < U_GROUP_NUMBER; u++) {
          for (uint8_t B_SRS = 0; B_SRS < B_SRS_NUMBER; B_SRS++) {
            for (uint8_t C_SRS = 0; C_SRS < C_SRS_NUMBER; C_SRS++) {
              p_SRS_Resource->transmissionComb = K_TC;
              p_SRS_Resource->combOffset = comboffset;
              p_SRS_Resource->cyclicShift = cyclicShift;
              p_SRS_Resource->sequenceId = u;
              p_SRS_Resource->freqHopping_b_SRS = B_SRS;
              p_SRS_Resource->freqHopping_c_SRS = C_SRS;
              number_of_test++;
              if (test_srs_single(frame_parms, txptr, proc) != 0) {
                return (-1);
              }
            }
          }
        }
      }
    }
  }

  printf("Number of srs generations %d\n", number_of_test);
  return (0);
}

/*******************************************************************
*
* NAME :         test_srs
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  test of SRS feature
*
*********************************************************************/
int test_srs(PHY_VARS_NR_UE *PHY_vars_UE)
{
  int32_t *txptr;
  NR_DL_FRAME_PARMS *frame_parms = &(PHY_vars_UE->frame_parms);
  UE_nr_rxtx_proc_t *p_rxtx_proc;
  int v_return = 0;

  generate_reference_signals();

  /* start test of sounding reference signals */
  default_srs_configuration(frame_parms);

  txptr = calloc(frame_parms->samples_per_slot , sizeof(int32_t));
  if (txptr == NULL) {
    printf("Error test_srs: memory allocation problem txptr \n");
    assert(0);
  }

  p_rxtx_proc = calloc( 1, sizeof(UE_nr_rxtx_proc_t));
  if (p_rxtx_proc == NULL) {
    printf("Error test_srs: memory allocation problem p_rxtx_proc \n");
    assert(0);
  }

  if (test_srs_group(frame_parms,
                     txptr,
                     p_rxtx_proc) != 0) {
    v_return = -1;
  }

  if (test_srs_specific(frame_parms,
                        txptr,
                        p_rxtx_proc) != 0) {
    v_return = -1;
  }

  if (test_srs_periodicity(PHY_vars_UE, p_rxtx_proc) != 0) {
    v_return = -1;
  }

  for (int i = 0; i < frame_parms->srs_nr.number_srs_Resource_Set; i++) {
    if (frame_parms->srs_nr.p_SRS_ResourceSetList[i] != NULL) {
      for (int j = 0; j < frame_parms->srs_nr.p_SRS_ResourceSetList[i]->number_srs_Resource; j++) {
        if (frame_parms->srs_nr.p_SRS_ResourceSetList[0]->p_srs_ResourceList[0] != NULL) {
          free(frame_parms->srs_nr.p_SRS_ResourceSetList[0]->p_srs_ResourceList[0]);
        }
        else {
          printf("Error test_srs: memory allocation problem p_srs_ResourceList \n");
          assert(0);
        }
      }
      free(frame_parms->srs_nr.p_SRS_ResourceSetList[i]);
    }
    else {
      printf("Error test_srs: memory allocation problem \n");
      assert(0);
    }
  }

  free(txptr);
  free(p_rxtx_proc);

  free_ul_reference_signal_sequences();

  free_ul_ref_sigs();

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
  uint8_t N_RB_DL=100;
  lte_prefix_type_t extended_prefix_flag = NORMAL;
  int Nid_cell[] = {(3*0+0)};
  VOID_PARAMETER argc;
  VOID_PARAMETER argv;

  printf(" SRS TEST \n");
  printf("----------\n");

  if (init_test(nb_antennas_tx, nb_antennas_rx, transmission_mode, extended_prefix_flag, frame_type, Nid_cell[0], N_RB_DL) != 0) {
    printf("Initialisation problem for synchronisation test \n");
    exit(-1);;
  }

  if (test_srs(PHY_vars_UE) != 0) {
    printf("Test NR SRS is fail \n");
  }
  else {
    printf("Test NR SRS is pass \n");
  }

  free_context_synchro_nr();

  return(0);
}
