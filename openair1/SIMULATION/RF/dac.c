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
//#define DEBUG_DAC 1
#include <math.h>
#include <stdio.h>
#include "PHY/TOOLS/defs.h"

void dac(double **s_re,
         double **s_im,
         uint32_t **input,
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

double dac_fixed_gain(double **s_re,
                      double **s_im,
                      uint32_t **input,
                      uint32_t input_offset,
                      uint32_t nb_tx_antennas,
                      uint32_t length,
                      uint32_t input_offset_meas,
                      uint32_t length_meas,
                      uint8_t B,
                      double txpwr_dBm,
                      int NB_RE)
{

  int i;
  int aa;
  double amp,amp1;

  amp = //sqrt(NB_RE)*pow(10.0,.05*txpwr_dBm)/sqrt(nb_tx_antennas); //this is amp per tx antenna
    pow(10.0,.05*txpwr_dBm)/sqrt(nb_tx_antennas); //this is amp per tx antenna
  amp1 = 0;

  for (aa=0; aa<nb_tx_antennas; aa++) {
    amp1 += sqrt((double)signal_energy((int32_t*)&input[aa][input_offset_meas],length_meas)/NB_RE);
  }

  amp1/=nb_tx_antennas;

  //  printf("DAC: amp1 %f dB (%d,%d), tx_power %f\n",20*log10(amp1),input_offset,input_offset_meas,txpwr_dBm);

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

  for (i=0; i<length; i++) {
    for (aa=0; aa<nb_tx_antennas; aa++) {
      s_re[aa][i] = amp*((double)(((short *)input[aa]))[((i+input_offset)<<1)])/amp1; ///(1<<(B-1));
      s_im[aa][i] = amp*((double)(((short *)input[aa]))[((i+input_offset)<<1)+1])/amp1; ///(1<<(B-1));
    }
  }

  //  printf("ener %e\n",signal_energy_fp(s_re,s_im,nb_tx_antennas,length,0));

  return(signal_energy_fp(s_re,s_im,nb_tx_antennas,length_meas,0)/NB_RE);
}
