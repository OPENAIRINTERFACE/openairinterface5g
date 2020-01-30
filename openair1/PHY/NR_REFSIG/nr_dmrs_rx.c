/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file PHY/NR_REFSIG/nr_dl_dmrs.c
 * \brief Top-level routines for generating DMRS from 38-211
 * \author
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email:
 * \note
 * \warning
 */

//#define NR_PBCH_DMRS_LENGTH_DWORD 5
//#define NR_PBCH_DMRS_LENGTH 144
//#define DEBUG_PDCCH

#include "refsig_defs_ue.h"
#include "PHY/defs_nr_UE.h"
#include "nr_refsig.h"
#include "PHY/defs_gNB.h"

/*Table 7.4.1.1.2-1/2 from 38.211 */
int wf1[8][2] = {{1,1},{1,-1},{1,1},{1,-1},{1,1},{1,-1},{1,1},{1,1}};
int wt1[8][2] = {{1,1},{1,1},{1,1},{1,1},{1,-1},{1,-1},{1,-1},{1,-1}};
int wf2[12][2] = {{1,1},{1,-1},{1,1},{1,-1},{1,1},{1,-1},{1,1},{1,1},{1,1},{1,-1},{1,1},{1,1}};
int wt2[12][2] = {{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,-1},{1,-1},{1,-1},{1,-1},{1,-1},{1,-1}};


short nr_rx_mod_table[14]  = {0,0,23170,-23170,-23170,23170,23170,-23170,23170,23170,-23170,-23170,-23170,23170};
short nr_rx_nmod_table[14] = {0,0,-23170,23170,23170,-23170,-23170,23170,-23170,-23170,23170,23170,23170,-23170};


int nr_pusch_dmrs_rx(PHY_VARS_gNB *gNB,
                     unsigned int Ns,
                     unsigned int *nr_gold_pusch,
                     int32_t *output,
                     unsigned short p,
                     unsigned char lp,
                     unsigned short nb_pusch_rb,
                     uint8_t dmrs_type)
{
  int8_t w;
  short *mod_table;
  unsigned char idx=0;

  typedef int array_of_w[2];
  array_of_w *wf;
  array_of_w *wt;

  wf = (dmrs_type==pusch_dmrs_type1) ? wf1 : wf2;
  wt = (dmrs_type==pusch_dmrs_type1) ? wt1 : wt2;

  if (dmrs_type > 2)
    LOG_E(PHY,"Bad PUSCH DMRS config type %d\n", dmrs_type);

  if ((p>=1000) && (p<((dmrs_type==pusch_dmrs_type1) ? 1008 : 1012))) {
      if (gNB->frame_parms.Ncp == NORMAL) {

        for (int i=0; i<nb_pusch_rb*((dmrs_type==pusch_dmrs_type1) ? 6:4); i++) {

          w = (wf[p-1000][i&1])*(wt[p-1000][lp]);
          mod_table = (w==1) ? nr_rx_mod_table : nr_rx_nmod_table;

          idx = ((((nr_gold_pusch[(i<<1)>>5])>>((i<<1)&0x1f))&1)<<1) ^ (((nr_gold_pusch[((i<<1)+1)>>5])>>(((i<<1)+1)&0x1f))&1);
        ((int16_t*)output)[i<<1] = mod_table[(NR_MOD_TABLE_QPSK_OFFSET + idx)<<1];
        ((int16_t*)output)[(i<<1)+1] = mod_table[((NR_MOD_TABLE_QPSK_OFFSET + idx)<<1) + 1];
#ifdef DEBUG_PUSCH
        printf("nr_pusch_dmrs_rx dmrs config type %d port %d nb_pusch_rb %d\n", dmrs_type, p, nb_pusch_rb);
        printf("wf[%d] = %d wt[%d]= %d\n", i&1, wf[p-1000][i&1], lp, wt[p-1000][lp]);
        printf("i %d idx %d pusch gold %u b0-b1 %d-%d mod_dmrs %d %d\n", i, idx, nr_gold_pusch[(i<<1)>>5], (((nr_gold_pusch[(i<<1)>>5])>>((i<<1)&0x1f))&1),
            (((nr_gold_pusch[((i<<1)+1)>>5])>>(((i<<1)+1)&0x1f))&1), ((int16_t*)output)[i<<1], ((int16_t*)output)[(i<<1)+1]);
#endif
        }
      } else {
        LOG_E(PHY,"extended cp not supported for PUSCH DMRS yet\n");
      }
  } else {
    LOG_E(PHY,"Illegal p %d PUSCH DMRS port\n",p);
  }

  return(0);
}



int nr_pdsch_dmrs_rx(PHY_VARS_NR_UE *ue,
                     unsigned int Ns,
                     unsigned int *nr_gold_pdsch,
                     int32_t *output,
                     unsigned short p,
                     unsigned char lp,
                     unsigned short nb_pdsch_rb)
{
  int8_t w,config_type;
  short *mod_table;
  unsigned char idx=0;

  typedef int array_of_w[2];
  array_of_w *wf;
  array_of_w *wt;

  config_type = 0; //to be updated by higher layer

  wf = (config_type==0) ? wf1 : wf2;
  wt = (config_type==0) ? wt1 : wt2;

  if (config_type > 1)
    LOG_E(PHY,"Bad PDSCH DMRS config type %d\n", config_type);

  if ((p>=1000) && (p<((config_type==0) ? 1008 : 1012))) {
      if (ue->frame_parms.Ncp == NORMAL) {

        for (int i=0; i<nb_pdsch_rb*((config_type==0) ? 6:4); i++) {

        	w = (wf[p-1000][i&1])*(wt[p-1000][lp]);
        	mod_table = (w==1) ? nr_rx_mod_table : nr_rx_nmod_table;

        	idx = ((((nr_gold_pdsch[(i<<1)>>5])>>((i<<1)&0x1f))&1)<<1) ^ (((nr_gold_pdsch[((i<<1)+1)>>5])>>(((i<<1)+1)&0x1f))&1);
    		((int16_t*)output)[i<<1] = mod_table[(NR_MOD_TABLE_QPSK_OFFSET + idx)<<1];
    		((int16_t*)output)[(i<<1)+1] = mod_table[((NR_MOD_TABLE_QPSK_OFFSET + idx)<<1) + 1];
#ifdef DEBUG_PDSCH
    		printf("nr_pdsch_dmrs_rx dmrs config type %d port %d nb_pdsch_rb %d\n", config_type, p, nb_pdsch_rb);
    		printf("wf[%d] = %d wt[%d]= %d\n", i&1, wf[p-1000][i&1], lp, wt[p-1000][lp]);
    		printf("i %d idx %d pdsch gold %u b0-b1 %d-%d mod_dmrs %d %d\n", i, idx, nr_gold_pdsch[(i<<1)>>5], (((nr_gold_pdsch[(i<<1)>>5])>>((i<<1)&0x1f))&1),
    				(((nr_gold_pdsch[((i<<1)+1)>>5])>>(((i<<1)+1)&0x1f))&1), ((int16_t*)output)[i<<1], ((int16_t*)output)[(i<<1)+1]);
#endif
       	}
      } else {
        LOG_E(PHY,"extended cp not supported for PDSCH DMRS yet\n");
      }
  } else {
    LOG_E(PHY,"Illegal p %d PDSCH DMRS port\n",p);
  }

  return(0);
}


int nr_pdcch_dmrs_rx(PHY_VARS_NR_UE *ue,
                     uint8_t eNB_offset,
                     unsigned int Ns,
                     unsigned int *nr_gold_pdcch,
                     int32_t *output,
                     unsigned short p,
                     unsigned short nb_rb_coreset)
{
  uint8_t idx=0;
  //uint8_t pdcch_rb_offset =0;
  //nr_gold_pdcch += ((int)floor(ue->frame_parms.ssb_start_subcarrier/12)+pdcch_rb_offset)*3/32;

  if (p==2000) {
    for (int i=0; i<((nb_rb_coreset*6)>>1); i++) {
      idx = ((((nr_gold_pdcch[(i<<1)>>5])>>((i<<1)&0x1f))&1)<<1) ^ (((nr_gold_pdcch[((i<<1)+1)>>5])>>(((i<<1)+1)&0x1f))&1);
      ((int16_t*)output)[i<<1] = nr_rx_mod_table[(NR_MOD_TABLE_QPSK_OFFSET + idx)<<1];
      ((int16_t*)output)[(i<<1)+1] = nr_rx_mod_table[((NR_MOD_TABLE_QPSK_OFFSET + idx)<<1) + 1];
#ifdef DEBUG_PDCCH
      if (i<8)
        printf("i %d idx %d pdcch gold %u b0-b1 %d-%d mod_dmrs %d %d addr %p\n", i, idx, nr_gold_pdcch[(i<<1)>>5], (((nr_gold_pdcch[(i<<1)>>5])>>((i<<1)&0x1f))&1),
               (((nr_gold_pdcch[((i<<1)+1)>>5])>>(((i<<1)+1)&0x1f))&1), ((int16_t*)output)[i<<1], ((int16_t*)output)[(i<<1)+1],&output[0]);
#endif
    }
  }

  return(0);
}


int nr_pbch_dmrs_rx(int symbol,
                    unsigned int *nr_gold_pbch,
                    int32_t *output)
{
  int m,m0,m1;
  uint8_t idx=0;
  AssertFatal(symbol>=0 && symbol <3,"illegal symbol %d\n",symbol);
  if (symbol == 0) {
    m0=0;
    m1=60;
  }
  else if (symbol == 1) {
    m0=60;
    m1=84;
  }
  else {
    m0=84;
    m1=144;
  }
  //    printf("Generating pilots symbol %d, m0 %d, m1 %d\n",symbol,m0,m1);
  /// QPSK modulation
  for (m=m0; m<m1; m++) {
    idx = ((((nr_gold_pbch[(m<<1)>>5])>>((m<<1)&0x1f))&1)<<1) ^ (((nr_gold_pbch[((m<<1)+1)>>5])>>(((m<<1)+1)&0x1f))&1);
    ((int16_t*)output)[(m-m0)<<1] = nr_rx_mod_table[(NR_MOD_TABLE_QPSK_OFFSET + idx)<<1];
    ((int16_t*)output)[((m-m0)<<1)+1] = nr_rx_mod_table[((NR_MOD_TABLE_QPSK_OFFSET + idx)<<1) + 1];
    
#ifdef DEBUG_PBCH
    if (m<16)
      {printf("nr_gold_pbch[(m<<1)>>5] %x\n",nr_gold_pbch[(m<<1)>>5]);
	printf("m %d  output %d %d addr %p\n", m, ((int16_t*)output)[m<<1], ((int16_t*)output)[(m<<1)+1],&output[0]);
      }
#endif
  }
  
  return(0);
}
