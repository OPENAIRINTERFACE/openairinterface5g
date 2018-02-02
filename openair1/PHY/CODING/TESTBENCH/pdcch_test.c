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

#include "PHY/defs.h"
#include "PHY/vars.h"
#include "ARCH/CBMIMO1/DEVICE_DRIVER/vars.h"
#include "MAC_INTERFACE/vars.h"
#include "PHY/CODING/defs.h"
//#include "PHY/CODING/lte_interleaver.h"
//#include "PHY/CODING/lte_interleaver_inline.h"


#include "SIMULATION/TOOLS/defs.h"
#include "PHY/LTE_TRANSPORT/defs.h"
//#include "PHY/LTE_TRANSPORT/dci.h"

//#include "decoder.h"

char current_dlsch_cqi;

void lte_param_init(unsigned char N_tx, unsigned char N_rx)
{

  printf("Start lte_param_init\n");
  PHY_vars = malloc(sizeof(PHY_VARS));
  PHY_config = malloc(sizeof(PHY_CONFIG));
  mac_xface = malloc(sizeof(MAC_xface));

  randominit(0);
  set_taus_seed(0);

  crcTableInit();

  lte_frame_parms = &(PHY_config->lte_frame_parms);   //openair1/PHY/impl_defs_lte.h
  lte_ue_common_vars = &(PHY_vars->lte_ue_common_vars);
  lte_ue_dlsch_vars = &(PHY_vars->lte_ue_dlsch_vars);
  lte_ue_pbch_vars = &(PHY_vars->lte_ue_pbch_vars);
  lte_ue_pdcch_vars = &(PHY_vars->lte_ue_pdcch_vars);
  lte_ue_pbch_vars = &(PHY_vars->lte_ue_pbch_vars[0]);
  lte_ue_dlsch_vars_cntl = &PHY_vars->lte_ue_dlsch_vars_cntl[0];
  lte_ue_dlsch_vars_ra   = &PHY_vars->lte_ue_dlsch_vars_ra[0];
  lte_ue_dlsch_vars_1A   = &PHY_vars->lte_ue_dlsch_vars_1A[0];

  lte_frame_parms->N_RB_DL            = 25;   //50 for 10MHz and 25 for 5 MHz
  lte_frame_parms->Ncp                = 1;
  lte_frame_parms->Nid_cell           = 0;
  lte_frame_parms->nushift            = 1;
  lte_frame_parms->nb_antennas_tx     = N_tx;
  lte_frame_parms->nb_antennas_rx     = N_rx;
  init_frame_parms(lte_frame_parms);

  copy_lte_parms_to_phy_framing(lte_frame_parms, &(PHY_config->PHY_framing));

  phy_init_top(N_tx); //allocation

  lte_frame_parms->twiddle_fft      = twiddle_fft;
  lte_frame_parms->twiddle_ifft     = twiddle_ifft;
  lte_frame_parms->rev              = rev;

  generate_64qam_table();
  generate_16qam_table();
  phy_init_lte_ue(lte_frame_parms,lte_ue_common_vars,lte_ue_dlsch_vars,lte_ue_dlsch_vars_cntl,lte_ue_dlsch_vars_ra,lte_ue_dlsch_vars_1A,lte_ue_pbch_vars,lte_ue_pdcch_vars);//allocation
  printf("Done lte_param_init\n");
}


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

char quantize(double D,double x,unsigned char B)
{

  double qxd;
  char maxlev;

  qxd = floor(x/D);
  //  printf("x=%f,qxd=%f\n",x,qxd);

  maxlev = 1<<(B-1);

  if (qxd <= -maxlev)
    qxd = -maxlev;
  else if (qxd >= maxlev)
    qxd = maxlev-1;

  return((char)qxd);
}

#define MAX_BLOCK_LENGTH 6000
static char channel_output[3*(MAX_DCI_SIZE_BITS+8)]__attribute__ ((aligned(16)));
static unsigned char decoded_output[1+(MAX_BLOCK_LENGTH>>3)];

int test_pdcch(double sigma,
               unsigned int DCI_LENGTH,
               unsigned int ntrials,
               unsigned int *errors,
               unsigned int *crc_misses,
               unsigned int *trials,
               unsigned int DCI_FMT)
{

  unsigned int i,n,coded_bits;
  unsigned char e[576];
  char e_rx[576] __attribute__ ((aligned(16)));
  unsigned char a[MAX_DCI_SIZE_BITS>>3];
  unsigned int crc,crce;

  memset(a,0,MAX_DCI_SIZE_BITS>>3);

  for (i=0; i<(DCI_LENGTH>>3); i++)
    a[i] = i;


  switch (DCI_FMT) {
  case 0:
    coded_bits = 72;
    break;

  case 1:
    coded_bits = 144;
    break;

  case 2:
    coded_bits = 288;
    break;

  case 3:
    coded_bits = 576;
    break;

  default:
    coded_bits = 72;
    break;
  }

  for (n=0; n<ntrials; n++) {


    dci_encoding(a,DCI_LENGTH,coded_bits,(unsigned char*)&e[0],0);

    for (i = 0; i < coded_bits; i++) {
      //      printf("e[%d] = %d -> ",i,e[i]);
      if (e[i] == 0)
        e_rx[i] = quantize4bit(sigma/4.0,(sqrt(2.0)*sigma*gaussdouble(0.0,1.0))- 1);
      else
        e_rx[i] = quantize4bit(sigma/4.0,(sqrt(2.0)*sigma*gaussdouble(0.0,1.0))+ 1);

      //      printf("e[%d] = %d\n",i,e_rx[i]);
    }

    // now do decoding

    dci_decoding(DCI_LENGTH,DCI_FMT,&e_rx[0],decoded_output);
    crce = extract_crc(decoded_output,DCI_LENGTH);
    crc = crc16(decoded_output,DCI_LENGTH);
    // check for errors
    /*printf("DCI_LEN %d : %x,%x\n",DCI_LENGTH,
     extract_crc(decoded_output,DCI_LENGTH),
     crc);
    */

    for (i=0; i<(((DCI_LENGTH>>3))); i++) {
      //      printf("decoded output %d -> %x\n",i,decoded_output[i]);
      if (decoded_output[i]!=i) {
        *errors = *errors+1;

        if ((extract_crc(decoded_output,DCI_LENGTH) ^ (crc16(decoded_output,DCI_LENGTH)>>16)) == 0) {
          *crc_misses = *crc_misses+1;
          printf("%x %x => %x\n",extract_crc(decoded_output,DCI_LENGTH),(crc16(decoded_output,DCI_LENGTH)>>16),
                 (extract_crc(decoded_output,DCI_LENGTH) ^ (crc16(decoded_output,DCI_LENGTH)>>16)));
        }

        break;
      }
    }

    //exit(-1);
    if (*errors == 100)
      break;
  }

  *trials = n;


  return(0);

}

#define NTRIALS 1000000

int main(int argc, char *argv[])
{

  int ret,ret2;
  unsigned int errors,crc_misses,trials;
  double SNR,sigma,rate;
  unsigned char qbits;

  char done=0;


  unsigned short iind;
  unsigned int coded_bits;

  unsigned char DCI_FMT,DCI_LENGTH;

  PHY_config = malloc(sizeof(PHY_CONFIG));
  PHY_vars = malloc(sizeof(PHY_VARS));
  mac_xface = malloc(sizeof(MAC_xface));


  randominit(0);
  set_taus_seed(0);

  crcTableInit();
  ccodelte_init();
  ccodelte_init_inv();

  phy_generate_viterbi_tables_lte();
  lte_frame_parms = &(PHY_config->lte_frame_parms);
  lte_frame_parms->N_RB_DL            = 25;   //50 for 10MHz and 25 for 5 MHz
  lte_frame_parms->Ncp                = 1;
  lte_frame_parms->Nid_cell           = 0;
  lte_frame_parms->nushift            = 1;
  lte_frame_parms->nb_antennas_tx     = 2;
  lte_frame_parms->nb_antennas_rx     = 2;
  init_frame_parms(lte_frame_parms);

  copy_lte_parms_to_phy_framing(lte_frame_parms, &(PHY_config->PHY_framing));
  lte_param_init(2,2);

  phy_init_top(2); //allocation

  if (argc>1)
    DCI_FMT = atof(argv[1]);
  else
    DCI_FMT = 2;

  printf("DCI_FMT %d\n",DCI_FMT);


  if (argc>2)
    DCI_LENGTH = atoi(argv[2]);
  else
    DCI_LENGTH = sizeof_DCI0_5MHz_TDD_0_t;



  for (SNR=-2; SNR<4; SNR+=.2) {


    printf("\n\nSNR %f dB\n",SNR);

    sigma = pow(10.0,-.05*SNR);

    errors=0;
    crc_misses=0;

    if (done == 0) {

      printf("PDCCH %d\n",DCI_LENGTH);

      ret = test_pdcch(sigma,   // noise standard deviation
                       DCI_LENGTH,
                       NTRIALS,
                       &errors,
                       &crc_misses,
                       &trials,
                       DCI_FMT);

      if (ret>=0)
        printf("ref: Errors %d (%f), CRC Misses %d (%f)\n",errors,(double)errors/trials,crc_misses,(double)crc_misses/trials);

      if (((double)errors/trials) < 1e-3)
        done=1;
    }

    if (done==1) {
      printf("done\n");
      break;
    }
  }

  return(0);
}
