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

#define DEFINE_VARIABLES_LOWPAPR_SEQUENCES_NR_H
#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"
#undef DEFINE_VARIABLES_LOWPAPR_SEQUENCES_NR_H

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

int16_t *base_sequence_less_than_36(unsigned int M_ZC,
                                    unsigned int u,
                                    unsigned int scaling)
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

int16_t *base_sequence_36_or_larger(unsigned int Msc_RS,
                                    unsigned int u,
                                    unsigned int v,
                                    unsigned int scaling)
{
  int16_t *rv_overbar;
  unsigned int N_ZC;
  double q_overbar, x;
  unsigned int q,m,n;
  unsigned int M_ZC = ul_allocated_re[Msc_RS];

  rv_overbar = malloc16(IQ_SIZE*M_ZC);
  if (rv_overbar == NULL) {
    msg("Fatal memory allocation problem \n");
    assert(0);
  }

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

/*******************************************************************
*
* NAME :         generate_ul_srs_sequences
*
* PARAMETERS :   scaling to apply
*
* RETURN :       none
*
* DESCRIPTION :  uplink reference signal sequences generation
*                which are Low-PAPR base sequences
*                see TS 38.211 5.2.2 Low-PAPR sequence generation
*
*********************************************************************/

void generate_ul_reference_signal_sequences(unsigned int scaling)
{
	unsigned int u,v,Msc_RS;

#if 0

    char output_file[255];
    char sequence_name[255];

#endif

  for (Msc_RS=0; Msc_RS <= INDEX_SB_LESS_32; Msc_RS++) {
	v = 0;
    for (u=0; u < U_GROUP_NUMBER; u++) {
      rv_ul_ref_sig[u][v][Msc_RS] = base_sequence_less_than_36(ul_allocated_re[Msc_RS], u, scaling);
#if 0
      sprintf(output_file, "rv_seq_%d_%d_%d.m", u, v, ul_allocated_re[Msc_RS]);
      sprintf(sequence_name, "rv_seq_%d_%d_%d.m", u, v, ul_allocated_re[Msc_RS]);
      printf("u %d Msc_RS %d allocate memory %x of size %d \n", u, Msc_RS, rv_ul_ref_sig[u][v][Msc_RS], (IQ_SIZE* ul_allocated_re[Msc_RS]));
      write_output(output_file, sequence_name,  rv_ul_ref_sig[u][v][Msc_RS], ul_allocated_re[Msc_RS], 1, 1);

#endif
    }
  }

  for (Msc_RS=INDEX_SB_LESS_32+1; Msc_RS < SRS_SB_CONF; Msc_RS++) {
    for (u=0; u < U_GROUP_NUMBER; u++) {
      for (v=0; v < V_BASE_SEQUENCE_NUMBER; v++) {
        rv_ul_ref_sig[u][v][Msc_RS] = base_sequence_36_or_larger(Msc_RS, u, v, scaling);
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
          free16(rv_ul_ref_sig[u][v][Msc_RS],2*sizeof(int16_t)*ul_allocated_re[Msc_RS]);
      }
    }
  }
}
