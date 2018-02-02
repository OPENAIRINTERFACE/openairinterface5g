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

#ifndef __SIMULATION_TOOLS_DEFS_H__
#define __SIMULATION_TOOLS_DEFS_H__
#include "PHY/defs.h"

/** @defgroup _numerical_ Useful Numerical Functions
 *@{
The present clause specifies several numerical functions for testing of digital communication systems.

-# Generation of Uniform Random Bits
-# Generation of Quantized Gaussian Random Variables
-# Generation of Floating-point Gaussian Random Variables
-# Generic Multipath Channel Generator

 * @defgroup _channel_ Multipath channel generator
 * @ingroup _numerical_
 * @{

*/

#define NB_SAMPLES_CHANNEL_OFFSET 4

typedef struct {
  ///Number of tx antennas
  uint8_t nb_tx;
  ///Number of rx antennas
  uint8_t nb_rx;
  ///number of taps
  uint8_t nb_taps;
  ///linear amplitudes of taps
  double *amps;
  ///Delays of the taps in mus. length(delays)=nb_taps. Has to be between 0 and Td.
  double *delays;
  ///length of impulse response. should be set to 11+2*bw*t_max
  uint8_t channel_length;
  ///channel state vector. size(state) = nb_taps * (n_tx * n_rx);
  struct complex **a;
  ///interpolated (sample-spaced) channel impulse response. size(ch) = (n_tx * n_rx) * channel_length. ATTENTION: the dimensions of ch are the transposed ones of a. This is to allow the use of BLAS when applying the correlation matrices to the state.
  struct complex **ch;
  ///Sampled frequency response (90 kHz resolution)
  struct complex **chF;
  ///Maximum path delay in mus.
  double Td;
  ///Channel bandwidth in MHz.
  double channel_bandwidth;
  ///System sampling rate in Msps.
  double sampling_rate;
  ///Ricean factor of first tap wrt other taps (0..1, where 0 means AWGN and 1 means Rayleigh channel).
  double ricean_factor;
  ///Angle of arrival of wavefront (in radians). For Ricean channel only. This assumes that both RX and TX have linear antenna arrays with lambda/2 antenna spacing. Furhter it is assumed that the arrays are parallel to each other and that they are far enough apart so that we can safely assume plane wave propagation.
  double aoa;
  ///If set to 1, aoa is randomized according to a uniform random distribution
  int8_t random_aoa;
  ///in Hz. if >0 generate a channel with a Clarke's Doppler profile with a maximum Doppler bandwidth max_Doppler. CURRENTLY NOT IMPLEMENTED!
  double max_Doppler;
  ///Square root of the full correlation matrix size(R_tx) = nb_taps * (n_tx * n_rx) * (n_tx * n_rx).
  struct complex **R_sqrt;
  ///path loss including shadow fading in dB
  double path_loss_dB;
  ///additional delay of channel in samples.
  int32_t channel_offset;
  ///This parameter (0...1) allows for simple 1st order temporal variation. 0 means a new channel every call, 1 means keep channel constant all the time
  double forgetting_factor;
  ///needs to be set to 1 for the first call, 0 otherwise.
  uint8_t first_run;
  /// initial phase for frequency offset simulation
  double ip;
  /// number of paths taken by transmit signal
  uint16_t nb_paths;
  /// timing measurements
  time_stats_t random_channel;
  time_stats_t interp_time;
  time_stats_t interp_freq;
  time_stats_t convolution;
} channel_desc_t;

typedef struct {
  /// Number of sectors (set to 1 in case of an omnidirectional antenna)
  uint8_t n_sectors;
  /// Antenna orientation for each sector (for non-omnidirectional antennas) in radians wrt north
  double alpha_rad[3];
  /// Antenna 3dB beam width (in radians)
  double phi_rad;
  /// Antenna gain (dBi)
  double ant_gain_dBi;
  /// Tx power (dBm)
  double tx_power_dBm;
  /// Rx noise level (dB)
  double rx_noise_level;
  ///x coordinate (cartesian, in m)
  double x;
  ///y coordinate (cartesian, in m)
  double y;
  ///z coordinate (antenna height, in m)
  double z;
  /// direction of travel in radians wrt north
  double direction_rad;
  /// speed of node (m/s)
  double speed;
} node_desc_t;

typedef enum {
  rural=0,
  urban,
  indoor
} scenario_t;

typedef struct {
  /// Scenario classifcation
  scenario_t scenario;
  /// Carrier frequency in Hz
  double carrier_frequency;
  /// Bandwidth (in Hz)
  double bandwidth;
  /// path loss at 0m distance in dB
  double path_loss_0;
  /// path loss exponent
  double path_loss_exponent;
  /// shadow fading standard deviation [dB] (assuming log-normal shadow fading with 0 mean)
  double shadow_fading_std;
  /// correlation distance of shadow fading
  double shadow_fading_correlation_distance;
  /// Shadowing correlation between cells
  double shadow_fading_correlation_cells;
  /// Shadowing correlation between sectors
  double shadow_fading_correlation_sectors;
  /// Rice factor???
  /// Walls (penetration loss)
  /// Nodes in the scenario
  node_desc_t* nodes;
} scenario_desc_t;

typedef enum {
  custom=0,
  SCM_A,
  SCM_B,
  SCM_C,
  SCM_D,
  EPA,
  EVA,
  ETU,
  MBSFN,
  Rayleigh8,
  Rayleigh1,
  Rayleigh1_800,
  Rayleigh1_corr,
  Rayleigh1_anticorr,
  Rice8,
  Rice1,
  Rice1_corr,
  Rice1_anticorr,
  AWGN,
  Rayleigh1_orthogonal,
  Rayleigh1_orth_eff_ch_TM4_prec_real,
  Rayleigh1_orth_eff_ch_TM4_prec_imag,
  Rayleigh8_orth_eff_ch_TM4_prec_real,
  Rayleigh8_orth_eff_ch_TM4_prec_imag,
  TS_SHIFT,
  EPA_low,
  EPA_medium,
  EPA_high,
} SCM_t;

/**
\brief This routine initializes a new channel descriptor
\param nb_tx Number of TX antennas
\param nb_rx Number of RX antennas
\param nb_taps Number of taps
\param channel_length Length of the interpolated channel impulse response
\param amps Linear amplitudes of the taps (length(amps)=channel_length). The values should sum up to 1.
\param delays Delays of the taps. If delays==NULL the taps are assumed to be spaced equidistantly between 0 and t_max.
\param R_sqrt Channel correlation matrix. If R_sqrt==NULL, no channel correlation is applied.
\param Td Maximum path delay in mus.
\param BW Channel bandwidth in MHz.
\param ricean_factor Ricean factor applied to all taps.
\param aoa Anlge of arrival
\param forgetting_factor This parameter (0...1) allows for simple 1st order temporal variation
\param max_Doppler This is the maximum Doppler frequency for Jakes' Model
\param channel_offset This is a time delay to apply to channel
\param path_loss_dB This is the path loss in dB
\param random_aoa If set to 1, AoA of ricean component is randomized
*/

//channel_desc_t *new_channel_desc(uint8_t nb_tx,uint8_t nb_rx, uint8_t nb_taps, uint8_t channel_length, double *amps, double* delays, struct complex** R_sqrt, double Td, double BW, double ricean_factor, double aoa, double forgetting_factor, double max_Doppler, int32_t channel_offset, double path_loss_dB,uint8_t random_aoa);

channel_desc_t *new_channel_desc_scm(uint8_t nb_tx,
                                     uint8_t nb_rx,
                                     SCM_t channel_model,
				     double sampling_rate,
                                     double channel_bandwidth,
                                     double forgetting_factor,
                                     int32_t channel_offset,
                                     double path_loss_dB);



/** \fn void random_channel(channel_desc_t *desc)
\brief This routine generates a random channel response (time domain) according to a tapped delay line model.
\param desc Pointer to the channel descriptor
*/
int random_channel(channel_desc_t *desc, uint8_t abstraction_flag);

/**\fn void multipath_channel(channel_desc_t *desc,
           double tx_sig_re[2],
           double tx_sig_im[2],
           double rx_sig_re[2],
           double rx_sig_im[2],
           uint32_t length,
           uint8_t keep_channel)

\brief This function generates and applys a random frequency selective random channel model.
@param desc Pointer to channel descriptor
@param tx_sig_re input signal (real component)
@param tx_sig_im input signal (imaginary component)
@param rx_sig_re output signal (real component)
@param rx_sig_im output signal (imaginary component)
@param length Length of input signal
@param keep_channel Set to 1 to keep channel constant for null-B/F
*/

void multipath_channel(channel_desc_t *desc,
                       double *tx_sig_re[2],
                       double *tx_sig_im[2],
                       double *rx_sig_re[2],
                       double *rx_sig_im[2],
                       uint32_t length,
                       uint8_t keep_channel);
/*
\fn double compute_pbch_sinr(channel_desc_t *desc,
                             channel_desc_t *desc_i1,
           channel_desc_t *desc_i2,
           double snr_dB,double snr_i1_dB,
           double snr_i2_dB,
           uint16_t nb_rb)

\brief This function computes the average SINR over all frequency resources of the PBCH.  It is used for PHY abstraction of the PBCH BLER
@param desc Pointer to channel descriptor of eNB
@param desc Pointer to channel descriptor of interfering eNB 1
@param desc Pointer to channel descriptor of interfering eNB 2
@param snr_dB SNR of eNB
@param snr_i1_dB SNR of interfering eNB 1
@param snr_i2_dB SNR of interfering eNB 2
@param nb_rb Number of RBs in system
*/
double compute_pbch_sinr(channel_desc_t *desc,
                         channel_desc_t *desc_i1,
                         channel_desc_t *desc_i2,
                         double snr_dB,double snr_i1_dB,
                         double snr_i2_dB,
                         uint16_t nb_rb);

double compute_sinr(channel_desc_t *desc,
                    channel_desc_t *desc_i1,
                    channel_desc_t *desc_i2,
                    double snr_dB,double snr_i1_dB,
                    double snr_i2_dB,
                    uint16_t nb_rb);

double pbch_bler(double sinr);

void load_pbch_desc(FILE *pbch_file_fd);

/**@}*/

/**
 * @defgroup _taus_ Tausworthe Uniform Random Variable Generator
 * @ingroup _numerical_
 * @{
\fn unsigned int taus()
\brief Tausworthe Uniform Random Generator.  This is based on the hardware implementation described in
  Lee et al, "A Hardware Gaussian Noise Generator Usign the Box-Muller Method and its Error Analysis," IEEE Trans. on Computers, 2006.
*/
unsigned int taus(void);


/**
\fn void set_taus_seed(unsigned int seed_init)
\brief Sets the seed for the Tausworthe generator.
@param seed_init 0 means generate based on CPU time, otherwise provide the seed
*/
void set_taus_seed(unsigned int seed_init);
/**@} */

/** @defgroup _gauss_ Generation of Quantized Gaussian Random Variables
 * @ingroup _numerical_
 * @{
This set of routines are used to generate quantized (i.e. fixed-point) Gaussian random noise efficiently.
The use of these routines allows for rapid computer simulation of digital communication systems. The method
is based on a lookup-table of the quantized normal probability distribution.  The routines assume that the
continuous-valued Gaussian random-variable,\f$x\f$ is quantized
to \f$N\f$ bits over the interval \f$[-L\sigma,L\sigma)\f$ where \f$N\f$ and \f$L\f$ control the precision
and range of the quantization.  The
random variable, \f$l\in\{-2^{N-1},-2^{N-1}+1,\cdots,0,1,\cdots,2^{N-1}-1\}\f$ corresponds to the event,
\f$E_l =
\begin{cases}
x\in\left[-\infty,-L\sigma\right) & l=-2^{N-1}, \\
x\in\left[\frac{lL\sigma}{2^{N-1}},\frac{(l+1)L\sigma}{2^{N-1}}\right) & <l>-2^{N-1}, \\
x\in\left[L\sigma,\infty\right) & l>-2^{N-1},
\end{cases}\f$
which occurs with probability
\f$\Pr(E_l) =
\begin{cases}
\mathrm{erfc}(L) & l=-2^{N-1}, \\
\mathrm{erfc}(L) & l>-2^{N-1}, \\
\mathrm{erf}\left(\frac{lL}{2^{N-1}}\right) \mathrm{erfc}\left(\frac{(l-1)L}{2^{N-1}}\right)& l>-2^{N-1}.
\end{cases}\f$
*/


/** \fn unsigned int *generate_gauss_LUT(unsigned char Nbits,unsigned char L)
\brief This routine generates a Gaussian pdf lookup table (LUT).  The table has \f$2^{\mathrm{Nbits}-1}\f$ entries which represent
the right half of the pdf.  The data stored in position \f$i\f$ is actually the scaled cumulative probability distribution,
\f$2^{31}\mathrm{erf}\left(\frac{iL}{2^{N-1}}\right)\f$.  This represents the average number of times that the random variable
falls in the interval \f$\left[0,\frac{i}{2^{N-1}}\right)\f$.  This format allows for rapid conversion of uniform 32-bit
random variables to \f$N\f$-bit Gaussian random variables using binary search.
@see gauss
@param Nbits Number of bits for the output variable
@param L Number of standard deviations in range
*/
unsigned int *generate_gauss_LUT(unsigned char Nbits,unsigned char L);

/** \fn int gauss(unsigned int *gauss_LUT,unsigned char Nbits);
\brief This routine returns a zero-mean unit-variance Gaussian random variable.
 Given a 32-bit uniform random variable,
\f$\mathrm{u}\f$ (from \ref _taus_, we first extract the sign and then search in the monotonically increasing Gaussian LUT for
the two entries \f$(i,i+1)\f$ for which
\f$ 2^{31}\mathrm{erf}\left(\frac{i}{2^{Nbits-1}}\right) < |u| \leq 2^{31}\mathrm{erf}\left(\frac{i+1}{2^{Nbits-1}}\right) \f$ and assign
the value \f$\mathrm{sgn}(u)i\f$.  The search requires at most \f$Nbits-1\f$ comparisons.
@see generate_gauss_LUT
@see taus
@param gauss_LUT pointer to lookup-table
@param Nbits number of bits for output variable ( between 1 and 16)
*/
int gauss(unsigned int *gauss_LUT,unsigned char Nbits);

double gaussdouble(double,double);
void randominit(unsigned int seed_init);
double uniformrandom(void);
int freq_channel(channel_desc_t *desc,uint16_t nb_rb, int16_t n_samples);
int init_freq_channel(channel_desc_t *desc,uint16_t nb_rb,int16_t n_samples);
uint8_t multipath_channel_nosigconv(channel_desc_t *desc);
void multipath_tv_channel(channel_desc_t *desc,
                          double **tx_sig_re,
                          double **tx_sig_im,
                          double **rx_sig_re,
                          double **rx_sig_im,
                          uint16_t length,
                          uint8_t keep_channel);

/**@} */
/**@} */

double N_RB2sampling_rate(uint16_t N_RB);
double N_RB2channel_bandwidth(uint16_t N_RB);

#endif


