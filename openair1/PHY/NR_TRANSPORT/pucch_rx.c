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

/*! \file PHY/NR_TRANSPORT/pucch_rx.c
 * \brief Top-level routines for decoding the PUCCH physical channel
 * \author A. Mico Pereperez, Padarthi Naga Prasanth, Francesco Mani, Raymond Knopp
 * \date 2020
 * \version 0.2
 * \company Eurecom
 * \email:
 * \note
 * \warning
 */
#include<stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "PHY/impl_defs_nr.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_gNB.h"
#include "PHY/sse_intrin.h"
#include "PHY/NR_UE_TRANSPORT/pucch_nr.h"
#include <openair1/PHY/CODING/nrSmallBlock/nr_small_block_defs.h>
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/NR_REFSIG/nr_refsig.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "T.h"

//#define DEBUG_NR_PUCCH_RX 1

NR_gNB_PUCCH_t *new_gNB_pucch(void){
    NR_gNB_PUCCH_t *pucch;
    pucch = (NR_gNB_PUCCH_t *)malloc16(sizeof(NR_gNB_PUCCH_t));
    pucch->active = 0;
    return (pucch);
}

int nr_find_pucch(uint16_t rnti,
                  int frame,
                  int slot,
                  PHY_VARS_gNB *gNB) {

  AssertFatal(gNB!=NULL,"gNB is null\n");
  int index = -1;

  for (int i=0; i<NUMBER_OF_NR_PUCCH_MAX; i++) {
    AssertFatal(gNB->pucch[i]!=NULL,"gNB->pucch[%d] is null\n",i);
    if ((gNB->pucch[i]->active >0) &&
        (gNB->pucch[i]->pucch_pdu.rnti==rnti) &&
        (gNB->pucch[i]->frame==frame) &&
        (gNB->pucch[i]->slot==slot)) return(i);
    else if ((gNB->pucch[i]->active == 0) && (index==-1)) index=i;
  }

  if (index==-1)
    LOG_E(MAC,"PUCCH list is full\n");

  return(index);
}

void nr_fill_pucch(PHY_VARS_gNB *gNB,
                   int frame,
                   int slot,
                   nfapi_nr_pucch_pdu_t *pucch_pdu) {

  int id = nr_find_pucch(pucch_pdu->rnti,frame,slot,gNB);
  AssertFatal( (id>=0) && (id<NUMBER_OF_NR_PUCCH_MAX),
              "invalid id found for pucch !!! rnti %04x id %d\n",pucch_pdu->rnti,id);

  NR_gNB_PUCCH_t  *pucch = gNB->pucch[id];
  pucch->frame = frame;
  pucch->slot = slot;
  pucch->active = 1;
  memcpy((void*)&pucch->pucch_pdu, (void*)pucch_pdu, sizeof(nfapi_nr_pucch_pdu_t));
}


int get_pucch0_cs_lut_index(PHY_VARS_gNB *gNB,nfapi_nr_pucch_pdu_t* pucch_pdu) {

  int i=0;

#ifdef DEBUG_NR_PUCCH_RX
  printf("getting index for LUT with %d entries, Nid %d\n",gNB->pucch0_lut.nb_id, pucch_pdu->hopping_id);
#endif

  for (i=0;i<gNB->pucch0_lut.nb_id;i++) {
    if (gNB->pucch0_lut.Nid[i] == pucch_pdu->hopping_id) break;
  }
#ifdef DEBUG_NR_PUCCH_RX
  printf("found index %d\n",i);
#endif
  if (i<gNB->pucch0_lut.nb_id) return(i);

#ifdef DEBUG_NR_PUCCH_RX
  printf("Initializing PUCCH0 LUT index %i with Nid %d\n",i, pucch_pdu->hopping_id);
#endif
  // initialize
  gNB->pucch0_lut.Nid[gNB->pucch0_lut.nb_id]=pucch_pdu->hopping_id;
  for (int slot=0;slot<10<<pucch_pdu->subcarrier_spacing;slot++)
    for (int symbol=0;symbol<14;symbol++)
      gNB->pucch0_lut.lut[gNB->pucch0_lut.nb_id][slot][symbol] = (int)floor(nr_cyclic_shift_hopping(pucch_pdu->hopping_id,0,0,symbol,0,slot)/0.5235987756);
  gNB->pucch0_lut.nb_id++;
  return(gNB->pucch0_lut.nb_id-1);
}


  
int16_t idft12_re[12][12] = {
  {23170,23170,23170,23170,23170,23170,23170,23170,23170,23170,23170,23170},
  {23170,20066,11585,0,-11585,-20066,-23170,-20066,-11585,0,11585,20066},
  {23170,11585,-11585,-23170,-11585,11585,23170,11585,-11585,-23170,-11585,11585},
  {23170,0,-23170,0,23170,0,-23170,0,23170,0,-23170,0},
  {23170,-11585,-11585,23170,-11585,-11585,23170,-11585,-11585,23170,-11585,-11585},
  {23170,-20066,11585,0,-11585,20066,-23170,20066,-11585,0,11585,-20066},
  {23170,-23170,23170,-23170,23170,-23170,23170,-23170,23170,-23170,23170,-23170},
  {23170,-20066,11585,0,-11585,20066,-23170,20066,-11585,0,11585,-20066},
  {23170,-11585,-11585,23170,-11585,-11585,23170,-11585,-11585,23170,-11585,-11585},
  {23170,0,-23170,0,23170,0,-23170,0,23170,0,-23170,0},
  {23170,11585,-11585,-23170,-11585,11585,23170,11585,-11585,-23170,-11585,11585},
  {23170,20066,11585,0,-11585,-20066,-23170,-20066,-11585,0,11585,20066}
};

int16_t idft12_im[12][12] = {
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,11585,20066,23170,20066,11585,0,-11585,-20066,-23170,-20066,-11585},
  {0,20066,20066,0,-20066,-20066,0,20066,20066,0,-20066,-20066},
  {0,23170,0,-23170,0,23170,0,-23170,0,23170,0,-23170},
  {0,20066,-20066,0,20066,-20066,0,20066,-20066,0,20066,-20066},
  {0,11585,-20066,23170,-20066,11585,0,-11585,20066,-23170,20066,-11585},
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,-11585,20066,-23170,20066,-11585,0,11585,-20066,23170,-20066,11585},
  {0,-20066,20066,0,-20066,20066,0,-20066,20066,0,-20066,20066},
  {0,-23170,0,23170,0,-23170,0,23170,0,-23170,0,23170},
  {0,-20066,-20066,0,20066,20066,0,-20066,-20066,0,20066,20066},
  {0,-11585,-20066,-23170,-20066,-11585,0,11585,20066,23170,20066,11585}
};


void nr_decode_pucch0(PHY_VARS_gNB *gNB,
                      int frame,
                      int slot,
                      nfapi_nr_uci_pucch_pdu_format_0_1_t* uci_pdu,
                      nfapi_nr_pucch_pdu_t* pucch_pdu) {

  int32_t **rxdataF = gNB->common_vars.rxdataF;
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;

  int nr_sequences;
  const uint8_t *mcs;

  pucch_GroupHopping_t pucch_GroupHopping = pucch_pdu->group_hop_flag + (pucch_pdu->sequence_hop_flag<<1);

  AssertFatal(pucch_pdu->bit_len_harq > 0 || pucch_pdu->sr_flag > 0,
	      "Either bit_len_harq (%d) or sr_flag (%d) must be > 0\n",
	      pucch_pdu->bit_len_harq,pucch_pdu->sr_flag);

  NR_gNB_UCI_STATS_t *uci_stats=NULL;
  NR_gNB_UCI_STATS_t *first_uci_stats=NULL;
  for (int i=0;i<NUMBER_OF_NR_UCI_STATS_MAX;i++)
     if (gNB->uci_stats[i].rnti == pucch_pdu->rnti) {
        uci_stats = &gNB->uci_stats[i];
        break;
     } else if (first_uci_stats == NULL && gNB->uci_stats[i].rnti == 0) first_uci_stats = &gNB->uci_stats[i];

  if (uci_stats == NULL) { uci_stats=first_uci_stats; uci_stats->rnti = pucch_pdu->rnti;}

  AssertFatal(uci_stats!=NULL,"No stat index found\n");
  uci_stats->frame = frame;

  if(pucch_pdu->bit_len_harq==0){
    mcs=table1_mcs;
    nr_sequences=1;
  }
  else if(pucch_pdu->bit_len_harq==1){
    mcs=table1_mcs;
    nr_sequences=4>>(1-pucch_pdu->sr_flag);
  }
  else{
    mcs=table2_mcs;
    nr_sequences=8>>(1-pucch_pdu->sr_flag);
  }

  LOG_I(PHY,"pucch0: nr_symbols %d, start_symbol %d, prb_start %d, second_hop_prb %d,  group_hop_flag %d, sequence_hop_flag %d, O_ACK %d, O_SR %d, mcs %d initial_cyclic_shift %d\n",pucch_pdu->nr_of_symbols,pucch_pdu->start_symbol_index,pucch_pdu->prb_start,pucch_pdu->second_hop_prb,pucch_pdu->group_hop_flag,pucch_pdu->sequence_hop_flag,pucch_pdu->bit_len_harq,pucch_pdu->sr_flag,mcs[0],pucch_pdu->initial_cyclic_shift);

  int cs_ind = get_pucch0_cs_lut_index(gNB,pucch_pdu);
  /*
   * Implement TS 38.211 Subclause 6.3.2.3.1 Sequence generation
   *
   */
  /*
   * Defining cyclic shift hopping TS 38.211 Subclause 6.3.2.2.2
   */
  // alpha is cyclic shift
  //double alpha;
  // lnormal is the OFDM symbol number in the PUCCH transmission where l=0 corresponds to the first OFDM symbol of the PUCCH transmission
  //uint8_t lnormal;
  // lprime is the index of the OFDM symbol in the slot that corresponds to the first OFDM symbol of the PUCCH transmission in the slot given by [5, TS 38.213]
  //uint8_t lprime;

  /*
   * in TS 38.213 Subclause 9.2.1 it is said that:
   * for PUCCH format 0 or PUCCH format 1, the index of the cyclic shift
   * is indicated by higher layer parameter PUCCH-F0-F1-initial-cyclic-shift
   */

  /*
   * Implementing TS 38.211 Subclause 6.3.2.3.1, the sequence x(n) shall be generated according to:
   * x(l*12+n) = r_u_v_alpha_delta(n)
   */
  // the value of u,v (delta always 0 for PUCCH) has to be calculated according to TS 38.211 Subclause 6.3.2.2.1
  uint8_t u[2]={0,0},v[2]={0,0};

  // x_n contains the sequence r_u_v_alpha_delta(n)

  int n,i,l;
  int prb_offset[2] = {pucch_pdu->bwp_start+pucch_pdu->prb_start, pucch_pdu->bwp_start+pucch_pdu->prb_start};

  nr_group_sequence_hopping(pucch_GroupHopping,pucch_pdu->hopping_id,0,slot,&u[0],&v[0]); // calculating u and v value first hop
  LOG_I(PHY,"pucch0: cs_ind %d, u %d, v %d\n",cs_ind,u[0],v[0]);


  if (pucch_pdu->freq_hop_flag == 1) {
    nr_group_sequence_hopping(pucch_GroupHopping,pucch_pdu->hopping_id,1,slot,&u[1],&v[1]); // calculating u and v value second hop
    LOG_I(PHY,"pucch0 second hop: u %d, v %d\n",u[1],v[1]);
    prb_offset[1] = pucch_pdu->bwp_start+pucch_pdu->second_hop_prb;
  }


  AssertFatal(pucch_pdu->nr_of_symbols < 3,"nr_of_symbols %d not allowed\n",pucch_pdu->nr_of_symbols);
  uint32_t re_offset[2]={0,0};
  uint8_t l2;

  const int16_t *x_re[2],*x_im[2];
  x_re[0] = table_5_2_2_2_2_Re[u[0]];
  x_im[0] = table_5_2_2_2_2_Im[u[0]];
  x_re[1] = table_5_2_2_2_2_Re[u[1]];
  x_im[1] = table_5_2_2_2_2_Im[u[1]];

  int16_t xr[1+frame_parms->nb_antennas_rx][1+pucch_pdu->nr_of_symbols][24]  __attribute__((aligned(32)));
  int64_t xrtmag=0;
  uint8_t maxpos=0;
  uint8_t index=0;
  for (l=0; l<pucch_pdu->nr_of_symbols; l++) {
    for (int aarx=0;aarx<frame_parms->nb_antennas_rx;aarx++) {
      memset((void*)xr[aarx][l],0,24*sizeof(int16_t));
    }
  }
  int n2;

  for (l=0; l<pucch_pdu->nr_of_symbols; l++) {
    l2 = l+pucch_pdu->start_symbol_index;
    re_offset[l] = (12*prb_offset[l]) + frame_parms->first_carrier_offset;
    if (re_offset[l]>= frame_parms->ofdm_symbol_size)
      re_offset[l]-=frame_parms->ofdm_symbol_size;
  
    AssertFatal(re_offset[l]+12 < frame_parms->ofdm_symbol_size,"pucch straddles DC carrier, handle this!\n");
    int16_t *r;
    for (int aa=0;aa<frame_parms->nb_antennas_rx;aa++) {
      r=(int16_t*)&rxdataF[aa][(l2*frame_parms->ofdm_symbol_size)+re_offset[l]];
      n2=0;
      for (n=0;n<12;n++,n2+=2) {
        xr[aa][l][n2]  +=(int16_t)(((int32_t)x_re[l][n]*r[n2]+(int32_t)x_im[l][n]*r[n2+1])>>15);
        xr[aa][l][n2+1]+=(int16_t)(((int32_t)x_re[l][n]*r[n2+1]-(int32_t)x_im[l][n]*r[n2])>>15);
#ifdef DEBUG_NR_PUCCH_RX
        printf("x (%d,%d), r%d.%d (%d,%d), xr (%d,%d)\n",
	               x_re[l][n],x_im[l][n],l2,re_offset[l],r[n2],r[n2+1],xr[aa][l][n2],xr[aa][l][n2+1]);
#endif
      }
    }
  }

  int32_t corr_re[1+frame_parms->nb_antennas_rx][2];
  int32_t corr_im[1+frame_parms->nb_antennas_rx][2];
  //int32_t no_corr = 0;
  int seq_index;
  int64_t temp;
  int64_t av_corr=0;

  for(i=0;i<nr_sequences;i++){

    for (l=0;l<pucch_pdu->nr_of_symbols;l++) {
      seq_index = (pucch_pdu->initial_cyclic_shift+
		   mcs[i]+
		   gNB->pucch0_lut.lut[cs_ind][slot][l+pucch_pdu->start_symbol_index])%12;
#ifdef DEBUG_NR_PUCCH_RX
      printf("PUCCH symbol %d seq %d, seq_index %d, mcs %d\n",l,i,seq_index,mcs[i]);
#endif
      for (int aa=0;aa<frame_parms->nb_antennas_rx;aa++) {
        corr_re[aa][l]=0;
        corr_im[aa][l]=0;
        
        n2=0;
        for (n=0;n<12;n++,n2+=2) {
          corr_re[aa][l]+=(xr[aa][l][n2]*idft12_re[seq_index][n]+xr[aa][l][n2+1]*idft12_im[seq_index][n])>>15;
          corr_im[aa][l]+=(xr[aa][l][n2]*idft12_im[seq_index][n]-xr[aa][l][n2+1]*idft12_re[seq_index][n])>>15;
        }
      }
    }
#ifdef DEBUG_NR_PUCCH_RX
    LOG_I(PHY,"PUCCH IDFT = (%d,%d)=>%f\n",corr_re[0][0],corr_im[0][0],10*log10((double)corr_re[0][0]*corr_re[0][0] + (double)corr_im[0][0]*corr_im[0][0]));
    if (l>1) LOG_I(PHY,"PUCCH 2nd symbol IDFT[%d/%d] = (%d,%d)=>%f\n",mcs[i],seq_index,corr_re[0][1],corr_im[0][1],10*log10((double)corr_re[0][1]*corr_re[0][1] + (double)corr_im[0][1]*corr_im[0][1]));
#endif
    if (pucch_pdu->freq_hop_flag == 0 && l==1) {// non-coherent correlation
      temp=0;
      for (int aa=0;aa<frame_parms->nb_antennas_rx;aa++) 
        temp+=(int64_t)corr_re[aa][0]*corr_re[aa][0] + (int64_t)corr_im[aa][0]*corr_im[aa][0];
    }
    else if (pucch_pdu->freq_hop_flag == 0 && l==2) {
      int64_t corr_re2=0;
      int64_t corr_im2=0;
      temp=0;
      for (int aa=0;aa<frame_parms->nb_antennas_rx;aa++) {
        corr_re2 = (int64_t)corr_re[aa][0]+corr_re[aa][1];
        corr_im2 = (int64_t)corr_im[aa][0]+corr_im[aa][1];
      // coherent combining of 2 symbols and then complex modulus for single-frequency case
        temp+=corr_re2*corr_re2 + corr_im2*corr_im2;
      }
    }
    else if (pucch_pdu->freq_hop_flag == 1) {
      // full non-coherent combining of 2 symbols for frequency-hopping case
      temp=0;
      for (int aa=0;aa<frame_parms->nb_antennas_rx;aa++)
        temp += (int64_t)corr_re[aa][0]*corr_re[aa][0] + (int64_t)corr_im[aa][0]*corr_im[aa][0] + (int64_t)corr_re[aa][1]*corr_re[aa][1] + (int64_t)corr_im[aa][1]*corr_im[aa][1];
    }
    else AssertFatal(1==0,"shouldn't happen\n");

    av_corr+=temp;
    if (temp>xrtmag) {
      xrtmag=temp;
      maxpos=i;
      uci_stats->current_pucch0_stat0 = 0;
      int64_t temp2=0,temp3=0;;
      for (int aa=0;aa<frame_parms->nb_antennas_rx;aa++) {
        temp2 += ((int64_t)corr_re[aa][0]*corr_re[aa][0] + (int64_t)corr_im[aa][0]*corr_im[aa][0]);
        if (l==2) temp3 += ((int64_t)corr_re[aa][1]*corr_re[aa][1] + (int64_t)corr_im[aa][1]*corr_im[aa][1]);
      }
      uci_stats->current_pucch0_stat0= dB_fixed64(temp2);
      if (l==2) uci_stats->current_pucch0_stat1= dB_fixed64(temp3);
    }
  }

  av_corr/=nr_sequences/l;

  uint8_t xrtmag_dB = dB_fixed64(xrtmag/frame_parms->nb_antennas_rx);
 
#ifdef DEBUG_NR_PUCCH_RX
  printf("PUCCH 0 : maxpos %d\n",maxpos);
#endif

  index=maxpos;
  uci_stats->pucch0_n00 = gNB->measurements.n0_subband_power_tot_dB[prb_offset[0]];
  uci_stats->pucch0_n01 = gNB->measurements.n0_subband_power_tot_dB[prb_offset[1]];
  LOG_D(PHY,"n00[%d] = %d, n01[%d] = %d\n",prb_offset[0],uci_stats->pucch0_n00,prb_offset[1],uci_stats->pucch0_n01);
  // estimate CQI for MAC (from antenna port 0 only)
  int max_n0 = uci_stats->pucch0_n00>uci_stats->pucch0_n01 ? uci_stats->pucch0_n00:uci_stats->pucch0_n01;
  int SNRtimes10 = dB_fixed_times10(signal_energy_nodc(&rxdataF[0][pucch_pdu->start_symbol_index*frame_parms->ofdm_symbol_size+re_offset[0]],12)) - (10*max_n0);
  int cqi;
  if (SNRtimes10 < -640) cqi=0;
  else if (SNRtimes10 >  635) cqi=255;
  else cqi=(640+SNRtimes10)/5;

  uci_stats->pucch0_thres = gNB->pucch0_thres + (10*max_n0);
  bool no_conf=false;
  if (nr_sequences>1) {
    if (10*xrtmag_dB < uci_stats->pucch0_thres)
      no_conf=true;
  }
  gNB->bad_pucch += no_conf;
  // first bit of bitmap for sr presence and second bit for acknack presence
  uci_pdu->pduBitmap = pucch_pdu->sr_flag | ((pucch_pdu->bit_len_harq>0)<<1);
  uci_pdu->pucch_format = 0; // format 0
  uci_pdu->rnti = pucch_pdu->rnti;
  uci_pdu->ul_cqi = cqi;
  uci_pdu->timing_advance = 0xffff; // currently not valid
  uci_pdu->rssi = 1280 - (10*dB_fixed(32767*32767)-dB_fixed_times10(signal_energy_nodc(&rxdataF[0][pucch_pdu->start_symbol_index*frame_parms->ofdm_symbol_size+re_offset[0]],12)));

  if (pucch_pdu->bit_len_harq==0) {
    uci_pdu->harq = NULL;
    uci_pdu->sr = calloc(1,sizeof(*uci_pdu->sr));
    uci_pdu->sr->sr_confidence_level = no_conf ? 1 : 0;
    uci_stats->pucch0_sr_trials++;
    if (xrtmag_dB>(gNB->measurements.n0_power_tot_dB)) {
      uci_pdu->sr->sr_indication = 1;
      uci_stats->pucch0_positive_SR++;
    } else {
      uci_pdu->sr->sr_indication = 0;
    }
  }
  else if (pucch_pdu->bit_len_harq==1) {
    uci_pdu->harq = calloc(1,sizeof(*uci_pdu->harq));
    uci_pdu->harq->num_harq = 1;
    uci_pdu->harq->harq_confidence_level = no_conf ? 1 : 0;
    uci_pdu->harq->harq_list = (nfapi_nr_harq_t*)malloc(1);
    uci_pdu->harq->harq_list[0].harq_value = index&0x01;
    LOG_I(PHY,"Slot %d HARQ value %d, index %d with confidence level (0 is good, 1 is bad) %d xrt_mag %d n0 %d pucch0_thres %d\n",
          slot,uci_pdu->harq->harq_list[0].harq_value,index,uci_pdu->harq->harq_confidence_level,xrtmag_dB,max_n0,uci_stats->pucch0_thres);
    if (pucch_pdu->sr_flag == 1) {
      uci_pdu->sr = calloc(1,sizeof(*uci_pdu->sr));
      uci_pdu->sr->sr_indication = (index>1) ? 1 : 0;
      uci_pdu->sr->sr_confidence_level = no_conf ? 1 : 0;
      uci_stats->pucch0_positive_SR++;
    }
    uci_stats->pucch01_trials++;
  }
  else {
    uci_pdu->harq = calloc(1,sizeof(*uci_pdu->harq));
    uci_pdu->harq->num_harq = 2;
    uci_pdu->harq->harq_confidence_level = (no_conf) ? 1 : 0;
    uci_pdu->harq->harq_list = (nfapi_nr_harq_t*)malloc(2);
    uci_pdu->harq->harq_list[1].harq_value = index&0x01;
    uci_pdu->harq->harq_list[0].harq_value = (index>>1)&0x01;
    LOG_D(PHY, "Slot %d HARQ values %d and %d with confidence level (0 is good, 1 is bad) %d, xrt_mag %d\n",
          slot,uci_pdu->harq->harq_list[1].harq_value,uci_pdu->harq->harq_list[0].harq_value,uci_pdu->harq->harq_confidence_level,xrtmag_dB);
    if (pucch_pdu->sr_flag == 1) {
      uci_pdu->sr = calloc(1,sizeof(*uci_pdu->sr));
      uci_pdu->sr->sr_indication = (index>3) ? 1 : 0;
      uci_pdu->sr->sr_confidence_level = (no_conf) ? 1 : 0;
    }
  }
}





void nr_decode_pucch1(  int32_t **rxdataF,
		        pucch_GroupHopping_t pucch_GroupHopping,
                        uint32_t n_id,       // hoppingID higher layer parameter  
                        uint64_t *payload,
		       	NR_DL_FRAME_PARMS *frame_parms, 
                        int16_t amp,
                        int nr_tti_tx,
                        uint8_t m0,
                        uint8_t nrofSymbols,
                        uint8_t startingSymbolIndex,
                        uint16_t startingPRB,
                        uint16_t startingPRB_intraSlotHopping,
                        uint8_t timeDomainOCC,
                        uint8_t nr_bit) {
#ifdef DEBUG_NR_PUCCH_RX
  printf("\t [nr_generate_pucch1] start function at slot(nr_tti_tx)=%d payload=%lp m0=%d nrofSymbols=%d startingSymbolIndex=%d startingPRB=%d startingPRB_intraSlotHopping=%d timeDomainOCC=%d nr_bit=%d\n",
         nr_tti_tx,payload,m0,nrofSymbols,startingSymbolIndex,startingPRB,startingPRB_intraSlotHopping,timeDomainOCC,nr_bit);
#endif
  /*
   * Implement TS 38.211 Subclause 6.3.2.4.1 Sequence modulation
   *
   */
  // complex-valued symbol d_re, d_im containing complex-valued symbol d(0):
  int16_t d_re=0, d_im=0,d1_re=0,d1_im=0;
#ifdef DEBUG_NR_PUCCH_RX
  printf("\t [nr_generate_pucch1] sequence modulation: payload=%lp \tde_re=%d \tde_im=%d\n",payload,d_re,d_im);
#endif
  /*
   * Defining cyclic shift hopping TS 38.211 Subclause 6.3.2.2.2
   */
  // alpha is cyclic shift
  double alpha;
  // lnormal is the OFDM symbol number in the PUCCH transmission where l=0 corresponds to the first OFDM symbol of the PUCCH transmission
  //uint8_t lnormal = 0 ;
  // lprime is the index of the OFDM symbol in the slot that corresponds to the first OFDM symbol of the PUCCH transmission in the slot given by [5, TS 38.213]
  uint8_t lprime = startingSymbolIndex;
  // mcs = 0 except for PUCCH format 0
  uint8_t mcs=0;
  // r_u_v_alpha_delta_re and r_u_v_alpha_delta_im tables containing the sequence y(n) for the PUCCH, when they are multiplied by d(0)
  // r_u_v_alpha_delta_dmrs_re and r_u_v_alpha_delta_dmrs_im tables containing the sequence for the DM-RS.
  int16_t r_u_v_alpha_delta_re[12],r_u_v_alpha_delta_im[12],r_u_v_alpha_delta_dmrs_re[12],r_u_v_alpha_delta_dmrs_im[12];
  /*
   * in TS 38.213 Subclause 9.2.1 it is said that:
   * for PUCCH format 0 or PUCCH format 1, the index of the cyclic shift
   * is indicated by higher layer parameter PUCCH-F0-F1-initial-cyclic-shift
   */
  /*
   * the complex-valued symbol d_0 shall be multiplied with a sequence r_u_v_alpha_delta(n): y(n) = d_0 * r_u_v_alpha_delta(n)
   */
  // the value of u,v (delta always 0 for PUCCH) has to be calculated according to TS 38.211 Subclause 6.3.2.2.1
  uint8_t u=0,v=0;//,delta=0;
  // if frequency hopping is disabled, intraSlotFrequencyHopping is not provided
  //              n_hop = 0
  // if frequency hopping is enabled,  intraSlotFrequencyHopping is     provided
  //              n_hop = 0 for first hop
  //              n_hop = 1 for second hop
  uint8_t n_hop = 0;
  // Intra-slot frequency hopping shall be assumed when the higher-layer parameter intraSlotFrequencyHopping is provided,
  // regardless of whether the frequency-hop distance is zero or not,
  // otherwise no intra-slot frequency hopping shall be assumed
  //uint8_t PUCCH_Frequency_Hopping = 0 ; // from higher layers
  uint8_t intraSlotFrequencyHopping = 0;

  if (startingPRB != startingPRB_intraSlotHopping) {
    intraSlotFrequencyHopping=1;
  }

#ifdef DEBUG_NR_PUCCH_RX
  printf("\t [nr_generate_pucch1] intraSlotFrequencyHopping = %d \n",intraSlotFrequencyHopping);
#endif
  /*
   * Implementing TS 38.211 Subclause 6.3.2.4.2 Mapping to physical resources
   */
  //int32_t *txptr;
  uint32_t re_offset=0;
  int i=0;
#define MAX_SIZE_Z 168 // this value has to be calculated from mprime*12*table_6_3_2_4_1_1_N_SF_mprime_PUCCH_1_noHop[pucch_symbol_length]+m*12+n
  int16_t z_re_rx[MAX_SIZE_Z],z_im_rx[MAX_SIZE_Z],z_re_temp,z_im_temp;
  int16_t z_dmrs_re_rx[MAX_SIZE_Z],z_dmrs_im_rx[MAX_SIZE_Z],z_dmrs_re_temp,z_dmrs_im_temp;
  memset(z_re_rx,0,MAX_SIZE_Z*sizeof(int16_t));
  memset(z_im_rx,0,MAX_SIZE_Z*sizeof(int16_t));
  memset(z_dmrs_re_rx,0,MAX_SIZE_Z*sizeof(int16_t));
  memset(z_dmrs_im_rx,0,MAX_SIZE_Z*sizeof(int16_t));
  int l=0;
  for(l=0;l<nrofSymbols;l++){     //extracting data and dmrs from rxdataF
    if ((intraSlotFrequencyHopping == 1) && (l<floor(nrofSymbols/2))) { // intra-slot hopping enabled, we need to calculate new offset PRB
      startingPRB = startingPRB + startingPRB_intraSlotHopping;
    }

    if ((startingPRB <  (frame_parms->N_RB_DL>>1)) && ((frame_parms->N_RB_DL & 1) == 0)) { // if number RBs in bandwidth is even and current PRB is lower band
      re_offset = ((l+startingSymbolIndex)*frame_parms->ofdm_symbol_size) + (12*startingPRB) + frame_parms->first_carrier_offset;
    }

    if ((startingPRB >= (frame_parms->N_RB_DL>>1)) && ((frame_parms->N_RB_DL & 1) == 0)) { // if number RBs in bandwidth is even and current PRB is upper band
      re_offset = ((l+startingSymbolIndex)*frame_parms->ofdm_symbol_size) + (12*(startingPRB-(frame_parms->N_RB_DL>>1)));
    }

    if ((startingPRB <  (frame_parms->N_RB_DL>>1)) && ((frame_parms->N_RB_DL & 1) == 1)) { // if number RBs in bandwidth is odd  and current PRB is lower band
      re_offset = ((l+startingSymbolIndex)*frame_parms->ofdm_symbol_size) + (12*startingPRB) + frame_parms->first_carrier_offset;
    }

    if ((startingPRB >  (frame_parms->N_RB_DL>>1)) && ((frame_parms->N_RB_DL & 1) == 1)) { // if number RBs in bandwidth is odd  and current PRB is upper band
      re_offset = ((l+startingSymbolIndex)*frame_parms->ofdm_symbol_size) + (12*(startingPRB-(frame_parms->N_RB_DL>>1))) + 6;
    }

    if ((startingPRB == (frame_parms->N_RB_DL>>1)) && ((frame_parms->N_RB_DL & 1) == 1)) { // if number RBs in bandwidth is odd  and current PRB contains DC
      re_offset = ((l+startingSymbolIndex)*frame_parms->ofdm_symbol_size) + (12*startingPRB) + frame_parms->first_carrier_offset;
    }

    for (int n=0; n<12; n++) {
      if ((n==6) && (startingPRB == (frame_parms->N_RB_DL>>1)) && ((frame_parms->N_RB_DL & 1) == 1)) {
        // if number RBs in bandwidth is odd  and current PRB contains DC, we need to recalculate the offset when n=6 (for second half PRB)
        re_offset = ((l+startingSymbolIndex)*frame_parms->ofdm_symbol_size);
      }

      if (l%2 == 1) { // mapping PUCCH according to TS38.211 subclause 6.4.1.3.1
        z_re_rx[i+n] = ((int16_t *)&rxdataF[0][re_offset])[0];
        z_im_rx[i+n] = ((int16_t *)&rxdataF[0][re_offset])[1];
#ifdef DEBUG_NR_PUCCH_RX
        printf("\t [nr_generate_pucch1] mapping PUCCH to RE \t amp=%d \tofdm_symbol_size=%d \tN_RB_DL=%d \tfirst_carrier_offset=%d \tz_pucch[%d]=txptr(%u)=(x_n(l=%d,n=%d)=(%d,%d))\n",
               amp,frame_parms->ofdm_symbol_size,frame_parms->N_RB_DL,frame_parms->first_carrier_offset,i+n,re_offset,
               l,n,((int16_t *)&rxdataF[0][re_offset])[0],((int16_t *)&rxdataF[0][re_offset])[1]);
#endif
      }

      if (l%2 == 0) { // mapping DM-RS signal according to TS38.211 subclause 6.4.1.3.1
        z_dmrs_re_rx[i+n] = ((int16_t *)&rxdataF[0][re_offset])[0];
        z_dmrs_im_rx[i+n] = ((int16_t *)&rxdataF[0][re_offset])[1];
	//	printf("%d\t%d\t%d\n",l,z_dmrs_re_rx[i+n],z_dmrs_im_rx[i+n]);
#ifdef DEBUG_NR_PUCCH_RX
        printf("\t [nr_generate_pucch1] mapping DM-RS to RE \t amp=%d \tofdm_symbol_size=%d \tN_RB_DL=%d \tfirst_carrier_offset=%d \tz_dm-rs[%d]=txptr(%u)=(x_n(l=%d,n=%d)=(%d,%d))\n",
               amp,frame_parms->ofdm_symbol_size,frame_parms->N_RB_DL,frame_parms->first_carrier_offset,i+n,re_offset,
               l,n,((int16_t *)&rxdataF[0][re_offset])[0],((int16_t *)&rxdataF[0][re_offset])[1]);
#endif
	//        printf("l=%d\ti=%d\tre_offset=%d\treceived dmrs re=%d\tim=%d\n",l,i,re_offset,z_dmrs_re_rx[i+n],z_dmrs_im_rx[i+n]);
      }

      re_offset++;
    }
    if (l%2 == 1) i+=12;
  }
  int16_t y_n_re[12],y_n_im[12],y1_n_re[12],y1_n_im[12];
  memset(y_n_re,0,12*sizeof(int16_t));
  memset(y_n_im,0,12*sizeof(int16_t));
  memset(y1_n_re,0,12*sizeof(int16_t));
  memset(y1_n_im,0,12*sizeof(int16_t));
  //generating transmitted sequence and dmrs
  for (l=0; l<nrofSymbols; l++) {
#ifdef DEBUG_NR_PUCCH_RX
    printf("\t [nr_generate_pucch1] for symbol l=%d, lprime=%d\n",
           l,lprime);
#endif
    // y_n contains the complex value d multiplied by the sequence r_u_v
    if ((intraSlotFrequencyHopping == 1) && (l >= (int)floor(nrofSymbols/2))) n_hop = 1; // n_hop = 1 for second hop

#ifdef DEBUG_NR_PUCCH_RX
    printf("\t [nr_generate_pucch1] entering function nr_group_sequence_hopping with n_hop=%d, nr_tti_tx=%d\n",
           n_hop,nr_tti_tx);
#endif
    nr_group_sequence_hopping(pucch_GroupHopping,n_id,n_hop,nr_tti_tx,&u,&v); // calculating u and v value
    alpha = nr_cyclic_shift_hopping(n_id,m0,mcs,l,lprime,nr_tti_tx);
    
    for (int n=0; n<12; n++) {  // generating low papr sequences
      if(l%2==1){ 
        r_u_v_alpha_delta_re[n] = (int16_t)(((((int32_t)(round(32767*cos(alpha*n))) * table_5_2_2_2_2_Re[u][n])>>15)
                                             - (((int32_t)(round(32767*sin(alpha*n))) * table_5_2_2_2_2_Im[u][n])>>15))); // Re part of base sequence shifted by alpha
        r_u_v_alpha_delta_im[n] = (int16_t)(((((int32_t)(round(32767*cos(alpha*n))) * table_5_2_2_2_2_Im[u][n])>>15)
                                             + (((int32_t)(round(32767*sin(alpha*n))) * table_5_2_2_2_2_Re[u][n])>>15))); // Im part of base sequence shifted by alpha
      }
      else{
        r_u_v_alpha_delta_dmrs_re[n] = (int16_t)(((((int32_t)(round(32767*cos(alpha*n))) * table_5_2_2_2_2_Re[u][n])>>15)
						  - (((int32_t)(round(32767*sin(alpha*n))) * table_5_2_2_2_2_Im[u][n])>>15))); // Re part of DMRS base sequence shifted by alpha
        r_u_v_alpha_delta_dmrs_im[n] = (int16_t)(((((int32_t)(round(32767*cos(alpha*n))) * table_5_2_2_2_2_Im[u][n])>>15)
						  + (((int32_t)(round(32767*sin(alpha*n))) * table_5_2_2_2_2_Re[u][n])>>15))); // Im part of DMRS base sequence shifted by alpha
        r_u_v_alpha_delta_dmrs_re[n] = (int16_t)(((int32_t)(amp*r_u_v_alpha_delta_dmrs_re[n]))>>15);
        r_u_v_alpha_delta_dmrs_im[n] = (int16_t)(((int32_t)(amp*r_u_v_alpha_delta_dmrs_im[n]))>>15);
      }
      //      printf("symbol=%d\tr_u_rx_re=%d\tr_u_rx_im=%d\n",l,r_u_v_alpha_delta_dmrs_re[n], r_u_v_alpha_delta_dmrs_im[n]);
      // PUCCH sequence = DM-RS sequence multiplied by d(0)
      /*      y_n_re[n]               = (int16_t)(((((int32_t)(r_u_v_alpha_delta_re[n])*d_re)>>15)
	      - (((int32_t)(r_u_v_alpha_delta_im[n])*d_im)>>15))); // Re part of y(n)
	      y_n_im[n]               = (int16_t)(((((int32_t)(r_u_v_alpha_delta_re[n])*d_im)>>15)
	      + (((int32_t)(r_u_v_alpha_delta_im[n])*d_re)>>15))); // Im part of y(n) */
#ifdef DEBUG_NR_PUCCH_RX
      printf("\t [nr_generate_pucch1] sequence generation \tu=%d \tv=%d \talpha=%lf \tr_u_v_alpha_delta[n=%d]=(%d,%d) \ty_n[n=%d]=(%d,%d)\n",
             u,v,alpha,n,r_u_v_alpha_delta_re[n],r_u_v_alpha_delta_im[n],n,y_n_re[n],y_n_im[n]);
#endif
    }
    /*
     * The block of complex-valued symbols y(n) shall be block-wise spread with the orthogonal sequence wi(m)
     * (defined in table_6_3_2_4_1_2_Wi_Re and table_6_3_2_4_1_2_Wi_Im)
     * z(mprime*12*table_6_3_2_4_1_1_N_SF_mprime_PUCCH_1_noHop[pucch_symbol_length]+m*12+n)=wi(m)*y(n)
     *
     * The block of complex-valued symbols r_u_v_alpha_dmrs_delta(n) for DM-RS shall be block-wise spread with the orthogonal sequence wi(m)
     * (defined in table_6_3_2_4_1_2_Wi_Re and table_6_3_2_4_1_2_Wi_Im)
     * z(mprime*12*table_6_4_1_3_1_1_1_N_SF_mprime_PUCCH_1_noHop[pucch_symbol_length]+m*12+n)=wi(m)*y(n)
     *
     */
    // the orthogonal sequence index for wi(m) defined in TS 38.213 Subclause 9.2.1
    // the index of the orthogonal cover code is from a set determined as described in [4, TS 38.211]
    // and is indicated by higher layer parameter PUCCH-F1-time-domain-OCC
    // In the PUCCH_Config IE, the PUCCH-format1, timeDomainOCC field
    uint8_t w_index = timeDomainOCC;
    // N_SF_mprime_PUCCH_1 contains N_SF_mprime from table 6.3.2.4.1-1   (depending on number of PUCCH symbols nrofSymbols, mprime and intra-slot hopping enabled/disabled)
    uint8_t N_SF_mprime_PUCCH_1;
    // N_SF_mprime_PUCCH_1 contains N_SF_mprime from table 6.4.1.3.1.1-1 (depending on number of PUCCH symbols nrofSymbols, mprime and intra-slot hopping enabled/disabled)
    uint8_t N_SF_mprime_PUCCH_DMRS_1;
    // N_SF_mprime_PUCCH_1 contains N_SF_mprime from table 6.3.2.4.1-1   (depending on number of PUCCH symbols nrofSymbols, mprime=0 and intra-slot hopping enabled/disabled)
    uint8_t N_SF_mprime0_PUCCH_1;
    // N_SF_mprime_PUCCH_1 contains N_SF_mprime from table 6.4.1.3.1.1-1 (depending on number of PUCCH symbols nrofSymbols, mprime=0 and intra-slot hopping enabled/disabled)
    uint8_t N_SF_mprime0_PUCCH_DMRS_1;
    // mprime is 0 if no intra-slot hopping / mprime is {0,1} if intra-slot hopping
    uint8_t mprime = 0;

    if (intraSlotFrequencyHopping == 0) { // intra-slot hopping disabled
#ifdef DEBUG_NR_PUCCH_RX
      printf("\t [nr_generate_pucch1] block-wise spread with the orthogonal sequence wi(m) if intraSlotFrequencyHopping = %d, intra-slot hopping disabled\n",
             intraSlotFrequencyHopping);
#endif
      N_SF_mprime_PUCCH_1       =   table_6_3_2_4_1_1_N_SF_mprime_PUCCH_1_noHop[nrofSymbols-1]; // only if intra-slot hopping not enabled (PUCCH)
      N_SF_mprime_PUCCH_DMRS_1  = table_6_4_1_3_1_1_1_N_SF_mprime_PUCCH_1_noHop[nrofSymbols-1]; // only if intra-slot hopping not enabled (DM-RS)
      N_SF_mprime0_PUCCH_1      =   table_6_3_2_4_1_1_N_SF_mprime_PUCCH_1_noHop[nrofSymbols-1]; // only if intra-slot hopping not enabled mprime = 0 (PUCCH)
      N_SF_mprime0_PUCCH_DMRS_1 = table_6_4_1_3_1_1_1_N_SF_mprime_PUCCH_1_noHop[nrofSymbols-1]; // only if intra-slot hopping not enabled mprime = 0 (DM-RS)
#ifdef DEBUG_NR_PUCCH_RX
      printf("\t [nr_generate_pucch1] w_index = %d, N_SF_mprime_PUCCH_1 = %d, N_SF_mprime_PUCCH_DMRS_1 = %d, N_SF_mprime0_PUCCH_1 = %d, N_SF_mprime0_PUCCH_DMRS_1 = %d\n",
             w_index, N_SF_mprime_PUCCH_1,N_SF_mprime_PUCCH_DMRS_1,N_SF_mprime0_PUCCH_1,N_SF_mprime0_PUCCH_DMRS_1);
#endif
      if(l%2==1){
        for (int m=0; m < N_SF_mprime_PUCCH_1; m++) {
	  if(floor(l/2)*12==(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)){
            for (int n=0; n<12 ; n++) {
              z_re_temp = (int16_t)(((((int32_t)(table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m])*z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15)
				     + (((int32_t)(table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m])*z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15))>>1);
              z_im_temp = (int16_t)(((((int32_t)(table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m])*z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15)
				     - (((int32_t)(table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m])*z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15))>>1);
              z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n]=z_re_temp; 
              z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n]=z_im_temp; 
	      //	      printf("symbol=%d\tz_re_rx=%d\tz_im_rx=%d\t",l,(int)z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n],(int)z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n]);
#ifdef DEBUG_NR_PUCCH_RX
              printf("\t [nr_generate_pucch1] block-wise spread with wi(m) (mprime=%d, m=%d, n=%d) z[%d] = ((%d * %d - %d * %d), (%d * %d + %d * %d)) = (%d,%d)\n",
                     mprime, m, n, (mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n,
                     table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m],y_n_re[n],table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m],y_n_im[n],
                     table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m],y_n_im[n],table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m],y_n_re[n],
                     z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n],z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n]);
#endif   
	      // multiplying with conjugate of low papr sequence  
	      z_re_temp = (int16_t)(((((int32_t)(r_u_v_alpha_delta_re[n])*z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15)
				     + (((int32_t)(r_u_v_alpha_delta_im[n])*z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15))>>1); 
              z_im_temp = (int16_t)(((((int32_t)(r_u_v_alpha_delta_re[n])*z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15)
				     - (((int32_t)(r_u_v_alpha_delta_im[n])*z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15))>>1);
              z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n] = z_re_temp;
              z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n] = z_im_temp;
	      /*	      if(z_re_temp<0){
			      printf("\nBug detection %d\t%d\t%d\t%d\n",r_u_v_alpha_delta_re[n],z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n],(((int32_t)(r_u_v_alpha_delta_re[n])*z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15),(((int32_t)(r_u_v_alpha_delta_im[n])*z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15));
			      }
			      printf("z1_re_rx=%d\tz1_im_rx=%d\n",(int)z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n],(int)z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n]); */ 
	    }
	  }
        }
      }

      else{
        for (int m=0; m < N_SF_mprime_PUCCH_DMRS_1; m++) {
          if(floor(l/2)*12==(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)){
            for (int n=0; n<12 ; n++) {
              z_dmrs_re_temp = (int16_t)(((((int32_t)(table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_DMRS_1][w_index][m])*z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15)
					  + (((int32_t)(table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_DMRS_1][w_index][m])*z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15))>>1);
              z_dmrs_im_temp =  (int16_t)(((((int32_t)(table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_DMRS_1][w_index][m])*z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15)
					   - (((int32_t)(table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_DMRS_1][w_index][m])*z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15))>>1);
              z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n] = z_dmrs_re_temp;
              z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n] = z_dmrs_im_temp;
	      //              printf("symbol=%d\tz_dmrs_re_rx=%d\tz_dmrs_im_rx=%d\t",l,(int)z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n],(int)z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n]);
#ifdef DEBUG_NR_PUCCH_RX
              printf("\t [nr_generate_pucch1] block-wise spread with wi(m) (mprime=%d, m=%d, n=%d) z[%d] = ((%d * %d - %d * %d), (%d * %d + %d * %d)) = (%d,%d)\n",
                     mprime, m, n, (mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n,
                     table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m],r_u_v_alpha_delta_dmrs_re[n],table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m],r_u_v_alpha_delta_dmrs_im[n],
                     table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m],r_u_v_alpha_delta_dmrs_im[n],table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m],r_u_v_alpha_delta_dmrs_re[n],
                     z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n],z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n]);
#endif
              //finding channel coeffcients by dividing received dmrs with actual dmrs and storing them in z_dmrs_re_rx and z_dmrs_im_rx arrays
              z_dmrs_re_temp = (int16_t)(((((int32_t)(r_u_v_alpha_delta_dmrs_re[n])*z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15)
					  + (((int32_t)(r_u_v_alpha_delta_dmrs_im[n])*z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15))>>1); 
              z_dmrs_im_temp = (int16_t)(((((int32_t)(r_u_v_alpha_delta_dmrs_re[n])*z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15)
					  - (((int32_t)(r_u_v_alpha_delta_dmrs_im[n])*z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15))>>1);
	      /*	      if(z_dmrs_re_temp<0){
			      printf("\nBug detection %d\t%d\t%d\t%d\n",r_u_v_alpha_delta_dmrs_re[n],z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n],(((int32_t)(r_u_v_alpha_delta_dmrs_re[n])*z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15),(((int32_t)(r_u_v_alpha_delta_dmrs_im[n])*z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15));
			      }*/
	      z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n] = z_dmrs_re_temp;
	      z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n] = z_dmrs_im_temp; 
	      //	      printf("z1_dmrs_re_rx=%d\tz1_dmrs_im_rx=%d\n",(int)z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n],(int)z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n]);
	      /* z_dmrs_re_rx[(int)(l/2)*12+n]=z_dmrs_re_rx[(int)(l/2)*12+n]/r_u_v_alpha_delta_dmrs_re[n]; 
		 z_dmrs_im_rx[(int)(l/2)*12+n]=z_dmrs_im_rx[(int)(l/2)*12+n]/r_u_v_alpha_delta_dmrs_im[n]; */
	    }
	  }
        }
      }
    }

    if (intraSlotFrequencyHopping == 1) { // intra-slot hopping enabled
#ifdef DEBUG_NR_PUCCH_RX
      printf("\t [nr_generate_pucch1] block-wise spread with the orthogonal sequence wi(m) if intraSlotFrequencyHopping = %d, intra-slot hopping enabled\n",
             intraSlotFrequencyHopping);
#endif
      N_SF_mprime_PUCCH_1       =   table_6_3_2_4_1_1_N_SF_mprime_PUCCH_1_m0Hop[nrofSymbols-1]; // only if intra-slot hopping enabled mprime = 0 (PUCCH)
      N_SF_mprime_PUCCH_DMRS_1  = table_6_4_1_3_1_1_1_N_SF_mprime_PUCCH_1_m0Hop[nrofSymbols-1]; // only if intra-slot hopping enabled mprime = 0 (DM-RS)
      N_SF_mprime0_PUCCH_1      =   table_6_3_2_4_1_1_N_SF_mprime_PUCCH_1_m0Hop[nrofSymbols-1]; // only if intra-slot hopping enabled mprime = 0 (PUCCH)
      N_SF_mprime0_PUCCH_DMRS_1 = table_6_4_1_3_1_1_1_N_SF_mprime_PUCCH_1_m0Hop[nrofSymbols-1]; // only if intra-slot hopping enabled mprime = 0 (DM-RS)
#ifdef DEBUG_NR_PUCCH_RX
      printf("\t [nr_generate_pucch1] w_index = %d, N_SF_mprime_PUCCH_1 = %d, N_SF_mprime_PUCCH_DMRS_1 = %d, N_SF_mprime0_PUCCH_1 = %d, N_SF_mprime0_PUCCH_DMRS_1 = %d\n",
             w_index, N_SF_mprime_PUCCH_1,N_SF_mprime_PUCCH_DMRS_1,N_SF_mprime0_PUCCH_1,N_SF_mprime0_PUCCH_DMRS_1);
#endif

      for (mprime = 0; mprime<2; mprime++) { // mprime can get values {0,1}
	if(l%2==1){
          for (int m=0; m < N_SF_mprime_PUCCH_1; m++) {
            if(floor(l/2)*12==(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)){
              for (int n=0; n<12 ; n++) {
                z_re_temp = (int16_t)(((((int32_t)(table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m])*z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15)
				       + (((int32_t)(table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m])*z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15))>>1);
                z_im_temp = (int16_t)(((((int32_t)(table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m])*z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15)
				       - (((int32_t)(table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m])*z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15))>>1);
                z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n] = z_re_temp;
                z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n] = z_im_temp;
#ifdef DEBUG_NR_PUCCH_RX
                printf("\t [nr_generate_pucch1] block-wise spread with wi(m) (mprime=%d, m=%d, n=%d) z[%d] = ((%d * %d - %d * %d), (%d * %d + %d * %d)) = (%d,%d)\n",
                       mprime, m, n, (mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n,
                       table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m],y_n_re[n],table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m],y_n_im[n],
                       table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m],y_n_im[n],table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m],y_n_re[n],
                       z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n],z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n]);
#endif 
                z_re_temp = (int16_t)(((((int32_t)(r_u_v_alpha_delta_re[n])*z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15)
				       + (((int32_t)(r_u_v_alpha_delta_im[n])*z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15))>>1); 
                z_im_temp = (int16_t)(((((int32_t)(r_u_v_alpha_delta_re[n])*z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15)
				       - (((int32_t)(r_u_v_alpha_delta_im[n])*z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n])>>15))>>1); 	  
	        z_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n] = z_re_temp; 
                z_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n] = z_im_temp; 
	      }
	    }
	  }
        }

	else{
	  for (int m=0; m < N_SF_mprime_PUCCH_DMRS_1; m++) {
            if(floor(l/2)*12==(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)){
              for (int n=0; n<12 ; n++) {
                z_dmrs_re_temp = (int16_t)(((((int32_t)(table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_DMRS_1][w_index][m])*z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15)
					    + (((int32_t)(table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_DMRS_1][w_index][m])*z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15))>>1);
                z_dmrs_im_temp = (int16_t)(((((int32_t)(table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_DMRS_1][w_index][m])*z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15)
					    - (((int32_t)(table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_DMRS_1][w_index][m])*z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15))>>1);
                z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n] = z_dmrs_re_temp; 
                z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n] = z_dmrs_im_temp; 
#ifdef DEBUG_NR_PUCCH_RX
                printf("\t [nr_generate_pucch1] block-wise spread with wi(m) (mprime=%d, m=%d, n=%d) z[%d] = ((%d * %d - %d * %d), (%d * %d + %d * %d)) = (%d,%d)\n",
                       mprime, m, n, (mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n,
                       table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m],r_u_v_alpha_delta_dmrs_re[n],table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m],r_u_v_alpha_delta_dmrs_im[n],
                       table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m],r_u_v_alpha_delta_dmrs_im[n],table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m],r_u_v_alpha_delta_dmrs_re[n],
                       z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n],z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_1)+(m*12)+n]);
#endif
                //finding channel coeffcients by dividing received dmrs with actual dmrs and storing them in z_dmrs_re_rx and z_dmrs_im_rx arrays
                z_dmrs_re_temp = (int16_t)(((((int32_t)(r_u_v_alpha_delta_dmrs_re[n])*z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15)
					    + (((int32_t)(r_u_v_alpha_delta_dmrs_im[n])*z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15))>>1); 
                z_dmrs_im_temp = (int16_t)(((((int32_t)(r_u_v_alpha_delta_dmrs_re[n])*z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15)
					    - (((int32_t)(r_u_v_alpha_delta_dmrs_im[n])*z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n])>>15))>>1);
	        z_dmrs_re_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n] = z_dmrs_re_temp; 
                z_dmrs_im_rx[(mprime*12*N_SF_mprime0_PUCCH_DMRS_1)+(m*12)+n] = z_dmrs_im_temp; 

		/* 	z_dmrs_re_rx[(int)(l/2)*12+n]=z_dmrs_re_rx[(int)(l/2)*12+n]/r_u_v_alpha_delta_dmrs_re[n]; 
			z_dmrs_im_rx[(int)(l/2)*12+n]=z_dmrs_im_rx[(int)(l/2)*12+n]/r_u_v_alpha_delta_dmrs_im[n]; */
	      }
	    }
	  }
        }

        N_SF_mprime_PUCCH_1       =   table_6_3_2_4_1_1_N_SF_mprime_PUCCH_1_m1Hop[nrofSymbols-1]; // only if intra-slot hopping enabled mprime = 1 (PUCCH)
        N_SF_mprime_PUCCH_DMRS_1  = table_6_4_1_3_1_1_1_N_SF_mprime_PUCCH_1_m1Hop[nrofSymbols-1]; // only if intra-slot hopping enabled mprime = 1 (DM-RS)
      }
    }
  }
  int16_t H_re[12],H_im[12],H1_re[12],H1_im[12];
  memset(H_re,0,12*sizeof(int16_t));
  memset(H_im,0,12*sizeof(int16_t));
  memset(H1_re,0,12*sizeof(int16_t));
  memset(H1_im,0,12*sizeof(int16_t)); 
  //averaging channel coefficients
  for(l=0;l<=ceil(nrofSymbols/2);l++){
    if(intraSlotFrequencyHopping==0){
      for(int n=0;n<12;n++){
        H_re[n]=round(z_dmrs_re_rx[l*12+n]/ceil(nrofSymbols/2))+H_re[n];
        H_im[n]=round(z_dmrs_im_rx[l*12+n]/ceil(nrofSymbols/2))+H_im[n];
      }
    }
    else{
      if(l<round(nrofSymbols/4)){
        for(int n=0;n<12;n++){
          H_re[n]=round(z_dmrs_re_rx[l*12+n]/round(nrofSymbols/4))+H_re[n];
          H_im[n]=round(z_dmrs_im_rx[l*12+n]/round(nrofSymbols/4))+H_im[n];
	}
      }
      else{
        for(int n=0;n<12;n++){
          H1_re[n]=round(z_dmrs_re_rx[l*12+n]/(ceil(nrofSymbols/2)-round(nrofSymbols/4)))+H1_re[n];
          H1_im[n]=round(z_dmrs_im_rx[l*12+n]/(ceil(nrofSymbols/2))-round(nrofSymbols/4))+H1_im[n];
	} 
      }
    }
  }
  //averaging information sequences
  for(l=0;l<floor(nrofSymbols/2);l++){
    if(intraSlotFrequencyHopping==0){
      for(int n=0;n<12;n++){
        y_n_re[n]=round(z_re_rx[l*12+n]/floor(nrofSymbols/2))+y_n_re[n];
        y_n_im[n]=round(z_im_rx[l*12+n]/floor(nrofSymbols/2))+y_n_im[n];
      }
    }
    else{
      if(l<floor(nrofSymbols/4)){
        for(int n=0;n<12;n++){
          y_n_re[n]=round(z_re_rx[l*12+n]/floor(nrofSymbols/4))+y_n_re[n];
          y_n_im[n]=round(z_im_rx[l*12+n]/floor(nrofSymbols/4))+y_n_im[n];
	}	     
      }
      else{
        for(int n=0;n<12;n++){
          y1_n_re[n]=round(z_re_rx[l*12+n]/round(nrofSymbols/4))+y1_n_re[n];
          y1_n_im[n]=round(z_im_rx[l*12+n]/round(nrofSymbols/4))+y1_n_im[n];
        }
      }	
    }
  }
  // mrc combining to obtain z_re and z_im
  if(intraSlotFrequencyHopping==0){
    for(int n=0;n<12;n++){
      d_re = round(((int16_t)(((((int32_t)(H_re[n])*y_n_re[n])>>15) + (((int32_t)(H_im[n])*y_n_im[n])>>15))>>1))/12)+d_re; 
      d_im = round(((int16_t)(((((int32_t)(H_re[n])*y_n_im[n])>>15) - (((int32_t)(H_im[n])*y_n_re[n])>>15))>>1))/12)+d_im; 
    }
  }
  else{
    for(int n=0;n<12;n++){
      d_re = round(((int16_t)(((((int32_t)(H_re[n])*y_n_re[n])>>15) + (((int32_t)(H_im[n])*y_n_im[n])>>15))>>1))/12)+d_re; 
      d_im = round(((int16_t)(((((int32_t)(H_re[n])*y_n_im[n])>>15) - (((int32_t)(H_im[n])*y_n_re[n])>>15))>>1))/12)+d_im;
      d1_re = round(((int16_t)(((((int32_t)(H1_re[n])*y1_n_re[n])>>15) + (((int32_t)(H1_im[n])*y1_n_im[n])>>15))>>1))/12)+d1_re; 
      d1_im = round(((int16_t)(((((int32_t)(H1_re[n])*y1_n_im[n])>>15) - (((int32_t)(H1_im[n])*y1_n_re[n])>>15))>>1))/12)+d1_im; 
    }
    d_re=round(d_re/2);
    d_im=round(d_im/2);
    d1_re=round(d1_re/2);
    d1_im=round(d1_im/2);
    d_re=d_re+d1_re;
    d_im=d_im+d1_im;
  }
  //Decoding QPSK or BPSK symbols to obtain payload bits
  if(nr_bit==1){
    if((d_re+d_im)>0){
      *payload=0;
    }
    else{
      *payload=1;
    } 
  }
  else if(nr_bit==2){
    if((d_re>0)&&(d_im>0)){
      *payload=0;
    }
    else if((d_re<0)&&(d_im>0)){
      *payload=1;
    } 
    else if((d_re>0)&&(d_im<0)){
      *payload=2;
    }
    else{
      *payload=3;
    }
  }
}

__m256i pucch2_3bit[8*2];
__m256i pucch2_4bit[16*2];
__m256i pucch2_5bit[32*2];
__m256i pucch2_6bit[64*2];
__m256i pucch2_7bit[128*2];
__m256i pucch2_8bit[256*2];
__m256i pucch2_9bit[512*2];
__m256i pucch2_10bit[1024*2];
__m256i pucch2_11bit[2048*2];

__m256i *pucch2_lut[9]={pucch2_3bit,
			pucch2_4bit,
			pucch2_5bit,
			pucch2_6bit,
			pucch2_7bit,
			pucch2_8bit,
			pucch2_9bit,
			pucch2_10bit,
			pucch2_11bit};

__m64 pucch2_polar_4bit[16];
__m128i pucch2_polar_llr_num_lut[256],pucch2_polar_llr_den_lut[256];

void init_pucch2_luts() {

  uint32_t out;
  int8_t bit; 
  
  for (int b=3;b<12;b++) {
    for (uint16_t i=0;i<(1<<b);i++) {
      out=encodeSmallBlock(&i,b);
#ifdef DEBUG_NR_PUCCH_RX
      if (b==3) printf("in %d, out %x\n",i,out);
#endif
      __m256i *lut_i=&pucch2_lut[b-3][i<<1];
      __m256i *lut_ip1=&pucch2_lut[b-3][1+(i<<1)];
      bit = (out&0x1) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,0);
      bit = (out&0x2) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,0);
      bit = (out&0x4) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,1);
      bit = (out&0x8) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,1);
      bit = (out&0x10) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,2);
      bit = (out&0x20) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,2);
      bit = (out&0x40) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,3);
      bit = (out&0x80) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,3);
      bit = (out&0x100) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,4);
      bit = (out&0x200) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,4);
      bit = (out&0x400) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,5);
      bit = (out&0x800) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,5);
      bit = (out&0x1000) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,6);
      bit = (out&0x2000) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,6);
      bit = (out&0x4000) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,7);
      bit = (out&0x8000) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,7);
      bit = (out&0x10000) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,8);
      bit = (out&0x20000) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,8);
      bit = (out&0x40000) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,9);
      bit = (out&0x80000) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,9);
      bit = (out&0x100000) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,10);
      bit = (out&0x200000) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,10);
      bit = (out&0x400000) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,11);
      bit = (out&0x800000) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,11);
      bit = (out&0x1000000) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,12);
      bit = (out&0x2000000) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,12);
      bit = (out&0x4000000) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,13);
      bit = (out&0x8000000) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,13);
      bit = (out&0x10000000) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,14);
      bit = (out&0x20000000) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,14);
      bit = (out&0x40000000) > 0 ? -1 : 1;
      *lut_i = _mm256_insert_epi16(*lut_i,bit,15);
      bit = (out&0x80000000) > 0 ? -1 : 1;
      *lut_ip1 = _mm256_insert_epi16(*lut_ip1,bit,15);
    }
  }
  for (uint16_t i=0;i<16;i++) {
    __m64 *lut_i=&pucch2_polar_4bit[i];

    bit = (i&0x1) > 0 ? -1 : 1;
    *lut_i = _mm_insert_pi16(*lut_i,bit,0);
    bit = (i&0x2) > 0 ? -1 : 1;
    *lut_i = _mm_insert_pi16(*lut_i,bit,1);
    bit = (i&0x4) > 0 ? -1 : 1;
    *lut_i = _mm_insert_pi16(*lut_i,bit,2);
    bit = (i&0x8) > 0 ? -1 : 1;
    *lut_i = _mm_insert_pi16(*lut_i,bit,3);
  }
  for (int i=0;i<256;i++) {
    __m128i *lut_num_i=&pucch2_polar_llr_num_lut[i];
    __m128i *lut_den_i=&pucch2_polar_llr_den_lut[i];
    bit = (i&0x1) > 0 ? 0 : 1;
   *lut_num_i = _mm_insert_epi16(*lut_num_i,bit,0);
   *lut_den_i = _mm_insert_epi16(*lut_den_i,1-bit,0);

    bit = (i&0x10) > 0 ? 0 : 1;
   *lut_num_i = _mm_insert_epi16(*lut_num_i,bit,1);
   *lut_den_i = _mm_insert_epi16(*lut_den_i,1-bit,1);

    bit = (i&0x2) > 0 ? 0 : 1;
   *lut_num_i = _mm_insert_epi16(*lut_num_i,bit,2);
   *lut_den_i = _mm_insert_epi16(*lut_den_i,1-bit,2);

    bit = (i&0x20) > 0 ? 0 : 1;
   *lut_num_i = _mm_insert_epi16(*lut_num_i,bit,3);
   *lut_den_i = _mm_insert_epi16(*lut_den_i,1-bit,3);

    bit = (i&0x4) > 0 ? 0 : 1;
   *lut_num_i = _mm_insert_epi16(*lut_num_i,bit,4);
   *lut_den_i = _mm_insert_epi16(*lut_den_i,1-bit,4);

    bit = (i&0x40) > 0 ? 0 : 1;
   *lut_num_i = _mm_insert_epi16(*lut_num_i,bit,5);
   *lut_den_i = _mm_insert_epi16(*lut_den_i,1-bit,5);

    bit = (i&0x8) > 0 ? 0 : 1;
   *lut_num_i = _mm_insert_epi16(*lut_num_i,bit,6);
   *lut_den_i = _mm_insert_epi16(*lut_den_i,1-bit,6);

    bit = (i&0x80) > 0 ? 0 : 1;
   *lut_num_i = _mm_insert_epi16(*lut_num_i,bit,7);
   *lut_den_i = _mm_insert_epi16(*lut_den_i,1-bit,7);

#ifdef DEBUG_NR_PUCCH_RX
   printf("i %d, lut_num (%d,%d,%d,%d,%d,%d,%d,%d)\n",i,
	  ((int16_t *)lut_num_i)[0],
	  ((int16_t *)lut_num_i)[1],
	  ((int16_t *)lut_num_i)[2],
	  ((int16_t *)lut_num_i)[3],
	  ((int16_t *)lut_num_i)[4],
	  ((int16_t *)lut_num_i)[5],
	  ((int16_t *)lut_num_i)[6],
	  ((int16_t *)lut_num_i)[7]);
#endif
  }
}


void nr_decode_pucch2(PHY_VARS_gNB *gNB,
                      int slot,
                      nfapi_nr_uci_pucch_pdu_format_2_3_4_t* uci_pdu,
                      nfapi_nr_pucch_pdu_t* pucch_pdu) {

  int32_t **rxdataF = gNB->common_vars.rxdataF;
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  //pucch_GroupHopping_t pucch_GroupHopping = pucch_pdu->group_hop_flag + (pucch_pdu->sequence_hop_flag<<1);

  AssertFatal(pucch_pdu->nr_of_symbols==1 || pucch_pdu->nr_of_symbols==2,
	      "Illegal number of symbols  for PUCCH 2 %d\n",pucch_pdu->nr_of_symbols);

  AssertFatal((pucch_pdu->prb_start-((pucch_pdu->prb_start>>2)<<2))==0,
              "Current pucch2 receiver implementation requires a PRB offset multiple of 4. The one selected is %d",
              pucch_pdu->prb_start);

  //extract pucch and dmrs first

  int l2=pucch_pdu->start_symbol_index;
  int re_offset[2];
  re_offset[0] = 12*(pucch_pdu->prb_start+pucch_pdu->bwp_start) + frame_parms->first_carrier_offset;
  if (re_offset[0]>= frame_parms->ofdm_symbol_size) 
    re_offset[0]-=frame_parms->ofdm_symbol_size;
  if (pucch_pdu->freq_hop_flag == 0) re_offset[1] = re_offset[0];
  else {
    re_offset[1] = 12*(pucch_pdu->second_hop_prb+pucch_pdu->bwp_start) + frame_parms->first_carrier_offset;
    if (re_offset[1]>= frame_parms->ofdm_symbol_size) 
      re_offset[1]-=frame_parms->ofdm_symbol_size;
  }
  AssertFatal(pucch_pdu->prb_size*pucch_pdu->nr_of_symbols > 1,"number of PRB*SYMB (%d,%d)< 2",
	      pucch_pdu->prb_size,pucch_pdu->nr_of_symbols);

  int Prx = gNB->gNB_config.carrier_config.num_rx_ant.value;
  int Prx2 = (Prx==1)?2:Prx;
  // use 2 for Nb antennas in case of single antenna to allow the following allocations
  int prb_size_ext = pucch_pdu->prb_size+(pucch_pdu->prb_size&1);
  int16_t r_re_ext[Prx2][2][8*prb_size_ext] __attribute__((aligned(32)));
  int16_t r_im_ext[Prx2][2][8*prb_size_ext] __attribute__((aligned(32)));
  int16_t r_re_ext2[Prx2][2][8*prb_size_ext] __attribute__((aligned(32)));
  int16_t r_im_ext2[Prx2][2][8*prb_size_ext] __attribute__((aligned(32)));
  int16_t rd_re_ext[Prx2][2][4*prb_size_ext] __attribute__((aligned(32)));
  int16_t rd_im_ext[Prx2][2][4*prb_size_ext] __attribute__((aligned(32)));
  int16_t *r_re_ext_p,*r_im_ext_p,*rd_re_ext_p,*rd_im_ext_p;

  int nb_re_pucch = 12*pucch_pdu->prb_size;

  int16_t rp[Prx2][2][nb_re_pucch*2],*tmp_rp;
  __m64 dmrs_re,dmrs_im;

  for (int aa=0;aa<Prx;aa++){
    for (int symb=0;symb<pucch_pdu->nr_of_symbols;symb++) { 
      tmp_rp = ((int16_t *)&rxdataF[aa][(l2+symb)*frame_parms->ofdm_symbol_size]);
      
      if (re_offset[symb] + nb_re_pucch < frame_parms->ofdm_symbol_size) {
        memcpy1((void*)rp[aa][symb],(void*)&tmp_rp[re_offset[symb]*2],nb_re_pucch*sizeof(int32_t));
      }
      else {
        int neg_length = frame_parms->ofdm_symbol_size-re_offset[symb];
        int pos_length = nb_re_pucch-neg_length;
        memcpy1((void*)rp[aa][symb],(void*)&tmp_rp[re_offset[symb]*2],neg_length*sizeof(int32_t));
        memcpy1((void*)&rp[aa][symb][neg_length*2],(void*)tmp_rp,pos_length*sizeof(int32_t));
      }
    }
  }
#ifdef DEBUG_NR_PUCCH_RX
  printf("Decoding pucch2 for %d symbols, %d PRB\n",pucch_pdu->nr_of_symbols,pucch_pdu->prb_size);
#endif

  int nc_group_size=1; // 2 PRB
  int ngroup = prb_size_ext/nc_group_size/2;
  int32_t corr32_re[2][ngroup][Prx2],corr32_im[2][ngroup][Prx2];
  for (int aa=0;aa<Prx;aa++) 
    for (int group=0;group<ngroup;group++) { 
      corr32_re[0][group][aa]=0; corr32_im[0][group][aa]=0; 
      corr32_re[1][group][aa]=0; corr32_im[1][group][aa]=0;
    }

  //  AssertFatal((pucch_pdu->prb_size&1) == 0,"prb_size %d is not a multiple of 2\n",pucch_pdu->prb_size);
  if ((pucch_pdu->prb_size&1) > 0) { // if the number of PRBs is odd
    for (int symb=0; symb<pucch_pdu->nr_of_symbols;symb++) {
      for (int aa=0;aa<Prx;aa++) {
        memset(&r_re_ext[aa][symb][8*pucch_pdu->prb_size],0,8*pucch_pdu->prb_size*sizeof(int16_t));
        memset(&r_im_ext[aa][symb][8*pucch_pdu->prb_size],0,8*pucch_pdu->prb_size*sizeof(int16_t));
        memset(&rd_re_ext[aa][symb][4*pucch_pdu->prb_size],0,8*pucch_pdu->prb_size*sizeof(int16_t));
        memset(&rd_im_ext[aa][symb][4*pucch_pdu->prb_size],0,8*pucch_pdu->prb_size*sizeof(int16_t));
      }
    }
  }
  for (int symb=0; symb<pucch_pdu->nr_of_symbols;symb++) {
    // 24 REs contains 48x16-bit, so 6x8x16-bit 
    for (int prb=0;prb<pucch_pdu->prb_size;prb+=2) {
      for (int aa=0;aa<Prx;aa++) {
        r_re_ext_p=&r_re_ext[aa][symb][8*prb];
        r_im_ext_p=&r_im_ext[aa][symb][8*prb];
        rd_re_ext_p=&rd_re_ext[aa][symb][4*prb];
        rd_im_ext_p=&rd_im_ext[aa][symb][4*prb];
        
        for (int idx=0; idx<8; idx++) {
          r_re_ext_p[idx<<1]=rp[aa][symb][prb*24+6*idx];
          r_im_ext_p[idx<<1]=rp[aa][symb][prb*24+1+6*idx];
          rd_re_ext_p[idx]=rp[aa][symb][prb*24+2+6*idx];
          rd_im_ext_p[idx]=rp[aa][symb][prb*24+3+6*idx];
          r_re_ext_p[1+(idx<<1)]=rp[aa][symb][prb*24+4+6*idx];
          r_im_ext_p[1+(idx<<1)]=rp[aa][symb][prb*24+5+6*idx];
        }
        
#ifdef DEBUG_NR_PUCCH_RX
        for (int i=0;i<8;i++) printf("Ant %d PRB %d dmrs[%d] -> (%d,%d)\n",aa,prb+(i>>2),i,rd_re_ext_p[i],rd_im_ext_p[i]);
        for (int i=0;i<16;i++) printf("Ant %d PRB %d data[%d] -> (%d,%d)\n",aa,prb+(i>>3),i,r_re_ext_p[i],r_im_ext_p[i]);
#endif
      } // aa
    } // prb
    
    // first compute DMRS component
    
    uint32_t x1, x2, s=0;
    x2 = (((1<<17)*((14*slot) + (pucch_pdu->start_symbol_index+symb) + 1)*((2*pucch_pdu->dmrs_scrambling_id) + 1)) + (2*pucch_pdu->dmrs_scrambling_id))%(1U<<31); // c_init calculation according to TS38.211 subclause
#ifdef DEBUG_NR_PUCCH_RX
    printf("slot %d, start_symbol_index %d, symbol %d, dmrs_scrambling_id %d\n",
           slot,pucch_pdu->start_symbol_index,symb,pucch_pdu->dmrs_scrambling_id);
#endif
    int reset = 1;
    for (int i=0; i<=(pucch_pdu->prb_start>>2); i++) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
    }
    
    for (int group=0;group<ngroup;group++) {
      // each group has 8*nc_group_size elements, compute 1 complex correlation with DMRS per group
      // non-coherent combining across groups
      dmrs_re = byte2m64_re[((uint8_t*)&s)[(group&1)<<1]];
      dmrs_im = byte2m64_im[((uint8_t*)&s)[(group&1)<<1]];
#ifdef DEBUG_NR_PUCCH_RX
      printf("Group %d: s %x x2 %x ((%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
             group,
             ((uint16_t*)&s)[0],x2,
             ((int16_t*)&dmrs_re)[0],((int16_t*)&dmrs_im)[0],    
             ((int16_t*)&dmrs_re)[1],((int16_t*)&dmrs_im)[1],    
             ((int16_t*)&dmrs_re)[2],((int16_t*)&dmrs_im)[2],    
             ((int16_t*)&dmrs_re)[3],((int16_t*)&dmrs_im)[3]);   
#endif
      for (int aa=0;aa<Prx;aa++) {
        rd_re_ext_p=&rd_re_ext[aa][symb][8*group];
        rd_im_ext_p=&rd_im_ext[aa][symb][8*group];
        
#ifdef DEBUG_NR_PUCCH_RX
        printf("Group %d: rd ((%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
               group,
               rd_re_ext_p[0],rd_im_ext_p[0],
               rd_re_ext_p[1],rd_im_ext_p[1],
               rd_re_ext_p[2],rd_im_ext_p[2],
               rd_re_ext_p[3],rd_im_ext_p[3]);
#endif
        corr32_re[symb][group][aa]+=(rd_re_ext_p[0]*((int16_t*)&dmrs_re)[0] + rd_im_ext_p[0]*((int16_t*)&dmrs_im)[0]); 
        corr32_im[symb][group][aa]+=(-rd_re_ext_p[0]*((int16_t*)&dmrs_im)[0] + rd_im_ext_p[0]*((int16_t*)&dmrs_re)[0]); 
        corr32_re[symb][group][aa]+=(rd_re_ext_p[1]*((int16_t*)&dmrs_re)[1] + rd_im_ext_p[1]*((int16_t*)&dmrs_im)[1]); 
        corr32_im[symb][group][aa]+=(-rd_re_ext_p[1]*((int16_t*)&dmrs_im)[1] + rd_im_ext_p[1]*((int16_t*)&dmrs_re)[1]); 
        corr32_re[symb][group][aa]+=(rd_re_ext_p[2]*((int16_t*)&dmrs_re)[2] + rd_im_ext_p[2]*((int16_t*)&dmrs_im)[2]); 
        corr32_im[symb][group][aa]+=(-rd_re_ext_p[2]*((int16_t*)&dmrs_im)[2] + rd_im_ext_p[2]*((int16_t*)&dmrs_re)[2]); 
        corr32_re[symb][group][aa]+=(rd_re_ext_p[3]*((int16_t*)&dmrs_re)[3] + rd_im_ext_p[3]*((int16_t*)&dmrs_im)[3]); 
        corr32_im[symb][group][aa]+=(-rd_re_ext_p[3]*((int16_t*)&dmrs_im)[3] + rd_im_ext_p[3]*((int16_t*)&dmrs_re)[3]); 
      }
      dmrs_re = byte2m64_re[((uint8_t*)&s)[1+((group&1)<<1)]];
      dmrs_im = byte2m64_im[((uint8_t*)&s)[1+((group&1)<<1)]];
#ifdef DEBUG_NR_PUCCH_RX
      printf("Group %d: s %x ((%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
             group,
             ((uint16_t*)&s)[1],
             ((int16_t*)&dmrs_re)[0],((int16_t*)&dmrs_im)[0],    
             ((int16_t*)&dmrs_re)[1],((int16_t*)&dmrs_im)[1],    
             ((int16_t*)&dmrs_re)[2],((int16_t*)&dmrs_im)[2],    
             ((int16_t*)&dmrs_re)[3],((int16_t*)&dmrs_im)[3]);
#endif
      for (int aa=0;aa<Prx;aa++) {
        rd_re_ext_p=&rd_re_ext[aa][symb][8*group];
        rd_im_ext_p=&rd_im_ext[aa][symb][8*group];
#ifdef DEBUG_NR_PUCCH_RX
        printf("Group %d: rd ((%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
               group,
               rd_re_ext_p[4],rd_im_ext_p[4],
               rd_re_ext_p[5],rd_im_ext_p[5],
               rd_re_ext_p[6],rd_im_ext_p[6],
               rd_re_ext_p[7],rd_im_ext_p[7]);
#endif
        corr32_re[symb][group][aa]+=(rd_re_ext_p[4]*((int16_t*)&dmrs_re)[0] + rd_im_ext_p[4]*((int16_t*)&dmrs_im)[0]); 
        corr32_im[symb][group][aa]+=(-rd_re_ext_p[4]*((int16_t*)&dmrs_im)[0] + rd_im_ext_p[4]*((int16_t*)&dmrs_re)[0]); 
        corr32_re[symb][group][aa]+=(rd_re_ext_p[5]*((int16_t*)&dmrs_re)[1] + rd_im_ext_p[5]*((int16_t*)&dmrs_im)[1]); 
        corr32_im[symb][group][aa]+=(-rd_re_ext_p[5]*((int16_t*)&dmrs_im)[1] + rd_im_ext_p[5]*((int16_t*)&dmrs_re)[1]); 
        corr32_re[symb][group][aa]+=(rd_re_ext_p[6]*((int16_t*)&dmrs_re)[2] + rd_im_ext_p[6]*((int16_t*)&dmrs_im)[2]); 
        corr32_im[symb][group][aa]+=(-rd_re_ext_p[6]*((int16_t*)&dmrs_im)[2] + rd_im_ext_p[6]*((int16_t*)&dmrs_re)[2]); 
        corr32_re[symb][group][aa]+=(rd_re_ext_p[7]*((int16_t*)&dmrs_re)[3] + rd_im_ext_p[7]*((int16_t*)&dmrs_im)[3]); 
        corr32_im[symb][group][aa]+=(-rd_re_ext_p[7]*((int16_t*)&dmrs_im)[3] + rd_im_ext_p[7]*((int16_t*)&dmrs_re)[3]); 
        /*	corr32_re[group][aa]>>=5;
                corr32_im[group][aa]>>=5;*/
#ifdef DEBUG_NR_PUCCH_RX
        printf("Group %d: corr32 (%d,%d)\n",group,corr32_re[symb][group][aa],corr32_im[symb][group][aa]);
#endif
      } //aa    
      
      if ((group&1) == 1) s = lte_gold_generic(&x1, &x2, 0);
    } // group
  } // symb

  uint32_t x1, x2, s=0;  
  // unscrambling
  x2 = ((pucch_pdu->rnti)<<15)+pucch_pdu->data_scrambling_id;
  s = lte_gold_generic(&x1, &x2, 1);
#ifdef DEBUG_NR_PUCCH_RX
  printf("x2 %x, s %x\n",x2,s);
#endif
  for (int symb=0;symb<pucch_pdu->nr_of_symbols;symb++) {
    __m64 c_re0,c_im0,c_re1,c_im1,c_re2,c_im2,c_re3,c_im3;
    int re_off=0;
    for (int prb=0;prb<prb_size_ext;prb+=2,re_off+=16) {
      c_re0 = byte2m64_re[((uint8_t*)&s)[0]];
      c_im0 = byte2m64_im[((uint8_t*)&s)[0]];
      c_re1 = byte2m64_re[((uint8_t*)&s)[1]];
      c_im1 = byte2m64_im[((uint8_t*)&s)[1]];
      c_re2 = byte2m64_re[((uint8_t*)&s)[2]];
      c_im2 = byte2m64_im[((uint8_t*)&s)[2]];
      c_re3 = byte2m64_re[((uint8_t*)&s)[3]];
      c_im3 = byte2m64_im[((uint8_t*)&s)[3]];
      
      for (int aa=0;aa<Prx;aa++) {
#ifdef DEBUG_NR_PUCCH_RX
        printf("prb %d: rd ((%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
               prb,
               r_re_ext[aa][symb][re_off],r_im_ext[aa][symb][re_off],
               r_re_ext[aa][symb][re_off+1],r_im_ext[aa][symb][re_off+1],
               r_re_ext[aa][symb][re_off+2],r_im_ext[aa][symb][re_off+2],
               r_re_ext[aa][symb][re_off+3],r_im_ext[aa][symb][re_off+3],
               r_re_ext[aa][symb][re_off+4],r_im_ext[aa][symb][re_off+4],
               r_re_ext[aa][symb][re_off+5],r_im_ext[aa][symb][re_off+5],
               r_re_ext[aa][symb][re_off+6],r_im_ext[aa][symb][re_off+6],
               r_re_ext[aa][symb][re_off+7],r_im_ext[aa][symb][re_off+7]);
        printf("prb %d (%x): c ((%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
               prb,s,
               ((int16_t*)&c_re0)[0],((int16_t*)&c_im0)[0],
               ((int16_t*)&c_re0)[1],((int16_t*)&c_im0)[1],
               ((int16_t*)&c_re0)[2],((int16_t*)&c_im0)[2],
               ((int16_t*)&c_re0)[3],((int16_t*)&c_im0)[3],
               ((int16_t*)&c_re1)[0],((int16_t*)&c_im1)[0],
               ((int16_t*)&c_re1)[1],((int16_t*)&c_im1)[1],
               ((int16_t*)&c_re1)[2],((int16_t*)&c_im1)[2],
               ((int16_t*)&c_re1)[3],((int16_t*)&c_im1)[3]
               );
        printf("prb %d: rd ((%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
               prb+1,
               r_re_ext[aa][symb][re_off+8],r_im_ext[aa][symb][re_off+8],
               r_re_ext[aa][symb][re_off+9],r_im_ext[aa][symb][re_off+9],
               r_re_ext[aa][symb][re_off+10],r_im_ext[aa][symb][re_off+10],
               r_re_ext[aa][symb][re_off+11],r_im_ext[aa][symb][re_off+11],
               r_re_ext[aa][symb][re_off+12],r_im_ext[aa][symb][re_off+12],
               r_re_ext[aa][symb][re_off+13],r_im_ext[aa][symb][re_off+13],
               r_re_ext[aa][symb][re_off+14],r_im_ext[aa][symb][re_off+14],
               r_re_ext[aa][symb][re_off+15],r_im_ext[aa][symb][re_off+15]);
        printf("prb %d (%x): c ((%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
               prb+1,s,
               ((int16_t*)&c_re2)[0],((int16_t*)&c_im2)[0],
               ((int16_t*)&c_re2)[1],((int16_t*)&c_im2)[1],
               ((int16_t*)&c_re2)[2],((int16_t*)&c_im2)[2],
               ((int16_t*)&c_re2)[3],((int16_t*)&c_im2)[3],
               ((int16_t*)&c_re3)[0],((int16_t*)&c_im3)[0],
               ((int16_t*)&c_re3)[1],((int16_t*)&c_im3)[1],
               ((int16_t*)&c_re3)[2],((int16_t*)&c_im3)[2],
               ((int16_t*)&c_re3)[3],((int16_t*)&c_im3)[3]
               );
#endif
        
        ((__m64*)&r_re_ext2[aa][symb][re_off])[0] = _mm_mullo_pi16(((__m64*)&r_re_ext[aa][symb][re_off])[0],c_im0);
        ((__m64*)&r_re_ext[aa][symb][re_off])[0] = _mm_mullo_pi16(((__m64*)&r_re_ext[aa][symb][re_off])[0],c_re0);
        ((__m64*)&r_im_ext2[aa][symb][re_off])[0] = _mm_mullo_pi16(((__m64*)&r_im_ext[aa][symb][re_off])[0],c_re0);
        ((__m64*)&r_im_ext[aa][symb][re_off])[0] = _mm_mullo_pi16(((__m64*)&r_im_ext[aa][symb][re_off])[0],c_im0);
        
        ((__m64*)&r_re_ext2[aa][symb][re_off])[1] = _mm_mullo_pi16(((__m64*)&r_re_ext[aa][symb][re_off])[1],c_im1);
        ((__m64*)&r_re_ext[aa][symb][re_off])[1] = _mm_mullo_pi16(((__m64*)&r_re_ext[aa][symb][re_off])[1],c_re1);
        ((__m64*)&r_im_ext2[aa][symb][re_off])[1] = _mm_mullo_pi16(((__m64*)&r_im_ext[aa][symb][re_off])[1],c_re1);
        ((__m64*)&r_im_ext[aa][symb][re_off])[1] = _mm_mullo_pi16(((__m64*)&r_im_ext[aa][symb][re_off])[1],c_im1);
        
        ((__m64*)&r_re_ext2[aa][symb][re_off])[2] = _mm_mullo_pi16(((__m64*)&r_re_ext[aa][symb][re_off])[2],c_im2);
        ((__m64*)&r_re_ext[aa][symb][re_off])[2] = _mm_mullo_pi16(((__m64*)&r_re_ext[aa][symb][re_off])[2],c_re2);
        ((__m64*)&r_im_ext2[aa][symb][re_off])[2] = _mm_mullo_pi16(((__m64*)&r_im_ext[aa][symb][re_off])[2],c_re2);
        ((__m64*)&r_im_ext[aa][symb][re_off])[2] = _mm_mullo_pi16(((__m64*)&r_im_ext[aa][symb][re_off])[2],c_im2);
        
        ((__m64*)&r_re_ext2[aa][symb][re_off])[3] = _mm_mullo_pi16(((__m64*)&r_re_ext[aa][symb][re_off])[3],c_im3);
        ((__m64*)&r_re_ext[aa][symb][re_off])[3] = _mm_mullo_pi16(((__m64*)&r_re_ext[aa][symb][re_off])[3],c_re3);
        ((__m64*)&r_im_ext2[aa][symb][re_off])[3] = _mm_mullo_pi16(((__m64*)&r_im_ext[aa][symb][re_off])[3],c_re3);
        ((__m64*)&r_im_ext[aa][symb][re_off])[3] = _mm_mullo_pi16(((__m64*)&r_im_ext[aa][symb][re_off])[3],c_im3);
        
#ifdef DEBUG_NR_PUCCH_RX
        printf("prb %d: r ((%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
               prb,
               r_re_ext[aa][symb][re_off],r_im_ext[aa][symb][re_off],
               r_re_ext[aa][symb][re_off+1],r_im_ext[aa][symb][re_off+1],
               r_re_ext[aa][symb][re_off+2],r_im_ext[aa][symb][re_off+2],
               r_re_ext[aa][symb][re_off+3],r_im_ext[aa][symb][re_off+3],
               r_re_ext[aa][symb][re_off+4],r_im_ext[aa][symb][re_off+4],
               r_re_ext[aa][symb][re_off+5],r_im_ext[aa][symb][re_off+5],
               r_re_ext[aa][symb][re_off+6],r_im_ext[aa][symb][re_off+6],
               r_re_ext[aa][symb][re_off+7],r_im_ext[aa][symb][re_off+7]);
        printf("prb %d: r ((%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
               prb+1,
               r_re_ext[aa][symb][re_off+8],r_im_ext[aa][symb][re_off+8],
               r_re_ext[aa][symb][re_off+9],r_im_ext[aa][symb][re_off+9],
               r_re_ext[aa][symb][re_off+10],r_im_ext[aa][symb][re_off+10],
               r_re_ext[aa][symb][re_off+11],r_im_ext[aa][symb][re_off+11],
               r_re_ext[aa][symb][re_off+12],r_im_ext[aa][symb][re_off+12],
               r_re_ext[aa][symb][re_off+13],r_im_ext[aa][symb][re_off+13],
               r_re_ext[aa][symb][re_off+14],r_im_ext[aa][symb][re_off+14],
               r_re_ext[aa][symb][re_off+15],r_im_ext[aa][symb][re_off+15]);
#endif      
      }
      s = lte_gold_generic(&x1, &x2, 0);
#ifdef DEBUG_NR_PUCCH_RX
      printf("\n");
#endif
    }
  } //symb
  int nb_bit = pucch_pdu->bit_len_harq+pucch_pdu->sr_flag+pucch_pdu->bit_len_csi_part1+pucch_pdu->bit_len_csi_part2;
  AssertFatal(nb_bit > 2  && nb_bit< 65,"illegal length (%d : %d,%d,%d,%d)\n",nb_bit,pucch_pdu->bit_len_harq,pucch_pdu->sr_flag,pucch_pdu->bit_len_csi_part1,pucch_pdu->bit_len_csi_part2);

  uint64_t decodedPayload[2];
  uint8_t corr_dB;
  int decoderState=2;
  if (nb_bit < 12) { // short blocklength case
    __m256i *rp_re[Prx2][2];
    __m256i *rp2_re[Prx2][2];
    __m256i *rp_im[Prx2][2];
    __m256i *rp2_im[Prx2][2];
    for (int aa=0;aa<Prx;aa++) {
      for (int symb=0;symb<pucch_pdu->nr_of_symbols;symb++) {
        rp_re[aa][symb] = (__m256i*)r_re_ext[aa][symb];
        rp_im[aa][symb] = (__m256i*)r_im_ext[aa][symb];
        rp2_re[aa][symb] = (__m256i*)r_re_ext2[aa][symb];
        rp2_im[aa][symb] = (__m256i*)r_im_ext2[aa][symb];
      }
    }
    __m256i prod_re[Prx2],prod_im[Prx2];
    int64_t corr=0;
    int cw_ML=0;
    
    
    for (int cw=0;cw<1<<nb_bit;cw++) {
#ifdef DEBUG_NR_PUCCH_RX
      printf("cw %d:",cw);
      for (int i=0;i<32;i+=2) {
	printf("%d,%d,",
	       ((int16_t*)&pucch2_lut[nb_bit-3][cw<<1])[i>>1],
	       ((int16_t*)&pucch2_lut[nb_bit-3][cw<<1])[1+(i>>1)]);
      }
      printf("\n");
#endif
      int64_t corr_tmp = 0;

      for (int symb=0;symb<pucch_pdu->nr_of_symbols;symb++) {
        for (int group=0;group<ngroup;group++) {
          // do complex correlation
          for (int aa=0;aa<Prx;aa++) {
            prod_re[aa] = /*_mm256_srai_epi16(*/_mm256_adds_epi16(_mm256_mullo_epi16(pucch2_lut[nb_bit-3][cw<<1],rp_re[aa][symb][group]),
                                                                  _mm256_mullo_epi16(pucch2_lut[nb_bit-3][(cw<<1)+1],rp_im[aa][symb][group]))/*,5)*/;
            prod_im[aa] = /*_mm256_srai_epi16(*/_mm256_subs_epi16(_mm256_mullo_epi16(pucch2_lut[nb_bit-3][cw<<1],rp2_im[aa][symb][group]),
                                                                  _mm256_mullo_epi16(pucch2_lut[nb_bit-3][(cw<<1)+1],rp2_re[aa][symb][group]))/*,5)*/;
#ifdef DEBUG_NR_PUCCH_RX
            printf("prod_re[%d] => (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\n",aa,
                   ((int16_t*)&prod_re[aa])[0],((int16_t*)&prod_re[aa])[1],((int16_t*)&prod_re[aa])[2],((int16_t*)&prod_re[aa])[3],
                   ((int16_t*)&prod_re[aa])[4],((int16_t*)&prod_re[aa])[5],((int16_t*)&prod_re[aa])[6],((int16_t*)&prod_re[aa])[7],
                   ((int16_t*)&prod_re[aa])[8],((int16_t*)&prod_re[aa])[9],((int16_t*)&prod_re[aa])[10],((int16_t*)&prod_re[aa])[11],
                   ((int16_t*)&prod_re[aa])[12],((int16_t*)&prod_re[aa])[13],((int16_t*)&prod_re[aa])[14],((int16_t*)&prod_re[aa])[15]);
            printf("prod_im[%d] => (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\n",aa,
                   ((int16_t*)&prod_im[aa])[0],((int16_t*)&prod_im[aa])[1],((int16_t*)&prod_im[aa])[2],((int16_t*)&prod_im[aa])[3],
                   ((int16_t*)&prod_im[aa])[4],((int16_t*)&prod_im[aa])[5],((int16_t*)&prod_im[aa])[6],((int16_t*)&prod_im[aa])[7],
                   ((int16_t*)&prod_im[aa])[8],((int16_t*)&prod_im[aa])[9],((int16_t*)&prod_im[aa])[10],((int16_t*)&prod_im[aa])[11],
                   ((int16_t*)&prod_im[aa])[12],((int16_t*)&prod_im[aa])[13],((int16_t*)&prod_im[aa])[14],((int16_t*)&prod_im[aa])[15]);
            
#endif
            prod_re[aa] = _mm256_hadds_epi16(prod_re[aa],prod_re[aa]);// 0+1
            prod_im[aa] = _mm256_hadds_epi16(prod_im[aa],prod_im[aa]);
            prod_re[aa] = _mm256_hadds_epi16(prod_re[aa],prod_re[aa]);// 0+1+2+3
            prod_im[aa] = _mm256_hadds_epi16(prod_im[aa],prod_im[aa]);
            prod_re[aa] = _mm256_hadds_epi16(prod_re[aa],prod_re[aa]);// 0+1+2+3+4+5+6+7
            prod_im[aa] = _mm256_hadds_epi16(prod_im[aa],prod_im[aa]);
            prod_re[aa] = _mm256_hadds_epi16(prod_re[aa],prod_re[aa]);// 0+1+2+3+4+5+6+7+8+9+10+11+12+13+14+15
            prod_im[aa] = _mm256_hadds_epi16(prod_im[aa],prod_im[aa]);
          }
          int64_t corr_re=0,corr_im=0;
          
          
          for (int aa=0;aa<Prx;aa++) {
#ifdef DEBUG_NR_PUCCH_RX
            printf("pucch2 cw %d group %d aa %d: (%d,%d)+(%d,%d) = (%d,%d)\n",cw,group,aa,
              corr32_re[symb][group][aa],corr32_im[symb][group][aa],
              ((int16_t*)(&prod_re[aa]))[0],
              ((int16_t*)(&prod_im[aa]))[0],
              corr32_re[symb][group][aa]+((int16_t*)(&prod_re[aa]))[0],
              corr32_im[symb][group][aa]+((int16_t*)(&prod_im[aa]))[0]);
            
#endif
            LOG_D(PHY,"pucch2 cw %d group %d aa %d: (%d,%d)+(%d,%d) = (%d,%d)\n",cw,group,aa,
              corr32_re[symb][group][aa],corr32_im[symb][group][aa],
              ((int16_t*)(&prod_re[aa]))[0],
              ((int16_t*)(&prod_im[aa]))[0],
              corr32_re[symb][group][aa]+((int16_t*)(&prod_re[aa]))[0],
              corr32_im[symb][group][aa]+((int16_t*)(&prod_im[aa]))[0]);
            
            corr_re = ( corr32_re[symb][group][aa]+((int16_t*)(&prod_re[aa]))[0]);
            corr_im = ( corr32_im[symb][group][aa]+((int16_t*)(&prod_im[aa]))[0]);
            
            corr_tmp += corr_re*corr_re + corr_im*corr_im;	
          } // aa loop
        }// group loop
      } // symb loop 
      if (corr_tmp > corr) {
         corr = corr_tmp;
         cw_ML=cw;
      }
    } // cw loop
    corr_dB = dB_fixed64((uint64_t)corr);
#ifdef DEBUG_NR_PUCCH_RX
    printf("cw_ML %d, metric %d dB\n",cw_ML,corr_dB);
#endif
    LOG_D(PHY,"cw_ML %d, metric %d dB\n",cw_ML,corr_dB);
    decodedPayload[0]=(uint64_t)cw_ML;
  }
  else { // polar coded case

    t_nrPolar_params *currentPtr = nr_polar_params(2,nb_bit,pucch_pdu->prb_size,1,&gNB->uci_polarParams);
    __m64 *rp_re[Prx2][2];
    __m64 *rp2_re[Prx2][2];
    __m64 *rp_im[Prx2][2];
    __m64 *rp2_im[Prx2][2];
    __m128i llrs[pucch_pdu->prb_size*2*pucch_pdu->nr_of_symbols];

    for (int aa=0;aa<Prx;aa++) {
      for (int symb=0;symb<pucch_pdu->nr_of_symbols;symb++) {
        rp_re[aa][symb] = (__m64*)r_re_ext[aa][symb];
        rp_im[aa][symb] = (__m64*)r_im_ext[aa][symb];
        rp2_re[aa][symb] = (__m64*)r_re_ext2[aa][symb];
        rp2_im[aa][symb] = (__m64*)r_im_ext2[aa][symb];
      }
    }
    __m64 prod_re[Prx2],prod_im[Prx2];

#ifdef DEBUG_NR_PUCCH_RX
    for (int cw=0;cw<16;cw++) {

      printf("cw %d:",cw);
      for (int i=0;i<4;i++) {
	printf("%d,",
	       ((int16_t*)&pucch2_polar_4bit[cw])[i>>1]);
      }
      printf("\n");
    }
#endif
    
    // non-coherent LLR computation on groups of 4 REs (half-PRBs)
    int32_t corr_re,corr_im,corr_tmp;
    __m128i corr16,llr_num,llr_den;
    uint64_t corr = 0;
    for (int symb=0;symb<pucch_pdu->nr_of_symbols;symb++) {
      for (int half_prb=0;half_prb<(2*pucch_pdu->prb_size);half_prb++) {
        llr_num=_mm_set1_epi16(0);llr_den=_mm_set1_epi16(0);
        for (int cw=0;cw<256;cw++) {
          corr_tmp=0;
          for (int aa=0;aa<Prx;aa++) { 
            prod_re[aa] = _mm_srai_pi16(_mm_adds_pi16(_mm_mullo_pi16(pucch2_polar_4bit[cw&15],rp_re[aa][symb][half_prb]),
                                                      _mm_mullo_pi16(pucch2_polar_4bit[cw>>4],rp_im[aa][symb][half_prb])),5);
            prod_im[aa] = _mm_srai_pi16(_mm_subs_pi16(_mm_mullo_pi16(pucch2_polar_4bit[cw&15],rp2_im[aa][symb][half_prb]),
                                                      _mm_mullo_pi16(pucch2_polar_4bit[cw>>4],rp2_re[aa][symb][half_prb])),5);
            prod_re[aa] = _mm_hadds_pi16(prod_re[aa],prod_re[aa]);// 0+1
            prod_im[aa] = _mm_hadds_pi16(prod_im[aa],prod_im[aa]);
            prod_re[aa] = _mm_hadds_pi16(prod_re[aa],prod_re[aa]);// 0+1+2+3
            prod_im[aa] = _mm_hadds_pi16(prod_im[aa],prod_im[aa]);
            
            // this is for UL CQI measurement
            if (cw==0) corr += ((int64_t)corr32_re[symb][half_prb>>2][aa]*corr32_re[symb][half_prb>>2][aa])+
                         ((int64_t)corr32_im[symb][half_prb>>2][aa]*corr32_im[symb][half_prb>>2][aa]);
            
            
            corr_re = ( corr32_re[symb][half_prb>>2][aa]/(2*nc_group_size*4/2)+((int16_t*)(&prod_re[aa]))[0]);
            corr_im = ( corr32_im[symb][half_prb>>2][aa]/(2*nc_group_size*4/2)+((int16_t*)(&prod_im[aa]))[0]);
            corr_tmp += corr_re*corr_re + corr_im*corr_im;
            /*
              LOG_D(PHY,"pucch2 half_prb %d cw %d (%d,%d) aa %d: (%d,%d,%d,%d,%d,%d,%d,%d)x(%d,%d,%d,%d,%d,%d,%d,%d)  (%d,%d)+(%d,%d) = (%d,%d) => %d\n",
              half_prb,cw,cw&15,cw>>4,aa,
              ((int16_t*)&pucch2_polar_4bit[cw&15])[0],((int16_t*)&pucch2_polar_4bit[cw>>4])[0],
              ((int16_t*)&pucch2_polar_4bit[cw&15])[1],((int16_t*)&pucch2_polar_4bit[cw>>4])[1],
              ((int16_t*)&pucch2_polar_4bit[cw&15])[2],((int16_t*)&pucch2_polar_4bit[cw>>4])[2],
                ((int16_t*)&pucch2_polar_4bit[cw&15])[3],((int16_t*)&pucch2_polar_4bit[cw>>4])[3],
                ((int16_t*)&rp_re[aa][half_prb])[0],((int16_t*)&rp_im[aa][half_prb])[0],
                ((int16_t*)&rp_re[aa][half_prb])[1],((int16_t*)&rp_im[aa][half_prb])[1],
                ((int16_t*)&rp_re[aa][half_prb])[2],((int16_t*)&rp_im[aa][half_prb])[2],
                ((int16_t*)&rp_re[aa][half_prb])[3],((int16_t*)&rp_im[aa][half_prb])[3],
                corr32_re[half_prb>>2][aa]/(2*nc_group_size*4/2),corr32_im[half_prb>>2][aa]/(2*nc_group_size*4/2),
                ((int16_t*)(&prod_re[aa]))[0],
                ((int16_t*)(&prod_im[aa]))[0],
                corr_re,
                corr_im,
                corr_tmp);
          */
          }
          corr16 = _mm_set1_epi16((int16_t)(corr_tmp>>8));
	/*	
	LOG_D(PHY,"half_prb %d cw %d corr16 %d\n",half_prb,cw,corr_tmp>>8);
	*/
          llr_num = _mm_max_epi16(_mm_mullo_epi16(corr16,pucch2_polar_llr_num_lut[cw]),llr_num);
          llr_den = _mm_max_epi16(_mm_mullo_epi16(corr16,pucch2_polar_llr_den_lut[cw]),llr_den);
	/*
	LOG_D(PHY,"lut_num (%d,%d,%d,%d,%d,%d,%d,%d)\n",
	      ((int16_t*)&pucch2_polar_llr_num_lut[cw])[0],
	      ((int16_t*)&pucch2_polar_llr_num_lut[cw])[1],
	      ((int16_t*)&pucch2_polar_llr_num_lut[cw])[2],
	      ((int16_t*)&pucch2_polar_llr_num_lut[cw])[3],
	      ((int16_t*)&pucch2_polar_llr_num_lut[cw])[4],
	      ((int16_t*)&pucch2_polar_llr_num_lut[cw])[5],
	      ((int16_t*)&pucch2_polar_llr_num_lut[cw])[6],
	      ((int16_t*)&pucch2_polar_llr_num_lut[cw])[7]);
	
	LOG_D(PHY,"llr_num (%d,%d,%d,%d,%d,%d,%d,%d)\n",
	      ((int16_t*)&llr_num)[0],
	      ((int16_t*)&llr_num)[1],
	      ((int16_t*)&llr_num)[2],
	      ((int16_t*)&llr_num)[3],
	      ((int16_t*)&llr_num)[4],
	      ((int16_t*)&llr_num)[5],
	      ((int16_t*)&llr_num)[6],
	      ((int16_t*)&llr_num)[7]);
	LOG_D(PHY,"llr_den (%d,%d,%d,%d,%d,%d,%d,%d)\n",
	      ((int16_t*)&llr_den)[0],
	      ((int16_t*)&llr_den)[1],
	      ((int16_t*)&llr_den)[2],
	      ((int16_t*)&llr_den)[3],
	      ((int16_t*)&llr_den)[4],
	      ((int16_t*)&llr_den)[5],
	      ((int16_t*)&llr_den)[6],
	      ((int16_t*)&llr_den)[7]);
	*/	
        }
      // compute llrs
        llrs[half_prb + (symb*2*pucch_pdu->prb_size)] = _mm_subs_epi16(llr_num,llr_den);
        LOG_D(PHY,"llrs[%d] : (%d,%d,%d,%d,%d,%d,%d,%d)\n",
              half_prb,
              ((int16_t*)&llrs[half_prb])[0],
              ((int16_t*)&llrs[half_prb])[1],
              ((int16_t*)&llrs[half_prb])[2],
              ((int16_t*)&llrs[half_prb])[3],
              ((int16_t*)&llrs[half_prb])[4],
              ((int16_t*)&llrs[half_prb])[5],
              ((int16_t*)&llrs[half_prb])[6],
              ((int16_t*)&llrs[half_prb])[7]);
      } // half_prb
    } // symb
    // run polar decoder on llrs
    decoderState = polar_decoder_int16((int16_t*)llrs, decodedPayload, 0, currentPtr);
    LOG_D(PHY,"UCI decoderState %d, payload[0] %llu\n",decoderState,(unsigned long long)decodedPayload[0]);
    if (decoderState>0) decoderState=1;
    corr_dB = dB_fixed64(corr);
    LOG_D(PHY,"metric %d dB\n",corr_dB);
  }

  int re_off = (12*pucch_pdu->prb_start) + (12*pucch_pdu->bwp_start) + frame_parms->first_carrier_offset;
  // estimate CQI for MAC (from antenna port 0 only)
  int SNRtimes10 = dB_fixed_times10(signal_energy_nodc(&rxdataF[0][(l2*frame_parms->ofdm_symbol_size)+re_off],12*pucch_pdu->prb_size)) - (10*gNB->measurements.n0_power_tot_dB);
  int cqi,bit_left;
  if (SNRtimes10 < -640) cqi=0;
  else if (SNRtimes10 >  635) cqi=255;
  else cqi=(640+SNRtimes10)/5;

  uci_pdu->harq.harq_bit_len = pucch_pdu->bit_len_harq;
  uci_pdu->pduBitmap=0;
  uci_pdu->rnti=pucch_pdu->rnti;
  uci_pdu->handle=pucch_pdu->handle;
  uci_pdu->pucch_format=0;
  uci_pdu->ul_cqi=cqi;
  uci_pdu->timing_advance=0xffff; // currently not valid
  uci_pdu->rssi=1280 - (10*dB_fixed(32767*32767)-dB_fixed_times10(signal_energy_nodc(&rxdataF[0][(l2*frame_parms->ofdm_symbol_size)+re_off],12*pucch_pdu->prb_size)));
  if (pucch_pdu->bit_len_harq>0) {
    int harq_bytes=pucch_pdu->bit_len_harq>>3;
    if ((pucch_pdu->bit_len_harq&7) > 0) harq_bytes++;
    uci_pdu->pduBitmap|=2;
    uci_pdu->harq.harq_payload = (uint8_t*)malloc(harq_bytes);
    uci_pdu->harq.harq_crc = decoderState;
    int i=0;
    for (;i<harq_bytes-1;i++) {
      uci_pdu->harq.harq_payload[i] = decodedPayload[0] & 255;
      decodedPayload[0]>>=8;
    }
    bit_left = pucch_pdu->bit_len_harq-((harq_bytes-1)<<3);
    uci_pdu->harq.harq_payload[i] = decodedPayload[0] & ((1<<bit_left)-1);
    decodedPayload[0] >>= pucch_pdu->bit_len_harq;
  }
  
  if (pucch_pdu->sr_flag == 1) {
    uci_pdu->pduBitmap|=1;
    uci_pdu->sr.sr_bit_len = 1;
    uci_pdu->sr.sr_payload = malloc(1);
    uci_pdu->sr.sr_payload[0] = decodedPayload[0]&1;
    decodedPayload[0] = decodedPayload[0]>>1;
  }
  // csi
  if (pucch_pdu->bit_len_csi_part1>0) {
    uci_pdu->pduBitmap|=4;
    uci_pdu->csi_part1.csi_part1_bit_len=pucch_pdu->bit_len_csi_part1;
    int csi_part1_bytes=pucch_pdu->bit_len_csi_part1>>3;
    if ((pucch_pdu->bit_len_csi_part1&7) > 0) csi_part1_bytes++;
    uci_pdu->csi_part1.csi_part1_payload = (uint8_t*)malloc(csi_part1_bytes);
    uci_pdu->csi_part1.csi_part1_crc = decoderState;
    int i=0;
    for (;i<csi_part1_bytes-1;i++) {
      uci_pdu->csi_part1.csi_part1_payload[i] = decodedPayload[0] & 255;
      decodedPayload[0]>>=8;
    }
    bit_left = pucch_pdu->bit_len_csi_part1-((csi_part1_bytes-1)<<3);
    uci_pdu->csi_part1.csi_part1_payload[i] = decodedPayload[0] & ((1<<bit_left)-1);
    decodedPayload[0] >>= pucch_pdu->bit_len_csi_part1;
  }
  
  if (pucch_pdu->bit_len_csi_part2>0) {
    uci_pdu->pduBitmap|=8;
  }
}

void nr_dump_uci_stats(FILE *fd,PHY_VARS_gNB *gNB,int frame) {

   int strpos=0;
   char output[16384];

   for (int i=0;i<NUMBER_OF_NR_UCI_STATS_MAX;i++){
      if (gNB->uci_stats[i].rnti>0) {
         NR_gNB_UCI_STATS_t *uci_stats = &gNB->uci_stats[i];
         if (uci_stats->pucch0_sr_trials > 0)
             strpos+=sprintf(output+strpos,"UCI %d RNTI %x: pucch0_sr_trials %d, pucch0_n00 %d dB, pucch0_n01 %d dB, pucch0_sr_thres %d dB, current pucch1_stat0 %d dB, current pucch1_stat1 %d dB, positive SR count %d\n",
                             i,uci_stats->rnti,uci_stats->pucch0_sr_trials,uci_stats->pucch0_n00,uci_stats->pucch0_n01,uci_stats->pucch0_sr_thres,dB_fixed(uci_stats->current_pucch0_sr_stat0),dB_fixed(uci_stats->current_pucch0_sr_stat1),uci_stats->pucch0_positive_SR);
         if (uci_stats->pucch01_trials > 0)
            strpos+=sprintf(output+strpos,"UCI %d RNTI %x: pucch01_trials %d, pucch0_n00 %d dB, pucch0_n01 %d dB, pucch0_thres %d dB, current pucch0_stat0 %d dB, current pucch1_stat1 %d dB, pucch01_DTX %d\n",
                            i,uci_stats->rnti,uci_stats->pucch01_trials,uci_stats->pucch0_n01,uci_stats->pucch0_n01,uci_stats->pucch0_thres,dB_fixed(uci_stats->current_pucch0_stat0),dB_fixed(uci_stats->current_pucch0_stat1),uci_stats->pucch01_DTX);

         if (uci_stats->pucch02_trials > 0)
             strpos+=sprintf(output+strpos,"UCI %d RNTI %x: pucch01_trials %d, pucch0_n00 %d dB, pucch0_n01 %d dB, pucch0_thres %d dB, current pucch0_stat0 %d dB, current pucch0_stat1 %d dB, pucch01_DTX %d\n",
                             i,uci_stats->rnti,uci_stats->pucch02_trials,uci_stats->pucch0_n00,uci_stats->pucch0_n01,uci_stats->pucch0_thres,dB_fixed(uci_stats->current_pucch0_stat0),dB_fixed(uci_stats->current_pucch0_stat1),uci_stats->pucch02_DTX);

         if (uci_stats->pucch2_trials > 0)
           strpos+=sprintf(output+strpos,"UCI %d RNTI %x: pucch2_trials %d, pucch2_DTX %d\n",
                           i,uci_stats->rnti,
                           uci_stats->pucch2_trials,
                           uci_stats->pucch2_DTX);
       }
    }
    if (fd) fprintf(fd,"%s",output);
    else    printf("%s",output);
}
