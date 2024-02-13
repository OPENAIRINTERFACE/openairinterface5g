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
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_REFSIG/nr_refsig.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "nfapi/oai_integration/vendor_ext.h"

#include "T.h"

//#define DEBUG_NR_PUCCH_RX 1

void nr_fill_pucch(PHY_VARS_gNB *gNB,
                   int frame,
                   int slot,
                   nfapi_nr_pucch_pdu_t *pucch_pdu)
{

  if (NFAPI_MODE == NFAPI_MODE_PNF)
    gNB->pucch[0].active = 0; // check if ture in monolithic mode

  bool found = false;
  for (int i = 0; i < gNB->max_nb_pucch; i++) {
    NR_gNB_PUCCH_t *pucch = &gNB->pucch[i];
    if (pucch->active == 0) {
      pucch->frame = frame;
      pucch->slot = slot;
      pucch->active = 1;
      memcpy((void *)&pucch->pucch_pdu, (void *)pucch_pdu, sizeof(nfapi_nr_pucch_pdu_t));
      LOG_D(PHY,
            "Programming PUCCH[%d] for %d.%d, format %d, nb_harq %d, nb_sr %d, nb_csi %d\n",
            i,
            pucch->frame,
            pucch->slot,
            pucch->pucch_pdu.format_type,
            pucch->pucch_pdu.bit_len_harq,
            pucch->pucch_pdu.sr_flag,
            pucch->pucch_pdu.bit_len_csi_part1);
      found = true;
      break;
    }
  }
  AssertFatal(found, "PUCCH list is full\n");
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
  
static const int16_t idft12_re[12][12] = {
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

static const int16_t idft12_im[12][12] = {
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
//************************************************************************//
void nr_decode_pucch0(PHY_VARS_gNB *gNB,
                      int frame,
                      int slot,
                      nfapi_nr_uci_pucch_pdu_format_0_1_t *uci_pdu,
                      nfapi_nr_pucch_pdu_t *pucch_pdu)
{
  c16_t **rxdataF = gNB->common_vars.rxdataF;
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  int soffset = (slot & 3) * frame_parms->symbols_per_slot * frame_parms->ofdm_symbol_size;

  AssertFatal(pucch_pdu->bit_len_harq > 0 || pucch_pdu->sr_flag > 0,
              "Either bit_len_harq (%d) or sr_flag (%d) must be > 0\n",
              pucch_pdu->bit_len_harq,
              pucch_pdu->sr_flag);

  NR_gNB_PHY_STATS_t *phy_stats = get_phy_stats(gNB, pucch_pdu->rnti);
  AssertFatal(phy_stats != NULL, "phy_stats shouldn't be NULL\n");
  phy_stats->frame = frame;
  NR_gNB_UCI_STATS_t *uci_stats = &phy_stats->uci_stats;

  int nr_sequences;
  const uint8_t *mcs;
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

  LOG_D(PHY,"pucch0: nr_symbols %d, start_symbol %d, prb_start %d, second_hop_prb %d,  group_hop_flag %d, sequence_hop_flag %d, O_ACK %d, O_SR %d, mcs %d initial_cyclic_shift %d\n",
        pucch_pdu->nr_of_symbols,pucch_pdu->start_symbol_index,pucch_pdu->prb_start,pucch_pdu->second_hop_prb,pucch_pdu->group_hop_flag,pucch_pdu->sequence_hop_flag,pucch_pdu->bit_len_harq,
        pucch_pdu->sr_flag,mcs[0],pucch_pdu->initial_cyclic_shift);

  int cs_ind = get_pucch0_cs_lut_index(gNB,pucch_pdu);
  /*
   * Implement TS 38.211 Subclause 6.3.2.3.1 Sequence generation
   * Defining cyclic shift hopping TS 38.211 Subclause 6.3.2.2.2
   * in TS 38.213 Subclause 9.2.1 it is said that:
   * for PUCCH format 0 or PUCCH format 1, the index of the cyclic shift
   * is indicated by higher layer parameter PUCCH-F0-F1-initial-cyclic-shift
   */
  int prb_offset[2] = {pucch_pdu->bwp_start+pucch_pdu->prb_start, pucch_pdu->bwp_start+pucch_pdu->prb_start};

  pucch_GroupHopping_t pucch_GroupHopping = pucch_pdu->group_hop_flag + (pucch_pdu->sequence_hop_flag << 1);
  // the value of u,v (delta always 0 for PUCCH) has to be calculated according
  // to TS 38.211 Subclause 6.3.2.2.1
  uint8_t u[2] = {0}, v[2] = {0};
  nr_group_sequence_hopping(pucch_GroupHopping, pucch_pdu->hopping_id, 0, slot, u,
                            v); // calculating u and v value first hop
  LOG_D(PHY,"pucch0: u %d, v %d\n",u[0],v[0]);

  if (pucch_pdu->freq_hop_flag == 1) {
    nr_group_sequence_hopping(pucch_GroupHopping,
                              pucch_pdu->hopping_id,
                              1,
                              slot,
                              &u[1],
                              &v[1]); // calculating u and v value second hop
    LOG_D(PHY,"pucch0 second hop: u %d, v %d\n",u[1],v[1]);
    prb_offset[1] = pucch_pdu->bwp_start + pucch_pdu->second_hop_prb;
  }

  AssertFatal(pucch_pdu->nr_of_symbols < 3,"nr_of_symbols %d not allowed\n",pucch_pdu->nr_of_symbols);
  uint32_t re_offset[2] = {0};

  const int16_t *x_re[2],*x_im[2];
  x_re[0] = table_5_2_2_2_2_Re[u[0]];
  x_im[0] = table_5_2_2_2_2_Im[u[0]];
  x_re[1] = table_5_2_2_2_2_Re[u[1]];
  x_im[1] = table_5_2_2_2_2_Im[u[1]];

  c64_t xr[frame_parms->nb_antennas_rx][pucch_pdu->nr_of_symbols][12]  __attribute__((aligned(32)));
  int64_t xrtmag=0,xrtmag_next=0;
  uint8_t maxpos=0;
  uint8_t index=0;

  int nb_re_pucch = 12*pucch_pdu->prb_size;  // prb size is 1
  int signal_energy = 0, signal_energy_ant0 = 0;

  for (int l=0; l<pucch_pdu->nr_of_symbols; l++) {
    uint8_t l2 = l + pucch_pdu->start_symbol_index;

    re_offset[l] = (12*prb_offset[l]) + frame_parms->first_carrier_offset;
    if (re_offset[l]>= frame_parms->ofdm_symbol_size)
      re_offset[l]-=frame_parms->ofdm_symbol_size;

    for (int aa=0;aa<frame_parms->nb_antennas_rx;aa++) {
      c16_t rp[nb_re_pucch];
      memset(rp, 0, sizeof(rp));
      c16_t *tmp_rp = &rxdataF[aa][soffset + l2 * frame_parms->ofdm_symbol_size];
      if(re_offset[l] + nb_re_pucch > frame_parms->ofdm_symbol_size) {
        int neg_length = frame_parms->ofdm_symbol_size-re_offset[l];
        int pos_length = nb_re_pucch-neg_length;
        memcpy(rp, &tmp_rp[re_offset[l]], neg_length * sizeof(*tmp_rp));
        memcpy(&rp[neg_length], tmp_rp, pos_length * sizeof(*tmp_rp));
      }
      else
        memcpy(rp, &tmp_rp[re_offset[l]], nb_re_pucch * sizeof(*tmp_rp));

      for (int n = 0; n < nb_re_pucch; n++) {
        xr[aa][l][n].r = (int32_t)x_re[l][n] * rp[n].r + (int32_t)x_im[l][n] * rp[n].i;
        xr[aa][l][n].i = (int32_t)x_re[l][n] * rp[n].i - (int32_t)x_im[l][n] * rp[n].r;
#ifdef DEBUG_NR_PUCCH_RX
        printf("x (%d,%d), xr (%ld,%ld)\n", x_re[l][n], x_im[l][n], xr[aa][l][n].r, xr[aa][l][n].i);
#endif
      }
      int energ = signal_energy_nodc((int32_t *)rp, nb_re_pucch);
      signal_energy += energ;
      if (aa == 0)
        signal_energy_ant0 += energ;
    }
  }
  signal_energy /= (pucch_pdu->nr_of_symbols * frame_parms->nb_antennas_rx);
  int pucch_power_dBtimes10 = 10 * dB_fixed(signal_energy);

  //int32_t no_corr = 0;
  int seq_index = 0;

  for (int i = 0; i < nr_sequences; i++) {
    c64_t corr[frame_parms->nb_antennas_rx][2];
    for (int aa=0;aa<frame_parms->nb_antennas_rx;aa++) {
      for (int l=0;l<pucch_pdu->nr_of_symbols;l++) {
        seq_index = (pucch_pdu->initial_cyclic_shift+
                     mcs[i]+
                     gNB->pucch0_lut.lut[cs_ind][slot][l+pucch_pdu->start_symbol_index])%12;
#ifdef DEBUG_NR_PUCCH_RX
        printf("PUCCH symbol %d seq %d, seq_index %d, mcs %d\n",l,i,seq_index,mcs[i]);
#endif
        corr[aa][l]=(c64_t){0};
        for (int n = 0; n < 12; n++) {
          corr[aa][l].r += xr[aa][l][n].r * idft12_re[seq_index][n] + xr[aa][l][n].i * idft12_im[seq_index][n];
          corr[aa][l].i += xr[aa][l][n].r * idft12_im[seq_index][n] - xr[aa][l][n].i * idft12_re[seq_index][n];
        }
        corr[aa][l].r >>= 31;
        corr[aa][l].i >>= 31;
      }
    }
    LOG_D(PHY,"PUCCH IDFT[%d/%d] = (%ld,%ld)=>%f\n",
          mcs[i],seq_index,corr[0][0].r,corr[0][0].i,
          10*log10((double)squaredMod(corr[0][0])));
    if (pucch_pdu->nr_of_symbols==2)
      LOG_D(PHY,
            "PUCCH 2nd symbol IDFT[%d/%d] = (%ld,%ld)=>%f\n",
            mcs[i],
            seq_index,
            corr[0][1].r,
            corr[0][1].i,
            10 * log10((double)squaredMod(corr[0][1])));
    int64_t temp = 0;
    if (pucch_pdu->freq_hop_flag == 0) {
      if (pucch_pdu->nr_of_symbols == 1) { // non-coherent correlation
        for (int aa = 0; aa < frame_parms->nb_antennas_rx; aa++)
          temp += squaredMod(corr[aa][0]);
      } else {
        for (int aa = 0; aa < frame_parms->nb_antennas_rx; aa++) {
          c64_t corr2;
          csum(corr2, corr[aa][0], corr[aa][1]);
          // coherent combining of 2 symbols and then complex modulus for
          // single-frequency case
          temp += corr2.r * corr2.r + corr2.i * corr2.i;
        }
      }
    } else {
      // full non-coherent combining of 2 symbols for frequency-hopping case
      for (int aa = 0; aa < frame_parms->nb_antennas_rx; aa++)
        temp += squaredMod(corr[aa][0]) + squaredMod(corr[aa][1]);
    }

    if (temp>xrtmag) {
      xrtmag_next = xrtmag;
      xrtmag=temp;
      LOG_D(PHY,"Sequence %d xrtmag %ld xrtmag_next %ld\n", i, xrtmag, xrtmag_next);
      maxpos=i;
      uci_stats->current_pucch0_stat0 = 0;
      int64_t temp2=0,temp3=0;;
      for (int aa=0;aa<frame_parms->nb_antennas_rx;aa++) {
        temp2 += squaredMod(corr[aa][0]);
        if (pucch_pdu->nr_of_symbols==2)
          temp3 += squaredMod(corr[aa][1]);
      }
      uci_stats->current_pucch0_stat0= dB_fixed64(temp2);
      if ( pucch_pdu->nr_of_symbols==2)
        uci_stats->current_pucch0_stat1 = dB_fixed64(temp3);
    }
    else if (temp>xrtmag_next)
      xrtmag_next = temp;
  }

  int xrtmag_dBtimes10 = 10*(int)dB_fixed64(xrtmag/(12*pucch_pdu->nr_of_symbols));
  int xrtmag_next_dBtimes10 = 10*(int)dB_fixed64(xrtmag_next/(12*pucch_pdu->nr_of_symbols));
#ifdef DEBUG_NR_PUCCH_RX
  printf("PUCCH 0 : maxpos %d\n",maxpos);
#endif

  index=maxpos;
  uci_stats->pucch0_n00 = gNB->measurements.n0_subband_power_tot_dB[prb_offset[0]];
  uci_stats->pucch0_n01 = gNB->measurements.n0_subband_power_tot_dB[prb_offset[1]];
  LOG_D(PHY,"n00[%d] = %d, n01[%d] = %d\n",prb_offset[0],uci_stats->pucch0_n00,prb_offset[1],uci_stats->pucch0_n01);
  // estimate CQI for MAC (from antenna port 0 only)
  int max_n0 =
      max(gNB->measurements.n0_subband_power_tot_dB[prb_offset[0]], gNB->measurements.n0_subband_power_tot_dB[prb_offset[1]]);
  const int SNRtimes10 = pucch_power_dBtimes10 - (10 * max_n0);
  int cqi;
  if (SNRtimes10 < -640)
    cqi = 0;
  else if (SNRtimes10 > 635)
    cqi = 255;
  else
    cqi = (640 + SNRtimes10) / 5;

  uci_stats->pucch0_thres = gNB->pucch0_thres; /* + (10*max_n0);*/
  bool no_conf=false;
  if (nr_sequences>1) {
    if (/*xrtmag_dBtimes10 < (30+xrtmag_next_dBtimes10) ||*/ SNRtimes10 < gNB->pucch0_thres) {
      no_conf=true;
      LOG_D(PHY,
            "%d.%d PUCCH bad confidence: %d threshold, %d, %d, %d\n",
            frame,
            slot,
            uci_stats->pucch0_thres,
            SNRtimes10,
            xrtmag_dBtimes10,
            xrtmag_next_dBtimes10);
    }
  }
  gNB->bad_pucch += no_conf;
  // first bit of bitmap for sr presence and second bit for acknack presence
  uci_pdu->pduBitmap = pucch_pdu->sr_flag | ((pucch_pdu->bit_len_harq>0)<<1);
  uci_pdu->pucch_format = 0; // format 0
  uci_pdu->rnti = pucch_pdu->rnti;
  uci_pdu->ul_cqi = cqi;
  uci_pdu->timing_advance = 0xffff; // currently not valid
  uci_pdu->rssi = 1280 - (10 * dB_fixed(32767 * 32767)) - dB_fixed_times10(signal_energy_ant0);

  if (pucch_pdu->bit_len_harq==0) {
    uci_pdu->sr.sr_confidence_level = SNRtimes10 < uci_stats->pucch0_thres;
    uci_stats->pucch0_sr_trials++;
    if (xrtmag_dBtimes10>(10*max_n0+100)) {
      uci_pdu->sr.sr_indication = 1;
      uci_stats->pucch0_positive_SR++;
      LOG_D(PHY,"PUCCH0 got positive SR. Cumulative number of positive SR %d\n", uci_stats->pucch0_positive_SR);
    } else {
      uci_pdu->sr.sr_indication = 0;
    }
  }
  else if (pucch_pdu->bit_len_harq==1) {
    uci_pdu->harq.num_harq = 1;
    uci_pdu->harq.harq_confidence_level = no_conf;
    uci_pdu->harq.harq_list[0].harq_value = !(index&0x01);
    LOG_D(PHY,
          "[DLSCH/PDSCH/PUCCH] %d.%d HARQ %s with confidence level %s xrt_mag "
          "%d xrt_mag_next %d pucch_power_dBtimes10 %d n0 %d "
          "(%d,%d) pucch0_thres %d, "
          "cqi %d, SNRtimes10 %d, energy %f\n",
          frame,
          slot,
          uci_pdu->harq.harq_list[0].harq_value == 0 ? "ACK" : "NACK",
          uci_pdu->harq.harq_confidence_level == 0 ? "good" : "bad",
          xrtmag_dBtimes10,
          xrtmag_next_dBtimes10,
          pucch_power_dBtimes10,
          max_n0,
          uci_stats->pucch0_n00,
          uci_stats->pucch0_n01,
          uci_stats->pucch0_thres,
          cqi,
          SNRtimes10,
          10 * log10((double)signal_energy_ant0));

    if (pucch_pdu->sr_flag == 1) {
      uci_pdu->sr.sr_indication = (index>1);
      uci_pdu->sr.sr_confidence_level = no_conf;
      if(uci_pdu->sr.sr_indication == 1 && uci_pdu->sr.sr_confidence_level == 0) {
        uci_stats->pucch0_positive_SR++;
        LOG_D(PHY,"PUCCH0 got positive SR. Cumulative number of positive SR %d\n", uci_stats->pucch0_positive_SR);
      }
    }
    uci_stats->pucch01_trials++;
  }
  else {
    uci_pdu->harq.num_harq = 2;
    uci_pdu->harq.harq_confidence_level = no_conf;

    uci_pdu->harq.harq_list[1].harq_value = !(index&0x01);
    uci_pdu->harq.harq_list[0].harq_value = !((index>>1)&0x01);
    LOG_D(PHY,
          "[DLSCH/PDSCH/PUCCH] %d.%d HARQ values (%s, %s) with confidence level %s, xrt_mag %d xrt_mag_next %d pucch_power_dBtimes10 %d n0 %d (%d,%d) "
          "pucch0_thres %d, cqi %d, SNRtimes10 %d\n",
          frame,
          slot,
          uci_pdu->harq.harq_list[1].harq_value == 0 ? "ACK" : "NACK",
          uci_pdu->harq.harq_list[0].harq_value == 0 ? "ACK" : "NACK",
          uci_pdu->harq.harq_confidence_level == 0 ? "good" : "bad",
          xrtmag_dBtimes10,
          xrtmag_next_dBtimes10,
          pucch_power_dBtimes10,
          max_n0,
          uci_stats->pucch0_n00,
          uci_stats->pucch0_n01,
          uci_stats->pucch0_thres,
          cqi,
          SNRtimes10);
    if (pucch_pdu->sr_flag == 1) {
      uci_pdu->sr.sr_indication = (index>3) ? 1 : 0;
      uci_pdu->sr.sr_confidence_level = no_conf;
      if(uci_pdu->sr.sr_indication == 1 && uci_pdu->sr.sr_confidence_level == 0) {
        uci_stats->pucch0_positive_SR++;
        LOG_D(PHY,"PUCCH0 got positive SR. Cumulative number of positive SR %d\n", uci_stats->pucch0_positive_SR);
      }
    }
  }
}
//*****************************************************************//
void nr_decode_pucch1(c16_t **rxdataF,
                      pucch_GroupHopping_t pucch_GroupHopping,
                      uint32_t n_id, // hoppingID higher layer parameter
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
                      uint8_t nr_bit)
{
#ifdef DEBUG_NR_PUCCH_RX
  printf(
      "\t [nr_generate_pucch1] start function at slot(nr_tti_tx)=%d "
      "payload=%lux m0=%d nrofSymbols=%d startingSymbolIndex=%d "
      "startingPRB=%d startingPRB_intraSlotHopping=%d timeDomainOCC=%d "
      "nr_bit=%d\n",
      nr_tti_tx,
      *payload,
      m0,
      nrofSymbols,
      startingSymbolIndex,
      startingPRB,
      startingPRB_intraSlotHopping,
      timeDomainOCC,
      nr_bit);
#endif
  /*
   * Implement TS 38.211 Subclause 6.3.2.4.1 Sequence modulation
   *
   */
  const int soffset = (nr_tti_tx % RU_RX_SLOT_DEPTH) * frame_parms->symbols_per_slot * frame_parms->ofdm_symbol_size;
  // lprime is the index of the OFDM symbol in the slot that corresponds to the first OFDM symbol of the PUCCH transmission in the slot given by [5, TS 38.213]
  const int lprime = startingSymbolIndex;
  // mcs = 0 except for PUCCH format 0
  const uint8_t mcs = 0;
  // r_u_v_alpha_delta_re and r_u_v_alpha_delta_im tables containing the sequence y(n) for the PUCCH, when they are multiplied by d(0)
  // r_u_v_alpha_delta_dmrs_re and r_u_v_alpha_delta_dmrs_im tables containing the sequence for the DM-RS.
  c16_t r_u_v_alpha_delta[12], r_u_v_alpha_delta_dmrs[12];
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

  // Intra-slot frequency hopping shall be assumed when the higher-layer parameter intraSlotFrequencyHopping is provided,
  // regardless of whether the frequency-hop distance is zero or not,
  // otherwise no intra-slot frequency hopping shall be assumed
  //uint8_t PUCCH_Frequency_Hopping = 0 ; // from higher layers
  const bool intraSlotFrequencyHopping = startingPRB != startingPRB_intraSlotHopping;

#ifdef DEBUG_NR_PUCCH_RX
  printf("\t [nr_generate_pucch1] intraSlotFrequencyHopping = %d \n",intraSlotFrequencyHopping);
#endif
  /*
   * Implementing TS 38.211 Subclause 6.3.2.4.2 Mapping to physical resources
   */
#define MAX_SIZE_Z 168 // this value has to be calculated from mprime*12*table_6_3_2_4_1_1_N_SF_mprime_PUCCH_1_noHop[pucch_symbol_length]+m*12+n
  c16_t z_rx[MAX_SIZE_Z] = {0};
  c16_t z_dmrs_rx[MAX_SIZE_Z] = {0};
  const int half_nb_rb_dl = frame_parms->N_RB_DL >> 1;
  const bool nb_rb_is_even = (frame_parms->N_RB_DL & 1) == 0;
  for (int l = 0; l < nrofSymbols; l++) { // extracting data and dmrs from rxdataF
    if (intraSlotFrequencyHopping && (l < floor(nrofSymbols / 2))) { // intra-slot hopping enabled, we need
                                                                     // to calculate new offset PRB
      startingPRB = startingPRB + startingPRB_intraSlotHopping;
    }
    int re_offset = (l + startingSymbolIndex) * frame_parms->ofdm_symbol_size;

    if (nb_rb_is_even) {
      if (startingPRB < half_nb_rb_dl) // if number RBs in bandwidth is even and
                                       // current PRB is lower band
        re_offset += 12 * startingPRB + frame_parms->first_carrier_offset;
      else // if number RBs in bandwidth is even and current PRB is upper band
        re_offset += 12 * (startingPRB - half_nb_rb_dl);
    } else {
      if (startingPRB < half_nb_rb_dl) // if number RBs in bandwidth is odd  and
                                       // current PRB is lower band
        re_offset += 12 * startingPRB + frame_parms->first_carrier_offset;
      else if (startingPRB > half_nb_rb_dl) // if number RBs in bandwidth is odd
                                            // and current PRB is upper band
        re_offset += 12 * (startingPRB - half_nb_rb_dl) + 6;
      else // if number RBs in bandwidth is odd  and current PRB contains DC
        re_offset += 12 * startingPRB + frame_parms->first_carrier_offset;
    }

    for (int n=0; n<12; n++) {
      const int current_subcarrier = l * 12 + n;
      if (n == 6 && startingPRB == half_nb_rb_dl && !nb_rb_is_even) {
        // if number RBs in bandwidth is odd  and current PRB contains DC, we need to recalculate the offset when n=6 (for second half PRB)
        re_offset = ((l+startingSymbolIndex)*frame_parms->ofdm_symbol_size);
      }

      if (l % 2 == 1) // mapping PUCCH or DM-RS according to TS38.211 subclause 6.4.1.3.1
        z_rx[current_subcarrier] = rxdataF[0][soffset + re_offset];
      else
        z_dmrs_rx[current_subcarrier] = rxdataF[0][soffset + re_offset];
#ifdef DEBUG_NR_PUCCH_RX
      printf(
          "\t [nr_generate_pucch1] mapping %s to RE \t amp=%d "
          "\tofdm_symbol_size=%d \tN_RB_DL=%d \tfirst_carrier_offset=%d "
          "\tz_pucch[%d]=txptr(%u)=(x_n(l=%d,n=%d)=(%d,%d))\n",
          l % 2 ? "PUCCH" : "DM-RS",
          amp,
          frame_parms->ofdm_symbol_size,
          frame_parms->N_RB_DL,
          frame_parms->first_carrier_offset,
          current_subcarrier,
          re_offset,
          l,
          n,
          rxdataF[0][soffset + re_offset].r,
          rxdataF[0][soffset + re_offset].i);
#endif
      re_offset++;
    }
  }
  cd_t y_n[12] = {0}, y1_n[12] = {0};
  //generating transmitted sequence and dmrs
  for (int l = 0; l < nrofSymbols; l++) {
#ifdef DEBUG_NR_PUCCH_RX
    printf("\t [nr_generate_pucch1] for symbol l=%d, lprime=%d\n",
           l,lprime);
#endif
    // y_n contains the complex value d multiplied by the sequence r_u_v
    // if frequency hopping is disabled, intraSlotFrequencyHopping is not
    // provided
    //              n_hop = 0
    // if frequency hopping is enabled,  intraSlotFrequencyHopping is provided
    //              n_hop = 0 for first hop
    //              n_hop = 1 for second hop
    const int n_hop = intraSlotFrequencyHopping && l >= nrofSymbols / 2 ? 1 : 0;

#ifdef DEBUG_NR_PUCCH_RX
    printf("\t [nr_generate_pucch1] entering function nr_group_sequence_hopping with n_hop=%d, nr_tti_tx=%d\n",
           n_hop,nr_tti_tx);
#endif
    nr_group_sequence_hopping(pucch_GroupHopping,n_id,n_hop,nr_tti_tx,&u,&v); // calculating u and v value
    // Defining cyclic shift hopping TS 38.211 Subclause 6.3.2.2.2
    double alpha = nr_cyclic_shift_hopping(n_id, m0, mcs, l, lprime, nr_tti_tx);
    for (int n=0; n<12; n++) {  // generating low papr sequences
      const c16_t angle = {lround(32767 * cos(alpha * n)), lround(32767 * sin(alpha * n))};
      const c16_t table = {table_5_2_2_2_2_Re[u][n], table_5_2_2_2_2_Im[u][n]};
      if (l % 2 == 1)
        r_u_v_alpha_delta[n] = c16mulShift(angle, table, 15);
      else
        r_u_v_alpha_delta_dmrs[n] = c16mulRealShift(c16mulShift(angle, table, 15), amp, 15);
#ifdef DEBUG_NR_PUCCH_RX
      printf(
          "\t [nr_generate_pucch1] sequence generation \tu=%d \tv=%d "
          "\talpha=%lf \tr_u_v_alpha_delta[n=%d]=(%d,%d) "
          "\ty_n[n=%d]=(%f,%f)\n",
          u,
          v,
          alpha,
          n,
          r_u_v_alpha_delta[n].r,
          r_u_v_alpha_delta[n].i,
          n,
          y_n[n].r,
          y_n[n].i);
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
    const int w_index = timeDomainOCC;
    if (intraSlotFrequencyHopping == false) { // intra-slot hopping disabled
#ifdef DEBUG_NR_PUCCH_RX
      printf("\t [nr_generate_pucch1] block-wise spread with the orthogonal sequence wi(m) if intraSlotFrequencyHopping = %d, intra-slot hopping disabled\n",
             intraSlotFrequencyHopping);
#endif
      // mprime is 0 in this not hopping case
      // N_SF_mprime_PUCCH_1 contains N_SF_mprime from table 6.3.2.4.1-1
      // (depending on number of PUCCH symbols nrofSymbols, mprime and
      // intra-slot hopping enabled/disabled) N_SF_mprime_PUCCH_1 contains
      // N_SF_mprime from table 6.4.1.3.1.1-1 (depending on number of PUCCH
      // symbols nrofSymbols, mprime and intra-slot hopping enabled/disabled)
      // N_SF_mprime_PUCCH_1 contains N_SF_mprime from table 6.3.2.4.1-1
      // (depending on number of PUCCH symbols nrofSymbols, mprime=0 and
      // intra-slot hopping enabled/disabled) N_SF_mprime_PUCCH_1 contains
      // N_SF_mprime from table 6.4.1.3.1.1-1 (depending on number of PUCCH
      // symbols nrofSymbols, mprime=0 and intra-slot hopping enabled/disabled)
      // mprime is 0 if no intra-slot hopping / mprime is {0,1} if intra-slot
      // hopping
      int N_SF_mprime_PUCCH_1 =
          table_6_3_2_4_1_1_N_SF_mprime_PUCCH_1_noHop[nrofSymbols - 1]; // only if intra-slot hopping not enabled (PUCCH)
      int N_SF_mprime_PUCCH_DMRS_1 =
          table_6_4_1_3_1_1_1_N_SF_mprime_PUCCH_1_noHop[nrofSymbols - 1]; // only if intra-slot hopping not enabled (DM-RS)
#ifdef DEBUG_NR_PUCCH_RX
      printf("\t [nr_generate_pucch1] w_index = %d, N_SF_mprime_PUCCH_1 = %d, N_SF_mprime_PUCCH_DMRS_1 = %d, N_SF_mprime0_PUCCH_1 = %d, N_SF_mprime0_PUCCH_DMRS_1 = %d\n",
             w_index, N_SF_mprime_PUCCH_1,N_SF_mprime_PUCCH_DMRS_1,N_SF_mprime0_PUCCH_1,N_SF_mprime0_PUCCH_DMRS_1);
#endif

      if(l%2==1){
        for (int m=0; m < N_SF_mprime_PUCCH_1; m++) {
          c16_t table = {table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m],
                         table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m]};
          if (l / 2 == m) {
            for (int n = 0; n < 12; n++) {
              c16_t *zPtr = z_rx + m * 12 + n;
              *zPtr = c16MulConjShift(table, *zPtr, 15);
              // multiplying with conjugate of low papr sequence
              *zPtr = c16MulConjShift(r_u_v_alpha_delta[n], *zPtr, 16);
            }
          }
        }
      } else {
        for (int m=0; m < N_SF_mprime_PUCCH_DMRS_1; m++) {
          const c16_t table = {table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_DMRS_1][w_index][m],
                               table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_DMRS_1][w_index][m]};
          if (l / 2 == m) {
            for (int n = 0; n < 12; n++) {
              c16_t *zDmrsPtr = z_dmrs_rx + m * 12 + n;
              *zDmrsPtr = c16MulConjShift(table, *zDmrsPtr, 15);
              // finding channel coeffcients by dividing received dmrs with actual dmrs and storing them in z_dmrs_re_rx and
              // z_dmrs_im_rx arrays
              *zDmrsPtr = c16MulConjShift(r_u_v_alpha_delta_dmrs[n], *zDmrsPtr, 16);
            }
          }
        }
      }
    }

    if (intraSlotFrequencyHopping == true) { // intra-slot hopping enabled
#ifdef DEBUG_NR_PUCCH_RX
      printf("\t [nr_generate_pucch1] block-wise spread with the orthogonal sequence wi(m) if intraSlotFrequencyHopping = %d, intra-slot hopping enabled\n",
             intraSlotFrequencyHopping);
#endif
      // N_SF_mprime_PUCCH_1 contains N_SF_mprime from table 6.3.2.4.1-1
      // (depending on number of PUCCH symbols nrofSymbols, mprime and
      // intra-slot hopping enabled/disabled) N_SF_mprime_PUCCH_1 contains
      // N_SF_mprime from table 6.4.1.3.1.1-1 (depending on number of PUCCH
      // symbols nrofSymbols, mprime and intra-slot hopping enabled/disabled)
      // N_SF_mprime_PUCCH_1 contains N_SF_mprime from table 6.3.2.4.1-1
      // (depending on number of PUCCH symbols nrofSymbols, mprime=0 and
      // intra-slot hopping enabled/disabled) N_SF_mprime_PUCCH_1 contains
      // N_SF_mprime from table 6.4.1.3.1.1-1 (depending on number of PUCCH
      // symbols nrofSymbols, mprime=0 and intra-slot hopping enabled/disabled)
      // mprime is 0 if no intra-slot hopping / mprime is {0,1} if intra-slot
      // hopping
      int N_SF_mprime_PUCCH_1 =
          table_6_3_2_4_1_1_N_SF_mprime_PUCCH_1_m0Hop[nrofSymbols - 1]; // only if intra-slot hopping enabled mprime = 0 (PUCCH)
      int N_SF_mprime_PUCCH_DMRS_1 =
          table_6_4_1_3_1_1_1_N_SF_mprime_PUCCH_1_m0Hop[nrofSymbols - 1]; // only if intra-slot hopping enabled mprime = 0 (DM-RS)
      int N_SF_mprime0_PUCCH_1 =
          table_6_3_2_4_1_1_N_SF_mprime_PUCCH_1_m0Hop[nrofSymbols - 1]; // only if intra-slot hopping enabled mprime = 0 (PUCCH)
      int N_SF_mprime0_PUCCH_DMRS_1 =
          table_6_4_1_3_1_1_1_N_SF_mprime_PUCCH_1_m0Hop[nrofSymbols - 1]; // only if intra-slot hopping enabled mprime = 0 (DM-RS)
#ifdef DEBUG_NR_PUCCH_RX
      printf("\t [nr_generate_pucch1] w_index = %d, N_SF_mprime_PUCCH_1 = %d, N_SF_mprime_PUCCH_DMRS_1 = %d, N_SF_mprime0_PUCCH_1 = %d, N_SF_mprime0_PUCCH_DMRS_1 = %d\n",
             w_index, N_SF_mprime_PUCCH_1,N_SF_mprime_PUCCH_DMRS_1,N_SF_mprime0_PUCCH_1,N_SF_mprime0_PUCCH_DMRS_1);
#endif

      for (int mprime = 0; mprime < 2; mprime++) { // mprime can get values {0,1}
        if (l % 2 == 1) {
          for (int m = 0; m < N_SF_mprime_PUCCH_1; m++) {
            c16_t table = {table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m],
                           table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m]};
            if (floor(l / 2) * 12 == (mprime * 12 * N_SF_mprime0_PUCCH_1) + (m * 12)) {
              for (int n = 0; n < 12; n++) {
                c16_t *zPtr = z_rx + (mprime * 12 * N_SF_mprime0_PUCCH_1) + (m * 12) + n;
                *zPtr = c16MulConjShift(table, *zPtr, 15);
                *zPtr = c16MulConjShift(r_u_v_alpha_delta[n], *zPtr, 16);
              }
            }
          }
        }

        else {
          for (int m = 0; m < N_SF_mprime_PUCCH_DMRS_1; m++) {
            c16_t table = {table_6_3_2_4_1_2_Wi_Re[N_SF_mprime_PUCCH_1][w_index][m],
                           table_6_3_2_4_1_2_Wi_Im[N_SF_mprime_PUCCH_1][w_index][m]};
            if (floor(l / 2) * 12 == (mprime * 12 * N_SF_mprime0_PUCCH_DMRS_1) + (m * 12)) {
              for (int n = 0; n < 12; n++) {
                c16_t *zDmrsPtr = z_dmrs_rx + (mprime * 12 * N_SF_mprime0_PUCCH_DMRS_1) + (m * 12) + n;
                *zDmrsPtr = c16MulConjShift(table, *zDmrsPtr, 15);
                // finding channel coeffcients by dividing received dmrs with actual dmrs and storing them in z_dmrs_re_rx and
                // z_dmrs_im_rx arrays
                *zDmrsPtr = c16MulConjShift(r_u_v_alpha_delta_dmrs[n], *zDmrsPtr, 16);
              }
            }
          }
        }

        N_SF_mprime_PUCCH_1 =
            table_6_3_2_4_1_1_N_SF_mprime_PUCCH_1_m1Hop[nrofSymbols - 1]; // only if intra-slot hopping enabled mprime = 1 (PUCCH)
        N_SF_mprime_PUCCH_DMRS_1  = table_6_4_1_3_1_1_1_N_SF_mprime_PUCCH_1_m1Hop[nrofSymbols-1]; // only if intra-slot hopping enabled mprime = 1 (DM-RS)
      }
    }
  }
  cd_t H[12] = {0}, H1[12] = {0};
  const double half_nb_symbols = nrofSymbols / 2.0;
  const double quarter_nb_symbols = nrofSymbols / 4.0;
  for (int l = 0; l <= half_nb_symbols; l++) {
    if (intraSlotFrequencyHopping == false) {
      for (int n = 0; n < 12; n++) {
        H[n].r += z_dmrs_rx[l * 12 + n].r / half_nb_symbols;
        H[n].i += z_dmrs_rx[l * 12 + n].i / half_nb_symbols;
        y_n[n].r += z_rx[l * 12 + n].r / half_nb_symbols;
        y_n[n].i += z_rx[l * 12 + n].i / half_nb_symbols;
      }
    } else {
      if (l < nrofSymbols / 4) {
        for (int n = 0; n < 12; n++) {
          H[n].r += z_dmrs_rx[l * 12 + n].r / quarter_nb_symbols;
          H[n].i += z_dmrs_rx[l * 12 + n].i / quarter_nb_symbols;
          y_n[n].r += z_rx[l * 12 + n].r / quarter_nb_symbols;
          y_n[n].i += z_rx[l * 12 + n].i / quarter_nb_symbols;
        }
      } else {
        for (int n = 0; n < 12; n++) {
          H1[n].r += z_dmrs_rx[l * 12 + n].r / quarter_nb_symbols;
          H1[n].i += z_dmrs_rx[l * 12 + n].i / quarter_nb_symbols;
          y1_n[n].r += z_rx[l * 12 + n].r / quarter_nb_symbols;
          y1_n[n].i += z_rx[l * 12 + n].i / quarter_nb_symbols;
        }
      }
    }
  }
  // mrc combining to obtain z_re and z_im
  cd_t d = {0};
  if (intraSlotFrequencyHopping == false) {
    // complex-valued symbol d_re, d_im containing complex-valued symbol d(0):
    for (int n = 0; n < 12; n++) {
      d.r += H[n].r * y_n[n].r + H[n].i * y_n[n].i;
      d.i += H[n].r * y_n[n].i - H[n].i * y_n[n].r;
    }
  } else {
    for (int n = 0; n < 12; n++) {
      d.r += H[n].r * y_n[n].r + H[n].i * y_n[n].i;
      d.i += H[n].r * y_n[n].i - H[n].i * y_n[n].r;
      d.r += H[n].r * y1_n[n].r + H[n].i * y1_n[n].i;
      d.i += H[n].r * y1_n[n].i - H[n].i * y1_n[n].r;
    }
  }
  //Decoding QPSK or BPSK symbols to obtain payload bits
  if (nr_bit == 1) {
    if ((d.r + d.i) > 0) {
      *payload = 0;
    } else {
      *payload = 1;
    }
  } else if (nr_bit == 2) {
    if ((d.r > 0) && (d.i > 0)) {
      *payload = 0;
    } else if ((d.r < 0) && (d.i > 0)) {
      *payload = 1;
    } else if ((d.r > 0) && (d.i < 0)) {
      *payload = 2;
    } else {
      *payload = 3;
    }
  }
}

static simde__m256i pucch2_3bit[8 * 2];
static simde__m256i pucch2_4bit[16 * 2];
static simde__m256i pucch2_5bit[32 * 2];
static simde__m256i pucch2_6bit[64 * 2];
static simde__m256i pucch2_7bit[128 * 2];
static simde__m256i pucch2_8bit[256 * 2];
static simde__m256i pucch2_9bit[512 * 2];
static simde__m256i pucch2_10bit[1024 * 2];
static simde__m256i pucch2_11bit[2048 * 2];

static simde__m256i *pucch2_lut[9] =
    {pucch2_3bit, pucch2_4bit, pucch2_5bit, pucch2_6bit, pucch2_7bit, pucch2_8bit, pucch2_9bit, pucch2_10bit, pucch2_11bit};

simde__m64 pucch2_polar_4bit[16];
simde__m128i pucch2_polar_llr_num_lut[256],pucch2_polar_llr_den_lut[256];

void init_pucch2_luts() {

  uint32_t out;
  int8_t bit; 
  
  for (int b=3;b<12;b++) {
    for (int i = 0; i < (1 << b); i++) {
      out = encodeSmallBlock(i, b);
#ifdef DEBUG_NR_PUCCH_RX
      if (b==3) printf("in %d, out %x\n",i,out);
#endif
      simde__m256i *lut_i=&pucch2_lut[b-3][i<<1];
      simde__m256i *lut_ip1=&pucch2_lut[b-3][1+(i<<1)];
      bit = (out&0x1) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,0);
      bit = (out&0x2) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,0);
      bit = (out&0x4) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,1);
      bit = (out&0x8) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,1);
      bit = (out&0x10) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,2);
      bit = (out&0x20) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,2);
      bit = (out&0x40) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,3);
      bit = (out&0x80) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,3);
      bit = (out&0x100) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,4);
      bit = (out&0x200) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,4);
      bit = (out&0x400) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,5);
      bit = (out&0x800) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,5);
      bit = (out&0x1000) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,6);
      bit = (out&0x2000) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,6);
      bit = (out&0x4000) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,7);
      bit = (out&0x8000) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,7);
      bit = (out&0x10000) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,8);
      bit = (out&0x20000) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,8);
      bit = (out&0x40000) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,9);
      bit = (out&0x80000) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,9);
      bit = (out&0x100000) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,10);
      bit = (out&0x200000) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,10);
      bit = (out&0x400000) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,11);
      bit = (out&0x800000) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,11);
      bit = (out&0x1000000) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,12);
      bit = (out&0x2000000) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,12);
      bit = (out&0x4000000) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,13);
      bit = (out&0x8000000) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,13);
      bit = (out&0x10000000) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,14);
      bit = (out&0x20000000) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,14);
      bit = (out&0x40000000) > 0 ? -1 : 1;
      *lut_i = simde_mm256_insert_epi16(*lut_i,bit,15);
      bit = (out&0x80000000) > 0 ? -1 : 1;
      *lut_ip1 = simde_mm256_insert_epi16(*lut_ip1,bit,15);
    }
  }
  for (int i = 0; i < 16; i++) {
    simde__m64 *lut_i=&pucch2_polar_4bit[i];

    bit = (i&0x1) > 0 ? -1 : 1;
    *lut_i = simde_mm_insert_pi16(*lut_i,bit,0);
    bit = (i&0x2) > 0 ? -1 : 1;
    *lut_i = simde_mm_insert_pi16(*lut_i,bit,1);
    bit = (i&0x4) > 0 ? -1 : 1;
    *lut_i = simde_mm_insert_pi16(*lut_i,bit,2);
    bit = (i&0x8) > 0 ? -1 : 1;
    *lut_i = simde_mm_insert_pi16(*lut_i,bit,3);
  }
  for (int i=0;i<256;i++) {
    simde__m128i *lut_num_i=&pucch2_polar_llr_num_lut[i];
    simde__m128i *lut_den_i=&pucch2_polar_llr_den_lut[i];
    bit = (i&0x1) > 0 ? 0 : 1;
    *lut_num_i = simde_mm_insert_epi16(*lut_num_i, bit, 0);
    *lut_den_i = simde_mm_insert_epi16(*lut_den_i, 1 - bit, 0);

    bit = (i&0x10) > 0 ? 0 : 1;
    *lut_num_i = simde_mm_insert_epi16(*lut_num_i, bit, 1);
    *lut_den_i = simde_mm_insert_epi16(*lut_den_i, 1 - bit, 1);

    bit = (i&0x2) > 0 ? 0 : 1;
    *lut_num_i = simde_mm_insert_epi16(*lut_num_i, bit, 2);
    *lut_den_i = simde_mm_insert_epi16(*lut_den_i, 1 - bit, 2);

    bit = (i&0x20) > 0 ? 0 : 1;
    *lut_num_i = simde_mm_insert_epi16(*lut_num_i, bit, 3);
    *lut_den_i = simde_mm_insert_epi16(*lut_den_i, 1 - bit, 3);

    bit = (i&0x4) > 0 ? 0 : 1;
    *lut_num_i = simde_mm_insert_epi16(*lut_num_i, bit, 4);
    *lut_den_i = simde_mm_insert_epi16(*lut_den_i, 1 - bit, 4);

    bit = (i&0x40) > 0 ? 0 : 1;
    *lut_num_i = simde_mm_insert_epi16(*lut_num_i, bit, 5);
    *lut_den_i = simde_mm_insert_epi16(*lut_den_i, 1 - bit, 5);

    bit = (i&0x8) > 0 ? 0 : 1;
    *lut_num_i = simde_mm_insert_epi16(*lut_num_i, bit, 6);
    *lut_den_i = simde_mm_insert_epi16(*lut_den_i, 1 - bit, 6);

    bit = (i&0x80) > 0 ? 0 : 1;
    *lut_num_i = simde_mm_insert_epi16(*lut_num_i, bit, 7);
    *lut_den_i = simde_mm_insert_epi16(*lut_den_i, 1 - bit, 7);

#ifdef DEBUG_NR_PUCCH_RX
    printf("i %d, lut_num (%d,%d,%d,%d,%d,%d,%d,%d)\n",
           i,
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
                      int frame,
                      int slot,
                      nfapi_nr_uci_pucch_pdu_format_2_3_4_t* uci_pdu,
                      nfapi_nr_pucch_pdu_t* pucch_pdu) {

  c16_t **rxdataF = gNB->common_vars.rxdataF;
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  //pucch_GroupHopping_t pucch_GroupHopping = pucch_pdu->group_hop_flag + (pucch_pdu->sequence_hop_flag<<1);

  AssertFatal(pucch_pdu->nr_of_symbols == 1 || pucch_pdu->nr_of_symbols == 2,
              "Illegal number of symbols  for PUCCH 2 %d\n",
              pucch_pdu->nr_of_symbols);

  AssertFatal((pucch_pdu->prb_start-((pucch_pdu->prb_start>>2)<<2))==0,
              "Current pucch2 receiver implementation requires a PRB offset multiple of 4. The one selected is %d",
              pucch_pdu->prb_start);

  //extract pucch and dmrs first

  int l2 = pucch_pdu->start_symbol_index;
  int soffset = (slot % RU_RX_SLOT_DEPTH) * frame_parms->symbols_per_slot * frame_parms->ofdm_symbol_size;
  int re_offset[2];
  re_offset[0] =
      (12 * (pucch_pdu->prb_start + pucch_pdu->bwp_start) + frame_parms->first_carrier_offset) % frame_parms->ofdm_symbol_size;
  if (pucch_pdu->freq_hop_flag == 0)
    re_offset[1] = re_offset[0];
  else {
    re_offset[1] = 12*(pucch_pdu->second_hop_prb+pucch_pdu->bwp_start) + frame_parms->first_carrier_offset;
    if (re_offset[1]>= frame_parms->ofdm_symbol_size)
      re_offset[1]-=frame_parms->ofdm_symbol_size;
  }
  AssertFatal(pucch_pdu->prb_size * pucch_pdu->nr_of_symbols > 1,
              "number of PRB*SYMB (%d,%d)< 2",
              pucch_pdu->prb_size,
              pucch_pdu->nr_of_symbols);

  int Prx = gNB->gNB_config.carrier_config.num_rx_ant.value;
  //  AssertFatal((pucch_pdu->prb_size&1) == 0,"prb_size %d is not a multiple of2\n",pucch_pdu->prb_size);
  int Prx2 = (Prx==1)?2:Prx;
  // use 2 for Nb antennas in case of single antenna to allow the following allocations
  int nb_re_pucch = 12*pucch_pdu->prb_size;
  int prb_size_ext = pucch_pdu->prb_size+(pucch_pdu->prb_size&1);

  c16_t rp[Prx2][2][nb_re_pucch];
  memset(rp, 0, sizeof(rp));

  for (int aa=0;aa<Prx;aa++){
    for (int symb=0;symb<pucch_pdu->nr_of_symbols;symb++) {
      c16_t *tmp_rp = ((c16_t *)&rxdataF[aa][soffset + (l2 + symb) * frame_parms->ofdm_symbol_size]);

      if (re_offset[symb] + nb_re_pucch < frame_parms->ofdm_symbol_size) {
        memcpy(rp[aa][symb], &tmp_rp[re_offset[symb]], nb_re_pucch * sizeof(c16_t));
      }
      else {
        int neg_length = frame_parms->ofdm_symbol_size-re_offset[symb];
        int pos_length = nb_re_pucch-neg_length;
        memcpy(rp[aa][symb], &tmp_rp[re_offset[symb]], neg_length * sizeof(c16_t));
        memcpy(&rp[aa][symb][neg_length], tmp_rp, pos_length * sizeof(c16_t));
      }
    }
  }
  LOG_D(PHY,
        "%d.%d Decoding pucch2 for %d symbols, %d PRB, nb_harq %d, nb_sr %d, nb_csi %d/%d\n",
        frame,
        slot,
        pucch_pdu->nr_of_symbols,
        pucch_pdu->prb_size,
        pucch_pdu->bit_len_harq,
        pucch_pdu->sr_flag,
        pucch_pdu->bit_len_csi_part1,
        pucch_pdu->bit_len_csi_part2);

  int nc_group_size=1; // 2 PRB
  int ngroup = prb_size_ext/nc_group_size/2;
  int32_t corr32_re[2][ngroup][Prx2],corr32_im[2][ngroup][Prx2];
  memset(corr32_re, 0, sizeof(corr32_re));
  memset(corr32_im, 0, sizeof(corr32_im));

  int16_t r_re_ext[Prx2][2][8 * prb_size_ext] __attribute__((aligned(32)));
  int16_t r_im_ext[Prx2][2][8 * prb_size_ext] __attribute__((aligned(32)));
  int16_t r_re_ext2[Prx2][2][8 * prb_size_ext] __attribute__((aligned(32)));
  int16_t r_im_ext2[Prx2][2][8 * prb_size_ext] __attribute__((aligned(32)));
  int16_t rd_re_ext[Prx2][2][4 * prb_size_ext] __attribute__((aligned(32)));
  int16_t rd_im_ext[Prx2][2][4 * prb_size_ext] __attribute__((aligned(32)));

  if (pucch_pdu->prb_size != prb_size_ext) {
    // if the number of PRBs is odd
    // we fill the unsed part of the arrays
    for (int aa = 0; aa < Prx; aa++) {
      for (int symb = 0; symb < pucch_pdu->nr_of_symbols; symb++) {
        const int sz = pucch_pdu->prb_size;
        memset(r_re_ext[aa][symb] + 8 * sz, 0, 8 * sizeof(int16_t));
        memset(r_im_ext[aa][symb] + 8 * sz, 0, 8 * sizeof(int16_t));
        memset(rd_re_ext[aa][symb] + 4 * sz, 0, 4 * sizeof(int16_t));
        memset(rd_im_ext[aa][symb] + 4 * sz, 0, 4 * sizeof(int16_t));
      }
    }
  }

  for (int symb=0; symb<pucch_pdu->nr_of_symbols;symb++) {
    // 24 REs contains 48x16-bit, so 6x8x16-bit
    for (int aa = 0; aa < Prx; aa++) {
      for (int prb = 0; prb < pucch_pdu->prb_size; prb++) {
        int16_t *r_re_ext_p = &r_re_ext[aa][symb][8 * prb];
        int16_t *r_im_ext_p = &r_im_ext[aa][symb][8 * prb];
        int16_t *rd_re_ext_p = &rd_re_ext[aa][symb][4 * prb];
        int16_t *rd_im_ext_p = &rd_im_ext[aa][symb][4 * prb];

        for (int idx = 0; idx < 4; idx++) {
          c16_t *rp_base = rp[aa][symb] + prb * 12 + 3 * idx;
          AssertFatal(prb * 12 + 3 * idx + 2 < nb_re_pucch, "");
          r_re_ext_p[idx << 1] = rp_base->r;
          r_im_ext_p[idx << 1] = rp_base->i;
          rp_base++;
          rd_re_ext_p[idx] = rp_base->r;
          rd_im_ext_p[idx] = rp_base->i;
          rp_base++;
          r_re_ext_p[1 + (idx << 1)] = rp_base->r;
          r_im_ext_p[1 + (idx << 1)] = rp_base->i;
        }

#ifdef DEBUG_NR_PUCCH_RX
        for (int i = 0; i < 8; i++)
          printf("Ant %d PRB %d dmrs[%d] -> (%d,%d)\n", aa, prb + (i >> 2), i, rd_re_ext_p[i], rd_im_ext_p[i]);
        for (int i = 0; i < 16; i++)
          printf("Ant %d PRB %d data[%d] -> (%d,%d)\n", aa, prb + (i >> 3), i, r_re_ext_p[i], r_im_ext_p[i]);
#endif
      }
    }

    // first compute DMRS component

    uint32_t x1 = 0, x2 = 0, sGold = 0;
    uint8_t *sGold8 = (uint8_t *)&sGold;
    x2 = (((1<<17)*((14*slot) + (pucch_pdu->start_symbol_index+symb) + 1)*((2*pucch_pdu->dmrs_scrambling_id) + 1)) + (2*pucch_pdu->dmrs_scrambling_id))%(1U<<31); // c_init calculation according to TS38.211 subclause
#ifdef DEBUG_NR_PUCCH_RX
    printf("slot %d, start_symbol_index %d, symbol %d, dmrs_scrambling_id %d\n",
           slot,pucch_pdu->start_symbol_index,symb,pucch_pdu->dmrs_scrambling_id);
#endif
    int reset = 1;
    for (int i=0; i<=(pucch_pdu->prb_start>>2); i++) {
      sGold = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
    }

    for (int group = 0; group < ngroup; group++) {
      // each group has 8*nc_group_size elements, compute 1 complex correlation with DMRS per group
      // non-coherent combining across groups
      simde__m64 dmrs_re = byte2m64_re[sGold8[(group & 1) << 1]];
      int16_t *dmrs_re16 = (int16_t *)&dmrs_re;
      simde__m64 dmrs_im = byte2m64_im[sGold8[(group & 1) << 1]];
      int16_t *dmrs_im16 = (int16_t *)&dmrs_im;
#ifdef DEBUG_NR_PUCCH_RX
      printf("Group %d: x2 %x ((%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
             group,
             x2,
             dmrs_re16[0],
             dmrs_im16[0],
             dmrs_re16[1],
             dmrs_im16[1],
             dmrs_re16[2],
             dmrs_im16[2],
             dmrs_re16[3],
             dmrs_im16[3]);
#endif
      for (int aa=0;aa<Prx;aa++) {
        int16_t *rd_re_ext_p = &rd_re_ext[aa][symb][8 * group];
        int16_t *rd_im_ext_p = &rd_im_ext[aa][symb][8 * group];

#ifdef DEBUG_NR_PUCCH_RX
        printf("Group %d: rd ((%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
               group,
               rd_re_ext_p[0],rd_im_ext_p[0],
               rd_re_ext_p[1],rd_im_ext_p[1],
               rd_re_ext_p[2],rd_im_ext_p[2],
               rd_re_ext_p[3],rd_im_ext_p[3]);
#endif
        for (int z = 0; z < 4; z++) {
          corr32_re[symb][group][aa] += rd_re_ext_p[z] * dmrs_re16[z] + rd_im_ext_p[z] * dmrs_im16[z];
          corr32_im[symb][group][aa] += -rd_re_ext_p[z] * dmrs_im16[z] + rd_im_ext_p[z] * dmrs_re16[z];
        }
      }
      dmrs_re = byte2m64_re[sGold8[1 + ((group & 1) << 1)]];
      dmrs_im = byte2m64_im[sGold8[1 + ((group & 1) << 1)]];
#ifdef DEBUG_NR_PUCCH_RX
      printf("Group %d: s %x ((%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
             group,
             ((uint16_t *)&sGold)[1],
             dmrs_re16[0],
             dmrs_im16[0],
             dmrs_re16[1],
             dmrs_im16[1],
             dmrs_re16[2],
             dmrs_im16[2],
             dmrs_re16[3],
             dmrs_im16[3]);
#endif
      for (int aa=0;aa<Prx;aa++) {
        int16_t *rd_re_ext_p = &rd_re_ext[aa][symb][8 * group];
        int16_t *rd_im_ext_p = &rd_im_ext[aa][symb][8 * group];
#ifdef DEBUG_NR_PUCCH_RX
        printf("Group %d: rd ((%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
               group,
               rd_re_ext_p[4],rd_im_ext_p[4],
               rd_re_ext_p[5],rd_im_ext_p[5],
               rd_re_ext_p[6],rd_im_ext_p[6],
               rd_re_ext_p[7],rd_im_ext_p[7]);
#endif
        for (int z = 0; z < 4; z++) {
          corr32_re[symb][group][aa] += rd_re_ext_p[z + 4] * dmrs_re16[z] + rd_im_ext_p[z + 4] * dmrs_im16[z];
          corr32_im[symb][group][aa] += -rd_re_ext_p[z + 4] * dmrs_im16[z] + rd_im_ext_p[z + 4] * dmrs_re16[z];
        }
        /*      corr32_re[group][aa]>>=5;
                corr32_im[group][aa]>>=5;*/
#ifdef DEBUG_NR_PUCCH_RX
        printf("Group %d: corr32 (%d,%d)\n",group,corr32_re[symb][group][aa],corr32_im[symb][group][aa]);
#endif
      } //aa    

      if ((group & 1) == 1)
        sGold = lte_gold_generic(&x1, &x2, 0);
    } // group
  } // symb

  uint32_t x1, x2, sGold = 0;
  // unscrambling
  x2 = ((pucch_pdu->rnti)<<15)+pucch_pdu->data_scrambling_id;
  sGold = lte_gold_generic(&x1, &x2, 1);
  uint8_t *sGold8 = (uint8_t *)&sGold;
#ifdef DEBUG_NR_PUCCH_RX
  printf("x2 %x\n", x2);
#endif
  for (int symb=0;symb<pucch_pdu->nr_of_symbols;symb++) {
    simde__m64 c_re[4], c_im[4];
    int re_off=0;
    for (int prb=0;prb<prb_size_ext;prb+=2,re_off+=16) {
      for (int z = 0; z < 4; z++) {
        c_re[z] = byte2m64_re[sGold8[z]];
        c_im[z] = byte2m64_im[sGold8[z]];
      }

      for (int aa=0;aa<Prx;aa++) {
#ifdef DEBUG_NR_PUCCH_RX
        printf("prb %d: rd ((%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
               prb,
               r_re_ext[aa][symb][re_off],
               r_im_ext[aa][symb][re_off],
               r_re_ext[aa][symb][re_off + 1],
               r_im_ext[aa][symb][re_off + 1],
               r_re_ext[aa][symb][re_off + 2],
               r_im_ext[aa][symb][re_off + 2],
               r_re_ext[aa][symb][re_off + 3],
               r_im_ext[aa][symb][re_off + 3],
               r_re_ext[aa][symb][re_off + 4],
               r_im_ext[aa][symb][re_off + 4],
               r_re_ext[aa][symb][re_off + 5],
               r_im_ext[aa][symb][re_off + 5],
               r_re_ext[aa][symb][re_off + 6],
               r_im_ext[aa][symb][re_off + 6],
               r_re_ext[aa][symb][re_off + 7],
               r_im_ext[aa][symb][re_off + 7]);
        printf("prb %d: rd ((%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
               prb+1,
               r_re_ext[aa][symb][re_off + 8],
               r_im_ext[aa][symb][re_off + 8],
               r_re_ext[aa][symb][re_off + 9],
               r_im_ext[aa][symb][re_off + 9],
               r_re_ext[aa][symb][re_off + 10],
               r_im_ext[aa][symb][re_off + 10],
               r_re_ext[aa][symb][re_off + 11],
               r_im_ext[aa][symb][re_off + 11],
               r_re_ext[aa][symb][re_off + 12],
               r_im_ext[aa][symb][re_off + 12],
               r_re_ext[aa][symb][re_off + 13],
               r_im_ext[aa][symb][re_off + 13],
               r_re_ext[aa][symb][re_off + 14],
               r_im_ext[aa][symb][re_off + 14],
               r_re_ext[aa][symb][re_off + 15],
               r_im_ext[aa][symb][re_off + 15]);
#endif

        simde__m64 *r_re_ext_64 = (simde__m64 *)&r_re_ext[aa][symb][re_off];
        simde__m64 *r_re_ext2_64 = (simde__m64 *)&r_re_ext2[aa][symb][re_off];
        simde__m64 *r_im_ext_64 = (simde__m64 *)&r_im_ext[aa][symb][re_off];
        simde__m64 *r_im_ext2_64 = (simde__m64 *)&r_im_ext2[aa][symb][re_off];
        for (int z = 0; z < 4; z++) {
          r_re_ext2_64[z] = simde_mm_mullo_pi16(r_re_ext_64[z], c_im[z]);
          r_re_ext_64[z] = simde_mm_mullo_pi16(r_re_ext_64[z], c_re[z]);
          r_im_ext2_64[z] = simde_mm_mullo_pi16(r_im_ext_64[z], c_re[z]);
          r_im_ext_64[z] = simde_mm_mullo_pi16(r_im_ext_64[z], c_im[z]);
        }

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
      sGold = lte_gold_generic(&x1, &x2, 0);
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
    simde__m256i *rp_re[Prx2][2];
    simde__m256i *rp2_re[Prx2][2];
    simde__m256i *rp_im[Prx2][2];
    simde__m256i *rp2_im[Prx2][2];
    for (int aa=0;aa<Prx;aa++) {
      for (int symb=0;symb<pucch_pdu->nr_of_symbols;symb++) {
        rp_re[aa][symb] = (simde__m256i*)r_re_ext[aa][symb];
        rp_im[aa][symb] = (simde__m256i*)r_im_ext[aa][symb];
        rp2_re[aa][symb] = (simde__m256i*)r_re_ext2[aa][symb];
        rp2_im[aa][symb] = (simde__m256i*)r_im_ext2[aa][symb];
      }
    }
    simde__m256i prod_re[Prx2],prod_im[Prx2];
    uint64_t corr=0;
    int cw_ML=0;
    
    
    for (int cw=0;cw<1<<nb_bit;cw++) {
#ifdef DEBUG_NR_PUCCH_RX
      printf("cw %d:",cw);
      for (int i=0;i<32;i+=2) {
        printf("%d,%d,",
               ((int16_t *)&pucch2_lut[nb_bit - 3][cw << 1])[i >> 1],
               ((int16_t *)&pucch2_lut[nb_bit - 3][cw << 1])[1 + (i >> 1)]);
      }
      printf("\n");
#endif
      uint64_t corr_tmp = 0;

      for (int symb=0;symb<pucch_pdu->nr_of_symbols;symb++) {
        for (int group=0;group<ngroup;group++) {
          // do complex correlation
          for (int aa=0;aa<Prx;aa++) {
            prod_re[aa] = /*simde_mm256_srai_epi16(*/ simde_mm256_adds_epi16(
                simde_mm256_mullo_epi16(pucch2_lut[nb_bit - 3][cw << 1], rp_re[aa][symb][group]),
                simde_mm256_mullo_epi16(pucch2_lut[nb_bit - 3][(cw << 1) + 1], rp_im[aa][symb][group])) /*,5)*/;
            prod_im[aa] = /*simde_mm256_srai_epi16(*/ simde_mm256_subs_epi16(
                simde_mm256_mullo_epi16(pucch2_lut[nb_bit - 3][cw << 1], rp2_im[aa][symb][group]),
                simde_mm256_mullo_epi16(pucch2_lut[nb_bit - 3][(cw << 1) + 1], rp2_re[aa][symb][group])) /*,5)*/;
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
            prod_re[aa] = simde_mm256_hadds_epi16(prod_re[aa],prod_re[aa]);// 0+1
#ifdef DEBUG_NR_PUCCH_RX
            printf("0.prod_re[%d] => (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\n",aa,
                   ((int16_t*)&prod_re[aa])[0],((int16_t*)&prod_re[aa])[1],((int16_t*)&prod_re[aa])[2],((int16_t*)&prod_re[aa])[3],
                   ((int16_t*)&prod_re[aa])[4],((int16_t*)&prod_re[aa])[5],((int16_t*)&prod_re[aa])[6],((int16_t*)&prod_re[aa])[7],
                   ((int16_t*)&prod_re[aa])[8],((int16_t*)&prod_re[aa])[9],((int16_t*)&prod_re[aa])[10],((int16_t*)&prod_re[aa])[11],
                   ((int16_t*)&prod_re[aa])[12],((int16_t*)&prod_re[aa])[13],((int16_t*)&prod_re[aa])[14],((int16_t*)&prod_re[aa])[15]);
#endif
            prod_im[aa] = simde_mm256_hadds_epi16(prod_im[aa],prod_im[aa]);
            prod_re[aa] = simde_mm256_hadds_epi16(prod_re[aa],prod_re[aa]);// 0+1+2+3
#ifdef DEBUG_NR_PUCCH_RX
            printf("1.prod_re[%d] => (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\n",aa,
                   ((int16_t*)&prod_re[aa])[0],((int16_t*)&prod_re[aa])[1],((int16_t*)&prod_re[aa])[2],((int16_t*)&prod_re[aa])[3],
                   ((int16_t*)&prod_re[aa])[4],((int16_t*)&prod_re[aa])[5],((int16_t*)&prod_re[aa])[6],((int16_t*)&prod_re[aa])[7],
                   ((int16_t*)&prod_re[aa])[8],((int16_t*)&prod_re[aa])[9],((int16_t*)&prod_re[aa])[10],((int16_t*)&prod_re[aa])[11],
                   ((int16_t*)&prod_re[aa])[12],((int16_t*)&prod_re[aa])[13],((int16_t*)&prod_re[aa])[14],((int16_t*)&prod_re[aa])[15]);
#endif
            prod_im[aa] = simde_mm256_hadds_epi16(prod_im[aa],prod_im[aa]);
            prod_re[aa] = simde_mm256_hadds_epi16(prod_re[aa],prod_re[aa]);// 0+1+2+3+4+5+6+7
#ifdef DEBUG_NR_PUCCH_RX
            printf("2.prod_re[%d] => (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\n",aa,
                   ((int16_t*)&prod_re[aa])[0],((int16_t*)&prod_re[aa])[1],((int16_t*)&prod_re[aa])[2],((int16_t*)&prod_re[aa])[3],
                   ((int16_t*)&prod_re[aa])[4],((int16_t*)&prod_re[aa])[5],((int16_t*)&prod_re[aa])[6],((int16_t*)&prod_re[aa])[7],
                   ((int16_t*)&prod_re[aa])[8],((int16_t*)&prod_re[aa])[9],((int16_t*)&prod_re[aa])[10],((int16_t*)&prod_re[aa])[11],
                   ((int16_t*)&prod_re[aa])[12],((int16_t*)&prod_re[aa])[13],((int16_t*)&prod_re[aa])[14],((int16_t*)&prod_re[aa])[15]);
#endif
            prod_im[aa] = simde_mm256_hadds_epi16(prod_im[aa],prod_im[aa]);
          }
          int64_t corr_re=0,corr_im=0;


          for (int aa=0;aa<Prx;aa++) {

            corr_re = ( corr32_re[symb][group][aa]+((int16_t*)(&prod_re[aa]))[0]+((int16_t*)(&prod_re[aa]))[8]);
            corr_im = ( corr32_im[symb][group][aa]+((int16_t*)(&prod_im[aa]))[0]+((int16_t*)(&prod_im[aa]))[8]);
#ifdef DEBUG_NR_PUCCH_RX
            printf("pucch2 cw %d group %d aa %d: (%d,%d)+(%d,%d) = (%ld,%ld)\n",
                   cw,
                   group,
                   aa,
                   corr32_re[symb][group][aa],
                   corr32_im[symb][group][aa],
                   ((int16_t *)(&prod_re[aa]))[0] + ((int16_t *)(&prod_re[aa]))[8],
                   ((int16_t *)(&prod_im[aa]))[0] + ((int16_t *)(&prod_im[aa]))[8],
                   corr_re,
                   corr_im);

#endif

            corr_tmp += corr_re*corr_re + corr_im*corr_im;
          } // aa loop
        }// group loop
      } // symb loop
      if (corr_tmp > corr) {
        corr = corr_tmp;
        cw_ML = cw;
#ifdef DEBUG_NR_PUCCH_RX
        printf("slot %d PUCCH2 cw_ML %d, corr %lu\n", slot, cw_ML, corr);
#endif
      }
    } // cw loop
    corr_dB = dB_fixed64((uint64_t)corr);
#ifdef DEBUG_NR_PUCCH_RX
    printf("slot %d PUCCH2 cw_ML %d, metric %d \n",slot,cw_ML,corr_dB);
#endif
    decodedPayload[0]=(uint64_t)cw_ML;
  }
  else { // polar coded case

    simde__m64 *rp_re[Prx2][2];
    simde__m64 *rp2_re[Prx2][2];
    simde__m64 *rp_im[Prx2][2];
    simde__m64 *rp2_im[Prx2][2];
    simde__m128i llrs[pucch_pdu->prb_size*2*pucch_pdu->nr_of_symbols];

    for (int aa=0;aa<Prx;aa++) {
      for (int symb=0;symb<pucch_pdu->nr_of_symbols;symb++) {
        rp_re[aa][symb] = (simde__m64*)r_re_ext[aa][symb];
        rp_im[aa][symb] = (simde__m64*)r_im_ext[aa][symb];
        rp2_re[aa][symb] = (simde__m64*)r_re_ext2[aa][symb];
        rp2_im[aa][symb] = (simde__m64*)r_im_ext2[aa][symb];
      }
    }
    simde__m64 prod_re[Prx2],prod_im[Prx2];

#ifdef DEBUG_NR_PUCCH_RX
    for (int cw=0;cw<16;cw++) {

      printf("cw %d:",cw);
      for (int i=0;i<4;i++) {
        printf("%d,", ((int16_t *)&pucch2_polar_4bit[cw])[i >> 1]);
      }
      printf("\n");
    }
#endif
    
    // non-coherent LLR computation on groups of 4 REs (half-PRBs)
    int32_t corr_re,corr_im,corr_tmp;
    simde__m128i corr16,llr_num,llr_den;
    uint64_t corr = 0;
    for (int symb=0;symb<pucch_pdu->nr_of_symbols;symb++) {
      for (int half_prb=0;half_prb<(2*pucch_pdu->prb_size);half_prb++) {
        llr_num=simde_mm_set1_epi16(0);llr_den=simde_mm_set1_epi16(0);
        for (int cw=0;cw<256;cw++) {
          corr_tmp=0;
          for (int aa=0;aa<Prx;aa++) {
            prod_re[aa] =
                simde_mm_srai_pi16(simde_mm_adds_pi16(simde_mm_mullo_pi16(pucch2_polar_4bit[cw & 15], rp_re[aa][symb][half_prb]),
                                                      simde_mm_mullo_pi16(pucch2_polar_4bit[cw >> 4], rp_im[aa][symb][half_prb])),
                                   5);
            prod_im[aa] =
                simde_mm_srai_pi16(simde_mm_subs_pi16(simde_mm_mullo_pi16(pucch2_polar_4bit[cw & 15], rp2_im[aa][symb][half_prb]),
                                                      simde_mm_mullo_pi16(pucch2_polar_4bit[cw >> 4], rp2_re[aa][symb][half_prb])),
                                   5);
            prod_re[aa] = simde_mm_hadds_pi16(prod_re[aa],prod_re[aa]);// 0+1
            prod_im[aa] = simde_mm_hadds_pi16(prod_im[aa],prod_im[aa]);
            prod_re[aa] = simde_mm_hadds_pi16(prod_re[aa],prod_re[aa]);// 0+1+2+3
            prod_im[aa] = simde_mm_hadds_pi16(prod_im[aa],prod_im[aa]);

            // this is for UL CQI measurement
            if (cw==0) corr += ((int64_t)corr32_re[symb][half_prb>>2][aa]*corr32_re[symb][half_prb>>2][aa])+
                         ((int64_t)corr32_im[symb][half_prb>>2][aa]*corr32_im[symb][half_prb>>2][aa]);


            corr_re = ( corr32_re[symb][half_prb>>2][aa]/(2*nc_group_size*4/2)+((int16_t*)(&prod_re[aa]))[0]);
            corr_im = ( corr32_im[symb][half_prb>>2][aa]/(2*nc_group_size*4/2)+((int16_t*)(&prod_im[aa]))[0]);
            corr_tmp += corr_re*corr_re + corr_im*corr_im;
            /*
              LOG_D(PHY,"pucch2 half_prb %d cw %d (%d,%d) aa %d: (%d,%d,%d,%d,%d,%d,%d,%d)x(%d,%d,%d,%d,%d,%d,%d,%d) (%d,%d)+(%d,%d)
              = (%d,%d) => %d\n", half_prb,cw,cw&15,cw>>4,aa,
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
          corr16 = simde_mm_set1_epi16((int16_t)(corr_tmp >> 8));

          LOG_D(PHY, "half_prb %d cw %d corr16 %d\n", half_prb, cw, corr_tmp >> 8);

          llr_num = simde_mm_max_epi16(simde_mm_mullo_epi16(corr16, pucch2_polar_llr_num_lut[cw]), llr_num);
          llr_den = simde_mm_max_epi16(simde_mm_mullo_epi16(corr16, pucch2_polar_llr_den_lut[cw]), llr_den);

          LOG_D(PHY,
                "lut_num (%d,%d,%d,%d,%d,%d,%d,%d)\n",
                ((int16_t *)&pucch2_polar_llr_num_lut[cw])[0],
                ((int16_t *)&pucch2_polar_llr_num_lut[cw])[1],
                ((int16_t *)&pucch2_polar_llr_num_lut[cw])[2],
                ((int16_t *)&pucch2_polar_llr_num_lut[cw])[3],
                ((int16_t *)&pucch2_polar_llr_num_lut[cw])[4],
                ((int16_t *)&pucch2_polar_llr_num_lut[cw])[5],
                ((int16_t *)&pucch2_polar_llr_num_lut[cw])[6],
                ((int16_t *)&pucch2_polar_llr_num_lut[cw])[7]);

          LOG_D(PHY,
                "llr_num (%d,%d,%d,%d,%d,%d,%d,%d)\n",
                ((int16_t *)&llr_num)[0],
                ((int16_t *)&llr_num)[1],
                ((int16_t *)&llr_num)[2],
                ((int16_t *)&llr_num)[3],
                ((int16_t *)&llr_num)[4],
                ((int16_t *)&llr_num)[5],
                ((int16_t *)&llr_num)[6],
                ((int16_t *)&llr_num)[7]);
          LOG_D(PHY,
                "llr_den (%d,%d,%d,%d,%d,%d,%d,%d)\n",
                ((int16_t *)&llr_den)[0],
                ((int16_t *)&llr_den)[1],
                ((int16_t *)&llr_den)[2],
                ((int16_t *)&llr_den)[3],
                ((int16_t *)&llr_den)[4],
                ((int16_t *)&llr_den)[5],
                ((int16_t *)&llr_den)[6],
                ((int16_t *)&llr_den)[7]);
        }
        // compute llrs
        llrs[half_prb + (symb*2*pucch_pdu->prb_size)] = simde_mm_subs_epi16(llr_num,llr_den);
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
    decoderState = polar_decoder_int16((int16_t *)llrs, decodedPayload, 0, NR_POLAR_UCI_PUCCH_MESSAGE_TYPE, nb_bit, pucch_pdu->prb_size);

    // Decoder reversal
    decodedPayload[0] = reverse_bits(decodedPayload[0], nb_bit);

    if (decoderState>0) decoderState=1;
    corr_dB = dB_fixed64(corr);
    LOG_D(PHY,"metric %d dB\n",corr_dB);
  }

  LOG_D(PHY, "UCI decoderState %d, payload[0] %llu\n", decoderState, (unsigned long long)decodedPayload[0]);

  // estimate CQI for MAC (from antenna port 0 only)
  // TODO this computation is wrong -> to be ignored at MAC for now
  int SNRtimes10 =
      dB_fixed_times10(signal_energy_nodc((int32_t *)&rxdataF[0][soffset + (l2 * frame_parms->ofdm_symbol_size) + re_offset[0]],
                                          12 * pucch_pdu->prb_size))
      - (10 * gNB->measurements.n0_power_tot_dB);
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
  uci_pdu->rssi=1280 - (10*dB_fixed(32767*32767)-dB_fixed_times10(signal_energy_nodc((int32_t *)&rxdataF[0][soffset+(l2*frame_parms->ofdm_symbol_size)+re_offset[0]],12*pucch_pdu->prb_size)));
  if (pucch_pdu->bit_len_harq>0) {
    int harq_bytes=pucch_pdu->bit_len_harq>>3;
    if ((pucch_pdu->bit_len_harq&7) > 0) harq_bytes++;
    uci_pdu->pduBitmap|=2;
    uci_pdu->harq.harq_payload = (uint8_t*)malloc(harq_bytes);
    uci_pdu->harq.harq_crc = decoderState;
    LOG_D(PHY,"[DLSCH/PDSCH/PUCCH2] %d.%d HARQ bytes (%d) Decoder state %d\n",
          frame,slot,harq_bytes,decoderState);
    int i=0;
    for (;i<harq_bytes-1;i++) {
      uci_pdu->harq.harq_payload[i] = decodedPayload[0] & 255;
      LOG_D(PHY, "[DLSCH/PDSCH/PUCCH2] %d.%d HARQ payload (%d) = %d\n", frame, slot, i, uci_pdu->harq.harq_payload[i]);
      decodedPayload[0]>>=8;
    }
    bit_left = pucch_pdu->bit_len_harq-((harq_bytes-1)<<3);
    uci_pdu->harq.harq_payload[i] = decodedPayload[0] & ((1<<bit_left)-1);
    LOG_D(PHY, "[DLSCH/PDSCH/PUCCH2] %d.%d HARQ payload (%d) = %d\n", frame, slot, i, uci_pdu->harq.harq_payload[i]);
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
    decodedPayload[0] = pucch_pdu->bit_len_csi_part1 < 64 ? decodedPayload[0] >> bit_left : 0;
  }
  
  if (pucch_pdu->bit_len_csi_part2>0) {
    uci_pdu->pduBitmap|=8;
  }
}

void nr_dump_uci_stats(FILE *fd,PHY_VARS_gNB *gNB,int frame) {
  int strpos = 0;
  char output[16384];

  for (int i = 0; i < MAX_MOBILES_PER_GNB; i++) {
    NR_gNB_PHY_STATS_t *stats = &gNB->phy_stats[i];
    if (!stats->active)
      return;
    NR_gNB_UCI_STATS_t *uci_stats = &stats->uci_stats;
    if (uci_stats->pucch0_sr_trials > 0)
      strpos += sprintf(output + strpos,
                        "UCI %d RNTI %x: pucch0_sr_trials %d, pucch0_n00 %d dB, pucch0_n01 %d dB, pucch0_sr_thres %d dB, current "
                        "pucch1_stat0 %d dB, current pucch1_stat1 %d dB, positive SR count %d\n",
                        i,
                        stats->rnti,
                        uci_stats->pucch0_sr_trials,
                        uci_stats->pucch0_n00,
                        uci_stats->pucch0_n01,
                        uci_stats->pucch0_sr_thres,
                        dB_fixed(uci_stats->current_pucch0_sr_stat0),
                        dB_fixed(uci_stats->current_pucch0_sr_stat1),
                        uci_stats->pucch0_positive_SR);
    if (uci_stats->pucch01_trials > 0)
      strpos += sprintf(output + strpos,
                        "UCI %d RNTI %x: pucch01_trials %d, pucch0_n00 %d dB, pucch0_n01 %d dB, pucch0_thres %d dB, current "
                        "pucch0_stat0 %d dB, current pucch1_stat1 %d dB, pucch01_DTX %d\n",
                        i,
                        stats->rnti,
                        uci_stats->pucch01_trials,
                        uci_stats->pucch0_n01,
                        uci_stats->pucch0_n01,
                        uci_stats->pucch0_thres,
                        dB_fixed(uci_stats->current_pucch0_stat0),
                        dB_fixed(uci_stats->current_pucch0_stat1),
                        uci_stats->pucch01_DTX);

    if (uci_stats->pucch02_trials > 0)
      strpos += sprintf(output + strpos,
                        "UCI %d RNTI %x: pucch01_trials %d, pucch0_n00 %d dB, pucch0_n01 %d dB, pucch0_thres %d dB, current "
                        "pucch0_stat0 %d dB, current pucch0_stat1 %d dB, pucch01_DTX %d\n",
                        i,
                        stats->rnti,
                        uci_stats->pucch02_trials,
                        uci_stats->pucch0_n00,
                        uci_stats->pucch0_n01,
                        uci_stats->pucch0_thres,
                        dB_fixed(uci_stats->current_pucch0_stat0),
                        dB_fixed(uci_stats->current_pucch0_stat1),
                        uci_stats->pucch02_DTX);

    if (uci_stats->pucch2_trials > 0)
      strpos += sprintf(output + strpos,
                        "UCI %d RNTI %x: pucch2_trials %d, pucch2_DTX %d\n",
                        i,
                        stats->rnti,
                        uci_stats->pucch2_trials,
                        uci_stats->pucch2_DTX);
  }
  if (fd)
    fprintf(fd, "%s", output);
  else
    printf("%s", output);
}
