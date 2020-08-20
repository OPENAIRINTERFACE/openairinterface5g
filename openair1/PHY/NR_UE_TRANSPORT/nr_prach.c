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

extern uint16_t prach_root_sequence_map_0_3[838];
extern uint16_t prach_root_sequence_map_abc[138];
extern uint16_t nr_du[838];
extern int16_t nr_ru[2*839];
extern const char *prachfmt[9];
extern const char *prachfmt03[4];

// Note:
// - prach_fmt_id is an ID used to map to the corresponding PRACH format value in prachfmt
// WIP todo:
// - take prach start symbol into account
// - idft for short sequence assumes we are transmitting starting in symbol 0 of a PRACH slot
// - Assumes that PRACH SCS is same as PUSCH SCS @ 30 kHz, take values for formats 0-2 and adjust for others below
// - Preamble index different from 0 is not detected by gNB
int32_t generate_nr_prach(PHY_VARS_NR_UE *ue, uint8_t gNB_id, uint8_t slot){

  NR_DL_FRAME_PARMS *fp=&ue->frame_parms;
  fapi_nr_config_request_t *nrUE_config = &ue->nrUE_config;
  NR_PRACH_RESOURCES_t *prach_resources = ue->prach_resources[gNB_id];
  fapi_nr_ul_config_prach_pdu *prach_pdu = &ue->prach_vars[gNB_id]->prach_pdu;

  uint8_t Mod_id, fd_occasion, preamble_index, restricted_set, not_found;
  uint16_t rootSequenceIndex, prach_fmt_id, NCS, *prach_root_sequence_map, preamble_offset = 0;
  uint16_t preamble_shift = 0, preamble_index0, n_shift_ra, n_shift_ra_bar, d_start, numshift, N_ZC, u, offset, offset2, first_nonzero_root_idx;
  int16_t prach_tmp[98304*2*4] __attribute__((aligned(32)));

  int16_t Ncp = 0, amp, *prach, *prach2, *prachF, *Xu;
  int32_t Xu_re, Xu_im, samp_count;
  int prach_start, prach_sequence_length, i, prach_len, dftlen, mu, kbar, K, n_ra_prb, k;
  //int restricted_Type;

  prach                   = prach_tmp;
  prachF                  = ue->prach_vars[gNB_id]->prachF;
  amp                     = ue->prach_vars[gNB_id]->amp;
  Mod_id                  = ue->Mod_id;
  prach_sequence_length   = nrUE_config->prach_config.prach_sequence_length;
  N_ZC                    = (prach_sequence_length == 0) ? 839:139;
  mu                      = nrUE_config->prach_config.prach_sub_c_spacing;
  restricted_set          = prach_pdu->restricted_set;
  rootSequenceIndex       = prach_pdu->root_seq_id;
  n_ra_prb                = prach_pdu->freq_msg1;
  NCS                     = prach_pdu->num_cs;
  prach_fmt_id            = prach_pdu->prach_format;
  preamble_index          = prach_resources->ra_PreambleIndex;
  fd_occasion             = 0;
  prach_len               = 0;
  dftlen                  = 0;
  first_nonzero_root_idx  = 0;
  kbar                    = 1;
  K                       = 24;
  k                       = 12*n_ra_prb - 6*fp->N_RB_UL;
  //prachStartSymbol     = prach_config_pdu->prach_start_symbol
  //restricted_Type         = 0;

  compute_nr_prach_seq(nrUE_config->prach_config.prach_sequence_length,
                       nrUE_config->prach_config.num_prach_fd_occasions_list[fd_occasion].num_root_sequences,
                       nrUE_config->prach_config.num_prach_fd_occasions_list[fd_occasion].prach_root_sequence_index,
                       ue->X_u);

  if (mu == 0)
    samp_count = fp->samples_per_subframe;
  else
    samp_count = (slot%(fp->slots_per_subframe/2)) ? fp->samples_per_slotN0 : fp->samples_per_slot0;

  #ifdef OAI_USRP
    prach_start = (ue->rx_offset + slot*samp_count - ue->hw_timing_advance - ue->N_TA_offset);
  #else //normal case (simulation)
    prach_start = slot*samp_count - ue->N_TA_offset;
  #endif

  if (prach_start<0)
    prach_start += (fp->samples_per_subframe*NR_NUMBER_OF_SUBFRAMES_PER_FRAME);

  if (prach_start >= (fp->samples_per_subframe*NR_NUMBER_OF_SUBFRAMES_PER_FRAME))
    prach_start -= (fp->samples_per_subframe*NR_NUMBER_OF_SUBFRAMES_PER_FRAME);

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
      LOG_I(PHY, "PRACH [UE %d] generate PRACH in slot %d for RootSeqIndex %d, Preamble Index %d, PRACH Format %s, NCS %d (N_ZC %d): Preamble_offset %d, Preamble_shift %d msg1 frequency start %d\n",
        Mod_id,
        slot,
        rootSequenceIndex,
        preamble_index,
        prach_sequence_length == 0 ? prachfmt03[prach_fmt_id]  : prachfmt[prach_fmt_id],
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

  LOG_I(PHY, "PRACH [UE %d] in slot %d, placing PRACH in position %d, msg1 frequency start %d, preamble_offset %d, first_nonzero_root_idx %d\n", Mod_id,
    slot,
    k,
    n_ra_prb,
    preamble_offset,
    first_nonzero_root_idx);

  Xu = (int16_t*)ue->X_u[preamble_offset-first_nonzero_root_idx];

  #if defined (PRACH_WRITE_OUTPUT_DEBUG)
    LOG_M("X_u.m", "X_u", (int16_t*)ue->X_u[preamble_offset-first_nonzero_root_idx], N_ZC, 1, 1);
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

  if (fp->N_RB_UL <= 100)
    AssertFatal(1 == 0, "N_RB_UL %d not support for NR PRACH yet\n", fp->N_RB_UL);
  else if (fp->N_RB_UL < 137) {
    if (fp->threequarter_fs == 0) {
      //40 MHz @ 61.44 Ms/s
      //50 MHz @ 61.44 Ms/s
      if (prach_sequence_length == 0) {
        if (prach_fmt_id == 0 || prach_fmt_id == 1 || prach_fmt_id == 2)
            dftlen = 49152;
        if (prach_fmt_id == 3)
            dftlen = 12288;
      } // 839 sequence
      else {
        switch (mu){
          case 1:
            dftlen = 2048;
            break;
          default:
            AssertFatal(1 == 0, "Shouldn't get here\n");
            break;
        }
      }
    } else { // threequarter sampling
      //  40 MHz @ 46.08 Ms/s
      if (prach_sequence_length == 0) {
        AssertFatal(fp->N_RB_UL <= 107, "cannot do 108..136 PRBs with 3/4 sampling\n");
        if (prach_fmt_id == 0 || prach_fmt_id == 1 || prach_fmt_id == 2)
          dftlen = 36864;
        if (prach_fmt_id == 3)
          dftlen = 9216;
      } else {
        switch (mu){
          case 1:
            dftlen = 1536;
          break;
          default:
            AssertFatal(1 == 0, "Shouldn't get here\n");
            break;
        }
      } // short format
    } // 3/4 sampling
  } // <=50 MHz BW
  else if (fp->N_RB_UL <= 273) {
    if (fp->threequarter_fs == 0) {
    //80,90,100 MHz @ 122.88 Ms/s
      if (prach_sequence_length == 0) {
        if (prach_fmt_id == 0 || prach_fmt_id == 1 || prach_fmt_id == 2)
          dftlen = 98304;
        if (prach_fmt_id == 3)
          dftlen = 24576;
      }
    } else { // threequarter sampling
      switch (mu){
        case 1:
          dftlen = 4096;
          break;
        default:
          AssertFatal(1 == 0, "Shouldn't get here\n");
          break;
      }
    }
  } else {
    AssertFatal(fp->N_RB_UL <= 217, "cannot do more than 217 PRBs with 3/4 sampling\n");
    //  80 MHz @ 92.16 Ms/s
    if (prach_sequence_length == 0) {
      if (prach_fmt_id == 0 || prach_fmt_id == 1 || prach_fmt_id == 2)
        dftlen = 73728;
      if (prach_fmt_id == 3)
        dftlen = 18432;
    } else {
      switch (mu){
        case 1:
          dftlen = 3072;
          break;
        default:
          AssertFatal(1 == 0, "Shouldn't get here\n");
          break;
      }
    }
  }

  for (offset=0,offset2=0; offset<N_ZC; offset++,offset2+=preamble_shift) {

    if (offset2 >= N_ZC)
      offset2 -= N_ZC;

    Xu_re = (((int32_t)Xu[offset<<1]*amp)>>15);
    Xu_im = (((int32_t)Xu[1+(offset<<1)]*amp)>>15);
    prachF[k++]= ((Xu_re*nr_ru[offset2<<1]) - (Xu_im*nr_ru[1+(offset2<<1)]))>>15;
    prachF[k++]= ((Xu_im*nr_ru[offset2<<1]) + (Xu_re*nr_ru[1+(offset2<<1)]))>>15;

    if (k==dftlen) k=0;
  }

  #if defined (PRACH_WRITE_OUTPUT_DEBUG)
    LOG_M("prachF.m", "prachF", &prachF[1804], 1024, 1, 1);
    LOG_M("Xu.m", "Xu", Xu, N_ZC, 1, 1);
  #endif

  if (prach_sequence_length == 0) {

    AssertFatal(prach_fmt_id < 4, "Illegal PRACH format %d for sequence length 839\n", prach_fmt_id);

    switch (prach_fmt_id) {
    case 0:
      Ncp = 3168;
      break;
    case 1:
      Ncp = 21024;
      break;
    case 2:
      Ncp = 4688;
      break;
    case 3:
      Ncp = 3168;
      break;
    }

  } else {

    switch (prach_fmt_id) {
    case 0: //A1
      Ncp = 288/(1<<mu);
      break;
    case 1: //A2
      Ncp = 576/(1<<mu);
      break;
    case 2: //A3
      Ncp = 864/(1<<mu);
      break;
    case 3: //B1
      Ncp = 216/(1<<mu);
    break;
    case 4: //B2
      Ncp = 360/(1<<mu);
      break;
    case 5: //B3
      Ncp = 504/(1<<mu);
      break;
    case 6: //B4
      Ncp = 936/(1<<mu);
      break;
    case 7: //C0
      Ncp = 1240/(1<<mu);
      break;
    case 8: //C2
      Ncp = 2048/(1<<mu);
      break;
    default:
      AssertFatal(1==0,"Unknown PRACH format ID %d\n", prach_fmt_id);
      break;
    }
  }

  #ifdef NR_PRACH_DEBUG
    LOG_D(PHY, "PRACH [UE %d] Ncp %d, dftlen %d \n", Mod_id, Ncp, dftlen);
  #endif

  if (fp->N_RB_UL <= 100)
    AssertFatal(1==0,"N_RB_UL %d not supported for NR PRACH yet\n",fp->N_RB_UL);
  else if (fp->N_RB_UL < 137) { // 46.08 or 61.44 Ms/s
    if (fp->threequarter_fs == 0) { // full sampling @ 61.44 Ms/s
      Ncp<<=1;
      // This is after cyclic prefix (Ncp<<1 samples for 30.72 Ms/s, Ncp<<2 samples for 61.44 Ms/s
      prach2 = prach+(Ncp<<1);
      if (prach_sequence_length == 0){
        if (prach_fmt_id == 0) { //24576 samples @ 30.72 Ms/s, 49152 samples @ 61.44 Ms/s
          idft(IDFT_49152,prachF,prach2,1);
          // here we have |empty | Prach49152|
          memmove(prach,prach+(49152<<1),(Ncp<<2));
          // here we have |Prefix | Prach49152|
          prach_len = 49152+Ncp;
        } else if (prach_fmt_id == 1) { //24576 samples @ 30.72 Ms/s, 49152 samples @ 61.44 Ms/s
          idft(IDFT_49152,prachF,prach2,1);
          memmove(prach2+(49152<<1),prach2,(49152<<2));
          // here we have |empty | Prach49152 | Prach49152|
          memmove(prach,prach+(49152<<2),(Ncp<<2));
          // here we have |Prefix | Prach49152 | Prach49152|
          prach_len = (49152*2)+Ncp;
        } else if (prach_fmt_id == 2) { //24576 samples @ 30.72 Ms/s, 49152 samples @ 61.44 Ms/s
          idft(IDFT_49152,prachF,prach2,1);
          memmove(prach2+(49152<<1),prach2,(49152<<2));
          // here we have |empty | Prach49152 | Prach49152| empty49152 | empty49152
          memmove(prach2+(49152<<2),prach2,(49152<<3));
          // here we have |empty | Prach49152 | Prach49152| Prach49152 | Prach49152
          memmove(prach,prach+(49152<<3),(Ncp<<2));
          // here we have |Prefix | Prach49152 | Prach49152| Prach49152 | Prach49152
          prach_len = (49152*4)+Ncp;
        } else if (prach_fmt_id == 3) { // //6144 samples @ 30.72 Ms/s, 12288 samples @ 61.44 Ms/s
          idft(IDFT_12288,prachF,prach2,1);
          memmove(prach2+(12288<<1),prach2,(12288<<2));
          // here we have |empty | Prach12288 | Prach12288| empty12288 | empty12288
          memmove(prach2+(12288<<2),prach2,(12288<<3));
          // here we have |empty | Prach12288 | Prach12288| Prach12288 | Prach12288
          memmove(prach,prach+(12288<<3),(Ncp<<2));
          // here we have |Prefix | Prach12288 | Prach12288| Prach12288 | Prach12288
          prach_len = (12288*4)+Ncp;
        }
      } else { // short PRACH sequence
        if (prach_fmt_id == 0 || prach_fmt_id == 3 || prach_fmt_id == 7) {
          Ncp+=32; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_2048,prachF,prach2,1);
          // here we have |empty | Prach2048 |
          if (prach_fmt_id != 7) {
            memmove(prach2+(2048<<1),prach2,(2048<<2));
            prach_len = (2048*2)+Ncp;
          }
          else prach_len = (2048*1)+Ncp;
          memmove(prach,prach+(2048<<1),(Ncp<<2));
          // here we have |Prefix | Prach2048 | Prach2048 (if ! 0xc0)  |
        } else if (prach_fmt_id == 1 || prach_fmt_id == 4) { // 6x2048
          Ncp+=32; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_2048,prachF,prach2,1);
          // here we have |empty | Prach2048 |
          memmove(prach2+(2048<<1),prach2,(2048<<2));
          // here we have |empty | Prach2048 | Prach2048| empty2048 | empty2048 |
          memmove(prach2+(2048<<2),prach2,(2048<<3));
          // here we have |empty | Prach2048 | Prach2048| Prach2048 | Prach2048 |
          memmove(prach,prach+(2048<<1),(Ncp<<2));
          // here we have |Prefix | Prach2048 |
          prach_len = (2048*4)+Ncp;
        } else if (prach_fmt_id == 2 || prach_fmt_id == 5) { // 6x2048
          Ncp+=32;
          prach2 = prach+(Ncp<<1);
          idft(IDFT_2048,prachF,prach2,1);
          // here we have |empty | Prach2048 |
          memmove(prach2+(2048<<1),prach2,(2048<<2));
          // here we have |empty | Prach2048 | Prach2048| empty2048 | empty2048 | empty2048 | empty2048
          memmove(prach2+(2048<<2),prach2,(2048<<3));
          // here we have |empty | Prach2048 | Prach2048| Prach2048 | Prach2048 | empty2048 | empty2048
          memmove(prach2+(2048<<3),prach2,(2048<<3));
          // here we have |empty | Prach2048 | Prach2048| Prach2048 | Prach2048 | Prach2048 | Prach2048
          memmove(prach,prach+(2048<<1),(Ncp<<2));
          // here we have |Prefix | Prach2048 |
          prach_len = (2048*6)+Ncp;
        } else if (prach_fmt_id == 6) { // 12x2048
          Ncp+=32; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_2048,prachF,prach2,1);
          // here we have |empty | Prach2048 |
          memmove(prach2+(2048<<1),prach2,(2048<<2));
          // here we have |empty | Prach2048 | Prach2048| empty2048 | empty2048 | empty2048 | empty2048
          memmove(prach2+(2048<<2),prach2,(2048<<3));
          // here we have |empty | Prach2048 | Prach2048| Prach2048 | Prach2048 | empty2048 | empty2048
          memmove(prach2+(2048<<3),prach2,(2048<<3));
          // here we have |empty | Prach2048 | Prach2048| Prach2048 | Prach2048 | Prach2048 | Prach2048
          memmove(prach2+(2048<<1)*6,prach2,(2048<<2)*6);
          // here we have |empty | Prach2048 | Prach2048| Prach2048 | Prach2048 | Prach2048 | Prach2048 | Prach2048 | Prach2048| Prach2048 | Prach2048 | Prach2048 | Prach2048|
          memmove(prach,prach+(2048<<1),(Ncp<<2));
          // here we have |Prefix | Prach2048 | Prach2048| Prach2048 | Prach2048 | Prach2048 | Prach2048 | Prach2048 | Prach2048| Prach2048 | Prach2048 | Prach2048 | Prach2048|
          prach_len = (2048*12)+Ncp;
        }
      }
    } else {  // threequarter sampling @ 46.08 Ms/s
      Ncp = (Ncp*3)/2;
      prach2 = prach+(Ncp<<1);
      if (prach_sequence_length == 0){
        if (prach_fmt_id == 0) {
          idft(IDFT_36864,prachF,prach2,1);
          // here we have |empty | Prach73728|
          memmove(prach,prach+(36864<<1),(Ncp<<2));
          // here we have |Prefix | Prach73728|
          prach_len = (36864*1)+Ncp;
        } else if (prach_fmt_id == 1) {
          idft(IDFT_36864,prachF,prach2,1);
          memmove(prach2+(36864<<1),prach2,(36864<<2));
          // here we have |empty | Prach73728 | Prach73728|
          memmove(prach,prach+(36864<<2),(Ncp<<2));
          // here we have |Prefix | Prach73728 | Prach73728|
          prach_len = (36864*2)+Ncp;
        } else if (prach_fmt_id == 2) {
          idft(IDFT_36864,prachF,prach2,1);
          memmove(prach2+(36864<<1),prach2,(36864<<2));
          // here we have |empty | Prach73728 | Prach73728| empty73728 | empty73728
          memmove(prach2+(36864<<2),prach2,(36864<<3));
          // here we have |empty | Prach73728 | Prach73728| Prach73728 | Prach73728
          memmove(prach,prach+(36864<<3),(Ncp<<2));
          // here we have |Prefix | Prach73728 | Prach73728| Prach73728 | Prach73728
          prach_len = (36864*4)+Ncp;
        } else if (prach_fmt_id == 3) {
          idft(IDFT_9216,prachF,prach2,1);
          memmove(prach2+(9216<<1),prach2,(9216<<2));
          // here we have |empty | Prach9216 | Prach9216| empty9216 | empty9216
          memmove(prach2+(9216<<2),prach2,(9216<<3));
          // here we have |empty | Prach9216 | Prach9216| Prach9216 | Prach9216
          memmove(prach,prach+(9216<<3),(Ncp<<2));
          // here we have |Prefix | Prach9216 | Prach9216| Prach9216 | Prach9216
          prach_len = (9216*4)+Ncp;
        }
      } else { // short sequence
        if (prach_fmt_id == 0 || prach_fmt_id == 3 || prach_fmt_id == 7) {
          Ncp+=24; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_1536,prachF,prach2,1);
          // here we have |empty | Prach1536 |
          if (prach_fmt_id != 7) {
            memmove(prach2+(1536<<1),prach2,(1536<<2));
            prach_len = (1536*2)+Ncp;
          }	else prach_len = (1536*1)+Ncp;

          memmove(prach,prach+(1536<<1),(Ncp<<2));
          // here we have |Prefix | Prach1536 | Prach1536 (if ! 0xc0) |

        } else if (prach_fmt_id == 1 || prach_fmt_id == 4) { // 6x1536

          Ncp+=24; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_1536,prachF,prach2,1);
          // here we have |empty | Prach1536 |
          memmove(prach2+(1536<<1),prach2,(1536<<2));
          // here we have |empty | Prach1536 | Prach1536| empty1536 | empty1536 |
          memmove(prach2+(1536<<2),prach2,(1536<<3));
          // here we have |empty | Prach1536 | Prach1536| Prach1536 | Prach1536 |
          memmove(prach,prach+(1536<<1),(Ncp<<2));
          // here we have |Prefix | Prach1536 |
          prach_len = (1536*4)+Ncp;
        } else if (prach_fmt_id == 2 || prach_fmt_id == 5) { // 6x1536
          Ncp+=24; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_1536,prachF,prach2,1);
          // here we have |empty | Prach1536 |
          memmove(prach2+(1536<<1),prach2,(1536<<2));
          // here we have |empty | Prach1536 | Prach1536| empty1536 | empty1536 | empty1536 | empty1536
          memmove(prach2+(1536<<2),prach2,(1536<<3));
          // here we have |empty | Prach1536 | Prach1536| Prach1536 | Prach1536 | empty1536 | empty1536
          memmove(prach2+(1536<<3),prach2,(1536<<3));
          // here we have |empty | Prach1536 | Prach1536| Prach1536 | Prach1536 | Prach1536 | Prach1536
          memmove(prach,prach+(1536<<1),(Ncp<<2));
          // here we have |Prefix | Prach1536 |
          prach_len = (1536*6)+Ncp;
        } else if (prach_fmt_id == 6) { // 12x1536
          Ncp+=24; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_1536,prachF,prach2,1);
          // here we have |empty | Prach1536 |
          memmove(prach2+(1536<<1),prach2,(1536<<2));
          // here we have |empty | Prach1536 | Prach1536| empty1536 | empty1536 | empty1536 | empty1536
          memmove(prach2+(1536<<2),prach2,(1536<<3));
          // here we have |empty | Prach1536 | Prach1536| Prach1536 | Prach1536 | empty1536 | empty1536
          memmove(prach2+(1536<<3),prach2,(1536<<3));
          // here we have |empty | Prach1536 | Prach1536| Prach1536 | Prach1536 | Prach1536 | Prach1536
          memmove(prach2+(1536<<1)*6,prach2,(1536<<2)*6);
          // here we have |empty | Prach1536 | Prach1536| Prach1536 | Prach1536 | Prach1536 | Prach1536 | Prach1536 | Prach1536| Prach1536 | Prach1536 | Prach1536 | Prach1536|
          memmove(prach,prach+(1536<<1),(Ncp<<2));
          // here we have |Prefix | Prach1536 | Prach1536| Prach1536 | Prach1536 | Prach1536 | Prach1536 | Prach1536 | Prach1536| Prach1536 | Prach1536 | Prach1536 | Prach1536|
          prach_len = (1536*12)+Ncp;
        }
      }
    }
  } else if (fp->N_RB_UL <= 273) {// 92.16 or 122.88 Ms/s
    if (fp->threequarter_fs == 0) { // full sampling @ 122.88 Ms/s
      Ncp<<=2;
      prach2 = prach+(Ncp<<1);
      if (prach_sequence_length == 0){
        if (prach_fmt_id == 0) { //24576 samples @ 30.72 Ms/s, 98304 samples @ 122.88 Ms/s
          idft(IDFT_98304,prachF,prach2,1);
          // here we have |empty | Prach98304|
          memmove(prach,prach+(98304<<1),(Ncp<<2));
          // here we have |Prefix | Prach98304|
          prach_len = (98304*1)+Ncp;
        } else if (prach_fmt_id == 1) {
          idft(IDFT_98304,prachF,prach2,1);
          memmove(prach2+(98304<<1),prach2,(98304<<2));
          // here we have |empty | Prach98304 | Prach98304|
          memmove(prach,prach+(98304<<2),(Ncp<<2));
          // here we have |Prefix | Prach98304 | Prach98304|
          prach_len = (98304*2)+Ncp;
        } else if (prach_fmt_id == 2) {
          idft(IDFT_98304,prachF,prach2,1);
          memmove(prach2+(98304<<1),prach2,(98304<<2));
          // here we have |empty | Prach98304 | Prach98304| empty98304 | empty98304
          memmove(prach2+(98304<<2),prach2,(98304<<3));
          // here we have |empty | Prach98304 | Prach98304| Prach98304 | Prach98304
          memmove(prach,prach+(98304<<3),(Ncp<<2));
          // here we have |Prefix | Prach98304 | Prach98304| Prach98304 | Prach98304
          prach_len = (98304*4)+Ncp;
        } else if (prach_fmt_id == 3) { // 4x6144, Ncp 3168
          idft(IDFT_24576,prachF,prach2,1);
          memmove(prach2+(24576<<1),prach2,(24576<<2));
          // here we have |empty | Prach24576 | Prach24576| empty24576 | empty24576
          memmove(prach2+(24576<<2),prach2,(24576<<3));
          // here we have |empty | Prach24576 | Prach24576| Prach24576 | Prach24576
          memmove(prach,prach+(24576<<3),(Ncp<<2));
          // here we have |Prefix | Prach24576 | Prach24576| Prach24576 | Prach24576
          prach_len = (24576*4)+Ncp;
        }
      } else { // short sequence
        if (prach_fmt_id == 0 || prach_fmt_id == 3 || prach_fmt_id == 7) {
          Ncp+=64; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_4096,prachF,prach2,1);
          // here we have |empty | Prach4096 |
          if (prach_fmt_id != 7) {
            memmove(prach2+(4096<<1),prach2,(4096<<2));
            prach_len = (4096*2)+Ncp; 
          }	else 	prach_len = (4096*1)+Ncp;
          memmove(prach,prach+(4096<<1),(Ncp<<2));
          // here we have |Prefix | Prach4096 | Prach4096 (if ! 0xc0) |
        } else if (prach_fmt_id == 1 || prach_fmt_id == 4) { // 4x4096
          Ncp+=64; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_4096,prachF,prach2,1);
          // here we have |empty | Prach4096 |
          memmove(prach2+(4096<<1),prach2,(4096<<2));
          // here we have |empty | Prach4096 | Prach4096| empty4096 | empty4096 |
          memmove(prach2+(4096<<2),prach2,(4096<<3));
          // here we have |empty | Prach4096 | Prach4096| Prach4096 | Prach4096 |
          memmove(prach,prach+(4096<<1),(Ncp<<2));
          // here we have |Prefix | Prach4096 |
          prach_len = (4096*4)+Ncp;
        } else if (prach_fmt_id == 2 || prach_fmt_id == 5) { // 6x4096
          Ncp+=64; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_4096,prachF,prach2,1);
          // here we have |empty | Prach4096 |
          memmove(prach2+(4096<<1),prach2,(4096<<2));
          // here we have |empty | Prach4096 | Prach4096| empty4096 | empty4096 | empty4096 | empty4096
          memmove(prach2+(4096<<2),prach2,(4096<<3));
          // here we have |empty | Prach4096 | Prach4096| Prach4096 | Prach4096 | empty4096 | empty4096
          memmove(prach2+(4096<<3),prach2,(4096<<3));
          // here we have |empty | Prach4096 | Prach4096| Prach4096 | Prach4096 | Prach4096 | Prach4096
          memmove(prach,prach+(4096<<1),(Ncp<<2));
          // here we have |Prefix | Prach4096 |
          prach_len = (4096*6)+Ncp;
        } else if (prach_fmt_id == 6) { // 12x4096
          Ncp+=64; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_4096,prachF,prach2,1);
          // here we have |empty | Prach4096 |
          memmove(prach2+(4096<<1),prach2,(4096<<2));
          // here we have |empty | Prach4096 | Prach4096| empty4096 | empty4096 | empty4096 | empty4096
          memmove(prach2+(4096<<2),prach2,(4096<<3));
          // here we have |empty | Prach4096 | Prach4096| Prach4096 | Prach4096 | empty4096 | empty4096
          memmove(prach2+(4096<<3),prach2,(4096<<3));
          // here we have |empty | Prach4096 | Prach4096| Prach4096 | Prach4096 | Prach4096 | Prach4096
          memmove(prach2+(4096<<1)*6,prach2,(4096<<2)*6);
          // here we have |empty | Prach4096 | Prach4096| Prach4096 | Prach4096 | Prach4096 | Prach4096 | Prach4096 | Prach4096| Prach4096 | Prach4096 | Prach4096 | Prach4096|
          memmove(prach,prach+(4096<<1),(Ncp<<2));
          // here we have |Prefix | Prach4096 | Prach4096| Prach4096 | Prach4096 | Prach4096 | Prach4096 | Prach4096 | Prach4096| Prach4096 | Prach4096 | Prach4096 | Prach4096|
          prach_len = (4096*12)+Ncp;
        }
      }
    } else { // three quarter sampling @ 92.16 Ms/s
      Ncp = (Ncp*3);
      prach2 = prach+(Ncp<<1);
      if (prach_sequence_length == 0){
        if (prach_fmt_id == 0) {
          idft(IDFT_73728,prachF,prach2,1);
          // here we have |empty | Prach73728|
          memmove(prach,prach+(73728<<1),(Ncp<<2));
          // here we have |Prefix | Prach73728|
          prach_len = (73728*1)+Ncp;
        } else if (prach_fmt_id == 1) {
          idft(IDFT_73728,prachF,prach2,1);
          memmove(prach2+(73728<<1),prach2,(73728<<2));
          // here we have |empty | Prach73728 | Prach73728|
          memmove(prach,prach+(73728<<2),(Ncp<<2));
          // here we have |Prefix | Prach73728 | Prach73728|
          prach_len = (73728*2)+Ncp;
        } if (prach_fmt_id == 2) {
          idft(IDFT_73728,prachF,prach2,1);
          memmove(prach2+(73728<<1),prach2,(73728<<2));
          // here we have |empty | Prach73728 | Prach73728| empty73728 | empty73728
          memmove(prach2+(73728<<2),prach2,(73728<<3));
          // here we have |empty | Prach73728 | Prach73728| Prach73728 | Prach73728
          memmove(prach,prach+(73728<<3),(Ncp<<2));
          // here we have |Prefix | Prach73728 | Prach73728| Prach73728 | Prach73728
          prach_len = (73728*4)+Ncp;
        } else if (prach_fmt_id == 3) {
          idft(IDFT_18432,prachF,prach2,1);
          memmove(prach2+(18432<<1),prach2,(18432<<2));
          // here we have |empty | Prach18432 | Prach18432| empty18432 | empty18432
          memmove(prach2+(18432<<2),prach2,(18432<<3));
          // here we have |empty | Prach18432 | Prach18432| Prach18432 | Prach18432
          memmove(prach,prach+(18432<<3),(Ncp<<2));
          // here we have |Prefix | Prach18432 | Prach18432| Prach18432 | Prach18432
          prach_len = (18432*4)+Ncp;
        }
      } else { // short sequence
        if (prach_fmt_id == 0 || prach_fmt_id == 3 || prach_fmt_id == 7) {
          Ncp+=48; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_3072,prachF,prach2,1);
          // here we have |empty | Prach3072 |
          if (prach_fmt_id != 7) {
            memmove(prach2+(3072<<1),prach2,(3072<<2));
            prach_len = (3072*2)+Ncp;
          } else 	  prach_len = (3072*1)+Ncp;
	       memmove(prach,prach+(3072<<1),(Ncp<<2));
	       // here we have |Prefix | Prach3072 | Prach3072 (if ! 0xc0) |
        } else if (prach_fmt_id == 2 || prach_fmt_id == 5) { // 6x3072
          Ncp+=48; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_3072,prachF,prach2,1);
          // here we have |empty | Prach3072 |
          memmove(prach2+(3072<<1),prach2,(3072<<2));
          // here we have |empty | Prach3072 | Prach3072| empty3072 | empty3072 | empty3072 | empty3072
          memmove(prach2+(3072<<2),prach2,(3072<<3));
          // here we have |empty | Prach3072 | Prach3072| Prach3072 | Prach3072 | empty3072 | empty3072
          memmove(prach2+(3072<<3),prach2,(3072<<3));
          // here we have |empty | Prach3072 | Prach3072| Prach3072 | Prach3072 | Prach3072 | Prach3072
          memmove(prach,prach+(3072<<1),(Ncp<<2));
          // here we have |Prefix | Prach3072 |
          prach_len = (3072*6)+Ncp;
        } else if (prach_fmt_id == 1 || prach_fmt_id == 4) { // 4x3072
          Ncp+=48; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_3072,prachF,prach2,1);
          // here we have |empty | Prach3072 |
          memmove(prach2+(3072<<1),prach2,(3072<<2));
          // here we have |empty | Prach3072 | Prach3072| empty3072 | empty3072 |
          memmove(prach2+(3072<<2),prach2,(3072<<3));
          // here we have |empty | Prach3072 | Prach3072| Prach3072 | Prach3072 |
          memmove(prach,prach+(3072<<1),(Ncp<<2));
          // here we have |Prefix | Prach3072 |
          prach_len = (3072*4)+Ncp;
        } else if (prach_fmt_id == 6) { // 12x3072
          Ncp+=48; // This assumes we are transmitting starting in symbol 0 of a PRACH slot, 30 kHz, full sampling
          prach2 = prach+(Ncp<<1);
          idft(IDFT_3072,prachF,prach2,1);
          // here we have |empty | Prach3072 |
          memmove(prach2+(3072<<1),prach2,(3072<<2));
          // here we have |empty | Prach3072 | Prach3072| empty3072 | empty3072 | empty3072 | empty3072
          memmove(prach2+(3072<<2),prach2,(3072<<3));
          // here we have |empty | Prach3072 | Prach3072| Prach3072 | Prach3072 | empty3072 | empty3072
          memmove(prach2+(3072<<3),prach2,(3072<<3));
          // here we have |empty | Prach3072 | Prach3072| Prach3072 | Prach3072 | Prach3072 | Prach3072
          memmove(prach2+(3072<<1)*6,prach2,(3072<<2)*6);
          // here we have |empty | Prach3072 | Prach3072| Prach3072 | Prach3072 | Prach3072 | Prach3072 | Prach3072 | Prach3072| Prach3072 | Prach3072 | Prach3072 | Prach3072|
          memmove(prach,prach+(3072<<1),(Ncp<<2));
          // here we have |Prefix | Prach3072 | Prach3072| Prach3072 | Prach3072 | Prach3072 | Prach3072 | Prach3072 | Prach3072| Prach3072 | Prach3072 | Prach3072 | Prach3072|
          prach_len = (3072*12)+Ncp;
        }
      }
    }
  }

  #ifdef NR_PRACH_DEBUG
    LOG_I(PHY, "PRACH [UE %d] N_RB_UL %d prach_start %d, prach_len %d, rx_offset %d, hw_timing_advance %d, N_TA_offset %d\n", Mod_id,
      fp->N_RB_UL,
      prach_start,
      prach_len,
      ue->rx_offset,
      ue->hw_timing_advance,
      ue->N_TA_offset);
  #endif

  #if defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR)
    int j, overflow = prach_start + prach_len - NR_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_subframe;

    #ifdef NR_PRACH_DEBUG
      LOG_I( PHY, "PRACH [UE %d] overflow = %d\n", Mod_id, overflow);
    #endif

    // prach_start=414.730, overflow=-39470 prach_len=6600 fp->samples_per_subframe*NR_NUMBER_OF_SUBFRAMES_PER_FRAME 460.800 prach_start+prach_len 421.330 fp->samples_per_subframe 46080

    // from prach_start=414.730 to prach_start+prach_len 421.330
    for (i = prach_start, j = 0; i < min(fp->samples_per_subframe*NR_NUMBER_OF_SUBFRAMES_PER_FRAME, prach_start + prach_len); i++, j++) {
      ((int16_t*)ue->common_vars.txdata[0])[2*i] = prach[2*j];
      ((int16_t*)ue->common_vars.txdata[0])[2*i+1] = prach[2*j+1];
    }

    for (i = 0; i < overflow; i++,j++) {
      ((int16_t*)ue->common_vars.txdata[0])[2*i] = prach[2*j];
      ((int16_t*)ue->common_vars.txdata[0])[2*i+1] = prach[2*j+1];
    }
  #else // simulators
    for (i=0; i<prach_len; i++) {
      ((int16_t*)(&ue->common_vars.txdata[0][prach_start]))[2*i] = prach[2*i];
      ((int16_t*)(&ue->common_vars.txdata[0][prach_start]))[2*i+1] = prach[2*i+1];
    }
  #endif

  //printf("----------------------\n");
  //for(int ii = prach_start; ii<2*(prach_start + prach_len); ii++){
  //  printf("PRACH rx data[%d] = %d\n", ii, ue->common_vars.txdata[0][ii]);
  //}
  //printf(" \n");

  #ifdef PRACH_WRITE_OUTPUT_DEBUG
    LOG_M("prach_tx0.m", "prachtx0", prach+(Ncp<<1), prach_len-Ncp, 1, 1);
    LOG_M("Prach_txsig.m","txs",(int16_t*)(&ue->common_vars.txdata[0][prach_start]), 2*(prach_start+prach_len), 1, 1)
  #endif

  return signal_energy((int*)prach, 256);
}

