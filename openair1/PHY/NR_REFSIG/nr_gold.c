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

#include "nr_refsig.h"

void nr_init_pbch_dmrs(PHY_VARS_gNB* gNB)
{
  unsigned int x1, x2;
  uint16_t Nid, i_ssb, i_ssb2;
  unsigned char Lmax, l, n_hf, N_hf;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  uint8_t reset;

  Nid = cfg->cell_config.phy_cell_id.value;

  Lmax = fp->Lmax;
  N_hf = (Lmax == 4)? 2:1;

  for (n_hf = 0; n_hf < N_hf; n_hf++) {
    for (l = 0; l < Lmax ; l++) {
      i_ssb = l & (Lmax-1);
      i_ssb2 = i_ssb + (n_hf<<2);

      reset = 1;
      x2 = (1<<11) * (i_ssb2 + 1) * ((Nid>>2) + 1) + (1<<6) * (i_ssb2 + 1) + (Nid&3);

      for (uint8_t n=0; n<NR_PBCH_DMRS_LENGTH_DWORD; n++) {
        gNB->nr_gold_pbch_dmrs[n_hf][l][n] = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }

    }
  }

}

void nr_init_pdcch_dmrs(PHY_VARS_gNB* gNB, uint32_t Nid)
{

  uint32_t x1, x2;
  uint8_t reset;
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  uint32_t ***pdcch_dmrs = gNB->nr_gold_pdcch_dmrs;
  int pdcch_dmrs_init_length =  (((fp->N_RB_DL<<1)*3)>>5)+1;

  for (uint8_t slot=0; slot<fp->slots_per_frame; slot++) {
    for (uint8_t symb=0; symb<fp->symbols_per_slot; symb++) {

      reset = 1;
      x2 = ((1<<17) * (fp->symbols_per_slot*slot+symb+1) * ((Nid<<1)+1) + (Nid<<1));
      LOG_D(PHY,"PDCCH DMRS slot %d, symb %d, Nid %d, x2 %x\n",slot,symb,Nid,x2);
      for (uint32_t n=0; n<pdcch_dmrs_init_length; n++) {
        pdcch_dmrs[slot][symb][n] = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }
    }  
  }

}


void nr_init_pdsch_dmrs(PHY_VARS_gNB* gNB, uint8_t nscid, uint32_t Nid) {
  
  uint32_t x1, x2;
  uint8_t reset;
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  uint32_t ****pdsch_dmrs = gNB->nr_gold_pdsch_dmrs;
  int pdsch_dmrs_init_length =  ((fp->N_RB_DL*12)>>5)+1;

  for (uint8_t slot=0; slot<fp->slots_per_frame; slot++) {

    for (uint8_t symb=0; symb<fp->symbols_per_slot; symb++) {
      reset = 1;
      x2 = ((1<<17) * (fp->symbols_per_slot*slot+symb+1) * ((Nid<<1)+1) +((Nid<<1)+nscid));
      LOG_D(PHY,"PDSCH DMRS slot %d, symb %d x2 %x, Nid %d,nscid %d\n",slot,symb,x2,Nid,nscid);
      for (uint32_t n=0; n<pdsch_dmrs_init_length; n++) {
        pdsch_dmrs[slot][symb][nscid][n] = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }
    }
  }
}


void nr_gold_pusch(PHY_VARS_gNB* gNB, int nscid, uint32_t nid) {

  unsigned char ns;
  unsigned int n,x1,x2;
  int reset;
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  unsigned short l;
  int pusch_dmrs_init_length =  ((fp->N_RB_UL*12)>>5)+1;

  for (ns=0; ns<fp->slots_per_frame; ns++) {
    for (l=0; l<fp->symbols_per_slot; l++) {
      reset = 1;
      x2 = ((1<<17) * (fp->symbols_per_slot*ns+l+1) * ((nid<<1)+1) +((nid<<1)+nscid));
      LOG_D(PHY,"DMRS slot %d, symb %d x2 %x\n",ns,l,x2);

      for (n=0; n<pusch_dmrs_init_length; n++) {
        gNB->nr_gold_pusch_dmrs[nscid][ns][l][n] = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }
    }
  }
}
