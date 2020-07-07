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
  unsigned int n, x1, x2;
  unsigned int Nid, i_ssb, i_ssb2;
  unsigned char Lmax, l, n_hf, N_hf;

  Nid = ue->frame_parms.Nid_cell;
  Lmax = ue->frame_parms.Lmax;
  N_hf = (Lmax == 4)? 2:1;

  for (n_hf = 0; n_hf < N_hf; n_hf++) {

    for (l = 0; l < Lmax ; l++) {
      i_ssb = l & (Lmax-1);
      i_ssb2 = i_ssb + (n_hf<<2);

      x1 = 1 + (1<<31);
      x2 = (1<<11) * (i_ssb2 + 1) * ((Nid>>2) + 1) + (1<<6) * (i_ssb2 + 1) + (Nid&3);
      x2 = x2 ^ ((x2 ^ (x2>>1) ^ (x2>>2) ^ (x2>>3))<<31);

      // skip first 50 double words (1600 bits)
      for (n = 1; n < 50; n++) {
        x1 = (x1>>1) ^ (x1>>4);
        x1 = x1 ^ (x1<<31) ^ (x1<<28);
        x2 = (x2>>1) ^ (x2>>2) ^ (x2>>3) ^ (x2>>4);
        x2 = x2 ^ (x2<<31) ^ (x2<<30) ^ (x2<<29) ^ (x2<<28);
      }

      for (n=0; n<NR_PBCH_DMRS_LENGTH_DWORD; n++) {
        x1 = (x1>>1) ^ (x1>>4);
        x1 = x1 ^ (x1<<31) ^ (x1<<28);
        x2 = (x2>>1) ^ (x2>>2) ^ (x2>>3) ^ (x2>>4);
        x2 = x2 ^ (x2<<31) ^ (x2<<30) ^ (x2<<29) ^ (x2<<28);
        ue->nr_gold_pbch[n_hf][l][n] = x1 ^ x2;
      }

    }
  }

}

void nr_gold_pdcch(PHY_VARS_NR_UE* ue,
                   unsigned short n_idDMRS,
                   unsigned short length_dmrs)
{
  unsigned char ns,l;
  unsigned int n,x1,x2,x2tmp0;
  unsigned int nid;

    if (n_idDMRS)
      nid = n_idDMRS;
    else
      nid = ue->frame_parms.Nid_cell;

    for (ns=0; ns<20; ns++) {

      for (l=0; l<length_dmrs; l++) {

    	x2tmp0 = ((14*ns+l+1)*((nid<<1)+1))<<17;
        x2 = (x2tmp0+(nid<<1))%(1<<31);  //cinit

        x1 = 1+ (1<<31);
        x2=x2 ^ ((x2 ^ (x2>>1) ^ (x2>>2) ^ (x2>>3))<<31);

        // skip first 50 double words (1600 bits)
        for (n=1; n<50; n++) {
          x1 = (x1>>1) ^ (x1>>4);
          x1 = x1 ^ (x1<<31) ^ (x1<<28);
          x2 = (x2>>1) ^ (x2>>2) ^ (x2>>3) ^ (x2>>4);
          x2 = x2 ^ (x2<<31) ^ (x2<<30) ^ (x2<<29) ^ (x2<<28);
            //printf("x1 : %x, x2 : %x\n",x1,x2);
        }

        for (n=0; n<52; n++) {
          x1 = (x1>>1) ^ (x1>>4);
          x1 = x1 ^ (x1<<31) ^ (x1<<28);
          x2 = (x2>>1) ^ (x2>>2) ^ (x2>>3) ^ (x2>>4);
          x2 = x2 ^ (x2<<31) ^ (x2<<30) ^ (x2<<29) ^ (x2<<28);
          ue->nr_gold_pdcch[0][ns][l][n] = x1^x2;
	  // if (ns==1 && l==0) printf("n=%d : c %x\n",n,x1^x2);
        }
      }
    }
}

void nr_gold_pdsch(PHY_VARS_NR_UE* ue,
                   unsigned short lbar,
                   unsigned short *n_idDMRS,
                   unsigned short length_dmrs)
{
  unsigned char ns,l;
  unsigned int n,x1,x2,x2tmp0;
  int nscid;
  unsigned int nid;

  /// to be updated from higher layer
  //unsigned short lbar = 0;

  for (nscid=0; nscid<2; nscid++) {
    if (n_idDMRS)
      nid = n_idDMRS[nscid];
    else
      nid = ue->frame_parms.Nid_cell;
      
      //printf("gold pdsch nid %d lbar %d\n",nid,lbar);

    for (ns=0; ns<20; ns++) {

      for (l=0; l<length_dmrs; l++) {

    	x2tmp0 = ((14*ns+(lbar+l)+1)*((nid<<1)+1))<<17;
        x2 = (x2tmp0+(nid<<1)+nscid)%(1<<31);  //cinit
	LOG_D(PHY,"UE DMRS slot %d, symb %d, lbar %d, x2 %x, nscid %d\n",ns,l,lbar,x2,nscid);
                //printf("ns %d gold pdsch x2 %d\n",ns,x2);

        x1 = 1+ (1<<31);
        x2=x2 ^ ((x2 ^ (x2>>1) ^ (x2>>2) ^ (x2>>3))<<31);

        // skip first 50 double words (1600 bits)
        for (n=1; n<50; n++) {
          x1 = (x1>>1) ^ (x1>>4);
          x1 = x1 ^ (x1<<31) ^ (x1<<28);
          x2 = (x2>>1) ^ (x2>>2) ^ (x2>>3) ^ (x2>>4);
          x2 = x2 ^ (x2<<31) ^ (x2<<30) ^ (x2<<29) ^ (x2<<28);
            //printf("x1 : %x, x2 : %x\n",x1,x2);
        }

        for (n=0; n<NR_MAX_PDSCH_DMRS_INIT_LENGTH_DWORD; n++) {
          x1 = (x1>>1) ^ (x1>>4);
          x1 = x1 ^ (x1<<31) ^ (x1<<28);
          x2 = (x2>>1) ^ (x2>>2) ^ (x2>>3) ^ (x2>>4);
          x2 = x2 ^ (x2<<31) ^ (x2<<30) ^ (x2<<29) ^ (x2<<28);
          ue->nr_gold_pdsch[nscid][ns][l][n] = x1^x2;
            // if ((ns==2)&&(l==0))
            //printf("n=%d : c %x\n",n,x1^x2);
        }

      }
    }
  }
}

void nr_init_pusch_dmrs(PHY_VARS_NR_UE* ue,
                        uint16_t *N_n_scid,
                        uint8_t n_scid)
{
  uint32_t x1, x2, n;
  uint8_t reset, slot, symb, q;
  NR_DL_FRAME_PARMS *fp = &ue->frame_parms;
  uint32_t ****pusch_dmrs = ue->nr_gold_pusch_dmrs;

  for (slot=0; slot<fp->slots_per_frame; slot++) {

    for (symb=0; symb<fp->symbols_per_slot; symb++) {

      reset = 1;
      x2 = ((1<<17) * (fp->symbols_per_slot*slot+symb+1) * ((N_n_scid[n_scid]<<1)+1) +((N_n_scid[n_scid]<<1)+n_scid));

      for (n=0; n<NR_MAX_PUSCH_DMRS_INIT_LENGTH_DWORD; n++) {
        pusch_dmrs[slot][symb][0][n] = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }

      for (q = 1; q < NR_MAX_NB_CODEWORDS; q++)
        memcpy(pusch_dmrs[slot][symb][q],pusch_dmrs[slot][symb][0],sizeof(uint32_t)*NR_MAX_PUSCH_DMRS_INIT_LENGTH_DWORD);
    }
  }
}
