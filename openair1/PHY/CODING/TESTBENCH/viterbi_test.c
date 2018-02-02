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

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "PHY/CODING/defs.h"
#include "SIMULATION/TOOLS/defs.h"

#define INPUT_LENGTH 5
#define F1 3
#define F2 10

// 4-bit quantizer
char quantize4bit(double D,double x)
{

  double qxd;

  qxd = floor(x/D);
  //  printf("x=%f,qxd=%f\n",x,qxd);

  if (qxd <= -8)
    qxd = -8;
  else if (qxd > 7)
    qxd = 7;

  return((char)qxd);
}

// 3-bit quantizer
char quantize3bit(double D,double x)
{

  double qxd;

  qxd = floor(x/D);

  if (qxd <= -4)
    qxd = -4;
  else if (qxd > 3)
    qxd = 3;

  return((char)qxd);
}


#define MAX_BLOCK_LENGTH 6000
static char channel_output[2*MAX_BLOCK_LENGTH]__attribute__ ((aligned(16)));
static unsigned char decoded_output[MAX_BLOCK_LENGTH/8];

int test_viterbi(double sigma,
                 unsigned int block_length, //in bytes (including crc)
                 unsigned char crc_len, //in bytes
                 unsigned int ntrials,
                 unsigned int *errors,
                 unsigned int *crc_misses)
{

  unsigned char test_input[block_length+1];
  //_declspec(align(16))  char channel_output[512];
  //_declspec(align(16))  unsigned char output[512],decoded_output[16], *inPtr, *outPtr;

  //  char channel_output[(2*8*block_length)+12];
  unsigned char output[(2*8*block_length)+12],decoded_output[block_length];
  unsigned int i,trial=0;
  unsigned int crc=0, crc_rx=0, crc_dec=0;
  unsigned char ret;

  if (crc_len > 3) {
    printf("Illegal crc length!\n");
    return -1;
  }

  *errors=0;
  *crc_misses=0;

  //init viterbi
  ccodedot11_init();
  ccodedot11_init_inv();

  phy_generate_viterbi_tables();

  while (trial++ < ntrials) {

    for (i=0; i<block_length-crc_len; i++) {

      test_input[i] = (unsigned char)(taus()&0xff);
    }

    switch (crc_len) {

    case 1:
      crc = crc8(test_input,
                 (block_length-1)<<3)>>24;
      break;

    case 2:
      crc = crc16(test_input,
                  (block_length-2)<<3)>>16;
      break;

    case 3:
      crc = crc24(test_input,
                  (block_length-4)<<3)>>8;
      break;

    default:
      break;

    }

    if (crc_len > 0)
      *(unsigned int*)(&test_input[block_length-4]) = crc;


    // encode
    ccodedot11_encode(block_length,
                      test_input,
                      output,
                      0);

    // channel
    for (i = 0; i < (2*8*block_length); i++) {
      //    printf("output[%d]=%d\n",i,output[i]);
      channel_output[i] = quantize4bit(sigma/4.0,2.0*output[i] - 1.0 + sigma*gaussdouble(0.0,1.0));
      //    printf("Position %d : %d\n",i,channel_output[i]);
    }

    // decode
    memset(decoded_output,0,block_length);
    phy_viterbi_dot11_sse2(channel_output,decoded_output,8*block_length);


    // crc check
    switch (crc_len) {

    case 1:
      crc_rx = crc8(decoded_output,
                    (block_length-1)<<3)>>24;
      break;

    case 2:
      crc_rx = crc16(decoded_output,
                     (block_length-2)<<3)>>16;
      break;

    case 3:
      crc_rx = crc24(decoded_output,
                     (block_length-4)<<3)>>8;
      break;

    default:
      break;
    }

    crc_dec = *(unsigned int*)(&decoded_output[block_length-4]);
    crc_dec &= 0x00ffffff;

    ret = (crc_dec!=crc_rx);
    *crc_misses = *crc_misses+ret;

    /*
    if (ret)
      printf("trial %d, crc=%x, crc_dec=%x, crc_rx = %x\n",trial, crc,crc_dec,crc_rx);
    */

    for (i=0; i<block_length-1; i++)
      if (decoded_output[i] != test_input[i]) {
        //printf("Iteration %d, error at %d, %x != %x\n",trial,i,decoded_output[i],test_input[i]);
        *errors = (*errors) + 1;
        break;
      }


  }


  return(0);
}



#define NTRIALS 100000

int main(int argc, char *argv[])
{

  int ret;
  unsigned int errors,crc_misses;
  unsigned int block_length = 132;
  unsigned int crc_len = 3;
  double SNR; //= atof(argv[1]);
  double sigma; //= sqrt(1.0/pow(10,EbN0/10.0));

  //printf("EbN0 = %f, sigma = %f\n",EbN0,sigma);

  randominit();
  set_taus_seed();

  crcTableInit();

  printf("EbN0, sigma, BER, FER\n");

  for (SNR=0; SNR<10; SNR++) {

    sigma = sqrt(1.0/pow(10,SNR/10.0));

    ret = test_viterbi(sigma,   // noise standard deviation
                       block_length,   // block length bytes
                       crc_len,    // crc length (24-bit)
                       NTRIALS,
                       &errors,
                       &crc_misses);


    printf("%f, %f, %f, %f\n", SNR, sigma, ((double) errors)/(NTRIALS), (double)crc_misses/NTRIALS);

    //if (ret>=0)
    //  printf("Errors %d (%f), CRC Misses %d (%f)\n",errors,(double)errors/NTRIALS,crc_misses,(double)crc_misses/NTRIALS);

  }

  return(0);
}

