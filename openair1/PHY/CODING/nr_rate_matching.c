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

/* file: nr_rate_matching.c
   purpose: Procedures for rate matching/interleaving for NR LDPC
   author: hongzhi.wang@tcl.com
*/

#include "PHY/defs_gNB.h"
#include "PHY/defs_nr_UE.h"

//#define RM_DEBUG 1

uint8_t index_k0[2][4] = {{0,17,33,56},{0,13,25,43}};


void nr_interleaving_ldpc(uint32_t E, uint8_t Qm, uint8_t *e,uint8_t *f)
{
  uint32_t EQm;

  EQm = E/Qm;
  memset(f,0,E*sizeof(uint8_t));

  for (int j = 0; j< EQm; j++){
	  for (int i = 0; i< Qm; i++){
		  f[(i+j*Qm)] = e[(i*EQm + j)];
	  }
  }

}


void nr_deinterleaving_ldpc(uint32_t E, uint8_t Qm, int16_t *e,int16_t *f)
{

  uint32_t EQm;

  EQm = E/Qm;

  for (int j = 0; j< EQm; j++){
	  for (int i = 0; i< Qm; i++){
		  e[(i*EQm + j)] = f[(i+j*Qm)];
	  }
  }
}


int nr_rate_matching_ldpc(uint8_t Ilbrm,
                          uint32_t Tbslbrm,
                          uint8_t BG,
                          uint16_t Z,
                          uint8_t *w,
                          uint8_t *e,
                          uint8_t C,
                          uint8_t rvidx,
                          uint32_t E)
{
  uint32_t Ncb,ind,k,Nref,N;

  if (C==0) {
    printf("nr_rate_matching: invalid parameters (C %d\n",C);
    return -1;
  }

  //Bit selection
  N = (BG==1)?(66*Z):(50*Z);

  if (Ilbrm == 0)
      Ncb = N;
  else {
      Nref = 3*Tbslbrm/(2*C); //R_LBRM = 2/3
      Ncb = min(N, Nref);
  }

  ind = (index_k0[BG-1][rvidx]*Ncb/N)*Z;

#ifdef RM_DEBUG
  printf("nr_rate_matching_ldpc: E %d, k0 %d, Ncb %d, rvidx %d\n", E, ind, Ncb, rvidx);
#endif

  k=0;

  for (; (ind<Ncb)&&(k<E); ind++) {

#ifdef RM_DEBUG
    printf("RM_TX k%d Ind: %d (%d)\n",k,ind,w[ind]);
#endif

    if (w[ind] != NR_NULL) e[k++]=w[ind];
  }

  while(k<E) {
    for (ind=0; (ind<Ncb)&&(k<E); ind++) {

#ifdef RM_DEBUG
      printf("RM_TX k%d Ind: %d (%d)\n",k,ind,w[ind]);
#endif

      if (w[ind] != NR_NULL) e[k++]=w[ind];
    }
  }


  return 0;
}

int nr_rate_matching_ldpc_rx(uint8_t Ilbrm,
                             uint32_t Tbslbrm,
                             uint8_t BG,
                             uint16_t Z,
                             int16_t *w,
                             int16_t *soft_input,
                             uint8_t C,
                             uint8_t rvidx,
                             uint8_t clear,
                             uint32_t E)
{
  uint32_t Ncb,ind,k,Nref,N;

#ifdef RM_DEBUG
  int nulled=0;
#endif

  if (C==0) {
    printf("nr_rate_matching: invalid parameters (C %d\n",C);
    return -1;
  }

  //Bit selection
  N = (BG==1)?(66*Z):(50*Z);

  if (Ilbrm == 0)
      Ncb = N;
  else {
      Nref = (3*Tbslbrm/(2*C)); //R_LBRM = 2/3
      Ncb = min(N, Nref);
  }

  ind = (index_k0[BG-1][rvidx]*Ncb/N)*Z;

#ifdef RM_DEBUG
  printf("nr_rate_matching_ldpc_rx: Clear %d, E %d, k0 %d, Ncb %d, rvidx %d\n", clear, E, ind, Ncb, rvidx);
#endif

  if (clear==1)
    memset(w,0,Ncb*sizeof(int16_t));

  k=0;

  for (; (ind<Ncb)&&(k<E); ind++) {
    if (soft_input[ind] != NR_NULL) {
      w[ind] += soft_input[k++];
#ifdef RM_DEBUG
      printf("RM_RX k%d Ind: %d (%d)\n",k-1,ind,w[ind]);
#endif
    }

#ifdef RM_DEBUG
    else {
      printf("RM_RX Ind: %d NULL %d\n",ind,nulled);
      nulled++;
    }

#endif
  }

  while(k<E) {
    for (ind=0; (ind<Ncb)&&(k<E); ind++) {
      if (soft_input[ind] != NR_NULL) {
        w[ind] += soft_input[k++];
#ifdef RM_DEBUG
        printf("RM_RX k%d Ind: %d (%d)(soft in %d)\n",k-1,ind,w[ind],soft_input[k-1]);
#endif
      }

#ifdef RM_DEBUG
      else {
        printf("RM_RX Ind: %d NULL %d\n",ind,nulled);
        nulled++;
      }

#endif
    }
  }

  return 0;
}
