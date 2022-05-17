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
* Author and copyright: Laurent Thomas, open-cells.com
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>


#include <common/utils/assertions.h>
#include <common/utils/LOG/log.h>
#include <common/config/config_userapi.h>
#include <openair1/SIMULATION/TOOLS/sim.h>
#include <common/utils/telnetsrv/telnetsrv.h>
#include <common/utils/load_module_shlib.h>
#include <targets/ARCH/rfsimulator/rfsimulator.h>

/*
  Legacy study:
  The parameters are:
  gain&loss (decay, signal power, ...)
  either a fixed gain in dB, a target power in dBm or ACG (automatic control gain) to a target average
  => don't redo the AGC, as it was used in UE case, that must have a AGC inside the UE
  will be better to handle the "set_gain()" called by UE to apply it's gain (enable test of UE power loop)
  lin_amp = pow(10.0,.05*txpwr_dBm)/sqrt(nb_tx_antennas);
  a lot of operations in legacy, grouped in one simulation signal decay: txgain*decay*rxgain

  multi_path (auto convolution, ISI, ...)
  either we regenerate the channel (call again random_channel(desc,0)), or we keep it over subframes
  legacy: we regenerate each sub frame in UL, and each frame only in DL
*/
void rxAddInput( const c16_t *input_sig,
		 c16_t *after_channel_sig,
                 int rxAnt,
                 channel_desc_t *channelDesc,
                 int nbSamples,
                 uint64_t TS,
                 uint32_t CirSize
               ) {
  // channelDesc->path_loss_dB should contain the total path gain
  // so, in actual RF: tx gain + path loss + rx gain (+antenna gain, ...)
  // UE and NB gain control to be added
  // Fixme: not sure when it is "volts" so dB is 20*log10(...) or "power", so dB is 10*log10(...)
  const double pathLossLinear = pow(10,channelDesc->path_loss_dB/20.0);
  // Energy in one sample to calibrate input noise
  // the normalized OAI value seems to be 256 as average amplitude (numerical amplification = 1)
  const double noise_per_sample = pow(10,channelDesc->noise_power_dB/10.0) * 256;
  const int dd = abs(channelDesc->channel_offset);
  const int nbTx=channelDesc->nb_tx;

  for (int i=0; i<nbSamples; i++) {
    struct complex16 *out_ptr=after_channel_sig+i;
    struct complexd rx_tmp= {0};

    for (int txAnt=0; txAnt < nbTx; txAnt++) {
      const struct complexd *channelModel= channelDesc->ch[rxAnt+(txAnt*channelDesc->nb_rx)];

      //const struct complex *channelModelEnd=channelModel+channelDesc->channel_length;
      for (int l = 0; l<(int)channelDesc->channel_length; l++) {
        // let's assume TS+i >= l
        // fixme: the rfsimulator current structure is interleaved antennas
        // this has been designed to not have to wait a full block transmission
        // but it is not very usefull
        // it would be better to split out each antenna in a separate flow
        // that will allow to mix ru antennas freely
        // (X + cirSize) % cirSize to ensure that index is positive
        const int idx = ((TS + i - l - dd) * nbTx + txAnt + CirSize) % CirSize;
        const struct complex16 tx16 = input_sig[idx];
        rx_tmp.r += tx16.r * channelModel[l].r - tx16.i * channelModel[l].i;
        rx_tmp.i += tx16.i * channelModel[l].r + tx16.r * channelModel[l].i;
      } //l
    }

    // Fixme: lround(), rount(), ... is detected by valgrind as error, not found why
    out_ptr->r += lround(rx_tmp.r*pathLossLinear + noise_per_sample*gaussZiggurat(0.0,1.0));
    out_ptr->i += lround(rx_tmp.i*pathLossLinear + noise_per_sample*gaussZiggurat(0.0,1.0));
    out_ptr++;
  }

  if ( (TS*nbTx)%CirSize+nbSamples <= CirSize )
    // Cast to a wrong type for compatibility !
    LOG_D(HW,"Input power %f, output power: %f, channel path loss %f, noise coeff: %f \n",
          10*log10((double)signal_energy((int32_t *)&input_sig[(TS*nbTx)%CirSize], nbSamples)),
          10*log10((double)signal_energy((int32_t *)after_channel_sig, nbSamples)),
          channelDesc->path_loss_dB,
          10*log10(noise_per_sample));
}

