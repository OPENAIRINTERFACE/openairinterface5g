#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "nr_transport_proto.h"
#include "PHY/impl_defs_top.h"
#include "PHY/NR_TRANSPORT/nr_sch_dmrs.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_REFSIG/ptrs_nr.h"
#include "PHY/NR_ESTIMATION/nr_ul_estimation.h"
#include "PHY/defs_nr_common.h"
#include "common/utils/nr/nr_common.h"
#include <openair1/PHY/TOOLS/phy_scope_interface.h>
#include "PHY/sse_intrin.h"

#define INVALID_VALUE 255

void nr_idft(int32_t *z, uint32_t Msc_PUSCH)
{

  simde__m128i idft_in128[1][3240], idft_out128[1][3240];
  simde__m128i norm128;
  int16_t *idft_in0 = (int16_t*)idft_in128[0], *idft_out0 = (int16_t*)idft_out128[0];

  int i, ip;

  LOG_T(PHY,"Doing lte_idft for Msc_PUSCH %d\n",Msc_PUSCH);

  if ((Msc_PUSCH % 1536) > 0) {
    // conjugate input
    for (i = 0; i < (Msc_PUSCH>>2); i++) {
      *&(((simde__m128i*)z)[i]) = simde_mm_sign_epi16(*&(((simde__m128i*)z)[i]), *(simde__m128i*)&conjugate2[0]);
    }
    for (i = 0, ip = 0; i < Msc_PUSCH; i++, ip+=4)
      ((uint32_t*)idft_in0)[ip+0] = z[i];
  }

  switch (Msc_PUSCH) {
    case 12:
      dft(DFT_12,(int16_t *)idft_in0, (int16_t *)idft_out0,0);

      norm128 = simde_mm_set1_epi16(9459);

      for (i = 0; i < 12; i++) {
        ((simde__m128i*)idft_out0)[i] = simde_mm_slli_epi16(simde_mm_mulhi_epi16(((simde__m128i*)idft_out0)[i], norm128), 1);
      }

      break;

    case 24:
      dft(DFT_24,idft_in0, idft_out0, 1);
      break;

    case 36:
      dft(DFT_36,idft_in0, idft_out0, 1);
      break;

    case 48:
      dft(DFT_48,idft_in0, idft_out0, 1);
      break;

    case 60:
      dft(DFT_60,idft_in0, idft_out0, 1);
      break;

    case 72:
      dft(DFT_72,idft_in0, idft_out0, 1);
      break;

    case 96:
      dft(DFT_96,idft_in0, idft_out0, 1);
      break;

    case 108:
      dft(DFT_108,idft_in0, idft_out0, 1);
      break;

    case 120:
      dft(DFT_120,idft_in0, idft_out0, 1);
      break;

    case 144:
      dft(DFT_144,idft_in0, idft_out0, 1);
      break;

    case 180:
      dft(DFT_180,idft_in0, idft_out0, 1);
      break;

    case 192:
      dft(DFT_192,idft_in0, idft_out0, 1);
      break;

    case 216:
      dft(DFT_216,idft_in0, idft_out0, 1);
      break;

    case 240:
      dft(DFT_240,idft_in0, idft_out0, 1);
      break;

    case 288:
      dft(DFT_288,idft_in0, idft_out0, 1);
      break;

    case 300:
      dft(DFT_300,idft_in0, idft_out0, 1);
      break;

    case 324:
      dft(DFT_324,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 360:
      dft(DFT_360,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 384:
      dft(DFT_384,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 432:
      dft(DFT_432,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 480:
      dft(DFT_480,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 540:
      dft(DFT_540,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 576:
      dft(DFT_576,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 600:
      dft(DFT_600,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 648:
      dft(DFT_648,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 720:
      dft(DFT_720,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 768:
      dft(DFT_768,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 864:
      dft(DFT_864,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 900:
      dft(DFT_900,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 960:
      dft(DFT_960,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 972:
      dft(DFT_972,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 1080:
      dft(DFT_1080,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 1152:
      dft(DFT_1152,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 1200:
      dft(DFT_1200,idft_in0, idft_out0, 1);
      break;

    case 1296:
      dft(DFT_1296,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 1440:
      dft(DFT_1440,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 1500:
      dft(DFT_1500,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 1536:
      //dft(DFT_1536,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      idft(IDFT_1536,(int16_t*)z, (int16_t*)z, 1);
      break;

    case 1620:
      dft(DFT_1620,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 1728:
      dft(DFT_1728,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 1800:
      dft(DFT_1800,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 1920:
      dft(DFT_1920,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 1944:
      dft(DFT_1944,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 2160:
      dft(DFT_2160,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 2304:
      dft(DFT_2304,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 2400:
      dft(DFT_2400,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 2592:
      dft(DFT_2592,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 2700:
      dft(DFT_2700,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 2880:
      dft(DFT_2880,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 2916:
      dft(DFT_2916,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 3000:
      dft(DFT_3000,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 3072:
      //dft(DFT_3072,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      idft(IDFT_3072,(int16_t*)z, (int16_t*)z, 1);
      break;

    case 3240:
      dft(DFT_3240,(int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    default:
      // should not be reached
      LOG_E( PHY, "Unsupported Msc_PUSCH value of %"PRIu16"\n", Msc_PUSCH );
      return;
  }

  if ((Msc_PUSCH % 1536) > 0) {
    for (i = 0, ip = 0; i < Msc_PUSCH; i++, ip+=4)
      z[i] = ((uint32_t*)idft_out0)[ip];

    // conjugate output
    for (i = 0; i < (Msc_PUSCH>>2); i++) {
      ((simde__m128i*)z)[i] = simde_mm_sign_epi16(((simde__m128i*)z)[i], *(simde__m128i*)&conjugate2[0]);
    }
  }

  simde_mm_empty();
  simde_m_empty();

}

static void nr_ulsch_extract_rbs (c16_t* const rxdataF,
                                   c16_t* const chF,
                                   c16_t *rxFext,
                                   c16_t *chFext,
                                   int rxoffset,
                                   int choffset,
                                   int aarx,
                                   int is_dmrs_symbol,
                                   nfapi_nr_pusch_pdu_t *pusch_pdu,
                                   NR_DL_FRAME_PARMS *frame_parms)
{
  uint8_t delta = 0;

  int start_re = (frame_parms->first_carrier_offset + (pusch_pdu->rb_start + pusch_pdu->bwp_start) * NR_NB_SC_PER_RB)%frame_parms->ofdm_symbol_size;
  int nb_re_pusch = NR_NB_SC_PER_RB * pusch_pdu->rb_size;


  c16_t *rxF        = &rxdataF[rxoffset];
  c16_t *rxF_ext    = &rxFext[0];
  c16_t *ul_ch0     = &chF[choffset]; 
  c16_t *ul_ch0_ext = &chFext[0];

  if (is_dmrs_symbol == 0) {
    if (start_re + nb_re_pusch <= frame_parms->ofdm_symbol_size)
      memcpy(rxF_ext, &rxF[start_re], nb_re_pusch * sizeof(c16_t));
    else 
    {
      int neg_length = frame_parms->ofdm_symbol_size - start_re;
      int pos_length = nb_re_pusch - neg_length;
      memcpy(rxF_ext, &rxF[start_re], neg_length * sizeof(c16_t));
      memcpy(&rxF_ext[neg_length], rxF, pos_length * sizeof(c16_t));
    }
   memcpy(ul_ch0_ext, ul_ch0, nb_re_pusch * sizeof(c16_t));
  }
  else if (pusch_pdu->dmrs_config_type == pusch_dmrs_type1) // 6 REs / PRB
  {
    AssertFatal(delta == 0 || delta == 1, "Illegal delta %d\n",delta);
    c16_t *rxF32        = &rxF[start_re];
    if (start_re + nb_re_pusch < frame_parms->ofdm_symbol_size) {
      for (int idx = 1 - delta; idx < nb_re_pusch; idx += 2) 
      {
        *rxF_ext++ = rxF32[idx];
        *ul_ch0_ext++ = ul_ch0[idx];
      }
    }
    else // handle the two pieces around DC
    {
      int neg_length = frame_parms->ofdm_symbol_size - start_re;
      int pos_length = nb_re_pusch - neg_length;
      int idx, idx2;
      for (idx = 1 - delta; idx < neg_length; idx += 2) 
      {
        *rxF_ext++ = rxF32[idx];
        *ul_ch0_ext++= ul_ch0[idx];
      }
      rxF32 = rxF;
      idx2 = idx;
      for (idx = 1 - delta; idx < pos_length; idx += 2, idx2 += 2) 
      {
        *rxF_ext++ = rxF32[idx];
        *ul_ch0_ext++ = ul_ch0[idx2];
      }
    }
  }
  else if (pusch_pdu->dmrs_config_type == pusch_dmrs_type2)  // 8 REs / PRB
  {
    AssertFatal(delta==0||delta==2||delta==4,"Illegal delta %d\n",delta);
    if (start_re + nb_re_pusch < frame_parms->ofdm_symbol_size) 
    {
      for (int idx = 0; idx < nb_re_pusch; idx ++) 
      {
        if (idx % 6 == 2 * delta || idx % 6 == 2 * delta + 1)
          continue;
        *rxF_ext++ = rxF[idx];
        *ul_ch0_ext++ = ul_ch0[idx];
      }
    }
    else 
    {
      int neg_length = frame_parms->ofdm_symbol_size - start_re;
      int pos_length = nb_re_pusch - neg_length;
      c16_t *rxF64 = &rxF[start_re];
      int idx, idx2;
      for (idx = 0; idx < neg_length; idx ++) 
      {
        if (idx % 6 == 2 * delta || idx % 6 == 2 * delta + 1)
          continue;
        *rxF_ext++ = rxF64[idx];
        *ul_ch0_ext++ = ul_ch0[idx];
      }
      rxF64 = rxF;
      idx2 = idx;
      for (idx = 0; idx < pos_length; idx++, idx2++) 
      {
        if (idx % 6 == 2 * delta || idx % 6 == 2 * delta + 1)
          continue;
        *rxF_ext++ = rxF64[idx];
        *ul_ch0_ext++ = ul_ch0[idx2];
      }
    }
  }
}

static void nr_ulsch_scale_channel(int **ul_ch_estimates_ext,
                                   NR_DL_FRAME_PARMS *frame_parms,
                                   uint8_t symbol,
                                   uint8_t is_dmrs_symbol,
                                   uint32_t len,
                                   uint8_t nrOfLayers,
                                   unsigned short nb_rb,
                                   int shift_ch_ext)
{
  // Determine scaling amplitude based the symbol
  int b = 3;
  short ch_amp = 1024 * 8;
  if (shift_ch_ext > 3) {
    b = 0;
    ch_amp >>= (shift_ch_ext - 3);
    if (ch_amp == 0) {
      ch_amp = 1;
    }
  } else {
    b -= shift_ch_ext;
  }
  simde__m128i ch_amp128 = simde_mm_set1_epi16(ch_amp); // Q3.13
  LOG_D(PHY, "Scaling PUSCH Chest in OFDM symbol %d by %d, pilots %d nb_rb %d NCP %d symbol %d\n", symbol, ch_amp, is_dmrs_symbol, nb_rb, frame_parms->Ncp, symbol);

  for (int aatx = 0; aatx < nrOfLayers; aatx++) {
    for (int aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      simde__m128i *ul_ch128 = (simde__m128i *)&ul_ch_estimates_ext[aatx * frame_parms->nb_antennas_rx + aarx][symbol * len];
      for (int i = 0; i < len >> 2; i++) {
        ul_ch128[i] = simde_mm_mulhi_epi16(ul_ch128[i], ch_amp128);
        ul_ch128[i] = simde_mm_slli_epi16(ul_ch128[i], b);
      }
    }
  }
}

static int get_nb_re_pusch (NR_DL_FRAME_PARMS *frame_parms, nfapi_nr_pusch_pdu_t *rel15_ul,int symbol) 
{
  uint8_t dmrs_symbol_flag = (rel15_ul->ul_dmrs_symb_pos >> symbol) & 0x01;
  if (dmrs_symbol_flag == 1) {
    if ((rel15_ul->ul_dmrs_symb_pos >> ((symbol + 1) % frame_parms->symbols_per_slot)) & 0x01)
      AssertFatal(1==0,"Double DMRS configuration is not yet supported\n");

    if (rel15_ul->dmrs_config_type == 0) {
      // if no data in dmrs cdm group is 1 only even REs have no data
      // if no data in dmrs cdm group is 2 both odd and even REs have no data
      return(rel15_ul->rb_size *(12 - (rel15_ul->num_dmrs_cdm_grps_no_data*6)));
    }
    else return(rel15_ul->rb_size *(12 - (rel15_ul->num_dmrs_cdm_grps_no_data*4)));
  } else return(rel15_ul->rb_size * NR_NB_SC_PER_RB);
}

// compute average channel_level on each (TX,RX) antenna pair
static void nr_ulsch_channel_level(int **ul_ch_estimates_ext,
                                   NR_DL_FRAME_PARMS *frame_parms,
                                   int32_t *avg,
                                   uint8_t symbol,
                                   uint32_t len,
                                   uint8_t nrOfLayers)
{
  simde__m128i *ul_ch128, avg128U;

  int16_t x = factor2(len);
  int16_t y = (len)>>x;

  for (int aatx = 0; aatx < nrOfLayers; aatx++) {
    for (int aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      //clear average level
      avg128U = simde_mm_setzero_si128();

      ul_ch128 = (simde__m128i *)&ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol * len];

      for (int i = 0; i < len >> 2; i++) {
        avg128U = simde_mm_add_epi32(avg128U, simde_mm_srai_epi32(simde_mm_madd_epi16(ul_ch128[i], ul_ch128[i]), x));
      }

      avg[aatx*frame_parms->nb_antennas_rx+aarx] = (((int32_t*)&avg128U)[0] +
                                                    ((int32_t*)&avg128U)[1] +
                                                    ((int32_t*)&avg128U)[2] +
                                                    ((int32_t*)&avg128U)[3]) / y;
    }
  }

  simde_mm_empty();
  simde_m_empty();
}

static void nr_ulsch_channel_compensation(c16_t *rxFext,
                                          c16_t *chFext,
                                          c16_t *ul_ch_maga,
                                          c16_t *ul_ch_magb,
                                          c16_t *ul_ch_magc,
                                          int32_t **rxComp,
                                          c16_t *rho,
                                          NR_DL_FRAME_PARMS *frame_parms,
                                          nfapi_nr_pusch_pdu_t* rel15_ul,
                                          uint32_t symbol,
                                          uint32_t buffer_length,
                                          uint32_t output_shift)
{
  int mod_order = rel15_ul->qam_mod_order;
  int nrOfLayers = rel15_ul->nrOfLayers;
  int nb_rx_ant = frame_parms->nb_antennas_rx;

  simde__m256i QAM_ampa_256 = simde_mm256_setzero_si256();
  simde__m256i QAM_ampb_256 = simde_mm256_setzero_si256();
  simde__m256i QAM_ampc_256 = simde_mm256_setzero_si256();

  if (mod_order == 4) {
    QAM_ampa_256 = simde_mm256_set1_epi16(QAM16_n1);
    QAM_ampb_256 = simde_mm256_setzero_si256();
    QAM_ampc_256 = simde_mm256_setzero_si256();
  }
  else if (mod_order == 6) {
    QAM_ampa_256 = simde_mm256_set1_epi16(QAM64_n1);
    QAM_ampb_256 = simde_mm256_set1_epi16(QAM64_n2);
    QAM_ampc_256 = simde_mm256_setzero_si256();
  }
  else if (mod_order == 8) {
    QAM_ampa_256 = simde_mm256_set1_epi16(QAM256_n1);
    QAM_ampb_256 = simde_mm256_set1_epi16(QAM256_n2);
    QAM_ampc_256 = simde_mm256_set1_epi16(QAM256_n3);
  }

  simde__m256i xmmp0, xmmp1, xmmp2, xmmp3, xmmp4;
  simde__m256i complex_shuffle256 = simde_mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);
  simde__m256i conj256 = simde_mm256_set_epi16(1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1);

  for (int aatx = 0; aatx < nrOfLayers; aatx++) {
    simde__m256i *rxComp_256 =     (simde__m256i*)     &rxComp[aatx * nb_rx_ant][symbol * buffer_length];
    simde__m256i *rxF_ch_maga_256 = (simde__m256i*)&ul_ch_maga[aatx * buffer_length];
    simde__m256i *rxF_ch_magb_256 = (simde__m256i*)&ul_ch_magb[aatx * buffer_length];
    simde__m256i *rxF_ch_magc_256 = (simde__m256i*)&ul_ch_magc[aatx * buffer_length];
    for (int aarx = 0; aarx < nb_rx_ant; aarx++) {
      simde__m256i *rxF_256 = (simde__m256i*) &rxFext[aarx * buffer_length];
      simde__m256i *chF_256 = (simde__m256i*) &chFext[(aatx * nb_rx_ant + aarx) * buffer_length];

      for (int i = 0; i < buffer_length >> 3; i++) 
      {
        xmmp0  = simde_mm256_madd_epi16(chF_256[i], rxF_256[i]);
        // xmmp0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
        xmmp1  = simde_mm256_shuffle_epi8(chF_256[i], complex_shuffle256);
        xmmp1  = simde_mm256_sign_epi16(xmmp1, conj256);
        xmmp1  = simde_mm256_madd_epi16(xmmp1, rxF_256[i]);
        // xmmp1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
        xmmp0  = simde_mm256_srai_epi32(xmmp0, output_shift);
        xmmp1  = simde_mm256_srai_epi32(xmmp1, output_shift);
        xmmp2  = simde_mm256_unpacklo_epi32(xmmp0, xmmp1);
        xmmp3  = simde_mm256_unpackhi_epi32(xmmp0, xmmp1);
        xmmp4  = simde_mm256_packs_epi32(xmmp2, xmmp3);

        xmmp0 = simde_mm256_madd_epi16(chF_256[i], chF_256[i]); // |h|^2
        xmmp0 = simde_mm256_srai_epi32(xmmp0, output_shift); 
        xmmp0 = simde_mm256_packs_epi32(xmmp0, xmmp0);
        xmmp1 = simde_mm256_unpacklo_epi16(xmmp0, xmmp0);

        xmmp2 = simde_mm256_mulhrs_epi16(xmmp1, QAM_ampa_256);
        xmmp3 = simde_mm256_mulhrs_epi16(xmmp1, QAM_ampb_256);
        xmmp1 = simde_mm256_mulhrs_epi16(xmmp1, QAM_ampc_256);

        // MRC
        rxComp_256[i] = simde_mm256_add_epi16(rxComp_256[i], xmmp4); 
        if (mod_order > 2)
          rxF_ch_maga_256[i] = simde_mm256_add_epi16(rxF_ch_maga_256[i], xmmp2); 
        if (mod_order > 4)
          rxF_ch_magb_256[i] = simde_mm256_add_epi16(rxF_ch_magb_256[i], xmmp3); 
        if (mod_order > 6)
          rxF_ch_magc_256[i] = simde_mm256_add_epi16(rxF_ch_magc_256[i], xmmp1); 
      }
      if (rho != NULL) {
        for (int atx = 0; atx < nrOfLayers; atx++) {
          simde__m256i *rho_256  = (simde__m256i *   )&rho[(aatx * nrOfLayers + atx) * buffer_length];
          simde__m256i *chF_256  = (simde__m256i *)&chFext[(aatx * nb_rx_ant + aarx) * buffer_length];
          simde__m256i *chF2_256 = (simde__m256i *)&chFext[ (atx * nb_rx_ant + aarx) * buffer_length];
          for (int i = 0; i < buffer_length >> 3; i++) {
            // multiply by conjugated channel
            xmmp0 = simde_mm256_madd_epi16(chF_256[i], chF2_256[i]);
            // xmmp0 contains real part of 4 consecutive outputs (32-bit)
            xmmp1 = simde_mm256_shuffle_epi8(chF_256[i], complex_shuffle256);
            xmmp1 = simde_mm256_sign_epi16(xmmp1, conj256);
            xmmp1 = simde_mm256_madd_epi16(xmmp1, chF2_256[i]);
            // xmmp0 contains imag part of 4 consecutive outputs (32-bit)
            xmmp0 = simde_mm256_srai_epi32(xmmp0, output_shift);
            xmmp1 = simde_mm256_srai_epi32(xmmp1, output_shift);
            xmmp2 = simde_mm256_unpacklo_epi32(xmmp0, xmmp1);
            xmmp3 = simde_mm256_unpackhi_epi32(xmmp0, xmmp1);

            rho_256[i] = simde_mm256_adds_epi16(rho_256[i], simde_mm256_packs_epi32(xmmp2, xmmp3));
          }
        }
      }
    }
  }

  simde_mm_empty();
  simde_m_empty();
}

// Zero Forcing Rx function: nr_det_HhH()
static void nr_ulsch_det_HhH (int32_t *after_mf_00,//a
                              int32_t *after_mf_01,//b
                              int32_t *after_mf_10,//c
                              int32_t *after_mf_11,//d
                              int32_t *det_fin,//1/ad-bc
                              unsigned short nb_rb,
                              unsigned char symbol,
                              int32_t shift)
{
  int16_t nr_conjug2[8]__attribute__((aligned(16))) = {1,-1,1,-1,1,-1,1,-1} ;
  unsigned short rb;
  simde__m128i *after_mf_00_128,*after_mf_01_128, *after_mf_10_128, *after_mf_11_128, ad_re_128, bc_re_128; //ad_im_128, bc_im_128;
  simde__m128i *det_fin_128, det_re_128; //det_im_128, tmp_det0, tmp_det1;

  after_mf_00_128 = (simde__m128i *)after_mf_00;
  after_mf_01_128 = (simde__m128i *)after_mf_01;
  after_mf_10_128 = (simde__m128i *)after_mf_10;
  after_mf_11_128 = (simde__m128i *)after_mf_11;

  det_fin_128 = (simde__m128i *)det_fin;

  for (rb=0; rb<3*nb_rb; rb++) {

    //complex multiplication (I_a+jQ_a)(I_d+jQ_d) = (I_aI_d - Q_aQ_d) + j(Q_aI_d + I_aQ_d)
    //The imag part is often zero, we compute only the real part
    ad_re_128 = simde_mm_sign_epi16(after_mf_00_128[0],*(simde__m128i*)&nr_conjug2[0]);
    ad_re_128 = simde_mm_madd_epi16(ad_re_128,after_mf_11_128[0]); //Re: I_a0*I_d0 - Q_a1*Q_d1
    //ad_im_128 = simde_mm_shufflelo_epi16(after_mf_00_128[0], SIMDE_MM_SHUFFLE(2,3,0,1));//permutes IQs for the low 64 bits as [I_a0 Q_a1 I_a2 Q_a3]_64bits to [Q_a1 I_a0 Q_a3 I_a2]_64bits
    //ad_im_128 = simde_mm_shufflehi_epi16(ad_im_128, SIMDE_MM_SHUFFLE(2,3,0,1));//permutes IQs for the high 64 bits as [I_a0 Q_a1 I_a2 Q_a3]_64bits to [Q_a1 I_a0 Q_a3 I_a2]_64bits
    //ad_im_128 = simde_mm_madd_epi16(ad_im_128,after_mf_11_128[0]);//Im: (Q_aI_d + I_aQ_d)

    //complex multiplication (I_b+jQ_b)(I_c+jQ_c) = (I_bI_c - Q_bQ_c) + j(Q_bI_c + I_bQ_c)
    //The imag part is often zero, we compute only the real part
    bc_re_128 = simde_mm_sign_epi16(after_mf_01_128[0],*(simde__m128i*)&nr_conjug2[0]);
    bc_re_128 = simde_mm_madd_epi16(bc_re_128,after_mf_10_128[0]); //Re: I_b0*I_c0 - Q_b1*Q_c1
    //bc_im_128 = simde_mm_shufflelo_epi16(after_mf_01_128[0], SIMDE_MM_SHUFFLE(2,3,0,1));//permutes IQs for the low 64 bits as [I_b0 Q_b1 I_b2 Q_b3]_64bits to [Q_b1 I_b0 Q_b3 I_b2]_64bits
    //bc_im_128 = simde_mm_shufflehi_epi16(bc_im_128, SIMDE_MM_SHUFFLE(2,3,0,1));//permutes IQs for the high 64 bits as [I_b0 Q_b1 I_b2 Q_b3]_64bits to [Q_b1 I_b0 Q_b3 I_b2]_64bits
    //bc_im_128 = simde_mm_madd_epi16(bc_im_128,after_mf_10_128[0]);//Im: (Q_bI_c + I_bQ_c)

    det_re_128 = simde_mm_sub_epi32(ad_re_128, bc_re_128);
    //det_im_128 = simde_mm_sub_epi32(ad_im_128, bc_im_128);

    //det in Q30 format
    det_fin_128[0] = simde_mm_abs_epi32(det_re_128);


#ifdef DEBUG_DLSCH_DEMOD
     printf("\n Computing det_HhH_inv \n");
     //print_ints("det_re_128:",(int32_t*)&det_re_128);
     //print_ints("det_im_128:",(int32_t*)&det_im_128);
     print_ints("det_fin_128:",(int32_t*)&det_fin_128[0]);
#endif
    det_fin_128+=1;
    after_mf_00_128+=1;
    after_mf_01_128+=1;
    after_mf_10_128+=1;
    after_mf_11_128+=1;
  }
  simde_mm_empty();
  simde_m_empty();
}

/* Zero Forcing Rx function: nr_conjch0_mult_ch1()
 *
 *
 * */
static void nr_ulsch_conjch0_mult_ch1(int *ch0,
                                      int *ch1,
                                      int32_t *ch0conj_ch1,
                                      unsigned short nb_rb,
                                      unsigned char output_shift0)
{
  //This function is used to compute multiplications in H_hermitian * H matrix
  short nr_conjugate[8]__attribute__((aligned(16))) = {-1,1,-1,1,-1,1,-1,1};
  unsigned short rb;
  simde__m128i *dl_ch0_128,*dl_ch1_128, *ch0conj_ch1_128, mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3;

  dl_ch0_128 = (simde__m128i *)ch0;
  dl_ch1_128 = (simde__m128i *)ch1;

  ch0conj_ch1_128 = (simde__m128i *)ch0conj_ch1;

  for (rb=0; rb<3*nb_rb; rb++) {

    mmtmpD0 = simde_mm_madd_epi16(dl_ch0_128[0],dl_ch1_128[0]);
    mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch0_128[0], SIMDE_MM_SHUFFLE(2,3,0,1));
    mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
    mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i*)&nr_conjugate[0]);
    mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,dl_ch1_128[0]);
    mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift0);
    mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift0);
    mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
    mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

    ch0conj_ch1_128[0] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);

    /*printf("\n Computing conjugates \n");
    print_shorts("ch0:",(int16_t*)&dl_ch0_128[0]);
    print_shorts("ch1:",(int16_t*)&dl_ch1_128[0]);
    print_shorts("pack:",(int16_t*)&ch0conj_ch1_128[0]);*/

    dl_ch0_128+=1;
    dl_ch1_128+=1;
    ch0conj_ch1_128+=1;
  }
  simde_mm_empty();
  simde_m_empty();
}

static simde__m128i nr_ulsch_comp_muli_sum(simde__m128i input_x,
                                           simde__m128i input_y,
                                           simde__m128i input_w,
                                           simde__m128i input_z,
                                           simde__m128i det)
{
  int16_t nr_conjug2[8]__attribute__((aligned(16))) = {1,-1,1,-1,1,-1,1,-1} ;

  simde__m128i xy_re_128, xy_im_128, wz_re_128, wz_im_128;
  simde__m128i output, tmp_z0, tmp_z1;

  // complex multiplication (x_re + jx_im)*(y_re + jy_im) = (x_re*y_re - x_im*y_im) + j(x_im*y_re + x_re*y_im)
  // the real part
  xy_re_128 = simde_mm_sign_epi16(input_x,*(simde__m128i*)&nr_conjug2[0]);
  xy_re_128 = simde_mm_madd_epi16(xy_re_128,input_y); //Re: (x_re*y_re - x_im*y_im)

  // the imag part
  xy_im_128 = simde_mm_shufflelo_epi16(input_x, SIMDE_MM_SHUFFLE(2,3,0,1));//permutes IQs for the low 64 bits as [I_a0 Q_a1 I_a2 Q_a3]_64bits to [Q_a1 I_a0 Q_a3 I_a2]_64bits
  xy_im_128 = simde_mm_shufflehi_epi16(xy_im_128, SIMDE_MM_SHUFFLE(2,3,0,1));//permutes IQs for the high 64 bits as [I_a0 Q_a1 I_a2 Q_a3]_64bits to [Q_a1 I_a0 Q_a3 I_a2]_64bits
  xy_im_128 = simde_mm_madd_epi16(xy_im_128,input_y);//Im: (x_im*y_re + x_re*y_im)

  // complex multiplication (w_re + jw_im)*(z_re + jz_im) = (w_re*z_re - w_im*z_im) + j(w_im*z_re + w_re*z_im)
  // the real part
  wz_re_128 = simde_mm_sign_epi16(input_w,*(simde__m128i*)&nr_conjug2[0]);
  wz_re_128 = simde_mm_madd_epi16(wz_re_128,input_z); //Re: (w_re*z_re - w_im*z_im)

  // the imag part
  wz_im_128 = simde_mm_shufflelo_epi16(input_w, SIMDE_MM_SHUFFLE(2,3,0,1));//permutes IQs for the low 64 bits as [I_a0 Q_a1 I_a2 Q_a3]_64bits to [Q_a1 I_a0 Q_a3 I_a2]_64bits
  wz_im_128 = simde_mm_shufflehi_epi16(wz_im_128, SIMDE_MM_SHUFFLE(2,3,0,1));//permutes IQs for the high 64 bits as [I_a0 Q_a1 I_a2 Q_a3]_64bits to [Q_a1 I_a0 Q_a3 I_a2]_64bits
  wz_im_128 = simde_mm_madd_epi16(wz_im_128,input_z);//Im: (w_im*z_re + w_re*z_im)


  xy_re_128 = simde_mm_sub_epi32(xy_re_128, wz_re_128);
  xy_im_128 = simde_mm_sub_epi32(xy_im_128, wz_im_128);
  //print_ints("rx_re:",(int32_t*)&xy_re_128[0]);
  //print_ints("rx_Img:",(int32_t*)&xy_im_128[0]);
  //divide by matrix det and convert back to Q15 before packing
  int sum_det =0;
  for (int k=0; k<4;k++) {
    sum_det += ((((int *)&det)[k])>>2);
    //printf("det_%d = %d log2 =%d \n",k,(((int *)&det[0])[k]),log2_approx(((int *)&det[0])[k]));
    }

  int b = log2_approx(sum_det) - 8;
  if (b > 0) {
    xy_re_128 = simde_mm_srai_epi32(xy_re_128, b);
    xy_im_128 = simde_mm_srai_epi32(xy_im_128, b);
  } else {
    xy_re_128 = simde_mm_slli_epi32(xy_re_128, -b);
    xy_im_128 = simde_mm_slli_epi32(xy_im_128, -b);
  }

  tmp_z0  = simde_mm_unpacklo_epi32(xy_re_128,xy_im_128);
  //print_ints("unpack lo:",&tmp_z0[0]);
  tmp_z1  = simde_mm_unpackhi_epi32(xy_re_128,xy_im_128);
  //print_ints("unpack hi:",&tmp_z1[0]);
  output = simde_mm_packs_epi32(tmp_z0,tmp_z1);

  simde_mm_empty();
  simde_m_empty();
  return(output);
}

/* Zero Forcing Rx function: nr_construct_HhH_elements()
 *
 *
 * */
static void nr_ulsch_construct_HhH_elements(int *conjch00_ch00,
                                            int *conjch01_ch01,
                                            int *conjch11_ch11,
                                            int *conjch10_ch10,//
                                            int *conjch20_ch20,
                                            int *conjch21_ch21,
                                            int *conjch30_ch30,
                                            int *conjch31_ch31,
                                            int *conjch00_ch01,//00_01
                                            int *conjch01_ch00,//01_00
                                            int *conjch10_ch11,//10_11
                                            int *conjch11_ch10,//11_10
                                            int *conjch20_ch21,
                                            int *conjch21_ch20,
                                            int *conjch30_ch31,
                                            int *conjch31_ch30,
                                            int32_t *after_mf_00,
                                            int32_t *after_mf_01,
                                            int32_t *after_mf_10,
                                            int32_t *after_mf_11,
                                            unsigned short nb_rb,
                                            unsigned char symbol)
{
  //This function is used to construct the (H_hermitian * H matrix) matrix elements
  unsigned short rb;
  simde__m128i *conjch00_ch00_128, *conjch01_ch01_128, *conjch11_ch11_128, *conjch10_ch10_128;
  simde__m128i *conjch20_ch20_128, *conjch21_ch21_128, *conjch30_ch30_128, *conjch31_ch31_128;
  simde__m128i *conjch00_ch01_128, *conjch01_ch00_128, *conjch10_ch11_128, *conjch11_ch10_128;
  simde__m128i *conjch20_ch21_128, *conjch21_ch20_128, *conjch30_ch31_128, *conjch31_ch30_128;
  simde__m128i *after_mf_00_128, *after_mf_01_128, *after_mf_10_128, *after_mf_11_128;

  conjch00_ch00_128 = (simde__m128i *)conjch00_ch00;
  conjch01_ch01_128 = (simde__m128i *)conjch01_ch01;
  conjch11_ch11_128 = (simde__m128i *)conjch11_ch11;
  conjch10_ch10_128 = (simde__m128i *)conjch10_ch10;

  conjch20_ch20_128 = (simde__m128i *)conjch20_ch20;
  conjch21_ch21_128 = (simde__m128i *)conjch21_ch21;
  conjch30_ch30_128 = (simde__m128i *)conjch30_ch30;
  conjch31_ch31_128 = (simde__m128i *)conjch31_ch31;

  conjch00_ch01_128 = (simde__m128i *)conjch00_ch01;
  conjch01_ch00_128 = (simde__m128i *)conjch01_ch00;
  conjch10_ch11_128 = (simde__m128i *)conjch10_ch11;
  conjch11_ch10_128 = (simde__m128i *)conjch11_ch10;

  conjch20_ch21_128 = (simde__m128i *)conjch20_ch21;
  conjch21_ch20_128 = (simde__m128i *)conjch21_ch20;
  conjch30_ch31_128 = (simde__m128i *)conjch30_ch31;
  conjch31_ch30_128 = (simde__m128i *)conjch31_ch30;

  after_mf_00_128 = (simde__m128i *)after_mf_00;
  after_mf_01_128 = (simde__m128i *)after_mf_01;
  after_mf_10_128 = (simde__m128i *)after_mf_10;
  after_mf_11_128 = (simde__m128i *)after_mf_11;

  for (rb=0; rb<3*nb_rb; rb++) {

    after_mf_00_128[0] =simde_mm_adds_epi16(conjch00_ch00_128[0],conjch10_ch10_128[0]);//00_00 + 10_10
    if (conjch20_ch20 != NULL) after_mf_00_128[0] =simde_mm_adds_epi16(after_mf_00_128[0],conjch20_ch20_128[0]);
    if (conjch30_ch30 != NULL) after_mf_00_128[0] =simde_mm_adds_epi16(after_mf_00_128[0],conjch30_ch30_128[0]);

    after_mf_11_128[0] =simde_mm_adds_epi16(conjch01_ch01_128[0], conjch11_ch11_128[0]); //01_01 + 11_11
    if (conjch21_ch21 != NULL) after_mf_11_128[0] =simde_mm_adds_epi16(after_mf_11_128[0],conjch21_ch21_128[0]);
    if (conjch31_ch31 != NULL) after_mf_11_128[0] =simde_mm_adds_epi16(after_mf_11_128[0],conjch31_ch31_128[0]);

    after_mf_01_128[0] =simde_mm_adds_epi16(conjch00_ch01_128[0], conjch10_ch11_128[0]);//00_01 + 10_11
    if (conjch20_ch21 != NULL) after_mf_01_128[0] =simde_mm_adds_epi16(after_mf_01_128[0],conjch20_ch21_128[0]);
    if (conjch30_ch31 != NULL) after_mf_01_128[0] =simde_mm_adds_epi16(after_mf_01_128[0],conjch30_ch31_128[0]);

    after_mf_10_128[0] =simde_mm_adds_epi16(conjch01_ch00_128[0], conjch11_ch10_128[0]);//01_00 + 11_10
    if (conjch21_ch20 != NULL) after_mf_10_128[0] =simde_mm_adds_epi16(after_mf_10_128[0],conjch21_ch20_128[0]);
    if (conjch31_ch30 != NULL) after_mf_10_128[0] =simde_mm_adds_epi16(after_mf_10_128[0],conjch31_ch30_128[0]);

#ifdef DEBUG_DLSCH_DEMOD
    if ((rb<=30))
    {
      printf(" \n construct_HhH_elements \n");
      print_shorts("after_mf_00_128:",(int16_t*)&after_mf_00_128[0]);
      print_shorts("after_mf_01_128:",(int16_t*)&after_mf_01_128[0]);
      print_shorts("after_mf_10_128:",(int16_t*)&after_mf_10_128[0]);
      print_shorts("after_mf_11_128:",(int16_t*)&after_mf_11_128[0]);
    }
#endif
    conjch00_ch00_128+=1;
    conjch10_ch10_128+=1;
    conjch01_ch01_128+=1;
    conjch11_ch11_128+=1;

    if (conjch20_ch20 != NULL) conjch20_ch20_128+=1;
    if (conjch21_ch21 != NULL) conjch21_ch21_128+=1;
    if (conjch30_ch30 != NULL) conjch30_ch30_128+=1;
    if (conjch31_ch31 != NULL) conjch31_ch31_128+=1;

    conjch00_ch01_128+=1;
    conjch01_ch00_128+=1;
    conjch10_ch11_128+=1;
    conjch11_ch10_128+=1;

    if (conjch20_ch21 != NULL) conjch20_ch21_128+=1;
    if (conjch21_ch20 != NULL) conjch21_ch20_128+=1;
    if (conjch30_ch31 != NULL) conjch30_ch31_128+=1;
    if (conjch31_ch30 != NULL) conjch31_ch30_128+=1;

    after_mf_00_128 += 1;
    after_mf_01_128 += 1;
    after_mf_10_128 += 1;
    after_mf_11_128 += 1;
  }
  simde_mm_empty();
  simde_m_empty();
}

// MMSE Rx function: nr_ulsch_mmse_2layers()
static uint8_t nr_ulsch_mmse_2layers(NR_DL_FRAME_PARMS *frame_parms,
                                     int **rxdataF_comp,
                                     int **ul_ch_mag,
                                     int **ul_ch_magb,
                                     int **ul_ch_magc,
                                     int **ul_ch_estimates_ext,
                                     unsigned short nb_rb,
                                     unsigned char n_rx,
                                     unsigned char mod_order,
                                     int shift,
                                     unsigned char symbol,
                                     int length,
                                     uint32_t noise_var,
                                     uint32_t buffer_length)
{
  int *ch00, *ch01, *ch10, *ch11;
  int *ch20, *ch30, *ch21, *ch31;
  uint32_t nb_rb_0 = length/12 + ((length%12)?1:0);

  /* we need at least alignment to 16 bytes, let's put 32 to be sure
   * (maybe not necessary but doesn't hurt)
   */
  int32_t conjch00_ch01[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch01_ch00[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch10_ch11[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch11_ch10[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch00_ch00[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch01_ch01[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch10_ch10[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch11_ch11[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch20_ch20[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch21_ch21[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch30_ch30[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch31_ch31[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch20_ch21[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch30_ch31[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch21_ch20[12*nb_rb] __attribute__((aligned(32)));
  int32_t conjch31_ch30[12*nb_rb] __attribute__((aligned(32)));

  int32_t af_mf_00[12*nb_rb] __attribute__((aligned(32)));
  int32_t af_mf_01[12*nb_rb] __attribute__((aligned(32)));
  int32_t af_mf_10[12*nb_rb] __attribute__((aligned(32)));
  int32_t af_mf_11[12*nb_rb] __attribute__((aligned(32)));
  int32_t determ_fin[12*nb_rb] __attribute__((aligned(32)));

  switch (n_rx) {
    case 2://
      ch00 = &((int *)ul_ch_estimates_ext)[0 * buffer_length];
      ch01 = &((int *)ul_ch_estimates_ext)[2 * buffer_length];
      ch10 = &((int *)ul_ch_estimates_ext)[1 * buffer_length];
      ch11 = &((int *)ul_ch_estimates_ext)[3 * buffer_length];
      ch20 = NULL;
      ch21 = NULL;
      ch30 = NULL;
      ch31 = NULL;
      break;

    case 4://
      ch00 = &((int *)ul_ch_estimates_ext)[0 * buffer_length];
      ch01 = &((int *)ul_ch_estimates_ext)[4 * buffer_length];
      ch10 = &((int *)ul_ch_estimates_ext)[1 * buffer_length];
      ch11 = &((int *)ul_ch_estimates_ext)[5 * buffer_length];
      ch20 = &((int *)ul_ch_estimates_ext)[2 * buffer_length];
      ch21 = &((int *)ul_ch_estimates_ext)[6 * buffer_length];
      ch30 = &((int *)ul_ch_estimates_ext)[3 * buffer_length];
      ch31 = &((int *)ul_ch_estimates_ext)[7 * buffer_length];
      break;

    default:
      return -1;
      break;
  }

  /* 1- Compute the rx channel matrix after compensation: (1/2^log2_max)x(H_herm x H)
   * for n_rx = 2
   * |conj_H_00       conj_H_10|    | H_00         H_01|   |(conj_H_00xH_00+conj_H_10xH_10)   (conj_H_00xH_01+conj_H_10xH_11)|
   * |                         |  x |                  | = |                                                                 |
   * |conj_H_01       conj_H_11|    | H_10         H_11|   |(conj_H_01xH_00+conj_H_11xH_10)   (conj_H_01xH_01+conj_H_11xH_11)|
   *
   */

  if (n_rx>=2){
    // (1/2^log2_maxh)*conj_H_00xH_00: (1/(64*2))conjH_00*H_00*2^15
    nr_ulsch_conjch0_mult_ch1(ch00,
                        ch00,
                        conjch00_ch00,
                        nb_rb_0,
                        shift);
    // (1/2^log2_maxh)*conj_H_10xH_10: (1/(64*2))conjH_10*H_10*2^15
    nr_ulsch_conjch0_mult_ch1(ch10,
                        ch10,
                        conjch10_ch10,
                        nb_rb_0,
                        shift);
    // conj_H_00xH_01
    nr_ulsch_conjch0_mult_ch1(ch00,
                        ch01,
                        conjch00_ch01,
                        nb_rb_0,
                        shift); // this shift is equal to the channel level log2_maxh
    // conj_H_10xH_11
    nr_ulsch_conjch0_mult_ch1(ch10,
                        ch11,
                        conjch10_ch11,
                        nb_rb_0,
                        shift);
    // conj_H_01xH_01
    nr_ulsch_conjch0_mult_ch1(ch01,
                        ch01,
                        conjch01_ch01,
                        nb_rb_0,
                        shift);
    // conj_H_11xH_11
    nr_ulsch_conjch0_mult_ch1(ch11,
                        ch11,
                        conjch11_ch11,
                        nb_rb_0,
                        shift);
    // conj_H_01xH_00
    nr_ulsch_conjch0_mult_ch1(ch01,
                        ch00,
                        conjch01_ch00,
                        nb_rb_0,
                        shift);
    // conj_H_11xH_10
    nr_ulsch_conjch0_mult_ch1(ch11,
                        ch10,
                        conjch11_ch10,
                        nb_rb_0,
                        shift);
  }
  if (n_rx==4){
    // (1/2^log2_maxh)*conj_H_20xH_20: (1/(64*2*16))conjH_20*H_20*2^15
    nr_ulsch_conjch0_mult_ch1(ch20,
                        ch20,
                        conjch20_ch20,
                        nb_rb_0,
                        shift);

    // (1/2^log2_maxh)*conj_H_30xH_30: (1/(64*2*4))conjH_30*H_30*2^15
    nr_ulsch_conjch0_mult_ch1(ch30,
                        ch30,
                        conjch30_ch30,
                        nb_rb_0,
                        shift);

    // (1/2^log2_maxh)*conj_H_20xH_20: (1/(64*2))conjH_20*H_20*2^15
    nr_ulsch_conjch0_mult_ch1(ch20,
                        ch21,
                        conjch20_ch21,
                        nb_rb_0,
                        shift);

    nr_ulsch_conjch0_mult_ch1(ch30,
                        ch31,
                        conjch30_ch31,
                        nb_rb_0,
                        shift);

    nr_ulsch_conjch0_mult_ch1(ch21,
                        ch21,
                        conjch21_ch21,
                        nb_rb_0,
                        shift);

    nr_ulsch_conjch0_mult_ch1(ch31,
                        ch31,
                        conjch31_ch31,
                        nb_rb_0,
                        shift);

    // (1/2^log2_maxh)*conj_H_20xH_20: (1/(64*2))conjH_20*H_20*2^15
    nr_ulsch_conjch0_mult_ch1(ch21,
                        ch20,
                        conjch21_ch20,
                        nb_rb_0,
                        shift);

    nr_ulsch_conjch0_mult_ch1(ch31,
                        ch30,
                        conjch31_ch30,
                        nb_rb_0,
                        shift);

    nr_ulsch_construct_HhH_elements(conjch00_ch00,
                              conjch01_ch01,
                              conjch11_ch11,
                              conjch10_ch10,//
                              conjch20_ch20,
                              conjch21_ch21,
                              conjch30_ch30,
                              conjch31_ch31,
                              conjch00_ch01,
                              conjch01_ch00,
                              conjch10_ch11,
                              conjch11_ch10,//
                              conjch20_ch21,
                              conjch21_ch20,
                              conjch30_ch31,
                              conjch31_ch30,
                              af_mf_00,
                              af_mf_01,
                              af_mf_10,
                              af_mf_11,
                              nb_rb_0,
                              symbol);
  }
  if (n_rx==2){
    nr_ulsch_construct_HhH_elements(conjch00_ch00,
                              conjch01_ch01,
                              conjch11_ch11,
                              conjch10_ch10,//
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              conjch00_ch01,
                              conjch01_ch00,
                              conjch10_ch11,
                              conjch11_ch10,//
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              af_mf_00,
                              af_mf_01,
                              af_mf_10,
                              af_mf_11,
                              nb_rb_0,
                              symbol);
  }

  // Add noise_var such that: H^h * H + noise_var * I
  if (noise_var != 0) {
    simde__m128i nvar_128i = simde_mm_set1_epi32(noise_var);
    simde__m128i *af_mf_00_128i = (simde__m128i *)af_mf_00;
    simde__m128i *af_mf_11_128i = (simde__m128i *)af_mf_11;
    for (int k = 0; k < 3 * nb_rb_0; k++) {
      af_mf_00_128i[0] = simde_mm_add_epi32(af_mf_00_128i[0], nvar_128i);
      af_mf_11_128i[0] = simde_mm_add_epi32(af_mf_11_128i[0], nvar_128i);
      af_mf_00_128i++;
      af_mf_11_128i++;
    }
  }

  //det_HhH = ad -bc
  nr_ulsch_det_HhH(af_mf_00,//a
             af_mf_01,//b
             af_mf_10,//c
             af_mf_11,//d
             determ_fin,
             nb_rb_0,
             symbol,
             shift);
  /* 2- Compute the channel matrix inversion **********************************
   *
     *    |(conj_H_00xH_00+conj_H_10xH_10)   (conj_H_00xH_01+conj_H_10xH_11)|
     * A= |                                                                 |
     *    |(conj_H_01xH_00+conj_H_11xH_10)   (conj_H_01xH_01+conj_H_11xH_11)|
     *
     *
     *
     *inv(A) =(1/det)*[d  -b
     *                 -c  a]
     *
     *
     **************************************************************************/
  simde__m128i *ul_ch_mag128_0 = NULL, *ul_ch_mag128b_0 = NULL, *ul_ch_mag128c_0 = NULL; // Layer 0
  simde__m128i *ul_ch_mag128_1 = NULL, *ul_ch_mag128b_1 = NULL, *ul_ch_mag128c_1 = NULL; // Layer 1
  simde__m128i mmtmpD0, mmtmpD1, mmtmpD2, mmtmpD3;
  simde__m128i QAM_amp128 = {0}, QAM_amp128b = {0}, QAM_amp128c = {0};

  simde__m128i *determ_fin_128 = (simde__m128i *)&determ_fin[0];

  simde__m128i *after_mf_a_128 = (simde__m128i *)af_mf_00;
  simde__m128i *after_mf_b_128 = (simde__m128i *)af_mf_01;
  simde__m128i *after_mf_c_128 = (simde__m128i *)af_mf_10;
  simde__m128i *after_mf_d_128 = (simde__m128i *)af_mf_11;
  
  simde__m128i *rxdataF_comp128_0 = (simde__m128i *)&rxdataF_comp[0][symbol * buffer_length];
  simde__m128i *rxdataF_comp128_1 = (simde__m128i *)&rxdataF_comp[n_rx][symbol * buffer_length];

  if (mod_order > 2) {
    if (mod_order == 4) {
      QAM_amp128 = simde_mm_set1_epi16(QAM16_n1); // 2/sqrt(10)
      QAM_amp128b = simde_mm_setzero_si128();
      QAM_amp128c = simde_mm_setzero_si128();
    } else if (mod_order == 6) {
      QAM_amp128 = simde_mm_set1_epi16(QAM64_n1); // 4/sqrt{42}
      QAM_amp128b = simde_mm_set1_epi16(QAM64_n2); // 2/sqrt{42}
      QAM_amp128c = simde_mm_setzero_si128();
    } else if (mod_order == 8) {
      QAM_amp128 =  simde_mm_set1_epi16(QAM256_n1);
      QAM_amp128b = simde_mm_set1_epi16(QAM256_n2);
      QAM_amp128c = simde_mm_set1_epi16(QAM256_n3);
    }
    ul_ch_mag128_0  = (simde__m128i *) &ul_ch_mag[0];
    ul_ch_mag128b_0 = (simde__m128i *)&ul_ch_magb[0];
    ul_ch_mag128c_0 = (simde__m128i *)&ul_ch_magc[0];
    ul_ch_mag128_1  = (simde__m128i *) &((int *)ul_ch_mag)[1 * buffer_length];
    ul_ch_mag128b_1 = (simde__m128i *)&((int *)ul_ch_magb)[1 * buffer_length];
    ul_ch_mag128c_1 = (simde__m128i *)&((int *)ul_ch_magc)[1 * buffer_length];
  }

  for (int rb = 0; rb < 3 * nb_rb_0; rb++) {

    // Magnitude computation
    if (mod_order > 2) {

      int sum_det = 0;
      for (int k = 0; k < 4; k++) {
        sum_det += ((((int *)&determ_fin_128[0])[k]) >> 2);
      }

      int b = log2_approx(sum_det) - 8;
      if (b > 0) {
        mmtmpD2 = simde_mm_srai_epi32(determ_fin_128[0], b);
      } else {
        mmtmpD2 = simde_mm_slli_epi32(determ_fin_128[0], -b);
      }
      mmtmpD3 = simde_mm_unpacklo_epi32(mmtmpD2, mmtmpD2);
      mmtmpD2 = simde_mm_unpackhi_epi32(mmtmpD2, mmtmpD2);
      mmtmpD2 = simde_mm_packs_epi32(mmtmpD3, mmtmpD2);

      // Layer 0
      ul_ch_mag128_0[0] = mmtmpD2;
      ul_ch_mag128b_0[0] = mmtmpD2;
      ul_ch_mag128c_0[0] = mmtmpD2;
      ul_ch_mag128_0[0] = simde_mm_mulhi_epi16(ul_ch_mag128_0[0], QAM_amp128);
      ul_ch_mag128_0[0] = simde_mm_slli_epi16(ul_ch_mag128_0[0], 1);
      ul_ch_mag128b_0[0] = simde_mm_mulhi_epi16(ul_ch_mag128b_0[0], QAM_amp128b);
      ul_ch_mag128b_0[0] = simde_mm_slli_epi16(ul_ch_mag128b_0[0], 1);
      ul_ch_mag128c_0[0] = simde_mm_mulhi_epi16(ul_ch_mag128c_0[0], QAM_amp128c);
      ul_ch_mag128c_0[0] = simde_mm_slli_epi16(ul_ch_mag128c_0[0], 1);

      // Layer 1
      ul_ch_mag128_1[0] = mmtmpD2;
      ul_ch_mag128b_1[0] = mmtmpD2;
      ul_ch_mag128c_1[0] = mmtmpD2;
      ul_ch_mag128_1[0] = simde_mm_mulhi_epi16(ul_ch_mag128_1[0], QAM_amp128);
      ul_ch_mag128_1[0] = simde_mm_slli_epi16(ul_ch_mag128_1[0], 1);
      ul_ch_mag128b_1[0] = simde_mm_mulhi_epi16(ul_ch_mag128b_1[0], QAM_amp128b);
      ul_ch_mag128b_1[0] = simde_mm_slli_epi16(ul_ch_mag128b_1[0], 1);
      ul_ch_mag128c_1[0] = simde_mm_mulhi_epi16(ul_ch_mag128c_1[0], QAM_amp128c);
      ul_ch_mag128c_1[0] = simde_mm_slli_epi16(ul_ch_mag128c_1[0], 1);
    }

    // multiply by channel Inv
    //rxdataF_zf128_0 = rxdataF_comp128_0*d - b*rxdataF_comp128_1
    //rxdataF_zf128_1 = rxdataF_comp128_1*a - c*rxdataF_comp128_0
    //printf("layer_1 \n");
    mmtmpD0 = nr_ulsch_comp_muli_sum(rxdataF_comp128_0[0],
                               after_mf_d_128[0],
                               rxdataF_comp128_1[0],
                               after_mf_b_128[0],
                               determ_fin_128[0]);

    //printf("layer_2 \n");
    mmtmpD1 = nr_ulsch_comp_muli_sum(rxdataF_comp128_1[0],
                               after_mf_a_128[0],
                               rxdataF_comp128_0[0],
                               after_mf_c_128[0],
                               determ_fin_128[0]);

    rxdataF_comp128_0[0] = mmtmpD0;
    rxdataF_comp128_1[0] = mmtmpD1;

#ifdef DEBUG_DLSCH_DEMOD
    printf("\n Rx signal after ZF l%d rb%d\n",symbol,rb);
    print_shorts(" Rx layer 1:",(int16_t*)&rxdataF_comp128_0[0]);
    print_shorts(" Rx layer 2:",(int16_t*)&rxdataF_comp128_1[0]);
#endif
    determ_fin_128 += 1;
    ul_ch_mag128_0 += 1;
    ul_ch_mag128_1 += 1;
    ul_ch_mag128b_0 += 1;
    ul_ch_mag128b_1 += 1;
    ul_ch_mag128c_0 += 1;
    ul_ch_mag128c_1 += 1;
    rxdataF_comp128_0 += 1;
    rxdataF_comp128_1 += 1;
    after_mf_a_128 += 1;
    after_mf_b_128 += 1;
    after_mf_c_128 += 1;
    after_mf_d_128 += 1;
  }
  simde_mm_empty();
  simde_m_empty();
   return(0);
}

static void inner_rx(PHY_VARS_gNB *gNB,
                     int ulsch_id,
                     int slot,
                     NR_DL_FRAME_PARMS *frame_parms,
                     NR_gNB_PUSCH *pusch_vars,
                     nfapi_nr_pusch_pdu_t *rel15_ul,
                     c16_t **rxF,
                     c16_t **ul_ch,
                     int16_t **llr,
                     int soffset,
                     int length,
                     int symbol,
                     int output_shift,
                     uint32_t nvar)
{
  int nb_layer = rel15_ul->nrOfLayers;
  int nb_rx_ant = frame_parms->nb_antennas_rx;
  int dmrs_symbol_flag = (rel15_ul->ul_dmrs_symb_pos >> symbol) & 0x01;
  int buffer_length = (rel15_ul->rb_size * NR_NB_SC_PER_RB + 15) & ~15;
  c16_t rxFext[nb_rx_ant][buffer_length] __attribute__((aligned(32)));
  c16_t chFext[nb_layer][nb_rx_ant][buffer_length] __attribute__((aligned(32)));

  memset(rxFext, 0, sizeof(c16_t) * nb_rx_ant * buffer_length);
  memset(chFext, 0, sizeof(c16_t) * nb_layer * nb_rx_ant* buffer_length);

  for (int aarx = 0; aarx < nb_rx_ant; aarx++) {
    for (int aatx = 0; aatx < nb_layer; aatx++) {
      nr_ulsch_extract_rbs(rxF[aarx],
                            (c16_t *)pusch_vars->ul_ch_estimates[aatx * nb_rx_ant + aarx],
                            rxFext[aarx],
                            chFext[aatx][aarx],
                            soffset+(symbol * frame_parms->ofdm_symbol_size),
                            pusch_vars->dmrs_symbol * frame_parms->ofdm_symbol_size,
                            aarx,
                            dmrs_symbol_flag, 
                            rel15_ul,
                            frame_parms);
    }
  }
  c16_t rho[nb_layer][nb_layer][buffer_length] __attribute__((aligned(32)));
  c16_t rxF_ch_maga  [nb_layer][buffer_length] __attribute__((aligned(32)));
  c16_t rxF_ch_magb  [nb_layer][buffer_length] __attribute__((aligned(32)));
  c16_t rxF_ch_magc  [nb_layer][buffer_length] __attribute__((aligned(32)));

  memset(rho, 0, sizeof(c16_t) * nb_layer * nb_layer* buffer_length);
  memset(rxF_ch_maga, 0, sizeof(c16_t) * nb_layer * buffer_length);
  memset(rxF_ch_magb, 0, sizeof(c16_t) * nb_layer * buffer_length);
  memset(rxF_ch_magc, 0, sizeof(c16_t) * nb_layer * buffer_length);
  for (int i = 0; i < nb_layer; i++)
    memset(&pusch_vars->rxdataF_comp[i*nb_rx_ant][symbol * buffer_length], 0, sizeof(int32_t) * buffer_length);

  nr_ulsch_channel_compensation((c16_t*)rxFext,
                                (c16_t*)chFext,
                                (c16_t*)rxF_ch_maga,
                                (c16_t*)rxF_ch_magb,
                                (c16_t*)rxF_ch_magc,
                                pusch_vars->rxdataF_comp,
                                (nb_layer == 1) ? NULL : (c16_t*)rho,
                                frame_parms,
                                rel15_ul,
                                symbol,
                                buffer_length,
                                output_shift);

  if (nb_layer == 1 && rel15_ul->transform_precoding == transformPrecoder_enabled && rel15_ul->qam_mod_order <= 6) {
    if (rel15_ul->qam_mod_order > 2)
      nr_freq_equalization(frame_parms,
                          &pusch_vars->rxdataF_comp[0][symbol * buffer_length],
                          (int *)rxF_ch_maga,
                          (int *)rxF_ch_magb,
                          symbol,
                          pusch_vars->ul_valid_re_per_slot[symbol],
                          rel15_ul->qam_mod_order);
    nr_idft(&pusch_vars->rxdataF_comp[0][symbol * buffer_length], pusch_vars->ul_valid_re_per_slot[symbol]);
  }
  if (rel15_ul->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) {
    nr_pusch_ptrs_processing(gNB,
                             frame_parms,
                             rel15_ul,
                             ulsch_id,
                             slot,
                             symbol,
                             buffer_length);
    pusch_vars->ul_valid_re_per_slot[symbol] -= pusch_vars->ptrs_re_per_slot;
  }

  if (nb_layer == 2) {
    if (rel15_ul->qam_mod_order < 6) {
      nr_ulsch_compute_ML_llr(pusch_vars,
                              symbol,
                              (c16_t*)&pusch_vars->rxdataF_comp[0][symbol * buffer_length],
                              (c16_t*)&pusch_vars->rxdataF_comp[nb_rx_ant][symbol * buffer_length],
                              rxF_ch_maga[0],
                              rxF_ch_maga[1],
                              (c16_t*)&llr[0][pusch_vars->llr_offset[symbol]],
                              (c16_t*)&llr[1][pusch_vars->llr_offset[symbol]],
                              rho[0][1],
                              rho[1][0],
                              pusch_vars->ul_valid_re_per_slot[symbol],
                              rel15_ul->qam_mod_order);
    }
    else {
      nr_ulsch_mmse_2layers(frame_parms,
                            (int32_t **)pusch_vars->rxdataF_comp,
                            (int **)rxF_ch_maga, 
                            (int **)rxF_ch_magb, 
                            (int **)rxF_ch_magc, 
                            (int **)chFext, 
                            rel15_ul->rb_size,
                            frame_parms->nb_antennas_rx,
                            rel15_ul->qam_mod_order,
                            pusch_vars->log2_maxh,
                            symbol,
                            pusch_vars->ul_valid_re_per_slot[symbol],
                            nvar,
                            buffer_length);
    }
  }
  if (nb_layer != 2 || rel15_ul->qam_mod_order >= 6)
    for (int aatx = 0; aatx < nb_layer; aatx++) 
      nr_ulsch_compute_llr((int32_t*)&pusch_vars->rxdataF_comp[aatx * nb_rx_ant][symbol * buffer_length],
                          (int32_t*)rxF_ch_maga[aatx],
                          (int32_t*)rxF_ch_magb[aatx],
                          (int32_t*)rxF_ch_magc[aatx],
                          &llr[aatx][pusch_vars->llr_offset[symbol]],
                          pusch_vars->ul_valid_re_per_slot[symbol],
                          symbol,
                          rel15_ul->qam_mod_order);
}

static void nr_pusch_symbol_processing(void *arg)
{
  puschSymbolProc_t *rdata=(puschSymbolProc_t*)arg;

  PHY_VARS_gNB *gNB = rdata->gNB;
  NR_DL_FRAME_PARMS *frame_parms = rdata->frame_parms;
  nfapi_nr_pusch_pdu_t *rel15_ul = rdata->rel15_ul;
  int ulsch_id = rdata->ulsch_id;
  int slot = rdata->slot;
  NR_gNB_PUSCH *pusch_vars = &gNB->pusch_vars[ulsch_id];
  for (int symbol = rdata->startSymbol; symbol < rdata->startSymbol+rdata->numSymbols; symbol++) {
    int dmrs_symbol_flag = (rel15_ul->ul_dmrs_symb_pos >> symbol) & 0x01;
    if (dmrs_symbol_flag == 1) 
    {
      if ((rel15_ul->ul_dmrs_symb_pos >> ((symbol + 1) % frame_parms->symbols_per_slot)) & 0x01)
        AssertFatal(1==0,"Double DMRS configuration is not yet supported\n");
      gNB->pusch_vars[ulsch_id].dmrs_symbol = symbol;
    }

    if (gNB->pusch_vars[ulsch_id].ul_valid_re_per_slot[symbol] == 0) 
      continue;
    int soffset = (slot % RU_RX_SLOT_DEPTH) * frame_parms->symbols_per_slot * frame_parms->ofdm_symbol_size;
    inner_rx(gNB,
             ulsch_id,
             slot,
             frame_parms,
             pusch_vars, 
             rel15_ul,
             gNB->common_vars.rxdataF, 
             (c16_t**)gNB->pusch_vars[ulsch_id].ul_ch_estimates, 
             rdata->llr_layers,
             soffset,
             gNB->pusch_vars[ulsch_id].ul_valid_re_per_slot[symbol],
             symbol, 
             gNB->pusch_vars[ulsch_id].log2_maxh,
             rdata->nvar);

    int nb_re_pusch = gNB->pusch_vars[ulsch_id].ul_valid_re_per_slot[symbol];
    // layer de-mapping
    int16_t* llr_ptr = &rdata->llr_layers[0][pusch_vars->llr_offset[symbol]];
    if (rel15_ul->nrOfLayers != 1) {
      llr_ptr = &rdata->llr[pusch_vars->llr_offset[symbol] * rel15_ul->nrOfLayers];
      for (int i = 0; i < (nb_re_pusch); i++) 
        for (int l = 0; l < rel15_ul->nrOfLayers; l++) 
          for (int m = 0; m < rel15_ul->qam_mod_order; m++) 
            llr_ptr[i*rel15_ul->nrOfLayers*rel15_ul->qam_mod_order+l*rel15_ul->qam_mod_order+m] = rdata->llr_layers[l][pusch_vars->llr_offset[symbol] + i*rel15_ul->qam_mod_order+m];
    }
    // unscrambling
    int16_t *llr16 = (int16_t*)&rdata->llr[pusch_vars->llr_offset[symbol] * rel15_ul->nrOfLayers];
    for (int i = 0; i < (nb_re_pusch * rel15_ul->qam_mod_order * rel15_ul->nrOfLayers); i++) 
      llr16[i] = llr_ptr[i] * rdata->s[i];
  }
}


int nr_rx_pusch_tp(PHY_VARS_gNB *gNB,
                   uint8_t ulsch_id,
                   uint32_t frame,
                   uint8_t slot,
                   unsigned char harq_pid)
{
  uint8_t aarx;
  uint32_t bwp_start_subcarrier;

  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  nfapi_nr_pusch_pdu_t *rel15_ul = &gNB->ulsch[ulsch_id].harq_process->ulsch_pdu;

  NR_gNB_PUSCH *pusch_vars = &gNB->pusch_vars[ulsch_id];
  pusch_vars->dmrs_symbol = INVALID_VALUE;
  gNB->nbSymb = 0;
  bwp_start_subcarrier = ((rel15_ul->rb_start + rel15_ul->bwp_start)*NR_NB_SC_PER_RB + frame_parms->first_carrier_offset) % frame_parms->ofdm_symbol_size;
  LOG_D(PHY,"pusch %d.%d : bwp_start_subcarrier %d, rb_start %d, first_carrier_offset %d\n", frame,slot,bwp_start_subcarrier, rel15_ul->rb_start, frame_parms->first_carrier_offset);
  LOG_D(PHY,"pusch %d.%d : ul_dmrs_symb_pos %x\n",frame,slot,rel15_ul->ul_dmrs_symb_pos);

  //----------------------------------------------------------
  //------------------- Channel estimation -------------------
  //----------------------------------------------------------
  start_meas(&gNB->ulsch_channel_estimation_stats);
  int max_ch = 0;
  uint32_t nvar = 0;
  for(uint8_t symbol = rel15_ul->start_symbol_index; symbol < (rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols); symbol++) {
    uint8_t dmrs_symbol_flag = (rel15_ul->ul_dmrs_symb_pos >> symbol) & 0x01;
    LOG_D(PHY, "symbol %d, dmrs_symbol_flag :%d\n", symbol, dmrs_symbol_flag);
    
    if (dmrs_symbol_flag == 1) {
      if (pusch_vars->dmrs_symbol == INVALID_VALUE)
        pusch_vars->dmrs_symbol = symbol;

      for (int nl=0; nl<rel15_ul->nrOfLayers; nl++) {
        uint32_t nvar_tmp = 0;
        nr_pusch_channel_estimation(gNB,
                                    slot,
                                    get_dmrs_port(nl,rel15_ul->dmrs_ports),
                                    symbol,
                                    ulsch_id,
                                    bwp_start_subcarrier,
                                    rel15_ul,
                                    &max_ch,
                                    &nvar_tmp);
        nvar += nvar_tmp;
      }
      // measure the SNR from the channel estimation
      nr_gnb_measurements(gNB, 
                          &gNB->ulsch[ulsch_id], 
                          pusch_vars, 
                          symbol, 
                          rel15_ul->nrOfLayers);
      allocCast2D(n0_subband_power,
                  unsigned int,
                  gNB->measurements.n0_subband_power,
                  frame_parms->nb_antennas_rx,
                  frame_parms->N_RB_UL,
                  false);
      for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) 
      {
        if (symbol == rel15_ul->start_symbol_index) 
        {
          pusch_vars->ulsch_power[aarx] = 0;
          pusch_vars->ulsch_noise_power[aarx] = 0;
        }
        for (int aatx = 0; aatx < rel15_ul->nrOfLayers; aatx++) {
          pusch_vars->ulsch_power[aarx] += signal_energy_nodc(
              &pusch_vars->ul_ch_estimates[aatx * gNB->frame_parms.nb_antennas_rx + aarx][symbol * frame_parms->ofdm_symbol_size],
              rel15_ul->rb_size * 12);
        }
        for (int rb = 0; rb < rel15_ul->rb_size; rb++)
          pusch_vars->ulsch_noise_power[aarx] += 
            n0_subband_power[aarx][rel15_ul->bwp_start + rel15_ul->rb_start + rb] / rel15_ul->rb_size;
      }
    }
  }

  nvar /= (rel15_ul->nr_of_symbols * rel15_ul->nrOfLayers * frame_parms->nb_antennas_rx);

  // averaging time domain channel estimates
  if (gNB->chest_time == 1) 
  {
    nr_chest_time_domain_avg(frame_parms,
                             pusch_vars->ul_ch_estimates,
                             rel15_ul->nr_of_symbols,
                             rel15_ul->start_symbol_index,
                             rel15_ul->ul_dmrs_symb_pos,
                             rel15_ul->rb_size);
    pusch_vars->dmrs_symbol = get_next_dmrs_symbol_in_slot(rel15_ul->ul_dmrs_symb_pos, 
                                                           rel15_ul->start_symbol_index, 
                                                           rel15_ul->nr_of_symbols);
  }

  stop_meas(&gNB->ulsch_channel_estimation_stats);

  start_meas(&gNB->rx_pusch_init_stats);

  // Scrambling initialization
  int number_dmrs_symbols = 0;
  for (int l = rel15_ul->start_symbol_index; l < rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols; l++)
    number_dmrs_symbols += ((rel15_ul->ul_dmrs_symb_pos)>>l) & 0x01;
  int nb_re_dmrs;
  if (rel15_ul->dmrs_config_type == pusch_dmrs_type1)
    nb_re_dmrs = 6*rel15_ul->num_dmrs_cdm_grps_no_data;
  else
    nb_re_dmrs = 4*rel15_ul->num_dmrs_cdm_grps_no_data;

  uint32_t unav_res = 0;
  if (rel15_ul->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) {
    uint16_t ptrsSymbPos = 0;
    set_ptrs_symb_idx(&ptrsSymbPos,
                      rel15_ul->nr_of_symbols,
                      rel15_ul->start_symbol_index,
                      1 << rel15_ul->pusch_ptrs.ptrs_time_density,
                      rel15_ul->ul_dmrs_symb_pos);
    int ptrsSymbPerSlot = get_ptrs_symbols_in_slot(ptrsSymbPos, rel15_ul->start_symbol_index, rel15_ul->nr_of_symbols);
    int n_ptrs = (rel15_ul->rb_size + rel15_ul->pusch_ptrs.ptrs_freq_density - 1) / rel15_ul->pusch_ptrs.ptrs_freq_density;
    unav_res = n_ptrs * ptrsSymbPerSlot;
  }

  // get how many bit in a slot //
  int G = nr_get_G(rel15_ul->rb_size,
                   rel15_ul->nr_of_symbols,
                   nb_re_dmrs,
                   number_dmrs_symbols, // number of dmrs symbols irrespective of single or double symbol dmrs
                   unav_res,
                   rel15_ul->qam_mod_order,
                   rel15_ul->nrOfLayers);
  gNB->ulsch[ulsch_id].unav_res = unav_res;

  // initialize scrambling sequence //
  int16_t s[G+96] __attribute__((aligned(32)));

  nr_codeword_unscrambling_init(s, G, 0, rel15_ul->data_scrambling_id, rel15_ul->rnti); 

  // first the computation of channel levels

  int nb_re_pusch = 0, meas_symbol = -1;
  for(meas_symbol = rel15_ul->start_symbol_index; 
      meas_symbol < (rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols); 
      meas_symbol++) 
    if ((nb_re_pusch = get_nb_re_pusch(frame_parms,rel15_ul,meas_symbol)) > 0)
      break;

  AssertFatal(nb_re_pusch>0 && meas_symbol>=0,"nb_re_pusch %d cannot be 0 or meas_symbol %d cannot be negative here\n",nb_re_pusch,meas_symbol);

  // extract the first dmrs for the channel level computation
  // extract the data in the OFDM frame, to the start of the array
  int soffset = (slot % RU_RX_SLOT_DEPTH) * frame_parms->symbols_per_slot * frame_parms->ofdm_symbol_size;

  nb_re_pusch = (nb_re_pusch + 15) & ~15;

  for (int aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) 
    for (int aatx = 0; aatx < rel15_ul->nrOfLayers; aatx++) 
      nr_ulsch_extract_rbs(gNB->common_vars.rxdataF[aarx],
                            (c16_t *)pusch_vars->ul_ch_estimates[aatx * frame_parms->nb_antennas_rx + aarx],
                            (c16_t*)&pusch_vars->rxdataF_ext[aarx][meas_symbol * nb_re_pusch],
                            (c16_t*)&pusch_vars->ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][meas_symbol * nb_re_pusch],
                            soffset + meas_symbol * frame_parms->ofdm_symbol_size,
                            pusch_vars->dmrs_symbol * frame_parms->ofdm_symbol_size,
                            aarx,
                            (rel15_ul->ul_dmrs_symb_pos >> meas_symbol) & 0x01, 
                            rel15_ul,
                            frame_parms);

  int avgs = 0;
  int avg[frame_parms->nb_antennas_rx*rel15_ul->nrOfLayers];
  uint8_t shift_ch_ext = rel15_ul->nrOfLayers > 1 ? log2_approx(max_ch >> 11) : 0;

  //----------------------------------------------------------
  //--------------------- Channel Scaling --------------------
  //----------------------------------------------------------
  nr_ulsch_scale_channel(pusch_vars->ul_ch_estimates_ext,
                         frame_parms,
                         meas_symbol,
                         (rel15_ul->ul_dmrs_symb_pos >> meas_symbol) & 0x01,
                         nb_re_pusch,
                         rel15_ul->nrOfLayers,
                         rel15_ul->rb_size,
                         shift_ch_ext);
  
  nr_ulsch_channel_level(pusch_vars->ul_ch_estimates_ext,
                         frame_parms,
                         avg,
                         meas_symbol, // index of the start symbol
                         nb_re_pusch, // number of the re in pusch
                         rel15_ul->nrOfLayers);

  for (int aatx = 0; aatx < rel15_ul->nrOfLayers; aatx++)
    for (int aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++)
       avgs = cmax(avgs, avg[aatx*frame_parms->nb_antennas_rx+aarx]);
  
  pusch_vars->log2_maxh = (log2_approx(avgs) >> 1);

  if (rel15_ul->nrOfLayers == 2 && rel15_ul->qam_mod_order >= 6)
    pusch_vars->log2_maxh = (log2_approx(avgs) >> 1) - 3; // for MMSE
  else if (rel15_ul->nrOfLayers == 1)
    pusch_vars->log2_maxh = (log2_approx(avgs) >> 1) + 1 + log2_approx(frame_parms->nb_antennas_rx >> 2);
  
  if (pusch_vars->log2_maxh < 0)
    pusch_vars->log2_maxh = 0;

  stop_meas(&gNB->rx_pusch_init_stats);

  start_meas(&gNB->rx_pusch_symbol_processing_stats);
  int numSymbols = gNB->num_pusch_symbols_per_thread;

  for(uint8_t symbol = rel15_ul->start_symbol_index; 
      symbol < (rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols); 
      symbol += numSymbols) 
  {
    int total_res = 0;
    for (int s = 0; s < numSymbols;s++) { 
      pusch_vars->ul_valid_re_per_slot[symbol+s] = get_nb_re_pusch(frame_parms,rel15_ul,symbol+s);
      pusch_vars->llr_offset[symbol+s] = ((symbol+s) == rel15_ul->start_symbol_index) ? 
                                         0 : 
                                         pusch_vars->llr_offset[symbol+s-1] + pusch_vars->ul_valid_re_per_slot[symbol+s-1] * rel15_ul->qam_mod_order;
      total_res+=pusch_vars->ul_valid_re_per_slot[symbol+s];
    }
    if (total_res > 0) {
      union puschSymbolReqUnion id = {.s={ulsch_id,frame,slot,0}};
      id.p=1+symbol;
      notifiedFIFO_elt_t *req = newNotifiedFIFO_elt(sizeof(puschSymbolProc_t), id.p, &gNB->respPuschSymb, &nr_pusch_symbol_processing); // create a job for Tpool
      puschSymbolProc_t *rdata = (puschSymbolProc_t*)NotifiedFifoData(req); // data for the job

      rdata->gNB = gNB;
      rdata->frame_parms = frame_parms;
      rdata->rel15_ul = rel15_ul;
      rdata->slot = slot;
      rdata->startSymbol = symbol;
      rdata->numSymbols = numSymbols;
      rdata->ulsch_id = ulsch_id;
      rdata->llr = pusch_vars->llr;
      rdata->llr_layers = pusch_vars->llr_layers;
      rdata->s   = &s[pusch_vars->llr_offset[symbol]*rel15_ul->nrOfLayers];
      rdata->nvar = nvar;

      if (rel15_ul->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) {
        nr_pusch_symbol_processing(rdata);
      } else {
        pushTpool(&gNB->threadPool, req);
        gNB->nbSymb++;
      }

      LOG_D(PHY,"%d.%d Added symbol %d (count %d) to process, in pipe\n",frame,slot,symbol,gNB->nbSymb);
    }
  } // symbol loop

  while (gNB->nbSymb > 0) {
    notifiedFIFO_elt_t *req = pullTpool(&gNB->respPuschSymb, &gNB->threadPool);
    gNB->nbSymb--;
    delNotifiedFIFO_elt(req);
  }

  stop_meas(&gNB->rx_pusch_symbol_processing_stats);
  return 0;
}
