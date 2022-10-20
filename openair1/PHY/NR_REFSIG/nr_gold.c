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
  unsigned int x1 = 0, x2 = 0;
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
  uint32_t x1 = 0, x2 = 0;
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
  uint32_t x1 = 0, x2 = 0;
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
  unsigned int n = 0, x1 = 0, x2 = 0;
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


void nr_init_prs(PHY_VARS_gNB* gNB)
{
  unsigned int x1 = 0, x2 = 0;
  uint16_t Nid;

  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  gNB->nr_gold_prs = (uint32_t ****)malloc16(gNB->prs_vars.NumPRSResources*sizeof(uint32_t ***));
  uint32_t ****prs = gNB->nr_gold_prs;
  AssertFatal(prs!=NULL, "NR init: positioning reference signal malloc failed\n");
  for (int rsc=0; rsc < gNB->prs_vars.NumPRSResources; rsc++) {
    prs[rsc] = (uint32_t ***)malloc16(fp->slots_per_frame*sizeof(uint32_t **));
    AssertFatal(prs[rsc]!=NULL, "NR init: positioning reference signal for rsc %d - malloc failed\n", rsc);

    for (int slot=0; slot<fp->slots_per_frame; slot++) {
      prs[rsc][slot] = (uint32_t **)malloc16(fp->symbols_per_slot*sizeof(uint32_t *));
      AssertFatal(prs[rsc][slot]!=NULL, "NR init: positioning reference signal for slot %d - malloc failed\n", slot);

      for (int symb=0; symb<fp->symbols_per_slot; symb++) {
        prs[rsc][slot][symb] = (uint32_t *)malloc16(NR_MAX_PRS_INIT_LENGTH_DWORD*sizeof(uint32_t));
        AssertFatal(prs[rsc][slot][symb]!=NULL, "NR init: positioning reference signal for rsc %d slot %d symbol %d - malloc failed\n", rsc, slot, symb);
      }
    }
  }

  uint8_t reset;
  uint8_t slotNum, symNum, rsc_id;

  for (rsc_id = 0; rsc_id < gNB->prs_vars.NumPRSResources; rsc_id++) {
    Nid = gNB->prs_vars.prs_cfg[rsc_id].NPRSID; // seed value
    LOG_I(PHY, "Initiaized NR-PRS sequence with PRS_ID %3d for resource %d\n", Nid, rsc_id);
    for (slotNum = 0; slotNum < fp->slots_per_frame; slotNum++) {
      for (symNum = 0; symNum < fp->symbols_per_slot ; symNum++) {
        reset = 1;
        // initial x2 for prs as ts138.211
        uint32_t c_init1, c_init2, c_init3;
        uint32_t pow22=1<<22;
        uint32_t pow10=1<<10;
        c_init1 = pow22*ceil(Nid/1024);
        c_init2 = pow10*(slotNum+symNum+1)*(2*(Nid%1024)+1);
        c_init3 = Nid%1024;
        x2 = c_init1 + c_init2 + c_init3;

        for (uint8_t n=0; n<NR_MAX_PRS_INIT_LENGTH_DWORD; n++) {
          gNB->nr_gold_prs[rsc_id][slotNum][symNum][n] = lte_gold_generic(&x1, &x2, reset);      
          reset = 0;
          //printf("%d \n",gNB->nr_gold_prs[slotNum][symNum][n]); 
        }
      }
    }
  }
}
