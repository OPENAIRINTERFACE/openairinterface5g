/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*!
 * \brief Top-level routines for demodulating the PDSCH physical channel from 38-211, V15.2 2018-06
 */

#include "PHY/NR_REFSIG/ptrs_nr.h"
#include "common/platform_constants.h"
#include "nr_phy_common.h"
#include "nr_channel_compensation.h"
#include "nr_compute_llr.h"
#include "nr_layer_demapping.h"
#include "PHY/defs_nr_UE.h"
#include "nr_transport_proto_ue.h"
#include "PHY/sse_intrin.h"
#include "T.h"
#include "bits.h"
#include "openair1/PHY/NR_UE_ESTIMATION/nr_estimation.h"
#include "PHY/NR_REFSIG/nr_refsig.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "common/utils/nr/nr_common.h"
#include "utils.h"
#include <complex.h>
#include "openair1/PHY/TOOLS/phy_scope_interface.h"
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_nr_interface.h"

// #define DEBUG_HARQ(a...) printf(a)
#define DEBUG_HARQ(...)
//#define DEBUG_DLSCH_DEMOD
//#define DEBUG_PDSCH_RX

#define print_ints(s,x) printf("%s = %d %d %d %d\n",s,(x)[0],(x)[1],(x)[2],(x)[3])
#define print_shorts(s,x) printf("%s = [%d+j*%d, %d+j*%d, %d+j*%d, %d+j*%d]\n",s,(x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5],(x)[6],(x)[7])

static bool overlap_csi_symbol(fapi_nr_dl_config_csirs_pdu_rel15_t *csi_pdu, int symbol)
{
  int num_l0 [18] = {1, 1, 1, 1, 2, 1, 2, 2, 1, 2, 2, 2, 2, 2, 4, 2, 2, 4};
  for (int s = 0; s < num_l0[csi_pdu->row - 1]; s++) {
    if (symbol == csi_pdu->symb_l0 + s)
      return true;
  }
  // check also l1 if relevant
  if (csi_pdu->row == 13 || csi_pdu->row == 14 || csi_pdu->row == 16 || csi_pdu->row == 17) {
    for (int s = 0; s < 2; s++) { // two consecutive symbols including l1
      if (symbol == csi_pdu->symb_l1 + s)
        return true;
    }
  }
  return false;
}

static uint32_t build_csi_overlap_bitmap(fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config, int symbol)
{
  // LS 16 bits for even RBs, MS 16 bits for odd RBs
  uint32_t csi_res_bitmap = 0;
  int num_k[18] = {1, 1, 1, 1, 1, 4, 2, 2, 6, 3, 4, 4, 3, 3, 3, 4, 4, 4};
  for (int i = 0; i < dlsch_config->numCsiRsForRateMatching; i++) {
    fapi_nr_dl_config_csirs_pdu_rel15_t *csi_pdu = &dlsch_config->csiRsForRateMatching[i];

    if (!overlap_csi_symbol(csi_pdu, symbol))
      continue;

    int num_kp = 1;
    int mult = 1;
    int k0_step = 0;
    int num_k0 = 1;
    switch (csi_pdu->row) {
      case 1:
        k0_step = 4;
        num_k0 = 3;
        break;
      case 2:
        break;
      case 4:
        num_kp = 2;
        mult = 4;
        k0_step = 2;
        num_k0 = 2;
        break;
      default:
        num_kp = 2;
        mult = 2;
    }
    int found = 0;
    int bit = 0;
    uint32_t temp_res_map = 0;
    while (found < num_k[csi_pdu->row - 1]) {
      if ((csi_pdu->freq_domain >> bit) & 0x01) {
        for (int k0 = 0; k0 < num_k0; k0++) {
          for (int kp = 0; kp < num_kp; kp++) {
            int re = (bit * mult) + (k0 * k0_step) + kp;
            temp_res_map |= (1 << re);
          }
        }
        found++;
      }
      bit++;
      AssertFatal(bit < 13,
                  "Couldn't find %d positive bits in bitmap %d for CSI freq. domain\n",
                  num_k[csi_pdu->row - 1],
                  csi_pdu->freq_domain);
    }
    if (csi_pdu->freq_density < 2)
      csi_res_bitmap |= (temp_res_map << (16 * csi_pdu->freq_density));
    else
      csi_res_bitmap |= (temp_res_map + (temp_res_map << 16));
  }
  return csi_res_bitmap;
}

//==============================================================================================
// Pre-processing for LLR computation
//==============================================================================================

static void nr_dlsch_channel_level_median(uint32_t rx_size_symbol,
                                          c16_t dl_ch_estimates_ext[][rx_size_symbol],
                                          int32_t median[MAX_ANT][MAX_ANT],
                                          int n_tx,
                                          int n_rx,
                                          int length)
{
  for (int aatx = 0; aatx < n_tx; aatx++) {
    for (int aarx = 0; aarx < n_rx; aarx++) {
      int64_t max = median[aatx][aarx]; // initialize the med point for max
      int64_t min = median[aatx][aarx]; // initialize the med point for min
      simde__m128i *dl_ch128 = (simde__m128i *)dl_ch_estimates_ext[aatx * n_rx + aarx];

      const int length2 = length >> 2; // length = number of REs, hence length2=nb_REs*(32/128) in SIMD loop

      for (int ii = 0; ii < length2; ii++) {
        simde__m128i norm128D =
            simde_mm_srai_epi32(simde_mm_madd_epi16(*dl_ch128, *dl_ch128), 2); //[|H_0|²/4 |H_1|²/4 |H_2|²/4 |H_3|²/4]
        int32_t *tmp = (int32_t *)&norm128D;
        int64_t norm_pack = (int64_t)tmp[0] + tmp[1] + tmp[2] + tmp[3];

        if (norm_pack > max)
          max = norm_pack;
        if (norm_pack < min)
          min = norm_pack;
        dl_ch128+=1;
      }

      median[aatx][aarx] = (max + min) >> 1;
      LOG_D(PHY, "Channel level  median [%d][%d]: %d max = %ld min = %ld\n", aatx, aarx, median[aatx][aarx], max, min);
    }
  }
}

//==============================================================================================
// Extraction functions
//==============================================================================================

static void nr_dlsch_extract_rbs(uint32_t rxdataF_sz,
                                 c16_t rxdataF[][rxdataF_sz],
                                 uint32_t rx_size_symbol,
                                 uint32_t pdsch_est_size,
                                 int32_t dl_ch_estimates[][pdsch_est_size],
                                 c16_t rxdataF_ext[][rx_size_symbol],
                                 c16_t dl_ch_estimates_ext[][rx_size_symbol],
                                 unsigned char symbol,
                                 uint8_t pilots,
                                 const fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config,
                                 const freq_alloc_bitmap_t *freq_alloc,
                                 uint8_t Nl,
                                 NR_DL_FRAME_PARMS *fp,
                                 uint32_t csi_res_bitmap,
                                 int chest_time_type,
                                 uint16_t rnti,
                                 bool is_ptrs)
{
  int config_type = dlsch_config->dmrsConfigType;
  int n_dmrs_cdm_groups = dlsch_config->n_dmrs_cdm_groups;
  if (config_type == NFAPI_NR_DMRS_TYPE1)
    AssertFatal(n_dmrs_cdm_groups == 1
                || n_dmrs_cdm_groups == 2,
                "n_dmrs_cdm_groups %d is illegal\n",
                n_dmrs_cdm_groups);
  else
    AssertFatal(n_dmrs_cdm_groups == 1
                || n_dmrs_cdm_groups == 2 
                || n_dmrs_cdm_groups == 3,
                "n_dmrs_cdm_groups %d is illegal\n",
                n_dmrs_cdm_groups);

  uint32_t dmrs_rb_bitmap = 0;
  if (pilots) {
    dmrs_rb_bitmap = 0xfff; // all REs taken by dmrs
    if (config_type == NFAPI_NR_DMRS_TYPE1 && n_dmrs_cdm_groups == 1)
      dmrs_rb_bitmap = 0x555; // alternating REs starting from 0
    if (config_type == NFAPI_NR_DMRS_TYPE2 && n_dmrs_cdm_groups == 1)
      dmrs_rb_bitmap = 0xc3;  // REs 0,1 and 6,7
    if (config_type == NFAPI_NR_DMRS_TYPE2 && n_dmrs_cdm_groups == 2)
      dmrs_rb_bitmap = 0x3cf;  // REs 0,1,2,3 and 6,7,8,9
  }

  // csi_res_bitmap LS 16 bits for even RBs, MS 16 bits for odd RBs
  uint32_t csi_res_even = csi_res_bitmap & 0xfff;
  uint32_t csi_res_odd = (csi_res_bitmap >> 16) & 0xfff;
  AssertFatal((dmrs_rb_bitmap & csi_res_even) == 0, "DMRS RE overlapping with CSI RE, it shouldn't happen\n");
  AssertFatal((dmrs_rb_bitmap & csi_res_odd) == 0, "DMRS RE overlapping with CSI RE, it shouldn't happen\n");
  uint32_t dmrs_csi_overlap_even = csi_res_even | dmrs_rb_bitmap;
  uint32_t dmrs_csi_overlap_odd = csi_res_odd | dmrs_rb_bitmap;
  int8_t validDmrsEst;
  if (chest_time_type == 0)
    validDmrsEst = get_valid_dmrs_idx_for_channel_est(dlsch_config->dlDmrsSymbPos, symbol);
  else
    validDmrsEst = get_next_dmrs_symbol_in_slot(dlsch_config->dlDmrsSymbPos, 0, 14); // get first dmrs symbol index

  const uint k_rb_ref = is_ptrs ? get_ptrs_k_RB(freq_alloc->num_rbs, dlsch_config->PTRSFreqDensity, rnti) : 0;
  const uint k_ptrs = dlsch_config->PTRSFreqDensity;
  const uint k_re_ref = dlsch_config->PTRSReOffset;
  int pos = 0;
  int block_start, block_end;
  int offset = 0;
  while (find_next_rb_block(freq_alloc->bitmap, dlsch_config->BWPSize, &pos, &block_start, &block_end)) {
    int start_rb = block_start + dlsch_config->BWPStart;
    int nb_rb = block_end - block_start + 1;
    const int start_re = CIRCULAR_INC(fp->first_carrier_offset, start_rb * NR_NB_SC_PER_RB, fp->ofdm_symbol_size);
    for (int aarx = 0; aarx < fp->nb_antennas_rx; aarx++) {
      c16_t *rxF_ext = rxdataF_ext[aarx] + offset;
      c16_t *rxF = &rxdataF[aarx][symbol * fp->ofdm_symbol_size];
      for (int l = 0; l < Nl; l++) {
        int32_t *dl_ch0 = &dl_ch_estimates[(l * fp->nb_antennas_rx) + aarx][validDmrsEst * fp->ofdm_symbol_size];
        c16_t *dl_ch0_ext = dl_ch_estimates_ext[(l * fp->nb_antennas_rx) + aarx] + offset;
        if (!is_ptrs && pilots == 0 && csi_res_bitmap == 0) { // data symbol only
          if (l == 0) {
            if (start_re + nb_rb * NR_NB_SC_PER_RB <= fp->ofdm_symbol_size) {
              memcpy(rxF_ext, &rxF[start_re], nb_rb * NR_NB_SC_PER_RB * sizeof(int32_t));
            } else {
              int neg_length = fp->ofdm_symbol_size - start_re;
              int pos_length = nb_rb * NR_NB_SC_PER_RB - neg_length;
              memcpy(rxF_ext, &rxF[start_re], neg_length * sizeof(int32_t));
              memcpy(&rxF_ext[neg_length], rxF, pos_length * sizeof(int32_t));
            }
          }
          memcpy(dl_ch0_ext, dl_ch0, nb_rb * NR_NB_SC_PER_RB * sizeof(int32_t));
        } else {
          int j = 0;
          int k = start_re;
          for (int rb = start_rb; rb < start_rb + nb_rb; rb++) {
            const uint start_rb_alloc = freq_alloc->first_rb + dlsch_config->BWPStart;
            uint16_t ptrs_re_map = is_ptrs ? get_ptrs_re_bitmap(rb - start_rb_alloc, k_re_ref, k_ptrs, k_rb_ref) : 0;
            uint32_t overlap_map = (ptrs_re_map & 0xfff) | (rb % 2 ? dmrs_csi_overlap_odd : dmrs_csi_overlap_even);
            for (int re = 0; re < NR_NB_SC_PER_RB; re++) {
              if (((overlap_map >> re) & 0x01) == 0) {
                // DATA RE
                if (l == 0)
                  rxF_ext[j] = rxF[k];
                dl_ch0_ext[j] = *(c16_t *)(dl_ch0 + re);
                j++;
              }
              k = CIRCULAR_INC(k, 1, fp->ofdm_symbol_size);
            }
            dl_ch0 += 12;
          }
        }
      }
    }
    offset += nb_rb * NR_NB_SC_PER_RB;
  }
}

/*
 * nr_a_sum_b(): Compute the complex addition x=x+y
 */
void nr_a_sum_b(c16_t *input_x, c16_t *input_y, unsigned short nb_rb)
{
  simde__m128i *x = (simde__m128i *)input_x;
  simde__m128i *y = (simde__m128i *)input_y;
  for (int rb = 0; rb < nb_rb; rb++) {
    x[0] = simde_mm_adds_epi16(x[0], y[0]);
    x[1] = simde_mm_adds_epi16(x[1], y[1]);
    x[2] = simde_mm_adds_epi16(x[2], y[2]);
    x += 3;
    y += 3;
  }
}

/* Zero Forcing Rx function: nr_element_sign()
 * Compute b=sign*a */
static inline void nr_element_sign(c16_t *a, c16_t *b, unsigned short nb_rb, int32_t sign)
{
  const int16_t nr_sign[8] __attribute__((aligned(16))) = {-1, -1, -1, -1, -1, -1, -1, -1};
  simde__m128i *a_128,*b_128;

  a_128 = (simde__m128i *)a;
  b_128 = (simde__m128i *)b;

  for (int rb = 0; rb < 3 * nb_rb; rb++) {
    if (sign < 0)
      b_128[rb] = simde_mm_sign_epi16(a_128[rb], ((simde__m128i *)nr_sign)[0]);
    else
      b_128[rb] = a_128[rb];

#ifdef DEBUG_DLSCH_DEMOD
    print_shorts("b:", (int16_t *)b_128);
#endif
  }
}

/* Zero Forcing Rx function: nr_det_4x4()
 * Compute the matrix determinant for 4x4 Matrix
 *
 * */
static void nr_determin(int size,
                        c16_t *a44[][size], //
                        c16_t *ad_bc, // ad-bc
                        unsigned short nb_rb,
                        int32_t sign,
                        int32_t shift0)
{
  AssertFatal(size > 0, "impossible null size in nr_determin");

  if(size==1) {
    nr_element_sign(a44[0][0], // a
                    ad_bc, // b
                    nb_rb,
                    sign);
  } else {
    int16_t k, rr[size - 1], cc[size - 1];
    c16_t outtemp[12 * nb_rb] __attribute__((aligned(32)));
    c16_t outtemp1[12 * nb_rb] __attribute__((aligned(32)));
    c16_t *sub_matrix[size - 1][size - 1];
    for (int rtx=0;rtx<size;rtx++) {//row calculation for determin
      int ctx=0;
      //find the submatrix row and column indices
      k=0;
      for(int rrtx=0;rrtx<size;rrtx++)
        if(rrtx != rtx) rr[k++] = rrtx;
      k=0;
      for(int cctx=0;cctx<size;cctx++)
        if(cctx != ctx) cc[k++] = cctx;
      // fill out the sub matrix corresponds to this element

      for (int ridx = 0; ridx < (size - 1); ridx++)
        for (int cidx = 0; cidx < (size - 1); cidx++)
          sub_matrix[cidx][ridx] = a44[cc[cidx]][rr[ridx]];

      nr_determin(size - 1,
                  sub_matrix, // a33
                  outtemp,
                  nb_rb,
                  ((rtx & 1) == 1 ? -1 : 1) * ((ctx & 1) == 1 ? -1 : 1) * sign,
                  shift0);
      mult_complex_vectors(a44[ctx][rtx], outtemp, rtx == 0 ? ad_bc : outtemp1, sizeofArray(outtemp1), shift0);

      if (rtx != 0)
        nr_a_sum_b(ad_bc, outtemp1, nb_rb);
    }
  }
}

static double complex nr_determin_cpx(int32_t size, // size
                                      double complex a44_cpx[][size], //
                                      int32_t sign)
{
  double complex outtemp, outtemp1;
  //Allocate the submatrix elements
  DevAssert(size > 0);
  if(size==1) {
    return (a44_cpx[0][0] * sign);
  }else {
    double complex sub_matrix[size - 1][size - 1];
    int16_t k, rr[size - 1], cc[size - 1];
    outtemp1 = 0;
    for (int rtx=0;rtx<size;rtx++) {//row calculation for determin
      int ctx=0;
      //find the submatrix row and column indices
      k=0;
      for(int rrtx=0;rrtx<size;rrtx++)
        if(rrtx != rtx) rr[k++] = rrtx;
      k=0;
      for(int cctx=0;cctx<size;cctx++)
        if(cctx != ctx) cc[k++] = cctx;
      //fill out the sub matrix corresponds to this element
       for (int ridx=0;ridx<(size-1);ridx++)
         for (int cidx=0;cidx<(size-1);cidx++)
           sub_matrix[cidx][ridx] = a44_cpx[cc[cidx]][rr[ridx]];

       outtemp = nr_determin_cpx(size - 1,
                                 sub_matrix, // a33
                                 ((rtx & 1) == 1 ? -1 : 1) * ((ctx & 1) == 1 ? -1 : 1) * sign);
       outtemp1 += a44_cpx[ctx][rtx] * outtemp;
    }

    return((double complex)outtemp1);
  }
}

/* Zero Forcing Rx function: nr_matrix_inverse()
 * Compute the matrix inverse and determinant up to 4x4 Matrix
 *
 * */
uint8_t nr_matrix_inverse(int32_t size,
                          c16_t *a44[][size], // Input matrix//conjH_H_elements[0]
                          c16_t *inv_H_h_H[][size], // Inverse
                          c16_t *ad_bc, // determin
                          unsigned short nb_rb,
                          int32_t flag, // fixed point or floating flag
                          int32_t shift0)
{
  DevAssert(size > 1);
  int16_t k,rr[size-1],cc[size-1];

  if(flag) {//fixed point SIMD calc.
    //Allocate the submatrix elements
    c16_t *sub_matrix[size - 1][size - 1];

    //Compute Matrix determinant
    nr_determin(size,
                a44, //
                ad_bc, // determinant
                nb_rb,
                +1,
                shift0);
    //print_shorts("nr_det_",(int16_t*)&ad_bc[0]);

    //Compute Inversion of the H^*H matrix
    /* For 2x2 MIMO matrix, we compute
     * *        |(conj_H_00xH_00+conj_H_10xH_10)   (conj_H_00xH_01+conj_H_10xH_11)|
     * * H_h_H= |                                                                 |
     * *        |(conj_H_01xH_00+conj_H_11xH_10)   (conj_H_01xH_01+conj_H_11xH_11)|
     * *
     * *inv(H_h_H) =(1/det)*[d  -b
     * *                     -c  a]
     * **************************************************************************/
    for (int rtx=0;rtx<size;rtx++) {//row
      k=0;
      for(int rrtx=0;rrtx<size;rrtx++)
        if(rrtx != rtx) rr[k++] = rrtx;
      for (int ctx=0;ctx<size;ctx++) {//column
        k=0;
        for(int cctx=0;cctx<size;cctx++)
          if(cctx != ctx) cc[k++] = cctx;

        //fill out the sub matrix corresponds to this element
        for (int ridx=0;ridx<(size-1);ridx++)
          for (int cidx=0;cidx<(size-1);cidx++)
            // To verify
            sub_matrix[cidx][ridx]=a44[cc[cidx]][rr[ridx]];

        nr_determin(size - 1, // size
                    sub_matrix,
                    inv_H_h_H[rtx][ctx], // out transpose
                    nb_rb,
                    ((rtx & 1) == 1 ? -1 : 1) * ((ctx & 1) == 1 ? -1 : 1),
                    shift0);
      }
    }
  }
  else {//floating point calc.
    //Allocate the submatrix elements
    double complex sub_matrix_cpx[size - 1][size - 1];
    //Convert the IQ samples (in Q15 format) to float complex
    double complex a44_cpx[size][size];
    double complex inv_H_h_H_cpx[size][size];
    double complex determin_cpx;
    for (int i=0; i<12*nb_rb; i++) {

      //Convert Q15 to floating point
      for (int rtx=0;rtx<size;rtx++) {//row
        for (int ctx=0;ctx<size;ctx++) {//column
          a44_cpx[ctx][rtx] =
              ((double)(a44[ctx][rtx])[i].r) / (1 << (shift0 - 1)) + I * ((double)(a44[ctx][rtx])[i].i) / (1 << (shift0 - 1));
        }
      }
      //Compute Matrix determinant (copy real value only)
      determin_cpx = nr_determin_cpx(size,
                                     a44_cpx, //
                                     +1);
      //if (i<4) printf("order %d nr_det_cpx = %lf+j%lf \n",log2_approx(creal(determin_cpx)),creal(determin_cpx),cimag(determin_cpx));

      //Round and convert to Q15 (Out in the same format as Fixed point).
      if (creal(determin_cpx)>0) {//determin of the symmetric matrix is real part only
        ((short *)ad_bc)[i << 1] = (short)((creal(determin_cpx) * (1 << (shift0))) + 0.5); //
      } else {
        ((short *)ad_bc)[i << 1] = (short)((creal(determin_cpx) * (1 << (shift0))) - 0.5); //
      }
      //Compute Inversion of the H^*H matrix (normalized output divide by determinant)
      for (int rtx=0;rtx<size;rtx++) {//row
        k=0;
        for(int rrtx=0;rrtx<size;rrtx++)
          if(rrtx != rtx) rr[k++] = rrtx;
        for (int ctx=0;ctx<size;ctx++) {//column
          k=0;
          for(int cctx=0;cctx<size;cctx++)
            if(cctx != ctx) cc[k++] = cctx;

          //fill out the sub matrix corresponds to this element
          for (int ridx=0;ridx<(size-1);ridx++)
            for (int cidx=0;cidx<(size-1);cidx++)
              sub_matrix_cpx[cidx][ridx] = a44_cpx[cc[cidx]][rr[ridx]];

          inv_H_h_H_cpx[rtx][ctx] = nr_determin_cpx(size - 1, // size,
                                                    sub_matrix_cpx, //
                                                    ((rtx & 1) == 1 ? -1 : 1) * ((ctx & 1) == 1 ? -1 : 1));
          //if (i==0) printf("H_h_H(r%d,c%d)=%lf+j%lf --> inv_H_h_H(%d,%d) = %lf+j%lf \n",rtx,ctx,creal(a44_cpx[ctx*size+rtx]),cimag(a44_cpx[ctx*size+rtx]),ctx,rtx,creal(inv_H_h_H_cpx[rtx*size+ctx]),cimag(inv_H_h_H_cpx[rtx*size+ctx]));

          if (creal(inv_H_h_H_cpx[rtx][ctx]) > 0)
            inv_H_h_H[rtx][ctx][i].r = (short)((creal(inv_H_h_H_cpx[rtx][ctx]) * (1 << (shift0 - 1))) + 0.5); // Convert to Q 18
          else
            inv_H_h_H[rtx][ctx][i].r = (short)((creal(inv_H_h_H_cpx[rtx][ctx]) * (1 << (shift0 - 1))) - 0.5); //

          if (cimag(inv_H_h_H_cpx[rtx][ctx]) > 0)
            inv_H_h_H[rtx][ctx][i].i = (short)((cimag(inv_H_h_H_cpx[rtx][ctx]) * (1 << (shift0 - 1))) + 0.5); //
          else
            inv_H_h_H[rtx][ctx][i].i = (short)((cimag(inv_H_h_H_cpx[rtx][ctx]) * (1 << (shift0 - 1))) - 0.5); //

          //if (i<4) printf("inv_H_h_H_FP(%d,%d)= %d+j%d \n",ctx,rtx, ((short *) inv_H_h_H[rtx*size+ctx])[i<<1],((short *) inv_H_h_H[rtx*size+ctx])[(i<<1)+1]);
        }
      }
    }
  }
  return(0);
}

/*
 * MMSE Rx function: up to 4 layers
 */
static void nr_dlsch_mmse(uint32_t pdsch_buf_size_max,
                          uint32_t rx_size_symbol,
                          unsigned char n_rx,
                          unsigned char nl, // number of layer
                          c16_t rxdataF_comp[nl][pdsch_buf_size_max],
                          c16_t dl_ch_mag[][pdsch_buf_size_max],
                          c16_t dl_ch_magb[][pdsch_buf_size_max],
                          c16_t dl_ch_magr[][pdsch_buf_size_max],
                          c16_t dl_ch_estimates_ext[][rx_size_symbol],
                          unsigned char mod_order,
                          int shift,
                          int length,
                          uint32_t noise_var)
{
  uint32_t nb_rb_0 = (length + 11) / 12;
  c16_t determ_fin[12 * nb_rb_0] __attribute__((aligned(32)));

  ///Allocate H^*H matrix elements and sub elements
  c16_t conjH_H_elements_data[n_rx][nl][nl][12 * nb_rb_0];
  memset(conjH_H_elements_data, 0, sizeof(conjH_H_elements_data));
  c16_t *conjH_H_elements[n_rx][nl][nl];
  for (int aarx = 0; aarx < n_rx; aarx++)
    for (int rtx = 0; rtx < nl; rtx++)
      for (int ctx = 0; ctx < nl; ctx++)
        conjH_H_elements[aarx][rtx][ctx] = conjH_H_elements_data[aarx][rtx][ctx];

  //Compute H^*H matrix elements and sub elements:(1/2^log2_maxh)*conjH_H_elements
  for (int rtx = 0; rtx < nl; rtx++) {//row
    for (int ctx = 0; ctx < nl; ctx++) {//column
      for (int aarx = 0; aarx < n_rx; aarx++)  {
        c16_t *ch0r = dl_ch_estimates_ext[rtx * n_rx + aarx];
        c16_t *ch0c = dl_ch_estimates_ext[ctx * n_rx + aarx];
        mult_cpx_conj_vector(ch0r,
                            ch0c,
                            conjH_H_elements[aarx][ctx][rtx], // sic
                            nb_rb_0 * NR_NB_SC_PER_RB,
                            shift);
        if (aarx != 0)
          nr_a_sum_b(conjH_H_elements[0][ctx][rtx], conjH_H_elements[aarx][ctx][rtx], nb_rb_0);
      }
    }
  }

  // Add noise_var such that: H^h * H + noise_var * I
  if (noise_var != 0) {
    simde__m128i nvar_128i = simde_mm_set1_epi32(noise_var >> 3);
    for (int p = 0; p < nl; p++) {
      simde__m128i *conjH_H_128i = (simde__m128i *)conjH_H_elements[0][p][p];
      for (int k = 0; k < 3 * nb_rb_0; k++) {
        conjH_H_128i[0] = simde_mm_add_epi32(conjH_H_128i[0], nvar_128i);
        conjH_H_128i++;
      }
    }
  }

  //Compute the inverse and determinant of the H^*H matrix
  //Allocate the inverse matrix
  c16_t *inv_H_h_H[nl][nl];
  c16_t inv_H_h_H_data[nl][nl][12 * nb_rb_0];
  memset(inv_H_h_H_data, 0, sizeof(inv_H_h_H_data));
  for (int rtx = 0; rtx < nl; rtx++)
    for (int ctx = 0; ctx < nl; ctx++)
      inv_H_h_H[ctx][rtx] = inv_H_h_H_data[ctx][rtx];

  int fp_flag = 1;//0: float point calc 1: Fixed point calc
  nr_matrix_inverse(nl,
                    conjH_H_elements[0], // Input matrix
                    inv_H_h_H, // Inverse
                    determ_fin, // determin
                    nb_rb_0,
                    fp_flag, // fixed point flag
                    shift - (fp_flag == 1 ? 1 : 0)); // the out put is Q15

  // multiply Matrix inversion pf H_h_H by the rx signal vector
  c16_t outtemp[12 * nb_rb_0] __attribute__((aligned(32)));
  //Allocate rxdataF for zforcing out
  c16_t rxdataF_zforcing[nl][12 * nb_rb_0];
  memset(rxdataF_zforcing, 0, sizeof(rxdataF_zforcing));

  for (int rtx = 0; rtx < nl; rtx++) {//Output Layers row
    // loop over Layers rtx=0,...,N_Layers-1
    for (int ctx = 0; ctx < nl; ctx++) { // column multi
      // printf("Computing r_%d c_%d\n",rtx,ctx);
      // print_shorts(" H_h_H=",(int16_t*)&conjH_H_elements[ctx*nl+rtx][0][0]);
      // print_shorts(" Inv_H_h_H=",(int16_t*)&inv_H_h_H[ctx*nl+rtx][0]);
      mult_complex_vectors(inv_H_h_H[ctx][rtx],
                           rxdataF_comp[ctx],
                           outtemp,
                           sizeofArray(outtemp),
                           shift - (fp_flag == 1 ? 1 : 0));
      nr_a_sum_b(rxdataF_zforcing[rtx], outtemp, nb_rb_0); // a = a + b
    }
#ifdef DEBUG_DLSCH_DEMOD
    printf("Computing layer_%d \n", rtx);
    print_shorts(" Rx signal:=", (int16_t*)&rxdataF_zforcing[rtx][0]);
    print_shorts(" Rx signal:=", (int16_t*)&rxdataF_zforcing[rtx][4]);
    print_shorts(" Rx signal:=", (int16_t*)&rxdataF_zforcing[rtx][8]);
#endif
  }

  //Copy zero_forcing out to output array
  for (int rtx = 0; rtx < nl; rtx++)
    nr_element_sign(rxdataF_zforcing[rtx], rxdataF_comp[rtx], nb_rb_0, +1);

  //Update LLR thresholds with the Matrix determinant
  simde__m128i *dl_ch_mag128_0=NULL,*dl_ch_mag128b_0=NULL,*dl_ch_mag128r_0=NULL,*determ_fin_128;
  simde__m128i mmtmpD2,mmtmpD3;
  simde__m128i QAM_amp128={0},QAM_amp128b={0},QAM_amp128r={0};
  short nr_realpart[8]__attribute__((aligned(16))) = {1,0,1,0,1,0,1,0};
  determ_fin_128      = (simde__m128i *)&determ_fin[0];

  if (mod_order > 2) {
    if (mod_order == 4) {
      QAM_amp128 = simde_mm_set1_epi16(QAM16_n1);  //2/sqrt(10)
      QAM_amp128b = simde_mm_setzero_si128();
      QAM_amp128r = simde_mm_setzero_si128();
    } else if (mod_order == 6) {
      QAM_amp128  = simde_mm_set1_epi16(QAM64_n1); //4/sqrt{42}
      QAM_amp128b = simde_mm_set1_epi16(QAM64_n2); //2/sqrt{42}
      QAM_amp128r = simde_mm_setzero_si128();
    } else if (mod_order == 8) {
      QAM_amp128 = simde_mm_set1_epi16(QAM256_n1); //8/sqrt{170}
      QAM_amp128b = simde_mm_set1_epi16(QAM256_n2);//4/sqrt{170}
      QAM_amp128r = simde_mm_set1_epi16(QAM256_n3);//2/sqrt{170}
    }
    dl_ch_mag128_0 = (simde__m128i *)dl_ch_mag[0];
    dl_ch_mag128b_0 = (simde__m128i *)dl_ch_magb[0];
    dl_ch_mag128r_0 = (simde__m128i *)dl_ch_magr[0];

    for (int rb = 0; rb < 3 * nb_rb_0; rb++) {
      //for symmetric H_h_H matrix, the determinant is only real values
      mmtmpD2 = simde_mm_sign_epi16(determ_fin_128[0],*(simde__m128i*)&nr_realpart[0]);//set imag part to 0
      mmtmpD3 = simde_mm_shufflelo_epi16(mmtmpD2,SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD3 = simde_mm_shufflehi_epi16(mmtmpD3,SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD2 = simde_mm_add_epi16(mmtmpD2,mmtmpD3);

      dl_ch_mag128_0[0] = mmtmpD2;
      dl_ch_mag128b_0[0] = mmtmpD2;
      dl_ch_mag128r_0[0] = mmtmpD2;

      dl_ch_mag128_0[0] = simde_mm_mulhrs_epi16(dl_ch_mag128_0[0], QAM_amp128);
      dl_ch_mag128b_0[0] = simde_mm_mulhrs_epi16(dl_ch_mag128b_0[0],QAM_amp128b);
      dl_ch_mag128r_0[0] = simde_mm_mulhrs_epi16(dl_ch_mag128r_0[0],QAM_amp128r);

      determ_fin_128 += 1;
      dl_ch_mag128_0 += 1;
      dl_ch_mag128b_0 += 1;
      dl_ch_mag128r_0 += 1;
    }
  }
}

static void nr_dlsch_layer_demapping(const uint8_t Nl,
                                     const uint8_t mod_order,
                                     const int llrLayerSize,
                                     const int16_t llr_layers[NR_SYMBOLS_PER_SLOT][Nl][llrLayerSize],
                                     const fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config,
                                     const uint32_t re_len[NR_SYMBOLS_PER_SLOT],
                                     int16_t *llr)
{
  const int s0 = dlsch_config->start_symbol;
  const int s1 = dlsch_config->number_symbols;
  int k = 0;

  for (int i = s0; i < (s0 + s1); i++) {
    int16_t *p_layer[Nl];
    for (int l = 0; l < Nl; l++)
      p_layer[l] = (int16_t *)llr_layers[i][l];
    nr_layer_demapping(Nl, mod_order, re_len[i], p_layer, llr + k);
    k += re_len[i] * mod_order * Nl;
  }
}

/* Computes LLRs from compensated PDSCH signal per OFDM symbol for all layers */
static int nr_dlsch_llr(const NR_UE_DLSCH_t *dlsch,
                        const int len,
                        const int pdsch_buf_size_max,
                        const c16_t dl_ch_mag[pdsch_buf_size_max],
                        const c16_t dl_ch_magb[pdsch_buf_size_max],
                        const c16_t dl_ch_magr[pdsch_buf_size_max],
                        const int nb_antennas_rx,
                        const c16_t rxdataF_comp[dlsch->cw_info.Nl][pdsch_buf_size_max],
                        const int llrSize,
                        int16_t layer_llr[dlsch->cw_info.Nl][llrSize])
{
  switch (dlsch->cw_info.qamModOrder) {
    case 2 :
      for (int l = 0; l < dlsch->cw_info.Nl; l++)
        nr_qpsk_llr(rxdataF_comp[l], layer_llr[l], len);
      break;

    case 4 :
      for (int l = 0; l < dlsch->cw_info.Nl; l++)
        nr_16qam_llr(rxdataF_comp[l], dl_ch_mag, layer_llr[l], len);
      break;

    case 6 :
      for(int l=0; l < dlsch->cw_info.Nl; l++)
        nr_64qam_llr(rxdataF_comp[l], dl_ch_mag, dl_ch_magb, layer_llr[l], len);
      break;

    case 8:
      for(int l=0; l < dlsch->cw_info.Nl; l++)
        nr_256qam_llr(rxdataF_comp[l], dl_ch_mag, dl_ch_magb, dl_ch_magr, layer_llr[l], len);
      break;

    default:
      AssertFatal(false, "Unknown mod_order!!!!\n");
      break;
  }

  return 0;
}
//==============================================================================================

/* Main Function */

int nr_rx_pdsch(PHY_VARS_NR_UE *ue,
                const UE_nr_rxtx_proc_t *proc,
                NR_UE_DLSCH_t *dlsch,
                const freq_alloc_bitmap_t *freq_alloc,
                fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config,
                NR_DL_UE_HARQ_t *dlsch_harq,
                unsigned char symbol,
                bool first_symbol_flag,
                unsigned char harq_pid,
                uint32_t pdsch_est_size,
                int32_t dl_ch_estimates[][pdsch_est_size],
                int16_t *llr,
                uint32_t dl_valid_re[NR_SYMBOLS_PER_SLOT],
                c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP],
                int32_t *log2_maxh,
                uint32_t pdsch_buf_size_max,
                int nbRx,
                c16_t rxdataF_comp[][NR_MAX_NB_LAYERS][pdsch_buf_size_max],
                c16_t dl_ch_mag[][NR_MAX_NB_LAYERS][pdsch_buf_size_max],
                c16_t dl_ch_magb[][NR_MAX_NB_LAYERS][pdsch_buf_size_max],
                c16_t dl_ch_magr[][NR_MAX_NB_LAYERS][pdsch_buf_size_max],
                c16_t ptrs_phase,
                uint ptrs_re_per_symbol,
                uint32_t nvar,
                pdsch_scope_req_t *scope_req,
                c16_t rho_dl[][NR_MAX_NB_LAYERS * NR_MAX_NB_LAYERS][pdsch_buf_size_max],
                uint16_t is_ptrs)
{
  NR_DL_FRAME_PARMS *fp = &ue->frame_parms;
  const int nl = dlsch->cw_info.Nl;
  const int matrixSz = nbRx * nl;
  const uint32_t rx_size_symbol = ceil_mod(freq_alloc->num_rbs * NR_NB_SC_PER_RB, 16);
  __attribute__((aligned(64))) c16_t dl_ch_estimates_ext[matrixSz][rx_size_symbol];

  // Use ML-based LLR for 2-layer MIMO with QPSK/16QAM/64QAM (nl==2, qamModOrder<=6).
  // Controlled by ue->do_ml (set via -E flag in dlsim, or ue->do_ml in the UE struct).
  // When false (default), MMSE equalization is used for all configurations.
  bool do_ml = ue->do_ml;

  // Reinterpret flat dl_ch_estimates_ext as [nl][nbRx][rx_size_symbol]
  c16_t(*chFext)[nbRx][rx_size_symbol] = (void *)dl_ch_estimates_ext;

  c16_t *p_rxComp[nl];
  for (int l = 0; l < nl; l++)
    p_rxComp[l] = rxdataF_comp[symbol][l];

  NR_UE_COMMON *common_vars  = &ue->common_vars;
  const int frame = proc->frame_rx;
  const int nr_slot_rx = proc->nr_slot_rx;
  const int gNB_id = proc->gNB_id;
  uint8_t slot = 0;

  uint32_t nb_re_pdsch = -1;
  DevAssert(dlsch_harq);

  if (gNB_id > 2) {
    LOG_E(PHY, "Illegal gNB_id %d\n", gNB_id);
    return(-1);
  }

  if (!common_vars) {
    LOG_E(PHY, "dlsch_demodulation.c: Null common_vars\n");
    return(-1);
  }

  if(symbol > fp->symbols_per_slot >> 1)
    slot = 1;

  uint8_t pilots = (dlsch_config->dlDmrsSymbPos >> symbol) & 1;
  uint8_t config_type = dlsch_config->dmrsConfigType;

  const bool need_rho = do_ml ? (nl == 2 && dlsch_config->cw_info->qamModOrder <= 6) : false;

  //----------------------------------------------------------
  //--------------------- RBs extraction ---------------------
  //----------------------------------------------------------
  const bool meas_enabled = cpumeas(CPUMEAS_GETSTATE);
  int nb_rb_pdsch = freq_alloc->num_rbs;

  start_meas_nr_ue_phy(ue, DLSCH_EXTRACT_RBS_STATS);
  __attribute__((aligned(64))) c16_t rxdataF_ext[nbRx][rx_size_symbol];
  memset(rxdataF_ext, 0, sizeof(rxdataF_ext));

  uint32_t csi_res_bitmap = build_csi_overlap_bitmap(dlsch_config, symbol);
  LOG_D(PHY, "%d.%d symbol %d csi overlap bitmap %d\n", frame, nr_slot_rx, symbol, csi_res_bitmap);

  nr_dlsch_extract_rbs(fp->samples_per_slot_wCP,
                       rxdataF,
                       rx_size_symbol,
                       pdsch_est_size,
                       dl_ch_estimates,
                       rxdataF_ext,
                       dl_ch_estimates_ext,
                       symbol,
                       pilots,
                       dlsch_config,
                       freq_alloc,
                       nl,
                       fp,
                       csi_res_bitmap,
                       ue->chest_time,
                       dlsch->rnti,
                       is_ptrs);
  stop_meas_nr_ue_phy(ue, DLSCH_EXTRACT_RBS_STATS);
  if (scope_req->copy_chanest_to_scope) {
    size_t size = sizeof(c16_t) * nb_rb_pdsch * NR_NB_SC_PER_RB;
    int copy_index = symbol - dlsch_config->start_symbol;
    int offset = copy_index * size;
    UEscopeCopyUnsafe(ue, pdschChanEstimates, dl_ch_estimates_ext[0], size, offset, copy_index);
  }
  if (meas_enabled) {
    LOG_D(PHY,
          "[AbsSFN %u.%d] Slot%d Symbol %d: Pilot/Data extraction %5.2f \n",
          frame,
          nr_slot_rx,
          slot,
          symbol,
          ue->phy_cpu_stats.cpu_time_stats[DLSCH_EXTRACT_RBS_STATS].p_time / (cpuf * 1000.0));
  }
  if (ue->phy_sim_pdsch_rxdataF_ext)
    memcpy(ue->phy_sim_pdsch_rxdataF_ext + symbol * sizeof(rxdataF_ext), rxdataF_ext, sizeof(rxdataF_ext));

  nb_re_pdsch = (pilots == 1) ?
                ((config_type == NFAPI_NR_DMRS_TYPE1) ? nb_rb_pdsch * (12 - 6 * dlsch_config->n_dmrs_cdm_groups) :
                nb_rb_pdsch * (12 - 4 * dlsch_config->n_dmrs_cdm_groups)):
                (nb_rb_pdsch * 12);
  nb_re_pdsch -= ptrs_re_per_symbol;
  // Subtract CSI-RS REs from PDSCH RE count
  if (csi_res_bitmap != 0) {
    uint32_t csi_re_count = 0;
    uint32_t csi_res_even = csi_res_bitmap & 0xfff;
    uint32_t csi_res_odd = (csi_res_bitmap >> 16) & 0xfff;
    uint32_t count_even = count_bits(&csi_res_even, 1);
    uint32_t count_odd  = count_bits(&csi_res_odd, 1);
    int start = freq_alloc->first_rb + dlsch_config->BWPStart;
    int end = freq_alloc->last_rb + 1;
    for (int rb = start; rb < end; rb++) {
      if ((freq_alloc->bitmap[rb / 32] >> (rb % 32)) & 0x01)
        csi_re_count += (rb % 2 == 0) ? count_even : count_odd;
    }
    nb_re_pdsch = (nb_re_pdsch > csi_re_count) ? (nb_re_pdsch - csi_re_count) : 0;
    if (csi_re_count > 0) {
      LOG_D(NR_PHY,
            "[CSI OVERLAP] Frame/Slot %d.%d Symbol %d: CSI-RS overlapping PDSCH - %d CSI-RS REs skipped, %d data REs extracted\n",
            frame,
            nr_slot_rx,
            symbol,
            csi_re_count,
            nb_re_pdsch);
    }
  }

  if (scope_req->copy_rxdataF_to_scope) {
    size_t size = sizeof(c16_t) * nb_re_pdsch;
    int copy_index = symbol - dlsch_config->start_symbol;
    UEscopeCopyUnsafe(ue, pdschRxdataF, rxdataF_ext[0], size, scope_req->scope_rxdataF_offset, copy_index);
    scope_req->scope_rxdataF_offset += size;
  }

  //----------------------------------------------------------
  //--------------------- Channel Level Calc. ----------------
  //----------------------------------------------------------
  start_meas_nr_ue_phy(ue, DLSCH_CHANNEL_LEVEL_STATS);
  if (first_symbol_flag) {
    int32_t avg[nl][nbRx];
    if (nb_re_pdsch)
      for (int i = 0; i < nl; i++)
        nr_channel_level(0, rx_size_symbol, chFext[i], nbRx, avg[i], nb_re_pdsch);
    else
      LOG_E(NR_PHY, "Average channel level is 0: nb_rb_pdsch = %d, nb_re_pdsch = %d\n", nb_rb_pdsch, nb_re_pdsch);
    int avgs = 0;
    int32_t median[MAX_ANT][MAX_ANT];
    for (int l = 0; l < nl; l++)
      for (int aarx = 0; aarx < nbRx; aarx++) {
        avgs = cmax(avgs, avg[l][aarx]);
        LOG_D(PHY, "nb_rb %d avg_%d_%d Power per SC is %d\n", nb_rb_pdsch, aarx, l, avg[l][aarx]);
        LOG_D(PHY, "avgs Power per SC is %d\n", avgs);
        median[l][aarx] = avg[l][aarx];
      }

    if (nl > 1) {
      nr_dlsch_channel_level_median(rx_size_symbol, dl_ch_estimates_ext, median, nl, nbRx, nb_re_pdsch);
      for (int l = 0; l < nl; l++) {
        for (int aarx = 0; aarx < nbRx; aarx++) {
          avgs = cmax(avgs, median[l][aarx]);
        }
      }
    }
    // Output shift: half channel energy (log2|h|^2/2) + MRC antenna gain.
    // Single-layer adds +1 guard bit (raw peak); multi-layer uses median so no guard needed.
    if (nl == 1)
      *log2_maxh = (log2_approx(avgs) >> 1) + 1 + log2_approx(nbRx >> 1);
    else
      *log2_maxh = (log2_approx(avgs) >> 1) + log2_approx(nbRx >> 1);
    LOG_D(PHY, "[DLSCH] AbsSubframe %d.%d log2_maxh = %d (%d)\n", frame % 1024, nr_slot_rx, *log2_maxh, avgs);
#if T_TRACER
    T(T_UE_PHY_PDSCH_ENERGY,
      T_INT(gNB_id),
      T_INT(frame % 1024),
      T_INT(nr_slot_rx),
      T_INT(avg[0][0]), // layer 0, antenna 0
      T_INT(nbRx > 1 ? avg[0][1] : 0), // layer 0, antenna 1
      T_INT(nl > 1 ? avg[1][0] : 0), // layer 1, antenna 0
      T_INT(nl > 1 && nbRx > 1 ? avg[1][1] : 0)); // layer 1, antenna 1
#endif
  }
  stop_meas_nr_ue_phy(ue, DLSCH_CHANNEL_LEVEL_STATS);
  if (meas_enabled) {
    LOG_D(PHY,
          "[AbsSFN %u.%d] Slot%d Symbol %d first_symbol_flag %d: Channel Level  %5.2f \n",
          frame,
          nr_slot_rx,
          slot,
          symbol,
          first_symbol_flag,
          ue->phy_cpu_stats.cpu_time_stats[DLSCH_CHANNEL_LEVEL_STATS].p_time / (cpuf * 1000.0));
  }

  //----------------------------------------------------------
  //--------------------- channel compensation ---------------
  //----------------------------------------------------------
  start_meas_nr_ue_phy(ue, DLSCH_CHANNEL_COMPENSATION_STATS);
  nr_channel_compensation(rx_size_symbol,
                          pdsch_buf_size_max,
                          nbRx,
                          nl,
                          rxdataF_ext,
                          chFext,
                          dl_ch_mag[symbol],
                          dl_ch_magb[symbol],
                          dl_ch_magr[symbol],
                          p_rxComp,
                          need_rho ? (c16_t(*)[nl][pdsch_buf_size_max])rho_dl[symbol] : NULL,
                          ptrs_phase,
                          dlsch->cw_info.qamModOrder,
                          0, // symbol already baked into p_rxComp
                          *log2_maxh);
  stop_meas_nr_ue_phy(ue, DLSCH_CHANNEL_COMPENSATION_STATS);
  if (meas_enabled) {
    LOG_D(PHY,
          "[AbsSFN %u.%d] Slot%d Symbol %d log2_maxh %d Channel Comp  %5.2f \n",
          frame,
          nr_slot_rx,
          slot,
          symbol,
          *log2_maxh,
          ue->phy_cpu_stats.cpu_time_stats[DLSCH_CHANNEL_COMPENSATION_STATS].p_time / (cpuf * 1000.0));
  }
  // Please keep it: useful for debugging
#ifdef DEBUG_PDSCH_RX
  char filename[50];
  snprintf(filename, 50, "rxdataF0_symb_%d_nr_slot_rx_%d.m", symbol, nr_slot_rx);
  write_output(filename, "rxdataF0", &rxdataF[0][symbol * fp->ofdm_symbol_size], fp->ofdm_symbol_size, 1, 1);
  snprintf(filename, 50, "dl_ch_estimates0_symb_%d_nr_slot_rx_%d.m", symbol, nr_slot_rx);
  write_output(filename, "dl_ch_estimates0", &dl_ch_estimates[0][symbol * fp->ofdm_symbol_size], fp->ofdm_symbol_size, 1, 1);
  snprintf(filename, 50, "rxdataF_ext0_symb_%d_nr_slot_rx_%d.m", symbol, nr_slot_rx);
  write_output(filename, "rxdataF_ext0", &rxdataF_ext[0][0], rx_size_symbol, 1, 1);
  snprintf(filename, 50, "dl_ch_estimates_ext0_symb_%d_nr_slot_rx_%d.m", symbol, nr_slot_rx);
  write_output(filename, "dl_ch_estimates_ext0", &dl_ch_estimates_ext[0][0], rx_size_symbol, 1, 1);
  snprintf(filename, 50, "rxdataF_comp00_symb_%d_nr_slot_rx_%d.m", symbol, nr_slot_rx);
  write_output(filename, "rxdataF_comp00", &rxdataF_comp[0][0][symbol * pdsch_buf_size_max], pdsch_buf_size_max, 1, 1);
#endif

  // MRC is performed inline by nr_channel_compensation; apply MMSE for multi-layer
  start_meas_nr_ue_phy(ue, DLSCH_MRC_MMSE_STATS);
  if (nb_re_pdsch) {
    const uint8_t qamModOrder = dlsch->cw_info.qamModOrder;

    if ((nl > 2) || (nl == 2 && !do_ml)) {
      nr_dlsch_mmse(pdsch_buf_size_max,
                    rx_size_symbol,
                    nbRx,
                    nl,
                    rxdataF_comp[symbol],
                    dl_ch_mag[symbol],
                    dl_ch_magb[symbol],
                    dl_ch_magr[symbol],
                    dl_ch_estimates_ext,
                    qamModOrder,
                    *log2_maxh,
                    nb_re_pdsch,
                    nvar);
    } else if ((nl == 2) && (qamModOrder > 6) && do_ml) {
      nr_mmse_2layers(p_rxComp,
                      rx_size_symbol,
                      pdsch_buf_size_max,
                      nbRx,
                      nl,
                      dl_ch_mag[symbol],
                      dl_ch_magb[symbol],
                      dl_ch_magr[symbol],
                      chFext,
                      freq_alloc->num_rbs,
                      qamModOrder,
                      *log2_maxh,
                      0,
                      nb_re_pdsch,
                      nvar);
    }
  }
  stop_meas_nr_ue_phy(ue, DLSCH_MRC_MMSE_STATS);

  if (meas_enabled) {
    LOG_D(PHY,
          "[AbsSFN %u.%d] Slot%d Symbol %d: Channel Combine and MMSE %5.2f \n",
          frame,
          nr_slot_rx,
          slot,
          symbol,
          ue->phy_cpu_stats.cpu_time_stats[DLSCH_MRC_MMSE_STATS].p_time / (cpuf * 1000.0));
  }

  /* Store the valid DL RE's */
  dl_valid_re[symbol] = nb_re_pdsch;
  int startSymbIdx = 0;
  int nbSymb = 0;

  if(dlsch_harq->status == NR_ACTIVE) {
    startSymbIdx = dlsch_config->start_symbol;
    nbSymb = dlsch_config->number_symbols;
  }

  /* at last symbol in a slot calculate LLR's for whole slot */
  if (symbol == (startSymbIdx + nbSymb - 1)) {
    /* create LLR layer buffer */
    int max_symb_re = 0;
    GET_ARRAY_MAX(dl_valid_re, NR_SYMBOLS_PER_SLOT, max_symb_re);
    const int llr_per_symbol = max_symb_re * dlsch->cw_info.qamModOrder;
    __attribute__((aligned(64))) int16_t layer_llr[NR_SYMBOLS_PER_SLOT][nl][llr_per_symbol];

    // Generate LLR from PTRS compensated signal
    const uint8_t qamModOrder = dlsch->cw_info.qamModOrder;
    start_meas_nr_ue_phy(ue, DLSCH_LLR_STATS);
    for (int llr_sym = startSymbIdx; llr_sym < startSymbIdx + nbSymb; llr_sym++) {
      if (nl == 2 && qamModOrder <= 6 && do_ml) {
        // 2-layer QPSK/16QAM/64QAM: joint ML-LLR using inter-layer Tx correlation
        // rho_dl[llr_sym] is laid out as [nl*nl][rx_size_symbol]:
        // index 1 = rho[0][1], index nl (=2) = rho[1][0]
        nr_compute_ML_llr(rxdataF_comp[llr_sym][0],
                          rxdataF_comp[llr_sym][1],
                          dl_ch_mag[llr_sym][0],
                          dl_ch_mag[llr_sym][1],
                          layer_llr[llr_sym][0],
                          layer_llr[llr_sym][1],
                          rho_dl[llr_sym][1],
                          rho_dl[llr_sym][nl],
                          dl_valid_re[llr_sym],
                          qamModOrder);
      } else {
        nr_dlsch_llr(dlsch,
                     dl_valid_re[llr_sym],
                     pdsch_buf_size_max,
                     dl_ch_mag[llr_sym][0],
                     dl_ch_magb[llr_sym][0],
                     dl_ch_magr[llr_sym][0],
                     nbRx,
                     rxdataF_comp[llr_sym],
                     llr_per_symbol,
                     layer_llr[llr_sym]);
      }
    }
    stop_meas_nr_ue_phy(ue, DLSCH_LLR_STATS);
    start_meas_nr_ue_phy(ue, DLSCH_LAYER_DEMAPPING);
    nr_dlsch_layer_demapping(nl, dlsch->cw_info.qamModOrder, llr_per_symbol, layer_llr, dlsch_config, dl_valid_re, llr);
    stop_meas_nr_ue_phy(ue, DLSCH_LAYER_DEMAPPING);

    if (UEScopeHasTryLock(ue)) {
      metadata mt = {.frame = proc->frame_rx, .slot = proc->nr_slot_rx };
      int total_valid_res = 0;
      for (int i = startSymbIdx; i < startSymbIdx + nbSymb; i++) {
        total_valid_res += dl_valid_re[i];
      }
      if (UETryLockScopeData(ue, pdschRxdataF_comp, sizeof(c16_t), 1,  total_valid_res, &mt)) {
        size_t offset = 0;
        for (int i = startSymbIdx; i < startSymbIdx + nbSymb; i++) {
          size_t data_size = sizeof(c16_t) * dl_valid_re[i];
          UEscopeCopyUnsafe(ue, pdschRxdataF_comp, &rxdataF_comp[i][0][0], data_size, offset, i);
          offset += data_size;
        }
        UEunlockScopeData(ue, pdschRxdataF_comp)
      }
    } else {
      UEscopeCopy(ue, pdschRxdataF_comp, rxdataF_comp[0], sizeof(c16_t), nl, pdsch_buf_size_max, 0);
    }
  }

  if (meas_enabled) {
    LOG_D(PHY,
          "[AbsSFN %u.%d] Slot%d Symbol %d: LLR Computation  %5.2f \n",
          frame,
          nr_slot_rx,
          slot,
          symbol,
          ue->phy_cpu_stats.cpu_time_stats[DLSCH_LLR_STATS].p_time / (cpuf * 1000.0));
  }

#if T_TRACER
  T(T_UE_PHY_PDSCH_IQ,
    T_INT(gNB_id),
    T_INT(frame % 1024),
    T_INT(nr_slot_rx),
    T_INT(nb_rb_pdsch),
    T_INT(fp->N_RB_DL),
    T_INT(fp->symbols_per_slot),
    T_BUFFER(&rxdataF_comp[gNB_id][0], 2 * fp->N_RB_DL * 12 * fp->symbols_per_slot * 2));
#endif

  if (ue->phy_sim_pdsch_rxdataF_comp) {
    for (int a = 0; a < nbRx; a++) {
      memcpy((c16_t *)ue->phy_sim_pdsch_dl_ch_estimates + pdsch_est_size * a, dl_ch_estimates, pdsch_est_size * sizeof(c16_t));
    }
    for (int l = 0; l < nl; l++) {
      int offset = (void *)rxdataF_comp[symbol][l] - (void *)rxdataF_comp[0];
      memcpy(ue->phy_sim_pdsch_rxdataF_comp + offset, rxdataF_comp[symbol][l], sizeof(c16_t) * pdsch_buf_size_max);
    }
  }
  if (ue->phy_sim_pdsch_dl_ch_estimates_ext)
    memcpy(ue->phy_sim_pdsch_dl_ch_estimates_ext + symbol * sizeof(dl_ch_estimates_ext),
           dl_ch_estimates_ext,
           sizeof(dl_ch_estimates_ext));
  return 0;
}
