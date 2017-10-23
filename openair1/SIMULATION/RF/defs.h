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

void rf_rx_simple(double *r_re[2],
                  double *r_im[2],
                  unsigned int nb_rx_antennas,
                  unsigned int length,
                  double s_time,
                  double rx_gain_dB);


void adc(double *r_re[2],
         double *r_im[2],
         unsigned int input_offset,
         unsigned int output_offset,
         int **output,
         unsigned int nb_rx_antennas,
         unsigned int length,
         unsigned char B);

void dac(double *s_re[2],
         double *s_im[2],
         int **input,
         unsigned int input_offset,
         unsigned int nb_tx_antennas,
         unsigned int length,
         double amp_dBm,
         unsigned char B,
         unsigned int meas_length,
         unsigned int meas_offset);

double dac_fixed_gain(double *s_re[2],
                      double *s_im[2],
                      int **input,
                      unsigned int input_offset,
                      unsigned int nb_tx_antennas,
                      unsigned int length,
                      unsigned int input_offset_meas,
                      unsigned int length_meas,
                      unsigned char B,
                      double gain,
		      uint8_t do_amp_compute,
		      double *amp1,
                      int NB_RE);
