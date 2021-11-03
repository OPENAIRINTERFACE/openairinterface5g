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

#include "PHY/types.h"

/**
   \brief Gold Sequence Generation defined in 3x.211
   \param x1 x1 shift register
   \param x2 x2 shift register / cinit if reset is set
   \param reset Reset flag / reinitialize the generator
   \return c 32 bits of gold output
*/
extern inline  uint32_t lte_gold_generic(uint32_t *x1, uint32_t *x2, uint8_t reset)
{
  int32_t n;

  // 3GPP 3x.211
  // Nc = 1600
  // c(n)     = [x1(n+Nc) + x2(n+Nc)]mod2
  // x1(n+31) = [x1(n+3)                     + x1(n)]mod2
  // x2(n+31) = [x2(n+3) + x2(n+2) + x2(n+1) + x2(n)]mod2
  if (reset)
  {
      // Init value for x1: x1(0) = 1, x1(n) = 0, n=1,2,...,30
      // x1(31) = [x1(3) + x1(0)]mod2 = 1
      *x1 = 1 + (1U<<31);
      // Init value for x2: cinit = sum_{i=0}^30 x2*2^i
      // x2(31) = [x2(3)    + x2(2)    + x2(1)    + x2(0)]mod2
      //        =  (*x2>>3) ^ (*x2>>2) + (*x2>>1) + *x2
      *x2 = *x2 ^ ((*x2 ^ (*x2>>1) ^ (*x2>>2) ^ (*x2>>3))<<31);

      // x1 and x2 contain bits n = 0,1,...,31

      // Nc = 1600 bits are skipped at the beginning
      // i.e., 1600 / 32 = 50 32bit words

      for (n = 1; n < 50; n++)
      {
          // Compute x1(0),...,x1(27)
          *x1 = (*x1>>1) ^ (*x1>>4);
          // Compute x1(28),..,x1(31) and xor
          *x1 = *x1 ^ (*x1<<31) ^ (*x1<<28);
          // Compute x2(0),...,x2(27)
          *x2 = (*x2>>1) ^ (*x2>>2) ^ (*x2>>3) ^ (*x2>>4);
          // Compute x2(28),..,x2(31) and xor
          *x2 = *x2 ^ (*x2<<31) ^ (*x2<<30) ^ (*x2<<29) ^ (*x2<<28);
      }
  }

  *x1 = (*x1>>1) ^ (*x1>>4);
  *x1 = *x1 ^ (*x1<<31) ^ (*x1<<28);
  *x2 = (*x2>>1) ^ (*x2>>2) ^ (*x2>>3) ^ (*x2>>4);
  *x2 = *x2 ^ (*x2<<31) ^ (*x2<<30) ^ (*x2<<29) ^ (*x2<<28);
  //printf("printing c= %d \n",*x1^*x2);
  // c(n) = [x1(n+Nc) + x2(n+Nc)]mod2
  return(*x1^*x2);
}
