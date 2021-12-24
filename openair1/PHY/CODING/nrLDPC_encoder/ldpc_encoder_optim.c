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

/*!\file ldpc_encoder2.c
 * \brief Defines the optimized LDPC encoder
 * \author Florian Kaltenberger, Raymond Knopp, Kien le Trung (Eurecom)
 * \email openair_tech@eurecom.fr
 * \date 27-03-2018
 * \version 1.0
 * \note
 * \warning
 */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <types.h>
#include "assertions.h"
#include "common/utils/LOG/log.h"
#include "PHY/TOOLS/time_meas.h"
#include "openair1/PHY/CODING/nrLDPC_defs.h"
#include "ldpc_encode_parity_check.c"
#include "ldpc_generate_coefficient.c"
//#define DEBUG_LDPC





int nrLDPC_encod(unsigned char **test_input,unsigned char **channel_input,int Zc,int Kb,short block_length, short BG, encoder_implemparams_t *impp)
{

  short nrows=0,ncols=0;
  int i,i1,rate=3;
  int no_punctured_columns,removed_bit;

  int simd_size;

  //determine number of bits in codeword
   //if (block_length>3840)
   if (BG==1)
     {
       //BG=1;
       nrows=46; //parity check bits
       ncols=22; //info bits
       rate=3;
     }
     //else if (block_length<=3840)
    else if	(BG==2)
     {
       //BG=2;
       nrows=42; //parity check bits
       ncols=10; // info bits
       rate=5;

       }


#ifdef DEBUG_LDPC
  LOG_D(PHY,"ldpc_encoder_optim_8seg: BG %d, Zc %d, Kb %d, block_length %d\n",BG,Zc,Kb,block_length);
  LOG_D(PHY,"ldpc_encoder_optim_8seg: PDU %x %x %x %x\n",test_input[0][0],test_input[0][1],test_input[0][2],test_input[0][3]);
#endif

  if ((Zc&31) > 0) simd_size = 16;
  else simd_size = 32;

  unsigned char c[22*Zc] __attribute__((aligned(32))); //padded input, unpacked, max size
  unsigned char d[46*Zc] __attribute__((aligned(32))); //coded parity part output, unpacked, max size

  unsigned char c_extension[2*22*Zc*simd_size] __attribute__((aligned(32)));      //double size matrix of c

  // calculate number of punctured bits
  no_punctured_columns=(int)((nrows-2)*Zc+block_length-block_length*rate)/Zc;
  removed_bit=(nrows-no_punctured_columns-2) * Zc+block_length-(int)(block_length*rate);
  // printf("%d\n",no_punctured_columns);
  // printf("%d\n",removed_bit);
  // unpack input
  memset(c,0,sizeof(unsigned char) * ncols * Zc);
  memset(d,0,sizeof(unsigned char) * nrows * Zc);

  if(impp->tinput != NULL) start_meas(impp->tinput);
  for (i=0; i<block_length; i++) {
    c[i] = (test_input[0][i/8]&(128>>(i&7)))>>(7-(i&7));
      //printf("c(%d,%d)=%d\n",j,i,temp);
    }

  if(impp->tinput != NULL) stop_meas(impp->tinput);

  if ((BG==1 && Zc>176) || (BG==2 && Zc>64)) { 
    // extend matrix
    if(impp->tprep != NULL) start_meas(impp->tprep);
    for (i1=0; i1 < ncols; i1++)
      {
	memcpy(&c_extension[2*i1*Zc], &c[i1*Zc], Zc*sizeof(unsigned char));
	memcpy(&c_extension[(2*i1+1)*Zc], &c[i1*Zc], Zc*sizeof(unsigned char));
      }
    for (i1=1;i1<simd_size;i1++) {
      memcpy(&c_extension[(2*ncols*Zc*i1)], &c_extension[i1], (2*ncols*Zc*sizeof(unsigned char))-i1);
      //    memset(&c_extension[(2*ncols*Zc*i1)],0,i1);
      /*
	printf("shift %d: ",i1);
	for (int j=0;j<64;j++) printf("%d ",c_extension[(2*ncols*Zc*i1)+j]);
	printf("\n");
      */
    }
    if(impp->tprep != NULL) stop_meas(impp->tprep);
    //parity check part
    if(impp->tparity != NULL) start_meas(impp->tparity);
    encode_parity_check_part_optim(c_extension, d, BG, Zc, Kb);
    if(impp->tparity != NULL) stop_meas(impp->tparity);
  }
  else {
    if (encode_parity_check_part_orig(c, d, BG, Zc, Kb, block_length)!=0) {
      printf("Problem with encoder\n");
      return(-1);
    }
  }
  if(impp->toutput != NULL) start_meas(impp->toutput);
  // information part and puncture columns
  memcpy(&channel_input[0][0], &c[2*Zc], (block_length-2*Zc)*sizeof(unsigned char));
  memcpy(&channel_input[0][block_length-2*Zc], &d[0], ((nrows-no_punctured_columns) * Zc-removed_bit)*sizeof(unsigned char));

  if(impp->toutput != NULL) stop_meas(impp->toutput);
  return 0;
}


