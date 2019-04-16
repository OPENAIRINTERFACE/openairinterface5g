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

/*! \file PHY/NR_UE_TRANSPORT/nr_ulsch.c
* \brief Top-level routines for transmission of the PUSCH TS 38.211 v 15.4.0
* \author Khalid Ahmed
* \date 2019
* \version 0.1
* \company Fraunhofer IIS
* \email: khalid.ahmed@iis.fraunhofer.de
* \note
* \warning
*/
#include <stdint.h>
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "common/utils/assertions.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/defs_nr_common.h"
#include "PHY/TOOLS/tools_defs.h"

//#define DEBUG_SCFDMA

#ifdef DEBUG_SCFDMA

  FILE *debug_scfdma;

#endif



void nr_pusch_codeword_scrambling(uint8_t *in,
                         uint16_t size,
                         uint32_t Nid,
                         uint32_t n_RNTI,
                         uint32_t* out) {

  uint8_t reset, b_idx;
  uint32_t x1, x2, s=0, temp_out;

  reset = 1;
  x2 = (n_RNTI<<15) + Nid;

  for (int i=0; i<size; i++) {
    b_idx = i&0x1f;
    if (b_idx==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
      if (i)
        out++;
    }
    if (in[i]==NR_PUSCH_x)
      *out ^= 1<<b_idx;
    else if (in[i]==NR_PUSCH_y){
      if (b_idx!=0)
        *out ^= (*out & (1<<(b_idx-1)))<<1;
      else{

        temp_out = *(out-1);
        *out ^= temp_out>>31;

      }
    }
    else
      *out ^= (((in[i])&1) ^ ((s>>b_idx)&1))<<b_idx;
    //printf("i %d b_idx %d in %d s 0x%08x out 0x%08x\n", i, b_idx, in[i], s, *out);
  }

}


void pusch_transform_precoding(NR_UE_ULSCH_t *ulsch, NR_DL_FRAME_PARMS *frame_parms, int harq_pid){

  NR_UL_UE_HARQ_t *harq_process;
  int16_t x[8192] = {0}; // 8192 is the maximum number of fft bins
  uint32_t *dmod;
  int sc, pusch_symb, pusch_sc;
  int symb, k, l, num_mod_symb;

  harq_process = ulsch->harq_processes[harq_pid];

#ifdef DEBUG_SCFDMA
  debug_scfdma = fopen("debug_scfdma.txt","w");
#endif

  dmod = ulsch->d_mod;
  pusch_symb = ulsch->Nsymb_pusch;
  pusch_sc = ulsch->Nsc_pusch;
  num_mod_symb = harq_process->num_of_mod_symbols;

  void (*dft)(int16_t *,int16_t *, int);

  switch (frame_parms->ofdm_symbol_size) {
  case 128:
    dft = dft128;
    break;

  case 256:
    dft = dft256;
    break;

  case 512:
    dft = dft512;
    break;

  case 1024:
    dft = dft1024;
    break;

  case 1536:
    dft = dft1536;
    break;

  case 2048:
    dft = dft2048;
    break;

  case 4096:
    dft = dft4096;
    break;

  case 8192:
    dft = dft8192;
    break;

  default:
    dft = dft512;
    break;
  }

  k = 0;
  symb = 0;

  for(l = 0; l < pusch_symb; l++){

    for (sc = 0; sc < pusch_sc; sc++){

      x[sc*2] = (symb<num_mod_symb)?(AMP*((int16_t *)dmod)[symb*2])>>15:0;
      x[sc*2 + 1] = (symb<num_mod_symb)?(AMP*((int16_t *)dmod)[symb*2 + 1])>>15:0;

  #ifdef DEBUG_SCFDMA
      fprintf(debug_scfdma, "x[%d] = %d\n", symb*2, x[sc*2] );
      fprintf(debug_scfdma, "x[%d] = %d\n", symb*2 + 1, x[sc*2 + 1] );
  #endif

      symb++;

    }


    dft(x, (int16_t *)&ulsch->y[l*pusch_sc], 1);

  }

#ifdef DEBUG_SCFDMA

  for (symb = 0; symb < num_mod_symb; symb++)
  {
    fprintf(debug_scfdma, "ulsch->y[%d] = %d\n", symb*2, ((int16_t *)ulsch->y)[symb*2] );
    fprintf(debug_scfdma, "ulsch->y[%d] = %d\n", symb*2 + 1, ((int16_t *)ulsch->y)[symb*2 + 1] );
  }

  fclose(debug_scfdma);
#endif

}
