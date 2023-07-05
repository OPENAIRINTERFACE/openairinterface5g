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

/*! \file PHY/LTE_TRANSPORT/dlsch_llr_computation.c
 * \brief Top-level routines for LLR computation of the PDSCH physical channel from 36-211, V8.6 2009-03
 * \author R. Knopp, F. Kaltenberger,A. Bhamri, S. Aubert, S. Wagner, X Jiang
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr,ankit.bhamri@eurecom.fr,sebastien.aubert@eurecom.fr, sebastian.wagner@eurecom.fr
 * \note
 * \warning
 */

#include "PHY/defs_UE.h"
#include "PHY/TOOLS/tools_defs.h"
#include "PHY/phy_extern_ue.h"
#include "PHY/sse_intrin.h"

#if defined(__x86_64__)

 static const int16_t ones256[16] __attribute__((aligned(32))) = {0xffff,
                                                                  0xffff,
                                                                  0xffff,
                                                                  0xffff,
                                                                  0xffff,
                                                                  0xffff,
                                                                  0xffff,
                                                                  0xffff,
                                                                  0xffff,
                                                                  0xffff,
                                                                  0xffff,
                                                                  0xffff,
                                                                  0xffff,
                                                                  0xffff,
                                                                  0xffff,
                                                                  0xffff};


 // Auxiliary Makros

 // calculate interference magnitude
#define interference_abs_epi16(psi, int_ch_mag, int_mag, c1, c2)                   \
  tmp_result = simde_mm256_cmpgt_epi16(int_ch_mag, psi);                           \
  tmp_result2 = simde_mm256_xor_si256(tmp_result, (*(simde__m256i *)&ones256[0])); \
  tmp_result = simde_mm256_and_si256(tmp_result, c1);                              \
  tmp_result2 = simde_mm256_and_si256(tmp_result2, c2);                            \
  const simde__m256i int_mag = simde_mm256_or_si256(tmp_result, tmp_result2);

 // calculate interference magnitude
 // tmp_result = ones in shorts corr. to interval 2<=x<=4, tmp_result2 interval < 2, tmp_result3 interval 4<x<6 and tmp_result4
 // interval x>6
#define interference_abs_64qam_epi16(psi, int_ch_mag, int_two_ch_mag, int_three_ch_mag, a, c1, c3, c5, c7) \
  tmp_result = simde_mm256_cmpgt_epi16(int_two_ch_mag, psi);                                               \
  tmp_result3 = simde_mm256_xor_si256(tmp_result, (*(simde__m256i *)&ones256[0]));                         \
  tmp_result2 = simde_mm256_cmpgt_epi16(int_ch_mag, psi);                                                  \
  tmp_result = simde_mm256_xor_si256(tmp_result, tmp_result2);                                             \
  tmp_result4 = simde_mm256_cmpgt_epi16(psi, int_three_ch_mag);                                            \
  tmp_result3 = simde_mm256_xor_si256(tmp_result3, tmp_result4);                                           \
  tmp_result = simde_mm256_and_si256(tmp_result, c3);                                                      \
  tmp_result2 = simde_mm256_and_si256(tmp_result2, c1);                                                    \
  tmp_result3 = simde_mm256_and_si256(tmp_result3, c5);                                                    \
  tmp_result4 = simde_mm256_and_si256(tmp_result4, c7);                                                    \
  tmp_result = simde_mm256_or_si256(tmp_result, tmp_result2);                                              \
  tmp_result3 = simde_mm256_or_si256(tmp_result3, tmp_result4);                                            \
  const simde__m256i a = simde_mm256_or_si256(tmp_result, tmp_result3);

 // calculates psi_a = psi_r*a_r + psi_i*a_i
#define prodsum_psi_a_epi16(psi_r, a_r, psi_i, a_i, psi_a) \
  tmp_result = simde_mm256_mulhi_epi16(psi_r, a_r);        \
  tmp_result = simde_mm256_slli_epi16(tmp_result, 1);      \
  tmp_result2 = simde_mm256_mulhi_epi16(psi_i, a_i);       \
  tmp_result2 = simde_mm256_slli_epi16(tmp_result2, 1);    \
  simde__m256i psi_a = simde_mm256_adds_epi16(tmp_result, tmp_result2);

 // calculates a_sq = int_ch_mag*(a_r^2 + a_i^2)*scale_factor
#define square_a_epi16(a_r, a_i, int_ch_mag, scale_factor, a_sq)    \
  tmp_result = simde_mm256_mulhi_epi16(a_r, a_r);                   \
  tmp_result = simde_mm256_slli_epi16(tmp_result, 1);               \
  tmp_result = simde_mm256_mulhi_epi16(tmp_result, scale_factor);   \
  tmp_result = simde_mm256_slli_epi16(tmp_result, 1);               \
  tmp_result = simde_mm256_mulhi_epi16(tmp_result, int_ch_mag);     \
  tmp_result = simde_mm256_slli_epi16(tmp_result, 1);               \
  tmp_result2 = simde_mm256_mulhi_epi16(a_i, a_i);                  \
  tmp_result2 = simde_mm256_slli_epi16(tmp_result2, 1);             \
  tmp_result2 = simde_mm256_mulhi_epi16(tmp_result2, scale_factor); \
  tmp_result2 = simde_mm256_slli_epi16(tmp_result2, 1);             \
  tmp_result2 = simde_mm256_mulhi_epi16(tmp_result2, int_ch_mag);   \
  tmp_result2 = simde_mm256_slli_epi16(tmp_result2, 1);             \
  const simde__m256i a_sq = simde_mm256_adds_epi16(tmp_result, tmp_result2);

 // calculates a_sq = int_ch_mag*(a_r^2 + a_i^2)*scale_factor for 64-QAM
#define square_a_64qam_epi16(a_r, a_i, int_ch_mag, scale_factor, a_sq) \
  tmp_result = simde_mm256_mulhi_epi16(a_r, a_r);                      \
  tmp_result = simde_mm256_slli_epi16(tmp_result, 1);                  \
  tmp_result = simde_mm256_mulhi_epi16(tmp_result, scale_factor);      \
  tmp_result = simde_mm256_slli_epi16(tmp_result, 3);                  \
  tmp_result = simde_mm256_mulhi_epi16(tmp_result, int_ch_mag);        \
  tmp_result = simde_mm256_slli_epi16(tmp_result, 1);                  \
  tmp_result2 = simde_mm256_mulhi_epi16(a_i, a_i);                     \
  tmp_result2 = simde_mm256_slli_epi16(tmp_result2, 1);                \
  tmp_result2 = simde_mm256_mulhi_epi16(tmp_result2, scale_factor);    \
  tmp_result2 = simde_mm256_slli_epi16(tmp_result2, 3);                \
  tmp_result2 = simde_mm256_mulhi_epi16(tmp_result2, int_ch_mag);      \
  tmp_result2 = simde_mm256_slli_epi16(tmp_result2, 1);                \
  const simde__m256i a_sq = simde_mm256_adds_epi16(tmp_result, tmp_result2);

void qam64_qam16_avx2(short *stream0_in,
                      short *stream1_in,
                      short *ch_mag,
                      short *ch_mag_i,
                      short *stream0_out,
                      short *rho01,
                      int length
    )
{

  /*
    Author: S. Wagner
    Date: 31-07-12

    Input:
    stream0_in:  MF filter for 1st stream, i.e., y0=h0'*y
    stream1_in:  MF filter for 2nd stream, i.e., y1=h1'*y
    ch_mag:      4*h0/sqrt(42), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    ch_mag_i:    4*h1/sqrt(42), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    rho01:       Channel cross correlation, i.e., h1'*h0

    Output:
    stream0_out: output LLRs for 1st stream
  */


  simde__m256i *rho01_256i      = (simde__m256i *)rho01;
  simde__m256i *stream0_256i_in = (simde__m256i *)stream0_in;
  simde__m256i *stream1_256i_in = (simde__m256i *)stream1_in;
  simde__m256i *ch_mag_256i     = (simde__m256i *)ch_mag;
  simde__m256i *ch_mag_256i_i   = (simde__m256i *)ch_mag_i;

  simde__m256i ONE_OVER_SQRT_42 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(10112)); // round(1/sqrt(42)*2^16)
  simde__m256i THREE_OVER_SQRT_42 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(30337)); // round(3/sqrt(42)*2^16)
  simde__m256i FIVE_OVER_SQRT_42 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(25281)); // round(5/sqrt(42)*2^15)
  simde__m256i SEVEN_OVER_SQRT_42 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(17697)); // round(5/sqrt(42)*2^15)
  simde__m256i FORTYNINE_OVER_FOUR_SQRT_42 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(30969)); // round(49/(4*sqrt(42))*2^14), Q2.14
  simde__m256i THIRTYSEVEN_OVER_FOUR_SQRT_42 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(23385)); // round(37/(4*sqrt(42))*2^14), Q2.14
  simde__m256i TWENTYFIVE_OVER_FOUR_SQRT_42 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(31601)); // round(25/(4*sqrt(42))*2^15)
  simde__m256i TWENTYNINE_OVER_FOUR_SQRT_42 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(18329)); // round(29/(4*sqrt(42))*2^15), Q2.14
  simde__m256i SEVENTEEN_OVER_FOUR_SQRT_42 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(21489)); // round(17/(4*sqrt(42))*2^15)
  simde__m256i NINE_OVER_FOUR_SQRT_42 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(11376)); // round(9/(4*sqrt(42))*2^15)
  simde__m256i THIRTEEN_OVER_FOUR_SQRT_42 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(16433)); // round(13/(4*sqrt(42))*2^15)
  simde__m256i FIVE_OVER_FOUR_SQRT_42 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(6320)); // round(5/(4*sqrt(42))*2^15)
  simde__m256i ONE_OVER_FOUR_SQRT_42 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(1264)); // round(1/(4*sqrt(42))*2^15)
  simde__m256i ONE_OVER_SQRT_10_Q15 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(10362)); // round(1/sqrt(10)*2^15)
  simde__m256i THREE_OVER_SQRT_10 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(31086)); // round(3/sqrt(10)*2^15)
  simde__m256i SQRT_10_OVER_FOUR = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(25905)); // round(sqrt(10)/4*2^15)


  simde__m256i ch_mag_int;
  simde__m256i ch_mag_des;
  simde__m256i ch_mag_98_over_42_with_sigma2;
  simde__m256i ch_mag_74_over_42_with_sigma2;
  simde__m256i ch_mag_58_over_42_with_sigma2;
  simde__m256i ch_mag_50_over_42_with_sigma2;
  simde__m256i ch_mag_34_over_42_with_sigma2;
  simde__m256i ch_mag_18_over_42_with_sigma2;
  simde__m256i ch_mag_26_over_42_with_sigma2;
  simde__m256i ch_mag_10_over_42_with_sigma2;
  simde__m256i ch_mag_2_over_42_with_sigma2;
  simde__m256i  y0r_one_over_sqrt_21;
  simde__m256i  y0r_three_over_sqrt_21;
  simde__m256i  y0r_five_over_sqrt_21;
  simde__m256i  y0r_seven_over_sqrt_21;
  simde__m256i  y0i_one_over_sqrt_21;
  simde__m256i  y0i_three_over_sqrt_21;
  simde__m256i  y0i_five_over_sqrt_21;
  simde__m256i  y0i_seven_over_sqrt_21;

  int i,j;
  uint32_t len256 = (length)>>3;

  for (i=0; i<len256; i+=2) {

    // Get rho
    /*
  xmm0 = rho01_128i[i];
  xmm1 = rho01_128i[i+1];
  xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
  xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
  xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
  xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
  xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
  xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
  //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
  //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
  xmm2 = simde_mm_unpacklo_epi64(xmm0,xmm1); // Re(rho)
  xmm3 = simde_mm_unpackhi_epi64(xmm0,xmm1); // Im(rho)
    */
    simde__m256i xmm0, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8;
    simde_mm256_separate_real_imag_parts(&xmm2, &xmm3, rho01_256i[i], rho01_256i[i+1]);

    const simde__m256i rho_rpi = simde_mm256_adds_epi16(xmm2, xmm3); // rho = Re(rho) + Im(rho)
    const simde__m256i rho_rmi = simde_mm256_subs_epi16(xmm2, xmm3); // rho* = Re(rho) - Im(rho)

    // Compute the different rhos
    simde__m256i rho_rpi_1_1 = simde_mm256_mulhi_epi16(rho_rpi, ONE_OVER_SQRT_42);
    simde__m256i rho_rmi_1_1 = simde_mm256_mulhi_epi16(rho_rmi, ONE_OVER_SQRT_42);
    simde__m256i rho_rpi_3_3 = simde_mm256_mulhi_epi16(rho_rpi, THREE_OVER_SQRT_42);
    simde__m256i rho_rmi_3_3 = simde_mm256_mulhi_epi16(rho_rmi, THREE_OVER_SQRT_42);
    simde__m256i rho_rpi_5_5 = simde_mm256_mulhi_epi16(rho_rpi, FIVE_OVER_SQRT_42);
    simde__m256i rho_rmi_5_5 = simde_mm256_mulhi_epi16(rho_rmi, FIVE_OVER_SQRT_42);
    simde__m256i rho_rpi_7_7 = simde_mm256_mulhi_epi16(rho_rpi, SEVEN_OVER_SQRT_42);
    simde__m256i rho_rmi_7_7 = simde_mm256_mulhi_epi16(rho_rmi, SEVEN_OVER_SQRT_42);

    rho_rpi_5_5 = simde_mm256_slli_epi16(rho_rpi_5_5, 1);
    rho_rmi_5_5 = simde_mm256_slli_epi16(rho_rmi_5_5, 1);
    rho_rpi_7_7 = simde_mm256_slli_epi16(rho_rpi_7_7, 2);
    rho_rmi_7_7 = simde_mm256_slli_epi16(rho_rmi_7_7, 2);

    xmm4 = simde_mm256_mulhi_epi16(xmm2, ONE_OVER_SQRT_42);
    xmm5 = simde_mm256_mulhi_epi16(xmm3, ONE_OVER_SQRT_42);
    xmm6 = simde_mm256_mulhi_epi16(xmm3, THREE_OVER_SQRT_42);
    xmm7 = simde_mm256_mulhi_epi16(xmm3, FIVE_OVER_SQRT_42);
    xmm8 = simde_mm256_mulhi_epi16(xmm3, SEVEN_OVER_SQRT_42);
    xmm7 = simde_mm256_slli_epi16(xmm7, 1);
    xmm8 = simde_mm256_slli_epi16(xmm8, 2);

    simde__m256i rho_rpi_1_3 = simde_mm256_adds_epi16(xmm4, xmm6);
    simde__m256i rho_rmi_1_3 = simde_mm256_subs_epi16(xmm4, xmm6);
    simde__m256i rho_rpi_1_5 = simde_mm256_adds_epi16(xmm4, xmm7);
    simde__m256i rho_rmi_1_5 = simde_mm256_subs_epi16(xmm4, xmm7);
    simde__m256i rho_rpi_1_7 = simde_mm256_adds_epi16(xmm4, xmm8);
    simde__m256i rho_rmi_1_7 = simde_mm256_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm256_mulhi_epi16(xmm2, THREE_OVER_SQRT_42);
    simde__m256i rho_rpi_3_1 = simde_mm256_adds_epi16(xmm4, xmm5);
    simde__m256i rho_rmi_3_1 = simde_mm256_subs_epi16(xmm4, xmm5);
    simde__m256i rho_rpi_3_5 = simde_mm256_adds_epi16(xmm4, xmm7);
    simde__m256i rho_rmi_3_5 = simde_mm256_subs_epi16(xmm4, xmm7);
    simde__m256i rho_rpi_3_7 = simde_mm256_adds_epi16(xmm4, xmm8);
    simde__m256i rho_rmi_3_7 = simde_mm256_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm256_mulhi_epi16(xmm2, FIVE_OVER_SQRT_42);
    xmm4 = simde_mm256_slli_epi16(xmm4, 1);
    simde__m256i rho_rpi_5_1 = simde_mm256_adds_epi16(xmm4, xmm5);
    simde__m256i rho_rmi_5_1 = simde_mm256_subs_epi16(xmm4, xmm5);
    simde__m256i rho_rpi_5_3 = simde_mm256_adds_epi16(xmm4, xmm6);
    simde__m256i rho_rmi_5_3 = simde_mm256_subs_epi16(xmm4, xmm6);
    simde__m256i rho_rpi_5_7 = simde_mm256_adds_epi16(xmm4, xmm8);
    simde__m256i rho_rmi_5_7 = simde_mm256_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm256_mulhi_epi16(xmm2, SEVEN_OVER_SQRT_42);
    xmm4 = simde_mm256_slli_epi16(xmm4, 2);
    simde__m256i rho_rpi_7_1 = simde_mm256_adds_epi16(xmm4, xmm5);
    simde__m256i rho_rmi_7_1 = simde_mm256_subs_epi16(xmm4, xmm5);
    simde__m256i rho_rpi_7_3 = simde_mm256_adds_epi16(xmm4, xmm6);
    simde__m256i rho_rmi_7_3 = simde_mm256_subs_epi16(xmm4, xmm6);
    simde__m256i rho_rpi_7_5 = simde_mm256_adds_epi16(xmm4, xmm7);
    simde__m256i rho_rmi_7_5 = simde_mm256_subs_epi16(xmm4, xmm7);

    // Rearrange interfering MF output
    /*
    xmm0 = stream1_128i_in[i];
    xmm1 = stream1_128i_in[i+1];
    xmm0 = simde_mm256_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm256_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm256_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm256_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm256_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm256_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    y1r = simde_mm256_unpacklo_epi64(xmm0,xmm1); //[y1r(1),y1r(2),y1r(3),y1r(4)]
    y1i = simde_mm256_unpackhi_epi64(xmm0,xmm1); //[y1i(1),y1i(2),y1i(3),y1i(4)]
    */

    simde__m256i y1r, y1i;
    simde_mm256_separate_real_imag_parts(&y1r, &y1i, stream1_256i_in[i], stream1_256i_in[i+1]);

    // Psi_r calculation from rho_rpi or rho_rmi
    xmm0 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(0));// ZERO for abs_pi16
    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_7, y1r);
    simde__m256i psi_r_p7_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_5, y1r);
    simde__m256i psi_r_p7_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_3, y1r);
    simde__m256i psi_r_p7_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_1, y1r);
    simde__m256i psi_r_p7_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_1, y1r);
    simde__m256i psi_r_p7_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_3, y1r);
    simde__m256i psi_r_p7_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_5, y1r);
    simde__m256i psi_r_p7_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_7, y1r);
    simde__m256i psi_r_p7_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_7, y1r);
    simde__m256i psi_r_p5_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_5, y1r);
    simde__m256i psi_r_p5_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_3, y1r);
    simde__m256i psi_r_p5_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_1, y1r);
    simde__m256i psi_r_p5_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_1, y1r);
    simde__m256i psi_r_p5_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_3, y1r);
    simde__m256i psi_r_p5_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_5, y1r);
    simde__m256i psi_r_p5_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_7, y1r);
    simde__m256i psi_r_p5_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_7, y1r);
    simde__m256i psi_r_p3_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_5, y1r);
    simde__m256i psi_r_p3_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_3, y1r);
    simde__m256i psi_r_p3_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_1, y1r);
    simde__m256i psi_r_p3_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_1, y1r);
    simde__m256i psi_r_p3_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_3, y1r);
    simde__m256i psi_r_p3_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_5, y1r);
    simde__m256i psi_r_p3_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_7, y1r);
    simde__m256i psi_r_p3_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_7, y1r);
    simde__m256i psi_r_p1_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_5, y1r);
    simde__m256i psi_r_p1_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_3, y1r);
    simde__m256i psi_r_p1_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_1, y1r);
    simde__m256i psi_r_p1_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_1, y1r);
    simde__m256i psi_r_p1_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_3, y1r);
    simde__m256i psi_r_p1_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_5, y1r);
    simde__m256i psi_r_p1_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_7, y1r);
    simde__m256i psi_r_p1_m7 = simde_mm256_abs_epi16(xmm2);

    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_7, y1r);
    simde__m256i psi_r_m1_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_5, y1r);
    simde__m256i psi_r_m1_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_3, y1r);
    simde__m256i psi_r_m1_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_1, y1r);
    simde__m256i psi_r_m1_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_1, y1r);
    simde__m256i psi_r_m1_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_3, y1r);
    simde__m256i psi_r_m1_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_5, y1r);
    simde__m256i psi_r_m1_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_7, y1r);
    simde__m256i psi_r_m1_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_7, y1r);
    simde__m256i psi_r_m3_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_5, y1r);
    simde__m256i psi_r_m3_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_3, y1r);
    simde__m256i psi_r_m3_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_1, y1r);
    simde__m256i psi_r_m3_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_1, y1r);
    simde__m256i psi_r_m3_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_3, y1r);
    simde__m256i psi_r_m3_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_5, y1r);
    simde__m256i psi_r_m3_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_7, y1r);
    simde__m256i psi_r_m3_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_7, y1r);
    simde__m256i psi_r_m5_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_5, y1r);
    simde__m256i psi_r_m5_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_3, y1r);
    simde__m256i psi_r_m5_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_1, y1r);
    simde__m256i psi_r_m5_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_1, y1r);
    simde__m256i psi_r_m5_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_3, y1r);
    simde__m256i psi_r_m5_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_5, y1r);
    simde__m256i psi_r_m5_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_7, y1r);
    simde__m256i psi_r_m5_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_7, y1r);
    simde__m256i psi_r_m7_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_5, y1r);
    simde__m256i psi_r_m7_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_3, y1r);
    simde__m256i psi_r_m7_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_1, y1r);
    simde__m256i psi_r_m7_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_1, y1r);
    simde__m256i psi_r_m7_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_3, y1r);
    simde__m256i psi_r_m7_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_5, y1r);
    simde__m256i psi_r_m7_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_7, y1r);
    simde__m256i psi_r_m7_m7 = simde_mm256_abs_epi16(xmm2);

    // Psi_i calculation from rho_rpi or rho_rmi
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_7, y1i);
    simde__m256i psi_i_p7_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_7, y1i);
    simde__m256i psi_i_p7_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_7, y1i);
    simde__m256i psi_i_p7_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_7, y1i);
    simde__m256i psi_i_p7_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_7, y1i);
    simde__m256i psi_i_p7_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_7, y1i);
    simde__m256i psi_i_p7_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_7, y1i);
    simde__m256i psi_i_p7_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_7, y1i);
    simde__m256i psi_i_p7_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_5, y1i);
    simde__m256i psi_i_p5_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_5, y1i);
    simde__m256i psi_i_p5_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_5, y1i);
    simde__m256i psi_i_p5_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_5, y1i);
    simde__m256i psi_i_p5_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_5, y1i);
    simde__m256i psi_i_p5_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_5, y1i);
    simde__m256i psi_i_p5_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_5, y1i);
    simde__m256i psi_i_p5_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_5, y1i);
    simde__m256i psi_i_p5_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_3, y1i);
    simde__m256i psi_i_p3_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_3, y1i);
    simde__m256i psi_i_p3_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_3, y1i);
    simde__m256i psi_i_p3_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_3, y1i);
    simde__m256i psi_i_p3_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_3, y1i);
    simde__m256i psi_i_p3_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_3, y1i);
    simde__m256i psi_i_p3_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_3, y1i);
    simde__m256i psi_i_p3_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_3, y1i);
    simde__m256i psi_i_p3_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_1, y1i);
    simde__m256i psi_i_p1_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_1, y1i);
    simde__m256i psi_i_p1_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_1, y1i);
    simde__m256i psi_i_p1_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_1, y1i);
    simde__m256i psi_i_p1_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_1, y1i);
    simde__m256i psi_i_p1_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_1, y1i);
    simde__m256i psi_i_p1_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_1, y1i);
    simde__m256i psi_i_p1_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_1, y1i);
    simde__m256i psi_i_p1_m7 = simde_mm256_abs_epi16(xmm2);

    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_1, y1i);
    simde__m256i psi_i_m1_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_1, y1i);
    simde__m256i psi_i_m1_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_1, y1i);
    simde__m256i psi_i_m1_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_1, y1i);
    simde__m256i psi_i_m1_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_1, y1i);
    simde__m256i psi_i_m1_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_1, y1i);
    simde__m256i psi_i_m1_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_1, y1i);
    simde__m256i psi_i_m1_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_1, y1i);
    simde__m256i psi_i_m1_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_3, y1i);
    simde__m256i psi_i_m3_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_3, y1i);
    simde__m256i psi_i_m3_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_3, y1i);
    simde__m256i psi_i_m3_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_3, y1i);
    simde__m256i psi_i_m3_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_3, y1i);
    simde__m256i psi_i_m3_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_3, y1i);
    simde__m256i psi_i_m3_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_3, y1i);
    simde__m256i psi_i_m3_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_3, y1i);
    simde__m256i psi_i_m3_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_5, y1i);
    simde__m256i psi_i_m5_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_5, y1i);
    simde__m256i psi_i_m5_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_5, y1i);
    simde__m256i psi_i_m5_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_5, y1i);
    simde__m256i psi_i_m5_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_5, y1i);
    simde__m256i psi_i_m5_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_5, y1i);
    simde__m256i psi_i_m5_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_5, y1i);
    simde__m256i psi_i_m5_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_5, y1i);
    simde__m256i psi_i_m5_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_7, y1i);
    simde__m256i psi_i_m7_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_7, y1i);
    simde__m256i psi_i_m7_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_7, y1i);
    simde__m256i psi_i_m7_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_7, y1i);
    simde__m256i psi_i_m7_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_7, y1i);
    simde__m256i psi_i_m7_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_7, y1i);
    simde__m256i psi_i_m7_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_7, y1i);
    simde__m256i psi_i_m7_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_7, y1i);
    simde__m256i psi_i_m7_m7 = simde_mm256_abs_epi16(xmm2);

    /*
        // Rearrange desired MF output
        xmm0 = stream0_128i_in[i];
        xmm1 = stream0_128i_in[i+1];
        xmm0 = simde_mm256_shufflelo_epi16(xmm0,0xd8); //_MM_SHUFFLE(0,2,1,3));
        xmm0 = simde_mm256_shufflehi_epi16(xmm0,0xd8); //_MM_SHUFFLE(0,2,1,3));
        xmm0 = simde_mm256_shuffle_epi32(xmm0,0xd8); //_MM_SHUFFLE(0,2,1,3));
        xmm1 = simde_mm256_shufflelo_epi16(xmm1,0xd8); //_MM_SHUFFLE(0,2,1,3));
        xmm1 = simde_mm256_shufflehi_epi16(xmm1,0xd8); //_MM_SHUFFLE(0,2,1,3));
        xmm1 = simde_mm256_shuffle_epi32(xmm1,0xd8); //_MM_SHUFFLE(0,2,1,3));
        //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
        //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
        y0r = simde_mm256_unpacklo_epi64(xmm0,xmm1); // = [y0r(1),y0r(2),y0r(3),y0r(4)]
        y0i = simde_mm256_unpackhi_epi64(xmm0,xmm1);
    */
    simde__m256i y0r, y0i;
    simde_mm256_separate_real_imag_parts(&y0r, &y0i, stream0_256i_in[i], stream0_256i_in[i+1]);

    /*
    // Rearrange desired channel magnitudes
    xmm2 = ch_mag_128i[i]; // = [|h|^2(1),|h|^2(1),|h|^2(2),|h|^2(2)]*(2/sqrt(10))
    xmm3 = ch_mag_128i[i+1]; // = [|h|^2(3),|h|^2(3),|h|^2(4),|h|^2(4)]*(2/sqrt(10))
    xmm2 = simde_mm256_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm256_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm256_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm256_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm256_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm256_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    ch_mag_des = simde_mm256_unpacklo_epi64(xmm2,xmm3);
    */

    simde_mm256_separate_real_imag_parts(&ch_mag_des, &xmm2, ch_mag_256i[i], ch_mag_256i[i+1]);

    // Rearrange interfering channel magnitudes
    /*
    xmm2 = ch_mag_128i_i[i];
    xmm3 = ch_mag_128i_i[i+1];
    xmm2 = simde_mm256_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm256_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm256_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm256_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm256_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm256_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    ch_mag_int  = simde_mm256_unpacklo_epi64(xmm2,xmm3);
    */

    simde_mm256_separate_real_imag_parts(&ch_mag_int, &xmm2, ch_mag_256i_i[i], ch_mag_256i_i[i+1]);

    y0r_one_over_sqrt_21   = simde_mm256_mulhi_epi16(y0r, ONE_OVER_SQRT_42);
    y0r_three_over_sqrt_21 = simde_mm256_mulhi_epi16(y0r, THREE_OVER_SQRT_42);
    y0r_five_over_sqrt_21  = simde_mm256_mulhi_epi16(y0r, FIVE_OVER_SQRT_42);
    y0r_five_over_sqrt_21  = simde_mm256_slli_epi16(y0r_five_over_sqrt_21, 1);
    y0r_seven_over_sqrt_21 = simde_mm256_mulhi_epi16(y0r, SEVEN_OVER_SQRT_42);
    y0r_seven_over_sqrt_21 = simde_mm256_slli_epi16(y0r_seven_over_sqrt_21, 2); // Q2.14

    y0i_one_over_sqrt_21   = simde_mm256_mulhi_epi16(y0i, ONE_OVER_SQRT_42);
    y0i_three_over_sqrt_21 = simde_mm256_mulhi_epi16(y0i, THREE_OVER_SQRT_42);
    y0i_five_over_sqrt_21  = simde_mm256_mulhi_epi16(y0i, FIVE_OVER_SQRT_42);
    y0i_five_over_sqrt_21  = simde_mm256_slli_epi16(y0i_five_over_sqrt_21, 1);
    y0i_seven_over_sqrt_21 = simde_mm256_mulhi_epi16(y0i, SEVEN_OVER_SQRT_42);
    y0i_seven_over_sqrt_21 = simde_mm256_slli_epi16(y0i_seven_over_sqrt_21, 2); // Q2.14

    simde__m256i y0_p_7_1 = simde_mm256_adds_epi16(y0r_seven_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_p_7_3 = simde_mm256_adds_epi16(y0r_seven_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_p_7_5 = simde_mm256_adds_epi16(y0r_seven_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_p_7_7 = simde_mm256_adds_epi16(y0r_seven_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m256i y0_p_5_1 = simde_mm256_adds_epi16(y0r_five_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_p_5_3 = simde_mm256_adds_epi16(y0r_five_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_p_5_5 = simde_mm256_adds_epi16(y0r_five_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_p_5_7 = simde_mm256_adds_epi16(y0r_five_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m256i y0_p_3_1 = simde_mm256_adds_epi16(y0r_three_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_p_3_3 = simde_mm256_adds_epi16(y0r_three_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_p_3_5 = simde_mm256_adds_epi16(y0r_three_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_p_3_7 = simde_mm256_adds_epi16(y0r_three_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m256i y0_p_1_1 = simde_mm256_adds_epi16(y0r_one_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_p_1_3 = simde_mm256_adds_epi16(y0r_one_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_p_1_5 = simde_mm256_adds_epi16(y0r_one_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_p_1_7 = simde_mm256_adds_epi16(y0r_one_over_sqrt_21, y0i_seven_over_sqrt_21);

    simde__m256i y0_m_1_1 = simde_mm256_subs_epi16(y0r_one_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_m_1_3 = simde_mm256_subs_epi16(y0r_one_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_m_1_5 = simde_mm256_subs_epi16(y0r_one_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_m_1_7 = simde_mm256_subs_epi16(y0r_one_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m256i y0_m_3_1 = simde_mm256_subs_epi16(y0r_three_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_m_3_3 = simde_mm256_subs_epi16(y0r_three_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_m_3_5 = simde_mm256_subs_epi16(y0r_three_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_m_3_7 = simde_mm256_subs_epi16(y0r_three_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m256i y0_m_5_1 = simde_mm256_subs_epi16(y0r_five_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_m_5_3 = simde_mm256_subs_epi16(y0r_five_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_m_5_5 = simde_mm256_subs_epi16(y0r_five_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_m_5_7 = simde_mm256_subs_epi16(y0r_five_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m256i y0_m_7_1 = simde_mm256_subs_epi16(y0r_seven_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_m_7_3 = simde_mm256_subs_epi16(y0r_seven_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_m_7_5 = simde_mm256_subs_epi16(y0r_seven_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_m_7_7 = simde_mm256_subs_epi16(y0r_seven_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m256i tmp_result, tmp_result2;
    interference_abs_epi16(psi_r_p7_p7, ch_mag_int, a_r_p7_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p7_p5, ch_mag_int, a_r_p7_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p7_p3, ch_mag_int, a_r_p7_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p7_p1, ch_mag_int, a_r_p7_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p7_m1, ch_mag_int, a_r_p7_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p7_m3, ch_mag_int, a_r_p7_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p7_m5, ch_mag_int, a_r_p7_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p7_m7, ch_mag_int, a_r_p7_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_p7, ch_mag_int, a_r_p5_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_p5, ch_mag_int, a_r_p5_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_p3, ch_mag_int, a_r_p5_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_p1, ch_mag_int, a_r_p5_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_m1, ch_mag_int, a_r_p5_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_m3, ch_mag_int, a_r_p5_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_m5, ch_mag_int, a_r_p5_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p5_m7, ch_mag_int, a_r_p5_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_p7, ch_mag_int, a_r_p3_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_p5, ch_mag_int, a_r_p3_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_p3, ch_mag_int, a_r_p3_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_p1, ch_mag_int, a_r_p3_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_m1, ch_mag_int, a_r_p3_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_m3, ch_mag_int, a_r_p3_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_m5, ch_mag_int, a_r_p3_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p3_m7, ch_mag_int, a_r_p3_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_p7, ch_mag_int, a_r_p1_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_p5, ch_mag_int, a_r_p1_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_p3, ch_mag_int, a_r_p1_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_p1, ch_mag_int, a_r_p1_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_m1, ch_mag_int, a_r_p1_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_m3, ch_mag_int, a_r_p1_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_m5, ch_mag_int, a_r_p1_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_p1_m7, ch_mag_int, a_r_p1_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_p7, ch_mag_int, a_r_m1_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_p5, ch_mag_int, a_r_m1_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_p3, ch_mag_int, a_r_m1_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_p1, ch_mag_int, a_r_m1_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_m1, ch_mag_int, a_r_m1_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_m3, ch_mag_int, a_r_m1_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_m5, ch_mag_int, a_r_m1_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m1_m7, ch_mag_int, a_r_m1_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_p7, ch_mag_int, a_r_m3_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_p5, ch_mag_int, a_r_m3_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_p3, ch_mag_int, a_r_m3_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_p1, ch_mag_int, a_r_m3_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_m1, ch_mag_int, a_r_m3_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_m3, ch_mag_int, a_r_m3_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_m5, ch_mag_int, a_r_m3_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m3_m7, ch_mag_int, a_r_m3_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_p7, ch_mag_int, a_r_m5_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_p5, ch_mag_int, a_r_m5_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_p3, ch_mag_int, a_r_m5_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_p1, ch_mag_int, a_r_m5_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_m1, ch_mag_int, a_r_m5_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_m3, ch_mag_int, a_r_m5_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_m5, ch_mag_int, a_r_m5_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m5_m7, ch_mag_int, a_r_m5_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_p7, ch_mag_int, a_r_m7_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_p5, ch_mag_int, a_r_m7_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_p3, ch_mag_int, a_r_m7_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_p1, ch_mag_int, a_r_m7_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_m1, ch_mag_int, a_r_m7_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_m3, ch_mag_int, a_r_m7_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_m5, ch_mag_int, a_r_m7_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_r_m7_m7, ch_mag_int, a_r_m7_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);

    interference_abs_epi16(psi_i_p7_p7, ch_mag_int, a_i_p7_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p7_p5, ch_mag_int, a_i_p7_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p7_p3, ch_mag_int, a_i_p7_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p7_p1, ch_mag_int, a_i_p7_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p7_m1, ch_mag_int, a_i_p7_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p7_m3, ch_mag_int, a_i_p7_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p7_m5, ch_mag_int, a_i_p7_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p7_m7, ch_mag_int, a_i_p7_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_p7, ch_mag_int, a_i_p5_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_p5, ch_mag_int, a_i_p5_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_p3, ch_mag_int, a_i_p5_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_p1, ch_mag_int, a_i_p5_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_m1, ch_mag_int, a_i_p5_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_m3, ch_mag_int, a_i_p5_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_m5, ch_mag_int, a_i_p5_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p5_m7, ch_mag_int, a_i_p5_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_p7, ch_mag_int, a_i_p3_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_p5, ch_mag_int, a_i_p3_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_p3, ch_mag_int, a_i_p3_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_p1, ch_mag_int, a_i_p3_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_m1, ch_mag_int, a_i_p3_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_m3, ch_mag_int, a_i_p3_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_m5, ch_mag_int, a_i_p3_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p3_m7, ch_mag_int, a_i_p3_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_p7, ch_mag_int, a_i_p1_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_p5, ch_mag_int, a_i_p1_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_p3, ch_mag_int, a_i_p1_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_p1, ch_mag_int, a_i_p1_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_m1, ch_mag_int, a_i_p1_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_m3, ch_mag_int, a_i_p1_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_m5, ch_mag_int, a_i_p1_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_p1_m7, ch_mag_int, a_i_p1_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_p7, ch_mag_int, a_i_m1_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_p5, ch_mag_int, a_i_m1_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_p3, ch_mag_int, a_i_m1_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_p1, ch_mag_int, a_i_m1_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_m1, ch_mag_int, a_i_m1_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_m3, ch_mag_int, a_i_m1_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_m5, ch_mag_int, a_i_m1_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m1_m7, ch_mag_int, a_i_m1_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_p7, ch_mag_int, a_i_m3_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_p5, ch_mag_int, a_i_m3_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_p3, ch_mag_int, a_i_m3_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_p1, ch_mag_int, a_i_m3_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_m1, ch_mag_int, a_i_m3_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_m3, ch_mag_int, a_i_m3_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_m5, ch_mag_int, a_i_m3_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m3_m7, ch_mag_int, a_i_m3_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_p7, ch_mag_int, a_i_m5_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_p5, ch_mag_int, a_i_m5_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_p3, ch_mag_int, a_i_m5_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_p1, ch_mag_int, a_i_m5_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_m1, ch_mag_int, a_i_m5_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_m3, ch_mag_int, a_i_m5_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_m5, ch_mag_int, a_i_m5_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m5_m7, ch_mag_int, a_i_m5_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_p7, ch_mag_int, a_i_m7_p7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_p5, ch_mag_int, a_i_m7_p5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_p3, ch_mag_int, a_i_m7_p3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_p1, ch_mag_int, a_i_m7_p1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_m1, ch_mag_int, a_i_m7_m1, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_m3, ch_mag_int, a_i_m7_m3, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_m5, ch_mag_int, a_i_m7_m5, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);
    interference_abs_epi16(psi_i_m7_m7, ch_mag_int, a_i_m7_m7, ONE_OVER_SQRT_10_Q15, THREE_OVER_SQRT_10);

    // Calculation of a group of two terms in the bit metric involving product of psi and interference
    prodsum_psi_a_epi16(psi_r_p7_p7, a_r_p7_p7, psi_i_p7_p7, a_i_p7_p7, psi_a_p7_p7);
    prodsum_psi_a_epi16(psi_r_p7_p5, a_r_p7_p5, psi_i_p7_p5, a_i_p7_p5, psi_a_p7_p5);
    prodsum_psi_a_epi16(psi_r_p7_p3, a_r_p7_p3, psi_i_p7_p3, a_i_p7_p3, psi_a_p7_p3);
    prodsum_psi_a_epi16(psi_r_p7_p1, a_r_p7_p1, psi_i_p7_p1, a_i_p7_p1, psi_a_p7_p1);
    prodsum_psi_a_epi16(psi_r_p7_m1, a_r_p7_m1, psi_i_p7_m1, a_i_p7_m1, psi_a_p7_m1);
    prodsum_psi_a_epi16(psi_r_p7_m3, a_r_p7_m3, psi_i_p7_m3, a_i_p7_m3, psi_a_p7_m3);
    prodsum_psi_a_epi16(psi_r_p7_m5, a_r_p7_m5, psi_i_p7_m5, a_i_p7_m5, psi_a_p7_m5);
    prodsum_psi_a_epi16(psi_r_p7_m7, a_r_p7_m7, psi_i_p7_m7, a_i_p7_m7, psi_a_p7_m7);
    prodsum_psi_a_epi16(psi_r_p5_p7, a_r_p5_p7, psi_i_p5_p7, a_i_p5_p7, psi_a_p5_p7);
    prodsum_psi_a_epi16(psi_r_p5_p5, a_r_p5_p5, psi_i_p5_p5, a_i_p5_p5, psi_a_p5_p5);
    prodsum_psi_a_epi16(psi_r_p5_p3, a_r_p5_p3, psi_i_p5_p3, a_i_p5_p3, psi_a_p5_p3);
    prodsum_psi_a_epi16(psi_r_p5_p1, a_r_p5_p1, psi_i_p5_p1, a_i_p5_p1, psi_a_p5_p1);
    prodsum_psi_a_epi16(psi_r_p5_m1, a_r_p5_m1, psi_i_p5_m1, a_i_p5_m1, psi_a_p5_m1);
    prodsum_psi_a_epi16(psi_r_p5_m3, a_r_p5_m3, psi_i_p5_m3, a_i_p5_m3, psi_a_p5_m3);
    prodsum_psi_a_epi16(psi_r_p5_m5, a_r_p5_m5, psi_i_p5_m5, a_i_p5_m5, psi_a_p5_m5);
    prodsum_psi_a_epi16(psi_r_p5_m7, a_r_p5_m7, psi_i_p5_m7, a_i_p5_m7, psi_a_p5_m7);
    prodsum_psi_a_epi16(psi_r_p3_p7, a_r_p3_p7, psi_i_p3_p7, a_i_p3_p7, psi_a_p3_p7);
    prodsum_psi_a_epi16(psi_r_p3_p5, a_r_p3_p5, psi_i_p3_p5, a_i_p3_p5, psi_a_p3_p5);
    prodsum_psi_a_epi16(psi_r_p3_p3, a_r_p3_p3, psi_i_p3_p3, a_i_p3_p3, psi_a_p3_p3);
    prodsum_psi_a_epi16(psi_r_p3_p1, a_r_p3_p1, psi_i_p3_p1, a_i_p3_p1, psi_a_p3_p1);
    prodsum_psi_a_epi16(psi_r_p3_m1, a_r_p3_m1, psi_i_p3_m1, a_i_p3_m1, psi_a_p3_m1);
    prodsum_psi_a_epi16(psi_r_p3_m3, a_r_p3_m3, psi_i_p3_m3, a_i_p3_m3, psi_a_p3_m3);
    prodsum_psi_a_epi16(psi_r_p3_m5, a_r_p3_m5, psi_i_p3_m5, a_i_p3_m5, psi_a_p3_m5);
    prodsum_psi_a_epi16(psi_r_p3_m7, a_r_p3_m7, psi_i_p3_m7, a_i_p3_m7, psi_a_p3_m7);
    prodsum_psi_a_epi16(psi_r_p1_p7, a_r_p1_p7, psi_i_p1_p7, a_i_p1_p7, psi_a_p1_p7);
    prodsum_psi_a_epi16(psi_r_p1_p5, a_r_p1_p5, psi_i_p1_p5, a_i_p1_p5, psi_a_p1_p5);
    prodsum_psi_a_epi16(psi_r_p1_p3, a_r_p1_p3, psi_i_p1_p3, a_i_p1_p3, psi_a_p1_p3);
    prodsum_psi_a_epi16(psi_r_p1_p1, a_r_p1_p1, psi_i_p1_p1, a_i_p1_p1, psi_a_p1_p1);
    prodsum_psi_a_epi16(psi_r_p1_m1, a_r_p1_m1, psi_i_p1_m1, a_i_p1_m1, psi_a_p1_m1);
    prodsum_psi_a_epi16(psi_r_p1_m3, a_r_p1_m3, psi_i_p1_m3, a_i_p1_m3, psi_a_p1_m3);
    prodsum_psi_a_epi16(psi_r_p1_m5, a_r_p1_m5, psi_i_p1_m5, a_i_p1_m5, psi_a_p1_m5);
    prodsum_psi_a_epi16(psi_r_p1_m7, a_r_p1_m7, psi_i_p1_m7, a_i_p1_m7, psi_a_p1_m7);
    prodsum_psi_a_epi16(psi_r_m1_p7, a_r_m1_p7, psi_i_m1_p7, a_i_m1_p7, psi_a_m1_p7);
    prodsum_psi_a_epi16(psi_r_m1_p5, a_r_m1_p5, psi_i_m1_p5, a_i_m1_p5, psi_a_m1_p5);
    prodsum_psi_a_epi16(psi_r_m1_p3, a_r_m1_p3, psi_i_m1_p3, a_i_m1_p3, psi_a_m1_p3);
    prodsum_psi_a_epi16(psi_r_m1_p1, a_r_m1_p1, psi_i_m1_p1, a_i_m1_p1, psi_a_m1_p1);
    prodsum_psi_a_epi16(psi_r_m1_m1, a_r_m1_m1, psi_i_m1_m1, a_i_m1_m1, psi_a_m1_m1);
    prodsum_psi_a_epi16(psi_r_m1_m3, a_r_m1_m3, psi_i_m1_m3, a_i_m1_m3, psi_a_m1_m3);
    prodsum_psi_a_epi16(psi_r_m1_m5, a_r_m1_m5, psi_i_m1_m5, a_i_m1_m5, psi_a_m1_m5);
    prodsum_psi_a_epi16(psi_r_m1_m7, a_r_m1_m7, psi_i_m1_m7, a_i_m1_m7, psi_a_m1_m7);
    prodsum_psi_a_epi16(psi_r_m3_p7, a_r_m3_p7, psi_i_m3_p7, a_i_m3_p7, psi_a_m3_p7);
    prodsum_psi_a_epi16(psi_r_m3_p5, a_r_m3_p5, psi_i_m3_p5, a_i_m3_p5, psi_a_m3_p5);
    prodsum_psi_a_epi16(psi_r_m3_p3, a_r_m3_p3, psi_i_m3_p3, a_i_m3_p3, psi_a_m3_p3);
    prodsum_psi_a_epi16(psi_r_m3_p1, a_r_m3_p1, psi_i_m3_p1, a_i_m3_p1, psi_a_m3_p1);
    prodsum_psi_a_epi16(psi_r_m3_m1, a_r_m3_m1, psi_i_m3_m1, a_i_m3_m1, psi_a_m3_m1);
    prodsum_psi_a_epi16(psi_r_m3_m3, a_r_m3_m3, psi_i_m3_m3, a_i_m3_m3, psi_a_m3_m3);
    prodsum_psi_a_epi16(psi_r_m3_m5, a_r_m3_m5, psi_i_m3_m5, a_i_m3_m5, psi_a_m3_m5);
    prodsum_psi_a_epi16(psi_r_m3_m7, a_r_m3_m7, psi_i_m3_m7, a_i_m3_m7, psi_a_m3_m7);
    prodsum_psi_a_epi16(psi_r_m5_p7, a_r_m5_p7, psi_i_m5_p7, a_i_m5_p7, psi_a_m5_p7);
    prodsum_psi_a_epi16(psi_r_m5_p5, a_r_m5_p5, psi_i_m5_p5, a_i_m5_p5, psi_a_m5_p5);
    prodsum_psi_a_epi16(psi_r_m5_p3, a_r_m5_p3, psi_i_m5_p3, a_i_m5_p3, psi_a_m5_p3);
    prodsum_psi_a_epi16(psi_r_m5_p1, a_r_m5_p1, psi_i_m5_p1, a_i_m5_p1, psi_a_m5_p1);
    prodsum_psi_a_epi16(psi_r_m5_m1, a_r_m5_m1, psi_i_m5_m1, a_i_m5_m1, psi_a_m5_m1);
    prodsum_psi_a_epi16(psi_r_m5_m3, a_r_m5_m3, psi_i_m5_m3, a_i_m5_m3, psi_a_m5_m3);
    prodsum_psi_a_epi16(psi_r_m5_m5, a_r_m5_m5, psi_i_m5_m5, a_i_m5_m5, psi_a_m5_m5);
    prodsum_psi_a_epi16(psi_r_m5_m7, a_r_m5_m7, psi_i_m5_m7, a_i_m5_m7, psi_a_m5_m7);
    prodsum_psi_a_epi16(psi_r_m7_p7, a_r_m7_p7, psi_i_m7_p7, a_i_m7_p7, psi_a_m7_p7);
    prodsum_psi_a_epi16(psi_r_m7_p5, a_r_m7_p5, psi_i_m7_p5, a_i_m7_p5, psi_a_m7_p5);
    prodsum_psi_a_epi16(psi_r_m7_p3, a_r_m7_p3, psi_i_m7_p3, a_i_m7_p3, psi_a_m7_p3);
    prodsum_psi_a_epi16(psi_r_m7_p1, a_r_m7_p1, psi_i_m7_p1, a_i_m7_p1, psi_a_m7_p1);
    prodsum_psi_a_epi16(psi_r_m7_m1, a_r_m7_m1, psi_i_m7_m1, a_i_m7_m1, psi_a_m7_m1);
    prodsum_psi_a_epi16(psi_r_m7_m3, a_r_m7_m3, psi_i_m7_m3, a_i_m7_m3, psi_a_m7_m3);
    prodsum_psi_a_epi16(psi_r_m7_m5, a_r_m7_m5, psi_i_m7_m5, a_i_m7_m5, psi_a_m7_m5);
    prodsum_psi_a_epi16(psi_r_m7_m7, a_r_m7_m7, psi_i_m7_m7, a_i_m7_m7, psi_a_m7_m7);

    // Calculation of a group of two terms in the bit metric involving squares of interference
    square_a_epi16(a_r_p7_p7, a_i_p7_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_p7);
    square_a_epi16(a_r_p7_p5, a_i_p7_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_p5);
    square_a_epi16(a_r_p7_p3, a_i_p7_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_p3);
    square_a_epi16(a_r_p7_p1, a_i_p7_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_p1);
    square_a_epi16(a_r_p7_m1, a_i_p7_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_m1);
    square_a_epi16(a_r_p7_m3, a_i_p7_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_m3);
    square_a_epi16(a_r_p7_m5, a_i_p7_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_m5);
    square_a_epi16(a_r_p7_m7, a_i_p7_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p7_m7);
    square_a_epi16(a_r_p5_p7, a_i_p5_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_p7);
    square_a_epi16(a_r_p5_p5, a_i_p5_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_p5);
    square_a_epi16(a_r_p5_p3, a_i_p5_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_p3);
    square_a_epi16(a_r_p5_p1, a_i_p5_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_p1);
    square_a_epi16(a_r_p5_m1, a_i_p5_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_m1);
    square_a_epi16(a_r_p5_m3, a_i_p5_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_m3);
    square_a_epi16(a_r_p5_m5, a_i_p5_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_m5);
    square_a_epi16(a_r_p5_m7, a_i_p5_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p5_m7);
    square_a_epi16(a_r_p3_p7, a_i_p3_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_p7);
    square_a_epi16(a_r_p3_p5, a_i_p3_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_p5);
    square_a_epi16(a_r_p3_p3, a_i_p3_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_p3);
    square_a_epi16(a_r_p3_p1, a_i_p3_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_p1);
    square_a_epi16(a_r_p3_m1, a_i_p3_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_m1);
    square_a_epi16(a_r_p3_m3, a_i_p3_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_m3);
    square_a_epi16(a_r_p3_m5, a_i_p3_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_m5);
    square_a_epi16(a_r_p3_m7, a_i_p3_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p3_m7);
    square_a_epi16(a_r_p1_p7, a_i_p1_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_p7);
    square_a_epi16(a_r_p1_p5, a_i_p1_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_p5);
    square_a_epi16(a_r_p1_p3, a_i_p1_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_p3);
    square_a_epi16(a_r_p1_p1, a_i_p1_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_p1);
    square_a_epi16(a_r_p1_m1, a_i_p1_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_m1);
    square_a_epi16(a_r_p1_m3, a_i_p1_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_m3);
    square_a_epi16(a_r_p1_m5, a_i_p1_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_m5);
    square_a_epi16(a_r_p1_m7, a_i_p1_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_p1_m7);
    square_a_epi16(a_r_m1_p7, a_i_m1_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_p7);
    square_a_epi16(a_r_m1_p5, a_i_m1_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_p5);
    square_a_epi16(a_r_m1_p3, a_i_m1_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_p3);
    square_a_epi16(a_r_m1_p1, a_i_m1_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_p1);
    square_a_epi16(a_r_m1_m1, a_i_m1_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_m1);
    square_a_epi16(a_r_m1_m3, a_i_m1_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_m3);
    square_a_epi16(a_r_m1_m5, a_i_m1_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_m5);
    square_a_epi16(a_r_m1_m7, a_i_m1_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m1_m7);
    square_a_epi16(a_r_m3_p7, a_i_m3_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_p7);
    square_a_epi16(a_r_m3_p5, a_i_m3_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_p5);
    square_a_epi16(a_r_m3_p3, a_i_m3_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_p3);
    square_a_epi16(a_r_m3_p1, a_i_m3_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_p1);
    square_a_epi16(a_r_m3_m1, a_i_m3_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_m1);
    square_a_epi16(a_r_m3_m3, a_i_m3_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_m3);
    square_a_epi16(a_r_m3_m5, a_i_m3_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_m5);
    square_a_epi16(a_r_m3_m7, a_i_m3_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m3_m7);
    square_a_epi16(a_r_m5_p7, a_i_m5_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_p7);
    square_a_epi16(a_r_m5_p5, a_i_m5_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_p5);
    square_a_epi16(a_r_m5_p3, a_i_m5_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_p3);
    square_a_epi16(a_r_m5_p1, a_i_m5_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_p1);
    square_a_epi16(a_r_m5_m1, a_i_m5_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_m1);
    square_a_epi16(a_r_m5_m3, a_i_m5_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_m3);
    square_a_epi16(a_r_m5_m5, a_i_m5_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_m5);
    square_a_epi16(a_r_m5_m7, a_i_m5_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m5_m7);
    square_a_epi16(a_r_m7_p7, a_i_m7_p7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_p7);
    square_a_epi16(a_r_m7_p5, a_i_m7_p5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_p5);
    square_a_epi16(a_r_m7_p3, a_i_m7_p3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_p3);
    square_a_epi16(a_r_m7_p1, a_i_m7_p1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_p1);
    square_a_epi16(a_r_m7_m1, a_i_m7_m1, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_m1);
    square_a_epi16(a_r_m7_m3, a_i_m7_m3, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_m3);
    square_a_epi16(a_r_m7_m5, a_i_m7_m5, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_m5);
    square_a_epi16(a_r_m7_m7, a_i_m7_m7, ch_mag_int, SQRT_10_OVER_FOUR, a_sq_m7_m7);

    // Computing different multiples of ||h0||^2
    // x=1, y=1
    ch_mag_2_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,ONE_OVER_FOUR_SQRT_42);
    ch_mag_2_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_2_over_42_with_sigma2,1);
    // x=1, y=3
    ch_mag_10_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,FIVE_OVER_FOUR_SQRT_42);
    ch_mag_10_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_10_over_42_with_sigma2,1);
    // x=1, x=5
    ch_mag_26_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,THIRTEEN_OVER_FOUR_SQRT_42);
    ch_mag_26_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_26_over_42_with_sigma2,1);
    // x=1, y=7
    ch_mag_50_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,TWENTYFIVE_OVER_FOUR_SQRT_42);
    ch_mag_50_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_50_over_42_with_sigma2,1);
    // x=3, y=3
    ch_mag_18_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,NINE_OVER_FOUR_SQRT_42);
    ch_mag_18_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_18_over_42_with_sigma2,1);
    // x=3, y=5
    ch_mag_34_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,SEVENTEEN_OVER_FOUR_SQRT_42);
    ch_mag_34_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_34_over_42_with_sigma2,1);
    // x=3, y=7
    ch_mag_58_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,TWENTYNINE_OVER_FOUR_SQRT_42);
    ch_mag_58_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_58_over_42_with_sigma2,2);
    // x=5, y=5
    ch_mag_50_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,TWENTYFIVE_OVER_FOUR_SQRT_42);
    ch_mag_50_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_50_over_42_with_sigma2,1);
    // x=5, y=7
    ch_mag_74_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,THIRTYSEVEN_OVER_FOUR_SQRT_42);
    ch_mag_74_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_74_over_42_with_sigma2,2);
    // x=7, y=7
    ch_mag_98_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,FORTYNINE_OVER_FOUR_SQRT_42);
    ch_mag_98_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_98_over_42_with_sigma2,2);

    simde__m256i xmm1;
    // Computing Metrics
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_p7, a_sq_p7_p7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_7_7);
    simde__m256i bit_met_p7_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_p5, a_sq_p7_p5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_7_5);
    simde__m256i bit_met_p7_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_p3, a_sq_p7_p3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_7_3);
    simde__m256i bit_met_p7_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_p1, a_sq_p7_p1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_7_1);
    simde__m256i bit_met_p7_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_m1, a_sq_p7_m1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_7_1);
    simde__m256i bit_met_p7_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_m3, a_sq_p7_m3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_7_3);
    simde__m256i bit_met_p7_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_m5, a_sq_p7_m5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_7_5);
    simde__m256i bit_met_p7_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_m7, a_sq_p7_m7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_7_7);
    simde__m256i bit_met_p7_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_p7, a_sq_p5_p7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_5_7);
    simde__m256i bit_met_p5_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_p5, a_sq_p5_p5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_5_5);
    simde__m256i bit_met_p5_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_p3, a_sq_p5_p3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_5_3);
    simde__m256i bit_met_p5_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_p1, a_sq_p5_p1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_5_1);
    simde__m256i bit_met_p5_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_m1, a_sq_p5_m1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_5_1);
    simde__m256i bit_met_p5_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_m3, a_sq_p5_m3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_5_3);
    simde__m256i bit_met_p5_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_m5, a_sq_p5_m5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_5_5);
    simde__m256i bit_met_p5_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_m7, a_sq_p5_m7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_5_7);
    simde__m256i bit_met_p5_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_p7, a_sq_p3_p7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_3_7);
    simde__m256i bit_met_p3_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_p5, a_sq_p3_p5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_3_5);
    simde__m256i bit_met_p3_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_p3, a_sq_p3_p3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_3_3);
    simde__m256i bit_met_p3_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_p1, a_sq_p3_p1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_3_1);
    simde__m256i bit_met_p3_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_m1, a_sq_p3_m1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_3_1);
    simde__m256i bit_met_p3_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_m3, a_sq_p3_m3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_3_3);
    simde__m256i bit_met_p3_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_m5, a_sq_p3_m5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_3_5);
    simde__m256i bit_met_p3_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_m7, a_sq_p3_m7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_3_7);
    simde__m256i bit_met_p3_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_p7, a_sq_p1_p7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_1_7);
    simde__m256i bit_met_p1_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_p5, a_sq_p1_p5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_1_5);
    simde__m256i bit_met_p1_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_p3, a_sq_p1_p3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_1_3);
    simde__m256i bit_met_p1_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_p1, a_sq_p1_p1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_1_1);
    simde__m256i bit_met_p1_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_m1, a_sq_p1_m1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_1_1);
    simde__m256i bit_met_p1_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_m3, a_sq_p1_m3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_1_3);
    simde__m256i bit_met_p1_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_m5, a_sq_p1_m5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_1_5);
    simde__m256i bit_met_p1_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_m7, a_sq_p1_m7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_1_7);
    simde__m256i bit_met_p1_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);

    xmm0 = simde_mm256_subs_epi16(psi_a_m1_p7, a_sq_m1_p7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_1_7);
    simde__m256i bit_met_m1_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m1_p5, a_sq_m1_p5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_1_5);
    simde__m256i bit_met_m1_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m1_p3, a_sq_m1_p3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_1_3);
    simde__m256i bit_met_m1_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m1_p1, a_sq_m1_p1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_1_1);
    simde__m256i bit_met_m1_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m1_m1, a_sq_m1_m1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_1_1);
    simde__m256i bit_met_m1_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m1_m3, a_sq_m1_m3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_1_3);
    simde__m256i bit_met_m1_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m1_m5, a_sq_m1_m5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_1_5);
    simde__m256i bit_met_m1_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m1_m7, a_sq_m1_m7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_1_7);
    simde__m256i bit_met_m1_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_p7, a_sq_m3_p7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_3_7);
    simde__m256i bit_met_m3_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_p5, a_sq_m3_p5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_3_5);
    simde__m256i bit_met_m3_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_p3, a_sq_m3_p3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_3_3);
    simde__m256i bit_met_m3_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_p1, a_sq_m3_p1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_3_1);
    simde__m256i bit_met_m3_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_m1, a_sq_m3_m1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_3_1);
    simde__m256i bit_met_m3_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_m3, a_sq_m3_m3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_3_3);
    simde__m256i bit_met_m3_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_m5, a_sq_m3_m5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_3_5);
    simde__m256i bit_met_m3_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_m7, a_sq_m3_m7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_3_7);
    simde__m256i bit_met_m3_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_p7, a_sq_m5_p7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_5_7);
    simde__m256i bit_met_m5_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_p5, a_sq_m5_p5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_5_5);
    simde__m256i bit_met_m5_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_p3, a_sq_m5_p3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_5_3);
    simde__m256i bit_met_m5_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_p1, a_sq_m5_p1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_5_1);
    simde__m256i bit_met_m5_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_m1, a_sq_m5_m1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_5_1);
    simde__m256i bit_met_m5_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_m3, a_sq_m5_m3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_5_3);
    simde__m256i bit_met_m5_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_m5, a_sq_m5_m5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_5_5);
    simde__m256i bit_met_m5_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_m7, a_sq_m5_m7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_5_7);
    simde__m256i bit_met_m5_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_p7, a_sq_m7_p7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_7_7);
    simde__m256i bit_met_m7_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_p5, a_sq_m7_p5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_7_5);
    simde__m256i bit_met_m7_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_p3, a_sq_m7_p3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_7_3);
    simde__m256i bit_met_m7_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_p1, a_sq_m7_p1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_7_1);
    simde__m256i bit_met_m7_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_m1, a_sq_m7_m1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_7_1);
    simde__m256i bit_met_m7_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_m3, a_sq_m7_m3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_7_3);
    simde__m256i bit_met_m7_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_m5, a_sq_m7_m5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_7_5);
    simde__m256i bit_met_m7_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_m7, a_sq_m7_m7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_7_7);
    simde__m256i bit_met_m7_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);

    // Detection for 1st bit (LTE mapping)
    // bit = 1
    xmm0 = simde_mm256_max_epi16(bit_met_m7_p7, bit_met_m7_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_m7_p3, bit_met_m7_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_m7_m1, bit_met_m7_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_m7_m5, bit_met_m7_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    simde__m256i logmax_den_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_m5_p7, bit_met_m5_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_m5_p3, bit_met_m5_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_m5_m1, bit_met_m5_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m5, bit_met_m5_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_m3_p7, bit_met_m3_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_m3_p3, bit_met_m3_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_m3_m1, bit_met_m3_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_m3_m5, bit_met_m3_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_m1_p7, bit_met_m1_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_m1_p3, bit_met_m1_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m1, bit_met_m1_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_m1_m5, bit_met_m1_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);

    // bit = 0
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p7, bit_met_p7_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_p7_p3, bit_met_p7_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_p7_m1, bit_met_p7_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_p7_m5, bit_met_p7_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    simde__m256i logmax_num_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p5_p7, bit_met_p5_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_p5_p3, bit_met_p5_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_p5_m1, bit_met_p5_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_p5_m5, bit_met_p5_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p3_p7, bit_met_p3_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p3, bit_met_p3_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_p3_m1, bit_met_p3_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_p3_m5, bit_met_p3_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p1_p7, bit_met_p1_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_p1_p3, bit_met_p1_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_p1_m1, bit_met_p1_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_p1_m5, bit_met_p1_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);

    y0r = simde_mm256_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 2nd bit (LTE mapping)
    // bit = 1
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);

    // bit = 0
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p1, bit_met_m7_p1);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p5, bit_met_m7_p5);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);

    y1r = simde_mm256_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 3rd bit (LTE mapping)
    xmm0 = simde_mm256_max_epi16(bit_met_m7_m7, bit_met_m7_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m7_m3, bit_met_m7_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m7_p1, bit_met_m7_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m7_p5, bit_met_m7_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_m5_m7, bit_met_m5_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m5_m3, bit_met_m5_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m5_p1, bit_met_m5_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p5, bit_met_m5_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p5_m7, bit_met_p5_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p5_m3, bit_met_p5_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p5_p1, bit_met_p5_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p5_p5, bit_met_p5_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m7, bit_met_p7_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p7_m3, bit_met_p7_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p7_p1, bit_met_p7_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p7_p5, bit_met_p7_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm256_max_epi16(bit_met_m3_m7, bit_met_m3_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m3_m3, bit_met_m3_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m3_p5, bit_met_m3_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_m1_m7, bit_met_m1_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m1_m3, bit_met_m1_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m1_p5, bit_met_m1_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p1_m7, bit_met_p1_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p1_m3, bit_met_p1_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p1_p5, bit_met_p1_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p3_m7, bit_met_p3_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m3, bit_met_p3_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p3_p5, bit_met_p3_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);

    simde__m256i y2r = simde_mm256_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 4th bit (LTE mapping)
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p5, bit_met_m5_p5);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm256_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p1, bit_met_m7_p1);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);

    y0i = simde_mm256_subs_epi16(logmax_num_re0, logmax_den_re0);


    // Detection for 5th bit (LTE mapping)
    xmm0 = simde_mm256_max_epi16(bit_met_m7_m7, bit_met_m7_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m7_m3, bit_met_m7_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m7_p1, bit_met_m7_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m7_p5, bit_met_m7_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_m1_m7, bit_met_m1_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m1_m3, bit_met_m1_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m1_p5, bit_met_m1_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p1_m7, bit_met_p1_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p1_m3, bit_met_p1_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p1_p5, bit_met_p1_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m7, bit_met_p7_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p7_m3, bit_met_p7_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p7_p1, bit_met_p7_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p7_p5, bit_met_p7_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm256_max_epi16(bit_met_m5_m7, bit_met_m5_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m5_m3, bit_met_m5_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m5_p1, bit_met_m5_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p5, bit_met_m5_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_m3_m7, bit_met_m3_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m3_m3, bit_met_m3_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m3_p5, bit_met_m3_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p3_m7, bit_met_p3_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m3, bit_met_p3_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p3_p5, bit_met_p3_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p5_m7, bit_met_p5_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p5_m3, bit_met_p5_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p5_p1, bit_met_p5_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p5_p5, bit_met_p5_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);

    y1i = simde_mm256_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 6th bit (LTE mapping)
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p1, bit_met_m5_p1);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm256_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p5, bit_met_m7_p5);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);

    simde__m256i y2i = simde_mm256_subs_epi16(logmax_num_re0, logmax_den_re0);

    // map to output stream, difficult to do in SIMD since we have 6 16bit LLRs
    // RE 1
    j = 48*i;
    stream0_out[j + 0] = ((short *)&y0r)[0];
    stream0_out[j + 1] = ((short *)&y1r)[0];
    stream0_out[j + 2] = ((short *)&y2r)[0];
    stream0_out[j + 3] = ((short *)&y0i)[0];
    stream0_out[j + 4] = ((short *)&y1i)[0];
    stream0_out[j + 5] = ((short *)&y2i)[0];
    // RE 2
    stream0_out[j + 6] = ((short *)&y0r)[1];
    stream0_out[j + 7] = ((short *)&y1r)[1];
    stream0_out[j + 8] = ((short *)&y2r)[1];
    stream0_out[j + 9] = ((short *)&y0i)[1];
    stream0_out[j + 10] = ((short *)&y1i)[1];
    stream0_out[j + 11] = ((short *)&y2i)[1];
    // RE 3
    stream0_out[j + 12] = ((short *)&y0r)[2];
    stream0_out[j + 13] = ((short *)&y1r)[2];
    stream0_out[j + 14] = ((short *)&y2r)[2];
    stream0_out[j + 15] = ((short *)&y0i)[2];
    stream0_out[j + 16] = ((short *)&y1i)[2];
    stream0_out[j + 17] = ((short *)&y2i)[2];
    // RE 4
    stream0_out[j + 18] = ((short *)&y0r)[3];
    stream0_out[j + 19] = ((short *)&y1r)[3];
    stream0_out[j + 20] = ((short *)&y2r)[3];
    stream0_out[j + 21] = ((short *)&y0i)[3];
    stream0_out[j + 22] = ((short *)&y1i)[3];
    stream0_out[j + 23] = ((short *)&y2i)[3];
    // RE 5
    stream0_out[j + 24] = ((short *)&y0r)[4];
    stream0_out[j + 25] = ((short *)&y1r)[4];
    stream0_out[j + 26] = ((short *)&y2r)[4];
    stream0_out[j + 27] = ((short *)&y0i)[4];
    stream0_out[j + 28] = ((short *)&y1i)[4];
    stream0_out[j + 29] = ((short *)&y2i)[4];
    // RE 6
    stream0_out[j + 30] = ((short *)&y0r)[5];
    stream0_out[j + 31] = ((short *)&y1r)[5];
    stream0_out[j + 32] = ((short *)&y2r)[5];
    stream0_out[j + 33] = ((short *)&y0i)[5];
    stream0_out[j + 34] = ((short *)&y1i)[5];
    stream0_out[j + 35] = ((short *)&y2i)[5];
    // RE 7
    stream0_out[j + 36] = ((short *)&y0r)[6];
    stream0_out[j + 37] = ((short *)&y1r)[6];
    stream0_out[j + 38] = ((short *)&y2r)[6];
    stream0_out[j + 39] = ((short *)&y0i)[6];
    stream0_out[j + 40] = ((short *)&y1i)[6];
    stream0_out[j + 41] = ((short *)&y2i)[6];
    // RE 8
    stream0_out[j + 42] = ((short *)&y0r)[7];
    stream0_out[j + 43] = ((short *)&y1r)[7];
    stream0_out[j + 44] = ((short *)&y2r)[7];
    stream0_out[j + 45] = ((short *)&y0i)[7];
    stream0_out[j + 46] = ((short *)&y1i)[7];
    stream0_out[j + 47] = ((short *)&y2i)[7];

    // RE 9
    stream0_out[j + 48] = ((short *)&y0r)[8];
    stream0_out[j + 49] = ((short *)&y1r)[8];
    stream0_out[j + 50] = ((short *)&y2r)[8];
    stream0_out[j + 51] = ((short *)&y0i)[8];
    stream0_out[j + 52] = ((short *)&y1i)[8];
    stream0_out[j + 53] = ((short *)&y2i)[8];
    // RE 10
    stream0_out[j + 54] = ((short *)&y0r)[9];
    stream0_out[j + 55] = ((short *)&y1r)[9];
    stream0_out[j + 56] = ((short *)&y2r)[9];
    stream0_out[j + 57] = ((short *)&y0i)[9];
    stream0_out[j + 58] = ((short *)&y1i)[9];
    stream0_out[j + 59] = ((short *)&y2i)[9];
    // RE 11
    stream0_out[j + 60] = ((short *)&y0r)[10];
    stream0_out[j + 61] = ((short *)&y1r)[10];
    stream0_out[j + 62] = ((short *)&y2r)[10];
    stream0_out[j + 63] = ((short *)&y0i)[10];
    stream0_out[j + 64] = ((short *)&y1i)[10];
    stream0_out[j + 65] = ((short *)&y2i)[10];
    // RE 12
    stream0_out[j + 66] = ((short *)&y0r)[11];
    stream0_out[j + 67] = ((short *)&y1r)[11];
    stream0_out[j + 68] = ((short *)&y2r)[11];
    stream0_out[j + 69] = ((short *)&y0i)[11];
    stream0_out[j + 70] = ((short *)&y1i)[11];
    stream0_out[j + 71] = ((short *)&y2i)[11];
    // RE 13
    stream0_out[j + 72] = ((short *)&y0r)[12];
    stream0_out[j + 73] = ((short *)&y1r)[12];
    stream0_out[j + 74] = ((short *)&y2r)[12];
    stream0_out[j + 75] = ((short *)&y0i)[12];
    stream0_out[j + 76] = ((short *)&y1i)[12];
    stream0_out[j + 77] = ((short *)&y2i)[12];
    // RE 14
    stream0_out[j + 78] = ((short *)&y0r)[13];
    stream0_out[j + 79] = ((short *)&y1r)[13];
    stream0_out[j + 80] = ((short *)&y2r)[13];
    stream0_out[j + 81] = ((short *)&y0i)[13];
    stream0_out[j + 82] = ((short *)&y1i)[13];
    stream0_out[j + 83] = ((short *)&y2i)[13];
    // RE 15
    stream0_out[j + 84] = ((short *)&y0r)[14];
    stream0_out[j + 85] = ((short *)&y1r)[14];
    stream0_out[j + 86] = ((short *)&y2r)[14];
    stream0_out[j + 87] = ((short *)&y0i)[14];
    stream0_out[j + 88] = ((short *)&y1i)[14];
    stream0_out[j + 89] = ((short *)&y2i)[14];
    // RE 16
    stream0_out[j + 90] = ((short *)&y0r)[15];
    stream0_out[j + 91] = ((short *)&y1r)[15];
    stream0_out[j + 92] = ((short *)&y2r)[15];
    stream0_out[j + 93] = ((short *)&y0i)[15];
    stream0_out[j + 94] = ((short *)&y1i)[15];
    stream0_out[j + 95] = ((short *)&y2i)[15];

  }

  simde_mm_empty();
  simde_m_empty();

}

void qam64_qam64_avx2(int32_t *stream0_in,
                      int32_t *stream1_in,
                      int32_t *ch_mag,
                      int32_t *ch_mag_i,
                      int16_t *stream0_out,
                      int32_t *rho01,
                      int length
    )
{

  /*
    Author: S. Wagner
    Date: 28-02-17

    Input:
    stream0_in:  MF filter for 1st stream, i.e., y0=h0'*y
    stream1_in:  MF filter for 2nd stream, i.e., y1=h1'*y
    ch_mag:      4*h0/sqrt(42), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    ch_mag_i:    4*h1/sqrt(42), [Re0 Im0 Re1 Im1] s.t. Im0=Re0, Im1=Re1, etc
    rho01:       Channel cross correlation, i.e., h1'*h0

    Output:
    stream0_out: output LLRs for 1st stream
  */


  simde__m256i *rho01_256i      = (simde__m256i *)rho01;
  simde__m256i *stream0_256i_in = (simde__m256i *)stream0_in;
  simde__m256i *stream1_256i_in = (simde__m256i *)stream1_in;
  simde__m256i *ch_mag_256i     = (simde__m256i *)ch_mag;
  simde__m256i *ch_mag_256i_i   = (simde__m256i *)ch_mag_i;

  simde__m256i ONE_OVER_SQRT_42              = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(10112)); // round(1/sqrt(42)*2^16)
  simde__m256i THREE_OVER_SQRT_42            = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(30337)); // round(3/sqrt(42)*2^16)
  simde__m256i FIVE_OVER_SQRT_42             = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(25281)); // round(5/sqrt(42)*2^15)
  simde__m256i SEVEN_OVER_SQRT_42            = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(17697)); // round(7/sqrt(42)*2^14) Q2.14
  simde__m256i ONE_OVER_SQRT_2               = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(23170)); // round(1/sqrt(2)*2^15)
  simde__m256i ONE_OVER_SQRT_2_42            = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(3575));  // round(1/sqrt(2*42)*2^15)
  simde__m256i THREE_OVER_SQRT_2_42          = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(10726)); // round(3/sqrt(2*42)*2^15)
  simde__m256i FIVE_OVER_SQRT_2_42           = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(17876)); // round(5/sqrt(2*42)*2^15)
  simde__m256i SEVEN_OVER_SQRT_2_42          = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(25027)); // round(7/sqrt(2*42)*2^15)
  simde__m256i FORTYNINE_OVER_FOUR_SQRT_42   = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(30969)); // round(49/(4*sqrt(42))*2^14), Q2.14
  simde__m256i THIRTYSEVEN_OVER_FOUR_SQRT_42 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(23385)); // round(37/(4*sqrt(42))*2^14), Q2.14
  simde__m256i TWENTYFIVE_OVER_FOUR_SQRT_42  = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(31601)); // round(25/(4*sqrt(42))*2^15)
  simde__m256i TWENTYNINE_OVER_FOUR_SQRT_42  = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(18329)); // round(29/(4*sqrt(42))*2^15), Q2.14
  simde__m256i SEVENTEEN_OVER_FOUR_SQRT_42   = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(21489)); // round(17/(4*sqrt(42))*2^15)
  simde__m256i NINE_OVER_FOUR_SQRT_42        = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(11376)); // round(9/(4*sqrt(42))*2^15)
  simde__m256i THIRTEEN_OVER_FOUR_SQRT_42    = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(16433)); // round(13/(4*sqrt(42))*2^15)
  simde__m256i FIVE_OVER_FOUR_SQRT_42        = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(6320));  // round(5/(4*sqrt(42))*2^15)
  simde__m256i ONE_OVER_FOUR_SQRT_42         = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(1264));  // round(1/(4*sqrt(42))*2^15)
  simde__m256i SQRT_42_OVER_FOUR             = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(13272)); // round(sqrt(42)/4*2^13), Q3.12

  simde__m256i ch_mag_des;
  simde__m256i ch_mag_int;
  simde__m256i ch_mag_98_over_42_with_sigma2;
  simde__m256i ch_mag_74_over_42_with_sigma2;
  simde__m256i ch_mag_58_over_42_with_sigma2;
  simde__m256i ch_mag_50_over_42_with_sigma2;
  simde__m256i ch_mag_34_over_42_with_sigma2;
  simde__m256i ch_mag_18_over_42_with_sigma2;
  simde__m256i ch_mag_26_over_42_with_sigma2;
  simde__m256i ch_mag_10_over_42_with_sigma2;
  simde__m256i ch_mag_2_over_42_with_sigma2;
  simde__m256i y0r_one_over_sqrt_21;
  simde__m256i y0r_three_over_sqrt_21;
  simde__m256i y0r_five_over_sqrt_21;
  simde__m256i y0r_seven_over_sqrt_21;
  simde__m256i y0i_one_over_sqrt_21;
  simde__m256i y0i_three_over_sqrt_21;
  simde__m256i y0i_five_over_sqrt_21;
  simde__m256i y0i_seven_over_sqrt_21;
  simde__m256i ch_mag_int_with_sigma2;
  simde__m256i two_ch_mag_int_with_sigma2;
  simde__m256i three_ch_mag_int_with_sigma2;

  int i,j;
  uint32_t len256 = (length)>>3;

  for (i=0; i<len256; i+=2) {


    // Get rho
      /*
    xmm0 = rho01_256i[i];
    xmm1 = rho01_256i[i+1];
    xmm0 = simde_mm256_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm256_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm256_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));

    xmm1 = simde_mm256_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm256_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm256_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));

    //xmm0 = [Re(0,1,2,3)   Im(0,1,2,3)   Re(4,5,6,7)     Im(4,5,6,7)]
    //xmm0 = [Re(8,9,10,11) Im(8,9,10,11) Re(12,13,14,15) Im(12,13,14,15)]

    xmm2 = simde_mm256_unpacklo_epi64(xmm0, xmm1);
    //xmm2 = [Re(0,1,2,3) Re(8,9,10,11) Re(4,5,6,7) Re(12,13,14,15)]
    xmm2 = simde_mm256_permute4x64_epi64(xmm2,0xd8); // Re(rho)

    xmm3 = simde_mm256_unpackhi_epi64(xmm0, xmm1);
    //xmm3 = [Im(0,1,2,3) Im(8,9,10,11) Im(4,5,6,7) Im(12,13,14,15)]
    xmm3 = simde_mm256_permute4x64_epi64(xmm3,0xd8); // Im(rho)
      */
    simde__m256i xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8;
    simde_mm256_separate_real_imag_parts(&xmm2, &xmm3, rho01_256i[i], rho01_256i[i+1]);

    simde__m256i rho_rpi = simde_mm256_adds_epi16(xmm2, xmm3); // rho = Re(rho) + Im(rho)
    simde__m256i rho_rmi = simde_mm256_subs_epi16(xmm2, xmm3); // rho* = Re(rho) - Im(rho)

    // Compute the different rhos
    simde__m256i rho_rpi_1_1 = simde_mm256_mulhi_epi16(rho_rpi, ONE_OVER_SQRT_42);
    simde__m256i rho_rmi_1_1 = simde_mm256_mulhi_epi16(rho_rmi, ONE_OVER_SQRT_42);
    simde__m256i rho_rpi_3_3 = simde_mm256_mulhi_epi16(rho_rpi, THREE_OVER_SQRT_42);
    simde__m256i rho_rmi_3_3 = simde_mm256_mulhi_epi16(rho_rmi, THREE_OVER_SQRT_42);
    simde__m256i rho_rpi_5_5 = simde_mm256_mulhi_epi16(rho_rpi, FIVE_OVER_SQRT_42);
    simde__m256i rho_rmi_5_5 = simde_mm256_mulhi_epi16(rho_rmi, FIVE_OVER_SQRT_42);
    simde__m256i rho_rpi_7_7 = simde_mm256_mulhi_epi16(rho_rpi, SEVEN_OVER_SQRT_42);
    simde__m256i rho_rmi_7_7 = simde_mm256_mulhi_epi16(rho_rmi, SEVEN_OVER_SQRT_42);

    rho_rpi_5_5 = simde_mm256_slli_epi16(rho_rpi_5_5, 1);
    rho_rmi_5_5 = simde_mm256_slli_epi16(rho_rmi_5_5, 1);
    rho_rpi_7_7 = simde_mm256_slli_epi16(rho_rpi_7_7, 2);
    rho_rmi_7_7 = simde_mm256_slli_epi16(rho_rmi_7_7, 2);

    xmm4 = simde_mm256_mulhi_epi16(xmm2, ONE_OVER_SQRT_42);
    xmm5 = simde_mm256_mulhi_epi16(xmm3, ONE_OVER_SQRT_42);
    xmm6 = simde_mm256_mulhi_epi16(xmm3, THREE_OVER_SQRT_42);
    xmm7 = simde_mm256_mulhi_epi16(xmm3, FIVE_OVER_SQRT_42);
    xmm8 = simde_mm256_mulhi_epi16(xmm3, SEVEN_OVER_SQRT_42);
    xmm7 = simde_mm256_slli_epi16(xmm7, 1);
    xmm8 = simde_mm256_slli_epi16(xmm8, 2);

    simde__m256i rho_rpi_1_3 = simde_mm256_adds_epi16(xmm4, xmm6);
    simde__m256i rho_rmi_1_3 = simde_mm256_subs_epi16(xmm4, xmm6);
    simde__m256i rho_rpi_1_5 = simde_mm256_adds_epi16(xmm4, xmm7);
    simde__m256i rho_rmi_1_5 = simde_mm256_subs_epi16(xmm4, xmm7);
    simde__m256i rho_rpi_1_7 = simde_mm256_adds_epi16(xmm4, xmm8);
    simde__m256i rho_rmi_1_7 = simde_mm256_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm256_mulhi_epi16(xmm2, THREE_OVER_SQRT_42);
    simde__m256i rho_rpi_3_1 = simde_mm256_adds_epi16(xmm4, xmm5);
    simde__m256i rho_rmi_3_1 = simde_mm256_subs_epi16(xmm4, xmm5);
    simde__m256i rho_rpi_3_5 = simde_mm256_adds_epi16(xmm4, xmm7);
    simde__m256i rho_rmi_3_5 = simde_mm256_subs_epi16(xmm4, xmm7);
    simde__m256i rho_rpi_3_7 = simde_mm256_adds_epi16(xmm4, xmm8);
    simde__m256i rho_rmi_3_7 = simde_mm256_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm256_mulhi_epi16(xmm2, FIVE_OVER_SQRT_42);
    xmm4 = simde_mm256_slli_epi16(xmm4, 1);
    simde__m256i rho_rpi_5_1 = simde_mm256_adds_epi16(xmm4, xmm5);
    simde__m256i rho_rmi_5_1 = simde_mm256_subs_epi16(xmm4, xmm5);
    simde__m256i rho_rpi_5_3 = simde_mm256_adds_epi16(xmm4, xmm6);
    simde__m256i rho_rmi_5_3 = simde_mm256_subs_epi16(xmm4, xmm6);
    simde__m256i rho_rpi_5_7 = simde_mm256_adds_epi16(xmm4, xmm8);
    simde__m256i rho_rmi_5_7 = simde_mm256_subs_epi16(xmm4, xmm8);

    xmm4 = simde_mm256_mulhi_epi16(xmm2, SEVEN_OVER_SQRT_42);
    xmm4 = simde_mm256_slli_epi16(xmm4, 2);
    simde__m256i rho_rpi_7_1 = simde_mm256_adds_epi16(xmm4, xmm5);
    simde__m256i rho_rmi_7_1 = simde_mm256_subs_epi16(xmm4, xmm5);
    simde__m256i rho_rpi_7_3 = simde_mm256_adds_epi16(xmm4, xmm6);
    simde__m256i rho_rmi_7_3 = simde_mm256_subs_epi16(xmm4, xmm6);
    simde__m256i rho_rpi_7_5 = simde_mm256_adds_epi16(xmm4, xmm7);
    simde__m256i rho_rmi_7_5 = simde_mm256_subs_epi16(xmm4, xmm7);

    // Rearrange interfering MF output
    /*
    xmm0 = stream1_256i_in[i];
    xmm1 = stream1_256i_in[i+1];
    xmm0 = simde_mm256_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm256_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm256_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));

    xmm1 = simde_mm256_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm256_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm256_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));

    y1r = simde_mm256_unpacklo_epi64(xmm0, xmm1);
    y1r = simde_mm256_permute4x64_epi64(y1r,0xd8); // Re(y1)

    y1i = simde_mm256_unpackhi_epi64(xmm0, xmm1);
    y1i = simde_mm256_permute4x64_epi64(y1i,0xd8); // Im(y1)
    */
    simde__m256i y1r, y1i, xmm0;
    simde_mm256_separate_real_imag_parts(&y1r, &y1i, stream1_256i_in[i], stream1_256i_in[i+1]);

    // Psi_r calculation from rho_rpi or rho_rmi
    xmm0 = simde_mm256_broadcastw_epi16(simde_mm_set1_epi16(0));// ZERO for abs_pi16
    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_7, y1r);

    simde__m256i psi_r_p7_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_5, y1r);
    simde__m256i psi_r_p7_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_3, y1r);
    simde__m256i psi_r_p7_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_1, y1r);
    simde__m256i psi_r_p7_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_1, y1r);
    simde__m256i psi_r_p7_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_3, y1r);
    simde__m256i psi_r_p7_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_5, y1r);
    simde__m256i psi_r_p7_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_7, y1r);
    simde__m256i psi_r_p7_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_7, y1r);
    simde__m256i psi_r_p5_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_5, y1r);
    simde__m256i psi_r_p5_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_3, y1r);
    simde__m256i psi_r_p5_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_1, y1r);
    simde__m256i psi_r_p5_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_1, y1r);
    simde__m256i psi_r_p5_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_3, y1r);
    simde__m256i psi_r_p5_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_5, y1r);
    simde__m256i psi_r_p5_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_7, y1r);
    simde__m256i psi_r_p5_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_7, y1r);
    simde__m256i psi_r_p3_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_5, y1r);
    simde__m256i psi_r_p3_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_3, y1r);
    simde__m256i psi_r_p3_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_1, y1r);
    simde__m256i psi_r_p3_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_1, y1r);
    simde__m256i psi_r_p3_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_3, y1r);
    simde__m256i psi_r_p3_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_5, y1r);
    simde__m256i psi_r_p3_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_7, y1r);
    simde__m256i psi_r_p3_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_7, y1r);
    simde__m256i psi_r_p1_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_5, y1r);
    simde__m256i psi_r_p1_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_3, y1r);
    simde__m256i psi_r_p1_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_1, y1r);
    simde__m256i psi_r_p1_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_1, y1r);
    simde__m256i psi_r_p1_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_3, y1r);
    simde__m256i psi_r_p1_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_5, y1r);
    simde__m256i psi_r_p1_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_7, y1r);
    simde__m256i psi_r_p1_m7 = simde_mm256_abs_epi16(xmm2);

    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_7, y1r);
    simde__m256i psi_r_m1_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_5, y1r);
    simde__m256i psi_r_m1_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_3, y1r);
    simde__m256i psi_r_m1_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_1, y1r);
    simde__m256i psi_r_m1_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_1, y1r);
    simde__m256i psi_r_m1_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_3, y1r);
    simde__m256i psi_r_m1_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_5, y1r);
    simde__m256i psi_r_m1_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_7, y1r);
    simde__m256i psi_r_m1_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_7, y1r);
    simde__m256i psi_r_m3_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_5, y1r);
    simde__m256i psi_r_m3_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_3, y1r);
    simde__m256i psi_r_m3_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_1, y1r);
    simde__m256i psi_r_m3_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_1, y1r);
    simde__m256i psi_r_m3_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_3, y1r);
    simde__m256i psi_r_m3_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_5, y1r);
    simde__m256i psi_r_m3_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_7, y1r);
    simde__m256i psi_r_m3_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_7, y1r);
    simde__m256i psi_r_m5_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_5, y1r);
    simde__m256i psi_r_m5_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_3, y1r);
    simde__m256i psi_r_m5_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_1, y1r);
    simde__m256i psi_r_m5_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_1, y1r);
    simde__m256i psi_r_m5_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_3, y1r);
    simde__m256i psi_r_m5_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_5, y1r);
    simde__m256i psi_r_m5_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_7, y1r);
    simde__m256i psi_r_m5_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_7, y1r);
    simde__m256i psi_r_m7_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_5, y1r);
    simde__m256i psi_r_m7_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_3, y1r);
    simde__m256i psi_r_m7_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_1, y1r);
    simde__m256i psi_r_m7_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_1, y1r);
    simde__m256i psi_r_m7_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_3, y1r);
    simde__m256i psi_r_m7_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_5, y1r);
    simde__m256i psi_r_m7_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_7, y1r);
    simde__m256i psi_r_m7_m7 = simde_mm256_abs_epi16(xmm2);

    // Simde__M256i Psi_i calculation from rho_rpi or rho_rmi
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_7, y1i);
    simde__m256i psi_i_p7_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_7, y1i);
    simde__m256i psi_i_p7_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_7, y1i);
    simde__m256i psi_i_p7_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_7, y1i);
    simde__m256i psi_i_p7_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_7, y1i);
    simde__m256i psi_i_p7_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_7, y1i);
    simde__m256i psi_i_p7_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_7, y1i);
    simde__m256i psi_i_p7_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_7, y1i);
    simde__m256i psi_i_p7_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_5, y1i);
    simde__m256i psi_i_p5_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_5, y1i);
    simde__m256i psi_i_p5_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_5, y1i);
    simde__m256i psi_i_p5_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_5, y1i);
    simde__m256i psi_i_p5_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_5, y1i);
    simde__m256i psi_i_p5_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_5, y1i);
    simde__m256i psi_i_p5_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_5, y1i);
    simde__m256i psi_i_p5_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_5, y1i);
    simde__m256i psi_i_p5_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_3, y1i);
    simde__m256i psi_i_p3_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_3, y1i);
    simde__m256i psi_i_p3_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_3, y1i);
    simde__m256i psi_i_p3_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_3, y1i);
    simde__m256i psi_i_p3_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_3, y1i);
    simde__m256i psi_i_p3_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_3, y1i);
    simde__m256i psi_i_p3_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_3, y1i);
    simde__m256i psi_i_p3_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_3, y1i);
    simde__m256i psi_i_p3_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_7_1, y1i);
    simde__m256i psi_i_p1_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_5_1, y1i);
    simde__m256i psi_i_p1_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_3_1, y1i);
    simde__m256i psi_i_p1_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rmi_1_1, y1i);
    simde__m256i psi_i_p1_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_1_1, y1i);
    simde__m256i psi_i_p1_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_3_1, y1i);
    simde__m256i psi_i_p1_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_5_1, y1i);
    simde__m256i psi_i_p1_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rpi_7_1, y1i);
    simde__m256i psi_i_p1_m7 = simde_mm256_abs_epi16(xmm2);

    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_1, y1i);
    simde__m256i psi_i_m1_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_1, y1i);
    simde__m256i psi_i_m1_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_1, y1i);
    simde__m256i psi_i_m1_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_1, y1i);
    simde__m256i psi_i_m1_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_1, y1i);
    simde__m256i psi_i_m1_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_1, y1i);
    simde__m256i psi_i_m1_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_1, y1i);
    simde__m256i psi_i_m1_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_1, y1i);
    simde__m256i psi_i_m1_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_3, y1i);
    simde__m256i psi_i_m3_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_3, y1i);
    simde__m256i psi_i_m3_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_3, y1i);
    simde__m256i psi_i_m3_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_3, y1i);
    simde__m256i psi_i_m3_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_3, y1i);
    simde__m256i psi_i_m3_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_3, y1i);
    simde__m256i psi_i_m3_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_3, y1i);
    simde__m256i psi_i_m3_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_3, y1i);
    simde__m256i psi_i_m3_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_5, y1i);
    simde__m256i psi_i_m5_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_5, y1i);
    simde__m256i psi_i_m5_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_5, y1i);
    simde__m256i psi_i_m5_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_5, y1i);
    simde__m256i psi_i_m5_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_5, y1i);
    simde__m256i psi_i_m5_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_5, y1i);
    simde__m256i psi_i_m5_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_5, y1i);
    simde__m256i psi_i_m5_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_5, y1i);
    simde__m256i psi_i_m5_m7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_7_7, y1i);
    simde__m256i psi_i_m7_p7 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_5_7, y1i);
    simde__m256i psi_i_m7_p5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_3_7, y1i);
    simde__m256i psi_i_m7_p3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_subs_epi16(rho_rpi_1_7, y1i);
    simde__m256i psi_i_m7_p1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_1_7, y1i);
    simde__m256i psi_i_m7_m1 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_3_7, y1i);
    simde__m256i psi_i_m7_m3 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_5_7, y1i);
    simde__m256i psi_i_m7_m5 = simde_mm256_abs_epi16(xmm2);
    xmm2 = simde_mm256_adds_epi16(rho_rmi_7_7, y1i);
    simde__m256i psi_i_m7_m7 = simde_mm256_abs_epi16(xmm2);

    /*
    // Rearrange desired MF output
    xmm0 = stream0_256i_in[i];
    xmm1 = stream0_256i_in[i+1];
    xmm0 = simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 = simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 = simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    y0r = simde_mm_unpacklo_epi64(xmm0,xmm1); // = [y0r(1),y0r(2),y0r(3),y0r(4)]
    y0i = simde_mm_unpackhi_epi64(xmm0,xmm1);
    */
    simde__m256i y0r, y0i;
    simde_mm256_separate_real_imag_parts(&y0r, &y0i, stream0_256i_in[i], stream0_256i_in[i+1]);

    // Rearrange desired channel magnitudes
    // [|h|^2(1),|h|^2(1),|h|^2(2),|h|^2(2),...,,|h|^2(7),|h|^2(7)]*(2/sqrt(10))
    /*
    xmm2 = ch_mag_256i[i];
    xmm3 = ch_mag_256i[i+1];
    xmm2 = simde_mm_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    ch_mag_des = simde_mm_unpacklo_epi64(xmm2,xmm3);
    */
    // xmm2 is dummy variable that contains the same values as ch_mag_des
    simde_mm256_separate_real_imag_parts(&ch_mag_des, &xmm2, ch_mag_256i[i], ch_mag_256i[i+1]);


    // Rearrange interfering channel magnitudes
    /*
    xmm2 = ch_mag_256i_i[i];
    xmm3 = ch_mag_256i_i[i+1];
    xmm2 = simde_mm_shufflelo_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shufflehi_epi16(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm2 = simde_mm_shuffle_epi32(xmm2,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflelo_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shufflehi_epi16(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm3 = simde_mm_shuffle_epi32(xmm3,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    ch_mag_int  = simde_mm_unpacklo_epi64(xmm2,xmm3);
    */
    simde_mm256_separate_real_imag_parts(&ch_mag_int, &xmm2, ch_mag_256i_i[i], ch_mag_256i_i[i+1]);

    y0r_one_over_sqrt_21   = simde_mm256_mulhi_epi16(y0r, ONE_OVER_SQRT_42);
    y0r_three_over_sqrt_21 = simde_mm256_mulhi_epi16(y0r, THREE_OVER_SQRT_42);
    y0r_five_over_sqrt_21  = simde_mm256_mulhi_epi16(y0r, FIVE_OVER_SQRT_42);
    y0r_five_over_sqrt_21  = simde_mm256_slli_epi16(y0r_five_over_sqrt_21, 1);
    y0r_seven_over_sqrt_21 = simde_mm256_mulhi_epi16(y0r, SEVEN_OVER_SQRT_42);
    y0r_seven_over_sqrt_21 = simde_mm256_slli_epi16(y0r_seven_over_sqrt_21, 2); // Q2.14

    y0i_one_over_sqrt_21   = simde_mm256_mulhi_epi16(y0i, ONE_OVER_SQRT_42);
    y0i_three_over_sqrt_21 = simde_mm256_mulhi_epi16(y0i, THREE_OVER_SQRT_42);
    y0i_five_over_sqrt_21  = simde_mm256_mulhi_epi16(y0i, FIVE_OVER_SQRT_42);
    y0i_five_over_sqrt_21  = simde_mm256_slli_epi16(y0i_five_over_sqrt_21, 1);
    y0i_seven_over_sqrt_21 = simde_mm256_mulhi_epi16(y0i, SEVEN_OVER_SQRT_42);
    y0i_seven_over_sqrt_21 = simde_mm256_slli_epi16(y0i_seven_over_sqrt_21, 2); // Q2.14

    simde__m256i y0_p_7_1 = simde_mm256_adds_epi16(y0r_seven_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_p_7_3 = simde_mm256_adds_epi16(y0r_seven_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_p_7_5 = simde_mm256_adds_epi16(y0r_seven_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_p_7_7 = simde_mm256_adds_epi16(y0r_seven_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m256i y0_p_5_1 = simde_mm256_adds_epi16(y0r_five_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_p_5_3 = simde_mm256_adds_epi16(y0r_five_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_p_5_5 = simde_mm256_adds_epi16(y0r_five_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_p_5_7 = simde_mm256_adds_epi16(y0r_five_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m256i y0_p_3_1 = simde_mm256_adds_epi16(y0r_three_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_p_3_3 = simde_mm256_adds_epi16(y0r_three_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_p_3_5 = simde_mm256_adds_epi16(y0r_three_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_p_3_7 = simde_mm256_adds_epi16(y0r_three_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m256i y0_p_1_1 = simde_mm256_adds_epi16(y0r_one_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_p_1_3 = simde_mm256_adds_epi16(y0r_one_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_p_1_5 = simde_mm256_adds_epi16(y0r_one_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_p_1_7 = simde_mm256_adds_epi16(y0r_one_over_sqrt_21, y0i_seven_over_sqrt_21);

    simde__m256i y0_m_1_1 = simde_mm256_subs_epi16(y0r_one_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_m_1_3 = simde_mm256_subs_epi16(y0r_one_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_m_1_5 = simde_mm256_subs_epi16(y0r_one_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_m_1_7 = simde_mm256_subs_epi16(y0r_one_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m256i y0_m_3_1 = simde_mm256_subs_epi16(y0r_three_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_m_3_3 = simde_mm256_subs_epi16(y0r_three_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_m_3_5 = simde_mm256_subs_epi16(y0r_three_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_m_3_7 = simde_mm256_subs_epi16(y0r_three_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m256i y0_m_5_1 = simde_mm256_subs_epi16(y0r_five_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_m_5_3 = simde_mm256_subs_epi16(y0r_five_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_m_5_5 = simde_mm256_subs_epi16(y0r_five_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_m_5_7 = simde_mm256_subs_epi16(y0r_five_over_sqrt_21, y0i_seven_over_sqrt_21);
    simde__m256i y0_m_7_1 = simde_mm256_subs_epi16(y0r_seven_over_sqrt_21, y0i_one_over_sqrt_21);
    simde__m256i y0_m_7_3 = simde_mm256_subs_epi16(y0r_seven_over_sqrt_21, y0i_three_over_sqrt_21);
    simde__m256i y0_m_7_5 = simde_mm256_subs_epi16(y0r_seven_over_sqrt_21, y0i_five_over_sqrt_21);
    simde__m256i y0_m_7_7 = simde_mm256_subs_epi16(y0r_seven_over_sqrt_21, y0i_seven_over_sqrt_21);

    // Detection of interference term
    ch_mag_int_with_sigma2       = simde_mm256_srai_epi16(ch_mag_int, 1); // *2
    two_ch_mag_int_with_sigma2   = ch_mag_int; // *4
    three_ch_mag_int_with_sigma2 = simde_mm256_adds_epi16(ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2); // *6
    simde__m256i tmp_result, tmp_result2, tmp_result3, tmp_result4;
    interference_abs_64qam_epi16(psi_r_p7_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p7_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p7_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p7_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p7_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p7_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p7_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p7_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p7_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p5_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p5_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p3_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p3_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_p1_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_p1_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m1_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m1_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m3_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m3_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m5_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m5_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_r_m7_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_r_m7_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);

    interference_abs_64qam_epi16(psi_i_p7_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p7_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p7_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p7_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p7_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p7_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p7_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p7_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p7_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p5_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p5_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p3_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p3_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_p1_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_p1_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m1_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m1_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m3_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m3_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m5_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m5_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_p7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_p7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_p5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_p5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_p3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_p3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_p1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_p1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_m1, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_m1, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_m3, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_m3, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_m5, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_m5, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);
    interference_abs_64qam_epi16(psi_i_m7_m7, ch_mag_int_with_sigma2, two_ch_mag_int_with_sigma2, three_ch_mag_int_with_sigma2, a_i_m7_m7, ONE_OVER_SQRT_2_42, THREE_OVER_SQRT_2_42, FIVE_OVER_SQRT_2_42,
                                 SEVEN_OVER_SQRT_2_42);

    // Calculation of a group of two terms in the bit metric involving product of psi and interference
    prodsum_psi_a_epi16(psi_r_p7_p7, a_r_p7_p7, psi_i_p7_p7, a_i_p7_p7, psi_a_p7_p7);
    prodsum_psi_a_epi16(psi_r_p7_p5, a_r_p7_p5, psi_i_p7_p5, a_i_p7_p5, psi_a_p7_p5);
    prodsum_psi_a_epi16(psi_r_p7_p3, a_r_p7_p3, psi_i_p7_p3, a_i_p7_p3, psi_a_p7_p3);
    prodsum_psi_a_epi16(psi_r_p7_p1, a_r_p7_p1, psi_i_p7_p1, a_i_p7_p1, psi_a_p7_p1);
    prodsum_psi_a_epi16(psi_r_p7_m1, a_r_p7_m1, psi_i_p7_m1, a_i_p7_m1, psi_a_p7_m1);
    prodsum_psi_a_epi16(psi_r_p7_m3, a_r_p7_m3, psi_i_p7_m3, a_i_p7_m3, psi_a_p7_m3);
    prodsum_psi_a_epi16(psi_r_p7_m5, a_r_p7_m5, psi_i_p7_m5, a_i_p7_m5, psi_a_p7_m5);
    prodsum_psi_a_epi16(psi_r_p7_m7, a_r_p7_m7, psi_i_p7_m7, a_i_p7_m7, psi_a_p7_m7);
    prodsum_psi_a_epi16(psi_r_p5_p7, a_r_p5_p7, psi_i_p5_p7, a_i_p5_p7, psi_a_p5_p7);
    prodsum_psi_a_epi16(psi_r_p5_p5, a_r_p5_p5, psi_i_p5_p5, a_i_p5_p5, psi_a_p5_p5);
    prodsum_psi_a_epi16(psi_r_p5_p3, a_r_p5_p3, psi_i_p5_p3, a_i_p5_p3, psi_a_p5_p3);
    prodsum_psi_a_epi16(psi_r_p5_p1, a_r_p5_p1, psi_i_p5_p1, a_i_p5_p1, psi_a_p5_p1);
    prodsum_psi_a_epi16(psi_r_p5_m1, a_r_p5_m1, psi_i_p5_m1, a_i_p5_m1, psi_a_p5_m1);
    prodsum_psi_a_epi16(psi_r_p5_m3, a_r_p5_m3, psi_i_p5_m3, a_i_p5_m3, psi_a_p5_m3);
    prodsum_psi_a_epi16(psi_r_p5_m5, a_r_p5_m5, psi_i_p5_m5, a_i_p5_m5, psi_a_p5_m5);
    prodsum_psi_a_epi16(psi_r_p5_m7, a_r_p5_m7, psi_i_p5_m7, a_i_p5_m7, psi_a_p5_m7);
    prodsum_psi_a_epi16(psi_r_p3_p7, a_r_p3_p7, psi_i_p3_p7, a_i_p3_p7, psi_a_p3_p7);
    prodsum_psi_a_epi16(psi_r_p3_p5, a_r_p3_p5, psi_i_p3_p5, a_i_p3_p5, psi_a_p3_p5);
    prodsum_psi_a_epi16(psi_r_p3_p3, a_r_p3_p3, psi_i_p3_p3, a_i_p3_p3, psi_a_p3_p3);
    prodsum_psi_a_epi16(psi_r_p3_p1, a_r_p3_p1, psi_i_p3_p1, a_i_p3_p1, psi_a_p3_p1);
    prodsum_psi_a_epi16(psi_r_p3_m1, a_r_p3_m1, psi_i_p3_m1, a_i_p3_m1, psi_a_p3_m1);
    prodsum_psi_a_epi16(psi_r_p3_m3, a_r_p3_m3, psi_i_p3_m3, a_i_p3_m3, psi_a_p3_m3);
    prodsum_psi_a_epi16(psi_r_p3_m5, a_r_p3_m5, psi_i_p3_m5, a_i_p3_m5, psi_a_p3_m5);
    prodsum_psi_a_epi16(psi_r_p3_m7, a_r_p3_m7, psi_i_p3_m7, a_i_p3_m7, psi_a_p3_m7);
    prodsum_psi_a_epi16(psi_r_p1_p7, a_r_p1_p7, psi_i_p1_p7, a_i_p1_p7, psi_a_p1_p7);
    prodsum_psi_a_epi16(psi_r_p1_p5, a_r_p1_p5, psi_i_p1_p5, a_i_p1_p5, psi_a_p1_p5);
    prodsum_psi_a_epi16(psi_r_p1_p3, a_r_p1_p3, psi_i_p1_p3, a_i_p1_p3, psi_a_p1_p3);
    prodsum_psi_a_epi16(psi_r_p1_p1, a_r_p1_p1, psi_i_p1_p1, a_i_p1_p1, psi_a_p1_p1);
    prodsum_psi_a_epi16(psi_r_p1_m1, a_r_p1_m1, psi_i_p1_m1, a_i_p1_m1, psi_a_p1_m1);
    prodsum_psi_a_epi16(psi_r_p1_m3, a_r_p1_m3, psi_i_p1_m3, a_i_p1_m3, psi_a_p1_m3);
    prodsum_psi_a_epi16(psi_r_p1_m5, a_r_p1_m5, psi_i_p1_m5, a_i_p1_m5, psi_a_p1_m5);
    prodsum_psi_a_epi16(psi_r_p1_m7, a_r_p1_m7, psi_i_p1_m7, a_i_p1_m7, psi_a_p1_m7);
    prodsum_psi_a_epi16(psi_r_m1_p7, a_r_m1_p7, psi_i_m1_p7, a_i_m1_p7, psi_a_m1_p7);
    prodsum_psi_a_epi16(psi_r_m1_p5, a_r_m1_p5, psi_i_m1_p5, a_i_m1_p5, psi_a_m1_p5);
    prodsum_psi_a_epi16(psi_r_m1_p3, a_r_m1_p3, psi_i_m1_p3, a_i_m1_p3, psi_a_m1_p3);
    prodsum_psi_a_epi16(psi_r_m1_p1, a_r_m1_p1, psi_i_m1_p1, a_i_m1_p1, psi_a_m1_p1);
    prodsum_psi_a_epi16(psi_r_m1_m1, a_r_m1_m1, psi_i_m1_m1, a_i_m1_m1, psi_a_m1_m1);
    prodsum_psi_a_epi16(psi_r_m1_m3, a_r_m1_m3, psi_i_m1_m3, a_i_m1_m3, psi_a_m1_m3);
    prodsum_psi_a_epi16(psi_r_m1_m5, a_r_m1_m5, psi_i_m1_m5, a_i_m1_m5, psi_a_m1_m5);
    prodsum_psi_a_epi16(psi_r_m1_m7, a_r_m1_m7, psi_i_m1_m7, a_i_m1_m7, psi_a_m1_m7);
    prodsum_psi_a_epi16(psi_r_m3_p7, a_r_m3_p7, psi_i_m3_p7, a_i_m3_p7, psi_a_m3_p7);
    prodsum_psi_a_epi16(psi_r_m3_p5, a_r_m3_p5, psi_i_m3_p5, a_i_m3_p5, psi_a_m3_p5);
    prodsum_psi_a_epi16(psi_r_m3_p3, a_r_m3_p3, psi_i_m3_p3, a_i_m3_p3, psi_a_m3_p3);
    prodsum_psi_a_epi16(psi_r_m3_p1, a_r_m3_p1, psi_i_m3_p1, a_i_m3_p1, psi_a_m3_p1);
    prodsum_psi_a_epi16(psi_r_m3_m1, a_r_m3_m1, psi_i_m3_m1, a_i_m3_m1, psi_a_m3_m1);
    prodsum_psi_a_epi16(psi_r_m3_m3, a_r_m3_m3, psi_i_m3_m3, a_i_m3_m3, psi_a_m3_m3);
    prodsum_psi_a_epi16(psi_r_m3_m5, a_r_m3_m5, psi_i_m3_m5, a_i_m3_m5, psi_a_m3_m5);
    prodsum_psi_a_epi16(psi_r_m3_m7, a_r_m3_m7, psi_i_m3_m7, a_i_m3_m7, psi_a_m3_m7);
    prodsum_psi_a_epi16(psi_r_m5_p7, a_r_m5_p7, psi_i_m5_p7, a_i_m5_p7, psi_a_m5_p7);
    prodsum_psi_a_epi16(psi_r_m5_p5, a_r_m5_p5, psi_i_m5_p5, a_i_m5_p5, psi_a_m5_p5);
    prodsum_psi_a_epi16(psi_r_m5_p3, a_r_m5_p3, psi_i_m5_p3, a_i_m5_p3, psi_a_m5_p3);
    prodsum_psi_a_epi16(psi_r_m5_p1, a_r_m5_p1, psi_i_m5_p1, a_i_m5_p1, psi_a_m5_p1);
    prodsum_psi_a_epi16(psi_r_m5_m1, a_r_m5_m1, psi_i_m5_m1, a_i_m5_m1, psi_a_m5_m1);
    prodsum_psi_a_epi16(psi_r_m5_m3, a_r_m5_m3, psi_i_m5_m3, a_i_m5_m3, psi_a_m5_m3);
    prodsum_psi_a_epi16(psi_r_m5_m5, a_r_m5_m5, psi_i_m5_m5, a_i_m5_m5, psi_a_m5_m5);
    prodsum_psi_a_epi16(psi_r_m5_m7, a_r_m5_m7, psi_i_m5_m7, a_i_m5_m7, psi_a_m5_m7);
    prodsum_psi_a_epi16(psi_r_m7_p7, a_r_m7_p7, psi_i_m7_p7, a_i_m7_p7, psi_a_m7_p7);
    prodsum_psi_a_epi16(psi_r_m7_p5, a_r_m7_p5, psi_i_m7_p5, a_i_m7_p5, psi_a_m7_p5);
    prodsum_psi_a_epi16(psi_r_m7_p3, a_r_m7_p3, psi_i_m7_p3, a_i_m7_p3, psi_a_m7_p3);
    prodsum_psi_a_epi16(psi_r_m7_p1, a_r_m7_p1, psi_i_m7_p1, a_i_m7_p1, psi_a_m7_p1);
    prodsum_psi_a_epi16(psi_r_m7_m1, a_r_m7_m1, psi_i_m7_m1, a_i_m7_m1, psi_a_m7_m1);
    prodsum_psi_a_epi16(psi_r_m7_m3, a_r_m7_m3, psi_i_m7_m3, a_i_m7_m3, psi_a_m7_m3);
    prodsum_psi_a_epi16(psi_r_m7_m5, a_r_m7_m5, psi_i_m7_m5, a_i_m7_m5, psi_a_m7_m5);
    prodsum_psi_a_epi16(psi_r_m7_m7, a_r_m7_m7, psi_i_m7_m7, a_i_m7_m7, psi_a_m7_m7);

    // Multiply by sqrt(2)
    psi_a_p7_p7 = simde_mm256_mulhi_epi16(psi_a_p7_p7, ONE_OVER_SQRT_2);
    psi_a_p7_p7 = simde_mm256_slli_epi16(psi_a_p7_p7, 2);
    psi_a_p7_p5 = simde_mm256_mulhi_epi16(psi_a_p7_p5, ONE_OVER_SQRT_2);
    psi_a_p7_p5 = simde_mm256_slli_epi16(psi_a_p7_p5, 2);
    psi_a_p7_p3 = simde_mm256_mulhi_epi16(psi_a_p7_p3, ONE_OVER_SQRT_2);
    psi_a_p7_p3 = simde_mm256_slli_epi16(psi_a_p7_p3, 2);
    psi_a_p7_p1 = simde_mm256_mulhi_epi16(psi_a_p7_p1, ONE_OVER_SQRT_2);
    psi_a_p7_p1 = simde_mm256_slli_epi16(psi_a_p7_p1, 2);
    psi_a_p7_m1 = simde_mm256_mulhi_epi16(psi_a_p7_m1, ONE_OVER_SQRT_2);
    psi_a_p7_m1 = simde_mm256_slli_epi16(psi_a_p7_m1, 2);
    psi_a_p7_m3 = simde_mm256_mulhi_epi16(psi_a_p7_m3, ONE_OVER_SQRT_2);
    psi_a_p7_m3 = simde_mm256_slli_epi16(psi_a_p7_m3, 2);
    psi_a_p7_m5 = simde_mm256_mulhi_epi16(psi_a_p7_m5, ONE_OVER_SQRT_2);
    psi_a_p7_m5 = simde_mm256_slli_epi16(psi_a_p7_m5, 2);
    psi_a_p7_m7 = simde_mm256_mulhi_epi16(psi_a_p7_m7, ONE_OVER_SQRT_2);
    psi_a_p7_m7 = simde_mm256_slli_epi16(psi_a_p7_m7, 2);
    psi_a_p5_p7 = simde_mm256_mulhi_epi16(psi_a_p5_p7, ONE_OVER_SQRT_2);
    psi_a_p5_p7 = simde_mm256_slli_epi16(psi_a_p5_p7, 2);
    psi_a_p5_p5 = simde_mm256_mulhi_epi16(psi_a_p5_p5, ONE_OVER_SQRT_2);
    psi_a_p5_p5 = simde_mm256_slli_epi16(psi_a_p5_p5, 2);
    psi_a_p5_p3 = simde_mm256_mulhi_epi16(psi_a_p5_p3, ONE_OVER_SQRT_2);
    psi_a_p5_p3 = simde_mm256_slli_epi16(psi_a_p5_p3, 2);
    psi_a_p5_p1 = simde_mm256_mulhi_epi16(psi_a_p5_p1, ONE_OVER_SQRT_2);
    psi_a_p5_p1 = simde_mm256_slli_epi16(psi_a_p5_p1, 2);
    psi_a_p5_m1 = simde_mm256_mulhi_epi16(psi_a_p5_m1, ONE_OVER_SQRT_2);
    psi_a_p5_m1 = simde_mm256_slli_epi16(psi_a_p5_m1, 2);
    psi_a_p5_m3 = simde_mm256_mulhi_epi16(psi_a_p5_m3, ONE_OVER_SQRT_2);
    psi_a_p5_m3 = simde_mm256_slli_epi16(psi_a_p5_m3, 2);
    psi_a_p5_m5 = simde_mm256_mulhi_epi16(psi_a_p5_m5, ONE_OVER_SQRT_2);
    psi_a_p5_m5 = simde_mm256_slli_epi16(psi_a_p5_m5, 2);
    psi_a_p5_m7 = simde_mm256_mulhi_epi16(psi_a_p5_m7, ONE_OVER_SQRT_2);
    psi_a_p5_m7 = simde_mm256_slli_epi16(psi_a_p5_m7, 2);
    psi_a_p3_p7 = simde_mm256_mulhi_epi16(psi_a_p3_p7, ONE_OVER_SQRT_2);
    psi_a_p3_p7 = simde_mm256_slli_epi16(psi_a_p3_p7, 2);
    psi_a_p3_p5 = simde_mm256_mulhi_epi16(psi_a_p3_p5, ONE_OVER_SQRT_2);
    psi_a_p3_p5 = simde_mm256_slli_epi16(psi_a_p3_p5, 2);
    psi_a_p3_p3 = simde_mm256_mulhi_epi16(psi_a_p3_p3, ONE_OVER_SQRT_2);
    psi_a_p3_p3 = simde_mm256_slli_epi16(psi_a_p3_p3, 2);
    psi_a_p3_p1 = simde_mm256_mulhi_epi16(psi_a_p3_p1, ONE_OVER_SQRT_2);
    psi_a_p3_p1 = simde_mm256_slli_epi16(psi_a_p3_p1, 2);
    psi_a_p3_m1 = simde_mm256_mulhi_epi16(psi_a_p3_m1, ONE_OVER_SQRT_2);
    psi_a_p3_m1 = simde_mm256_slli_epi16(psi_a_p3_m1, 2);
    psi_a_p3_m3 = simde_mm256_mulhi_epi16(psi_a_p3_m3, ONE_OVER_SQRT_2);
    psi_a_p3_m3 = simde_mm256_slli_epi16(psi_a_p3_m3, 2);
    psi_a_p3_m5 = simde_mm256_mulhi_epi16(psi_a_p3_m5, ONE_OVER_SQRT_2);
    psi_a_p3_m5 = simde_mm256_slli_epi16(psi_a_p3_m5, 2);
    psi_a_p3_m7 = simde_mm256_mulhi_epi16(psi_a_p3_m7, ONE_OVER_SQRT_2);
    psi_a_p3_m7 = simde_mm256_slli_epi16(psi_a_p3_m7, 2);
    psi_a_p1_p7 = simde_mm256_mulhi_epi16(psi_a_p1_p7, ONE_OVER_SQRT_2);
    psi_a_p1_p7 = simde_mm256_slli_epi16(psi_a_p1_p7, 2);
    psi_a_p1_p5 = simde_mm256_mulhi_epi16(psi_a_p1_p5, ONE_OVER_SQRT_2);
    psi_a_p1_p5 = simde_mm256_slli_epi16(psi_a_p1_p5, 2);
    psi_a_p1_p3 = simde_mm256_mulhi_epi16(psi_a_p1_p3, ONE_OVER_SQRT_2);
    psi_a_p1_p3 = simde_mm256_slli_epi16(psi_a_p1_p3, 2);
    psi_a_p1_p1 = simde_mm256_mulhi_epi16(psi_a_p1_p1, ONE_OVER_SQRT_2);
    psi_a_p1_p1 = simde_mm256_slli_epi16(psi_a_p1_p1, 2);
    psi_a_p1_m1 = simde_mm256_mulhi_epi16(psi_a_p1_m1, ONE_OVER_SQRT_2);
    psi_a_p1_m1 = simde_mm256_slli_epi16(psi_a_p1_m1, 2);
    psi_a_p1_m3 = simde_mm256_mulhi_epi16(psi_a_p1_m3, ONE_OVER_SQRT_2);
    psi_a_p1_m3 = simde_mm256_slli_epi16(psi_a_p1_m3, 2);
    psi_a_p1_m5 = simde_mm256_mulhi_epi16(psi_a_p1_m5, ONE_OVER_SQRT_2);
    psi_a_p1_m5 = simde_mm256_slli_epi16(psi_a_p1_m5, 2);
    psi_a_p1_m7 = simde_mm256_mulhi_epi16(psi_a_p1_m7, ONE_OVER_SQRT_2);
    psi_a_p1_m7 = simde_mm256_slli_epi16(psi_a_p1_m7, 2);
    psi_a_m1_p7 = simde_mm256_mulhi_epi16(psi_a_m1_p7, ONE_OVER_SQRT_2);
    psi_a_m1_p7 = simde_mm256_slli_epi16(psi_a_m1_p7, 2);
    psi_a_m1_p5 = simde_mm256_mulhi_epi16(psi_a_m1_p5, ONE_OVER_SQRT_2);
    psi_a_m1_p5 = simde_mm256_slli_epi16(psi_a_m1_p5, 2);
    psi_a_m1_p3 = simde_mm256_mulhi_epi16(psi_a_m1_p3, ONE_OVER_SQRT_2);
    psi_a_m1_p3 = simde_mm256_slli_epi16(psi_a_m1_p3, 2);
    psi_a_m1_p1 = simde_mm256_mulhi_epi16(psi_a_m1_p1, ONE_OVER_SQRT_2);
    psi_a_m1_p1 = simde_mm256_slli_epi16(psi_a_m1_p1, 2);
    psi_a_m1_m1 = simde_mm256_mulhi_epi16(psi_a_m1_m1, ONE_OVER_SQRT_2);
    psi_a_m1_m1 = simde_mm256_slli_epi16(psi_a_m1_m1, 2);
    psi_a_m1_m3 = simde_mm256_mulhi_epi16(psi_a_m1_m3, ONE_OVER_SQRT_2);
    psi_a_m1_m3 = simde_mm256_slli_epi16(psi_a_m1_m3, 2);
    psi_a_m1_m5 = simde_mm256_mulhi_epi16(psi_a_m1_m5, ONE_OVER_SQRT_2);
    psi_a_m1_m5 = simde_mm256_slli_epi16(psi_a_m1_m5, 2);
    psi_a_m1_m7 = simde_mm256_mulhi_epi16(psi_a_m1_m7, ONE_OVER_SQRT_2);
    psi_a_m1_m7 = simde_mm256_slli_epi16(psi_a_m1_m7, 2);
    psi_a_m3_p7 = simde_mm256_mulhi_epi16(psi_a_m3_p7, ONE_OVER_SQRT_2);
    psi_a_m3_p7 = simde_mm256_slli_epi16(psi_a_m3_p7, 2);
    psi_a_m3_p5 = simde_mm256_mulhi_epi16(psi_a_m3_p5, ONE_OVER_SQRT_2);
    psi_a_m3_p5 = simde_mm256_slli_epi16(psi_a_m3_p5, 2);
    psi_a_m3_p3 = simde_mm256_mulhi_epi16(psi_a_m3_p3, ONE_OVER_SQRT_2);
    psi_a_m3_p3 = simde_mm256_slli_epi16(psi_a_m3_p3, 2);
    psi_a_m3_p1 = simde_mm256_mulhi_epi16(psi_a_m3_p1, ONE_OVER_SQRT_2);
    psi_a_m3_p1 = simde_mm256_slli_epi16(psi_a_m3_p1, 2);
    psi_a_m3_m1 = simde_mm256_mulhi_epi16(psi_a_m3_m1, ONE_OVER_SQRT_2);
    psi_a_m3_m1 = simde_mm256_slli_epi16(psi_a_m3_m1, 2);
    psi_a_m3_m3 = simde_mm256_mulhi_epi16(psi_a_m3_m3, ONE_OVER_SQRT_2);
    psi_a_m3_m3 = simde_mm256_slli_epi16(psi_a_m3_m3, 2);
    psi_a_m3_m5 = simde_mm256_mulhi_epi16(psi_a_m3_m5, ONE_OVER_SQRT_2);
    psi_a_m3_m5 = simde_mm256_slli_epi16(psi_a_m3_m5, 2);
    psi_a_m3_m7 = simde_mm256_mulhi_epi16(psi_a_m3_m7, ONE_OVER_SQRT_2);
    psi_a_m3_m7 = simde_mm256_slli_epi16(psi_a_m3_m7, 2);
    psi_a_m5_p7 = simde_mm256_mulhi_epi16(psi_a_m5_p7, ONE_OVER_SQRT_2);
    psi_a_m5_p7 = simde_mm256_slli_epi16(psi_a_m5_p7, 2);
    psi_a_m5_p5 = simde_mm256_mulhi_epi16(psi_a_m5_p5, ONE_OVER_SQRT_2);
    psi_a_m5_p5 = simde_mm256_slli_epi16(psi_a_m5_p5, 2);
    psi_a_m5_p3 = simde_mm256_mulhi_epi16(psi_a_m5_p3, ONE_OVER_SQRT_2);
    psi_a_m5_p3 = simde_mm256_slli_epi16(psi_a_m5_p3, 2);
    psi_a_m5_p1 = simde_mm256_mulhi_epi16(psi_a_m5_p1, ONE_OVER_SQRT_2);
    psi_a_m5_p1 = simde_mm256_slli_epi16(psi_a_m5_p1, 2);
    psi_a_m5_m1 = simde_mm256_mulhi_epi16(psi_a_m5_m1, ONE_OVER_SQRT_2);
    psi_a_m5_m1 = simde_mm256_slli_epi16(psi_a_m5_m1, 2);
    psi_a_m5_m3 = simde_mm256_mulhi_epi16(psi_a_m5_m3, ONE_OVER_SQRT_2);
    psi_a_m5_m3 = simde_mm256_slli_epi16(psi_a_m5_m3, 2);
    psi_a_m5_m5 = simde_mm256_mulhi_epi16(psi_a_m5_m5, ONE_OVER_SQRT_2);
    psi_a_m5_m5 = simde_mm256_slli_epi16(psi_a_m5_m5, 2);
    psi_a_m5_m7 = simde_mm256_mulhi_epi16(psi_a_m5_m7, ONE_OVER_SQRT_2);
    psi_a_m5_m7 = simde_mm256_slli_epi16(psi_a_m5_m7, 2);
    psi_a_m7_p7 = simde_mm256_mulhi_epi16(psi_a_m7_p7, ONE_OVER_SQRT_2);
    psi_a_m7_p7 = simde_mm256_slli_epi16(psi_a_m7_p7, 2);
    psi_a_m7_p5 = simde_mm256_mulhi_epi16(psi_a_m7_p5, ONE_OVER_SQRT_2);
    psi_a_m7_p5 = simde_mm256_slli_epi16(psi_a_m7_p5, 2);
    psi_a_m7_p3 = simde_mm256_mulhi_epi16(psi_a_m7_p3, ONE_OVER_SQRT_2);
    psi_a_m7_p3 = simde_mm256_slli_epi16(psi_a_m7_p3, 2);
    psi_a_m7_p1 = simde_mm256_mulhi_epi16(psi_a_m7_p1, ONE_OVER_SQRT_2);
    psi_a_m7_p1 = simde_mm256_slli_epi16(psi_a_m7_p1, 2);
    psi_a_m7_m1 = simde_mm256_mulhi_epi16(psi_a_m7_m1, ONE_OVER_SQRT_2);
    psi_a_m7_m1 = simde_mm256_slli_epi16(psi_a_m7_m1, 2);
    psi_a_m7_m3 = simde_mm256_mulhi_epi16(psi_a_m7_m3, ONE_OVER_SQRT_2);
    psi_a_m7_m3 = simde_mm256_slli_epi16(psi_a_m7_m3, 2);
    psi_a_m7_m5 = simde_mm256_mulhi_epi16(psi_a_m7_m5, ONE_OVER_SQRT_2);
    psi_a_m7_m5 = simde_mm256_slli_epi16(psi_a_m7_m5, 2);
    psi_a_m7_m7 = simde_mm256_mulhi_epi16(psi_a_m7_m7, ONE_OVER_SQRT_2);
    psi_a_m7_m7 = simde_mm256_slli_epi16(psi_a_m7_m7, 2);

    // Calculation of a group of two terms in the bit metric involving squares of interference
    square_a_64qam_epi16(a_r_p7_p7, a_i_p7_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_p7);
    square_a_64qam_epi16(a_r_p7_p5, a_i_p7_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_p5);
    square_a_64qam_epi16(a_r_p7_p3, a_i_p7_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_p3);
    square_a_64qam_epi16(a_r_p7_p1, a_i_p7_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_p1);
    square_a_64qam_epi16(a_r_p7_m1, a_i_p7_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_m1);
    square_a_64qam_epi16(a_r_p7_m3, a_i_p7_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_m3);
    square_a_64qam_epi16(a_r_p7_m5, a_i_p7_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_m5);
    square_a_64qam_epi16(a_r_p7_m7, a_i_p7_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p7_m7);
    square_a_64qam_epi16(a_r_p5_p7, a_i_p5_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_p7);
    square_a_64qam_epi16(a_r_p5_p5, a_i_p5_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_p5);
    square_a_64qam_epi16(a_r_p5_p3, a_i_p5_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_p3);
    square_a_64qam_epi16(a_r_p5_p1, a_i_p5_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_p1);
    square_a_64qam_epi16(a_r_p5_m1, a_i_p5_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_m1);
    square_a_64qam_epi16(a_r_p5_m3, a_i_p5_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_m3);
    square_a_64qam_epi16(a_r_p5_m5, a_i_p5_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_m5);
    square_a_64qam_epi16(a_r_p5_m7, a_i_p5_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p5_m7);
    square_a_64qam_epi16(a_r_p3_p7, a_i_p3_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_p7);
    square_a_64qam_epi16(a_r_p3_p5, a_i_p3_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_p5);
    square_a_64qam_epi16(a_r_p3_p3, a_i_p3_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_p3);
    square_a_64qam_epi16(a_r_p3_p1, a_i_p3_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_p1);
    square_a_64qam_epi16(a_r_p3_m1, a_i_p3_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_m1);
    square_a_64qam_epi16(a_r_p3_m3, a_i_p3_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_m3);
    square_a_64qam_epi16(a_r_p3_m5, a_i_p3_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_m5);
    square_a_64qam_epi16(a_r_p3_m7, a_i_p3_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p3_m7);
    square_a_64qam_epi16(a_r_p1_p7, a_i_p1_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_p7);
    square_a_64qam_epi16(a_r_p1_p5, a_i_p1_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_p5);
    square_a_64qam_epi16(a_r_p1_p3, a_i_p1_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_p3);
    square_a_64qam_epi16(a_r_p1_p1, a_i_p1_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_p1);
    square_a_64qam_epi16(a_r_p1_m1, a_i_p1_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_m1);
    square_a_64qam_epi16(a_r_p1_m3, a_i_p1_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_m3);
    square_a_64qam_epi16(a_r_p1_m5, a_i_p1_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_m5);
    square_a_64qam_epi16(a_r_p1_m7, a_i_p1_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_p1_m7);
    square_a_64qam_epi16(a_r_m1_p7, a_i_m1_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_p7);
    square_a_64qam_epi16(a_r_m1_p5, a_i_m1_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_p5);
    square_a_64qam_epi16(a_r_m1_p3, a_i_m1_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_p3);
    square_a_64qam_epi16(a_r_m1_p1, a_i_m1_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_p1);
    square_a_64qam_epi16(a_r_m1_m1, a_i_m1_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_m1);
    square_a_64qam_epi16(a_r_m1_m3, a_i_m1_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_m3);
    square_a_64qam_epi16(a_r_m1_m5, a_i_m1_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_m5);
    square_a_64qam_epi16(a_r_m1_m7, a_i_m1_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m1_m7);
    square_a_64qam_epi16(a_r_m3_p7, a_i_m3_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_p7);
    square_a_64qam_epi16(a_r_m3_p5, a_i_m3_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_p5);
    square_a_64qam_epi16(a_r_m3_p3, a_i_m3_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_p3);
    square_a_64qam_epi16(a_r_m3_p1, a_i_m3_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_p1);
    square_a_64qam_epi16(a_r_m3_m1, a_i_m3_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_m1);
    square_a_64qam_epi16(a_r_m3_m3, a_i_m3_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_m3);
    square_a_64qam_epi16(a_r_m3_m5, a_i_m3_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_m5);
    square_a_64qam_epi16(a_r_m3_m7, a_i_m3_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m3_m7);
    square_a_64qam_epi16(a_r_m5_p7, a_i_m5_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_p7);
    square_a_64qam_epi16(a_r_m5_p5, a_i_m5_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_p5);
    square_a_64qam_epi16(a_r_m5_p3, a_i_m5_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_p3);
    square_a_64qam_epi16(a_r_m5_p1, a_i_m5_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_p1);
    square_a_64qam_epi16(a_r_m5_m1, a_i_m5_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_m1);
    square_a_64qam_epi16(a_r_m5_m3, a_i_m5_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_m3);
    square_a_64qam_epi16(a_r_m5_m5, a_i_m5_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_m5);
    square_a_64qam_epi16(a_r_m5_m7, a_i_m5_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m5_m7);
    square_a_64qam_epi16(a_r_m7_p7, a_i_m7_p7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_p7);
    square_a_64qam_epi16(a_r_m7_p5, a_i_m7_p5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_p5);
    square_a_64qam_epi16(a_r_m7_p3, a_i_m7_p3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_p3);
    square_a_64qam_epi16(a_r_m7_p1, a_i_m7_p1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_p1);
    square_a_64qam_epi16(a_r_m7_m1, a_i_m7_m1, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_m1);
    square_a_64qam_epi16(a_r_m7_m3, a_i_m7_m3, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_m3);
    square_a_64qam_epi16(a_r_m7_m5, a_i_m7_m5, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_m5);
    square_a_64qam_epi16(a_r_m7_m7, a_i_m7_m7, ch_mag_int, SQRT_42_OVER_FOUR, a_sq_m7_m7);

    // Computing different multiples of ||h0||^2
    // x=1, y=1
    ch_mag_2_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,ONE_OVER_FOUR_SQRT_42);
    ch_mag_2_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_2_over_42_with_sigma2,1);
    // x=1, y=3
    ch_mag_10_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,FIVE_OVER_FOUR_SQRT_42);
    ch_mag_10_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_10_over_42_with_sigma2,1);
    // x=1, x=5
    ch_mag_26_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,THIRTEEN_OVER_FOUR_SQRT_42);
    ch_mag_26_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_26_over_42_with_sigma2,1);
    // x=1, y=7
    ch_mag_50_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,TWENTYFIVE_OVER_FOUR_SQRT_42);
    ch_mag_50_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_50_over_42_with_sigma2,1);
    // x=3, y=3
    ch_mag_18_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,NINE_OVER_FOUR_SQRT_42);
    ch_mag_18_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_18_over_42_with_sigma2,1);
    // x=3, y=5
    ch_mag_34_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,SEVENTEEN_OVER_FOUR_SQRT_42);
    ch_mag_34_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_34_over_42_with_sigma2,1);
    // x=3, y=7
    ch_mag_58_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,TWENTYNINE_OVER_FOUR_SQRT_42);
    ch_mag_58_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_58_over_42_with_sigma2,2);
    // x=5, y=5
    ch_mag_50_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,TWENTYFIVE_OVER_FOUR_SQRT_42);
    ch_mag_50_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_50_over_42_with_sigma2,1);
    // x=5, y=7
    ch_mag_74_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,THIRTYSEVEN_OVER_FOUR_SQRT_42);
    ch_mag_74_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_74_over_42_with_sigma2,2);
    // x=7, y=7
    ch_mag_98_over_42_with_sigma2 = simde_mm256_mulhi_epi16(ch_mag_des,FORTYNINE_OVER_FOUR_SQRT_42);
    ch_mag_98_over_42_with_sigma2 = simde_mm256_slli_epi16(ch_mag_98_over_42_with_sigma2,2);

    // Computing Metrics
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_p7, a_sq_p7_p7);
    simde__m256i xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_7_7);
    simde__m256i bit_met_p7_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_p5, a_sq_p7_p5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_7_5);
    simde__m256i bit_met_p7_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_p3, a_sq_p7_p3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_7_3);
    simde__m256i bit_met_p7_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_p1, a_sq_p7_p1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_7_1);
    simde__m256i bit_met_p7_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_m1, a_sq_p7_m1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_7_1);
    simde__m256i bit_met_p7_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_m3, a_sq_p7_m3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_7_3);
    simde__m256i bit_met_p7_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_m5, a_sq_p7_m5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_7_5);
    simde__m256i bit_met_p7_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p7_m7, a_sq_p7_m7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_7_7);
    simde__m256i bit_met_p7_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_p7, a_sq_p5_p7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_5_7);
    simde__m256i bit_met_p5_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_p5, a_sq_p5_p5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_5_5);
    simde__m256i bit_met_p5_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_p3, a_sq_p5_p3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_5_3);
    simde__m256i bit_met_p5_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_p1, a_sq_p5_p1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_5_1);
    simde__m256i bit_met_p5_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_m1, a_sq_p5_m1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_5_1);
    simde__m256i bit_met_p5_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_m3, a_sq_p5_m3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_5_3);
    simde__m256i bit_met_p5_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_m5, a_sq_p5_m5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_5_5);
    simde__m256i bit_met_p5_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p5_m7, a_sq_p5_m7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_5_7);
    simde__m256i bit_met_p5_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_p7, a_sq_p3_p7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_3_7);
    simde__m256i bit_met_p3_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_p5, a_sq_p3_p5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_3_5);
    simde__m256i bit_met_p3_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_p3, a_sq_p3_p3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_3_3);
    simde__m256i bit_met_p3_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_p1, a_sq_p3_p1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_3_1);
    simde__m256i bit_met_p3_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_m1, a_sq_p3_m1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_3_1);
    simde__m256i bit_met_p3_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_m3, a_sq_p3_m3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_3_3);
    simde__m256i bit_met_p3_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_m5, a_sq_p3_m5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_3_5);
    simde__m256i bit_met_p3_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p3_m7, a_sq_p3_m7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_3_7);
    simde__m256i bit_met_p3_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_p7, a_sq_p1_p7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_1_7);
    simde__m256i bit_met_p1_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_p5, a_sq_p1_p5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_1_5);
    simde__m256i bit_met_p1_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_p3, a_sq_p1_p3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_1_3);
    simde__m256i bit_met_p1_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_p1, a_sq_p1_p1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_p_1_1);
    simde__m256i bit_met_p1_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_m1, a_sq_p1_m1);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_1_1);
    simde__m256i bit_met_p1_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_m3, a_sq_p1_m3);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_1_3);
    simde__m256i bit_met_p1_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_m5, a_sq_p1_m5);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_1_5);
    simde__m256i bit_met_p1_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_p1_m7, a_sq_p1_m7);
    xmm1 = simde_mm256_adds_epi16(xmm0, y0_m_1_7);
    simde__m256i bit_met_p1_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);

    xmm0 = simde_mm256_subs_epi16(psi_a_m1_p7, a_sq_m1_p7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_1_7);
    simde__m256i bit_met_m1_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m1_p5, a_sq_m1_p5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_1_5);
    simde__m256i bit_met_m1_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m1_p3, a_sq_m1_p3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_1_3);
    simde__m256i bit_met_m1_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m1_p1, a_sq_m1_p1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_1_1);
    simde__m256i bit_met_m1_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m1_m1, a_sq_m1_m1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_1_1);
    simde__m256i bit_met_m1_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_2_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m1_m3, a_sq_m1_m3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_1_3);
    simde__m256i bit_met_m1_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m1_m5, a_sq_m1_m5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_1_5);
    simde__m256i bit_met_m1_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m1_m7, a_sq_m1_m7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_1_7);
    simde__m256i bit_met_m1_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_p7, a_sq_m3_p7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_3_7);
    simde__m256i bit_met_m3_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_p5, a_sq_m3_p5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_3_5);
    simde__m256i bit_met_m3_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_p3, a_sq_m3_p3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_3_3);
    simde__m256i bit_met_m3_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_p1, a_sq_m3_p1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_3_1);
    simde__m256i bit_met_m3_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_m1, a_sq_m3_m1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_3_1);
    simde__m256i bit_met_m3_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_10_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_m3, a_sq_m3_m3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_3_3);
    simde__m256i bit_met_m3_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_18_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_m5, a_sq_m3_m5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_3_5);
    simde__m256i bit_met_m3_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m3_m7, a_sq_m3_m7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_3_7);
    simde__m256i bit_met_m3_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_p7, a_sq_m5_p7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_5_7);
    simde__m256i bit_met_m5_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_p5, a_sq_m5_p5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_5_5);
    simde__m256i bit_met_m5_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_p3, a_sq_m5_p3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_5_3);
    simde__m256i bit_met_m5_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_p1, a_sq_m5_p1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_5_1);
    simde__m256i bit_met_m5_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_m1, a_sq_m5_m1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_5_1);
    simde__m256i bit_met_m5_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_26_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_m3, a_sq_m5_m3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_5_3);
    simde__m256i bit_met_m5_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_34_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_m5, a_sq_m5_m5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_5_5);
    simde__m256i bit_met_m5_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m5_m7, a_sq_m5_m7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_5_7);
    simde__m256i bit_met_m5_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_p7, a_sq_m7_p7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_7_7);
    simde__m256i bit_met_m7_p7 = simde_mm256_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_p5, a_sq_m7_p5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_7_5);
    simde__m256i bit_met_m7_p5 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_p3, a_sq_m7_p3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_7_3);
    simde__m256i bit_met_m7_p3 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_p1, a_sq_m7_p1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_m_7_1);
    simde__m256i bit_met_m7_p1 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_m1, a_sq_m7_m1);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_7_1);
    simde__m256i bit_met_m7_m1 = simde_mm256_subs_epi16(xmm1, ch_mag_50_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_m3, a_sq_m7_m3);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_7_3);
    simde__m256i bit_met_m7_m3 = simde_mm256_subs_epi16(xmm1, ch_mag_58_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_m5, a_sq_m7_m5);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_7_5);
    simde__m256i bit_met_m7_m5 = simde_mm256_subs_epi16(xmm1, ch_mag_74_over_42_with_sigma2);
    xmm0 = simde_mm256_subs_epi16(psi_a_m7_m7, a_sq_m7_m7);
    xmm1 = simde_mm256_subs_epi16(xmm0, y0_p_7_7);
    simde__m256i bit_met_m7_m7 = simde_mm256_subs_epi16(xmm1, ch_mag_98_over_42_with_sigma2);

    // Detection for 1st bit (LTE mapping)
    // bit = 1
    xmm0 = simde_mm256_max_epi16(bit_met_m7_p7, bit_met_m7_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_m7_p3, bit_met_m7_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_m7_m1, bit_met_m7_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_m7_m5, bit_met_m7_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    simde__m256i logmax_den_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_m5_p7, bit_met_m5_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_m5_p3, bit_met_m5_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_m5_m1, bit_met_m5_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m5, bit_met_m5_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_m3_p7, bit_met_m3_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_m3_p3, bit_met_m3_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_m3_m1, bit_met_m3_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_m3_m5, bit_met_m3_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_m1_p7, bit_met_m1_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_m1_p3, bit_met_m1_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m1, bit_met_m1_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_m1_m5, bit_met_m1_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);

    // bit = 0
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p7, bit_met_p7_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_p7_p3, bit_met_p7_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_p7_m1, bit_met_p7_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_p7_m5, bit_met_p7_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    simde__m256i logmax_num_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p5_p7, bit_met_p5_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_p5_p3, bit_met_p5_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_p5_m1, bit_met_p5_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_p5_m5, bit_met_p5_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p3_p7, bit_met_p3_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p3, bit_met_p3_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_p3_m1, bit_met_p3_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_p3_m5, bit_met_p3_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p1_p7, bit_met_p1_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_p1_p3, bit_met_p1_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_p1_m1, bit_met_p1_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_p1_m5, bit_met_p1_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);

    y0r = simde_mm256_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 2nd bit (LTE mapping)
    // bit = 1
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);

    // bit = 0
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p1, bit_met_m7_p1);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p5, bit_met_m7_p5);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);

    y1r = simde_mm256_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 3rd bit (LTE mapping)
    xmm0 = simde_mm256_max_epi16(bit_met_m7_m7, bit_met_m7_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m7_m3, bit_met_m7_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m7_p1, bit_met_m7_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m7_p5, bit_met_m7_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_m5_m7, bit_met_m5_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m5_m3, bit_met_m5_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m5_p1, bit_met_m5_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p5, bit_met_m5_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p5_m7, bit_met_p5_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p5_m3, bit_met_p5_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p5_p1, bit_met_p5_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p5_p5, bit_met_p5_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m7, bit_met_p7_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p7_m3, bit_met_p7_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p7_p1, bit_met_p7_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p7_p5, bit_met_p7_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm256_max_epi16(bit_met_m3_m7, bit_met_m3_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m3_m3, bit_met_m3_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m3_p5, bit_met_m3_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_m1_m7, bit_met_m1_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m1_m3, bit_met_m1_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m1_p5, bit_met_m1_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p1_m7, bit_met_p1_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p1_m3, bit_met_p1_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p1_p5, bit_met_p1_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p3_m7, bit_met_p3_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m3, bit_met_p3_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p3_p5, bit_met_p3_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);

    simde__m256i y2r = simde_mm256_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 4th bit (LTE mapping)
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p5, bit_met_m5_p5);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm256_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p1, bit_met_m7_p1);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);

    y0i = simde_mm256_subs_epi16(logmax_num_re0, logmax_den_re0);


    // Detection for 5th bit (LTE mapping)
    xmm0 = simde_mm256_max_epi16(bit_met_m7_m7, bit_met_m7_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m7_m3, bit_met_m7_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m7_p1, bit_met_m7_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m7_p5, bit_met_m7_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_m1_m7, bit_met_m1_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m1_m3, bit_met_m1_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p1, bit_met_m1_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m1_p5, bit_met_m1_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p1_m7, bit_met_p1_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p1_m3, bit_met_p1_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p1_p1, bit_met_p1_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p1_p5, bit_met_p1_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m7, bit_met_p7_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p7_m3, bit_met_p7_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p7_p1, bit_met_p7_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p7_p5, bit_met_p7_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm256_max_epi16(bit_met_m5_m7, bit_met_m5_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m5_m3, bit_met_m5_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m5_p1, bit_met_m5_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p5, bit_met_m5_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_m3_m7, bit_met_m3_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_m3_m3, bit_met_m3_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m3_p1, bit_met_m3_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m3_p5, bit_met_m3_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p3_m7, bit_met_p3_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m3, bit_met_p3_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p3_p1, bit_met_p3_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p3_p5, bit_met_p3_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p5_m7, bit_met_p5_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p5_m3, bit_met_p5_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_p5_p1, bit_met_p5_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_p5_p5, bit_met_p5_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);

    y1i = simde_mm256_subs_epi16(logmax_num_re0, logmax_den_re0);

    // Detection for 6th bit (LTE mapping)
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p7, bit_met_p5_p7);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p7, bit_met_p1_p7);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p7, bit_met_m3_p7);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p7, bit_met_m7_p7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p1, bit_met_p5_p1);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p1, bit_met_p1_p1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p1, bit_met_m3_p1);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p1, bit_met_m5_p1);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m1, bit_met_p5_m1);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m1, bit_met_p1_m1);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m1, bit_met_m3_m1);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m1, bit_met_m7_m1);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m7, bit_met_p5_m7);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m7, bit_met_p1_m7);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m7, bit_met_m3_m7);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m7, bit_met_m7_m7);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm4);
    logmax_den_re0 = simde_mm256_max_epi16(logmax_den_re0, xmm5);

    xmm0 = simde_mm256_max_epi16(bit_met_p7_m5, bit_met_p5_m5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m5, bit_met_p1_m5);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m5, bit_met_m3_m5);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m5, bit_met_m7_m5);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(xmm4, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_m3, bit_met_p5_m3);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_m3, bit_met_p1_m3);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_m3, bit_met_m3_m3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_m3, bit_met_m7_m3);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p3, bit_met_p5_p3);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p3, bit_met_p1_p3);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p3, bit_met_m3_p3);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p3, bit_met_m7_p3);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);
    xmm0 = simde_mm256_max_epi16(bit_met_p7_p5, bit_met_p5_p5);
    xmm1 = simde_mm256_max_epi16(bit_met_p3_p5, bit_met_p1_p5);
    xmm2 = simde_mm256_max_epi16(bit_met_m1_p5, bit_met_m3_p5);
    xmm3 = simde_mm256_max_epi16(bit_met_m5_p5, bit_met_m7_p5);
    xmm4 = simde_mm256_max_epi16(xmm0, xmm1);
    xmm5 = simde_mm256_max_epi16(xmm2, xmm3);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm4);
    logmax_num_re0 = simde_mm256_max_epi16(logmax_num_re0, xmm5);

    simde__m256i y2i = simde_mm256_subs_epi16(logmax_num_re0, logmax_den_re0);

    // map to output stream, difficult to do in SIMD since we have 6 16bit LLRs
    // RE 1
    j = 48*i;
    stream0_out[j + 0] = ((short *)&y0r)[0];
    stream0_out[j + 1] = ((short *)&y1r)[0];
    stream0_out[j + 2] = ((short *)&y2r)[0];
    stream0_out[j + 3] = ((short *)&y0i)[0];
    stream0_out[j + 4] = ((short *)&y1i)[0];
    stream0_out[j + 5] = ((short *)&y2i)[0];
    // RE 2
    stream0_out[j + 6] = ((short *)&y0r)[1];
    stream0_out[j + 7] = ((short *)&y1r)[1];
    stream0_out[j + 8] = ((short *)&y2r)[1];
    stream0_out[j + 9] = ((short *)&y0i)[1];
    stream0_out[j + 10] = ((short *)&y1i)[1];
    stream0_out[j + 11] = ((short *)&y2i)[1];
    // RE 3
    stream0_out[j + 12] = ((short *)&y0r)[2];
    stream0_out[j + 13] = ((short *)&y1r)[2];
    stream0_out[j + 14] = ((short *)&y2r)[2];
    stream0_out[j + 15] = ((short *)&y0i)[2];
    stream0_out[j + 16] = ((short *)&y1i)[2];
    stream0_out[j + 17] = ((short *)&y2i)[2];
    // RE 4
    stream0_out[j + 18] = ((short *)&y0r)[3];
    stream0_out[j + 19] = ((short *)&y1r)[3];
    stream0_out[j + 20] = ((short *)&y2r)[3];
    stream0_out[j + 21] = ((short *)&y0i)[3];
    stream0_out[j + 22] = ((short *)&y1i)[3];
    stream0_out[j + 23] = ((short *)&y2i)[3];
    // RE 5
    stream0_out[j + 24] = ((short *)&y0r)[4];
    stream0_out[j + 25] = ((short *)&y1r)[4];
    stream0_out[j + 26] = ((short *)&y2r)[4];
    stream0_out[j + 27] = ((short *)&y0i)[4];
    stream0_out[j + 28] = ((short *)&y1i)[4];
    stream0_out[j + 29] = ((short *)&y2i)[4];
    // RE 6
    stream0_out[j + 30] = ((short *)&y0r)[5];
    stream0_out[j + 31] = ((short *)&y1r)[5];
    stream0_out[j + 32] = ((short *)&y2r)[5];
    stream0_out[j + 33] = ((short *)&y0i)[5];
    stream0_out[j + 34] = ((short *)&y1i)[5];
    stream0_out[j + 35] = ((short *)&y2i)[5];
    // RE 7
    stream0_out[j + 36] = ((short *)&y0r)[6];
    stream0_out[j + 37] = ((short *)&y1r)[6];
    stream0_out[j + 38] = ((short *)&y2r)[6];
    stream0_out[j + 39] = ((short *)&y0i)[6];
    stream0_out[j + 40] = ((short *)&y1i)[6];
    stream0_out[j + 41] = ((short *)&y2i)[6];
    // RE 8
    stream0_out[j + 42] = ((short *)&y0r)[7];
    stream0_out[j + 43] = ((short *)&y1r)[7];
    stream0_out[j + 44] = ((short *)&y2r)[7];
    stream0_out[j + 45] = ((short *)&y0i)[7];
    stream0_out[j + 46] = ((short *)&y1i)[7];
    stream0_out[j + 47] = ((short *)&y2i)[7];

    // RE 9
    stream0_out[j + 48] = ((short *)&y0r)[8];
    stream0_out[j + 49] = ((short *)&y1r)[8];
    stream0_out[j + 50] = ((short *)&y2r)[8];
    stream0_out[j + 51] = ((short *)&y0i)[8];
    stream0_out[j + 52] = ((short *)&y1i)[8];
    stream0_out[j + 53] = ((short *)&y2i)[8];
    // RE 10
    stream0_out[j + 54] = ((short *)&y0r)[9];
    stream0_out[j + 55] = ((short *)&y1r)[9];
    stream0_out[j + 56] = ((short *)&y2r)[9];
    stream0_out[j + 57] = ((short *)&y0i)[9];
    stream0_out[j + 58] = ((short *)&y1i)[9];
    stream0_out[j + 59] = ((short *)&y2i)[9];
    // RE 11
    stream0_out[j + 60] = ((short *)&y0r)[10];
    stream0_out[j + 61] = ((short *)&y1r)[10];
    stream0_out[j + 62] = ((short *)&y2r)[10];
    stream0_out[j + 63] = ((short *)&y0i)[10];
    stream0_out[j + 64] = ((short *)&y1i)[10];
    stream0_out[j + 65] = ((short *)&y2i)[10];
    // RE 12
    stream0_out[j + 66] = ((short *)&y0r)[11];
    stream0_out[j + 67] = ((short *)&y1r)[11];
    stream0_out[j + 68] = ((short *)&y2r)[11];
    stream0_out[j + 69] = ((short *)&y0i)[11];
    stream0_out[j + 70] = ((short *)&y1i)[11];
    stream0_out[j + 71] = ((short *)&y2i)[11];
    // RE 13
    stream0_out[j + 72] = ((short *)&y0r)[12];
    stream0_out[j + 73] = ((short *)&y1r)[12];
    stream0_out[j + 74] = ((short *)&y2r)[12];
    stream0_out[j + 75] = ((short *)&y0i)[12];
    stream0_out[j + 76] = ((short *)&y1i)[12];
    stream0_out[j + 77] = ((short *)&y2i)[12];
    // RE 14
    stream0_out[j + 78] = ((short *)&y0r)[13];
    stream0_out[j + 79] = ((short *)&y1r)[13];
    stream0_out[j + 80] = ((short *)&y2r)[13];
    stream0_out[j + 81] = ((short *)&y0i)[13];
    stream0_out[j + 82] = ((short *)&y1i)[13];
    stream0_out[j + 83] = ((short *)&y2i)[13];
    // RE 15
    stream0_out[j + 84] = ((short *)&y0r)[14];
    stream0_out[j + 85] = ((short *)&y1r)[14];
    stream0_out[j + 86] = ((short *)&y2r)[14];
    stream0_out[j + 87] = ((short *)&y0i)[14];
    stream0_out[j + 88] = ((short *)&y1i)[14];
    stream0_out[j + 89] = ((short *)&y2i)[14];
    // RE 16
    stream0_out[j + 90] = ((short *)&y0r)[15];
    stream0_out[j + 91] = ((short *)&y1r)[15];
    stream0_out[j + 92] = ((short *)&y2r)[15];
    stream0_out[j + 93] = ((short *)&y0i)[15];
    stream0_out[j + 94] = ((short *)&y1i)[15];
    stream0_out[j + 95] = ((short *)&y2i)[15];


  }

  simde_mm_empty();
  simde_m_empty();
}

#endif