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
#include "PHY/defs.h"
#include "SCHED/extern.h"

//#define DEBUG_SEGMENTATION

int32_t nr_segmentation(unsigned char *input_buffer,
                     unsigned char **output_buffers,
                     unsigned int B,
                     unsigned int *C,
                     unsigned int *Kplus,
                     unsigned int *Kminus,
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
      LOG_E(PHY,"lte_segmentation.c: too many segments %d, B %d, L %d, Bprime %d\n",*C,B,L,Bprime);
    return(-1);
  }

  // Find K+
  Bprime_by_C = Bprime/(*C);
  /*if (Bprime <=192) {
	  Kb = 6;
  } else if (Bprime <=560) {
	  Kb = 8;
  } else if (Bprime <=640) {
	  Kb = 9;
  } else if (Bprime <=3840) {
	  Kb = 10;;
  } else {*/
	  Kb = 22;
  //}


if ((Bprime_by_C%Kb) > 0)
  Z  = (Bprime_by_C/Kb)+1;
else
	Z = (Bprime_by_C/Kb);

#ifdef DEBUG_SEGMENTATION
 printf("nr segmetation B %d Bprime %d Bprime_by_C %d z %d \n", B, Bprime, Bprime_by_C, Z);
#endif
	  
  if (Z <= 2) {
    *Kplus = 2;
    *Kminus = 0;
  } else if (Z<=16) { // increase by 1 byte til here
    *Kplus = Z;
    *Kminus = Z-1;
  } else if (Z <=32) { // increase by 2 bytes til here
    *Kplus = (Z>>1)<<1;

    if (*Kplus < Z)
      *Kplus = *Kplus + 2;

    *Kminus = (*Kplus - 2);
  } else if (Z <= 64) { // increase by 4 bytes til here
    *Kplus = (Z>>2)<<2;

    if (*Kplus < Z)
      *Kplus = *Kplus + 4;

    *Kminus = (*Kplus - 4);
  } else if (Z <=128 ) { // increase by 8 bytes til here

    *Kplus = (Z>>3)<<3;

    if (*Kplus < Z)
      *Kplus = *Kplus + 8;

#ifdef DEBUG_SEGMENTATION
    printf("Z_by_C %d , Kplus2 %d\n",Z,*Kplus);
#endif
    *Kminus = (*Kplus - 8);
  } else if (Z <= 256) { // increase by 4 bytes til here
      *Kplus = (Z>>4)<<4;

      if (*Kplus < Z)
        *Kplus = *Kplus + 16;

      *Kminus = (*Kplus - 16);
  } else if (Z <= 384) { // increase by 4 bytes til here
      *Kplus = (Z>>5)<<5;

      if (*Kplus < Z)
        *Kplus = *Kplus + 32;

      *Kminus = (*Kplus - 32);
  } else {
    //msg("nr_segmentation.c: Illegal codeword size !!!\n");
    return(-1);
  }
  *Zout = *Kplus;
  *Kplus = *Kplus*Kb;
  *Kminus = *Kminus*Kb;

  

  *F = ((*C)*(*Kplus) - (Bprime));

#ifdef DEBUG_SEGMENTATION
  printf("final nr seg output Z %d Kplus %d F %d \n", *Zout, *Kplus, *F);
  printf("C %d, Kplus %d, Kminus %d, Bprime_bytes %d, Bprime %d, F %d\n",*C,*Kplus,*Kminus,Bprime>>3,Bprime,*F);
#endif

  if ((input_buffer) && (output_buffers)) {

    for (k=0; k<*F>>3; k++) {
      output_buffers[0][k] = 0;
    }

    s=0;

    for (r=0; r<*C; r++) {

      //if (r<(B%(*C)))
        Kprime = *Kplus;
      //else
      //  Kprime = *Kminus;

      while (k<((Kprime - L)>>3)) {
        output_buffers[r][k] = input_buffer[s];
	//	printf("encoding segment %d : byte %d (%d) => %d\n",r,k,Kr>>3,input_buffer[s]);
        k++;
        s++;
      }

      /*
      if (*C > 1) { // add CRC
        crc = crc24b(output_buffers[r],Kprime-24)>>8;
        output_buffers[r][(Kprime-24)>>3] = ((uint8_t*)&crc)[2];
        output_buffers[r][1+((Kprime-24)>>3)] = ((uint8_t*)&crc)[1];
        output_buffers[r][2+((Kprime-24)>>3)] = ((uint8_t*)&crc)[0];
      }
      */
      k=0;
    }
  }

  return(0);
}



#ifdef MAIN
main()
{

  unsigned int Kplus,Kminus,C,F,Bbytes;

  for (Bbytes=5; Bbytes<8; Bbytes++) {
    nr_segmentation(0,0,Bbytes<<3,&C,&Kplus,&Kminus, &F);
    printf("Bbytes %d : C %d, Kplus %d, Kminus %d, F %d\n",
           Bbytes, C, Kplus, Kminus, F);
  }
}
#endif
