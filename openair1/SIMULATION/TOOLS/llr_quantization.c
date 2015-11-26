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
//#include <string.h>
#include <math.h>
//#include <unistd.h>
//#include <stdlib.h>
#include "PHY/TOOLS/defs.h"

//#define TEST_DEBUG

short max_array(const short *p, unsigned int size)
{
  short max = *p;
  unsigned int i;

  for (i = 1; i < size; ++i)
    if (max < p[i])
      max = p[i];

  return max;
}


short min_array(const short *p, unsigned int size)
{
  short min = *p;
  unsigned int i;

  for (i = 1; i < size; ++i)
    if (min > p[i])
      min = p[i];

  return min;
}


// Note that 'arraySize =  8*((3*8*6144)+12)';
void dlsch_LLR_quant(const short *llr, const unsigned int arraySize, const short Mlevel, short *llr_quant)
{

  unsigned int i, j;
  short max_llr, min_llr;
  short llr_interval;
  float quant_step;
  float *transLevelArray;

  min_llr = min_array(llr, arraySize);
  max_llr = max_array(llr, arraySize);
  llr_interval = (max_llr - min_llr);
  quant_step = (float)llr_interval / Mlevel;


  if ((Mlevel%2) != 0) {
    printf("Mlevel should be mutiple of 2...\n");
    exit(-1);
  }

  transLevelArray = (float *)malloc((Mlevel+1)*sizeof(float));

  if (!transLevelArray) {
    printf("Cannot allocate memory for transLevelArray...!\n");
    exit(-1);
  }

  for(j=0; j < Mlevel+1; j++) {
    transLevelArray[j] = min_llr + j*quant_step;
  }

  for (i=0; i < arraySize; i++) {
    for(j=0; j < Mlevel; j++) {
      if ((transLevelArray[j] <= llr[i]) && (llr[i] <= transLevelArray[j+1])) {
        llr_quant[i] = (short)(transLevelArray[j] +  quant_step/2);  // mid-points are selected;
        break;
      }

      if (transLevelArray[j+1] <= llr[i]) { // the last term;
        llr_quant[i] = (short)(transLevelArray[j] +  quant_step/2);  // mid-points are selected;
      }
    }
  }

#ifdef TEST_DEBUG
  printf("min_llr: %d  : max_llr: %d \n", min_llr, max_llr);
  printf("llr_interval: %d  : quant_step: %f \n", llr_interval, quant_step);
  printf("transLevelArray = [");

  for(j=0; j < Mlevel+1; j++)
    printf("%f ", transLevelArray[j]);

  printf("] \n\n");
#endif // TEST_DEBUG  

}

/*

void dlsch_LLR_quant2(const short *llr, const unsigned int arraySize, const short bitsPerLLR, short *llr_quant)
{

  unsigned int i;
  short max_llr, min_llr;
  short peak_llr;
  unsigned char shift;

  min_llr = min_array(llr, arraySize);
  max_llr = max_array(llr, arraySize);

  peak_llr = max(abs(max_llr), abs(min_llr));
  shift = log2_approx(peak_llr);

  if (bitsPerLLR < 1){
    printf("Bits per LLR should be greater than 0\n");
    exit(-1);
  }

  for (i=0; i < 6072; i++){
    llr_quant[i] = llr[i] >> max(0, shift-bitsPerLLR);  // quantize the LLRs;
    llr_quant[i] <<= max(0, shift-bitsPerLLR);          // reconstruct the LLRs;
    printf("[%d] llr: %d : llr_quant %d \n",i, llr[i], llr_quant[i]);
  }
  exit(-1);
}


void clearbits(short *llr, const unsigned short startbit, const unsigned short nbits)
{

  unsigned short i;

  for (i=0; i < nbits; i++){
    *llr &= ~(1 << (startbit + i));
  }
}

void dlsch_LLR_quant3(const short *llr, const unsigned int arraySize, const short bitsPerLLR, short *llr_quant)
{

  unsigned int i;
  unsigned char shift;
  unsigned int abs_mean_llr = 0;

  //mean of the LLRs;
  for(i=0; i < 6072; i++){
    abs_mean_llr += abs(llr[i]);
  }
  abs_mean_llr /= 6072;

  shift = log2_approx(abs_mean_llr);

  printf("abs_mean_llr=%d, shift = %d\n", abs_mean_llr, shift);
  exit(-1);

  for (i=0; i < 6072; i++){
    llr_quant[i] = llr[i] >> max(0, shift-bitsPerLLR);  // quantize the LLRs;
    printf("[%d] llr: %d : ", i, llr[i]);
    //mask the llr_quant values such that their values do not exceed 2^(bitsPerLLR);
    clearbits(&llr_quant[i], bitsPerLLR, sizeof(short)-bitsPerLLR-1);
    printf("llr_quant_mask: %d : ", llr_quant[i]);
    llr_quant[i] <<= max(0, shift-bitsPerLLR);          // reconstruct the LLRs;
    printf("llr_quant: %d \n", llr_quant[i]);

  }
}

*/


void dlsch_MRC_relay_LLR(const short *llr_quant, const unsigned int arraySize,  short *llr_quant_sum)
{

  unsigned int i;

  for (i=0; i < arraySize; i++) {
    llr_quant_sum[i] += llr_quant[i];
  }
}



#ifdef TEST_DEBUG

void test_llr_quant()
{
  short test[10] = {-1800, -5446, 345, 243, 130, -2111, 433, 4210, -10, -134};
  short channel_output[10];

  unsigned int i;


  dlsch_LLR_quant(test, 10, 2, channel_output);

  for (i = 0; i < 10; i++) {
    printf("llr: %d : llr_quant %d \n",test[i], channel_output[i]);
  }
}


void main()
{

  test_llr_quant();
}

#endif // TEST_DEBUG






