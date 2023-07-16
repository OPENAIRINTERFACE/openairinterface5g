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

/*! \file PHY/NR_TRANSPORT/nr_prach.c
 * \brief Routines for UE PRACH physical channel
 * \author R. Knopp, G. Casati
 * \date 2019
 * \version 0.2
 * \company Eurecom, Fraunhofer IIS
 * \email: knopp@eurecom.fr, guido.casati@iis.fraunhofer.de
 * \note
 * \warning
 */
#include "PHY/sse_intrin.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "PHY/impl_defs_nr.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "T.h"

//#define NR_PRACH_DEBUG 1
#include "openair1/PHY/NR_TRANSPORT/nr_prach.h"

// Note:
// - prach_fmt_id is an ID used to map to the corresponding PRACH format value in prachfmt
// WIP todo:
// - take prach start symbol into account
// - idft for short sequence assumes we are transmitting starting in symbol 0 of a PRACH slot
// - Assumes that PRACH SCS is same as PUSCH SCS @ 30 kHz, take values for formats 0-2 and adjust for others below
// - Preamble index different from 0 is not detected by gNB
int32_t generate_nr_prach(PHY_VARS_NR_UE *ue, uint8_t gNB_id, int frame, uint8_t slot){

  NR_DL_FRAME_PARMS *fp=&ue->frame_parms;
  fapi_nr_config_request_t *nrUE_config = &ue->nrUE_config;
  fapi_nr_ul_config_prach_pdu *prach_pdu = &ue->prach_vars[gNB_id]->prach_pdu;

  uint8_t Mod_id, fd_occasion, preamble_index, restricted_set, not_found;
  uint16_t rootSequenceIndex, prach_fmt_id, NCS, preamble_offset = 0;
  const uint16_t *prach_root_sequence_map;
  uint16_t preamble_shift = 0, preamble_index0, n_shift_ra, n_shift_ra_bar, d_start=INT16_MAX, numshift, N_ZC, u, offset, offset2, first_nonzero_root_idx;
  c16_t prach[(4688 + 4 * 24576) * 2] __attribute__((aligned(32))) = {0};
  int16_t prachF_tmp[(4688+4*24576)*4*2] __attribute__((aligned(32))) = {0};

  int16_t Ncp = 0;
  int prach_start, prach_sequence_length, i, prach_len, dftlen, mu, kbar, K, n_ra_prb, k, prachStartSymbol, sample_offset_slot;

  fd_occasion             = 0;
  prach_len               = 0;
  dftlen                  = 0;
  first_nonzero_root_idx = 0;
  int16_t amp = ue->prach_vars[gNB_id]->amp;
  int16_t *prachF = prachF_tmp;
  Mod_id                  = ue->Mod_id;
  prach_sequence_length   = nrUE_config->prach_config.prach_sequence_length;
  N_ZC                    = (prach_sequence_length == 0) ? 839:139;
  mu                      = nrUE_config->prach_config.prach_sub_c_spacing;
  restricted_set          = prach_pdu->restricted_set;
  rootSequenceIndex       = prach_pdu->root_seq_id;
  n_ra_prb                = nrUE_config->prach_config.num_prach_fd_occasions_list[fd_occasion].k1,//prach_pdu->freq_msg1;
  NCS                     = prach_pdu->num_cs;
  prach_fmt_id            = prach_pdu->prach_format;
  preamble_index          = prach_pdu->ra_PreambleIndex;
  kbar                    = 1;
  K                       = 24;
  k                       = 12*n_ra_prb - 6*fp->N_RB_UL;
  prachStartSymbol        = prach_pdu->prach_start_symbol;

  LOG_D(PHY,"Generate NR PRACH %d.%d\n", frame, slot);

  compute_nr_prach_seq(nrUE_config->prach_config.prach_sequence_length,
                       nrUE_config->prach_config.num_prach_fd_occasions_list[fd_occasion].num_root_sequences,
                       nrUE_config->prach_config.num_prach_fd_occasions_list[fd_occasion].prach_root_sequence_index,
                       ue->X_u);

  if (prachStartSymbol == 0) {
    sample_offset_slot = 0;
  } else if (fp->slots_per_subframe == 1) {
    if (prachStartSymbol <= 7)
      sample_offset_slot = (fp->ofdm_symbol_size + fp->nb_prefix_samples) * (prachStartSymbol - 1) + (fp->ofdm_symbol_size + fp->nb_prefix_samples0);
    else
      sample_offset_slot = (fp->ofdm_symbol_size + fp->nb_prefix_samples) * (prachStartSymbol - 2) + (fp->ofdm_symbol_size + fp->nb_prefix_samples0) * 2;
  } else {
    if (slot % (fp->slots_per_subframe / 2) == 0)
      sample_offset_slot = (fp->ofdm_symbol_size + fp->nb_prefix_samples) * (prachStartSymbol - 1) + (fp->ofdm_symbol_size + fp->nb_prefix_samples0);
    else
      sample_offset_slot = (fp->ofdm_symbol_size + fp->nb_prefix_samples) * prachStartSymbol;
  }

  prach_start = fp->get_samples_slot_timestamp(slot, fp, 0) + sample_offset_slot;

  //printf("prachstartsymbold %d, sample_offset_slot %d, prach_start %d\n",prachStartSymbol, sample_offset_slot, prach_start);

  // First compute physical root sequence
  /************************************************************************
  * 4G and NR NCS tables are slightly different and depend on prach format
  * Table 6.3.3.1-5:  for preamble formats with delta_f_RA = 1.25 Khz (formats 0,1,2)
  * Table 6.3.3.1-6:  for preamble formats with delta_f_RA = 5 Khz (formats 3)
  * NOTE: Restricted set type B is not implemented
  *************************************************************************/

  prach_root_sequence_map = (prach_sequence_length == 0) ? prach_root_sequence_map_0_3 : prach_root_sequence_map_abc;

  if (restricted_set == 0) {
    // This is the relative offset (for unrestricted case) in the root sequence table (5.7.2-4 from 36.211) for the given preamble index
    preamble_offset = ((NCS==0)? preamble_index : (preamble_index/(N_ZC/NCS)));
    // This is the \nu corresponding to the preamble index
    preamble_shift  = (NCS==0)? 0 : (preamble_index % (N_ZC/NCS));
    preamble_shift *= NCS;
  } else { // This is the high-speed case

    #ifdef NR_PRACH_DEBUG
      LOG_I(PHY, "PRACH [UE %d] High-speed mode, NCS %d\n", Mod_id, NCS);
    #endif

    not_found = 1;
    nr_fill_du(N_ZC,prach_root_sequence_map);
    preamble_index0 = preamble_index;
    // set preamble_offset to initial rootSequenceIndex and look if we need more root sequences for this
    // preamble index and find the corresponding cyclic shift
    preamble_offset = 0; // relative rootSequenceIndex;

    while (not_found == 1) {
      // current root depending on rootSequenceIndex and preamble_offset
      int index = (rootSequenceIndex + preamble_offset) % N_ZC;
      uint16_t n_group_ra = 0;

      if (prach_fmt_id<4) {
        // prach_root_sequence_map points to prach_root_sequence_map0_3
        DevAssert( index < sizeof(prach_root_sequence_map_0_3) / sizeof(prach_root_sequence_map_0_3[0]) );
      } else {
        // prach_root_sequence_map points to prach_root_sequence_map4
        DevAssert( index < sizeof(prach_root_sequence_map_abc) / sizeof(prach_root_sequence_map_abc[0]) );
      }

      u = prach_root_sequence_map[index];

      if ( (nr_du[u]<(N_ZC/3)) && (nr_du[u]>=NCS) ) {
        n_shift_ra     = nr_du[u]/NCS;
        d_start        = (nr_du[u]<<1) + (n_shift_ra * NCS);
        n_group_ra     = N_ZC/d_start;
        n_shift_ra_bar = max(0,(N_ZC-(nr_du[u]<<1)-(n_group_ra*d_start))/N_ZC);
      } else if  ( (nr_du[u]>=(N_ZC/3)) && (nr_du[u]<=((N_ZC - NCS)>>1)) ) {
        n_shift_ra     = (N_ZC - (nr_du[u]<<1))/NCS;
        d_start        = N_ZC - (nr_du[u]<<1) + (n_shift_ra * NCS);
        n_group_ra     = nr_du[u]/d_start;
        n_shift_ra_bar = min(n_shift_ra,max(0,(nr_du[u]- (n_group_ra*d_start))/NCS));
      } else {
        n_shift_ra     = 0;
        n_shift_ra_bar = 0;
      }

      // This is the number of cyclic shifts for the current root u
      numshift = (n_shift_ra*n_group_ra) + n_shift_ra_bar;

      if (numshift>0 && preamble_index0==preamble_index)
        first_nonzero_root_idx = preamble_offset;

      if (preamble_index0 < numshift) {
        not_found      = 0;
        preamble_shift = (d_start * (preamble_index0/n_shift_ra)) + ((preamble_index0%n_shift_ra)*NCS);

      } else { // skip to next rootSequenceIndex and recompute parameters
        preamble_offset++;
        preamble_index0 -= numshift;
      }
    }
  }

  // now generate PRACH signal
#ifdef NR_PRACH_DEBUG
    if (NCS>0)
      LOG_I(PHY, "PRACH [UE %d] generate PRACH in frame.slot %d.%d for RootSeqIndex %d, Preamble Index %d, PRACH Format %s, NCS %d (N_ZC %d): Preamble_offset %d, Preamble_shift %d msg1 frequency start %d\n",
        Mod_id,
        frame,
        slot,
        rootSequenceIndex,
        preamble_index,
        prachfmt[prach_fmt_id],
        NCS,
        N_ZC,
        preamble_offset,
        preamble_shift,
        n_ra_prb);
  #endif

  //  nsymb = (frame_parms->Ncp==0) ? 14:12;
  //  subframe_offset = (unsigned int)frame_parms->ofdm_symbol_size*slot*nsymb;

  if (prach_sequence_length == 0 && prach_fmt_id == 3) {
    K = 4;
    kbar = 10;
  } else if (prach_sequence_length == 1) {
    K = 1;
    kbar = 2;
  }

  if (k<0)
    k += fp->ofdm_symbol_size;

  k *= K;
  k += kbar;
  k *= 2;

  LOG_I(PHY, "PRACH [UE %d] in frame.slot %d.%d, placing PRACH in position %d, msg1 frequency start %d (k1 %d), preamble_offset %d, first_nonzero_root_idx %d\n",
        Mod_id,
        frame,
        slot,
        k,
        n_ra_prb,
        nrUE_config->prach_config.num_prach_fd_occasions_list[fd_occasion].k1,
        preamble_offset,
        first_nonzero_root_idx);

  // Ncp and dftlen here is given in terms of T_s wich is 30.72MHz sampling
  if (prach_sequence_length == 0) {
    switch (prach_fmt_id) {
    case 0:
      Ncp = 3168;
      dftlen = 24576;
      break;

    case 1:
      Ncp = 21024;
      dftlen = 24576;
      break;

    case 2:
      Ncp = 4688;
      dftlen = 24576;
      break;

    case 3:
      Ncp = 3168;
      dftlen = 6144;
      break;

    default:
      AssertFatal(1==0, "Illegal PRACH format %d for sequence length 839\n", prach_fmt_id);
      break;
    }
  } else {
    switch (prach_fmt_id) {
    case 4: //A1
      Ncp = 288 >> mu;
      break;

    case 5: //A2
      Ncp = 576 >> mu;
      break;

    case 6: //A3
      Ncp = 864 >> mu;
      break;

    case 7: //B1
      Ncp = 216 >> mu;
      break;

    /*
    case 4: //B2
      Ncp = 360 >> mu;
      break;

    case 5: //B3
      Ncp = 504 >> mu;
      break;
    */

    case 8: //B4
      Ncp = 936 >> mu;
      break;

    case 9: //C0
      Ncp = 1240 >> mu;
      break;

    case 10: //C2
      Ncp = 2048 >> mu;
      break;

    default:
      AssertFatal(1==0,"Unknown PRACH format ID %d\n", prach_fmt_id);
      break;
    }
    dftlen = 2048 >> mu;
  }

  //actually what we should be checking here is how often the current prach crosses a 0.5ms boundary. I am not quite sure for which paramter set this would be the case, so I will ignore it for now and just check if the prach starts on a 0.5ms boundary
  if(fp->numerology_index == 0) {
    if (prachStartSymbol == 0 || prachStartSymbol == 7)
      Ncp += 16;
  }
  else {
    if (slot%(fp->slots_per_subframe/2)==0 && prachStartSymbol == 0)
      Ncp += 16;
  }

  switch(fp->samples_per_subframe) {
  case 7680:
    // 5 MHz @ 7.68 Ms/s
    Ncp >>= 2;
    dftlen >>= 2;
    break;

  case 15360:
    // 10, 15 MHz @ 15.36 Ms/s
    Ncp >>= 1;
    dftlen >>= 1;
    break;

  case 30720:
    // 20, 25, 30 MHz @ 30.72 Ms/s
    break;

  case 46080:
    // 40 MHz @ 46.08 Ms/s
    Ncp = (Ncp*3)/2;
    dftlen = (dftlen*3)/2;
    break;

  case 61440:
    // 40, 50, 60 MHz @ 61.44 Ms/s
    Ncp <<= 1;
    dftlen <<= 1;
    break;

  case 92160:
    // 50, 60, 70, 80, 90 MHz @ 92.16 Ms/s
    Ncp *= 3;
    dftlen *= 3;
    break;

  case 122880:
    // 70, 80, 90, 100 MHz @ 122.88 Ms/s
    Ncp <<= 2;
    dftlen <<= 2;
    break;

  case 184320:
    // 100 MHz @ 184.32 Ms/s
    Ncp = Ncp*6;
    dftlen = dftlen*6;
    break;

  default:
    AssertFatal(1==0,"sample rate %f MHz not supported for numerology %d\n", fp->samples_per_subframe / 1000.0, mu);
  }

  #ifdef NR_PRACH_DEBUG
    LOG_I(PHY, "PRACH [UE %d] Ncp %d, dftlen %d \n", Mod_id, Ncp, dftlen);
  #endif

  /********************************************************
   *
   * In function init_prach_tables:
   * to compute quantized roots of unity ru(n) = 32767 * exp j*[ (2 * PI * n) / N_ZC ]
   *
   * In compute_prach_seq:
   * to calculate Xu = DFT xu = xu (inv_u*k) * Xu[0] (This is a Zadoff-Chou sequence property: DFT ZC sequence is another ZC sequence)
   *
   * In generate_prach:
   * to do the cyclic-shifted DFT by multiplying Xu[k] * ru[k*preamble_shift] as:
   * If X[k] = DFT x(n) -> X_shifted[k] = DFT x(n+preamble_shift) = X[k] * exp -j*[ (2*PI*k*preamble_shift) / N_ZC ]
   *
   *********************************************************/

    c16_t *Xu = ue->X_u[preamble_offset - first_nonzero_root_idx];

#if defined (PRACH_WRITE_OUTPUT_DEBUG)
    LOG_M("X_u.m", "X_u", (int16_t*)ue->X_u[preamble_offset-first_nonzero_root_idx], N_ZC, 1, 1);
  #endif

  for (offset=0,offset2=0; offset<N_ZC; offset++,offset2+=preamble_shift) {

    if (offset2 >= N_ZC)
      offset2 -= N_ZC;
    const int32_t Xu_re = (Xu[offset].r * amp) >> 15;
    const int32_t Xu_im = (Xu[offset].i * amp) >> 15;
    prachF[k++] = (Xu_re * nr_ru[offset2].r - Xu_im * nr_ru[offset2].i) >> 15;
    prachF[k++] = (Xu_im * nr_ru[offset2].r + Xu_re * nr_ru[offset2].i) >> 15;

    if (k==dftlen) k=0;
  }

  #if defined (PRACH_WRITE_OUTPUT_DEBUG)
    LOG_M("prachF.m", "prachF", &prachF[1804], 1024, 1, 1);
    LOG_M("Xu.m", "Xu", Xu, N_ZC, 1, 1);
  #endif

  // This is after cyclic prefix
    c16_t *prach2 = prach + Ncp;
    const idft_size_idx_t idft_size = get_idft(dftlen);
    idft(idft_size, prachF, (int16_t *)prach, 1);
    memmove(prach2, prach, (dftlen << 2));

    if (prach_sequence_length == 0) {
      if (prach_fmt_id == 0) {
        // here we have | empty  | Prach |
        memcpy(prach, prach + dftlen, (Ncp << 2));
        // here we have | Prefix | Prach |
        prach_len = dftlen + Ncp;
      } else if (prach_fmt_id == 1) {
        // here we have | empty  | Prach | empty |
        memcpy(prach2 + dftlen, prach2, (dftlen << 2));
        // here we have | empty  | Prach | Prach |
        memcpy(prach, prach + dftlen * 2, (Ncp << 2));
        // here we have | Prefix | Prach | Prach |
        prach_len = (dftlen * 2) + Ncp;
      } else if (prach_fmt_id == 2 || prach_fmt_id == 3) {
        // here we have | empty  | Prach | empty | empty | empty |
        memcpy(prach2 + dftlen, prach2, (dftlen << 2));
        // here we have | empty  | Prach | Prach | empty | empty |
        memcpy(prach2 + dftlen * 2, prach2, (dftlen << 3));
        // here we have | empty  | Prach | Prach | Prach | Prach |
        memcpy(prach, prach + dftlen * 4, (Ncp << 2));
        // here we have | Prefix | Prach | Prach | Prach | Prach |
        prach_len = (dftlen * 4) + Ncp;
      }
  } else { // short PRACH sequence
    if (prach_fmt_id == 9) {
      // here we have | empty  | Prach |
      memcpy(prach, prach + dftlen, (Ncp << 2));
      // here we have | Prefix | Prach |
      prach_len = (dftlen*1)+Ncp;
    } else if (prach_fmt_id == 4 || prach_fmt_id == 7) {
      // here we have | empty  | Prach | empty |
      memcpy(prach2 + dftlen, prach2, (dftlen << 2));
      // here we have | empty  | Prach | Prach |
      memcpy(prach, prach+(dftlen<<1), (Ncp<<2));
      // here we have | Prefix | Prach | Prach |
      prach_len = (dftlen*2)+Ncp;
    } else if (prach_fmt_id == 5 || prach_fmt_id == 10) { // 4xdftlen
      // here we have | empty  | Prach | empty | empty | empty |
      memcpy(prach2 + dftlen, prach2, (dftlen << 2));
      // here we have | empty  | Prach | Prach | empty | empty |
      memcpy(prach2 + dftlen * 2, prach2, (dftlen << 3));
      // here we have | empty  | Prach | Prach | Prach | Prach |
      memcpy(prach, prach + dftlen, (Ncp << 2));
      // here we have | Prefix | Prach | Prach | Prach | Prach |
      prach_len = (dftlen*4)+Ncp;
    } else if (prach_fmt_id == 6) { // 6xdftlen
      // here we have | empty  | Prach | empty | empty | empty | empty | empty |
      memcpy(prach2 + dftlen, prach2, (dftlen << 2));
      // here we have | empty  | Prach | Prach | empty | empty | empty | empty |
      memcpy(prach2 + dftlen * 2, prach2, (dftlen << 3));
      // here we have | empty  | Prach | Prach | Prach | Prach | empty | empty |
      memcpy(prach2 + dftlen * 4, prach2, (dftlen << 3));
      // here we have | empty  | Prach | Prach | Prach | Prach | Prach | Prach |
      memcpy(prach, prach + dftlen, (Ncp << 2));
      // here we have | Prefix | Prach | Prach | Prach | Prach | Prach | Prach |
      prach_len = (dftlen*6)+Ncp;
    } else if (prach_fmt_id == 8) { // 12xdftlen
      // here we have | empty  | Prach | empty | empty | empty | empty | empty | empty | empty | empty | empty | empty | empty |
      memcpy(prach2 + dftlen, prach2, (dftlen << 2));
      // here we have | empty  | Prach | Prach | empty | empty | empty | empty | empty | empty | empty | empty | empty | empty |
      memcpy(prach2 + dftlen * 2, prach2, (dftlen << 3));
      // here we have | empty  | Prach | Prach | Prach | Prach | empty | empty | empty | empty | empty | empty | empty | empty |
      memcpy(prach2 + dftlen * 4, prach2, (dftlen << 3));
      // here we have | empty  | Prach | Prach | Prach | Prach | Prach | Prach | empty | empty | empty | empty | empty | empty |
      memcpy(prach2 + dftlen * 6, prach2, (dftlen << 2) * 6);
      // here we have | empty  | Prach | Prach | Prach | Prach | Prach | Prach | Prach | Prach | Prach | Prach | Prach | Prach |
      memcpy(prach, prach + dftlen, (Ncp << 2));
      // here we have | Prefix | Prach | Prach | Prach | Prach | Prach | Prach | Prach | Prach | Prach | Prach | Prach | Prach |
      prach_len = (dftlen*12)+Ncp;
    }
  }

  #ifdef NR_PRACH_DEBUG
    LOG_I(PHY, "PRACH [UE %d] N_RB_UL %d prach_start %d, prach_len %d\n", Mod_id,
      fp->N_RB_UL,
      prach_start,
      prach_len);
  #endif

    for (i = 0; i < prach_len; i++)
      ue->common_vars.txData[0][prach_start + i] = prach[i];

#ifdef PRACH_WRITE_OUTPUT_DEBUG
    LOG_M("prach_tx0.m", "prachtx0", prach+(Ncp<<1), prach_len-Ncp, 1, 1);
    LOG_M("Prach_txsig.m","txs",(int16_t*)(&ue->common_vars.txdata[0][prach_start]), 2*(prach_start+prach_len), 1, 1)
  #endif

  return signal_energy((int*)prach, 256);
}

