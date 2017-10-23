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

#ifdef DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#endif

#include "PHY/defs.h"

//#define DEBUG_PHY 1

unsigned int ps0, ps1, ps2, pb;

//----------------------------------------------
//
/*!\brief Tausworthe Uniform Random Generator.  This is based on the hardware implementation described in
  Lee et al, "A Hardware Gaussian Noise Generator Usign the Box-Muller Method and its Error Analysis," IEEE Trans. on Computers, 2006.
*/
//

static inline void pset_taus_seed(unsigned int off)
{


  ps0 = (unsigned int)0x1e23d852 + (off<<4);
  ps1 = (unsigned int)0x81f38a1c + (off<<4);
  ps2 = (unsigned int)0xfe1a133e + (off<<4);

}

static inline unsigned int ptaus(void)
{

  pb = (((ps0 << 13) ^ ps0) >> 19);
  ps0 = (((ps0 & 0xFFFFFFFE) << 12)^  pb);
  pb = (((ps1 << 2) ^ ps1) >> 25);
  ps1 = (((ps1 & 0xFFFFFFF8) << 4)^  pb);
  pb = (((ps2 << 3) ^ ps2) >> 11);
  ps2 = (((ps2 & 0xFFFFFFF0) << 17)^  pb);
  return ps0 ^ ps1 ^ ps2;
}


int rate_matching(unsigned int N_coded,
                  unsigned int N_input,
                  unsigned char *inPtr,
                  unsigned char N_bps,
                  unsigned int off)
{


  unsigned int i,j,U,Umod,rep_flag=0;
  unsigned int N_coded2,N_input2,Mod_input2;
  int N_punctured;

  if ((9*N_coded<(N_input<<2)) || (N_input<(N_coded>>1))) {
    // check validity of parameters
    // the first condition represents the case where the rate is greater than 2/3
    // the second condition represents the case where the rate is less than 1/4
    msg("[PHY][CODING] Rate matching parameter error (N_coded %d, N_input %d, N_bps %d), exiting\n",N_coded,N_input,N_bps);
    return(-1);
  }

  //initialize all bits as transmitted
  for (i=0; i<N_input; i++)
    inPtr[i] |= 0x80;

  // No go puncture the bits that should be removed
  i=0;

  N_coded2 = N_coded/N_bps;
  pset_taus_seed(off);
  N_input2 = N_input/N_bps;

  N_punctured = N_input2-N_coded2;

  if (N_punctured < 0) {  // Repeat bits instead of puncturing
    N_punctured = -N_punctured;
    rep_flag = 1;
  }

#ifdef USER_MODE
#ifdef DEBUG_PHY
  printf("rate_matching : N_coded %d, N_input %d, N_bps %d\n",N_coded,N_input,N_bps);
  printf("rate_matching : N_punctured = %d (%d)\n",N_bps*N_punctured,rep_flag);
  printf("rate_matching : code rate = %f\n",(double)N_input/(double)N_coded/2.0);
#endif
#endif

  Mod_input2 = (1<<(1+log2_approx(N_input2)))-1;

  while (i < N_punctured) {
    // generate N_punctured random positions in the input vector
    do { // generate a modulo-N_input2 random variable
      U = ptaus();
      Umod = U&Mod_input2;

      //      printf("i %d, N_punctured %d U %u Umod %d Iptr[Umod] %x\n",i,N_punctured,U,Umod,inPtr[Umod]);
      // check if the bit is already punctured/repeated, if so skip it and generate another RV
      if ((Umod < N_input2) && (((inPtr[Umod]&0x80) == 0)||((inPtr[Umod]&0xc0) == 0xc0)))
        Umod=N_input2;

    } while (Umod>=N_input2);

    for (j=0; j<N_bps; j++) {          // loop over symbol bits
      if (rep_flag == 0)
        inPtr[Umod + (j*N_input2)] &= 0x7f; // clear MSB to indicate bit is punctured
      else
        inPtr[Umod + (j*N_input2)] |= 0xc0; // set MSB-1 to indicate bit is repeated

    }

    /*
    #ifdef USER_MODE
    #ifdef DEBUG_PHY
    printf("rate_matching : i= %d, U = %d\n",i,Umod);
    #endif
    #endif
    */
    i++;
  }

  return(0);
}

int rate_matching_lte(unsigned int N_coded,
                      unsigned int N_input,
                      unsigned char *inPtr,
                      unsigned int off)
{


  unsigned int i,U,Umod,rep_flag=0;
  unsigned int Mod_input;
  int N_punctured;

  if ((9*N_coded<(N_input<<2)) || (N_input<(N_coded>>1))) {
    // check validity of parameters
    // the first condition represents the case where the rate is greater than 2/3
    // the second condition represents the case where the rate is less than 1/4
    msg("[PHY][CODING] Rate matching parameter error (N_coded %d, N_input %d), exiting\n",N_coded,N_input);
    return(-1);
  }

  //initialize all bits as transmitted
  for (i=0; i<N_input; i++)
    inPtr[i] |= 0x80;

  // No go puncture the bits that should be removed
  i=0;


  pset_taus_seed(off);

  N_punctured = N_input-N_coded;

  if (N_punctured < 0) {  // Repeat bits instead of puncturing
    N_punctured = -N_punctured;
    rep_flag = 1;
  }

#ifdef USER_MODE
#ifdef DEBUG_PHY
  printf("rate_matching : N_coded %d, N_input %d\n",N_coded,N_input);

  if (rep_flag)
    printf("rate_matching : N_repeated = %d\n",N_punctured);
  else
    printf("rate_matching : N_punctured = %d\n",N_punctured);

  printf("rate_matching : code rate = %f\n",(double)N_input/(double)N_coded/3.0);
#endif
#endif

  Mod_input = (1<<(1+log2_approx(N_input)))-1;

  while (i < N_punctured) {
    // generate N_punctured random positions in the input vector
    do { // generate a modulo-N_input2 random variable
      U = ptaus();
      Umod = U&Mod_input;

      //      printf("i %d, N_punctured %d U %u Umod %d Iptr[Umod] %x\n",i,N_punctured,U,Umod,inPtr[Umod]);
      // check if the bit is already punctured/repeated, if so skip it and generate another RV
      if ((Umod < N_input) && (((inPtr[Umod]&0x80) == 0)||((inPtr[Umod]&0xc0) == 0xc0)))
        Umod=N_input;

    } while (Umod>=N_input);

    if (rep_flag == 0)
      inPtr[Umod] &= 0x7f; // clear MSB to indicate bit is punctured
    else
      inPtr[Umod] |= 0xc0; // set MSB-1 to indicate bit is repeated


    /*
    #ifdef USER_MODE
    #ifdef DEBUG_PHY
    printf("rate_matching : i= %d, U = %d\n",i,Umod);
    #endif
    #endif
    */
    i++;
  }

  return(0);
}



#ifdef MAIN
void main(int argc,char **argv)
{

  unsigned char input[1024];

  if (argc == 1) {
    printf("Provide an input\n");
    exit(-1);
  }

  memset(input,0,1024);

  rate_matching(4*157,4*256,input,atoi(argv[1]),0);
}
#endif
