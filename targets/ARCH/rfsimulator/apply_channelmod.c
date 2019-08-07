/*
  Author: Laurent THOMAS, Open Cells for Nokia
  copyleft: OpenAirInterface Software Alliance and it's licence

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
void rxAddInput( struct complex16 *input_sig, struct complex16 *after_channel_sig,
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
  //Fixme: modified the N0W computation, not understand the origin value
  const double KT=1.38e-23*290; //Boltzman*temperature
  // sampling rate is linked to acquisition band (the input pass band filter)
  const double noise_figure_watt = KT*channelDesc->sampling_rate;
  // Fixme: how to convert a noise in Watt into a 12 bits value out of the RF ADC ?
  // the parameter "-s" is declared as SNR, but the input power is not well defined
  // −132.24 dBm is a LTE subcarrier noise, that was used in origin code (15KHz BW thermal noise)
  const double rxGain= 132.24 - channelmod_get_snr_dB();
  // sqrt(4*noise_figure_watt) is the thermal noise factor (volts)
  // fixme: the last constant is pure trial results to make decent noise
  const double noise_per_sample = sqrt(4*noise_figure_watt) * pow(10,rxGain/20) *10;
  // Fixme: we don't fill the offset length samples at begining ?
  // anyway, in today code, channel_offset=0
  const int dd = abs(channelDesc->channel_offset);
  const int nbTx=channelDesc->nb_tx;

  for (int i=0; i<((int)nbSamples-dd); i++) {
    struct complex16 *out_ptr=after_channel_sig+dd+i;
    struct complex rx_tmp= {0};

    for (int txAnt=0; txAnt < nbTx; txAnt++) {
      const struct complex *channelModel= channelDesc->ch[rxAnt+(txAnt*channelDesc->nb_rx)];

      //const struct complex *channelModelEnd=channelModel+channelDesc->channel_length;
      for (int l = 0; l<(int)channelDesc->channel_length; l++) {
        // let's assume TS+i >= l
        // fixme: the rfsimulator current structure is interleaved antennas
        // this has been designed to not have to wait a full block transmission
        // but it is not very usefull
        // it would be better to split out each antenna in a separate flow
        // that will allow to mix ru antennas freely
        struct complex16 tx16=input_sig[((TS+i-l)*nbTx+txAnt)%CirSize];
        rx_tmp.x += tx16.r * channelModel[l].x - tx16.i * channelModel[l].y;
        rx_tmp.y += tx16.i * channelModel[l].x + tx16.r * channelModel[l].y;
      } //l
    }

    out_ptr->r += round(rx_tmp.x*pathLossLinear + noise_per_sample*gaussdouble(0.0,1.0));
    out_ptr->i += round(rx_tmp.y*pathLossLinear + noise_per_sample*gaussdouble(0.0,1.0));
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
