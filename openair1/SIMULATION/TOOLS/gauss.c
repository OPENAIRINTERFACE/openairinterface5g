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

#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include "defs.h"


unsigned int *generate_gauss_LUT(unsigned char Nbits,
                                 unsigned char L
                                ) {
  unsigned int *LUT_ptr,i;
  LUT_ptr = (unsigned int *)malloc((1<<(Nbits-1))*sizeof(int));
  assert(LUT_ptr);

  for (i=0; i<(1<<(Nbits-1)); i++) {
    LUT_ptr[i] = (unsigned int)((double)((unsigned int)(1<<31))*erf(i*L/(double)(1<<(Nbits-1))));
#ifdef LUTDEBUG
    printf("pos %u : LUT_ptr[%u]=%x (%f)\n",i,i,LUT_ptr[i],(double)(erf(i*L/(double)(1<<(Nbits-1)))));
#endif //LUTDEBUG
  }

  return(LUT_ptr);
}



int gauss(unsigned int *gauss_LUT,
          unsigned char Nbits
         ) {
  unsigned int search_pos,step_size,u,tmp,tmpm1,tmpp1,s;
  // Get a 32-bit uniform random-variable
  u = taus();
#ifdef DEBUG
  printf("u = %u\n",u);
#endif //DEBUG
  // if it is larger than 2^31 (here negative), save the sign and rescale down to 31-bits.
  s = u & 0x80000000;
  u &= 0x7fffffff;
#ifdef DEBUG
  printf("u = %x,s=%u\n",u,s);
#endif //DEBUG
  search_pos = (1<<(Nbits-2));   // starting position of the binary search
  step_size  = search_pos;

  do {
    step_size >>= 1;
    tmp = gauss_LUT[search_pos];
    tmpm1 = gauss_LUT[search_pos-1];
    tmpp1 = gauss_LUT[search_pos+1];
#ifdef DEBUG
    printf("search_pos %u, step_size %u: t %x tm %x,tp %x\n",search_pos,step_size,tmp,tmpm1,tmpp1);
#endif //DEBUG

    if (u <= tmp)
      if (u >tmpm1)
        return s==0 ? (search_pos-1) : 1-search_pos;
      else
        search_pos -= step_size;
    else if (u <= tmpp1)
      return s==0 ? search_pos : - search_pos;
    else
      search_pos += step_size;
  } while (step_size > 0);

  // If it gets here we're beyond the positive edge  so return max
  return s==0 ? (1<<(Nbits-1))-1 : 1-((1<<(Nbits-1)));
}


#ifdef GAUSSMAIN

#define Nhistbits 8

void main(int argc,char **argv) {
  unsigned int *gauss_LUT_ptr,i;
  unsigned int hist[(1<<Nhistbits)];
  int gvar,maxg=0,ming=9999999,maxnum=0,L,Ntrials,Nbits;
  double meang=0.0,varg=0.0;

  if (argc < 4) {
    printf("Not enough arguments: %s Nbits L Ntrials\n",argv[0]);
    exit(-1);
  }

  Nbits   = atoi(argv[1]);
  L       = atoi(argv[2]);
  Ntrials = atoi(argv[3]);
  set_taus_seed();
  // Generate Gaussian LUT 12-bit quantization over 5 standard deviations
  gauss_LUT_ptr = generate_gauss_LUT(Nbits,L);

  for (i=0; i<(1<<Nhistbits); i++)
    hist[i] = 0;

  for (i=0; i<Ntrials; i++) {
    gvar = gauss(gauss_LUT_ptr,Nbits);

    if (gvar == ((1<<(Nbits-1))-1))
      maxnum++;

    maxg = gvar > maxg ? gvar : maxg;
    ming = gvar < ming ? gvar : ming;
    meang += (double)gvar/Ntrials;
    varg += (double)gvar*gvar/Ntrials;
    gvar += (1<<(Nbits-1))-1;
    hist[(gvar/(1<<(Nbits-Nhistbits)))]++;
    //    printf("%d\n",gauss(gauss_LUT_ptr,Nbits));
  }

  printf("Tail probability = %e(%x)\n",2*erfc((double)L*gauss_LUT_ptr[(1<<(Nbits-1))-1]/(unsigned int)(1<<31)),gauss_LUT_ptr[(1<<(Nbits-1))-1]);
  printf("max %d, min %d, mean %f, stddev %f, Pr(maxnum)=%e(%d)\n",maxg,ming,meang,sqrt(varg),(double)maxnum/Ntrials,maxnum);
  //  for (i=0;i<(1<<Nhistbits);i++)
  //    printf("%d : %u\n",i,hist[i]);
  free(gauss_LUT_ptr);
}

#endif //GAUSSMAIN
