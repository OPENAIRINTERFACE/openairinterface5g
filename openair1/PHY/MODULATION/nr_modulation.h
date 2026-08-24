/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef __NR_MODULATION_H__
#define __NR_MODULATION_H__

#include <stdint.h>
#include "PHY/defs_nr_common.h"

#define DMRS_MOD_ORDER 2
/*Precoding matices: W[pmi][antenna_port][layer]*/
extern const char nr_W_1l_2p[6][2][1];
extern const char nr_W_2l_2p[3][2][2];
extern const char nr_W_1l_4p[28][4][1];
extern const char nr_W_2l_4p[22][4][2];
extern const char nr_W_3l_4p[7][4][3];
extern const char nr_W_4l_4p[5][4][4];
/*! \brief Perform NR modulation. TS 38.211 V15.4.0 subclause 5.1
  @param[in] in, Pointer to input bits
  @param[in] length, size of input bits
  @param[in] modulation_type, modulation order
  @param[out] out, complex valued modulated symbols
*/

void nr_modulation(const uint32_t *in,
                   uint32_t length,
                   uint16_t mod_order,
                   int16_t *out);

/*! \brief Perform NR layer mapping. TS 38.211 V15.4.0 subclause 7.3.1.3
  @param[in] mod_symbs, double Pointer to modulated symbols for each codeword
  @param[in] n_layers, number of layers
  @param[in] n_symbs, number of modulated symbols
  @param[out] tx_layers, modulated symbols for each layer
*/

void nr_layer_mapping(int nbCodes,
                      int encoded_len,
                      c16_t mod_symbs[nbCodes][encoded_len],
                      uint8_t n_layers,
                      int layerSz,
                      uint32_t n_symbs,
                      c16_t tx_layers[][layerSz]);

/*! \brief Perform NR layer mapping. TS 38.211 V15.4.0 subclause 7.3.1.3
  @param[in] ulsch_ue, double Pointer to NR_UE_ULSCH_t struct
  @param[in] n_layers, number of layers
  @param[in] n_symbs, number of modulated symbols
  @param[out] tx_layers, modulated symbols for each layer
*/
void nr_ue_layer_mapping(const c16_t *mod_symbs, const int n_layers, const int n_symbs, c16_t tx_layers[][n_symbs]);

/*!
\brief This function implements the OFDM front end processor on reception (FEP)
\param frame_parms Pointer to frame parameters
\param rxdata Pointer to input data in time domain for one frame
\param rxdataF Pointer to output data in frequency domain for one symbol
\param symbol symbol within slot (0..12/14)
\param slot Slot number
\param sample_offset offset within rxdata (points to beginning of symbol)
*/
int nr_symbol_fep_ul(const NR_DL_FRAME_PARMS *fp,
                     const c16_t *rxdata,
                     c16_t *rxdataF,
                     unsigned char symbol,
                     unsigned char slot,
                     int sample_offset);

/*!
\brief This function implements the dft transform precoding in PUSCH
\param z Pointer to output in frequnecy domain
\param d Pointer to input in time domain
\param Msc_PUSCH number of allocated data subcarriers
*/

void nr_dft(c16_t *z, c16_t *d, uint32_t Msc_PUSCH);

void nr_beam_precoding(c16_t **txdataF,
                       c16_t **txdataF_BF,
                       NR_DL_FRAME_PARMS *frame_parms,
                       int32_t ***beam_weights,
                       int symbol,
                       int aa,
                       int nb_antenna_ports,
                       int offset);

void apply_nr_rotation_TX(const NR_DL_FRAME_PARMS *fp,
                          c16_t *txdataF,
                          bool is_flat_buff,
                          const c16_t *symbol_rotation,
                          int slot,
                          int nb_rb,
                          int first_symbol,
                          int nsymb);

void nr_ofdm_demod_and_rx_rotation(c16_t **rxdata,
                                   c16_t **rxdataF,
                                   const NR_DL_FRAME_PARMS *fp,
                                   int nb_antennas,
                                   int slot,
                                   int slot_offsetF,
                                   enum nr_Link linktype,
                                   bool was_symbol_used[NR_SYMBOLS_PER_SLOT]);
void perform_symbol_rotation(const int nsymb, const int numerology_index, double f0, c16_t *symbol_rotation);

void init_symbol_rotation(NR_DL_FRAME_PARMS *fp);

void init_timeshift_rotation(const int ofdm_symbol_size,
                             const int nb_prefix_samples,
                             const uint ofdm_offset_divisor,
                             c16_t *timeshift_symbol_rotation);

void apply_nr_rotation_symbol_RX(const int symbols_per_slot,
                                 const int slots_per_subframe,
                                 const c16_t *timeshift_symbol_rotation,
                                 const int first_carrier_offset,
                                 c16_t *rxdataF,
                                 const c16_t *rot,
                                 int nb_rb,
                                 int slot,
                                 int symbol);

/*! \brief Perform NR precoding. TS 38.211 V15.4.0 subclause 6.3.1.5
  @param[in] datatx_F_precoding, Pointer to n_layers*re data array
  @param[in] prec_matrix, Pointer to precoding matrix
  @param[in] n_layers, number of DLSCH layers
*/
c16_t nr_layer_precoder(int sz, c16_t datatx_F_precoding[][sz], const char *prec_matrix, uint8_t n_layers, int32_t re_offset);

c16_t nr_layer_precoder_cm(int n_layers,
                           int symSz,
                           c16_t datatx_F_precoding[n_layers][symSz],
                           int ap,
                           c16_t weights[NR_MAX_NB_LAYERS][NR_MAX_CSI_PORTS],
                           int offset);

/*! \brief Precoding with SIMDe, txdataF_precoded[] = prec_matrix[] * txdataF_res_mapped[]
  @param[in]  txdataF_res_mapped Tx data after resource mapping, before precoding.
  @param[in]  prec_matrix        Weights of precoding matrix.
  @param[in]  re_cnt             Number of RE (sub carrier) to write to txdataF_precoded, should be multiple of 4.
  @param[out] txdataF_precoded   Precoded antenna data
*/
void nr_layer_precoder_simd(const int n_layers,
                            const int symSz,
                            const c16_t txdataF_res_mapped[n_layers][symSz],
                            const int ant,
                            c16_t weights[NR_MAX_NB_LAYERS][NR_MAX_CSI_PORTS],
                            const int sc_offset,
                            const int re_cnt,
                            c16_t *txdataF_precoded);

void nr_normal_prefix_mod(c16_t *txdataF,
                          c16_t *txdata,
                          uint8_t nsymb,
                          const NR_DL_FRAME_PARMS *frame_parms,
                          uint32_t slot,
                          bool was_symbol_used[NR_SYMBOLS_PER_SLOT]);

void fft_shift(const c16_t *in,
               uint32_t in_symb_sz,
               uint16_t num_prb,
               c16_t *out,
               uint16_t fft_size_out,
               uint16_t start_symb,
               uint16_t num_symb);
#endif
