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

//#define DEBUG_DAC 1
#include <math.h>
#include <stdio.h>
#include "PHY/TOOLS/tools_defs.h"
#include "rf.h"
void dac(double *s_re[2],
         double *s_im[2],
         int32_t **input,
         uint32_t input_offset,
         uint32_t nb_tx_antennas,
         uint32_t length,
         double amp_dBm,
         uint8_t B,
         uint32_t meas_length,
         uint32_t meas_offset)
{

  int i;
  int aa;
  double V=0.0,amp;

  for (i=0; i<length; i++) {
    for (aa=0; aa<nb_tx_antennas; aa++) {
      s_re[aa][i] = ((double)(((short *)input[aa]))[((i+input_offset)<<1)])/(1<<(B-1));
      s_im[aa][i] = ((double)(((short *)input[aa]))[((i+input_offset)<<1)+1])/(1<<(B-1));

    }
  }

  for (i=meas_offset; i<meas_offset+meas_length; i++) {
    for (aa=0; aa<nb_tx_antennas; aa++) {
      V= V + (s_re[aa][i]*s_re[aa][i]) + (s_im[aa][i]*s_im[aa][i]);
    }
  }

  V /= (meas_length);
#ifdef DEBUG_DAC
  printf("DAC: 10*log10(V)=%f (%f)\n",10*log10(V),V);
#endif

  if (V) {
    amp = pow(10.0,.1*amp_dBm)/V;
    amp = sqrt(amp);
  } else {
    amp = 1;
  }

  for (i=0; i<length; i++) {
    for (aa=0; aa<nb_tx_antennas; aa++) {
      s_re[aa][i] *= amp;
      s_im[aa][i] *= amp;
    }
  }
}

double dac_fixed_gain(double *s_re[2],
                      double *s_im[2],
                      int32_t **input,
                      uint32_t input_offset,
                      uint32_t nb_tx_antennas,
                      uint32_t length,
                      uint32_t input_offset_meas,
                      uint32_t length_meas,
                      uint8_t B,
                      double txpwr_dBm,
		      uint8_t do_amp_compute,
		      double *amp1,
                      int NB_RE)
{

  int i;
  int aa;
  double amp1_local,*amp1p;
  double amp = pow(10.0,.05*txpwr_dBm)/sqrt(nb_tx_antennas); //this is amp per tx antenna

  if (amp1==NULL) amp1p = &amp1_local;
  else            amp1p = amp1;

  if (do_amp_compute==1) {
    *amp1p = 0;
    for (aa=0; aa<nb_tx_antennas; aa++) {
      *amp1p += (double)signal_energy((int32_t*)&input[aa][input_offset_meas],length_meas)/NB_RE;
    }
    *amp1p/=nb_tx_antennas;
    *amp1p=sqrt(*amp1p);
  }



  //  printf("DAC: amp %f, amp1 %f dB (%d,%d), tx_power %f (%f),length %d,pos %d\n",20*log10(amp),20*log10(*amp1p),input_offset,input_offset_meas,txpwr_dBm,length, 10*log10((double)signal_energy((int32_t*)&input[0][input_offset_meas],length_meas)/NB_RE),input_offset_meas);

  /*
    if (nb_tx_antennas==2)
      amp1 = AMP/2;
    else if (nb_tx_antennas==4)
      amp1 = ((AMP*ONE_OVER_SQRT2_Q15)>>16);
    else //assume (nb_tx_antennas==1)
      amp1 = ((AMP*ONE_OVER_SQRT2_Q15)>>15);
    amp1 = amp1*sqrt(512.0/300.0); //account for loss due to null carriers
    //printf("DL: amp1 %f dB (%d,%d), tx_power %f\n",20*log10(amp1),input_offset,input_offset_meas,txpwr_dBm);
  */

  //  printf("DAC: amp/amp1p %f amp1 %f dB (%d,%d), tx_power %f\n",amp/(*amp1p),20*log10(*amp1p),input_offset,input_offset_meas,txpwr_dBm);
  for (i=0; i<length; i++) {
    for (aa=0; aa<nb_tx_antennas; aa++) {
      s_re[aa][i] = amp*((double)(((short *)input[aa]))[((i+input_offset)<<1)])/(*amp1p); 
      s_im[aa][i] = amp*((double)(((short *)input[aa]))[((i+input_offset)<<1)+1])/(*amp1p); 
    }
  }

  //  printf("ener %e\n",signal_energy_fp(s_re,s_im,nb_tx_antennas,length<length_meas?length:length_meas,0));

  return(signal_energy_fp(s_re,s_im,nb_tx_antennas,length<length_meas?length:length_meas,0)/NB_RE);
}
