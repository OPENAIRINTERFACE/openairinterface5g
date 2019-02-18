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
					 unsigned int *Zout,
                     unsigned int *F)
{

  unsigned int L,Bprime,Bprime_by_C,Z,r,Kb,k,s,crc,Kprime;

  if (B<=8448) {
    L=0;
    *C=1;
    Bprime=B;
  } else {
    L=24;
    *C = B/(8448-L);

    if ((8448-L)*(*C) < B)
      *C=*C+1;

    Bprime = B+((*C)*L);
#ifdef DEBUG_SEGMENTATION
    printf("Bprime %d\n",Bprime);
#endif
  }

  if ((*C)>MAX_NUM_DLSCH_SEGMENTS) {
      LOG_E(PHY,"nr_segmentation.c: too many segments %d, B %d, L %d, Bprime %d\n",*C,B,L,Bprime);
    return(-1);
  }

  // Find K+
  Bprime_by_C = Bprime/(*C);
  if (Bprime <=192) {
	  Kb = 6;
  } else if (Bprime <=560) {
	  Kb = 8;
  } else if (Bprime <=640) {
	  Kb = 9;
  } else if (Bprime <=3840) {
	  Kb = 10;;
  } else {
	  Kb = 22;
  }


if ((Bprime_by_C%Kb) > 0)
  Z  = (Bprime_by_C/Kb)+1;
else
	Z = (Bprime_by_C/Kb);

#ifdef DEBUG_SEGMENTATION
 printf("nr segmetation B %d Bprime %d Bprime_by_C %d z %d \n", B, Bprime, Bprime_by_C, Z);
#endif
	  
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
    printf("Z_by_C %d , K2 %d\n",Z,*K);
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
  *K = *K*Kb;

  *F = ((*K) - Bprime_by_C);

#ifdef DEBUG_SEGMENTATION
  printf("final nr seg output Z %d K %d F %d \n", *Zout, *K, *F);
  printf("C %d, K %d, Bprime_bytes %d, Bprime %d, F %d\n",*C,*K,Bprime>>3,Bprime,*F);
#endif

  if ((input_buffer) && (output_buffers)) {

    s = 0;

    for (r=0; r<*C; r++) {

      k = 0;
      Kprime = Bprime_by_C;

      while (k<((Kprime - L)>>3)) {
        output_buffers[r][k] = input_buffer[s];
		//printf("encoding segment %d : byte %d (%d) => %d\n",r,k,(Kprime-L)>>3,input_buffer[s]);
        k++;
        s++;
      }

      if (*F>0) {
        for (k=Kprime>>3; k<(*K)>>3; k++) {
          output_buffers[r][k] = NR_NULL;
          //printf("r %d filler bits [%d] = %d Kprime %d \n", r,k, output_buffers[r][k], Kprime);
        }
      }

      if (*C > 1) { // add CRC
        crc = crc24b(output_buffers[r],Kprime-L)>>8;
        output_buffers[r][(Kprime-L)>>3] = ((uint8_t*)&crc)[2];
        output_buffers[r][1+((Kprime-L)>>3)] = ((uint8_t*)&crc)[1];
        output_buffers[r][2+((Kprime-L)>>3)] = ((uint8_t*)&crc)[0];
      }
    }
  }

  return 0;
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
