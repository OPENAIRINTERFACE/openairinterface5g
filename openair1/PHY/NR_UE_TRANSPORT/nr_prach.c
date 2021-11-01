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
extern const char *prachfmt[];

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
  uint16_t preamble_shift = 0, preamble_index0, n_shift_ra, n_shift_ra_bar, d_start=INT16_MAX, numshift, N_ZC, u, offset, offset2, first_nonzero_root_idx;
  int16_t prach_tmp[98304*2*4] __attribute__((aligned(32)));

  int16_t Ncp = 0, amp, *prach, *prach2, *prachF, *Xu;
  int32_t Xu_re, Xu_im;
  int prach_start, prach_sequence_length, i, prach_len, dftlen, mu, kbar, K, n_ra_prb, k, prachStartSymbol, sample_offset_slot;
  //int restricted_Type;

  fd_occasion             = 0;
  prach_len               = 0;
  dftlen                  = 0;
  first_nonzero_root_idx  = 0;
  prach                   = prach_tmp;
  prachF                  = ue->prach_vars[gNB_id]->prachF;
  amp                     = ue->prach_vars[gNB_id]->amp;
  Mod_id                  = ue->Mod_id;
  prach_sequence_length   = nrUE_config->prach_config.prach_sequence_length;
  N_ZC                    = (prach_sequence_length == 0) ? 839:139;
  mu                      = nrUE_config->prach_config.prach_sub_c_spacing;
  restricted_set          = prach_pdu->restricted_set;
  rootSequenceIndex       = prach_pdu->root_seq_id;
  n_ra_prb                = nrUE_config->prach_config.num_prach_fd_occasions_list[fd_occasion].k1,//prach_pdu->freq_msg1;
  NCS                     = prach_pdu->num_cs;
  prach_fmt_id            = prach_pdu->prach_format;
  preamble_index          = prach_resources->ra_PreambleIndex;
  kbar                    = 1;
  K                       = 24;
  k                       = 12*n_ra_prb - 6*fp->N_RB_UL;
  prachStartSymbol        = prach_pdu->prach_start_symbol;
  //restricted_Type         = 0;

  compute_nr_prach_seq(nrUE_config->prach_config.prach_sequence_length,
                       nrUE_config->prach_config.num_prach_fd_occasions_list[fd_occasion].num_root_sequences,
                       nrUE_config->prach_config.num_prach_fd_occasions_list[fd_occasion].prach_root_sequence_index,
                       ue->X_u);

  if (slot % (fp->slots_per_subframe / 2) == 0)
    sample_offset_slot = (prachStartSymbol==0?0:fp->ofdm_symbol_size*prachStartSymbol+fp->nb_prefix_samples0+fp->nb_prefix_samples*(prachStartSymbol-1));
  else
    sample_offset_slot = (fp->ofdm_symbol_size + fp->nb_prefix_samples) * prachStartSymbol;

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
      LOG_I(PHY, "PRACH [UE %d] generate PRACH in slot %d for RootSeqIndex %d, Preamble Index %d, PRACH Format %s, NCS %d (N_ZC %d): Preamble_offset %d, Preamble_shift %d msg1 frequency start %d\n",
        Mod_id,
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

  LOG_I(PHY, "PRACH [UE %d] in slot %d, mu %d, fp->samples_per_subframe %d, prach_sequence_length %d, placing PRACH in position %d, msg1 frequency start %d (k1 %d), preamble_offset %d, first_nonzero_root_idx %d\n", Mod_id,
        slot,
        mu,
        fp->samples_per_subframe,
        prach_sequence_length,
        k,
        n_ra_prb,
        nrUE_config->prach_config.num_prach_fd_occasions_list[fd_occasion].k1,
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

  if (mu==1) {
    switch(fp->samples_per_subframe) {
    case 15360:
      // 10, 15 MHz @ 15.36 Ms/s
      if (prach_sequence_length == 0) {
        if (prach_fmt_id == 0 || prach_fmt_id == 1 || prach_fmt_id == 2)
          dftlen = 12288;
        if (prach_fmt_id == 3)
          dftlen = 3072;
      } else { // 839 sequence
        dftlen = 512;
      }
      break;

    case 30720:
      // 20, 25, 30 MHz @ 30.72 Ms/s
      if (prach_sequence_length == 0) {
        if (prach_fmt_id == 0 || prach_fmt_id == 1 || prach_fmt_id == 2)
          dftlen = 24576;
        if (prach_fmt_id == 3)
          dftlen = 6144;
      } else { // 839 sequence
        dftlen = 1024;
      }
      break;

    case 46080:
      // 40 MHz @ 46.08 Ms/s
      if (prach_sequence_length == 0) {
        if (prach_fmt_id == 0 || prach_fmt_id == 1 || prach_fmt_id == 2)
          dftlen = 36864;
        if (prach_fmt_id == 3)
          dftlen = 9216;
      } else { // 839 sequence
        dftlen = 1536;
      }
      break;

    case 61440:
      // 40, 50, 60 MHz @ 61.44 Ms/s
      if (prach_sequence_length == 0) {
        if (prach_fmt_id == 0 || prach_fmt_id == 1 || prach_fmt_id == 2)
          dftlen = 49152;
        if (prach_fmt_id == 3)
          dftlen = 12288;
      } else { // 839 sequence
        dftlen = 2048;
      }
      break;

    case 92160:
      // 50, 60, 70, 80, 90 MHz @ 92.16 Ms/s
      if (prach_sequence_length == 0) {
        if (prach_fmt_id == 0 || prach_fmt_id == 1 || prach_fmt_id == 2)
          dftlen = 73728;
        if (prach_fmt_id == 3)
          dftlen = 18432;
      } else { // 839 sequence
        dftlen = 3072;
      }
      break;

    case 122880:
      // 70, 80, 90, 100 MHz @ 122.88 Ms/s
      if (prach_sequence_length == 0) {
        if (prach_fmt_id == 0 || prach_fmt_id == 1 || prach_fmt_id == 2)
          dftlen = 98304;
        if (prach_fmt_id == 3)
          dftlen = 24576;
      } else { // 839 sequence
        dftlen = 4096;
      }
      break;

    default:
      AssertFatal(1==0,"sample rate %f MHz not supported for numerology %d\n", fp->samples_per_subframe / 1000.0, mu);
    }
  }
  else if (mu==3) {
    if (fp->threequarter_fs) 
      AssertFatal(1==0,"3/4 sampling not supported for numerology %d\n",mu);
    
    if (prach_sequence_length == 0) 
	AssertFatal(1==0,"long prach not supported for numerology %d\n",mu);

    if (fp->N_RB_UL == 32) 
      dftlen=512;
    else if (fp->N_RB_UL == 66) 
      dftlen=1024;
    else 
      AssertFatal(1==0,"N_RB_UL %d not support for numerology %d\n",fp->N_RB_UL,mu);
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

    // Ncp here is given in terms of T_s wich is 30.72MHz sampling
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
    case 4: //A1
      Ncp = 288/(1<<mu);
      break;
    case 5: //A2
      Ncp = 576/(1<<mu);
      break;
    case 6: //A3
      Ncp = 864/(1<<mu);
      break;
    case 7: //B1
      Ncp = 216/(1<<mu);
    break;
    /*
    case 4: //B2
      Ncp = 360/(1<<mu);
      break;
    case 5: //B3
      Ncp = 504/(1<<mu);
      break;
    */
    case 8: //B4
      Ncp = 936/(1<<mu);
      break;
    case 9: //C0
      Ncp = 1240/(1<<mu);
      break;
    case 10: //C2
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

  //actually what we should be checking here is how often the current prach crosses a 0.5ms boundary. I am not quite sure for which paramter set this would be the case, so I will ignore it for now and just check if the prach starts on a 0.5ms boundary
  uint8_t  use_extended_prach_prefix = 0;
  if(fp->numerology_index == 0) {
    if (prachStartSymbol == 0 || prachStartSymbol == 7)
      use_extended_prach_prefix = 1;
  }
  else {
    if (slot%(fp->slots_per_subframe/2)==0 && prachStartSymbol == 0)
      use_extended_prach_prefix = 1;
  }

  if (mu == 3) {
    switch (fp->samples_per_subframe) {
    case 61440: // 32 PRB case, 61.44 Msps
      Ncp<<=1; //to account for 61.44Mbps
      // This is after cyclic prefix 
      prach2 = prach+(Ncp<<1); //times 2 for complex samples
      if (prach_sequence_length == 0)
	AssertFatal(1==0,"no long PRACH for this PRACH size %d\n",fp->N_RB_UL);
      else {
	if (use_extended_prach_prefix) 
          Ncp+=32;  // 16*kappa, kappa=2 for 61.44Msps
	prach2 = prach+(Ncp<<1); //times 2 for complex samples
        if (prach_fmt_id == 4 || prach_fmt_id == 7 || prach_fmt_id == 9) {
          idft(IDFT_512,prachF,prach2,1);
          // here we have |empty | Prach512 |
          if (prach_fmt_id != 9) {
            memmove(prach2+(512<<1),prach2,(512<<2));
            prach_len = (512*2)+Ncp;
          }
          else prach_len = (512*1)+Ncp;
          memmove(prach,prach+(512<<1),(Ncp<<2));
          // here we have |Prefix | Prach512 | Prach512 (if ! 0xc0)  |
        } else if (prach_fmt_id == 5) { // 6x512
          idft(IDFT_512,prachF,prach2,1);
          // here we have |empty | Prach512 |
          memmove(prach2+(512<<1),prach2,(512<<2));
          // here we have |empty | Prach512 | Prach512| empty512 | empty512 |
          memmove(prach2+(512<<2),prach2,(512<<3));
          // here we have |empty | Prach512 | Prach512| Prach512 | Prach512 |
          memmove(prach,prach+(512<<1),(Ncp<<2));
          // here we have |Prefix | Prach512 |
          prach_len = (512*4)+Ncp;
        } else if (prach_fmt_id == 6) { // 6x512
          idft(IDFT_512,prachF,prach2,1);
          // here we have |empty | Prach512 |
          memmove(prach2+(512<<1),prach2,(512<<2));
          // here we have |empty | Prach512 | Prach512| empty512 | empty512 | empty512 | empty512
          memmove(prach2+(512<<2),prach2,(512<<3));
          // here we have |empty | Prach512 | Prach512| Prach512 | Prach512 | empty512 | empty512
          memmove(prach2+(512<<3),prach2,(512<<3));
          // here we have |empty | Prach512 | Prach512| Prach512 | Prach512 | Prach512 | Prach512
          memmove(prach,prach+(512<<1),(Ncp<<2));
          // here we have |Prefix | Prach512 |
          prach_len = (512*6)+Ncp;
        } else if (prach_fmt_id == 8) { // 12x512
          idft(IDFT_512,prachF,prach2,1);
          // here we have |empty | Prach512 |
          memmove(prach2+(512<<1),prach2,(512<<2));
          // here we have |empty | Prach512 | Prach512| empty512 | empty512 | empty512 | empty512
          memmove(prach2+(512<<2),prach2,(512<<3));
          // here we have |empty | Prach512 | Prach512| Prach512 | Prach512 | empty512 | empty512
          memmove(prach2+(512<<3),prach2,(512<<3));
          // here we have |empty | Prach512 | Prach512| Prach512 | Prach512 | Prach512 | Prach512
          memmove(prach2+(512<<1)*6,prach2,(512<<2)*6);
          // here we have |empty | Prach512 | Prach512| Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512| Prach512 | Prach512 | Prach512 | Prach512|
          memmove(prach,prach+(512<<1),(Ncp<<2));
          // here we have |Prefix | Prach512 | Prach512| Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512| Prach512 | Prach512 | Prach512 | Prach512|
          prach_len = (512*12)+Ncp;
	}		
      }
      break;

    case 122880: // 66 PRB case, 122.88 Msps
      Ncp<<=2; //to account for 122.88Mbps
      // This is after cyclic prefix 
      prach2 = prach+(Ncp<<1); //times 2 for complex samples
      if (prach_sequence_length == 0)
	AssertFatal(1==0,"no long PRACH for this PRACH size %d\n",fp->N_RB_UL);
      else {
	if (use_extended_prach_prefix) 
          Ncp+=64;  // 16*kappa, kappa=4 for 122.88Msps
	prach2 = prach+(Ncp<<1); //times 2 for complex samples
        if (prach_fmt_id == 4 || prach_fmt_id == 7 || prach_fmt_id == 9) {
          idft(IDFT_1024,prachF,prach2,1);
          // here we have |empty | Prach1024 |
          if (prach_fmt_id != 9) {
            memmove(prach2+(1024<<1),prach2,(1024<<2));
            prach_len = (1024*2)+Ncp;
          }
          else prach_len = (1024*1)+Ncp;
          memmove(prach,prach+(1024<<1),(Ncp<<2));
          // here we have |Prefix | Prach1024 | Prach1024 (if ! 0xc0)  |
        } else if (prach_fmt_id == 5) { // 6x1024
          idft(IDFT_1024,prachF,prach2,1);
          // here we have |empty | Prach1024 |
          memmove(prach2+(1024<<1),prach2,(1024<<2));
          // here we have |empty | Prach1024 | Prach1024| empty1024 | empty1024 |
          memmove(prach2+(1024<<2),prach2,(1024<<3));
          // here we have |empty | Prach1024 | Prach1024| Prach1024 | Prach1024 |
          memmove(prach,prach+(1024<<1),(Ncp<<2));
          // here we have |Prefix | Prach1024 |
          prach_len = (1024*4)+Ncp;
        } else if (prach_fmt_id == 6) { // 6x1024
          idft(IDFT_1024,prachF,prach2,1);
          // here we have |empty | Prach1024 |
          memmove(prach2+(1024<<1),prach2,(1024<<2));
          // here we have |empty | Prach1024 | Prach1024| empty1024 | empty1024 | empty1024 | empty1024
          memmove(prach2+(1024<<2),prach2,(1024<<3));
          // here we have |empty | Prach1024 | Prach1024| Prach1024 | Prach1024 | empty1024 | empty1024
          memmove(prach2+(1024<<3),prach2,(1024<<3));
          // here we have |empty | Prach1024 | Prach1024| Prach1024 | Prach1024 | Prach1024 | Prach1024
          memmove(prach,prach+(1024<<1),(Ncp<<2));
          // here we have |Prefix | Prach1024 |
          prach_len = (1024*6)+Ncp;
        } else if (prach_fmt_id == 8) { // 12x1024
          idft(IDFT_1024,prachF,prach2,1);
          // here we have |empty | Prach1024 |
          memmove(prach2+(1024<<1),prach2,(1024<<2));
          // here we have |empty | Prach1024 | Prach1024| empty1024 | empty1024 | empty1024 | empty1024
          memmove(prach2+(1024<<2),prach2,(1024<<3));
          // here we have |empty | Prach1024 | Prach1024| Prach1024 | Prach1024 | empty1024 | empty1024
          memmove(prach2+(1024<<3),prach2,(1024<<3));
          // here we have |empty | Prach1024 | Prach1024| Prach1024 | Prach1024 | Prach1024 | Prach1024
          memmove(prach2+(1024<<1)*6,prach2,(1024<<2)*6);
          // here we have |empty | Prach1024 | Prach1024| Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024| Prach1024 | Prach1024 | Prach1024 | Prach1024|
          memmove(prach,prach+(1024<<1),(Ncp<<2));
          // here we have |Prefix | Prach1024 | Prach1024| Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024| Prach1024 | Prach1024 | Prach1024 | Prach1024|
          prach_len = (1024*12)+Ncp;
	}	
      }
      break;

    default:
      AssertFatal(1==0,"sample rate %f MHz not supported for numerology %d\n", fp->samples_per_subframe / 1000.0, mu);
    }
  } else if (mu == 1) {
    switch (fp->samples_per_subframe) {
    case 15360: // full sampling @ 15.36 Ms/s
      Ncp = Ncp/2; // to account for 15.36 Ms/s
      // This is after cyclic prefix
      prach2 = prach+(2*Ncp); // times 2 for complex samples
      if (prach_sequence_length == 0){
        if (prach_fmt_id == 0) { // 24576 samples @ 30.72 Ms/s, 12288 samples @ 15.36 Ms/s
          idft(IDFT_12288,prachF,prach2,1);
          // here we have | empty  | Prach12288 |
          memmove(prach,prach+(12288<<1),(Ncp<<2));
          // here we have | Prefix | Prach12288 |
          prach_len = 12288+Ncp;
        } else if (prach_fmt_id == 1) { // 24576 samples @ 30.72 Ms/s, 12288 samples @ 15.36 Ms/s
          idft(IDFT_12288,prachF,prach2,1);
          // here we have | empty  | Prach12288 | empty12288 |
          memmove(prach2+(12288<<1),prach2,(12288<<2));
          // here we have | empty  | Prach12288 | Prach12288 |
          memmove(prach,prach+(12288<<2),(Ncp<<2));
          // here we have | Prefix | Prach12288 | Prach12288 |
          prach_len = (12288*2)+Ncp;
        } else if (prach_fmt_id == 2) { // 24576 samples @ 30.72 Ms/s, 12288 samples @ 15.36 Ms/s
          idft(IDFT_12288,prachF,prach2,1);
          // here we have | empty  | Prach12288 | empty12288 | empty12288 | empty12288 |
          memmove(prach2+(12288<<1),prach2,(12288<<2));
          // here we have | empty  | Prach12288 | Prach12288 | empty12288 | empty12288 |
          memmove(prach2+(12288<<2),prach2,(12288<<3));
          // here we have | empty  | Prach12288 | Prach12288 | Prach12288 | Prach12288 |
          memmove(prach,prach+(12288<<3),(Ncp<<2));
          // here we have | Prefix | Prach12288 | Prach12288 | Prach12288 | Prach12288 |
          prach_len = (12288*4)+Ncp;
        } else if (prach_fmt_id == 3) { // 6144 samples @ 30.72 Ms/s, 3072 samples @ 15.36 Ms/s
          idft(IDFT_3072,prachF,prach2,1);
          // here we have | empty  | Prach3072 | empty3072 | empty3072 | empty3072 |
          memmove(prach2+(3072<<1),prach2,(3072<<2));
          // here we have | empty  | Prach3072 | Prach3072 | empty3072 | empty3072 |
          memmove(prach2+(3072<<2),prach2,(3072<<3));
          // here we have | empty  | Prach3072 | Prach3072 | Prach3072 | Prach3072 |
          memmove(prach,prach+(3072<<3),(Ncp<<2));
          // here we have | Prefix | Prach3072 | Prach3072 | Prach3072 | Prach3072 |
          prach_len = (3072*4)+Ncp;
        }
      } else { // short PRACH sequence
	if (use_extended_prach_prefix)
	  Ncp += 8; // 16*kappa, kappa=0.5 for 15.36 Ms/s
	prach2 = prach+(2*Ncp); // times 2 for complex samples
        if (prach_fmt_id == 9) {
          idft(IDFT_512,prachF,prach2,1);
          // here we have | empty  | Prach512 |
          memmove(prach,prach+(512<<1),(Ncp<<2));
          // here we have | Prefix | Prach512 |
          prach_len = (512*1)+Ncp;
        } else if (prach_fmt_id == 4 || prach_fmt_id == 7) {
          idft(IDFT_512,prachF,prach2,1);
          // here we have | empty  | Prach512 | empty512 |
          memmove(prach2+(512<<1),prach2,(512<<2));
          // here we have | empty  | Prach512 | Prach512 |
          memmove(prach,prach+(512<<1),(Ncp<<2));
          // here we have | Prefix | Prach512 | Prach512 |
          prach_len = (512*2)+Ncp;
        } else if (prach_fmt_id == 5) { // 4x512
          idft(IDFT_512,prachF,prach2,1);
          // here we have | empty  | Prach512 | empty512 | empty512 | empty512 |
          memmove(prach2+(512<<1),prach2,(512<<2));
          // here we have | empty  | Prach512 | Prach512 | empty512 | empty512 |
          memmove(prach2+(512<<2),prach2,(512<<3));
          // here we have | empty  | Prach512 | Prach512 | Prach512 | Prach512 |
          memmove(prach,prach+(512<<1),(Ncp<<2));
          // here we have | Prefix | Prach512 | Prach512 | Prach512 | Prach512 |
          prach_len = (512*4)+Ncp;
        } else if (prach_fmt_id == 6) { // 6x512
          idft(IDFT_512,prachF,prach2,1);
          // here we have | empty  | Prach512 | empty512 | empty512 | empty512 | empty512 | empty512 |
          memmove(prach2+(512<<1),prach2,(512<<2));
          // here we have | empty  | Prach512 | Prach512 | empty512 | empty512 | empty512 | empty512 |
          memmove(prach2+(512<<2),prach2,(512<<3));
          // here we have | empty  | Prach512 | Prach512 | Prach512 | Prach512 | empty512 | empty512 |
          memmove(prach2+(512<<3),prach2,(512<<3));
          // here we have | empty  | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 |
          memmove(prach,prach+(512<<1),(Ncp<<2));
          // here we have | Prefix | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 |
          prach_len = (512*6)+Ncp;
        } else if (prach_fmt_id == 8) { // 12x512
          idft(IDFT_512,prachF,prach2,1);
          // here we have | empty  | Prach512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 |
          memmove(prach2+(512<<1),prach2,(512<<2));
          // here we have | empty  | Prach512 | Prach512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 |
          memmove(prach2+(512<<2),prach2,(512<<3));
          // here we have | empty  | Prach512 | Prach512 | Prach512 | Prach512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 |
          memmove(prach2+(512<<3),prach2,(512<<3));
          // here we have | empty  | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | empty512 | empty512 | empty512 | empty512 | empty512 | empty512 |
          memmove(prach2+(512<<1)*6,prach2,(512<<2)*6);
          // here we have | empty  | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 |
          memmove(prach,prach+(512<<1),(Ncp<<2));
          // here we have | Prefix | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 | Prach512 |
          prach_len = (512*12)+Ncp;
        }
      }
      break;

    case 30720: // full sampling @ 30.72 Ms/s
      Ncp = Ncp*1; // to account for 30.72 Ms/s
      // This is after cyclic prefix
      prach2 = prach+(2*Ncp); // times 2 for complex samples
      if (prach_sequence_length == 0){
        if (prach_fmt_id == 0) { // 24576 samples @ 30.72 Ms/s
          idft(IDFT_24576,prachF,prach2,1);
          // here we have | empty  | Prach24576 |
          memmove(prach,prach+(24576<<1),(Ncp<<2));
          // here we have | Prefix | Prach24576 |
          prach_len = 24576+Ncp;
        } else if (prach_fmt_id == 1) { // 24576 samples @ 30.72 Ms/s
          idft(IDFT_24576,prachF,prach2,1);
          // here we have | empty  | Prach24576 | empty24576 |
          memmove(prach2+(24576<<1),prach2,(24576<<2));
          // here we have | empty  | Prach24576 | Prach24576 |
          memmove(prach,prach+(24576<<2),(Ncp<<2));
          // here we have | Prefix | Prach24576 | Prach24576 |
          prach_len = (24576*2)+Ncp;
        } else if (prach_fmt_id == 2) { // 24576 samples @ 30.72 Ms/s
          idft(IDFT_24576,prachF,prach2,1);
          // here we have | empty  | Prach24576 | empty24576 | empty24576 | empty24576 |
          memmove(prach2+(24576<<1),prach2,(24576<<2));
          // here we have | empty  | Prach24576 | Prach24576 | empty24576 | empty24576 |
          memmove(prach2+(24576<<2),prach2,(24576<<3));
          // here we have | empty  | Prach24576 | Prach24576 | Prach24576 | Prach24576 |
          memmove(prach,prach+(24576<<3),(Ncp<<2));
          // here we have | Prefix | Prach24576 | Prach24576 | Prach24576 | Prach24576 |
          prach_len = (24576*4)+Ncp;
        } else if (prach_fmt_id == 3) { // 6144 samples @ 30.72 Ms/s
          idft(IDFT_6144,prachF,prach2,1);
          // here we have | empty  | Prach6144 | empty6144 | empty6144 | empty6144 |
          memmove(prach2+(6144<<1),prach2,(6144<<2));
          // here we have | empty  | Prach6144 | Prach6144 | empty6144 | empty6144 |
          memmove(prach2+(6144<<2),prach2,(6144<<3));
          // here we have | empty  | Prach6144 | Prach6144 | Prach6144 | Prach6144 |
          memmove(prach,prach+(6144<<3),(Ncp<<2));
          // here we have | Prefix | Prach6144 | Prach6144 | Prach6144 | Prach6144 |
          prach_len = (6144*4)+Ncp;
        }
      } else { // short PRACH sequence
	if (use_extended_prach_prefix)
	  Ncp += 16; // 16*kappa, kappa=1 for 30.72Msps
	prach2 = prach+(2*Ncp); // times 2 for complex samples
        if (prach_fmt_id == 9) {
          idft(IDFT_1024,prachF,prach2,1);
          // here we have | empty  | Prach1024 |
          memmove(prach,prach+(1024<<1),(Ncp<<2));
          // here we have | Prefix | Prach1024 |
          prach_len = (1024*1)+Ncp;
        } else if (prach_fmt_id == 4 || prach_fmt_id == 7) {
          idft(IDFT_1024,prachF,prach2,1);
          // here we have | empty  | Prach1024 | empty1024 |
          memmove(prach2+(1024<<1),prach2,(1024<<2));
          // here we have | empty  | Prach1024 | Prach1024 |
          memmove(prach,prach+(1024<<1),(Ncp<<2));
          // here we have | Prefix | Prach1024 | Prach1024 |
          prach_len = (1024*2)+Ncp;
        } else if (prach_fmt_id == 5) { // 4x1024
          idft(IDFT_1024,prachF,prach2,1);
          // here we have | empty  | Prach1024 | empty1024 | empty1024 | empty1024 |
          memmove(prach2+(1024<<1),prach2,(1024<<2));
          // here we have | empty  | Prach1024 | Prach1024 | empty1024 | empty1024 |
          memmove(prach2+(1024<<2),prach2,(1024<<3));
          // here we have | empty  | Prach1024 | Prach1024 | Prach1024 | Prach1024 |
          memmove(prach,prach+(1024<<1),(Ncp<<2));
          // here we have | Prefix | Prach1024 | Prach1024 | Prach1024 | Prach1024 |
          prach_len = (1024*4)+Ncp;
        } else if (prach_fmt_id == 6) { // 6x1024
          idft(IDFT_1024,prachF,prach2,1);
          // here we have | empty  | Prach1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 |
          memmove(prach2+(1024<<1),prach2,(1024<<2));
          // here we have | empty  | Prach1024 | Prach1024 | empty1024 | empty1024 | empty1024 | empty1024 |
          memmove(prach2+(1024<<2),prach2,(1024<<3));
          // here we have | empty  | Prach1024 | Prach1024 | Prach1024 | Prach1024 | empty1024 | empty1024 |
          memmove(prach2+(1024<<3),prach2,(1024<<3));
          // here we have | empty  | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 |
          memmove(prach,prach+(1024<<1),(Ncp<<2));
          // here we have | Prefix | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 |
          prach_len = (1024*6)+Ncp;
        } else if (prach_fmt_id == 8) { // 12x1024
          idft(IDFT_1024,prachF,prach2,1);
          // here we have | empty  | Prach1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 |
          memmove(prach2+(1024<<1),prach2,(1024<<2));
          // here we have | empty  | Prach1024 | Prach1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 |
          memmove(prach2+(1024<<2),prach2,(1024<<3));
          // here we have | empty  | Prach1024 | Prach1024 | Prach1024 | Prach1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 |
          memmove(prach2+(1024<<3),prach2,(1024<<3));
          // here we have | empty  | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 | empty1024 |
          memmove(prach2+(1024<<1)*6,prach2,(1024<<2)*6);
          // here we have | empty  | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 |
          memmove(prach,prach+(1024<<1),(Ncp<<2));
          // here we have | Prefix | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 | Prach1024 |
          prach_len = (1024*12)+Ncp;
        }
      }
      break;

    case 61440: // full sampling @ 61.44 Ms/s
      Ncp = Ncp*2; // to account for 61.44 Ms/s
      // This is after cyclic prefix 
      prach2 = prach+(Ncp<<1); //times 2 for complex samples
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
        } else if (prach_fmt_id == 3) { // 6144 samples @ 30.72 Ms/s, 12288 samples @ 61.44 Ms/s
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
	if (use_extended_prach_prefix) 
	  Ncp+=32; // 16*kappa, kappa=2 for 61.44Msps 
	prach2 = prach+(Ncp<<1); //times 2 for complex samples
        if (prach_fmt_id == 4 || prach_fmt_id == 7 || prach_fmt_id == 9) {
          idft(IDFT_2048,prachF,prach2,1);
          // here we have |empty | Prach2048 |
          if (prach_fmt_id != 9) {
            memmove(prach2+(2048<<1),prach2,(2048<<2));
            prach_len = (2048*2)+Ncp;
          }
          else prach_len = (2048*1)+Ncp;
          memmove(prach,prach+(2048<<1),(Ncp<<2));
          // here we have |Prefix | Prach2048 | Prach2048 (if ! 0xc0)  |
        } else if (prach_fmt_id == 5) { // 6x2048
          idft(IDFT_2048,prachF,prach2,1);
          // here we have |empty | Prach2048 |
          memmove(prach2+(2048<<1),prach2,(2048<<2));
          // here we have |empty | Prach2048 | Prach2048| empty2048 | empty2048 |
          memmove(prach2+(2048<<2),prach2,(2048<<3));
          // here we have |empty | Prach2048 | Prach2048| Prach2048 | Prach2048 |
          memmove(prach,prach+(2048<<1),(Ncp<<2));
          // here we have |Prefix | Prach2048 |
          prach_len = (2048*4)+Ncp;
        } else if (prach_fmt_id == 6) { // 6x2048
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
        } else if (prach_fmt_id == 8) { // 12x2048
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
      break;

    case 46080: // threequarter sampling @ 46.08 Ms/s
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
	if (use_extended_prach_prefix) 
	  Ncp+=24; // 16*kappa, kappa=1.5 for 46.08Msps 
	prach2 = prach+(Ncp<<1); //times 2 for complex samples
        if (prach_fmt_id == 4 || prach_fmt_id == 7 || prach_fmt_id == 9) {
          idft(IDFT_1536,prachF,prach2,1);
          // here we have |empty | Prach1536 |
          if (prach_fmt_id != 9) {
            memmove(prach2+(1536<<1),prach2,(1536<<2));
            prach_len = (1536*2)+Ncp;
          }	else prach_len = (1536*1)+Ncp;

          memmove(prach,prach+(1536<<1),(Ncp<<2));
          // here we have |Prefix | Prach1536 | Prach1536 (if ! 0xc0) |

        } else if (prach_fmt_id == 5) { // 6x1536
          idft(IDFT_1536,prachF,prach2,1);
          // here we have |empty | Prach1536 |
          memmove(prach2+(1536<<1),prach2,(1536<<2));
          // here we have |empty | Prach1536 | Prach1536| empty1536 | empty1536 |
          memmove(prach2+(1536<<2),prach2,(1536<<3));
          // here we have |empty | Prach1536 | Prach1536| Prach1536 | Prach1536 |
          memmove(prach,prach+(1536<<1),(Ncp<<2));
          // here we have |Prefix | Prach1536 |
          prach_len = (1536*4)+Ncp;
        } else if (prach_fmt_id == 6) { // 6x1536
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
        } else if (prach_fmt_id == 8) { // 12x1536
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
      break;

    case 122880: // full sampling @ 122.88 Ms/s
      Ncp<<=2; //to account for 122.88Mbps
      // This is after cyclic prefix
      prach2 = prach+(Ncp<<1); //times 2 for complex samples
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
	if (use_extended_prach_prefix) 
          Ncp+=64; // 16*kappa, kappa=4 for 122.88Msps
	prach2 = prach+(Ncp<<1); //times 2 for complex samples
        if (prach_fmt_id == 4 || prach_fmt_id == 7 || prach_fmt_id == 9) {
          idft(IDFT_4096,prachF,prach2,1);
          // here we have |empty | Prach4096 |
          if (prach_fmt_id != 9) {
            memmove(prach2+(4096<<1),prach2,(4096<<2));
            prach_len = (4096*2)+Ncp; 
          }	else 	prach_len = (4096*1)+Ncp;
          memmove(prach,prach+(4096<<1),(Ncp<<2));
          // here we have |Prefix | Prach4096 | Prach4096 (if ! 0xc0) |
        } else if (prach_fmt_id == 5) { // 4x4096
          idft(IDFT_4096,prachF,prach2,1);
          // here we have |empty | Prach4096 |
          memmove(prach2+(4096<<1),prach2,(4096<<2));
          // here we have |empty | Prach4096 | Prach4096| empty4096 | empty4096 |
          memmove(prach2+(4096<<2),prach2,(4096<<3));
          // here we have |empty | Prach4096 | Prach4096| Prach4096 | Prach4096 |
          memmove(prach,prach+(4096<<1),(Ncp<<2));
          // here we have |Prefix | Prach4096 |
          prach_len = (4096*4)+Ncp;
        } else if (prach_fmt_id == 6) { // 6x4096
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
        } else if (prach_fmt_id == 8) { // 12x4096
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
      break;

    case 92160: // three quarter sampling @ 92.16 Ms/s
      Ncp = (Ncp*3); //to account for 92.16 Msps
      prach2 = prach+(Ncp<<1); //times 2 for complex samples
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
	if (use_extended_prach_prefix) 
          Ncp+=48; // 16*kappa, kappa=3 for 92.16Msps 
	prach2 = prach+(Ncp<<1); //times 2 for complex samples
	if (prach_fmt_id == 4 || prach_fmt_id == 7 || prach_fmt_id == 9) {
          idft(IDFT_3072,prachF,prach2,1);
          // here we have |empty | Prach3072 |
          if (prach_fmt_id != 9) {
            memmove(prach2+(3072<<1),prach2,(3072<<2));
            prach_len = (3072*2)+Ncp;
          } else 	  prach_len = (3072*1)+Ncp;
	  memmove(prach,prach+(3072<<1),(Ncp<<2));
	  // here we have |Prefix | Prach3072 | Prach3072 (if ! 0xc0) |
        } else if (prach_fmt_id == 6) { // 6x3072
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
        } else if (prach_fmt_id == 5) { // 4x3072
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
      break;

    default:
      AssertFatal(1==0,"sample rate %f MHz not supported for numerology %d\n", fp->samples_per_subframe / 1000.0, mu);
    }
  }

  #ifdef NR_PRACH_DEBUG
    LOG_I(PHY, "PRACH [UE %d] N_RB_UL %d prach_start %d, prach_len %d\n", Mod_id,
      fp->N_RB_UL,
      prach_start,
      prach_len);
  #endif

  for (i=0; i<prach_len; i++) {
    ((int16_t*)(&ue->common_vars.txdata[0][prach_start]))[2*i] = prach[2*i];
    ((int16_t*)(&ue->common_vars.txdata[0][prach_start]))[2*i+1] = prach[2*i+1];
  }

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

