#include <stdio.h>
#include <immintrin.h>
//#include "../nrLDPCdecoder_defs.h"
#include "../nrLDPC_types.h"
#include "../nrLDPC_init.h"
//#include "../nrLDPC_mPass.h"
//#include "../nrLDPC_cnProc.h"
#include "../nrLDPC_bnProc.h"
#include "cnProc_gen.h"

int main(int argc, char *argv [])
{
  //short lift_size[51]= {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,18,20,22,24,26,28,30,32,36,40,44,48,52,56,60,64,72,80,88,96,104,112,120,128,144,160,176,192,208,224,240,256,288,320,352,384};
//  unsigned int errors, errors_bit, crc_misses;
 // double errors_bit_uncoded;
  //short block_length=8448; // decoder supports length: 1201 -> 1280, 2401 -> 2560

 // short No_iteration=5;
  //int n_segments=1;
  //double rate=0.333;
  //int nom_rate=1;
  //int denom_rate=3;
  //double SNR0=-2.0,SNR,SNR_lin;
  //unsigned char qbits=8;
 // unsigned int decoded_errors[10000]; // initiate the size of matrix equivalent to size of SNR
  //int c,i=0, i1 = 0;

 // int n_trials = 1;
 // double SNR_step = 0.1;

//  randominit(0);
  //int test_uncoded= 0;

  //short BG=1,Zc,Kb;


//  cpu_freq_GHz = get_cpu_freq_GHz();
  //printf("the decoder supports BG2, Kb=10, Z=128 & 256\n");
  //printf(" range of blocklength: 1201 -> 1280, 2401 -> 2560\n");
 // printf("block length %d: \n", block_length);
  //printf("n_trials %d: \n", n_trials);
 // printf("SNR0 %f: \n", SNR0);

  //find minimum value in all sets of lifting size
 /* Zc=0;

  for (i1=0; i1 < 51; i1++)
  {
    if (lift_size[i1] >= (double) block_length/Kb)
    {
      Zc = lift_size[i1];
      //printf("%d\n",Zc);
      break;
    }
  }*/
  // Allocate LDPC decoder buffers
 // p_nrLDPC_procBuf = nrLDPC_init_mem();

//  load_nrLDPClib();


//  load_nrLDPClib_ref("_orig", &encoder_orig);

  // Z=384, R=1/3
  nrLDPC_cnProc_BG1_generator(384,0);
	//nrLDPC_cnProc_BG1(&lut_numCnInCnGroups, &cnProcBuf, 380);
  //for (block_length=8;block_length<=MAX_BLOCK_LENGTH;block_length+=8)

  //determine number of bits in codeword
/*
  char fname[200];
  sprintf(fname,"cnProc_BG1_Zc_%d.c",384);
  FILE *fd=fopen(fname,"w");
//  AssertFatal(fd!=NULL,"cannot open %s\n",fname);
*/
  //fclose(fd);

  return(0);
}
