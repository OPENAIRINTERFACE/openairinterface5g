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

#include "refsig_defs_ue.h"

void nr_gold_pbch(PHY_VARS_NR_UE* ue)
{
  unsigned int n = 0, x1 = 0, x2 = 0;
  unsigned int Nid, i_ssb, i_ssb2;
  unsigned char Lmax, l, n_hf, N_hf;
  uint8_t reset;

  Nid = ue->frame_parms.Nid_cell;
  Lmax = ue->frame_parms.Lmax;
  N_hf = (Lmax == 4)? 2:1;

  for (n_hf = 0; n_hf < N_hf; n_hf++) {

    for (l = 0; l < Lmax ; l++) {
      i_ssb = l & (Lmax-1);
      i_ssb2 = i_ssb + (n_hf<<2);

      reset = 1;
      x2 = (1<<11) * (i_ssb2 + 1) * ((Nid>>2) + 1) + (1<<6) * (i_ssb2 + 1) + (Nid&3);

      for (n=0; n<NR_PBCH_DMRS_LENGTH_DWORD; n++) {
        ue->nr_gold_pbch[n_hf][l][n] = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }

    }
  }

}

void nr_gold_pdcch(PHY_VARS_NR_UE* ue,
                   unsigned short nid)
{
  unsigned int n = 0, x1 = 0, x2 = 0, x2tmp0 = 0;
  uint8_t reset;
  int pdcch_dmrs_init_length = (((ue->frame_parms.N_RB_DL << 1) * 3) >> 5) + 1;

  for (int ns = 0; ns < ue->frame_parms.slots_per_frame; ns++) {
    for (int l = 0; l < ue->frame_parms.symbols_per_slot; l++) {
      reset = 1;
      x2tmp0 = ((ue->frame_parms.symbols_per_slot * ns + l + 1) * ((nid << 1) + 1));
      x2tmp0 <<= 17;
      x2 = (x2tmp0 + (nid << 1)) % (1U << 31);  //cinit
      for (n=0; n<pdcch_dmrs_init_length; n++) {
        ue->nr_gold_pdcch[0][ns][l][n] = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }    
    }
  }
}

void nr_gold_pdsch(PHY_VARS_NR_UE* ue,
                   int nscid,
                   uint32_t nid) {
  unsigned int x1 = 0, x2 = 0, x2tmp0 = 0;
  uint8_t reset;
  int pdsch_dmrs_init_length =  ((ue->frame_parms.N_RB_DL*12)>>5)+1;

  for (int ns=0; ns<ue->frame_parms.slots_per_frame; ns++) {

    for (int l=0; l<ue->frame_parms.symbols_per_slot; l++) {

      reset = 1;
      x2tmp0 = ((ue->frame_parms.symbols_per_slot*ns+l+1)*((nid<<1)+1))<<17;
      x2 = (x2tmp0+(nid<<1)+nscid)%(1U<<31);  //cinit
      LOG_D(PHY,"UE DMRS slot %d, symb %d, x2 %x, nscid %d\n",ns,l,x2,nscid);

      for (int n=0; n<pdsch_dmrs_init_length; n++) {
        ue->nr_gold_pdsch[0][ns][l][nscid][n] = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }
    }
  }
}

void nr_init_pusch_dmrs(PHY_VARS_NR_UE* ue,
                        uint16_t N_n_scid,
                        uint8_t n_scid)
{
  uint32_t x1 = 0, x2 = 0, n = 0;
  NR_DL_FRAME_PARMS *fp = &ue->frame_parms;
  uint32_t ****pusch_dmrs = ue->nr_gold_pusch_dmrs;
  int pusch_dmrs_init_length = ((fp->N_RB_UL * 12) >> 5) + 1;

  for (int slot = 0; slot < fp->slots_per_frame; slot++) {
    for (int symb = 0; symb < fp->symbols_per_slot; symb++) {
      int reset = 1;
      x2 = ((1U << 17) * (fp->symbols_per_slot*slot + symb + 1) * ((N_n_scid << 1) + 1) + ((N_n_scid << 1) + n_scid));
      LOG_D(PHY,"DMRS slot %d, symb %d x2 %x\n", slot, symb, x2);
      for (n=0; n<pusch_dmrs_init_length; n++) {
        pusch_dmrs[slot][symb][n_scid][n] = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }
    }
  }
}

void init_nr_gold_prs(PHY_VARS_NR_UE* ue)
{
  unsigned int x1 = 0, x2 = 0;
  uint16_t Nid;

  NR_DL_FRAME_PARMS *fp = &ue->frame_parms;
  uint8_t reset;
  uint8_t slotNum, symNum, gnb, rsc;
  
  for(gnb = 0; gnb < ue->prs_active_gNBs; gnb++) {
    for(rsc = 0; rsc < ue->prs_vars[gnb]->NumPRSResources; rsc++) {
      Nid = ue->prs_vars[gnb]->prs_resource[rsc].prs_cfg.NPRSID; // seed value
      LOG_I(PHY,"Initialised NR-PRS sequence with PRS_ID %3d for resource %d\n",Nid, rsc);
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
            ue->nr_gold_prs[gnb][rsc][slotNum][symNum][n] = lte_gold_generic(&x1, &x2, reset);      
            reset = 0;
            //printf("%d \n",gNB->nr_gold_prs[slotNum][symNum][n]); 
	    
          }
        }
      }
    } // for rsc
  } // for gnb
}
