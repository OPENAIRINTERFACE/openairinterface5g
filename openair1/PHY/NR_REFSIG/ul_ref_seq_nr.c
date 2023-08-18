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

/***********************************************************************
*
* FILENAME    :  ul_ref_seq_nr.c
*
* MODULE      :  generation of uplink reference sequence for nr
*
* DESCRIPTION :  function to generate uplink reference sequences
*                see 3GPP TS 38.211 5.2.2 Low-PAPR sequence generation
*
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "defs.h"

#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"

/*******************************************************************
*
* NAME :         base_sequence_less_3_RB
*
* PARAMETERS :   M_ZC length of Zadoff Chu sequence
*                u sequence group number
*                scaling to apply
*
* RETURN :       pointer to generated sequence
*
* DESCRIPTION :  base sequence generation of less than 36 elements
*                see TS 38.211 5.2.2.2 Base sequences of length less than 36
*
*********************************************************************/

static int16_t *base_sequence_less_than_36(unsigned int M_ZC, unsigned int u, unsigned int scaling)
{
  char *phi_table;
  int16_t *rv_overbar;
  double x;
  unsigned int n;

  switch(M_ZC) {
    case 6:
      phi_table = (char *)phi_M_ZC_6;
      break;
    case 12:
      phi_table = (char *)phi_M_ZC_12;
      break;
    case 18:
      phi_table = (char *)phi_M_ZC_18;
      break;
    case 24:
      phi_table = (char *)phi_M_ZC_24;
      break;
    case 30:
      break;
    default:
      printf("function base_sequence_less_than 36_: unsupported base sequence size : %u \n", M_ZC);
      assert(0);
      break;
  }

  rv_overbar = malloc16(IQ_SIZE*M_ZC);

  if (rv_overbar == NULL) {
    msg("Fatal memory allocation problem \n");
    assert(0);
  }

  if (M_ZC == 30) {
    for (n=0; n<M_ZC; n++) {
      x = -(M_PI * (u + 1) * (n + 1) * (n + 2))/(double)31;
      rv_overbar[2*n]   =(int16_t)(floor(scaling*cos(x)));
      rv_overbar[2*n+1] =(int16_t)(floor(scaling*sin(x)));
    }
  }
  else {
    for (n=0; n<M_ZC; n++) {
      x = (double)phi_table[n + u*M_ZC] * (M_PI/4);
      rv_overbar[2*n]   = (int16_t)(floor(scaling*cos(x)));
      rv_overbar[2*n+1] = (int16_t)(floor(scaling*sin(x)));
    }
  }
  return rv_overbar;
}

/*******************************************************************
*
* NAME :         get_index_for_dmrs_lowpapr_seq
*
* PARAMETERS :   num_dmrs_res - Total number of DMRS RES possible in allocated RBs                
*                
*
* RETURN :       returns index of array dmrs_ul_allocated_res 
*
* DESCRIPTION :  finds the index which in turn is used to index into dmrs low papr sequences 
*
*********************************************************************/

int16_t get_index_for_dmrs_lowpapr_seq(int16_t num_dmrs_res) {

  int16_t index = num_dmrs_res/6 - 1;

  if (index >= MAX_INDEX_DMRS_UL_ALLOCATED_REs)
    index = MAX_INDEX_DMRS_UL_ALLOCATED_REs-1;

  for (;index >= 0; index--) {
    if (dmrs_ul_allocated_res[index] == num_dmrs_res)
      break;
  }

  LOG_D(PHY, "num_dmrs_res: %d    INDEX RETURNED:  %d", num_dmrs_res, index);

  return index;
}

/*******************************************************************
*
* NAME :         base_sequence_36_or_larger
*
* PARAMETERS :   M_ZC length of Zadoff chu sequence
*                u sequence group number
*                scaling to apply
*
* RETURN :       pointer to generated sequence
*
* DESCRIPTION :  base sequence generation of less than 36 elements
*                5.2.2.1 Base sequences of length 36 or larger
*
*********************************************************************/

static int16_t *base_sequence_36_or_larger(unsigned int Msc_RS,
                                           unsigned int u,
                                           unsigned int v,
                                           unsigned int scaling,
                                           unsigned int if_dmrs_seq)
{
  int16_t *rv_overbar;
  unsigned int N_ZC, M_ZC;
  double q_overbar, x;
  unsigned int q,m,n;
  

  if (if_dmrs_seq)
    M_ZC = dmrs_ul_allocated_res[Msc_RS];
  else
    M_ZC = ul_allocated_re[Msc_RS];
  

  rv_overbar = malloc16(IQ_SIZE*M_ZC);
  if (rv_overbar == NULL) {
    msg("Fatal memory allocation problem \n");
    assert(0);
  }

  if (if_dmrs_seq)
    N_ZC = dmrs_ref_ul_primes[Msc_RS];
  else 
    N_ZC = ref_ul_primes[Msc_RS]; /* The length N_ZC is given by the largest prime number such that N_ZC < M_ZC */
  

  q_overbar = N_ZC * (u+1)/(double)31;

  /*  q = (q_overbar + 1/2) + v.(-1)^(2q_overbar) */
  if ((((int)floor(2*q_overbar))&1) == 0)
    q = (int)(floor(q_overbar+.5)) - v;
  else
    q = (int)(floor(q_overbar+.5)) + v;

  for (n = 0; n < M_ZC; n++) {
    m=n%N_ZC;
    x = (double)q * m * (m+1)/N_ZC;
    rv_overbar[2*n]   =  (int16_t)(floor(scaling*cos(M_PI*x)));   /* cos(-x) = cos(x) */
    rv_overbar[2*n+1] = -(int16_t)(floor(scaling*sin(M_PI*x)));   /* sin(-x) = -sin(x) */
  }
  return rv_overbar;
}

int16_t *rv_ul_ref_sig[U_GROUP_NUMBER][V_BASE_SEQUENCE_NUMBER][SRS_SB_CONF];
int16_t *gNB_dmrs_lowpaprtype1_sequence[U_GROUP_NUMBER][V_BASE_SEQUENCE_NUMBER][MAX_INDEX_DMRS_UL_ALLOCATED_REs];
void generate_lowpapr_typ1_refsig_sequences(unsigned int scaling)
{
  /* prevent multiple calls, relevant when both UE & gNB initialize this */
  static bool already_called = false;
  if (already_called) return;
  already_called = true;

	unsigned int u,Msc_RS;
  unsigned int v = 0; // sequence hopping and group hopping are not supported yet

  for (Msc_RS=0; Msc_RS <= INDEX_SB_LESS_32; Msc_RS++) {  	
    for (u=0; u < U_GROUP_NUMBER; u++) {
      gNB_dmrs_lowpaprtype1_sequence[u][v][Msc_RS] = base_sequence_less_than_36(ul_allocated_re[Msc_RS], u, scaling);
    }
  }

  for (Msc_RS=INDEX_SB_LESS_32+1; Msc_RS < MAX_INDEX_DMRS_UL_ALLOCATED_REs; Msc_RS++) {    
    for (u=0; u < U_GROUP_NUMBER; u++) {        
      gNB_dmrs_lowpaprtype1_sequence[u][v][Msc_RS] = base_sequence_36_or_larger(Msc_RS, u, v, scaling, 1);           
    }    
  } 
}

int16_t *dmrs_lowpaprtype1_ul_ref_sig[U_GROUP_NUMBER][V_BASE_SEQUENCE_NUMBER][MAX_INDEX_DMRS_UL_ALLOCATED_REs];
void generate_ul_reference_signal_sequences(unsigned int scaling)
{
  /* prevent multiple calls, relevant when both UE & gNB initialize this */
  static bool already_called = false;
  if (already_called) return;
  already_called = true;

  unsigned int u,v,Msc_RS;

#if 0

    char output_file[255];
    char sequence_name[255];

#endif

  
  for (Msc_RS=0; Msc_RS <= INDEX_SB_LESS_32; Msc_RS++) {
  	v = 0;
    for (u=0; u < U_GROUP_NUMBER; u++) {
      rv_ul_ref_sig[u][v][Msc_RS] = base_sequence_less_than_36(ul_allocated_re[Msc_RS], u, scaling);
      dmrs_lowpaprtype1_ul_ref_sig[u][v][Msc_RS] = base_sequence_less_than_36(ul_allocated_re[Msc_RS], u, scaling);
    
#if 0
      sprintf(output_file, "rv_seq_%d_%d_%d.m", u, v, ul_allocated_re[Msc_RS]);
      sprintf(sequence_name, "rv_seq_%d_%d_%d.m", u, v, ul_allocated_re[Msc_RS]);
      printf("u %d Msc_RS %d allocate memory %x of size %d \n", u, Msc_RS, rv_ul_ref_sig[u][v][Msc_RS], (IQ_SIZE* ul_allocated_re[Msc_RS]));
      write_output(output_file, sequence_name,  rv_ul_ref_sig[u][v][Msc_RS], ul_allocated_re[Msc_RS], 1, 1);

#endif
    }
  }

 for (Msc_RS=INDEX_SB_LESS_32+1; Msc_RS < MAX_INDEX_DMRS_UL_ALLOCATED_REs; Msc_RS++) {
    v = 0;  // neither group hopping or sequnce hopping is supported for PUSCH DMRS hence v = 0
    for (u=0; u < U_GROUP_NUMBER; u++) {        
      dmrs_lowpaprtype1_ul_ref_sig[u][v][Msc_RS] = base_sequence_36_or_larger(Msc_RS, u, v, scaling, 1);  
    }
    
  }
 
  for (Msc_RS=INDEX_SB_LESS_32+1; Msc_RS < SRS_SB_CONF; Msc_RS++) {
    for (u=0; u < U_GROUP_NUMBER; u++) {
     for (v=0; v < V_BASE_SEQUENCE_NUMBER; v++) {
        rv_ul_ref_sig[u][v][Msc_RS] = base_sequence_36_or_larger(Msc_RS, u, v, scaling, 0);

#if 0
        sprintf(output_file, "rv_seq_%d_%d_%d.m", u, v, ul_allocated_re[Msc_RS]);
        sprintf(sequence_name, "rv_seq_%d_%d_%d.m", u, v, ul_allocated_re[Msc_RS]);
        printf("u %d Msc_RS %d allocate memory %x of size %d \n", u, Msc_RS, rv_ul_ref_sig[u][v][Msc_RS], (IQ_SIZE* ul_allocated_re[Msc_RS]));
        write_output(output_file, sequence_name,  rv_ul_ref_sig[u][v][Msc_RS], ul_allocated_re[Msc_RS], 1, 1);

#endif
      }
    }
  }


}


/*******************************************************************
*
* NAME :         free_ul_reference_signal_sequences
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  free of uplink reference signal sequences
*
*********************************************************************/
void free_ul_reference_signal_sequences(void)
{
  unsigned int u,v,Msc_RS;
  for (Msc_RS=0; Msc_RS < SRS_SB_CONF; Msc_RS++) {
    for (u=0; u < U_GROUP_NUMBER; u++) {
      for (v=0; v < V_BASE_SEQUENCE_NUMBER; v++) {
        if (rv_ul_ref_sig[u][v][Msc_RS])
          free_and_zero(rv_ul_ref_sig[u][v][Msc_RS]);
        if ((v==0) && (Msc_RS < MAX_INDEX_DMRS_UL_ALLOCATED_REs))
          if (dmrs_lowpaprtype1_ul_ref_sig[u][v][Msc_RS])
            free_and_zero(dmrs_lowpaprtype1_ul_ref_sig[u][v][Msc_RS]);
      }
    }
  }
}
/*******************************************************************
*
* NAME :         free_gnb_lowpapr_sequences
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  free of uplink reference signal sequences
*
*********************************************************************/
void free_gnb_lowpapr_sequences(void)
{
  unsigned int u,v,Msc_RS;
  for (Msc_RS=0; Msc_RS < MAX_INDEX_DMRS_UL_ALLOCATED_REs; Msc_RS++) {
    v=0;
    for (u=0; u < U_GROUP_NUMBER; u++) {      
      if (gNB_dmrs_lowpaprtype1_sequence[u][v][Msc_RS])
        free_and_zero(gNB_dmrs_lowpaprtype1_sequence[u][v][Msc_RS]);
    }
  }
}

