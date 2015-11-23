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
/** \brief Apply RF impairments to received signal
@param r_re Double two-dimensional array of Real part of received signal
@param r_im Double two-dimensional array of Imag part of received signal
@param r_re_i1 Double two-dimensional array of Real part of received interfering signal
@param r_im_i1 Double two-dimensional array of Imag part of received interfering signal
@param I0_dB Interference-level with respect to desired signal in dB
@param nb_rx_antennas Number of receive antennas
@param length of signal
@param s_time sampling time in s
@param f_off Frequency offset in Hz
@param drift Timing drift in Hz
@param noise_figure Noise figure in dB
@param rx_gain_dB Total receiver gain in dB
@param IP3_dBm Input IP3 in dBm
@param initial_phase Initial phase for receiver
@param pn_cutoff Phase noise cutoff frequency (loop filter)
@param pn_amp_dBc Phase noise amplitude with respect to carrier
@param IQ_imb_dB IQ amplitude imbalance in dB
@param IQ_phase IQ phase imbalance in radians*/
void rf_rx(double **r_re,
           double **r_im,
           double **r_re_i1,
           double **r_im_i1,
           double I0_dB,
           unsigned int nb_rx_antennas,
           unsigned int length,
           double s_time,
           double f_off,
           double drift,
           double noise_figure,
           double rx_gain_dB,
           int IP3_dBm,
           double *initial_phase,
           double pn_cutoff,
           double pn_amp_dBc,
           double IQ_imb_dB,
           double IQ_phase);

void rf_rx_simple(double **r_re,
                  double **r_im,
                  unsigned int nb_rx_antennas,
                  unsigned int length,
                  double s_time,
                  double rx_gain_dB);


void adc(double **r_re,
         double **r_im,
         unsigned int input_offset,
         unsigned int output_offset,
         int **output,
         unsigned int nb_rx_antennas,
         unsigned int length,
         unsigned char B);

void dac(double **s_re,
         double **s_im,
         int **input,
         unsigned int input_offset,
         unsigned int nb_tx_antennas,
         unsigned int length,
         double amp_dBm,
         unsigned char B,
         unsigned int meas_length,
         unsigned int meas_offset);

double dac_fixed_gain(double **s_re,
                      double **s_im,
                      int **input,
                      unsigned int input_offset,
                      unsigned int nb_tx_antennas,
                      unsigned int length,
                      unsigned int input_offset_meas,
                      unsigned int length_meas,
                      unsigned char B,
                      double gain,
                      int NB_RE);
