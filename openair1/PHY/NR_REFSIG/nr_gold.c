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

  for (uint8_t slot=0; slot<fp->slots_per_frame; slot++) {
    for (uint8_t symb=0; symb<fp->symbols_per_slot; symb++) {

      reset = 1;
      x2 = ((1<<17) * (fp->symbols_per_slot*slot+symb+1) * ((Nid<<1)+1) + (Nid<<1));

      for (uint32_t n=0; n<NR_MAX_PDCCH_DMRS_INIT_LENGTH_DWORD; n++) {
        pdcch_dmrs[slot][symb][n] = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }
    }  
  }

}


void nr_init_pdsch_dmrs(PHY_VARS_gNB* gNB, uint32_t Nid)
{
  
  uint32_t x1, x2;
  uint8_t reset, q;
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  uint32_t ****pdsch_dmrs = gNB->nr_gold_pdsch_dmrs;

  uint16_t N_n_scid[NR_MAX_NB_CODEWORDS]={Nid, Nid}; // Not correct, appropriate scrambling IDs have to be updated to support DCI 1_1
  uint8_t n_scid=0; // again works only for 1_0
  for (uint8_t slot=0; slot<fp->slots_per_frame; slot++) {

    for (uint8_t symb=0; symb<fp->symbols_per_slot; symb++) {
        reset = 1;
        x2 = ((1<<17) * (fp->symbols_per_slot*slot+symb+1) * ((N_n_scid[n_scid]<<1)+1) +((N_n_scid[n_scid]<<1)+n_scid));
	LOG_D(PHY,"DMRS slot %d, symb %d x2 %x\n",slot,symb,x2);
        for (uint32_t n=0; n<NR_MAX_PDSCH_DMRS_INIT_LENGTH_DWORD; n++) {
          pdsch_dmrs[slot][symb][0][n] = lte_gold_generic(&x1, &x2, reset);
          reset = 0;
        }

        for (q = 1; q < NR_MAX_NB_CODEWORDS; q++)
          memcpy(pdsch_dmrs[slot][symb][q],pdsch_dmrs[slot][symb][0],sizeof(uint32_t)*NR_MAX_PDSCH_DMRS_INIT_LENGTH_DWORD);
    }
  }
}


void nr_gold_pusch(PHY_VARS_gNB* gNB, uint32_t *Nid) {

  unsigned char ns;
  unsigned int n,x1,x2;
  int nscid, reset;
  unsigned int nid;
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  unsigned short l;

  for (nscid=0; nscid<2; nscid++) {
    nid = Nid[nscid];
    for (ns=0; ns<fp->slots_per_frame; ns++) {
      for (l=0; l<fp->symbols_per_slot; l++) {
        reset = 1;
        x2 = ((1<<17) * (fp->symbols_per_slot*ns+l+1) * ((nid<<1)+1) +((nid<<1)+nscid));
        LOG_D(PHY,"DMRS slot %d, symb %d x2 %x\n",ns,l,x2);

        for (n=0; n<NR_MAX_PUSCH_DMRS_INIT_LENGTH_DWORD; n++) {
          gNB->nr_gold_pusch_dmrs[nscid][ns][l][n] = lte_gold_generic(&x1, &x2, reset);
          reset = 0;
        }
      }
    }
  }
}


void nr_init_csi_rs(PHY_VARS_gNB* gNB, uint32_t Nid)
{
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  uint32_t ***csi_rs = gNB->nr_gold_csi_rs;
  uint32_t x1, x2;
  uint8_t reset;

  for (uint8_t slot=0; slot<fp->slots_per_frame; slot++) {
    for (uint8_t symb=0; symb<fp->symbols_per_slot; symb++) {

      reset = 1;
      x2 = ((1<<10) * (fp->symbols_per_slot*slot+symb+1) * ((Nid<<1)+1) + (Nid));

      for (uint32_t n=0; n<NR_MAX_CSI_RS_INIT_LENGTH_DWORD; n++) {
        csi_rs[slot][symb][n] = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }
    }  
  }

}

void nr_init_prs(PHY_VARS_gNB* gNB, uint32_t symNum)
{
  unsigned int x1, x2;
  uint16_t Nid, i_ssb, i_ssb2;
void nr_init_prs(PHY_VARS_gNB* gNB)
{
  unsigned int x1, x2;
  uint16_t Nid;
  //unsigned char Lmax, l, n_hf, N_hf;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  uint8_t reset;
  uint8_t slotNum, symNum;

  Nid = cfg->cell_config.phy_cell_id.value;

  //Lmax = fp->Lmax;
  //N_hf = (Lmax == 4)? 2:1;

  for (slotNum = 0; slotNum < fp->slots_per_frame-1; slotNum++) {
    for (symNum = 0; symNum < fp->symbols_per_slot-1 ; symNum++) {
      i_ssb = sumNum & (symbols_per_slot-1);
      i_ssb2 = i_ssb + (slot<<2);

  for (slotNum = 0; slotNum < fp->slots_per_frame; slotNum++) {
    for (symNum = 0; symNum < fp->symbols_per_slot ; symNum++) {
      reset = 1;
	printf("%d \n",slotNum);
	printf("%d \n",symNum);
      // initial x2 for prs as 38.211
      uint32_t c_init1, c_init2, c_init3;
      uint32_t pow22=1<<22;
      uint32_t pow10=1<<10;
      c_init1 = pow22*ceil(Nid/1024);
      c_init2 = pow10*(slotnum+symNum+1)*(2*(Nid%1024)+1);
      c_init3 = Nid%1024;
      x2 = c_init1 + c_init2 + c_init3;


      c_init2 = pow10*(slotNum+symNum+1)*(2*(Nid%1024)+1);
      c_init3 = Nid%1024;
      x2 = c_init1 + c_init2 + c_init3;
	printf("%d \n",x2);
      for (uint8_t n=0; n<NR_MAX_PRS_INIT_LENGTH_DWORD; n++) {
        gNB->nr_gold_prs[slotNum][symNum][n] = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }
    }
  }

}
