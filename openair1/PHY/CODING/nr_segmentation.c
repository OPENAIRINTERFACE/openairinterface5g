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

/* file: nr_segmentation.c
   purpose: Procedures for transport block segmentation for NR (LDPC-coded transport channels)
   author: Hongzhi WANG (TCL)
   date: 12.09.2017
*/
#include "PHY/defs_nr_UE.h"
//#include "SCHED/extern.h"

//#define DEBUG_SEGMENTATION

int32_t nr_segmentation(unsigned char *input_buffer,
                        unsigned char **output_buffers,
                        unsigned int B,
                        unsigned int *C,
                        unsigned int *K,
                        unsigned int *Zout, // [hna] Zout is Zc
                        unsigned int *F,
                        uint8_t BG)
{

  unsigned int L,Bprime,Z,r,Kcb,Kb,k,s,crc,Kprime;

  if (BG==1)
    Kcb=8448;
  else
    Kcb=3840;

  if (B<=Kcb) {
    L=0;
    *C=1;
    Bprime=B;
  } else {
    L=24;
    *C = B/(Kcb-L);

    if ((Kcb-L)*(*C) < B)
      *C=*C+1;

    Bprime = B+((*C)*L);
#ifdef DEBUG_SEGMENTATION
    printf("Bprime %u\n",Bprime);
#endif
  }

  if ((*C)>MAX_NUM_NR_DLSCH_SEGMENTS) {
    LOG_E(PHY,"nr_segmentation.c: too many segments %d, B %d, L %d, Bprime %d\n",*C,B,L,Bprime);
    return(-1);
  }

  // Find K+
  Kprime = Bprime/(*C);

  if (BG==1)
    Kb = 22;
  else {
    if (B > 640) {
      Kb = 10;
    } else if (B > 560) {
      Kb = 9;
    } else if (B > 192) {
      Kb = 8;
    }
    else {
      Kb = 6;
    }
  }


if ((Kprime%Kb) > 0)
  Z  = (Kprime/Kb)+1;
else
  Z = (Kprime/Kb);

 LOG_D(PHY,"nr segmetation B %u Bprime %u Kprime %u z %u \n", B, Bprime, Kprime, Z);
	  
  if (Z <= 2) {
    *K = 2;
  } else if (Z<=16) { // increase by 1 byte til here
    *K = Z;
  } else if (Z <=32) { // increase by 2 bytes til here
    *K = (Z>>1)<<1;

    if (*K < Z)
      *K = *K + 2;

  } else if (Z <= 64) { // increase by 4 bytes til here
    *K = (Z>>2)<<2;

    if (*K < Z)
      *K = *K + 4;

  } else if (Z <=128 ) { // increase by 8 bytes til here

    *K = (Z>>3)<<3;

    if (*K < Z)
      *K = *K + 8;

#ifdef DEBUG_SEGMENTATION
    printf("Z_by_C %u , K2 %u\n",Z,*K);
#endif
  } else if (Z <= 256) { // increase by 4 bytes til here
      *K = (Z>>4)<<4;

      if (*K < Z)
        *K = *K + 16;

  } else if (Z <= 384) { // increase by 4 bytes til here
      *K = (Z>>5)<<5;

      if (*K < Z)
        *K = *K + 32;

  } else {
    //msg("nr_segmentation.c: Illegal codeword size !!!\n");
    return -1;
  }

  *Zout = *K;

  if(BG==1)
    *K = *K*22;
  else
    *K = *K*10;

  *F = ((*K) - Kprime);

  LOG_D(PHY,"final nr seg output Z %u K %u F %u \n", *Zout, *K, *F);
  LOG_D(PHY,"C %u, K %u, Bprime_bytes %u, Bprime %u, F %u\n",*C,*K,Bprime>>3,Bprime,*F);

  if ((input_buffer) && (output_buffers)) {

    s = 0;

    for (r=0; r<*C; r++) {

      k = 0;

      while (k<((Kprime - L)>>3)) {
        output_buffers[r][k] = input_buffer[s];
		//printf("encoding segment %d : byte %d (%d) => %d\n",r,k,(Kprime-L)>>3,input_buffer[s]);
        k++;
        s++;
      }

      if (*C > 1) { // add CRC
        crc = crc24b(output_buffers[r],Kprime-L)>>8;
        output_buffers[r][(Kprime-L)>>3] = ((uint8_t*)&crc)[2];
        output_buffers[r][1+((Kprime-L)>>3)] = ((uint8_t*)&crc)[1];
        output_buffers[r][2+((Kprime-L)>>3)] = ((uint8_t*)&crc)[0];
      }

      if (*F>0) {
        for (k=Kprime>>3; k<(*K)>>3; k++) {
          output_buffers[r][k] = 0;
          //printf("r %d filler bits [%d] = %d Kprime %d \n", r,k, output_buffers[r][k], Kprime);
        }
      }

    }
  }

  return Kb;
}



#ifdef MAIN
main()
{

  unsigned int K,C,F,Bbytes, Zout;

  for (Bbytes=5; Bbytes<8; Bbytes++) {
    nr_segmentation(0,0,Bbytes<<3,&C,&K,&Zout, &F);
    printf("Bbytes %d : C %d, K %d, F %d\n",
           Bbytes, C, K, F);
  }
}
#endif
