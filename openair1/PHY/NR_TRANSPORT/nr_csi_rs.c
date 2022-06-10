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


#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "PHY/NR_REFSIG/nr_refsig.h"

//#define NR_CSIRS_DEBUG


void nr_init_csi_rs(NR_DL_FRAME_PARMS *fp, uint32_t ***csi_rs, uint32_t Nid) {
  uint32_t x1, x2;
  uint8_t reset;
  int csi_dmrs_init_length =  ((fp->N_RB_DL<<4)>>5)+1;
  for (uint8_t slot=0; slot<fp->slots_per_frame; slot++) {
    for (uint8_t symb=0; symb<fp->symbols_per_slot; symb++) {
      reset = 1;
      x2 = ((1<<10) * (fp->symbols_per_slot*slot+symb+1) * ((Nid<<1)+1) + (Nid));
      for (uint32_t n=0; n<csi_dmrs_init_length; n++) {
        csi_rs[slot][symb][n] = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }
    }
  }
}

void nr_generate_csi_rs(NR_DL_FRAME_PARMS frame_parms,
                        int32_t **dataF,
                        int16_t amp,
                        nr_csi_rs_info_t *nr_csi_rs_info,
                        nfapi_nr_dl_tti_csi_rs_pdu_rel15_t *csi_params,
                        int slot){

#ifdef NR_CSIRS_DEBUG
  LOG_I(NR_PHY, "csi_params->subcarrier_spacing = %i\n", csi_params->subcarrier_spacing);
  LOG_I(NR_PHY, "csi_params->cyclic_prefix = %i\n", csi_params->cyclic_prefix);
  LOG_I(NR_PHY, "csi_params->start_rb = %i\n", csi_params->start_rb);
  LOG_I(NR_PHY, "csi_params->nr_of_rbs = %i\n", csi_params->nr_of_rbs);
  LOG_I(NR_PHY, "csi_params->csi_type = %i (0:TRS, 1:CSI-RS NZP, 2:CSI-RS ZP)\n", csi_params->csi_type);
  LOG_I(NR_PHY, "csi_params->row = %i\n", csi_params->row);
  LOG_I(NR_PHY, "csi_params->freq_domain = %i\n", csi_params->freq_domain);
  LOG_I(NR_PHY, "csi_params->symb_l0 = %i\n", csi_params->symb_l0);
  LOG_I(NR_PHY, "csi_params->symb_l1 = %i\n", csi_params->symb_l1);
  LOG_I(NR_PHY, "csi_params->cdm_type = %i\n", csi_params->cdm_type);
  LOG_I(NR_PHY, "csi_params->freq_density = %i (0: dot5 (even RB), 1: dot5 (odd RB), 2: one, 3: three)\n", csi_params->freq_density);
  LOG_I(NR_PHY, "csi_params->scramb_id = %i\n", csi_params->scramb_id);
  LOG_I(NR_PHY, "csi_params->power_control_offset = %i\n", csi_params->power_control_offset);
  LOG_I(NR_PHY, "csi_params->power_control_offset_ss = %i\n", csi_params->power_control_offset_ss);
#endif

  int dataF_offset = slot*frame_parms.samples_per_slot_wCP;
  uint32_t **nr_gold_csi_rs = nr_csi_rs_info->nr_gold_csi_rs[slot];
  //*8(max allocation per RB)*2(QPSK))
  int csi_rs_length =  frame_parms.N_RB_DL<<4;
  int16_t mod_csi[frame_parms.symbols_per_slot][csi_rs_length>>1] __attribute__((aligned(16)));
  uint16_t b = csi_params->freq_domain;
  uint16_t n, p, k, l, mprime, na, kpn;
  uint8_t size, ports, kprime, lprime, i, gs;
  uint8_t j[16], k_n[6], koverline[16], loverline[16];
  int found = 0;
  int wf, wt, lp, kp, symb;
  uint8_t fi = 0;
  double rho, alpha;
  uint32_t beta = amp;
  nr_csi_rs_info->csi_rs_generated_signal_bits = log2_approx(amp);

  AssertFatal(b!=0, "Invalid CSI frequency domain mapping: no bit selected in bitmap\n");

  // if the scrambling id is not the one previously used to initialize we need to re-initialize the rs
  if (csi_params->scramb_id != nr_csi_rs_info->csi_gold_init) {
    nr_csi_rs_info->csi_gold_init = csi_params->scramb_id;
    nr_init_csi_rs(&frame_parms, nr_csi_rs_info->nr_gold_csi_rs, csi_params->scramb_id);
  }

  switch (csi_params->row) {
  // implementation of table 7.4.1.5.3-1 of 38.211
  // lprime and kprime are the max value of l' and k'
  case 1:
    ports = 1;
    kprime = 0;
    lprime = 0;
    size = 3;
    while (found < 1) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi;
        found++;
      }
      else
        fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = 0;
      loverline[i] = csi_params->symb_l0;
      koverline[i] = k_n[0] + (i<<2);
    }
    break;

  case 2:
    ports = 1;
    kprime = 0;
    lprime = 0;
    size = 1;
    while (found < 1) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi;
        found++;
      }
      else
        fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = 0;
      loverline[i] = csi_params->symb_l0;
      koverline[i] = k_n[0];
    }
    break;

  case 3:
    ports = 2;
    kprime = 1;
    lprime = 0;
    size = 1;
    while (found < 1) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      else
        fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = 0;
      loverline[i] = csi_params->symb_l0;
      koverline[i] = k_n[0];
    }
    break;

  case 4:
    ports = 4;
    kprime = 1;
    lprime = 0;
    size = 2;
    while (found < 1) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<2;
        found++;
      }
      else
        fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params->symb_l0;
      koverline[i] = k_n[0] + (i<<1);
    }
    break;

  case 5:
    ports = 4;
    kprime = 1;
    lprime = 0;
    size = 2;
    while (found < 1) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      else
        fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params->symb_l0 + i;
      koverline[i] = k_n[0];
    }
    break;

  case 6:
    ports = 8;
    kprime = 1;
    lprime = 0;
    size = 4;
    while (found < 4) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params->symb_l0;
      koverline[i] = k_n[i];
    }
    break;

  case 7:
    ports = 8;
    kprime = 1;
    lprime = 0;
    size = 4;
    while (found < 2) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params->symb_l0 + (i>>1);
      koverline[i] = k_n[i%2];
    }
    break;

  case 8:
    ports = 8;
    kprime = 1;
    lprime = 1;
    size = 2;
    while (found < 2) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params->symb_l0;
      koverline[i] = k_n[i];
    }
    break;

  case 9:
    ports = 12;
    kprime = 1;
    lprime = 0;
    size = 6;
    while (found < 6) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params->symb_l0;
      koverline[i] = k_n[i];
    }
    break;

  case 10:
    ports = 12;
    kprime = 1;
    lprime = 1;
    size = 3;
    while (found < 3) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params->symb_l0;
      koverline[i] = k_n[i];
    }
    break;

  case 11:
    ports = 16;
    kprime = 1;
    lprime = 0;
    size = 8;
    while (found < 4) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params->symb_l0 + (i>>2);
      koverline[i] = k_n[i%4];
    }
    break;

  case 12:
    ports = 16;
    kprime = 1;
    lprime = 1;
    size = 4;
    while (found < 4) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params->symb_l0;
      koverline[i] = k_n[i];
    }
    break;

  case 13:
    ports = 24;
    kprime = 1;
    lprime = 0;
    size = 12;
    while (found < 3) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      if (i<6)
        loverline[i] = csi_params->symb_l0 + i/3;
      else
        loverline[i] = csi_params->symb_l1 + i/9;
      koverline[i] = k_n[i%3];
    }
    break;

  case 14:
    ports = 24;
    kprime = 1;
    lprime = 1;
    size = 6;
    while (found < 3) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      if (i<3)
        loverline[i] = csi_params->symb_l0;
      else
        loverline[i] = csi_params->symb_l1;
      koverline[i] = k_n[i%3];
    }
    break;

  case 15:
    ports = 24;
    kprime = 1;
    lprime = 3;
    size = 3;
    while (found < 3) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params->symb_l0;
      koverline[i] = k_n[i];
    }
    break;

  case 16:
    ports = 32;
    kprime = 1;
    lprime = 0;
    size = 16;
    while (found < 4) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      if (i<8)
        loverline[i] = csi_params->symb_l0 + (i>>2);
      else
        loverline[i] = csi_params->symb_l1 + (i/12);
      koverline[i] = k_n[i%4];
    }
    break;

  case 17:
    ports = 32;
    kprime = 1;
    lprime = 1;
    size = 8;
    while (found < 4) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      if (i<4)
        loverline[i] = csi_params->symb_l0;
      else
        loverline[i] = csi_params->symb_l1;
      koverline[i] = k_n[i%4];
    }
    break;

  case 18:
    ports = 32;
    kprime = 1;
    lprime = 3;
    size = 4;
    while (found < 4) {
      if ((b >> fi) & 0x01) {
        k_n[found] = fi<<1;
        found++;
      }
      fi++;
    }
    for (i=0; i<size; i++) {
      j[i] = i;
      loverline[i] = csi_params->symb_l0;
      koverline[i] = k_n[i];
    }
    break;

  default:
    AssertFatal(0==1, "Row %d is not valid for CSI Table 7.4.1.5.3-1\n", csi_params->row);
  }

#ifdef NR_CSIRS_DEBUG
  printf(" row %d, n. of ports %d\n k' ",csi_params->row,ports);
  for (kp=0; kp<=kprime; kp++)
    printf("%d, ",kp);
  printf("l' ");
  for (lp=0; lp<=lprime; lp++)
    printf("%d, ",lp);
  printf("\n k overline ");
  for (i=0; i<size; i++)
    printf("%d, ",koverline[i]);
  printf("\n l overline ");
  for (i=0; i<size; i++)
    printf("%d, ",loverline[i]);
  printf("\n");
#endif


  // setting the frequency density from its index
  switch (csi_params->freq_density) {
  
  case 0:
    rho = 0.5;
    break;
  
  case 1:
    rho = 0.5;
    break;

   case 2:
    rho = 1;
    break;

   case 3:
    rho = 3;
    break;

  default:
    AssertFatal(0==1, "Invalid frequency density index for CSI\n");
  }

  if (ports == 1)
    alpha = rho;
  else
    alpha = 2*rho; 

#ifdef NR_CSIRS_DEBUG
    printf(" rho %f, alpha %f\n",rho,alpha);
#endif

  // CDM group size from CDM type index
  switch (csi_params->cdm_type) {
  
  case 0:
    gs = 1;
    break;
  
  case 1:
    gs = 2;
    break;

  case 2:
    gs = 4;
    break;

  case 3:
    gs = 8;
    break;

  default:
    AssertFatal(0==1, "Invalid cdm type index for CSI\n");
  }

  uint16_t csi_length;
  if (rho < 1) {
    if (csi_params->freq_density == 0) {
      csi_length = (((csi_params->start_rb + csi_params->nr_of_rbs)>>1)<<kprime)<<1;
    } else {
      csi_length = ((((csi_params->start_rb + csi_params->nr_of_rbs)>>1)<<kprime)+1)<<1;
    }
  } else {
    csi_length = (((uint16_t) rho*(csi_params->start_rb + csi_params->nr_of_rbs))<<kprime)<<1;
  }

#ifdef NR_CSIRS_DEBUG
    printf(" start rb %d, nr of rbs %d, csi length %d\n", csi_params->start_rb, csi_params->nr_of_rbs, csi_length);
#endif


  // TRS
  if (csi_params->csi_type == 0) {
    // ???
  }

  // NZP CSI RS
  if (csi_params->csi_type == 1) {
    // assuming amp is the amplitude of SSB channels
    switch (csi_params->power_control_offset_ss) {
    case 0:
      beta = (amp*ONE_OVER_SQRT2_Q15)>>15;
      break;
    case 1:
      beta = amp;
      break;
    case 2:
      beta = (amp*ONE_OVER_SQRT2_Q15)>>14;
      break;
    case 3:
      beta = amp<<1;
      break;
    default:
      AssertFatal(0==1, "Invalid SS power offset density index for CSI\n");
    }

    for (lp=0; lp<=lprime; lp++){
      symb = csi_params->symb_l0;
      nr_modulation(nr_gold_csi_rs[symb+lp], csi_length, DMRS_MOD_ORDER, mod_csi[symb+lp]);
      if ((csi_params->row == 5) || (csi_params->row == 7) || (csi_params->row == 11) || (csi_params->row == 13) || (csi_params->row == 16))
        nr_modulation(nr_gold_csi_rs[symb+1], csi_length, DMRS_MOD_ORDER, mod_csi[symb+1]);
      if ((csi_params->row == 14) || (csi_params->row == 13) || (csi_params->row == 16) || (csi_params->row == 17)) {
        symb = csi_params->symb_l1;
        nr_modulation(nr_gold_csi_rs[symb+lp], csi_length, DMRS_MOD_ORDER, mod_csi[symb+lp]);
        if ((csi_params->row == 13) || (csi_params->row == 16))
          nr_modulation(nr_gold_csi_rs[symb+1], csi_length, DMRS_MOD_ORDER, mod_csi[symb+1]);
      }
    }
  }

  uint16_t start_sc = frame_parms.first_carrier_offset;

  // resource mapping according to 38.211 7.4.1.5.3
  for (n=csi_params->start_rb; n<(csi_params->start_rb+csi_params->nr_of_rbs); n++) {
   if ( (csi_params->freq_density > 1) || (csi_params->freq_density == (n%2))) {  // for freq density 0.5 checks if even or odd RB
    for (int ji=0; ji<size; ji++) { // loop over CDM groups
      for (int s=0 ; s<gs; s++)  { // loop over each CDM group size
        p = s+j[ji]*gs; // port index
        for (kp=0; kp<=kprime; kp++) { // loop over frequency resource elements within a group
          k = (start_sc+(n*NR_NB_SC_PER_RB)+koverline[ji]+kp)%(frame_parms.ofdm_symbol_size);  // frequency index of current resource element
          // wf according to tables 7.4.5.3-2 to 7.4.5.3-5
          if (kp == 0)
            wf = 1;
          else
            wf = -2*(s%2)+1;
          na = n*alpha;
          kpn = (rho*koverline[ji])/NR_NB_SC_PER_RB;
          mprime = na + kp + kpn; // sequence index
          for (lp=0; lp<=lprime; lp++) { // loop over frequency resource elements within a group
            l = lp + loverline[ji];
            // wt according to tables 7.4.5.3-2 to 7.4.5.3-5 
            if (s < 2)
              wt = 1;
            else if (s < 4)
              wt = -2*(lp%2)+1;
            else if (s < 6)
              wt = -2*(lp/2)+1;
            else {
              if ((lp == 0) || (lp == 3))
                wt = 1;
              else
                wt = -1;
            }

            // ZP CSI RS
            if (csi_params->csi_type == 2) {
              ((int16_t*)dataF[p])[((l*frame_parms.ofdm_symbol_size + k)<<1)+(2*dataF_offset)] = 0;
              ((int16_t*)dataF[p])[((l*frame_parms.ofdm_symbol_size + k)<<1)+1+(2*dataF_offset)] = 0;
            }
            else {
              ((int16_t*)dataF[p])[((l*frame_parms.ofdm_symbol_size + k)<<1)+(2*dataF_offset)] = (beta*wt*wf*mod_csi[l][mprime<<1]) >> 15;
              ((int16_t*)dataF[p])[((l*frame_parms.ofdm_symbol_size + k)<<1)+1+(2*dataF_offset)] = (beta*wt*wf*mod_csi[l][(mprime<<1) + 1]) >> 15;
            }
#ifdef NR_CSIRS_DEBUG
            printf("l,k (%d,%d)  seq. index %d \t port %d \t (%d,%d)\n",l,k,mprime,p+3000,
                   ((int16_t*)dataF[p])[((l*frame_parms.ofdm_symbol_size + k)<<1)+(2*dataF_offset)],
                   ((int16_t*)dataF[p])[((l*frame_parms.ofdm_symbol_size + k)<<1)+1+(2*dataF_offset)]);
#endif
          }
        }
      }
    }
   }
  }
  nr_csi_rs_info->N_cdm_groups = size;
  nr_csi_rs_info->CDM_group_size = gs;
  nr_csi_rs_info->kprime = kprime;
  nr_csi_rs_info->lprime = lprime;
  nr_csi_rs_info->N_ports = ports;
  memcpy(nr_csi_rs_info->j,j,16*sizeof(uint8_t));
  memcpy(nr_csi_rs_info->koverline,koverline,16*sizeof(uint8_t));
  memcpy(nr_csi_rs_info->loverline,loverline,16*sizeof(uint8_t));

#ifdef NR_CSIRS_DEBUG
  LOG_I(NR_PHY, "nr_csi_rs_info->N_ports = %d\n", nr_csi_rs_info->N_ports);
  LOG_I(NR_PHY, "nr_csi_rs_info->N_cdm_groups = %d\n", nr_csi_rs_info->N_cdm_groups);
  LOG_I(NR_PHY, "nr_csi_rs_info->CDM_group_size = %d\n", nr_csi_rs_info->CDM_group_size);
  LOG_I(NR_PHY, "nr_csi_rs_info->kprime = %d\n", nr_csi_rs_info->kprime);
  LOG_I(NR_PHY, "nr_csi_rs_info->lprime = %d\n", nr_csi_rs_info->lprime);
  for(int ji=0; ji<nr_csi_rs_info->N_cdm_groups; ji++) {
    LOG_I(NR_PHY, "(CDM group %d) j = %d, koverline = %d, loverline = %d\n", ji, nr_csi_rs_info->j[ji], nr_csi_rs_info->koverline[ji], nr_csi_rs_info->loverline[ji]);
  }
#endif
}
