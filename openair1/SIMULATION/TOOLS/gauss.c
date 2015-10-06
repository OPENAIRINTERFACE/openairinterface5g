/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include "defs.h"


unsigned int *generate_gauss_LUT(unsigned char Nbits,
                                 unsigned char L
                                )
{

  unsigned int *LUT_ptr,i;

  LUT_ptr = (unsigned int *)malloc((1<<(Nbits-1))*sizeof(int));
  assert(LUT_ptr);


  for (i=0; i<(1<<(Nbits-1)); i++) {
    LUT_ptr[i] = (unsigned int)((double)((unsigned int)(1<<31))*erf(i*L/(double)(1<<(Nbits-1))));

#ifdef LUTDEBUG
    printf("pos %d : LUT_ptr[%d]=%x (%f)\n",i,i,LUT_ptr[i],(double)(erf(i*L/(double)(1<<(Nbits-1)))));
#endif //LUTDEBUG
  }

  return(LUT_ptr);
}



int gauss(unsigned int *gauss_LUT,
          unsigned char Nbits
         )
{

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
  printf("u = %x,s=%d\n",u,s);
#endif //DEBUG

  search_pos = (1<<(Nbits-2));   // starting position of the binary search
  step_size  = search_pos;

  do {

    step_size >>= 1;

    tmp = gauss_LUT[search_pos];
    tmpm1 = gauss_LUT[search_pos-1];
    tmpp1 = gauss_LUT[search_pos+1];
#ifdef DEBUG
    printf("search_pos %d, step_size %d: t %x tm %x,tp %x\n",search_pos,step_size,tmp,tmpm1,tmpp1);
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

void main(int argc,char **argv)
{

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
