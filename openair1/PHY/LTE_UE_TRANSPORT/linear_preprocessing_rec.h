
#include<stdio.h>
#include<math.h>
#include<complex.h>
#include <stdlib.h>
#include "PHY/defs_UE.h"


/* FUNCTIONS FOR LINEAR PREPROCESSING: MMSE, WHITENNING, etc*/
void transpose(int N, float complex *A, float complex *Result);

void conjugate_transpose(int N, float complex *A, float complex *Result);

void H_hermH_plus_sigma2I(int N, int M, float complex *A, float sigma2, float complex *Result);

void HH_herm_plus_sigma2I(int M, int N, float complex *A, float sigma2, float complex *Result);

void eigen_vectors_values(int N, float complex *A, float complex *Vectors, float *Values_Matrix);

void lin_eq_solver(int N, float complex *A, float complex* B);
//float complex* lin_eq_solver (int N, float complex* A, float complex* B);

/* mutl_matrix_matrix_row_based performs multiplications when matrix is row-oriented H[0], H[1]; H[2], H[3]*/
void mutl_matrix_matrix_row_based(float complex* M0, float complex* M1, int rows_M0, int col_M0, int rows_M1, int col_M1, float complex* Result );

/* mutl_matrix_matrix_col_based performs multiplications matrix is column-oriented H[0], H[2]; H[1], H[3]*/
void mutl_matrix_matrix_col_based(float complex* M0, float complex* M1, int rows_M0, int col_M0, int rows_M1, int col_M1, float complex* Result );

void compute_MMSE(float complex* H, int order_H, float sigma2, float complex* W_MMSE);

void compute_white_filter(float complex* H, int order_H, float sigma2, float complex* U_1, float complex* D_1);

void mmse_processing_oai(LTE_UE_PDSCH *pdsch_vars,
                         LTE_DL_FRAME_PARMS *frame_parms,
                         PHY_MEASUREMENTS *measurements,
                         unsigned char first_symbol_flag,
                         MIMO_mode_t mimo_mode,
                         unsigned short mmse_flag,
                         int noise_power,
                         unsigned char symbol,
                         unsigned short nb_rb);


void precode_channel_est(int32_t **dl_ch_estimates_ext,
                        LTE_DL_FRAME_PARMS *frame_parms,
                        LTE_UE_PDSCH *pdsch_vars,
                        unsigned char symbol,
                        unsigned short nb_rb,
                        MIMO_mode_t mimo_mode);


void rxdataF_to_float(int32_t **rxdataF_ext,
                      float complex **rxdataF_f,
                      int n_rx,
                      int length,
                      int start_point);

void chan_est_to_float(int32_t **dl_ch_estimates_ext,
                       float complex **dl_ch_estimates_ext_f,
                       int n_tx,
                       int n_rx,
                       int length,
                       int start_point);

void float_to_chan_est(int32_t **dl_ch_estimates_ext,
                      float complex **dl_ch_estimates_ext_f,
                      int n_tx,
                      int n_rx,
                      int length,
                      int start_point);

void float_to_rxdataF(int32_t **rxdataF_ext,
                      float complex **rxdataF_f,
                      int n_tx,
                      int n_rx,
                      int length,
                      int start_point);

void mult_mmse_rxdataF(float complex** Wmmse,
                       float complex** rxdataF_ext_f,
                       int n_tx,
                       int n_rx,
                       int length,
                       int start_point);

void mult_mmse_chan_est(float complex** Wmmse,
                        float complex** dl_ch_estimates_ext_f,
                        int n_tx,
                        int n_rx,
                        int length,
                        int start_point);

void  mmse_processing_core(int32_t **rxdataF_ext,
                           int32_t **dl_ch_estimates_ext,
                           int sigma2,
                           int n_tx,
                           int n_rx,
                           int length,
                           int start_point);

void mmse_processing_core_flp(float complex** rxdataF_ext_flcpx,
                              float complex **H,
                              int32_t **rxdataF_ext,
                              int32_t **dl_ch_estimates_ext,
                              float sigma2,
                              int n_tx,
                              int n_rx,
                              int length,
                              int start_point);

void whitening_processing_core_flp(float complex** rxdataF_ext_flcpx,
                                   float complex **H,
                                   int32_t **rxdataF_ext,
                                   int32_t **dl_ch_estimates_ext,
                                   float sigma2,
                                   int n_tx,
                                   int n_rx,
                                   int length,
                                   int start_point);

float sqrt_float(float x, float sqrt_x);
