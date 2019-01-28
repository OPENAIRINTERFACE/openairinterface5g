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


uint32_t nr_rate_matching_ldpc(uint8_t Ilbrm,
							   uint32_t Tbslbrm,
							   uint8_t BG,
							   uint16_t Z,
                               uint32_t G,
                               uint8_t *w,
                               uint8_t *e,
                               uint8_t C,
                               uint8_t rvidx,
                               uint8_t Qm,
                               uint8_t Nl,
                               uint8_t r)
{
  uint8_t Cprime;
  uint32_t Ncb,E,ind,k,Nref,N;
  //uint8_t *e2;

  AssertFatal(Nl>0,"Nl is 0\n");
  AssertFatal(Qm>0,"Qm is 0\n");

  //Bit selection
  N = (BG==1)?(66*Z):(50*Z);

  if (Ilbrm == 0)
	  Ncb = N;
  else {
      Nref = 3*Tbslbrm/(2*C); //R_LBRM = 2/3
      Ncb = min(N, Nref);
  }

#ifdef RM_DEBUG
  printf("nr_rate_matching: Ncb %d, rvidx %d, G %d, Qm %d, Nl%d, r %d\n",Ncb,rvidx, G, Qm,Nl,r);
#endif

  Cprime = C; //assume CBGTI not present

  if (r <= Cprime - ((G/(Nl*Qm))%Cprime) - 1)
      E = Nl*Qm*(G/(Nl*Qm*Cprime));
  else
	  E = Nl*Qm*((G/(Nl*Qm*Cprime))+1);

  ind = (index_k0[BG-1][rvidx]*Ncb/N)*Z;

#ifdef RM_DEBUG
  printf("nr_rate_matching: E %d, k0 %d Cprime %d modcprime %d\n",E,ind, Cprime,((G/(Nl*Qm))%Cprime));
#endif

  //e2 = e;

  k=0;

  for (; (ind<Ncb)&&(k<E); ind++) {

#ifdef RM_DEBUG
    printf("RM_TX k%d Ind: %d (%d)\n",k,ind,w[ind]);
#endif

    //if (w[ind] != NR_NULL) e2[k++]=w[ind];
    if (w[ind] != NR_NULL) e[k++]=w[ind];
  }

  while(k<E) {
    for (ind=0; (ind<Ncb)&&(k<E); ind++) {

#ifdef RM_DEBUG
      printf("RM_TX k%d Ind: %d (%d)\n",k,ind,w[ind]);
#endif

      //if (w[ind] != NR_NULL) e2[k++]=w[ind];
      if (w[ind] != NR_NULL) e[k++]=w[ind];
    }
  }


  return(E);
}

int nr_rate_matching_ldpc_rx(uint8_t Ilbrm,
		 	 	 	 	 	 uint32_t Tbslbrm,
							 uint8_t BG,
							 uint16_t Z,
							 uint32_t G,
                             int16_t *w,
                             int16_t *soft_input,
                             uint8_t C,
                             uint8_t rvidx,
                             uint8_t clear,
                             uint8_t Qm,
                             uint8_t Nl,
                             uint8_t r,
                             uint32_t *E_out)
{
  uint8_t Cprime;
  uint32_t Ncb,E,ind,k,Nref,N;

  int16_t *soft_input2;

#ifdef RM_DEBUG
  int nulled=0;
#endif

  if (C==0 || Qm==0 || Nl==0) {
    printf("nr_rate_matching: invalid parameters (C %d, Qm %d, Nl %d\n",C,Qm,Nl);
    return(-1);
  }

  AssertFatal(Nl>0,"Nl is 0\n");
  AssertFatal(Qm>0,"Qm is 0\n");

  //Bit selection
  N = (BG==1)?(66*Z):(50*Z);

  if (Ilbrm == 0)
	  Ncb = N;
  else {
      Nref = (3*Tbslbrm/(2*C)); //R_LBRM = 2/3
      Ncb = min(N, Nref);
  }

  Cprime = C; //assume CBGTI not present

  if (r <= Cprime - ((G/(Nl*Qm))%Cprime) - 1)
      E = Nl*Qm*(G/(Nl*Qm*Cprime));
  else
	  E = Nl*Qm*((G/(Nl*Qm*Cprime))+1);

  ind = (index_k0[BG-1][rvidx]*Ncb/N)*Z;

#ifdef RM_DEBUG
  printf("nr_rate_matching_ldpc_rx: Clear %d, E %d, Ncb %d,rvidx %d, G %d, Qm %d, Nl%d, r %d\n",clear,E,Ncb,rvidx, G, Qm,Nl,r);
#endif

  if (clear==1)
    memset(w,0,Ncb*sizeof(int16_t));

  soft_input2 = soft_input;
  k=0;

  for (; (ind<Ncb)&&(k<E); ind++) {
    if (soft_input2[ind] != NR_NULL) {
      w[ind] += soft_input2[k++];
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
      if (soft_input2[ind] != NR_NULL) {
        w[ind] += soft_input2[k++];
#ifdef RM_DEBUG
        printf("RM_RX k%d Ind: %d (%d)(soft in %d)\n",k-1,ind,w[ind],soft_input2[k-1]);
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

  *E_out = E;
  return(0);

}
