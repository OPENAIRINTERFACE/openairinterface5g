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

// 7.2 Pseudo-random sequence generation by a length-31 Gold sequence
// for MBSFN reference signals

/* c(n) = x1(n+Nc) + x2(n+Nc) mod 2
   x1(n+31) = (x1(n+3)+x1(n))mod 2
   x2(n+31) = (x2(n+3)+x2(n+2)+x2(n+1)+x2(n))mod 2
   x1(0)=1,x1(n)=0,n=1...30
   x2 <=> cinit = sum_{i=0}^{30} x2(i)2^i

   equivalent
   x1(n) = x1(n-28) + x1(n-31)
   x2(n) = x2(n-28) + x2(n-29) + x2(n-30) + x2(n-31)
   x1(0)=1,x1(1)=0,...x1(30)=0,x1(31)=1
   x2 <=> cinit, x2(31) = x2(3)+x2(2)+x2(1)+x2(0)

   N_RB^{max,DL}=110
   c_init = 2^9 * (7(n_s + 1) + l + 1)*(2N_{ID}^{MBSFN} + 1) + 2*N_{ID}^{MBSFN}
   N_{ID}^{MBSFN} = 0..255
   sfn = Subframe Number

   //unsigned int lte_gold_table[10][3][42];  // need 165 bytes for sequence
   //slot index x pilot within slot x sequence*/

#include "lte_refsig.h"

void lte_gold_mbsfn(LTE_DL_FRAME_PARMS *frame_parms,uint32_t lte_gold_mbsfn_table[10][3][42],uint16_t Nid_mbsfn)
{

  unsigned char sfn,l;
  unsigned int n,x1,x2;//,x1tmp,x2tmp;

  for (sfn=0; sfn<10; sfn++) {
    for (l=0; l<3; l++) {

      if (l==0)
        x2 = (Nid_mbsfn) + (((1+(Nid_mbsfn<<1))*(1 + 2 + (7*(1+(sfn<<1)))))<<9); //cinit
      else
        x2 = (Nid_mbsfn) + (((1+(Nid_mbsfn<<1))*(1 + ((l-1)<<2) + (7*(2+(sfn<<1)))))<<9); //cinit

      //x2 = frame_parms->Ncp + (Nid_cell<<1) + (1+(Nid_cell<<1))*(1 + (3*l) + (7*(1+ns))); //cinit
      //n = 0
      //      printf("cinit (sfn %d, l %d) => %d\n",sfn,l,x2);

      // Initializing the Sequence

      x1 = 1+ (1<<31);
      x2=x2 ^ ((x2 ^ (x2>>1) ^ (x2>>2) ^ (x2>>3))<<31);
      // skip first 50 double words (1600 bits)
      //      printf("n=0 : x1 %x, x2 %x\n",x1,x2);

      for (n=1; n<50; n++) {

        x1 = (x1>>1) ^ (x1>>4);
        x1 = x1 ^ (x1<<31) ^ (x1<<28);
        x2 = (x2>>1) ^ (x2>>2) ^ (x2>>3) ^ (x2>>4);
        x2 = x2 ^ (x2<<31) ^ (x2<<30) ^ (x2<<29) ^ (x2<<28);
        //  printf("x1 : %x, x2 : %x\n",x1,x2);

      }

      for (n=0; n<42; n++) {

        x1 = (x1>>1) ^ (x1>>4);
        x1 = x1 ^ (x1<<31) ^ (x1<<28);
        x2 = (x2>>1) ^ (x2>>2) ^ (x2>>3) ^ (x2>>4);
        x2 = x2 ^ (x2<<31) ^ (x2<<30) ^ (x2<<29) ^ (x2<<28);
        lte_gold_mbsfn_table[sfn][l][n] = x1^x2;
        //  printf("n=%d : c %x\n",n,x1^x2);
      }

    }

  }
}


#ifdef LTE_GOLD_MAIN
main()
{

  lte_gold_mbsfn(423,0);

}
#endif

