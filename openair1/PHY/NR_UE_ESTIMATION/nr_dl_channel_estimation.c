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


#include <string.h>

//#include "defs.h"
//#include "SCHED/defs.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "filt16a_32.h"
#include "T.h"
//#define DEBUG_PDSCH
//#define DEBUG_PDCCH


int nr_pbch_dmrs_correlation(PHY_VARS_NR_UE *ue,
			       uint8_t eNB_offset,
			       unsigned char Ns,
			       unsigned char symbol,
			       NR_UE_SSB *current_ssb)
{
  int pilot[200] __attribute__((aligned(16)));
  unsigned char aarx;
  unsigned short k;
  unsigned int pilot_cnt;
  int16_t ch[2],*pil,*rxF;
  int symbol_offset;
  int dmrss;

  uint8_t nushift;
  uint8_t ssb_index=current_ssb->i_ssb;
  uint8_t n_hf=current_ssb->n_hf;
  int **rxdataF=ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF;

  nushift =  ue->frame_parms.Nid_cell%4;
  ue->frame_parms.nushift = nushift;
  unsigned int  ssb_offset = ue->frame_parms.first_carrier_offset + ue->frame_parms.ssb_start_subcarrier;
  if (ssb_offset>= ue->frame_parms.ofdm_symbol_size) ssb_offset-=ue->frame_parms.ofdm_symbol_size;

  if (ue->is_synchronized ==0 ) dmrss = symbol-1;
  else dmrss = symbol-5;

  AssertFatal((symbol > 0 && symbol < 4 && ue->is_synchronized == 0) || 
	      (symbol > 4 && symbol < 8 && ue->is_synchronized == 1),
	      "symbol %d is illegal for PBCH DM-RS (is_synchronized %d)\n",
	      symbol,ue->is_synchronized);

  symbol_offset = ue->frame_parms.ofdm_symbol_size*symbol;


  k = nushift;

#ifdef DEBUG_CH
  printf("PBCH DMRS Correlation : ThreadId %d, eNB_offset %d , OFDM size %d, Ncp=%d, Ns=%d, k=%d symbol %d\n",ue->current_thread_id[Ns>>1], eNB_offset,ue->frame_parms.ofdm_symbol_size,
         ue->frame_parms.Ncp,Ns,k, symbol);
#endif

  // generate pilot
  nr_pbch_dmrs_rx(dmrss,ue->nr_gold_pbch[n_hf][ssb_index], &pilot[0]);

  int re_offset = ssb_offset;
  for (aarx=0; aarx<ue->frame_parms.nb_antennas_rx; aarx++) {

    pil   = (int16_t *)&pilot[0];
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];

#ifdef DEBUG_CH
    printf("pbch ch est pilot addr %p RB_DL %d\n",&pilot[0], ue->frame_parms.N_RB_DL);
    printf("k %d, first_carrier %d\n",k,ue->frame_parms.first_carrier_offset);
    printf("rxF addr %p\n", rxF);
#endif
    //if ((ue->frame_parms.N_RB_DL&1)==0) {

    // Treat first 2 pilots specially (left edge)
    ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
    ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

    current_ssb->c_re +=ch[0];
    current_ssb->c_im +=ch[1];

#ifdef DEBUG_CH
    printf("ch 0 %d\n",((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1]));
    printf("pilot 0 : rxF - > (%d,%d) addr %p  ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif

    pil+=2;
    re_offset = (re_offset+4)&(ue->frame_parms.ofdm_symbol_size-1);
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];


    ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
    ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

    current_ssb->c_re +=ch[0];
    current_ssb->c_im +=ch[1];

#ifdef DEBUG_CH
    printf("pilot 1 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

    pil+=2;
    re_offset = (re_offset+4)&(ue->frame_parms.ofdm_symbol_size-1);
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];

    current_ssb->c_re +=ch[0];
    current_ssb->c_im +=ch[1];

#ifdef DEBUG_CH
    printf("pilot 2 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

    pil+=2;
    re_offset = (re_offset+4)&(ue->frame_parms.ofdm_symbol_size-1);
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];

    for (pilot_cnt=3; pilot_cnt<(3*20); pilot_cnt+=3) {

      //	if (pilot_cnt == 30)
      //	  rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k)];

      // in 2nd symbol, skip middle  REs (48 with DMRS,  144 for SSS, and another 48 with DMRS) 
      if (dmrss == 1 && pilot_cnt == 12) {
	pilot_cnt=48;
	re_offset = (re_offset+144)&(ue->frame_parms.ofdm_symbol_size-1);
	rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
      }
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
      
      current_ssb->c_re +=ch[0];
      current_ssb->c_im +=ch[1];

#ifdef DEBUG_CH
      printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

      pil+=2;
      re_offset = (re_offset+4)&(ue->frame_parms.ofdm_symbol_size-1);
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
        
  
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

      current_ssb->c_re +=ch[0];
      current_ssb->c_im +=ch[1];

#ifdef DEBUG_CH
      printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      pil+=2;
      re_offset = (re_offset+4)&(ue->frame_parms.ofdm_symbol_size-1);
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
        

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

      current_ssb->c_re +=ch[0];
      current_ssb->c_im +=ch[1];

#ifdef DEBUG_CH
      printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+2,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

      pil+=2;
      re_offset = (re_offset+4)&(ue->frame_parms.ofdm_symbol_size-1);
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];

    }


    //}

  }
  return(0);
}


int nr_pbch_channel_estimation(PHY_VARS_NR_UE *ue,
			       uint8_t eNB_offset,
			       unsigned char Ns,
			       unsigned char symbol,
			       int dmrss,
			       uint8_t ssb_index,
			       uint8_t n_hf)
{
  int pilot[200] __attribute__((aligned(16)));
  unsigned char aarx;
  unsigned short k;
  unsigned int pilot_cnt;
  int16_t ch[2],*pil,*rxF,*dl_ch,*fl,*fm,*fr;
  int ch_offset,symbol_offset;

  //uint16_t Nid_cell = (eNB_offset == 0) ? ue->frame_parms.Nid_cell : ue->measurements.adj_cell_id[eNB_offset-1];

  uint8_t nushift;
  int **dl_ch_estimates  =ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].dl_ch_estimates[eNB_offset];
  int **rxdataF=ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF;

  nushift =  ue->frame_parms.Nid_cell%4;
  ue->frame_parms.nushift = nushift;
  unsigned int  ssb_offset = ue->frame_parms.first_carrier_offset + ue->frame_parms.ssb_start_subcarrier;
  if (ssb_offset>= ue->frame_parms.ofdm_symbol_size) ssb_offset-=ue->frame_parms.ofdm_symbol_size;

  if (ue->high_speed_flag == 0) // use second channel estimate position for temporary storage
    ch_offset     = ue->frame_parms.ofdm_symbol_size ;
  else
    ch_offset     = ue->frame_parms.ofdm_symbol_size*symbol;

  AssertFatal(dmrss >= 0 && dmrss < 3,
	      "symbol %d is illegal for PBCH DM-RS \n",
	      dmrss);

  symbol_offset = ue->frame_parms.ofdm_symbol_size*symbol;


  k = nushift;

#ifdef DEBUG_CH
  printf("PBCH Channel Estimation : ThreadId %d, eNB_offset %d ch_offset %d, OFDM size %d, Ncp=%d, Ns=%d, k=%d symbol %d\n",ue->current_thread_id[Ns], eNB_offset,ch_offset,ue->frame_parms.ofdm_symbol_size,
         ue->frame_parms.Ncp,Ns,k, symbol);
#endif

  switch (k) {
  case 0:
    fl = filt16a_l0;
    fm = filt16a_m0;
    fr = filt16a_r0;
    break;

  case 1:
    fl = filt16a_l1;
    fm = filt16a_m1;
    fr = filt16a_r1;
    break;

  case 2:
    fl = filt16a_l2;
    fm = filt16a_m2;
    fr = filt16a_r2;
    break;

  case 3:
    fl = filt16a_l3;
    fm = filt16a_m3;
    fr = filt16a_r3;
    break;

  default:
    msg("pbch_channel_estimation: k=%d -> ERROR\n",k);
    return(-1);
    break;
  }

  // generate pilot
  nr_pbch_dmrs_rx(dmrss,ue->nr_gold_pbch[n_hf][ssb_index], &pilot[0]);

  int re_offset = ssb_offset;
  for (aarx=0; aarx<ue->frame_parms.nb_antennas_rx; aarx++) {

    pil   = (int16_t *)&pilot[0];
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
    dl_ch = (int16_t *)&dl_ch_estimates[aarx][ch_offset];

    memset(dl_ch,0,4*(ue->frame_parms.ofdm_symbol_size));
    if (ue->high_speed_flag==0) // multiply previous channel estimate by ch_est_alpha
      multadd_complex_vector_real_scalar(dl_ch-(ue->frame_parms.ofdm_symbol_size<<1),
                                         ue->ch_est_alpha,dl_ch-(ue->frame_parms.ofdm_symbol_size<<1),
                                         1,ue->frame_parms.ofdm_symbol_size);
#ifdef DEBUG_CH
    printf("pbch ch est pilot addr %p RB_DL %d\n",&pilot[0], ue->frame_parms.N_RB_DL);
    printf("k %d, first_carrier %d\n",k,ue->frame_parms.first_carrier_offset);
    printf("rxF addr %p\n", rxF);
    printf("dl_ch addr %p\n",dl_ch);
#endif
    //if ((ue->frame_parms.N_RB_DL&1)==0) {

    // Treat first 2 pilots specially (left edge)
    ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
    ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_CH
    printf("ch 0 %d\n",((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1]));
    printf("pilot 0 : rxF - > (%d,%d) addr %p  ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif
    multadd_real_vector_complex_scalar(fl,
				       ch,
				       dl_ch,
				       16);
    pil+=2;
    re_offset = (re_offset+4)&(ue->frame_parms.ofdm_symbol_size-1);
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];

    //for (int i= 0; i<8; i++)
    //printf("dl_ch addr %p %d\n", dl_ch+i, *(dl_ch+i));

    ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
    ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);


#ifdef DEBUG_CH
    printf("pilot 1 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
    multadd_real_vector_complex_scalar(fm,
				       ch,
				       dl_ch,
				       16);
    pil+=2;
    re_offset = (re_offset+4)&(ue->frame_parms.ofdm_symbol_size-1);
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];

#ifdef DEBUG_CH
    printf("pilot 2 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

    multadd_real_vector_complex_scalar(fr,
				       ch,
				       dl_ch,
				       16);
    pil+=2;
    re_offset = (re_offset+4)&(ue->frame_parms.ofdm_symbol_size-1);
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
    dl_ch+=24;

    for (pilot_cnt=3; pilot_cnt<(3*20); pilot_cnt+=3) {

      //	if (pilot_cnt == 30)
      //	  rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k)];

      // in 2nd symbol, skip middle  REs (48 with DMRS,  144 for SSS, and another 48 with DMRS) 
      if (dmrss == 1 && pilot_cnt == 12) {
	pilot_cnt=48;
	re_offset = (re_offset+144)&(ue->frame_parms.ofdm_symbol_size-1);
	rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
	dl_ch += 288;
      }
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_CH
      printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fl,
					 ch,
					 dl_ch,
					 16);

      //for (int i= 0; i<8; i++)
      //            printf("pilot_cnt %d dl_ch %d %d\n", pilot_cnt, dl_ch+i, *(dl_ch+i));

      pil+=2;
      re_offset = (re_offset+4)&(ue->frame_parms.ofdm_symbol_size-1);
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
        
  
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_CH
      printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fm,
					 ch,
					 dl_ch,
					 16);
      pil+=2;
      re_offset = (re_offset+4)&(ue->frame_parms.ofdm_symbol_size-1);
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
        

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_CH
      printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+2,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

      multadd_real_vector_complex_scalar(fr,
					 ch,
					 dl_ch,
					 16);
      pil+=2;
      re_offset = (re_offset+4)&(ue->frame_parms.ofdm_symbol_size-1);
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
      dl_ch+=24;

    }


    //}

  }
  return(0);
}

int nr_pdcch_channel_estimation(PHY_VARS_NR_UE *ue,
				uint8_t eNB_offset,
				unsigned char Ns,
				unsigned char l,
				unsigned char symbol,
				unsigned short coreset_start_subcarrier,
				unsigned short nb_rb_coreset)
{
  int pilot[200] __attribute__((aligned(16)));
  unsigned char aarx,p;
  unsigned short k;
  unsigned int pilot_cnt;
  int16_t ch[2],*pil,*rxF,*dl_ch,*fl,*fm,*fr;
  int ch_offset,symbol_offset;

  //uint16_t Nid_cell = (eNB_offset == 0) ? ue->frame_parms.Nid_cell : ue->measurements.adj_cell_id[eNB_offset-1];

  uint8_t nushift;
  int **dl_ch_estimates  =ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].dl_ch_estimates[eNB_offset];
  int **rxdataF=ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF;

  nushift = 1;
  ue->frame_parms.nushift = nushift;

  if (ue->high_speed_flag == 0) // use second channel estimate position for temporary storage
    ch_offset     = ue->frame_parms.ofdm_symbol_size ;
  else
    ch_offset     = ue->frame_parms.ofdm_symbol_size*symbol;

  symbol_offset = ue->frame_parms.ofdm_symbol_size*symbol;

  k = coreset_start_subcarrier;

#ifdef DEBUG_PDCCH
  printf("PDCCH Channel Estimation : ThreadId %d, eNB_offset %d ch_offset %d, OFDM size %d, Ncp=%d, l=%d, Ns=%d, k=%d symbol %d\n",ue->current_thread_id[Ns], eNB_offset,ch_offset,ue->frame_parms.ofdm_symbol_size,
         ue->frame_parms.Ncp,l,Ns,k, symbol);
#endif

  fl = filt16a_l1;
  fm = filt16a_m1;
  fr = filt16a_r1;


  // generate pilot 
  nr_pdcch_dmrs_rx(ue,eNB_offset,Ns,ue->nr_gold_pdcch[eNB_offset][Ns][symbol], &pilot[0],2000,nb_rb_coreset);


  for (aarx=0; aarx<ue->frame_parms.nb_antennas_rx; aarx++) {

    pil   = (int16_t *)&pilot[0];
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+nushift)];
    dl_ch = (int16_t *)&dl_ch_estimates[aarx][ch_offset];

    memset(dl_ch,0,4*(ue->frame_parms.ofdm_symbol_size));
    if (ue->high_speed_flag==0) // multiply previous channel estimate by ch_est_alpha
      multadd_complex_vector_real_scalar(dl_ch-(ue->frame_parms.ofdm_symbol_size<<1),
                                         ue->ch_est_alpha,dl_ch-(ue->frame_parms.ofdm_symbol_size<<1),
                                         1,ue->frame_parms.ofdm_symbol_size);
#ifdef DEBUG_PDCCH
    printf("pdcch ch est pilot addr %p RB_DL %d\n",&pilot[0], ue->frame_parms.N_RB_DL);
    printf("k %d, first_carrier %d\n",k,ue->frame_parms.first_carrier_offset);
    printf("rxF addr %p\n", rxF);

    printf("dl_ch addr %p\n",dl_ch);
#endif
    //    if ((ue->frame_parms.N_RB_DL&1)==0) {
      // Treat first 2 pilots specially (left edge)
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDCCH
      printf("ch 0 %d\n",((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1]));
      printf("pilot 0 : rxF - > (%d,%d) addr %p  ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fl,
                                         ch,
                                         dl_ch,
                                         16);
      pil+=2;
      rxF+=8;
      //for (int i= 0; i<8; i++)
      //printf("dl_ch addr %p %d\n", dl_ch+i, *(dl_ch+i));

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDCCH
      printf("pilot 1 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fm,
                                         ch,
                                         dl_ch,
                                         16);
      pil+=2;
      rxF+=8;

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PDCCH
      printf("pilot 2 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

      multadd_real_vector_complex_scalar(fr,
                                         ch,
                                         dl_ch,
                                         16);
                                         
#ifdef DEBUG_PDCCH       
      for (int m =0; m<12; m++)
	printf("data :  dl_ch -> (%d,%d)\n",dl_ch[0+2*m],dl_ch[1+2*m]);
#endif      
      pil+=2;
      rxF+=8;
      dl_ch+=24;
      k+=12;
      
      

      for (pilot_cnt=3; pilot_cnt<(3*nb_rb_coreset); pilot_cnt+=3) {

        if (k >= ue->frame_parms.ofdm_symbol_size){
	  k-=ue->frame_parms.ofdm_symbol_size;
	  rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+nushift)];}

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDCCH
	printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
        multadd_real_vector_complex_scalar(fl,
                                           ch,
                                           dl_ch,
                                           16);

        //for (int i= 0; i<8; i++)
        //            printf("pilot_cnt %d dl_ch %d %d\n", pilot_cnt, dl_ch+i, *(dl_ch+i));

        pil+=2;
        rxF+=8;

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDCCH
	printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
        multadd_real_vector_complex_scalar(fm,
                                           ch,
                                           dl_ch,
                                           16);
        pil+=2;
        rxF+=8;

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PDCCH
	printf("pilot 1 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

        multadd_real_vector_complex_scalar(fr,
                                           ch,
                                           dl_ch,
                                           16);
        pil+=2;
        rxF+=8;
        dl_ch+=24;
        k+=12;

      }


      //}

  }
  
  void (*idft)(int16_t *,int16_t *, int);

  switch (ue->frame_parms.ofdm_symbol_size) {
  case 128:
    idft = idft128;
    break;

  case 256:
    idft = idft256;
    break;

  case 512:
    idft = idft512;
    break;

  case 1024:
    idft = idft1024;
    break;

  case 1536:
    idft = idft1536;
    break;

  case 2048:
    idft = idft2048;
    break;

  default:
    idft = idft512;
    break;
  }

  if( (Ns== 1) && (l == 0))
  {
      // do ifft of channel estimate
      for (aarx=0; aarx<ue->frame_parms.nb_antennas_rx; aarx++)
          for (p=0; p<ue->frame_parms.nb_antenna_ports_eNB; p++) {
              if (ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].dl_ch_estimates[eNB_offset][(p<<1)+aarx])
              {
                  LOG_D(PHY,"Channel Impulse Computation Slot %d ThreadId %d Symbol %d \n", Ns, ue->current_thread_id[Ns], l);
                  idft((int16_t*) &ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].dl_ch_estimates[eNB_offset][(p<<1)+aarx][0],
                          (int16_t*) ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].dl_ch_estimates_time[eNB_offset][(p<<1)+aarx],1);
              }
          }
  }

  return(0);
}

int nr_pdsch_channel_estimation(PHY_VARS_NR_UE *ue,
                              uint8_t eNB_offset,
                              unsigned char Ns,
                              unsigned short p,
                              unsigned char l,
                              unsigned char symbol,
							  unsigned short bwp_start_subcarrier,
							  unsigned short nb_rb_pdsch)
{
  int pilot[1320] __attribute__((aligned(16)));
  unsigned char aarx;
  unsigned short k;
  unsigned int pilot_cnt;
  int16_t ch[2],*pil,*rxF,*dl_ch,*fl,*fm,*fr,*fml,*fmr,*fmm;
  int ch_offset,symbol_offset;

  //uint16_t Nid_cell = (eNB_offset == 0) ? ue->frame_parms.Nid_cell : ue->measurements.adj_cell_id[eNB_offset-1];

  uint8_t nushift;
  int **dl_ch_estimates  =ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].dl_ch_estimates[eNB_offset];
  int **rxdataF=ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF;

  nushift = (p>>1)&1;
  ue->frame_parms.nushift = nushift;

  if (ue->high_speed_flag == 0) // use second channel estimate position for temporary storage
    ch_offset     = ue->frame_parms.ofdm_symbol_size ;
  else
    ch_offset     = ue->frame_parms.ofdm_symbol_size*symbol;

  symbol_offset = ue->frame_parms.ofdm_symbol_size*symbol;

  k = bwp_start_subcarrier;
  int re_offset = k;

#ifdef DEBUG_CH
  printf("PDSCH Channel Estimation : ThreadId %d, eNB_offset %d ch_offset %d, symbol_offset %d OFDM size %d, Ncp=%d, l=%d, Ns=%d, k=%d symbol %d\n",ue->current_thread_id[Ns], eNB_offset,ch_offset,symbol_offset,ue->frame_parms.ofdm_symbol_size,
         ue->frame_parms.Ncp,l,Ns,k, symbol);
#endif

  switch (nushift) {
   case 0:
         fl = filt8_l0;
         fm = filt8_m0;
         fr = filt8_r0;
         fmm = filt8_mm0;
         fml = filt8_m0;
         fmr = filt8_mr0;
         break;

   case 1:
         fl = filt8_l1;
         fm = filt8_m1;
         fr = filt8_r1;
         fmm = filt8_mm1;
         fml = filt8_ml1;
         fmr = filt8_m1;
         break;

   default:
     msg("pdsch_channel_estimation: nushift=%d -> ERROR\n",nushift);
     return(-1);
     break;
   }


  // generate pilot
  nr_pdsch_dmrs_rx(ue,Ns,ue->nr_gold_pdsch[eNB_offset][Ns][0], &pilot[0],1000,0,nb_rb_pdsch);

  for (aarx=0; aarx<ue->frame_parms.nb_antennas_rx; aarx++) {

    pil   = (int16_t *)&pilot[0];
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+nushift)];
    dl_ch = (int16_t *)&dl_ch_estimates[aarx][ch_offset];

    memset(dl_ch,0,4*(ue->frame_parms.ofdm_symbol_size));
    if (ue->high_speed_flag==0) // multiply previous channel estimate by ch_est_alpha
      multadd_complex_vector_real_scalar(dl_ch-(ue->frame_parms.ofdm_symbol_size<<1),
                                         ue->ch_est_alpha,dl_ch-(ue->frame_parms.ofdm_symbol_size<<1),
                                         1,ue->frame_parms.ofdm_symbol_size);
#ifdef DEBUG_PDSCH
    printf("ch est pilot addr %p RB_DL %d\n",&pilot[0], ue->frame_parms.N_RB_DL);
    printf("k %d, first_carrier %d\n",k,ue->frame_parms.first_carrier_offset);
    printf("rxF addr %p p %d\n", rxF,p);
    printf("dl_ch addr %p nushift %d\n",dl_ch,nushift);
#endif
    //if ((ue->frame_parms.N_RB_DL&1)==0) {

      // Treat first 2 pilots specially (left edge)
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDSCH
      printf("ch 0 %d\n",((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1]));
      printf("pilot 0 : rxF - > (%d,%d) addr %p  ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
      printf("data 0 : rxF - > (%d,%d) addr %p  ch -> (%d,%d), pil -> (%d,%d) \n",rxF[2],rxF[3],&rxF[2],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fl,
                                         ch,
                                         dl_ch,
                                         8);
      pil+=2;
      re_offset = (re_offset+2)&(ue->frame_parms.ofdm_symbol_size-1);
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
      //for (int i= 0; i<8; i++)
      //printf("dl_ch addr %p %d\n", dl_ch+i, *(dl_ch+i));

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDSCH
      printf("pilot 1 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fml,
                                         ch,
                                         dl_ch,
                                         8);
      pil+=2;
      re_offset = (re_offset+2)&(ue->frame_parms.ofdm_symbol_size-1);
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
      //printf("dl_ch addr %p\n",dl_ch);
      
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDSCH
      printf("pilot 2 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fmm,
                                         ch,
                                         dl_ch,
                                         8);
                                         
      //for (int i= 0; i<16; i++)
      //printf("dl_ch addr %p %d\n", dl_ch+i, *(dl_ch+i));
      
      pil+=2;
      re_offset = (re_offset+2)&(ue->frame_parms.ofdm_symbol_size-1);
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
      dl_ch+=8;

      for (pilot_cnt=3; pilot_cnt<(6*nb_rb_pdsch-3); pilot_cnt+=2) {
    	//if ((pilot_cnt%6)==0)
    		//dl_ch+=4;
		//printf("re_offset %d\n",re_offset);

        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDSCH
	printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
        multadd_real_vector_complex_scalar(fm,
                                           ch,
                                           dl_ch,
                                           8);

        pil+=2;
        re_offset = (re_offset+2)&(ue->frame_parms.ofdm_symbol_size-1);
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
      
        ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
        ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDSCH
	printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
        multadd_real_vector_complex_scalar(fmm,
                                           ch,
                                           dl_ch,
                                           8);
        pil+=2;
        re_offset = (re_offset+2)&(ue->frame_parms.ofdm_symbol_size-1);
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
        dl_ch+=8;

      }
      
      // Treat first 2 pilots specially (right edge)
	  ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDSCH
	printf("pilot %d : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fm,
                                         ch,
                                         dl_ch,
                                         8);
                                         
      //for (int i= 0; i<8; i++)
      //printf("dl_ch addr %p %d\n", dl_ch+i, *(dl_ch+i));

      pil+=2;
      re_offset = (re_offset+2)&(ue->frame_parms.ofdm_symbol_size-1);
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
             
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDSCH
      printf("ch 0 %d\n",((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1]));
      printf("pilot %d: rxF - > (%d,%d) addr %p  ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fmr,
                                         ch,
                                         dl_ch,
                                         8);
                                         
      pil+=2;
      re_offset = (re_offset+2)&(ue->frame_parms.ofdm_symbol_size-1);
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+nushift+re_offset)];
      dl_ch+=8;
      
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDSCH
      printf("pilot %d: rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+2,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fr,
                                         ch,
                                         dl_ch,
                                         8);
    //}

  }

  return(0);
}

