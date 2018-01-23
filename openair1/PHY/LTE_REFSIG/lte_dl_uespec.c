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

/*! \file PHY/LTE_REFSIG/lte_dl_ue_spec.c
* \brief Top-level routines for generating UE-specific Reference signals from 36-211, V11.3.0 2013-06
* \author R. Knopp X. Jiang
* \date 2015
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr xiwen.jiang@eurecom.fr
* \note
* \warning
*/

#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "PHY/defs.h"
#include "log.h"

//extern unsigned int lte_gold_table[3][20][2][14];
//#define DEBUG_DL_CELL_SPEC

int Wbar_NCP[8][4] = {{1,1,1,1},{1,-1,1,-1},{1,1,1,1},{1,-1,1,-1},{1,1,-1,-1},{-1,-1,1,1},{1,-1,-1,1},{-1,1,1,-1}};

/*
int lte_dl_ue_spec(PHY_VARS_eNB *eNB,
                   uint8_t UE_id,
                   int32_t *output,
                   short amp,
                   uint8_t Ns,
		   uint8_t lprime,
                   uint8_t p,
                   int SS_flag)
{

  int32_t qpsk[4],nqpsk[4];
  int16_t k=0,a;
  int mprime,ind,ind_dword,ind_qpsk_symb;
  unsigned nushift,kprime;
  //  LTE_eNB_DLSCH_t *dlsch = eNB->dlsch_eNB[UE_id][0];

  a = (amp*ONE_OVER_SQRT2_Q15)>>15;
  ((short *)&qpsk[0])[0] = a;
  ((short *)&qpsk[0])[1] = a;
  ((short *)&qpsk[1])[0] = -a;
  ((short *)&qpsk[1])[1] = a;
  ((short *)&qpsk[2])[0] = a;
  ((short *)&qpsk[2])[1] = -a;
  ((short *)&qpsk[3])[0] = -a;
  ((short *)&qpsk[3])[1] = -a;

  ((short *)&nqpsk[0])[0] = -a;
  ((short *)&nqpsk[0])[1] = -a;
  ((short *)&nqpsk[1])[0] = a;
  ((short *)&nqpsk[1])[1] = -a;
  ((short *)&nqpsk[2])[0] = -a;
  ((short *)&nqpsk[2])[1] = a;
  ((short *)&nqpsk[3])[0] = a;
  ((short *)&nqpsk[3])[1] = a;

  if (p==5) {
    if (SS_flag==0) {
      if (eNB->frame_parms.Ncp == NORMAL) {
      // this is 3GPP 36-211 6.10.3.2, NORMAL CP, p=5
      
      nushift =  eNB->frame_parms.Nid_cell%3;

      if(lprime==0){
        kprime=nushift;
      }else{
        kprime=(nushift+2*lprime)%4;
      }

      k = kprime+eNB->frame_parms.first_carrier_offset;
      printf("lte_dl_ue_spec:k=%d\n",k);
 
      for (mprime=0;mprime<3*nRB_PDSCH-1;mprime++) {
        ind = 3*lprime*nRB_PDSCH+mprime;
        ind_dword = ind>>4;
        ind_qpsk_symb = ind&0xf;

        output[k] = qpsk[(phy_vars_UE->lte_gold_uespec_port5_table[Ns][ind_dword]>>(2*ind_qpsk_symb))&3];
        //output[k] = 0xffffffff;

        k += 4;
        if (k >= eNB->frame_parms.ofdm_symbol_size) {
          k++;  // skip DC carrier
          k-=eNB->frame_parms.ofdm_symbol_size;
        }

       }
      }
    }
  } else if (p>=7) {
    printf("lte_dl_ue_spec:antenna ports >=7 not supported yet!\n");
  } else {
    LOG_E(PHY,"Illegal p %d UE specific pilots\n",p);
  }

  return(0);
}
*/


int lte_dl_ue_spec_rx(PHY_VARS_UE *ue,
                      int32_t *output,
                      unsigned char Ns,
                      unsigned char p, 
                      int lprime,
                      int SS_flag,
                      uint16_t nRB_PDSCH)
{
  int32_t qpsk[4],nqpsk[4],*qpsk_p,*output_p;
  int w,mprime,ind,l,ind_dword,ind_qpsk_symb,nPRB;
  short pamp;

  // Compute the correct pilot amplitude, sqrt_rho_b = Q3.13
  pamp = ONE_OVER_SQRT2_Q15;

  // This includes complex conjugate for channel estimation
  ((short *)&qpsk[0])[0] = pamp;
  ((short *)&qpsk[0])[1] = -pamp;
  ((short *)&qpsk[1])[0] = -pamp;
  ((short *)&qpsk[1])[1] = -pamp;
  ((short *)&qpsk[2])[0] = pamp;
  ((short *)&qpsk[2])[1] = pamp;
  ((short *)&qpsk[3])[0] = -pamp;
  ((short *)&qpsk[3])[1] = pamp;

  ((short *)&nqpsk[0])[0] = -pamp;
  ((short *)&nqpsk[0])[1] = pamp;
  ((short *)&nqpsk[1])[0] = pamp;
  ((short *)&nqpsk[1])[1] = pamp;
  ((short *)&nqpsk[2])[0] = -pamp;
  ((short *)&nqpsk[2])[1] = -pamp;
  ((short *)&nqpsk[3])[0] = pamp;
  ((short *)&nqpsk[3])[1] = -pamp;

  if (p>=7) {
    if (SS_flag==0) {
      if (ue->frame_parms.Ncp == NORMAL) {
        // this is 3GPP 36-211 6.10.3.2, NORMAL CP, p>=7

        output_p = output;

        ind = 3*lprime*ue->frame_parms.N_RB_DL;
        l = lprime + ((Ns&1)<<1);

        // loop over pairs of PRBs, this is the periodicity of the W_bar_NCP sequence
        // unroll the computations for the 6 pilots, select qpsk or nqpsk as function of W_bar_NCP
        for (nPRB=0; nPRB<ue->frame_parms.N_RB_DL; nPRB+=2) {

          // First pilot
          w = Wbar_NCP[p-7][l];
          qpsk_p = (w==1) ? qpsk : nqpsk;


          ind_dword     = ind>>4;
          ind_qpsk_symb = ind&0xf;

          *output_p = qpsk_p[(ue->lte_gold_uespec_table[0][Ns][lprime][ind_dword]>>(2*ind_qpsk_symb))&3];


#ifdef DEBUG_DL_UESPEC
          LOG_D(PHY,"Ns %d, l %d, m %d,ind_dword %d, ind_qpsk_symbol %d\n",
                Ns,l,m,ind_dword,ind_qpsk_symb);
          LOG_D(PHY,"index = %d\n",(eNB->lte_gold_uespec_table[0][Ns][lprime][ind_dword]>>(2*ind_qpsk_symb))&3);
#endif

          output_p++;
          ind++;

          w =  Wbar_NCP[p-7][3-l];
          qpsk_p = (w==1) ? qpsk : nqpsk;

          // Second pilot
          ind_dword     = ind>>4;
          ind_qpsk_symb = ind&0xf;

          *output_p = qpsk_p[(ue->lte_gold_uespec_table[0][Ns][lprime][ind_dword]>>(2*ind_qpsk_symb))&3];

#ifdef DEBUG_DL_UESPEC
          LOG_D(PHY,"Ns %d, l %d, m %d,ind_dword %d, ind_qpsk_symbol %d\n",
                Ns,l,m,ind_dword,ind_qpsk_symb);
          LOG_D(PHY,"index = %d\n",(ue->lte_gold_uespec_table[0][Ns][lprime][ind_dword]>>(2*ind_qpsk_symb))&3);
#endif

          output_p++;
          ind++;

          w = Wbar_NCP[p-7][l];
          qpsk_p = (w==1) ? qpsk : nqpsk;
          // Third pilot
          ind_dword     = ind>>4;
          ind_qpsk_symb = ind&0xf;

          *output_p = qpsk_p[(ue->lte_gold_uespec_table[0][Ns][lprime][ind_dword]>>(2*ind_qpsk_symb))&3];

#ifdef DEBUG_DL_UESPEC
          LOG_D(PHY,"Ns %d, l %d, m %d,ind_dword %d, ind_qpsk_symbol %d\n",
                Ns,l,m,ind_dword,ind_qpsk_symb);
          LOG_D(PHY,"index = %d\n",(eNB->lte_gold_uespec_table[0][Ns][lprime][ind_dword]>>(2*ind_qpsk_symb))&3);
#endif

          output_p++;
          ind++;

          // Fourth pilot
          w = Wbar_NCP[p-7][3-l];
          qpsk_p = (w==1) ? qpsk : nqpsk;


          ind_dword     = ind>>4;
          ind_qpsk_symb = ind&0xf;

          *output_p = qpsk_p[(ue->lte_gold_uespec_table[0][Ns][lprime][ind_dword]>>(2*ind_qpsk_symb))&3];


#ifdef DEBUG_DL_UESPEC
          LOG_D(PHY,"Ns %d, l %d, m %d,ind_dword %d, ind_qpsk_symbol %d\n",
                Ns,l,m,ind_dword,ind_qpsk_symb);
          LOG_D(PHY,"index = %d\n",(ue->lte_gold_uespec_table[0][Ns][lprime][ind_dword]>>(2*ind_qpsk_symb))&3);
#endif

          output_p++;
          ind++;

          w =  Wbar_NCP[p-7][l];
          qpsk_p = (w==1) ? qpsk : nqpsk;

          // Fifth pilot
          ind_dword     = ind>>4;
          ind_qpsk_symb = ind&0xf;

          *output_p = qpsk_p[(ue->lte_gold_uespec_table[0][Ns][lprime][ind_dword]>>(2*ind_qpsk_symb))&3];

#ifdef DEBUG_DL_UESPEC
          LOG_D(PHY,"Ns %d, l %d, m %d,ind_dword %d, ind_qpsk_symbol %d\n",
                Ns,l,m,ind_dword,ind_qpsk_symb);
          LOG_D(PHY,"index = %d\n",(ue->lte_gold_uespec_table[0][Ns][lprime][ind_dword]>>(2*ind_qpsk_symb))&3);
#endif

          output_p++;
          ind++;

          w = Wbar_NCP[p-7][3-l];
          qpsk_p = (w==1) ? qpsk : nqpsk;
          // Sixth pilot
          ind_dword     = ind>>4;
          ind_qpsk_symb = ind&0xf;

          *output_p = qpsk_p[(ue->lte_gold_uespec_table[0][Ns][lprime][ind_dword]>>(2*ind_qpsk_symb))&3];

#ifdef DEBUG_DL_UESPEC
          LOG_D(PHY,"Ns %d, l %d, m %d,ind_dword %d, ind_qpsk_symbol %d\n",
                Ns,l,m,ind_dword,ind_qpsk_symb);
          LOG_D(PHY,"index = %d\n",(ue->lte_gold_uespec_table[0][Ns][lprime][ind_dword]>>(2*ind_qpsk_symb))&3);
#endif

          output_p++;
          ind++;
        }
      } else {
        LOG_E(PHY,"Special subframe not supported for UE specific pilots yet\n");
      }
    }
  } else if (p==5) {
    if (SS_flag==0) {
      output_p = output;

      if (ue->frame_parms.Ncp == NORMAL) {
        for (mprime=0;mprime<3*nRB_PDSCH;mprime++) {

          ind = 3*lprime*nRB_PDSCH+mprime;
          ind_dword = ind>>4;
          ind_qpsk_symb = ind&0xf;

          *output_p = qpsk[(ue->lte_gold_uespec_port5_table[Ns][ind_dword]>>(2*ind_qpsk_symb))&3];
          //printf("lprime=%d,ind=%d,Ns=%d,output_p=(%d,%d)\n",lprime,ind,Ns,((short *)&output_p[0])[0],((short *)&output_p[0])[1]);
          output_p++;
      
        }
      }
    }
  } else {
    LOG_E(PHY,"Illegal p %d UE specific pilots\n",p);
  }

  return(0);
}



#ifdef LTE_DL_CELL_SPEC_MAIN



//extern int write_output(const char *,const char *,void *,int,int,char);
// flag change eren
extern int write_output(const char *,const char *,void *,int,int,char);
main()
{

  unsigned short Nid_cell=0;
  unsigned int Ncp = 0;
  int output00[1024];
  int output01[1024];
  int output10[1024];
  int output11[1024];

  memset(output00,0,1024*sizeof(int));
  memset(output01,0,1024*sizeof(int));
  memset(output10,0,1024*sizeof(int));
  memset(output11,0,1024*sizeof(int));

  lte_gold(Nid_cell,Ncp);

  lte_dl_ue_spec(output00,
                 ONE_OVER_SQRT2_Q15,
                 50,
                 Nid_cell,
                 Ncp,
                 0,
                 0,
                 0,
                 0);

  lte_dl_ue_spec(output10,
                 ONE_OVER_SQRT2_Q15,
                 50,
                 Nid_cell,
                 Ncp,
                 0,
                 1,
                 0,
                 0);

  lte_dl_ue_spec(output01,
                 ONE_OVER_SQRT2_Q15,
                 50,
                 Nid_cell,
                 Ncp,
                 0,
                 0,
                 1,
                 0);

  lte_dl_ue_spec(output11,
                 ONE_OVER_SQRT2_Q15,
                 50,
                 Nid_cell,
                 Ncp,
                 0,
                 1,
                 1,
                 0);


  write_output("dl_ue_spec00.m","dl_cs00",output00,1024,1,1);
  write_output("dl_ue_spec01.m","dl_cs01",output01,1024,1,1);
  write_output("dl_ue_spec10.m","dl_cs10",output10,1024,1,1);
  write_output("dl_ue_spec11.m","dl_cs11",output11,1024,1,1);
}


#endif
