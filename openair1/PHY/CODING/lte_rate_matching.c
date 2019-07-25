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

/* file: lte_rate_matching.c
   purpose: Procedures for rate matching/interleaving for LTE (turbo-coded transport channels) (TX/RX)
   author: raymond.knopp@eurecom.fr
   date: 21.10.2009
*/
#ifdef MAIN
  #include <stdio.h>
  #include <stdlib.h>
#endif
#include "PHY/defs_eNB.h"
#include "PHY/LTE_TRANSPORT/transport_common.h"

//#define cmin(a,b) ((a)<(b) ? (a) : (b))

static uint32_t bitrev[32]    = {0,16,8,24,4,20,12,28,2,18,10,26,6,22,14,30,1,17,9,25,5,21,13,29,3,19,11,27,7,23,15,31};
static uint32_t bitrev_x3[32] = {0,48,24,72,12,60,36,84,6,54,30,78,18,66,42,90,3,51,27,75,15,63,39,87,9,57,33,81,21,69,45,93};
static uint32_t bitrev_cc[32] = {1,17,9,25,5,21,13,29,3,19,11,27,7,23,15,31,0,16,8,24,4,20,12,28,2,18,10,26,6,22,14,30};

//#define RM_DEBUG_TX 1
//#define RM_DEBUG 1
//#define RM_DEBUG2 1
//#define RM_DEBUG_CC 1

uint32_t sub_block_interleaving_turbo(uint32_t D, uint8_t *d,uint8_t *w) {
  uint32_t RTC = (D>>5), ND, ND3;
  uint32_t row,col,Kpi;
  uint32_t index3,k,k2;
#ifdef RM_DEBUG
  uint32_t nulled=0;
#endif
  uint8_t *d1,*d2,*d3;

  if ((D&0x1f) > 0)
    RTC++;

  Kpi = (RTC<<5);
  //  Kpi3 = Kpi*3;
  ND = Kpi - D;
#ifdef RM_DEBUG
  printf("sub_block_interleaving_turbo : D = %d (%d)\n",D,D*3);
  printf("RTC = %d, Kpi=%d, ND=%d\n",RTC,Kpi,ND);
#endif
  ND3 = ND*3;
  // copy d02 to dD2 (for mod Kpi operation from clause (4), p.16 of 36.212
  d[(3*D)+2] = d[2];
  k=0;
  k2=0;
  d1 = d-ND3;
  d2 = d1+1;
  d3 = d1+5;

  for (col=0; col<32; col++) {
#ifdef RM_DEBUG
    printf("Col %d\n",col);
#endif
    index3 = bitrev_x3[col];//3*index;

    for (row=0; row<RTC; row++) {
      w[k]            =  d1[index3];//d[index3-ND3];
      w[Kpi+k2]       =  d2[index3];//d[index3-ND3+1];
      w[Kpi+1+k2]     =  d3[index3];//d[index3-ND3+5];
#ifdef RM_DEBUG
      printf("row %d, index %d, index-Nd %d index-Nd+1 %d (k,Kpi+2k,Kpi+2k+1) (%d,%d,%d) w(%d,%d,%d)\n",row,index,index-ND,((index+1)%Kpi)-ND,k,Kpi+(k<<1),Kpi+(k<<1)+1,w[k],w[Kpi+(k<<1)],w[Kpi+1+(k<<1)]);

      if (w[k]== LTE_NULL)
        nulled++;

      if (w[Kpi+(k<<1)] ==LTE_NULL)
        nulled++;

      if (w[Kpi+1+(k<<1)] ==LTE_NULL)
        nulled++;

#endif
      index3+=96;
      k++;
      k2+=2;
    }
  }

  if (ND>0)
    w[(3*Kpi) - 1] = LTE_NULL;

#ifdef RM_DEBUG

  if (ND>0) {
    printf("RM_TX: Nulled last component in pos %d\n",Kpi-1+k2);
    nulled++;
  }

  printf("RM_TX: Nulled %d\n",nulled);
#endif
  return(RTC);
}


uint32_t sub_block_interleaving_cc(uint32_t D, uint8_t *d,uint8_t *w) {
  uint32_t RCC = (D>>5), ND, ND3;
  uint32_t row,col,Kpi,index;
  uint32_t index3,k;
#ifdef RM_DEBUG_CC
  uint32_t nulled=0;
#endif

  if ((D&0x1f) > 0)
    RCC++;

  Kpi = (RCC<<5);
  //  Kpi3 = Kpi*3;
  ND = Kpi - D;
#ifdef RM_DEBUG_CC
  printf("sub_block_interleaving_cc : D = %d (%d), d %p, w %p\n",D,D*3,d,w);
  printf("RCC = %d, Kpi=%d, ND=%d\n",RCC,Kpi,ND);
#endif
  ND3 = ND*3;
  k=0;

  for (col=0; col<32; col++) {
#ifdef RM_DEBUG_CC
    printf("Col %d\n",col);
#endif
    index = bitrev_cc[col];
    index3 = 3*index;

    for (row=0; row<RCC; row++) {
      w[k]          =  d[(int32_t)index3-(int32_t)ND3];
      w[Kpi+k]     =   d[(int32_t)index3-(int32_t)ND3+1];
      w[(Kpi<<1)+k] =  d[(int32_t)index3-(int32_t)ND3+2];
#ifdef RM_DEBUG_CC
      printf("row %d, index %d k %d w(%d,%d,%d)\n",row,index,k,w[k],w[Kpi+k],w[(Kpi<<1)+k]);

      if (w[k]== LTE_NULL)
        nulled++;

      if (w[Kpi+k] ==LTE_NULL)
        nulled++;

      if (w[(Kpi<<1)+k] ==LTE_NULL)
        nulled++;

#endif
      index3+=96;
      index+=32;
      k++;
    }
  }

#ifdef RM_DEBUG_CC
  printf("RM_TX: Nulled %d\n",nulled);
#endif
  return(RCC);
}

void sub_block_deinterleaving_turbo(uint32_t D,int16_t *d,int16_t *w) {
  uint32_t RTC = (D>>5), ND, ND3;
  uint32_t row,col,Kpi,index;
  uint32_t index3,k,k2;
  int16_t *d1,*d2,*d3;

  if ((D&0x1f) > 0)
    RTC++;

  Kpi = (RTC<<5);
  //  Kpi3 = Kpi*3;
  ND = Kpi - D;
#ifdef RM_DEBUG2
  printf("sub_block_interleaving_turbo : D = %d (%d)\n",D,D*3);
  printf("RTC = %d, Kpi=%d, ND=%d\n",RTC,Kpi,ND);
#endif
  ND3 = ND*3;
  // copy d02 to dD2 (for mod Kpi operation from clause (4), p.16 of 36.212
  k=0;
  k2=0;
  d1 = d-ND3;
  d2 = d1+1;
  d3 = d1+5;

  for (col=0; col<32; col++) {
#ifdef RM_DEBUG2
    printf("Col %d\n",col);
#endif
    index = bitrev[col];
    index3 = bitrev_x3[col];//3*index;

    for (row=0; row<RTC; row++) {
      d1[index3]   = w[k];
      d2[index3]   = w[Kpi+k2];
      d3[index3]   = w[Kpi+1+k2];
      index3+=96;
      index+=32;
      k++;
      k2++;
      k2++;
    }
  }

  //  if (ND>0)
  //    d[2] = LTE_NULL;//d[(3*D)+2];
}

void sub_block_deinterleaving_cc(uint32_t D,int8_t *d,int8_t *w) {
  //WANG_Hao uint32_t RCC = (D>>5), ND, ND3;
  uint32_t RCC = (D>>5);
  ptrdiff_t   ND, ND3;
  uint32_t row,col,Kpi,index;
  //WANG_Hao uint32_t index3,k;
  ptrdiff_t index3;
  uint32_t k;

  if ((D&0x1f) > 0)
    RCC++;

  Kpi = (RCC<<5);
  //  Kpi3 = Kpi*3;
  ND = Kpi - D;
#ifdef RM_DEBUG2
  printf("sub_block_interleaving_cc : D = %d (%d), d %p, w %p\n",D,D*3,d,w);
  printf("RCC = %d, Kpi=%d, ND=%ld\n",RCC,Kpi,(long)ND);
#endif
  ND3 = ND*3;
  k=0;

  for (col=0; col<32; col++) {
#ifdef RM_DEBUG2
    printf("Col %d\n",col);
#endif
    index = bitrev_cc[col];
    index3 = 3*index;

    for (row=0; row<RCC; row++) {
      d[index3-ND3]   = w[k];
      d[index3-ND3+1] = w[Kpi+k];
      d[index3-ND3+2] = w[(Kpi<<1)+k];
#ifdef RM_DEBUG2
      printf("row %d, index %d k %d index3-ND3 %ld w(%d,%d,%d)\n",
             row,index,k,(long)(index3-ND3),w[k],w[Kpi+k],w[(Kpi<<1)+k]);
#endif
      index3+=96;
      index+=32;
      k++;
    }
  }
}

uint32_t generate_dummy_w(uint32_t D, uint8_t *w,uint8_t F) {
  uint32_t RTC = (D>>5), ND;
  uint32_t col,Kpi,index;
  int32_t k,k2;
#ifdef RM_DEBUG
  uint32_t nulled=0;
#endif
  uint8_t *wKpi,*wKpi1,*wKpi2,*wKpi4;

  if ((D&0x1f) > 0)
    RTC++;

  Kpi = (RTC<<5);
  //  Kpi3 = Kpi*3;
  ND = Kpi - D;
#ifdef RM_DEBUG
  printf("dummy sub_block_interleaving_turbo : D = %d (%d)\n",D,D*3);
  printf("RTC = %d, Kpi=%d, ND=%d, F=%d (Nulled %d)\n",RTC,Kpi,ND,F,(2*F + 3*ND));
#endif
  k=0;
  k2=0;
  wKpi = &w[Kpi];
  wKpi1 = &w[Kpi+1];
  wKpi2 = &w[Kpi+2];
  wKpi4 = &w[Kpi+4];

  for (col=0; col<32; col++) {
#ifdef RM_DEBUG
    printf("Col %d\n",col);
#endif
    index = bitrev[col];

    if (index<(ND+F)) {
      w[k]   =  LTE_NULL;
      wKpi[k2] = LTE_NULL;
#ifdef RM_DEBUG
      nulled+=2;
#endif
    }

    //bits beyond 32 due to "filler" bits
    if ((index+32)<(ND+F)) {
      w[k+1]   =  LTE_NULL;
      wKpi2[k2] = LTE_NULL;
#ifdef RM_DEBUG
      nulled+=2;
#endif
    }

    if ((index+64)<(ND+F)) {
      w[k+2]   =  LTE_NULL;
      wKpi4[k2] = LTE_NULL;
#ifdef RM_DEBUG
      nulled+=2;
#endif
    }

    if ((index+1)<ND) {
      wKpi1[k2] =  LTE_NULL;
#ifdef RM_DEBUG
      nulled+=1;
#endif
    }

#ifdef RM_DEBUG
    printf("k %d w (%d,%d,%d) w+1 (%d,%d,%d), index %d index-ND-F %d index+32-ND-F %d\n",k,w[k],w[Kpi+(k<<1)],w[Kpi+1+(k<<1)],w[k+1],w[2+Kpi+(k<<1)],w[2+Kpi+1+(k<<1)],index,index-ND-F,index+32-ND-F);
#endif
    k+=RTC;
    k2=k<<1;
  }

  // copy d02 to dD2 (for mod Kpi operation from clause (4), p.16 of 36.212
  if (ND>0)
    w[(3*Kpi)-1] = LTE_NULL;

#ifdef RM_DEBUG

  if (ND>0) {
    nulled++;
    printf("dummy_w: Nulled final position %d\n",(3*Kpi)-1);
  }

  printf("Nulled = %d\n",nulled);
#endif
  return(RTC);
}

uint32_t generate_dummy_w_cc(uint32_t D, uint8_t *w) {
  uint32_t RCC = (D>>5), ND;
  uint32_t col,Kpi,index;
  int32_t k;
#ifdef RM_DEBUG_CC
  uint32_t nulled=0;
#endif

  if ((D&0x1f) > 0)
    RCC++;

  Kpi = (RCC<<5);
  //  Kpi3 = Kpi*3;
  ND = Kpi - D;
#ifdef RM_DEBUG_CC
  printf("dummy sub_block_interleaving_cc : D = %d (%d)\n",D,D*3);
  printf("RCC = %d, Kpi=%d, ND=%d, (Nulled %d)\n",RCC,Kpi,ND,3*ND);
#endif
  //  ND3 = ND*3;
  // copy d02 to dD2 (for mod Kpi operation from clause (4), p.16 of 36.212
  k=0;

  for (col=0; col<32; col++) {
#ifdef RM_DEBUG_CC
    printf("Col %d\n",col);
#endif
    index = bitrev_cc[col];

    if (index<ND) {
      w[k]          = LTE_NULL;
      w[Kpi+k]      = LTE_NULL;
      w[(Kpi<<1)+k] = LTE_NULL;
#ifdef RM_DEBUG_CC
      nulled+=3;
#endif
    }

    /*
    //bits beyond 32 due to "filler" bits
    if (index+32<ND) {
      w[k+1]          = LTE_NULL;
      w[Kpi+1+k]      = LTE_NULL;
      w[(Kpi<<1)+1+k] = LTE_NULL;
    #ifdef RM_DEBUG
      nulled+=3;
    #endif
    }
    if (index+64<ND) {
      w[k+2]          = LTE_NULL;
      w[Kpi+2+k]      = LTE_NULL;
      w[(Kpi<<1)+2+k] = LTE_NULL;
    #ifdef RM_DEBUG
      nulled+=3;
    #endif
    }


    if ((index+1)<ND) {
      w[Kpi+1+(k<<1)] =  LTE_NULL;
    #ifdef RM_DEBUG
      nulled+=1;
    #endif
    }
    */
#ifdef RM_DEBUG_CC
    printf("k %d w (%d,%d,%d), index-ND %d index+32-ND %d\n",k,w[k],w[Kpi+k],w[(Kpi<<1)+k],index-ND,index+32-ND);
#endif
    k+=RCC;
  }

#ifdef RM_DEBUG_CC
  printf("Nulled = %d\n",nulled);
#endif
  return(RCC);
}


uint32_t lte_rate_matching_turbo(uint32_t RTC,
                                 uint32_t G,
                                 uint8_t *w,
                                 uint8_t *e,
                                 uint8_t C,
                                 uint32_t Nsoft,
                                 uint8_t Mdlharq,
                                 uint8_t Kmimo,
                                 uint8_t rvidx,
                                 uint8_t Qm,
                                 uint8_t Nl,
                                 uint8_t r,
                                 uint8_t nb_rb)
//                                 uint8_t m)
{
  uint32_t Nir,Ncb,Gp,GpmodC,E,Ncbmod,ind,k;
  //  int cnt=0;
  uint8_t *e2;
#ifdef RM_DEBUG_TX
  int code_block,round;
  int cnt;
  int zeroed=0;
  int oned=0;
  int twoed=0;
  int threed =0;
  uint32_t nulled=0;
  static unsigned char *counter_buffer[MAX_NUM_DLSCH_SEGMENTS][4];
  char fname[512];
#endif

  if (Mdlharq>0) {  // Downlink
    Nir = Nsoft/Kmimo/cmin(8,Mdlharq);
    Ncb = cmin(Nir/C,3*(RTC<<5));
  } else { // Uplink
    Nir=0;
    Ncb = 3*(RTC<<5); // Kw
  }

#ifdef RM_DEBUG_TX

  if (rvidx==0 && r==0) {
    for(round=0; round<4; round++)
      for (code_block=0; code_block<MAX_NUM_DLSCH_SEGMENTS; code_block++) {
        counter_buffer[code_block][round] = (unsigned char *)malloc(Ncb*sizeof(char));
        memset(counter_buffer[code_block][round],0,Ncb*sizeof(char));
      }
  } else if(rvidx==3) {
    sprintf(fname, "mcs%d_rate_matching_RB_%d.txt", m, nb_rb);
    // sprintf(fname,"mcs0_rate_matching_RB_6.txt");
  }

#endif

  // if (rvidx==3)
  //  for (cnt=0;cnt<Ncb;cnt++)
  //    counter_buffer[rvidx][cnt]=0;
  if (Ncb>(3*(RTC<<5)))
    AssertFatal(1==0,"Exiting, RM condition (Ncb %d, RTC %d, Nir/C %d, Nsoft %d, Kw %d)\n",Ncb,RTC,Nir/C,Nsoft,3*(RTC<<5));

  AssertFatal(Nl>0,"Nl is 0\n");
  AssertFatal(Qm>0,"Qm is 0\n");
  Gp = G/Nl/Qm;
  GpmodC = Gp%C;
#ifdef RM_DEBUG
  LOG_D(PHY,"lte_rate_matching_turbo: Ncb %d, Kw %d, Nir/C %d, rvidx %d, G %d, Qm %d, Nl%d, r %d\n",Ncb,3*(RTC<<5),Nir/C,rvidx, G, Qm,Nl,r);
#endif

  if (r < (C-(GpmodC)))
    E = Nl*Qm * (Gp/C);
  else
    E = Nl*Qm * ((GpmodC==0?0:1) + (Gp/C));

  Ncbmod = Ncb%(RTC<<3);
  ind = RTC * (2+(rvidx*(((Ncbmod==0)?0:1) + (Ncb/(RTC<<3)))*2));
#ifdef RM_DEBUG_TX
  printf("lte_rate_matching_turbo: E %d, k0 %d, Ncbmod %d, Ncb/(RTC<<3) %d\n",E,ind,Ncbmod,Ncb/(RTC<<3));
#endif
  //e2=e+(r*E);
  e2 = e;
  k=0;

  for (; (ind<Ncb)&&(k<E); ind++) {
    //    e2[k]=w[ind];
#ifdef RM_DEBUG_TX
    printf("RM_TX k%d Ind: %d (%d)\n",k,ind,w[ind]);
#endif

    if (w[ind] != LTE_NULL) e2[k++]=w[ind];
  }

  while(k<E) {
    for (ind=0; (ind<Ncb)&&(k<E); ind++) {
      //      e2[k] = w[ind];
#ifdef RM_DEBUG_TX
      printf("RM_TX k%d Ind: %d (%d)\n",k,ind,w[ind]);
#endif

      if (w[ind] != LTE_NULL) e2[k++]=w[ind];
    }
  }

  /*
  for (k=0;k<E;k++) {


    while(w[ind] == LTE_NULL) {
  #ifdef RM_DEBUG_TX
      printf("RM_tx : ind %d, NULL\n",ind);
      nulled++;
  #endif
      ind++;
      if (ind==Ncb)
  ind=0;
    }

    e2[k] = w[ind];
    //    printf("RM_TX k%d Ind: %d (%d)\n",k,ind,w[ind]);
    //    cnt = cnt+1;
  #ifdef RM_DEBUG_TX
    counter_buffer[r][rvidx][ind]++;
    // printf("Bit_Counter[%d][%d][%d]=%d\n",r,rvidx,ind,counter_buffer[r][rvidx][ind]);
    //    printf("k %d ind %d, w %c(%d)\n",k,ind,w[ind],w[ind]);
    // printf("RM_TX %d (%d) Ind: %d (%d)\n",k,k+r*E,ind,e2[k]);
  #endif
    ind++;
    if (ind==Ncb)
      ind=0;
  }

  #ifdef RM_DEBUG_TX
  if (rvidx==3){
    for(cnt=0;cnt<Ncb;cnt++)
      {
  fprintf(counter_fd,"%d %x %x %x %x %x %x %x %x %x %x %x %x\n",cnt,
    counter_buffer[0][0][cnt],counter_buffer[1][0][cnt],counter_buffer[2][0][cnt],
    counter_buffer[0][1][cnt],counter_buffer[1][1][cnt],counter_buffer[2][1][cnt],
    counter_buffer[0][2][cnt],counter_buffer[1][2][cnt],counter_buffer[2][2][cnt],
    counter_buffer[0][3][cnt],counter_buffer[1][3][cnt],counter_buffer[2][3][cnt]
    );
    }
    fclose(counter_fd);

  }


   for(cnt=0;cnt<Ncb;cnt++){
    printf("Bit_Counter[%d][%d]=%d\n",rvidx,cnt,counter_buffer[r][rvidx][cnt]);
    if(counter_buffer[r][rvidx][cnt]==0)
      zeroed++;
    else if(counter_buffer[r][rvidx][cnt]==1)
      oned++;
    else if(counter_buffer[r][rvidx][cnt]==2)
      twoed++;
    else if(counter_buffer[r][rvidx][cnt]==3)
      threed++;
   }

  printf("zeroed %d\n",zeroed);
  printf("oned %d\n",oned);
  printf("twoed %d\n",twoed);
  printf("threed %d\n",threed);

  printf("nulled %d\n",nulled);

  #endif
  */
  return(E);
}


uint32_t lte_rate_matching_cc(uint32_t RCC,
                              uint16_t E,
                              uint8_t *w,
                              uint8_t *e) {
  uint32_t ind=0,k;
  uint16_t Kw = 3*(RCC<<5);
#ifdef RM_DEBUG_CC
  uint32_t nulled=0;
  printf("lte_rate_matching_cc: Kw %d, E %d\n",Kw, E);
#endif

  for (k=0; k<E; k++) {
    while(w[ind] == LTE_NULL) {
#ifdef RM_DEBUG_CC
      nulled++;
      printf("RM_TX_CC : ind %d, NULL\n",ind);
#endif
      ind++;

      if (ind==Kw)
        ind=0;
    }

    e[k] = w[ind];
#ifdef RM_DEBUG_CC
    //    printf("k %d ind %d, w %c(%d)\n",k,ind,w[ind],w[ind]);
    printf("RM_TX_CC %d Ind: %d (%d)\n",k,ind,e[k]);
#endif
    ind++;

    if (ind==Kw)
      ind=0;
  }

#ifdef RM_DEBUG_CC
  printf("nulled %d\n",nulled);
#endif
  return(E);
}


int lte_rate_matching_turbo_rx(uint32_t RTC,
                               uint32_t G,
                               int16_t *w,
                               uint8_t *dummy_w,
                               int16_t *soft_input,
                               uint8_t C,
                               uint32_t Nsoft,
                               uint8_t Mdlharq,
                               uint8_t Kmimo,
                               uint8_t rvidx,
                               uint8_t clear,
                               uint8_t Qm,
                               uint8_t Nl,
                               uint8_t r,
                               uint32_t *E_out) {
  uint32_t Nir,Ncb,Gp,GpmodC,E,Ncbmod,ind,k;
  int16_t *soft_input2;
  //   int32_t w_tmp;
#ifdef RM_DEBUG
  int nulled=0;
#endif

  if (Kmimo==0 || C==0 || Qm==0 || Nl==0) {
    printf("lte_rate_matching.c: invalid parameters (Kmimo %d, Mdlharq %d, C %d, Qm %d, Nl %d\n",
           Kmimo,Mdlharq,C,Qm,Nl);
    return(-1);
  }

  if (Mdlharq>0) { // Downlink
    Nir = Nsoft/Kmimo/cmin(8,Mdlharq);
    Ncb = cmin(Nir/C,3*(RTC<<5));
  } else { // Uplink
    Nir=0;
    Ncb = 3*(RTC<<5);
  }

  AssertFatal(Nl>0,"Nl is 0\n");
  AssertFatal(Qm>0,"Qm is 0\n");
  Gp = G/Nl/Qm;
  GpmodC = Gp%C;

  if (r < (C-(GpmodC)))
    E = Nl*Qm * (Gp/C);
  else
    E = Nl*Qm * ((GpmodC==0?0:1) + (Gp/C));

  Ncbmod = Ncb%(RTC<<3);
  ind = RTC * (2+(rvidx*(((Ncbmod==0)?0:1) + (Ncb/(RTC<<3)))*2));
#ifdef RM_DEBUG
  printf("lte_rate_matching_turbo_rx: Clear %d, E %d, Ncb %d, Kw %d, rvidx %d, G %d, Qm %d, Nl%d, r %d\n",clear,E,Ncb,3*(RTC<<5),rvidx, G, Qm,Nl,r);
#endif

  if (clear==1)
    memset(w,0,Ncb*sizeof(int16_t));

  soft_input2 = soft_input;// + (r*E);
  k=0;

  for (; (ind<Ncb)&&(k<E); ind++) {
    if (dummy_w[ind] != LTE_NULL) {
      /*
      if ((w[ind]>0 && soft_input2[k]<0) ||
      (w[ind]<0 && soft_input2[k]>0))
      printf("ind %d: w %d => soft_in %d\n",ind,w[ind],soft_input2[k]);*/
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
      if (dummy_w[ind] != LTE_NULL) {
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

  /*
  for (k=0;k<E;k++) {


    while(dummy_w[ind] == LTE_NULL) {
  #ifdef RM_DEBUG
      printf("RM_rx : ind %d, NULL\n",ind);
  #endif
      ind++;
      if (ind==Ncb)
  ind=0;
    }
  */
  /*
  if (w[ind] != 0)
    printf("repetition %d (%d,%d,%d)\n",ind,rvidx,E,Ncb);
  */
  // Maximum-ratio combining of repeated bits and retransmissions
  /*
  w_tmp = (int) w[ind] + (int) soft_input2[k];
  if (w_tmp > 32767) {
    //#ifdef DEBUG_RM
    printf("OVERFLOW!!!!!, w_tmp = %d\n",w_tmp);
    //#endif
    w[ind] = 32767;
  }
  else if (w_tmp < -32768) {
    //#ifdef DEBUG_RM
    printf("UNDERFLOW!!!!!, w_tmp = %d\n",w_tmp);
    //#endif
    w[ind] = -32768;
  }
  else
  */
  /*
    w[ind] += soft_input2[k];
  #ifdef RM_DEBUG
      printf("RM_RX k%d Ind: %d (%d)\n",k,ind,w[ind]);
  #endif
    ind++;
    if (ind==Ncb)
      ind=0;
  }
  */
  *E_out = E;
  return(0);
}


void lte_rate_matching_cc_rx(uint32_t RCC,
                             uint16_t E,
                             int8_t *w,
                             uint8_t *dummy_w,
                             int8_t *soft_input) {
  uint32_t ind=0,k;
  uint16_t Kw = 3*(RCC<<5);
  uint32_t acc=1;
  int16_t w16[Kw];
#ifdef RM_DEBUG_CC
  uint32_t nulled=0;
  printf("lte_rate_matching_cc_rx: Kw %d, E %d, w %p, soft_input %p\n",3*(RCC<<5),E,w,soft_input);
#endif
  memset(w,0,Kw);
  memset(w16,0,Kw*sizeof(int16_t));

  for (k=0; k<E; k++) {
    while(dummy_w[ind] == LTE_NULL) {
#ifdef RM_DEBUG_CC
      nulled++;
      printf("RM_RX : ind %d, NULL\n",ind);
#endif
      ind++;

      if (ind==Kw)
        ind=0;
    }

    /*
    if (w[ind] != 0)
      printf("repetition %d (%d,%d,%d)\n",ind,rvidx,E,Ncb);
    */
    // Maximum-ratio combining of repeated bits and retransmissions
#ifdef RM_DEBUG_CC
    printf("RM_RX_CC k %d (%d) ind: %d (%d)\n",k,soft_input[k],ind,w16[ind]);
#endif
    w16[ind] += soft_input[k];
    ind++;

    if (ind==Kw) {
      ind=0;
      acc++;
    }
  }

  // rescale
  for (ind=0; ind<Kw; ind++) {
    //    w16[ind]=(w16[ind]/acc);
    if (w16[ind]>7)
      w[ind]=7;
    else if (w16[ind]<-8)
      w[ind]=-8;
    else
      w[ind]=(int8_t)w16[ind];
  }

#ifdef RM_DEBUG_CC
  printf("Nulled %d\n",nulled);
#endif
}


#ifdef MAIN

void main() {
  uint8_t d[96+3+(3*6144)];
  uint8_t w[3*6144],e[12*6144];
  uint32_t RTC,G,rvidx;
  uint32_t nb_rb=6;
  uint32_t mod_order = 4;
  uint32_t first_dlsch_symbol = 2;
  uint32_t i;
  G = ( nb_rb * (12 * mod_order) * (12-first_dlsch_symbol-3)) ;//( nb_rb * (12 * mod_order) * (14-first_dlsch_symbol-3)) :

  // initialize 96 first positions to "LTE_NULL"
  for (i=0; i<96; i++)
    d[i]=LTE_NULL;

  RTC = sub_block_interleaving_turbo(4+(192*8), &d[96], w);

  for (rvidx=0; rvidx<4; rvidx++) {
    lte_rate_matching_turbo(RTC,
                            G,
                            w,
                            e,
                            1,           //C
                            1827072,     //Nsoft,
                            8,           //Mdlharq,
                            1,           //Kmimo,
                            rvidx,       //rvidx,
                            mod_order,   //Qm,
                            1,           //Nl,
                            0            //r
                           );
  }
}

#endif

