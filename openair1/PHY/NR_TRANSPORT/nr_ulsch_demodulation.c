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

//#define DEBUG_CH_COMP
//#define DEBUG_RB_EXT
//#define DEBUG_CH_MAG
//#define ML_DEBUG

#define INVALID_VALUE 255

#ifdef __aarch64__
#define USE_128BIT
#endif

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

void nr_ulsch_extract_rbs0 (c16_t *rxdataF,
                            int32_t *chF,
                            int32_t *rxFext,
                            int32_t *chFext,
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


  int32_t *rxF        = (int32_t*)&rxdataF[rxoffset];
  int32_t *rxF_ext    = &rxFext[0];
  int32_t *ul_ch0     = &chF[choffset]; 
  int32_t *ul_ch0_ext = &chFext[0];


  if (is_dmrs_symbol == 0) {
    if (start_re + nb_re_pusch <= frame_parms->ofdm_symbol_size) {
      memcpy((void*)rxF_ext,
             (void*)&rxF[start_re],
             nb_re_pusch*sizeof(int32_t));
    } 
    else 
    {
      int neg_length = frame_parms->ofdm_symbol_size-start_re;
      int pos_length = nb_re_pusch - neg_length;
      memcpy((void*)rxF_ext, (void*)&rxF[start_re], neg_length * sizeof(int32_t));
      memcpy((void*)&rxF_ext[neg_length], (void*)rxF, pos_length * sizeof(int32_t));
    }
   memcpy((void*)ul_ch0_ext,(void*)ul_ch0,nb_re_pusch*sizeof(int32_t));
  }
  else if (pusch_pdu->dmrs_config_type == pusch_dmrs_type1) // 6 REs / PRB
  {
    AssertFatal(delta==0||delta==1,"Illegal delta %d\n",delta);
    int32_t *rxF32        = &rxF[start_re];
    int32_t *rxF_ext32    = rxF_ext;
    int32_t *ul_ch032     = ul_ch0;
    int32_t *ul_ch0_ext32 = ul_ch0_ext;
    int idx,idx2,idx3;
    if (start_re + nb_re_pusch < frame_parms->ofdm_symbol_size) {
      for (idx=1-delta,idx2=0;idx<nb_re_pusch;idx+=2,idx2++) {
        rxF_ext32[idx2] = rxF32[idx];
        ul_ch0_ext32[idx2]= ul_ch032[idx];
      }
    }
    else { // handle the two pieces around DC
      int neg_length = frame_parms->ofdm_symbol_size-start_re;
      int pos_length = nb_re_pusch-neg_length;
        
      for (idx=1-delta,idx2=0;idx<neg_length;idx+=2,idx2++) {
        rxF_ext32[idx2] = rxF32[idx];
        ul_ch0_ext32[idx2]= ul_ch032[idx];
      }
      rxF32=(int32_t*)rxF;
      idx3=idx;
      for (idx=1-delta;idx<pos_length;idx+=2,idx2++,idx3++) {
        rxF_ext32[idx2] = rxF32[idx];
        ul_ch0_ext32[idx2]= ul_ch032[idx3];
      }
    }
  }
  else if (pusch_pdu->dmrs_config_type == pusch_dmrs_type2)  // 8 REs / PRB
  {
    AssertFatal(delta==0||delta==2||delta==4,"Illegal delta %d\n",delta);
    if (start_re + nb_re_pusch < frame_parms->ofdm_symbol_size) {
      int64_t *rxF64        = (int64_t*)&rxF[start_re];
      int64_t *rxF_ext64    = (int64_t*)rxF_ext;
      int64_t *ul_ch064     = (int64_t*)ul_ch0;
      int64_t *ul_ch0_ext64 = (int64_t*)ul_ch0_ext;
      if (delta==0) {
        for (int idx=0;idx<nb_re_pusch>>1;idx+=6) {
           rxF_ext64[idx]=rxF64[idx+1];
           rxF_ext64[idx+1]=rxF64[idx+2];
           rxF_ext64[idx+2]=rxF64[idx+4];
           rxF_ext64[idx+3]=rxF64[idx+5];
           ul_ch0_ext64[idx]=ul_ch064[idx+1];
           ul_ch0_ext64[idx+1]=ul_ch064[idx+2];
           ul_ch0_ext64[idx+2]=ul_ch064[idx+4];
           ul_ch0_ext64[idx+3]=ul_ch064[idx+5];
        }
      }
      else if (delta==2) {
        for (int idx=0;idx<nb_re_pusch>>1;idx+=6) {
           rxF_ext64[idx]=rxF64[idx+0];
           rxF_ext64[idx+1]=rxF64[idx+2];
           rxF_ext64[idx+2]=rxF64[idx+3];
           rxF_ext64[idx+3]=rxF64[idx+5];
           ul_ch0_ext64[idx]=ul_ch064[idx+0];
           ul_ch0_ext64[idx+1]=ul_ch064[idx+2];
           ul_ch0_ext64[idx+2]=ul_ch064[idx+3];
           ul_ch0_ext64[idx+3]=ul_ch064[idx+5];
        }
      }
      else if (delta==4) {
        for (int idx=0;idx<nb_re_pusch>>1;idx+=6) {
           rxF_ext64[idx]=rxF64[idx+0];
           rxF_ext64[idx+1]=rxF64[idx+1];
           rxF_ext64[idx+2]=rxF64[idx+3];
           rxF_ext64[idx+3]=rxF64[idx+4];
           ul_ch0_ext64[idx]=ul_ch064[idx+0];
           ul_ch0_ext64[idx+1]=ul_ch064[idx+1];
           ul_ch0_ext64[idx+2]=ul_ch064[idx+3];
           ul_ch0_ext64[idx+3]=ul_ch064[idx+4];
        }
      }
    }
    else {
      int neg_length = frame_parms->ofdm_symbol_size-start_re;
      int pos_length = nb_re_pusch-neg_length;
      if ((pos_length%12) > 0 ) pos_length+=12;
      int64_t *rxF64        = (int64_t*)&rxF[start_re];
      int64_t *rxF_ext64    = (int64_t*)rxF_ext;
      int64_t *ul_ch064     = (int64_t*)ul_ch0;
      int64_t *ul_ch0_ext64 = (int64_t*)ul_ch0_ext;
      int idx=0;
      if (delta==0) {
        for (idx=0;idx<neg_length>>1;idx+=6) {
           rxF_ext64[idx]  =rxF64[idx+1];
           rxF_ext64[idx+1]=rxF64[idx+2];
           rxF_ext64[idx+2]=rxF64[idx+4];
           rxF_ext64[idx+3]=rxF64[idx+5];
           ul_ch0_ext64[idx]=ul_ch064[idx+1];
           ul_ch0_ext64[idx+1]=ul_ch064[idx+2];
           ul_ch0_ext64[idx+2]=ul_ch064[idx+4];
           ul_ch0_ext64[idx+3]=ul_ch064[idx+5];
        }
        if ((neg_length%12) > 0) {
          rxF_ext64[idx+4]=rxF64[idx+7];
          rxF_ext64[idx+5]=rxF64[idx+8];
          ul_ch0_ext64[idx+4]=ul_ch064[idx+7];
          ul_ch0_ext64[idx+5]=ul_ch064[idx+8];
        }
        rxF_ext64+=(neg_length/3);
        rxF64=(int64_t*)rxF;
        ul_ch0_ext64+=(neg_length/3);
        ul_ch064+=(neg_length>>1);
        for (idx=0;idx<pos_length>>1;idx+=6) {
           rxF_ext64[idx]  =rxF64[idx+1];
           rxF_ext64[idx+1]=rxF64[idx+2];
           rxF_ext64[idx+2]=rxF64[idx+4];
           rxF_ext64[idx+3]=rxF64[idx+5];
           ul_ch0_ext64[idx]=ul_ch064[idx+1];
           ul_ch0_ext64[idx+1]=ul_ch064[idx+2];
           ul_ch0_ext64[idx+2]=ul_ch064[idx+4];
           ul_ch0_ext64[idx+3]=ul_ch064[idx+5];
        }
      }
      else if (delta==2) {
        for (idx=0;idx<neg_length>>1;idx+=6) {
           rxF_ext64[idx]  =rxF64[idx+0];
           rxF_ext64[idx+1]=rxF64[idx+2];
           rxF_ext64[idx+2]=rxF64[idx+3];
           rxF_ext64[idx+3]=rxF64[idx+5];
           ul_ch0_ext64[idx]=ul_ch064[idx+0];
           ul_ch0_ext64[idx+1]=ul_ch064[idx+2];
           ul_ch0_ext64[idx+2]=ul_ch064[idx+3];
           ul_ch0_ext64[idx+3]=ul_ch064[idx+5];
        }
        if ((neg_length%12) > 0) {
          rxF_ext64[idx+4]=rxF64[idx+6];
          rxF_ext64[idx+5]=rxF64[idx+8];
          ul_ch0_ext64[idx+4]=ul_ch064[idx+6];
          ul_ch0_ext64[idx+5]=ul_ch064[idx+8];
        }
        rxF_ext64+=(neg_length/3);
        rxF64=(int64_t*)rxF;
        ul_ch0_ext64+=(neg_length/3);
        ul_ch064+=(neg_length>>1);
        for (idx=0;idx<pos_length>>1;idx+=6) {
           rxF_ext64[idx]  =rxF64[idx+0];
           rxF_ext64[idx+1]=rxF64[idx+2];
           rxF_ext64[idx+2]=rxF64[idx+3];
           rxF_ext64[idx+3]=rxF64[idx+5];
           ul_ch0_ext64[idx]=ul_ch064[idx+0];
           ul_ch0_ext64[idx+1]=ul_ch064[idx+2];
           ul_ch0_ext64[idx+2]=ul_ch064[idx+3];
           ul_ch0_ext64[idx+3]=ul_ch064[idx+5];
        }
      }
      else if (delta==4) {
        for (idx=0;idx<neg_length>>1;idx+=6) {
           rxF_ext64[idx]  =rxF64[idx+0];
           rxF_ext64[idx+1]=rxF64[idx+1];
           rxF_ext64[idx+2]=rxF64[idx+3];
           rxF_ext64[idx+3]=rxF64[idx+4];
           ul_ch0_ext64[idx]=ul_ch064[idx+0];
           ul_ch0_ext64[idx+1]=ul_ch064[idx+1];
           ul_ch0_ext64[idx+2]=ul_ch064[idx+3];
           ul_ch0_ext64[idx+3]=ul_ch064[idx+4];
        }
        if ((neg_length%12) > 0) {
          rxF_ext64[idx+4]=rxF64[idx+6];
          rxF_ext64[idx+5]=rxF64[idx+7];
          ul_ch0_ext64[idx+4]=ul_ch064[idx+6];
          ul_ch0_ext64[idx+5]=ul_ch064[idx+7];
        }
        rxF_ext64+=(neg_length/3);
        rxF64=(int64_t*)rxF;
        ul_ch0_ext64+=(neg_length/3);
        ul_ch064+=(neg_length>>1);
        for (idx=0;idx<pos_length>>1;idx+=6) {
           rxF_ext64[idx]  =rxF64[idx+0];
           rxF_ext64[idx+1]=rxF64[idx+1];
           rxF_ext64[idx+2]=rxF64[idx+3];
           rxF_ext64[idx+3]=rxF64[idx+4];
           ul_ch0_ext64[idx]=ul_ch064[idx+0];
           ul_ch0_ext64[idx+1]=ul_ch064[idx+1];
           ul_ch0_ext64[idx+2]=ul_ch064[idx+3];
           ul_ch0_ext64[idx+3]=ul_ch064[idx+4];
        }
      }
    }
  }
}

void nr_ulsch_extract_rbs(c16_t **rxdataF,
                          NR_gNB_PUSCH *pusch_vars,
                          int slot,
                          unsigned char symbol,
                          uint8_t is_dmrs_symbol,
                          nfapi_nr_pusch_pdu_t *pusch_pdu,
                          NR_DL_FRAME_PARMS *frame_parms) {

  unsigned short start_re, re, nb_re_pusch;
  unsigned char aarx, aatx;
  uint32_t rxF_ext_index = 0;
  uint32_t ul_ch0_ext_index = 0;
  uint32_t ul_ch0_index = 0;
  int16_t *rxF,*rxF_ext;
  int *ul_ch0,*ul_ch0_ext;
  int soffset = (slot&3)*frame_parms->symbols_per_slot*frame_parms->ofdm_symbol_size;

#ifdef DEBUG_RB_EXT
  printf("--------------------symbol = %d-----------------------\n", symbol);
  printf("--------------------ch_ext_index = %d-----------------------\n", symbol*NR_NB_SC_PER_RB * pusch_pdu->rb_size);
#endif

  uint8_t is_data_re;
  start_re = (frame_parms->first_carrier_offset + (pusch_pdu->rb_start + pusch_pdu->bwp_start) * NR_NB_SC_PER_RB)%frame_parms->ofdm_symbol_size;
  nb_re_pusch = NR_NB_SC_PER_RB * pusch_pdu->rb_size;

  int nb_re_pusch2 = nb_re_pusch + (nb_re_pusch&7);

  for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {

    rxF = (int16_t *)&rxdataF[aarx][soffset+(symbol * frame_parms->ofdm_symbol_size)];
    rxF_ext = (int16_t *)&pusch_vars->rxdataF_ext[aarx][symbol * nb_re_pusch2]; // [hna] rxdataF_ext isn't contiguous in order to solve an alignment problem ib llr computation in case of mod_order = 4, 6
    
    if (is_dmrs_symbol == 0) {
      if (start_re + nb_re_pusch <= frame_parms->ofdm_symbol_size) {
        memcpy((void*)rxF_ext, (void*)&rxF[start_re*2], nb_re_pusch*sizeof(int32_t));
      } else {
        int neg_length = frame_parms->ofdm_symbol_size-start_re;
        int pos_length = nb_re_pusch-neg_length;
        memcpy((void*)rxF_ext,(void*)&rxF[start_re*2],neg_length*sizeof(int32_t));
        memcpy((void*)&rxF_ext[2*neg_length],(void*)rxF,pos_length*sizeof(int32_t));
      }

      for (aatx = 0; aatx < pusch_pdu->nrOfLayers; aatx++) {
        ul_ch0 = &pusch_vars->ul_ch_estimates[aatx*frame_parms->nb_antennas_rx+aarx][pusch_vars->dmrs_symbol*frame_parms->ofdm_symbol_size]; // update channel estimates if new dmrs symbol are available
        ul_ch0_ext = &pusch_vars->ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol*nb_re_pusch2];
        memcpy((void*)ul_ch0_ext,(void*)ul_ch0,nb_re_pusch*sizeof(int32_t));
      }

    } else {

      for (aatx = 0; aatx < pusch_pdu->nrOfLayers; aatx++) {
        ul_ch0 = &pusch_vars->ul_ch_estimates[aatx*frame_parms->nb_antennas_rx+aarx][pusch_vars->dmrs_symbol*frame_parms->ofdm_symbol_size]; // update channel estimates if new dmrs symbol are available
        ul_ch0_ext = &pusch_vars->ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol*nb_re_pusch2];

        rxF_ext_index = 0;
        ul_ch0_ext_index = 0;
        ul_ch0_index = 0;
        for (re = 0; re < nb_re_pusch; re++) {
          uint16_t k = start_re + re;
          is_data_re = allowed_xlsch_re_in_dmrs_symbol(k, start_re, frame_parms->ofdm_symbol_size, pusch_pdu->num_dmrs_cdm_grps_no_data, pusch_pdu->dmrs_config_type);
          if (++k >= frame_parms->ofdm_symbol_size) {
            k -= frame_parms->ofdm_symbol_size;
          }

          #ifdef DEBUG_RB_EXT
          printf("re = %d, is_dmrs_symbol = %d, symbol = %d\n", re, is_dmrs_symbol, symbol);
          #endif

          // save only data and respective channel estimates
          if (is_data_re == 1) {
            if (aatx == 0) {
              rxF_ext[rxF_ext_index]     = (rxF[ ((start_re + re)*2)      % (frame_parms->ofdm_symbol_size*2)]);
              rxF_ext[rxF_ext_index + 1] = (rxF[(((start_re + re)*2) + 1) % (frame_parms->ofdm_symbol_size*2)]);
              rxF_ext_index +=2;
            }

            ul_ch0_ext[ul_ch0_ext_index] = ul_ch0[ul_ch0_index];
            ul_ch0_ext_index++;

            #ifdef DEBUG_RB_EXT
            printf("dmrs symb %d: rxF_ext[%u] = (%d,%d), ul_ch0_ext[%u] = (%d,%d)\n",
                 is_dmrs_symbol,rxF_ext_index>>1, rxF_ext[rxF_ext_index],rxF_ext[rxF_ext_index+1],
                 ul_ch0_ext_index,  ((int16_t*)&ul_ch0_ext[ul_ch0_ext_index])[0],  ((int16_t*)&ul_ch0_ext[ul_ch0_ext_index])[1]);
            #endif          
          } 
          ul_ch0_index++;
        }
      }
    }
  }
}

void nr_ulsch_scale_channel(int **ul_ch_estimates_ext,
                            NR_DL_FRAME_PARMS *frame_parms,
                            NR_gNB_ULSCH_t *ulsch_gNB,
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

  uint32_t nb_rb_0 = len / 12 + ((len % 12) ? 1 : 0);
  int off = ((nb_rb & 1) == 1) ? 4 : 0;
  for (int aatx = 0; aatx < nrOfLayers; aatx++) {
    for (int aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      simde__m128i *ul_ch128 = (simde__m128i *)&ul_ch_estimates_ext[aatx * frame_parms->nb_antennas_rx + aarx][symbol * (off + (nb_rb * NR_NB_SC_PER_RB))];
      for (int rb = 0; rb < nb_rb_0; rb++) {
        ul_ch128[0] = simde_mm_mulhi_epi16(ul_ch128[0], ch_amp128);
        ul_ch128[0] = simde_mm_slli_epi16(ul_ch128[0], b);

        ul_ch128[1] = simde_mm_mulhi_epi16(ul_ch128[1], ch_amp128);
        ul_ch128[1] = simde_mm_slli_epi16(ul_ch128[1], b);

        ul_ch128[2] = simde_mm_mulhi_epi16(ul_ch128[2], ch_amp128);
        ul_ch128[2] = simde_mm_slli_epi16(ul_ch128[2], b);
        ul_ch128 += 3;
      }
    }
  }
}

int get_nb_re_pusch (NR_DL_FRAME_PARMS *frame_parms, nfapi_nr_pusch_pdu_t *rel15_ul,int symbol) 
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

void inner_rx_qpsk (int *rxF, 
                    int *ul_ch, 
                    int16_t *llr, 
                    int aarx, 
                    int length, 
                    int output_shift)
{
#ifndef USE_128BIT
  register simde__m256i xmmp0, xmmp1, xmmp2, xmmp3, xmmp4;
  register simde__m256i complex_shuffle256 = simde_mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);
  register simde__m256i conj256 = simde_mm256_set_epi16(1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1);

  simde__m256i *rxF256  = (simde__m256i*)rxF;
  simde__m256i *ulch256 = (simde__m256i*)ul_ch;
  // need to use simde__m64 because llr output is not necessarily aligned to 256 bits, but it is always to 64 bits
  simde__m64   *llr64 = (simde__m64 *)llr;   
  for (int i=0; i<((length>>3)+((length&7)>0?1:0)); i++) {
    xmmp0  = simde_mm256_madd_epi16(ulch256[i], rxF256[i]);
    // xmmp0 contains real part of 8 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmmp1  = simde_mm256_shuffle_epi8(ulch256[i], complex_shuffle256);
    xmmp1  = simde_mm256_sign_epi16(xmmp1, conj256);
    xmmp1  = simde_mm256_madd_epi16(xmmp1, rxF256[i]);
    // xmmp1 contains imag part of 8 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmmp0  = simde_mm256_srai_epi32(xmmp0, output_shift);
    xmmp1  = simde_mm256_srai_epi32(xmmp1, output_shift);
    xmmp2  = simde_mm256_unpacklo_epi32(xmmp0, xmmp1);
    xmmp3  = simde_mm256_unpackhi_epi32(xmmp0, xmmp1);
    xmmp4  = simde_mm256_packs_epi32(xmmp2,xmmp3);
    if (aarx == 0)
    {
      *llr64    = (simde__m64)simde_mm256_extract_epi64(xmmp4,0); llr64++;
      *llr64    = (simde__m64)simde_mm256_extract_epi64(xmmp4,1); llr64++;
      *llr64    = (simde__m64)simde_mm256_extract_epi64(xmmp4,2); llr64++;
      *llr64    = (simde__m64)simde_mm256_extract_epi64(xmmp4,3); llr64++;
    }
    else
    {
      *llr64   = simde_mm_adds_pi16(*llr64,(simde__m64)(simde_mm256_extract_epi64(xmmp4,0))); llr64++;
      *llr64   = simde_mm_adds_pi16(*llr64,(simde__m64)(simde_mm256_extract_epi64(xmmp4,1))); llr64++;
      *llr64   = simde_mm_adds_pi16(*llr64,(simde__m64)(simde_mm256_extract_epi64(xmmp4,2))); llr64++;
      *llr64   = simde_mm_adds_pi16(*llr64,(simde__m64)(simde_mm256_extract_epi64(xmmp4,3))); llr64++;
    }
  }
#else
  simde__m128i xmmp0, xmmp1, xmmp2, xmmp3, xmmp4;
  register simde__m128i complex_shuffle128 = simde_mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
  register simde__m128i conj128 = simde_mm_set_epi16(1, -1, 1, -1, 1, -1, 1, -1);

  simde__m128i *rxF128  = (simde__m128i*)rxF;
  simde__m128i *ulch128 = (simde__m128i*)ul_ch;
  simde__m128i *llr128 = (simde__m128i*)llr;
  for (int i = 0; i < (length >> 2); i++) {
    xmmp0  = simde_mm_madd_epi16(ulch128[i], rxF128[i]);
    // xmmp0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmmp1  = simde_mm_shuffle_epi8(ulch128[i], complex_shuffle128);
    xmmp1  = simde_mm_sign_epi16(xmmp1, conj128);
    xmmp1  = simde_mm_madd_epi16(xmmp1, rxF128[i]);
    // xmmp1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmmp0  = simde_mm_srai_epi32(xmmp0, output_shift);
    xmmp1  = simde_mm_srai_epi32(xmmp1, output_shift);
    xmmp2  = simde_mm_unpacklo_epi32(xmmp0, xmmp1);
    xmmp3  = simde_mm_unpackhi_epi32(xmmp0, xmmp1);
    xmmp4  = simde_mm_packs_epi32(xmmp2, xmmp3);

    if (aarx == 0)
      *llr128 = xmmp4;
    else
      *llr128 = simde_mm_add_epi16(*llr128, xmmp4); 
    llr128++;
  }
  if (length & 3) 
  {
    int i = (length>>1) - 1;
    simde__m64* llr64 = (simde__m64*)llr128;
    simde__m64 xmm0, xmm1, xmm2, xmm3, xmm4;
    simde__m64 complex_shuffle64 = simde_mm_set_pi8(5, 4, 7, 6, 1, 0, 3, 2);
    simde__m64 conj64 = simde_mm_set_pi16(1, -1, 1, -1);
    simde__m64 *rxF64     = (simde__m64*)rxF;
    simde__m64 *ulch64    = (simde__m64*)ul_ch;

    xmm0 = simde_mm_madd_pi16(ulch64[i], rxF64[i]);
    // xmm0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i] 
    xmm1  = simde_mm_shuffle_pi8(ulch64[i], complex_shuffle64);
    xmm1 = simde_mm_sign_pi16(xmm1, conj64);
    xmm1  = simde_mm_madd_pi16(xmm1, rxF64[i]);
    // xmm1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmm0  = simde_mm_srai_pi32(xmm0, output_shift);
    xmm1  = simde_mm_srai_pi32(xmm1, output_shift);
    xmm2  = simde_mm_unpacklo_pi32(xmm0, xmm1);
    xmm3  = simde_mm_unpackhi_pi32(xmm0, xmm1);
    xmm4  = simde_mm_packs_pi32(xmm2, xmm3);

    if (aarx == 0)
      *llr64 = xmm4; 
    else
      *llr64 = simde_mm_add_pi16(*llr64, xmm4); 
  }
#endif
}

void inner_rx_16qam (int *rxF, 
                     int *ul_ch, 
                     int16_t *llr, 
                     int aarx, 
                     int length, 
                     int output_shift) 
{
#ifndef USE_128BIT
  register simde__m256i xmmp0,xmmp1,xmmp2,xmmp3,xmmp4,xmmp5;
  register simde__m256i complex_shuffle256 = simde_mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);
  register simde__m256i conj256 = simde_mm256_set_epi16(1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1);
 
  register simde__m256i QAM_amp256  = simde_mm256_set1_epi16(QAM16_n1);  // 2/sqrt(10)
  simde__m256i *rxF256  = (simde__m256i*)rxF;
  simde__m256i *ulch256 = (simde__m256i*)ul_ch;
  // need to use simde__m64 because llr output is not necessarily aligned to 256 bits, but it is always to 64 bits
  simde__m64 *llr64 = (simde__m64 *)llr;   

  for (int i = 0; i < ((length >> 3) + ((length & 7) > 0 ? 1 : 0)); i++) 
  {
    xmmp0  = simde_mm256_madd_epi16(ulch256[i], rxF256[i]);
    // xmmp0 contains real part of 8 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmmp1  = simde_mm256_shuffle_epi8(ulch256[i], complex_shuffle256);
    xmmp1  = simde_mm256_sign_epi16(xmmp1, conj256);
    xmmp1  = simde_mm256_madd_epi16(xmmp1, rxF256[i]);
    // xmmp1 contains imag part of 8 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmmp0  = simde_mm256_srai_epi32(xmmp0, output_shift);
    xmmp1  = simde_mm256_srai_epi32(xmmp1, output_shift);
    xmmp2  = simde_mm256_unpacklo_epi32(xmmp0, xmmp1);
    xmmp3  = simde_mm256_unpackhi_epi32(xmmp0, xmmp1);
    xmmp4  = simde_mm256_packs_epi32(xmmp2, xmmp3);

    // compute channel amplitude for LLR
    xmmp0 = simde_mm256_madd_epi16(ulch256[i], ulch256[i]);
    xmmp0 = simde_mm256_srai_epi32(xmmp0, output_shift);
    xmmp0 = simde_mm256_packs_epi32(xmmp0, xmmp0);
    xmmp1 = simde_mm256_unpacklo_epi16(xmmp0, xmmp0);
    xmmp1 = simde_mm256_mulhrs_epi16(xmmp1, QAM_amp256);

    xmmp2 = simde_mm256_abs_epi16(xmmp4);           // registers of even index in xmm0-> |y_R|, registers of odd index in xmm0-> |y_I|
    xmmp2 = simde_mm256_subs_epi16(xmmp1,xmmp2); // registers of even index in xmm0-> |y_R|-|h|^2, registers of odd index in xmm0-> |y_I|-|h|^2

    xmmp3 = simde_mm256_unpacklo_epi32(xmmp4,xmmp2); // llr128[0] contains the llrs of the 1st,2nd,5th and 6th REs
    xmmp5 = simde_mm256_unpackhi_epi32(xmmp4,xmmp2); // llr128[1] contains the llrs of the 3rd, 4th, 7th and 8th REs
    if (aarx == 0)
    {
      // 1st/2nd RE
      llr64[0] = (simde__m64)simde_mm256_extract_epi64(xmmp3,0); // llr32[0] low 16 bits-> y_R        , high 16 bits-> y_I
      // 3rd/4th RE
      llr64[1] = (simde__m64)simde_mm256_extract_epi64(xmmp3,1); // llr32[2] low 16 bits-> y_R        , high 16 bits-> y_I
      // 5th/6th RE
      llr64[2] = (simde__m64)simde_mm256_extract_epi64(xmmp5,0); // llr32[4] low 16 bits-> y_R        , high 16 bits-> y_I
      // 7Rh/8th RE
      llr64[3] = (simde__m64)simde_mm256_extract_epi64(xmmp5,1); // llr32[6] low 16 bits-> y_R        , high 16 bits-> y_I
      // 9th/10th RE
      llr64[4] = (simde__m64)simde_mm256_extract_epi64(xmmp3,2); // llr32[8] low 16 bits-> y_R        , high 16 bits-> y_I
      // 11th/12th RE
      llr64[5] = (simde__m64)simde_mm256_extract_epi64(xmmp3,3); // llr32[10] low 16 bits-> y_R        , high 16 bits-> y_I
      // 13th/14th RE
      llr64[6] = (simde__m64)simde_mm256_extract_epi64(xmmp5,2); // llr32[12] low 16 bits-> y_R        , high 16 bits-> y_I
      // 15th/16th RE
      llr64[7] = (simde__m64)simde_mm256_extract_epi64(xmmp5,3); // llr32[14] low 16 bits-> y_R        , high 16 bits-> y_I
      llr64+=8;
    }
    else
    {
      llr64[0] = simde_mm_adds_pi16(llr64[0],(simde__m64)simde_mm256_extract_epi64(xmmp3,0)); 
      llr64[1] = simde_mm_adds_pi16(llr64[1],(simde__m64)simde_mm256_extract_epi64(xmmp3,1)); 
      llr64[2] = simde_mm_adds_pi16(llr64[2],(simde__m64)simde_mm256_extract_epi64(xmmp5,0)); 
      llr64[3] = simde_mm_adds_pi16(llr64[3],(simde__m64)simde_mm256_extract_epi64(xmmp5,1)); 
      llr64[4] = simde_mm_adds_pi16(llr64[4],(simde__m64)simde_mm256_extract_epi64(xmmp3,2)); 
      llr64[5] = simde_mm_adds_pi16(llr64[5],(simde__m64)simde_mm256_extract_epi64(xmmp3,3)); 
      llr64[6] = simde_mm_adds_pi16(llr64[6],(simde__m64)simde_mm256_extract_epi64(xmmp5,2)); 
      llr64[7] = simde_mm_adds_pi16(llr64[7],(simde__m64)simde_mm256_extract_epi64(xmmp5,3)); 
      llr64 += 8;
    }
  }
#else
  simde__m128i xmmp0, xmmp1, xmmp2, xmmp3, xmmp4, xmmp5;
  register simde__m128i complex_shuffle128 = simde_mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
  register simde__m128i conj128 = simde_mm_set_epi16(1, -1, 1, -1, 1, -1, 1, -1);
 
  register simde__m128i QAM_amp128  = simde_mm_set1_epi16(QAM16_n1);  // 2/sqrt(10)
  simde__m128i *rxF128  = (simde__m128i*)rxF;
  simde__m128i *ulch128 = (simde__m128i*)ul_ch;
  // need to use simde__m64 because llr output is not necessarily aligned to 256 bits, but it is always to 64 bits
  simde__m64 *llr64 = (simde__m64 *)llr;

  for (int i = 0; i < (length >> 2); i++) 
  {
    xmmp0  = simde_mm_madd_epi16(ulch128[i], rxF128[i]);
    // xmmp0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmmp1  = simde_mm_shuffle_epi8(ulch128[i], complex_shuffle128);
    xmmp1  = simde_mm_sign_epi16(xmmp1, conj128);
    xmmp1  = simde_mm_madd_epi16(xmmp1, rxF128[i]);
    // xmmp1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmmp0  = simde_mm_srai_epi32(xmmp0, output_shift);
    xmmp1  = simde_mm_srai_epi32(xmmp1, output_shift);
    xmmp2  = simde_mm_unpacklo_epi32(xmmp0, xmmp1);
    xmmp3  = simde_mm_unpackhi_epi32(xmmp0, xmmp1);
    xmmp4  = simde_mm_packs_epi32(xmmp2, xmmp3);

    // compute channel amplitude for LLR
    xmmp0 = simde_mm_madd_epi16(ulch128[i], ulch128[i]); // |h|^2
    xmmp0 = simde_mm_srai_epi32(xmmp0, output_shift); 
    xmmp0 = simde_mm_packs_epi32(xmmp0, xmmp0);
    xmmp1 = simde_mm_unpacklo_epi16(xmmp0, xmmp0);
    xmmp1 = simde_mm_mulhrs_epi16(xmmp1, QAM_amp128);

    xmmp2 = simde_mm_abs_epi16(xmmp4);                // registers of even index in xmm0-> |y_R|, registers of odd index in xmm0-> |y_I|
    xmmp2 = simde_mm_subs_epi16(xmmp1, xmmp2);     // registers of even index in xmm0-> |y_R|-|h|^2, registers of odd index in xmm0-> |y_I|-|h|^2

    xmmp3 = simde_mm_unpacklo_epi32(xmmp4, xmmp2); // llr128[0] contains the llrs of the 1st,2nd,5th and 6th REs
    xmmp5 = simde_mm_unpackhi_epi32(xmmp4, xmmp2); // llr128[1] contains the llrs of the 3rd, 4th, 7th and 8th REs
    if (aarx == 0)
    {
      llr64[0] = (simde__m64)simde_mm_extract_epi64(xmmp3, 0); // llr32[0] low 16 bits-> y_R, high 16 bits-> y_I
      llr64[1] = (simde__m64)simde_mm_extract_epi64(xmmp3, 1); // llr32[2] low 16 bits-> y_R, high 16 bits-> y_I
      llr64[2] = (simde__m64)simde_mm_extract_epi64(xmmp5, 0); // llr32[4] low 16 bits-> y_R, high 16 bits-> y_I
      llr64[3] = (simde__m64)simde_mm_extract_epi64(xmmp5, 1); // llr32[6] low 16 bits-> y_R, high 16 bits-> y_I
    }
    else
    {
      llr64[0] = simde_mm_adds_pi16(llr64[0], (simde__m64)simde_mm_extract_epi64(xmmp3, 0));
      llr64[1] = simde_mm_adds_pi16(llr64[1], (simde__m64)simde_mm_extract_epi64(xmmp3, 1));
      llr64[2] = simde_mm_adds_pi16(llr64[2], (simde__m64)simde_mm_extract_epi64(xmmp5, 0));
      llr64[3] = simde_mm_adds_pi16(llr64[3], (simde__m64)simde_mm_extract_epi64(xmmp5, 1));
    }
    llr64 += 4;
  }
  if (length & 3) 
  {
    int i = (length>>1) - 1;
    simde__m64 xmm0, xmm1, xmm2, xmm3, xmm4;
    simde__m64 complex_shuffle64 = simde_mm_set_pi8(5,4,7,6,1,0,3,2);
    simde__m64 conj64 = simde_mm_set_pi16(1, -1, 1, -1);
    simde__m64 *rxF64     = (simde__m64*)rxF;
    simde__m64 *ulch64    = (simde__m64*)ul_ch;
    simde__m64 QAM_amp    = simde_mm_set1_pi16(QAM16_n1);

    xmm0  = simde_mm_madd_pi16(ulch64[i], rxF64[i]);
    // xmm0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmm1  = simde_mm_shuffle_pi8(ulch64[i], complex_shuffle64);
    xmm1 = simde_mm_sign_pi16(xmm1, conj64);
    xmm1  = simde_mm_madd_pi16(xmm1, rxF64[i]);
    // xmm1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmm0  = simde_mm_srai_pi32(xmm0, output_shift);
    xmm1  = simde_mm_srai_pi32(xmm1, output_shift);
    xmm2  = simde_mm_unpacklo_pi32(xmm0, xmm1);
    xmm3  = simde_mm_unpackhi_pi32(xmm0, xmm1);
    xmm4  = simde_mm_packs_pi32(xmm2, xmm3);

    // compute channel amplitude for LLR
    xmm0 = simde_mm_madd_pi16(ulch64[i], ulch64[i]); // |h|^2
    xmm0 = simde_mm_srai_pi32(xmm0, output_shift);
    xmm0 = simde_mm_packs_pi32(xmm0, xmm0);
    xmm2 = simde_mm_unpacklo_pi16(xmm0, xmm0);
    
    xmm1 = simde_mm_mulhrs_pi16(xmm2, QAM_amp);
    
    xmm0 = simde_mm_abs_pi16(xmm4);        // registers of even index in xmm0-> |y_R|, registers of odd index in xmm0-> |y_I|
    xmm0 = simde_mm_subs_pi16(xmm1, xmm0); // registers of even index in xmm0-> |y_R|-|h|^2, registers of odd index in xmm0-> |y_I|-|h|^2
    if (aarx == 0)
    {
      llr64[0] = simde_mm_set_pi32(simde_mm_extract_pi16(xmm0, 0), simde_mm_extract_pi16(xmm4, 0));
      llr64[1] = simde_mm_set_pi32(simde_mm_extract_pi16(xmm0, 1), simde_mm_extract_pi16(xmm4, 1));
    }
    else
    {
      llr64[0] = simde_mm_adds_pi16(llr64[0], simde_mm_set_pi32(simde_mm_extract_pi16(xmm0, 0),simde_mm_extract_pi16(xmm4, 0)));
      llr64[1] = simde_mm_adds_pi16(llr64[1], simde_mm_set_pi32(simde_mm_extract_pi16(xmm4, 1),simde_mm_extract_pi16(xmm1, 0)));
    }
  }

#endif
}

void inner_rx_64qam (int *restrict rxF, 
                     int *restrict ul_ch, 
                     int16_t *restrict llr, 
                     int aarx, 
                     int length, 
                     int output_shift) 
{
#ifndef USE_128BIT
  register simde__m256i xmmp0, xmmp1, xmmp2, xmmp3, xmmp4, xmmp6, xmmp7;
  register simde__m256i complex_shuffle256 = simde_mm256_set_epi8(29, 28, 31, 30, 25, 24, 27, 26, 21, 20, 23, 22, 17, 16, 19, 18, 13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6,  1, 0, 3, 2);
  register simde__m256i conj256 = simde_mm256_set_epi16(1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1);

  register simde__m256i QAM_amp256  = simde_mm256_set1_epi16(QAM64_n1);  // 2/sqrt(10)
  register simde__m256i QAM_amp256b = simde_mm256_set1_epi16(QAM64_n2);
  simde__m256i *rxF256  = (simde__m256i*)rxF;
  simde__m256i *ulch256 = (simde__m256i*)ul_ch;
  // need to use simde__m64 because llr output is not necessarily aligned to 256 bits, but it is always to 64 bits

  simde__m64 *llr64 = (simde__m64 *)llr;   
  for (int i=0;i<((length>>3)+((length&7)>0?1:0));i++) {
    xmmp0  = simde_mm256_madd_epi16(ulch256[i],rxF256[i]);
    // xmmp0 contains real part of 8 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmmp1  = simde_mm256_shuffle_epi8(ulch256[i],complex_shuffle256);
    xmmp1  = simde_mm256_sign_epi16(xmmp1,conj256);
    xmmp1  = simde_mm256_madd_epi16(xmmp1,rxF256[i]);
    // xmmp1 contains imag part of 8 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmmp0  = simde_mm256_srai_epi32(xmmp0,output_shift);
    xmmp1  = simde_mm256_srai_epi32(xmmp1,output_shift);
    xmmp2  = simde_mm256_unpacklo_epi32(xmmp0,xmmp1);
    xmmp3  = simde_mm256_unpackhi_epi32(xmmp0,xmmp1);
    xmmp4  = simde_mm256_packs_epi32(xmmp2,xmmp3);

    // compute channel amplitude for LLR
    xmmp0 = simde_mm256_madd_epi16(ulch256[i],ulch256[i]);
    xmmp0 = simde_mm256_srai_epi32(xmmp0,output_shift);
    xmmp0 = simde_mm256_packs_epi32(xmmp0,xmmp0);
    xmmp2 = simde_mm256_unpacklo_epi16(xmmp0,xmmp0);
    xmmp1 = simde_mm256_mulhrs_epi16(xmmp2,QAM_amp256);
    xmmp6 = simde_mm256_mulhrs_epi16(xmmp2,QAM_amp256b);

    xmmp2 = simde_mm256_abs_epi16(xmmp4); // registers of even index in xmm0-> |y_R|, registers of odd index in xmm0-> |y_I|
    xmmp2 = simde_mm256_subs_epi16(xmmp1,xmmp2); // registers of even index in xmm0-> |y_R|-|h|^2, registers of odd index in xmm0-> |y_I|-|h|^2
    xmmp7 = simde_mm256_abs_epi16(xmmp2);
    xmmp7 = simde_mm256_subs_epi16(xmmp6,xmmp7);
    
    if (aarx == 0)
    {
      llr64[0]  = simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp2,0),simde_mm256_extract_epi32(xmmp4,0));
      llr64[1]  = simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp4,1),simde_mm256_extract_epi32(xmmp7,0));
      llr64[2]  = simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp7,1),simde_mm256_extract_epi32(xmmp2,1));
      llr64[3]  = simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp2,2),simde_mm256_extract_epi32(xmmp4,2));
      llr64[4]  = simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp4,3),simde_mm256_extract_epi32(xmmp7,2));
      llr64[5]  = simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp7,3),simde_mm256_extract_epi32(xmmp2,3));
      llr64[6]  = simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp2,4),simde_mm256_extract_epi32(xmmp4,4));
      llr64[7]  = simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp4,5),simde_mm256_extract_epi32(xmmp7,4));
      llr64[8]  = simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp7,5),simde_mm256_extract_epi32(xmmp2,5));
      llr64[9]  = simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp2,6),simde_mm256_extract_epi32(xmmp4,6));
      llr64[10] = simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp4,7),simde_mm256_extract_epi32(xmmp7,6));
      llr64[11] = simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp7,7),simde_mm256_extract_epi32(xmmp2,7));
      llr64+=12;
    }
    else
    {
      llr64[0] = simde_mm_adds_pi16(llr64[0],simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp2,0),simde_mm256_extract_epi32(xmmp4,0)));
      llr64[1] = simde_mm_adds_pi16(llr64[1],simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp4,1),simde_mm256_extract_epi32(xmmp7,0)));
      llr64[2] = simde_mm_adds_pi16(llr64[2],simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp7,1),simde_mm256_extract_epi32(xmmp2,1)));
      llr64[3] = simde_mm_adds_pi16(llr64[3],simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp2,2),simde_mm256_extract_epi32(xmmp4,2)));
      llr64[4] = simde_mm_adds_pi16(llr64[4],simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp4,3),simde_mm256_extract_epi32(xmmp7,2)));
      llr64[5] = simde_mm_adds_pi16(llr64[5],simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp7,3),simde_mm256_extract_epi32(xmmp2,3)));
      llr64[6] = simde_mm_adds_pi16(llr64[6],simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp2,4),simde_mm256_extract_epi32(xmmp4,4)));
      llr64[7] = simde_mm_adds_pi16(llr64[7],simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp4,5),simde_mm256_extract_epi32(xmmp7,4)));
      llr64[8] = simde_mm_adds_pi16(llr64[8],simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp7,5),simde_mm256_extract_epi32(xmmp2,5)));
      llr64[9] = simde_mm_adds_pi16(llr64[9],simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp2,6),simde_mm256_extract_epi32(xmmp4,6)));
      llr64[10] = simde_mm_adds_pi16(llr64[10],simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp4,7),simde_mm256_extract_epi32(xmmp7,6)));
      llr64[11] = simde_mm_adds_pi16(llr64[11],simde_mm_set_pi32(simde_mm256_extract_epi32(xmmp7,7),simde_mm256_extract_epi32(xmmp2,7)));
      llr64+=12;
    }
  }
#else
  register simde__m128i xmmp0, xmmp1, xmmp2, xmmp3, xmmp4, xmmp6, xmmp7;
  register simde__m128i complex_shuffle128 = simde_mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
  register simde__m128i conj128 = simde_mm_set_epi16(1, -1, 1, -1, 1, -1, 1, -1);
  // register simde__m128i conj128 = simde_mm_set_epi16(1, -1, 1, -1, 1, -1, 1, -1);
  register simde__m128i QAM_amp128  = simde_mm_set1_epi16(QAM64_n1);  // 4/sqrt(42)
  register simde__m128i QAM_amp128b = simde_mm_set1_epi16(QAM64_n2);  // 2/sqrt(42)
  simde__m128i *rxF128  = (simde__m128i*) rxF;
  simde__m128i *ulch128 = (simde__m128i*) ul_ch;
  // need to use simde__m64 because llr output is not necessarily aligned to 256 bits, but it is always to 64 bits
  simde__m64 *llr64 = (simde__m64 *)llr;
  for (int i = 0; i < (length>>2); i++) 
  {
    xmmp0  = simde_mm_madd_epi16(ulch128[i], rxF128[i]);
    // xmmp0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmmp1  = simde_mm_shuffle_epi8(ulch128[i], complex_shuffle128);
    xmmp1  = simde_mm_sign_epi16(xmmp1, conj128);
    xmmp1  = simde_mm_madd_epi16(xmmp1, rxF128[i]);
    // xmmp1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmmp0  = simde_mm_srai_epi32(xmmp0, output_shift);
    xmmp1  = simde_mm_srai_epi32(xmmp1, output_shift);
    xmmp2  = simde_mm_unpacklo_epi32(xmmp0, xmmp1);
    xmmp3  = simde_mm_unpackhi_epi32(xmmp0, xmmp1);
    xmmp4  = simde_mm_packs_epi32(xmmp2, xmmp3);

    // compute channel amplitude for LLR
    xmmp0 = simde_mm_madd_epi16(ulch128[i], ulch128[i]);
    xmmp0 = simde_mm_srai_epi32(xmmp0, output_shift);
    xmmp0 = simde_mm_packs_epi32(xmmp0, xmmp0);
    xmmp2 = simde_mm_unpacklo_epi16(xmmp0, xmmp0);
    xmmp1 = simde_mm_mulhrs_epi16(xmmp2, QAM_amp128);
    xmmp6 = simde_mm_mulhrs_epi16(xmmp2, QAM_amp128b);

    xmmp2 = simde_mm_abs_epi16(xmmp4);            // registers of even index in xmm0-> |y_R|, registers of odd index in xmm0-> |y_I|
    xmmp2 = simde_mm_subs_epi16(xmmp1, xmmp2); // registers of even index in xmm0-> |y_R|-|h|^2, registers of odd index in xmm0-> |y_I|-|h|^2
    xmmp7 = simde_mm_abs_epi16(xmmp2);
    xmmp7 = simde_mm_subs_epi16(xmmp6, xmmp7);
    
    if (aarx == 0)
    {
      llr64[0] = simde_mm_set_pi32(simde_mm_extract_epi32(xmmp2, 0), simde_mm_extract_epi32(xmmp4, 0));
      llr64[1] = simde_mm_set_pi32(simde_mm_extract_epi32(xmmp4, 1), simde_mm_extract_epi32(xmmp7, 0));
      llr64[2] = simde_mm_set_pi32(simde_mm_extract_epi32(xmmp7, 1), simde_mm_extract_epi32(xmmp2, 1));
      llr64[3] = simde_mm_set_pi32(simde_mm_extract_epi32(xmmp2, 2), simde_mm_extract_epi32(xmmp4, 2));
      llr64[4] = simde_mm_set_pi32(simde_mm_extract_epi32(xmmp4, 3), simde_mm_extract_epi32(xmmp7, 2));
      llr64[5] = simde_mm_set_pi32(simde_mm_extract_epi32(xmmp7, 3), simde_mm_extract_epi32(xmmp2, 3));
      llr64 += 6;
    }
    else
    {
      llr64[0] = simde_mm_adds_pi16(llr64[0], simde_mm_set_pi32(simde_mm_extract_epi32(xmmp2, 0),simde_mm_extract_epi32(xmmp4, 0)));
      llr64[1] = simde_mm_adds_pi16(llr64[1], simde_mm_set_pi32(simde_mm_extract_epi32(xmmp4, 1),simde_mm_extract_epi32(xmmp7, 0)));
      llr64[2] = simde_mm_adds_pi16(llr64[2], simde_mm_set_pi32(simde_mm_extract_epi32(xmmp7, 1),simde_mm_extract_epi32(xmmp2, 1)));
      llr64[3] = simde_mm_adds_pi16(llr64[3], simde_mm_set_pi32(simde_mm_extract_epi32(xmmp2, 2),simde_mm_extract_epi32(xmmp4, 2)));
      llr64[4] = simde_mm_adds_pi16(llr64[4], simde_mm_set_pi32(simde_mm_extract_epi32(xmmp4, 3),simde_mm_extract_epi32(xmmp7, 2)));
      llr64[5] = simde_mm_adds_pi16(llr64[5], simde_mm_set_pi32(simde_mm_extract_epi32(xmmp7, 3),simde_mm_extract_epi32(xmmp2, 3)));
      llr64 += 6;
    }
  }
  if (length & 3) 
  {
    int i = (length>>1) - 1;
    simde__m64 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5;
    simde__m64 complex_shuffle64 = simde_mm_set_pi8(5,4,7,6,1,0,3,2);
    simde__m64 conj64 = simde_mm_set_pi16(1, -1, 1, -1);
    simde__m64 *rxF64     = (simde__m64*)rxF;
    simde__m64 *ulch64    = (simde__m64*)ul_ch;
    simde__m64 QAM_amp    = simde_mm_set1_pi16(QAM64_n1);
    simde__m64 QAM_ampb   = simde_mm_set1_pi16(QAM64_n2);

    xmm0  = simde_mm_madd_pi16(ulch64[i], rxF64[i]);
    // xmm0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmm1  = simde_mm_shuffle_pi8(ulch64[i], complex_shuffle64);
    xmm1 = simde_mm_sign_pi16(xmm1, conj64);
    xmm1  = simde_mm_madd_pi16(xmm1, rxF64[i]);
    // xmm1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmm0  = simde_mm_srai_pi32(xmm0, output_shift);
    xmm1  = simde_mm_srai_pi32(xmm1, output_shift);
    xmm2  = simde_mm_unpacklo_pi32(xmm0, xmm1);
    xmm3  = simde_mm_unpackhi_pi32(xmm0, xmm1);
    xmm4  = simde_mm_packs_pi32(xmm2, xmm3);

    // compute channel amplitude for LLR
    xmm0 = simde_mm_madd_pi16(ulch64[i], ulch64[i]); // |h|^2
    xmm0 = simde_mm_srai_pi32(xmm0, output_shift);
    xmm0 = simde_mm_packs_pi32(xmm0, xmm0);
    xmm2 = simde_mm_unpacklo_pi16(xmm0, xmm0);
    
    xmm1 = simde_mm_mulhrs_pi16(xmm2, QAM_amp);
    xmm5 = simde_mm_mulhrs_pi16(xmm2, QAM_ampb);
    
    xmm0 = simde_mm_abs_pi16(xmm4);        // registers of even index in xmm0-> |y_R|, registers of odd index in xmm0-> |y_I|
    xmm0 = simde_mm_subs_pi16(xmm1, xmm0); // registers of even index in xmm0-> |y_R|-|h|^2, registers of odd index in xmm0-> |y_I|-|h|^2
    xmm1 = simde_mm_abs_pi16(xmm0);
    xmm1 = simde_mm_subs_pi16(xmm5, xmm1); // contains 8 LLRs
    if (aarx == 0)
    {
      llr64[0] = simde_mm_set_pi32(simde_mm_extract_pi16(xmm0, 0), simde_mm_extract_pi16(xmm4, 0));
      llr64[1] = simde_mm_set_pi32(simde_mm_extract_pi16(xmm4, 1), simde_mm_extract_pi16(xmm1, 0));
      llr64[2] = simde_mm_set_pi32(simde_mm_extract_pi16(xmm1, 1), simde_mm_extract_pi16(xmm0, 1));
    }
    else
    {
      llr64[0] = simde_mm_adds_pi16(llr64[0], simde_mm_set_pi32(simde_mm_extract_pi16(xmm0, 0),simde_mm_extract_pi16(xmm4, 0)));
      llr64[1] = simde_mm_adds_pi16(llr64[1], simde_mm_set_pi32(simde_mm_extract_pi16(xmm4, 1),simde_mm_extract_pi16(xmm1, 0)));
      llr64[2] = simde_mm_adds_pi16(llr64[2], simde_mm_set_pi32(simde_mm_extract_pi16(xmm1, 1),simde_mm_extract_pi16(xmm0, 1)));
    }
  }
#endif
}

void inner_rx_256qam (int *rxF, 
                      int *ul_ch, 
                      int16_t *llr, 
                      int aarx, 
                      int length,
                      int output_shift) 
{
#ifndef USE_128BIT
  register simde__m256i xmmp0, xmmp1, xmmp2, xmmp3, xmmp4, xmmp5, xmmp6, xmmp7, xmmp8, xmmp9;
  register simde__m256i complex_shuffle256 = simde_mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);
  register simde__m256i conj256 = simde_mm256_set_epi16(1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1);
  
  register simde__m256i QAM_amp256  = simde_mm256_set1_epi16(QAM256_n1);
  register simde__m256i QAM_amp256b = simde_mm256_set1_epi16(QAM256_n2);
  register simde__m256i QAM_amp256c = simde_mm256_set1_epi16(QAM256_n3);
  simde__m256i *rxF256  = (simde__m256i*)rxF;
  simde__m256i *ulch256 = (simde__m256i*)ul_ch;
  simde__m256i *llr256 = (simde__m256i *)llr;
  for (int i = 0; i < ((length >> 3) + (( length & 7) > 0 ? 1 : 0)); i++) 
  {
    xmmp0  = simde_mm256_madd_epi16(ulch256[i],rxF256[i]);
    // xmmp0 contains real part of 8 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmmp1  = simde_mm256_shuffle_epi8(ulch256[i],complex_shuffle256);
    xmmp1  = simde_mm256_sign_epi16(xmmp1,conj256);
    xmmp1  = simde_mm256_madd_epi16(xmmp1,rxF256[i]);
    // xmmp1 contains imag part of 8 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmmp0  = simde_mm256_srai_epi32(xmmp0,output_shift);
    xmmp1  = simde_mm256_srai_epi32(xmmp1,output_shift);
    xmmp2  = simde_mm256_unpacklo_epi32(xmmp0,xmmp1);
    xmmp3  = simde_mm256_unpackhi_epi32(xmmp0,xmmp1);
    xmmp4  = simde_mm256_packs_epi32(xmmp2,xmmp3);

    // compute channel amplitude for LLR
    xmmp0 = simde_mm256_madd_epi16(ulch256[i],ulch256[i]);
    xmmp0 = simde_mm256_srai_epi32(xmmp0,output_shift);
    xmmp0 = simde_mm256_packs_epi32(xmmp0,xmmp0);   // contains 16 LLRs 
    xmmp2 = simde_mm256_unpacklo_epi16(xmmp0,xmmp0);
    xmmp1 = simde_mm256_mulhrs_epi16(xmmp2,QAM_amp256);
    xmmp6 = simde_mm256_mulhrs_epi16(xmmp2,QAM_amp256b);
    xmmp8 = simde_mm256_mulhrs_epi16(xmmp2,QAM_amp256c);

    xmmp2 = simde_mm256_abs_epi16(xmmp4); // registers of even index in xmm0-> |y_R|, registers of odd index in xmm0-> |y_I|
    xmmp2 = simde_mm256_subs_epi16(xmmp1,xmmp2); // registers of even index in xmm0-> |y_R|-|h|^2, registers of odd index in xmm0-> |y_I|-|h|^2
    // xmmp2 contains 16 LLRs
    xmmp7 = simde_mm256_abs_epi16(xmmp2);
    xmmp7 = simde_mm256_subs_epi16(xmmp6,xmmp7); // contains 16 LLRs
    xmmp9 = simde_mm256_abs_epi16(xmmp7);
    xmmp9 = simde_mm256_subs_epi16(xmmp8,xmmp9); // contains 16 LLRs

    // xmmp4 A0 A1 A2 A3 A4 A5 A6 A7
    // xmmp2 B0 B1 B2 B3 B4 B5 B6 B7
    // xmmp7 C0 C1 C2 C3 C4 C5 C6 C7
    // xmmp9 D0 D1 D2 D3 D4 D5 D6 D7  
    xmmp1 = simde_mm256_unpacklo_epi32(xmmp4,xmmp2); // A0 B0 A1 B1 A4 B4 A5 B5
    xmmp3 = simde_mm256_unpackhi_epi32(xmmp4,xmmp2); // A2 B2 A3 B3 A6 B6 A7 B7
    xmmp5 = simde_mm256_unpacklo_epi32(xmmp7,xmmp9); // C0 D0 C1 D1 C4 D4 C5 D5
    xmmp6 = simde_mm256_unpackhi_epi32(xmmp7,xmmp9); // C2 D2 C3 D3 C6 D6 C7 D7

    xmmp2 = simde_mm256_unpacklo_epi64(xmmp1,xmmp5); // A0 B0 C0 D0 A4 B4 C4 D4
    xmmp4 = simde_mm256_unpackhi_epi64(xmmp1,xmmp5); // A1 B1 C1 D1 A5 B5 C5 D5
    xmmp1 = simde_mm256_unpacklo_epi64(xmmp3,xmmp6); // A2 B2 C2 D2 A6 B6 C6 D6
    xmmp5 = simde_mm256_unpackhi_epi64(xmmp3,xmmp6); // A3 B3 C3 D3 A7 B7 C7 D7
    if (aarx == 0)
    {
      llr256[0] = simde_mm256_permute2x128_si256(xmmp2, xmmp4, 0x20); // A0 B0 C0 D0 A1 B1 C1 D1
      llr256[1] = simde_mm256_permute2x128_si256(xmmp1, xmmp5, 0x20); // A2 B2 C2 D2 A3 B3 C3 D3
      llr256[2] = simde_mm256_permute2x128_si256(xmmp2, xmmp4, 0x31); // A4 B4 C4 D4 A5 B5 C5 D5
      llr256[3] = simde_mm256_permute2x128_si256(xmmp1, xmmp5, 0x31); // A6 B6 C6 D6 A7 B7 C7 D7
      llr256+=4;
    }
    else
    {
      llr256[0] = simde_mm256_adds_epi16(llr256[0],simde_mm256_permute2x128_si256(xmmp2, xmmp4, 0x20)); // A0 B0 C0 D0 A1 B1 C1 D1
      llr256[1] = simde_mm256_adds_epi16(llr256[1],simde_mm256_permute2x128_si256(xmmp1, xmmp5, 0x20)); // A2 B2 C2 D2 A3 B3 C3 D3
      llr256[2] = simde_mm256_adds_epi16(llr256[2],simde_mm256_permute2x128_si256(xmmp2, xmmp4, 0x31)); // A4 B4 C4 D4 A5 B5 C5 D5
      llr256[3] = simde_mm256_adds_epi16(llr256[3],simde_mm256_permute2x128_si256(xmmp1, xmmp5, 0x31)); // A6 B6 C6 D6 A7 B7 C7 D7
      llr256+=4;
    }
  }
  simde__m128i *llr128 = (simde__m128i*)llr256;
  if ((length&7) >= 4) { //there is a single 128-bit input element remaining
    int nb_re128 = length>>2;
    simde__m128i xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6;
    simde__m128i complex_shuffle128 = simde_mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);
    simde__m128i conj128 = simde_mm_set_epi16(1,-1,1,-1,1,-1,1,-1);
    simde__m128i *rxF128     = (simde__m128i*)rxF;
    simde__m128i *ulch128    = (simde__m128i*)ul_ch;
    simde__m128i QAM_amp     = simde_mm_set1_epi16(QAM256_n1);  // 2/sqrt(10)
    simde__m128i QAM_ampb    = simde_mm_set1_epi16(QAM256_n2);
    simde__m128i QAM_ampc    = simde_mm_set1_epi16(QAM256_n3);

    xmm0  = simde_mm_madd_epi16(ulch128[nb_re128-1],rxF128[nb_re128-1]);
    // xmm0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmm1  = simde_mm_shuffle_epi8(ulch128[nb_re128-1],complex_shuffle128);
    xmm1  = simde_mm_sign_epi16(xmm1,conj128);
    xmm1  = simde_mm_madd_epi16(xmm1,rxF128[nb_re128-1]);
    // xmm1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmm0  = simde_mm_srai_epi32(xmm0,output_shift);
    xmm1  = simde_mm_srai_epi32(xmm1,output_shift);
    xmm2  = simde_mm_unpacklo_epi32(xmm0,xmm1);
    xmm3  = simde_mm_unpackhi_epi32(xmm0,xmm1);
    xmm4  = simde_mm_packs_epi32(xmm2,xmm3);

    // compute channel amplitude for LLR
    xmm0 = simde_mm_madd_epi16(ulch128[nb_re128-1],ulch128[nb_re128-1]);
    xmm0 = simde_mm_srai_epi32(xmm0,output_shift);
    xmm0 = simde_mm_packs_epi32(xmm0,xmm0);   // contains 16 LLRs 
    xmm2 = simde_mm_unpacklo_epi16(xmm0,xmm0);
    xmm1 = simde_mm_mulhrs_epi16(xmm2,QAM_amp);
    xmm5 = simde_mm_mulhrs_epi16(xmm2,QAM_ampb);
    xmm6 = simde_mm_mulhrs_epi16(xmm2,QAM_ampc);
    xmm0 = simde_mm_abs_epi16(xmm4); // registers of even index in xmm0-> |y_R|, registers of odd index in xmm0-> |y_I|
    xmm0 = simde_mm_subs_epi16(xmm1,xmm0); // registers of even index in xmm0-> |y_R|-|h|^2, registers of odd index in xmm0-> |y_I|-|h|^2
    //  xmmp2 contains 8 LLRs
    xmm1 = simde_mm_abs_epi16(xmm0);
    xmm1 = simde_mm_subs_epi16(xmm5,xmm1); // contains 8 LLRs
    xmm2 = simde_mm_abs_epi16(xmm1);
    xmm2 = simde_mm_subs_epi16(xmm6,xmm2); // contains 8 LLRs
    // rxF[i] A0 A1 A2 A3
    // xmm0   B0 B1 B2 B3
    // xmm1   C0 C1 C2 C3
    // xmm2   D0 D1 D2 D3
    xmm3 = simde_mm_unpacklo_epi32(rxF128[nb_re128-1],xmm0); // A0 B0 A1 B1 
    xmm4 = simde_mm_unpackhi_epi32(rxF128[nb_re128-1],xmm0); // A2 B2 A3 B3
    xmm5 = simde_mm_unpacklo_epi32(xmm1,xmm2);   // C0 D0 C1 D1
    xmm6 = simde_mm_unpackhi_epi32(xmm1,xmm2);   // C2 D2 C3 D3
    if (aarx == 0) {
      llr128[0] = simde_mm_unpacklo_epi64(xmm3,xmm5); // A0 B0 C0 D0
      llr128[1] = simde_mm_unpackhi_epi64(xmm3,xmm5); // A1 B1 C1 D1 
      llr128[2] = simde_mm_unpacklo_epi64(xmm4,xmm6); // A2 B2 C2 D2 
      llr128[3] = simde_mm_unpackhi_epi64(xmm4,xmm6); // A3 B3 C3 D3
      llr128+=4;
    }
    else 
    {
      llr128[0] = simde_mm_adds_epi16(llr128[0],simde_mm_unpacklo_epi64(xmm3,xmm5)); // A0 B0 C0 D0
      llr128[1] = simde_mm_adds_epi16(llr128[1],simde_mm_unpackhi_epi64(xmm3,xmm5)); // A1 B1 C1 D1 
      llr128[2] = simde_mm_adds_epi16(llr128[2],simde_mm_unpacklo_epi64(xmm4,xmm6)); // A2 B2 C2 D2 
      llr128[3] = simde_mm_adds_epi16(llr128[3],simde_mm_unpackhi_epi64(xmm4,xmm6)); // A3 B3 C3 D3
      llr128+=4;
   }
 }
#else
  simde__m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6;
  simde__m128i complex_shuffle128 = simde_mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
  simde__m128i conj128            = simde_mm_set_epi16(1, -1, 1, -1, 1, -1, 1, -1);
  simde__m128i *rxF128            = (simde__m128i*)rxF;
  simde__m128i *ulch128           = (simde__m128i*)ul_ch;
  simde__m128i QAM_amp            = simde_mm_set1_epi16(QAM256_n1);
  simde__m128i QAM_ampb           = simde_mm_set1_epi16(QAM256_n2);
  simde__m128i QAM_ampc           = simde_mm_set1_epi16(QAM256_n3);
  simde__m128i *llr128            = (simde__m128i*)llr;    
  for (int i = 0; i < (length >> 2); i++) 
  {
    xmm0  = simde_mm_madd_epi16(ulch128[i], rxF128[i]);
    // xmm0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmm1  = simde_mm_shuffle_epi8(ulch128[i], complex_shuffle128);
    xmm1 = simde_mm_sign_epi16(xmm1, conj128);
    xmm1  = simde_mm_madd_epi16(xmm1, rxF128[i]);
    // xmm1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmm0  = simde_mm_srai_epi32(xmm0, output_shift);
    xmm1  = simde_mm_srai_epi32(xmm1, output_shift);
    xmm2  = simde_mm_unpacklo_epi32(xmm0, xmm1);
    xmm3  = simde_mm_unpackhi_epi32(xmm0, xmm1);
    xmm4  = simde_mm_packs_epi32(xmm2, xmm3);

    // compute channel amplitude for LLR
    xmm0 = simde_mm_madd_epi16(ulch128[i], ulch128[i]); // |h|^2
    xmm0 = simde_mm_srai_epi32(xmm0, output_shift);
    xmm0 = simde_mm_packs_epi32(xmm0, xmm0);
    xmm2 = simde_mm_unpacklo_epi16(xmm0, xmm0);
    
    xmm1 = simde_mm_mulhrs_epi16(xmm2, QAM_amp);
    xmm5 = simde_mm_mulhrs_epi16(xmm2, QAM_ampb);
    xmm6 = simde_mm_mulhrs_epi16(xmm2, QAM_ampc);
    
    xmm0 = simde_mm_abs_epi16(xmm4);        // registers of even index in xmm0-> |y_R|, registers of odd index in xmm0-> |y_I|
    xmm0 = simde_mm_subs_epi16(xmm1, xmm0); // registers of even index in xmm0-> |y_R|-|h|^2, registers of odd index in xmm0-> |y_I|-|h|^2
    xmm1 = simde_mm_abs_epi16(xmm0);
    xmm1 = simde_mm_subs_epi16(xmm5, xmm1); // contains 8 LLRs
    xmm2 = simde_mm_abs_epi16(xmm1);
    xmm2 = simde_mm_subs_epi16(xmm6, xmm2); // contains 8 LLRs
    // rxF[i] A0 A1 A2 A3
    // xmm0   B0 B1 B2 B3
    // xmm1   C0 C1 C2 C3
    // xmm2   D0 D1 D2 D3
    xmm3 = simde_mm_unpacklo_epi32(xmm4, xmm0); // A0 B0 A1 B1 
    xmm4 = simde_mm_unpackhi_epi32(xmm4, xmm0); // A2 B2 A3 B3
    xmm5 = simde_mm_unpacklo_epi32(xmm1, xmm2);      // C0 D0 C1 D1
    xmm6 = simde_mm_unpackhi_epi32(xmm1, xmm2);      // C2 D2 C3 D3
    if (aarx == 0) {
      llr128[0] = simde_mm_unpacklo_epi64(xmm3, xmm5); // A0 B0 C0 D0
      llr128[1] = simde_mm_unpackhi_epi64(xmm3, xmm5); // A1 B1 C1 D1 
      llr128[2] = simde_mm_unpacklo_epi64(xmm4, xmm6); // A2 B2 C2 D2 
      llr128[3] = simde_mm_unpackhi_epi64(xmm4, xmm6); // A3 B3 C3 D3
    }
    else {
      llr128[0] = simde_mm_adds_epi16(llr128[0], simde_mm_unpacklo_epi64(xmm3, xmm5)); // A0 B0 C0 D0
      llr128[1] = simde_mm_adds_epi16(llr128[1], simde_mm_unpackhi_epi64(xmm3, xmm5)); // A1 B1 C1 D1 
      llr128[2] = simde_mm_adds_epi16(llr128[2], simde_mm_unpacklo_epi64(xmm4, xmm6)); // A2 B2 C2 D2 
      llr128[3] = simde_mm_adds_epi16(llr128[3], simde_mm_unpackhi_epi64(xmm4, xmm6)); // A3 B3 C3 D3
    }
    llr128+=4;
  }
  if (length & 3) 
  {
    simde__m64 *llr64 = (simde__m64*) llr128;
    int i = (length>>1) - 1;
    simde__m64 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6;
    simde__m64 complex_shuffle64 = simde_mm_set_pi8(5,4,7,6,1,0,3,2);
    simde__m64 conj64 = simde_mm_set_pi16(1, -1, 1, -1);
    simde__m64 *rxF64     = (simde__m64*)rxF;
    simde__m64 *ulch64    = (simde__m64*)ul_ch;
    simde__m64 QAM_amp    = simde_mm_set1_pi16(QAM256_n1);
    simde__m64 QAM_ampb   = simde_mm_set1_pi16(QAM256_n2);
    simde__m64 QAM_ampc   = simde_mm_set1_pi16(QAM256_n3);

    xmm0  = simde_mm_madd_pi16(ulch64[i], rxF64[i]);
    // xmm0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmm1  = simde_mm_shuffle_pi8(ulch64[i], complex_shuffle64);
    xmm1 = simde_mm_sign_pi16(xmm1, conj64);
    xmm1  = simde_mm_madd_pi16(xmm1, rxF64[i]);
    // xmm1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
    xmm0  = simde_mm_srai_pi32(xmm0, output_shift);
    xmm1  = simde_mm_srai_pi32(xmm1, output_shift);
    xmm2  = simde_mm_unpacklo_pi32(xmm0, xmm1);
    xmm3  = simde_mm_unpackhi_pi32(xmm0, xmm1);
    xmm4  = simde_mm_packs_pi32(xmm2, xmm3);

    // compute channel amplitude for LLR
    xmm0 = simde_mm_madd_pi16(ulch64[i], ulch64[i]); // |h|^2
    xmm0 = simde_mm_srai_pi32(xmm0, output_shift);
    xmm0 = simde_mm_packs_pi32(xmm0, xmm0);
    xmm2 = simde_mm_unpacklo_pi16(xmm0, xmm0);
    
    xmm1 = simde_mm_mulhrs_pi16(xmm2, QAM_amp);
    xmm5 = simde_mm_mulhrs_pi16(xmm2, QAM_ampb);
    xmm6 = simde_mm_mulhrs_pi16(xmm2, QAM_ampc);
    
    xmm0 = simde_mm_abs_pi16(xmm4);        // registers of even index in xmm0-> |y_R|, registers of odd index in xmm0-> |y_I|
    xmm0 = simde_mm_subs_pi16(xmm1, xmm0); // registers of even index in xmm0-> |y_R|-|h|^2, registers of odd index in xmm0-> |y_I|-|h|^2
    xmm1 = simde_mm_abs_pi16(xmm0);
    xmm1 = simde_mm_subs_pi16(xmm5, xmm1); // contains 8 LLRs
    xmm2 = simde_mm_abs_pi16(xmm1);
    xmm2 = simde_mm_subs_pi16(xmm6, xmm2); // contains 8 LLRs
    
    xmm3 = simde_mm_unpacklo_pi32(xmm4, xmm0);
    xmm4 = simde_mm_unpackhi_pi32(xmm4, xmm0);
    xmm5 = simde_mm_unpacklo_pi32(xmm1, xmm2);
    xmm6 = simde_mm_unpackhi_pi32(xmm1, xmm2);
    if (aarx == 0) {
      llr64[0] = simde_m_punpckldq(xmm3, xmm5);
      llr64[1] = simde_m_punpckhdq(xmm3, xmm5);
      llr64[2] = simde_m_punpckldq(xmm4, xmm6);
      llr64[3] = simde_m_punpckhdq(xmm4, xmm6);
    }
    else 
    {
      llr64[0] = simde_mm_adds_pi16(llr64[0], simde_m_punpckldq(xmm3, xmm5));
      llr64[1] = simde_mm_adds_pi16(llr64[1], simde_m_punpckhdq(xmm3, xmm5));
      llr64[2] = simde_mm_adds_pi16(llr64[2], simde_m_punpckldq(xmm4, xmm6));
      llr64[3] = simde_mm_adds_pi16(llr64[3], simde_m_punpckhdq(xmm4, xmm6));
    }
  }
#endif
}

void inner_rx_qpsk_2layer (NR_DL_FRAME_PARMS *frame_parms,
                           NR_gNB_PUSCH *pusch_vars, 
                           nfapi_nr_pusch_pdu_t *rel15_ul,
                           int **rxF, 
                           int **ul_ch, 
                           int16_t **llr,
                           int nb_layer,
                           int nb_rx_ant, 
                           int soffset,
                           int length, 
                           int symbol,
                           int short nb_rb,
                           int dmrs_symbol_flag,
                           int output_shift)
{
  int add_shift = 0;
  if (length % 8)
    add_shift = 8 - length % 8;
  int32_t rxFext[nb_rx_ant][length + add_shift] __attribute__((aligned(32)));
  int32_t chFext[nb_layer*nb_rx_ant][length + add_shift] __attribute__((aligned(32)));
  for (int aarx = 0; aarx < nb_rx_ant; aarx++) 
  {
    for (int aatx = 0; aatx < nb_layer; aatx++) 
    {
      nr_ulsch_extract_rbs0((c16_t *)rxF[aarx],
                            pusch_vars->ul_ch_estimates[aatx * nb_rx_ant + aarx],
                            rxFext[aarx],
                            chFext[aatx * nb_rx_ant + aarx],
                            soffset+(symbol * frame_parms->ofdm_symbol_size),
                            pusch_vars->dmrs_symbol * frame_parms->ofdm_symbol_size,
                            aarx,
                            dmrs_symbol_flag, 
                            rel15_ul,
                            frame_parms);
    }
  }
  int32_t rho[nb_layer*nb_layer][length + add_shift] __attribute__((aligned(32)));
  int32_t rxFext_comp[nb_layer][length + add_shift] __attribute__((aligned(32)));
  for (int aarx = 0; aarx < nb_rx_ant; aarx++) 
  {
    for (int aatx = 0; aatx < nb_layer; aatx++) 
    {
      for (int atx = 0; atx < nb_layer; atx++) 
      {
#ifdef USE_128BIT
        simde__m128i mmtmpD0, mmtmpD1, mmtmpD2, mmtmpD3, mmtmpD4;
        simde__m128i *rho128        = (simde__m128i *)rho[aatx*nb_layer+atx];
        simde__m128i *ul_ch128      = (simde__m128i *)chFext[aatx * nb_rx_ant + aarx];
        simde__m128i *ul_ch128_2    = (simde__m128i *)chFext[atx * nb_rx_ant + aarx];
        for (int i = 0; i < (length >> 2)+((length&3)?1:0); i++) 
        {
          // multiply by conjugated channel
          mmtmpD0 = simde_mm_madd_epi16(ul_ch128[i], ul_ch128_2[i]);
          // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
          mmtmpD1 = simde_mm_shufflelo_epi16(ul_ch128[i], SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_sign_epi16(mmtmpD1, *(simde__m128i*)&conjugate[0]);
          mmtmpD1 = simde_mm_madd_epi16(mmtmpD1, ul_ch128_2[i]);
          // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
          mmtmpD0 = simde_mm_srai_epi32(mmtmpD0, output_shift);
          mmtmpD1 = simde_mm_srai_epi32(mmtmpD1, output_shift);
          mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0, mmtmpD1);
          mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0, mmtmpD1);
          mmtmpD4 = simde_mm_packs_epi32(mmtmpD2, mmtmpD3);
          if (aarx == 0)
            rho128[i] = mmtmpD4;
          else
            rho128[i] = simde_mm_adds_epi16(rho128[i], mmtmpD4);
        }
#else
        simde__m256i mmtmpD0, mmtmpD1, mmtmpD2, mmtmpD3, mmtmpD4;
        simde__m256i *rho256        = (simde__m256i *)rho[aatx*nb_layer+atx];
        simde__m256i *ul_ch256      = (simde__m256i *)chFext[aatx * nb_rx_ant + aarx];
        simde__m256i *ul_ch256_2    = (simde__m256i *)chFext[atx * nb_rx_ant + aarx];
        register simde__m256i complex_shuffle256 = simde_mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);
        register simde__m256i conj256 = simde_mm256_set_epi16(1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1);
        for (int i = 0; i < ((length >> 3)+((length&7)?1:0)); i++) 
        {
          // multiply by conjugated channel
          mmtmpD0 = simde_mm256_madd_epi16(ul_ch256[i], ul_ch256_2[i]);
          // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
          mmtmpD1 = simde_mm256_shuffle_epi8(ul_ch256[i], complex_shuffle256);
          mmtmpD1 = simde_mm256_sign_epi16(mmtmpD1, conj256);
          mmtmpD1 = simde_mm256_madd_epi16(mmtmpD1, ul_ch256_2[i]);
          // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
          mmtmpD0 = simde_mm256_srai_epi32(mmtmpD0, output_shift);
          mmtmpD1 = simde_mm256_srai_epi32(mmtmpD1, output_shift);
          mmtmpD2 = simde_mm256_unpacklo_epi32(mmtmpD0, mmtmpD1);
          mmtmpD3 = simde_mm256_unpackhi_epi32(mmtmpD0, mmtmpD1);
          mmtmpD4 = simde_mm256_packs_epi32(mmtmpD2, mmtmpD3);
          if (aarx == 0)
            rho256[i] = mmtmpD4;
          else
            rho256[i] = simde_mm256_adds_epi16(rho256[i], mmtmpD4);
        }
#endif
      }
      // compensation
#ifdef USE_128BIT
      simde__m128i xmmp0, xmmp1, xmmp2, xmmp3, xmmp4;
      register simde__m128i complex_shuffle128 = simde_mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
      register simde__m128i conj128 = simde_mm_set_epi16(1, -1, 1, -1, 1, -1, 1, -1);
      simde__m128i *rxF128  = (simde__m128i*)rxFext[aarx];
      simde__m128i *ulch128 = (simde__m128i*)chFext[aatx * nb_rx_ant + aarx];
      simde__m128i *rxF_comp128 = (simde__m128i*)rxFext_comp[aatx];
      for (int i = 0; i < (length>>2) + ((length&3)?1:0); i++) 
      {
        xmmp0  = simde_mm_madd_epi16(ulch128[i], rxF128[i]);
        // xmmp0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
        xmmp1  = simde_mm_shuffle_epi8(ulch128[i], complex_shuffle128);
        xmmp1  = simde_mm_sign_epi16(xmmp1, conj128);
        xmmp1  = simde_mm_madd_epi16(xmmp1, rxF128[i]);
        // xmmp1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
        xmmp0  = simde_mm_srai_epi32(xmmp0, output_shift);
        xmmp1  = simde_mm_srai_epi32(xmmp1, output_shift);
        xmmp2  = simde_mm_unpacklo_epi32(xmmp0, xmmp1);
        xmmp3  = simde_mm_unpackhi_epi32(xmmp0, xmmp1);
        xmmp4  = simde_mm_packs_epi32(xmmp2, xmmp3);

        if (aarx == 0) 
          *rxF_comp128 = xmmp4;
        else
          *rxF_comp128 = simde_mm_adds_epi16(*rxF_comp128, xmmp4);

        rxF_comp128++;
      }
#else
      simde__m256i xmmp0, xmmp1, xmmp2, xmmp3, xmmp4;
      register simde__m256i complex_shuffle256 = simde_mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);
      register simde__m256i conj256 = simde_mm256_set_epi16(1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1);
      simde__m256i *rxF256  = (simde__m256i*)rxFext[aarx];
      simde__m256i *ulch256 = (simde__m256i*)chFext[aatx * nb_rx_ant + aarx];
      simde__m256i *rxF_comp256 = (simde__m256i*)rxFext_comp[aatx];
      for (int i = 0; i < (length>>3) + ((length&7)?1:0); i++) 
      {
        xmmp0  = simde_mm256_madd_epi16(ulch256[i], rxF256[i]);
        // xmmp0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
        xmmp1  = simde_mm256_shuffle_epi8(ulch256[i], complex_shuffle256);
        xmmp1  = simde_mm256_sign_epi16(xmmp1, conj256);
        xmmp1  = simde_mm256_madd_epi16(xmmp1, rxF256[i]);
        // xmmp1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
        xmmp0  = simde_mm256_srai_epi32(xmmp0, output_shift);
        xmmp1  = simde_mm256_srai_epi32(xmmp1, output_shift);
        xmmp2  = simde_mm256_unpacklo_epi32(xmmp0, xmmp1);
        xmmp3  = simde_mm256_unpackhi_epi32(xmmp0, xmmp1);
        xmmp4  = simde_mm256_packs_epi32(xmmp2, xmmp3);

        if (aarx == 0) 
          *rxF_comp256 = xmmp4;
        else
          *rxF_comp256 = simde_mm256_adds_epi16(*rxF_comp256, xmmp4);

        rxF_comp256++;
      }
#endif
    }
  }
  c16_t *rho0 = (c16_t *)rho[1];
  c16_t *rho1 = (c16_t *)rho[2];
  c16_t *llr_0 = (c16_t *)&llr[0][pusch_vars->llr_offset[symbol]];
  c16_t *llr_1 = (c16_t *)&llr[1][pusch_vars->llr_offset[symbol]];

  nr_ulsch_qpsk_qpsk((c16_t *)rxFext_comp[0], (c16_t *)rxFext_comp[1], llr_0, rho0, length);
  nr_ulsch_qpsk_qpsk((c16_t *)rxFext_comp[1], (c16_t *)rxFext_comp[0], llr_1, rho1, length);

  nr_ulsch_shift_llr(pusch_vars->llr_layers, length, pusch_vars->llr_offset[symbol] >> 1, 2, 4);

}


void inner_rx_16qam_2layer (NR_DL_FRAME_PARMS *frame_parms,
                            NR_gNB_PUSCH *pusch_vars, 
                            nfapi_nr_pusch_pdu_t *rel15_ul,
                            int **rxF, 
                            int **ul_ch, 
                            int16_t **llr,
                            int nb_layer,
                            int nb_rx_ant, 
                            int soffset,
                            int length, 
                            int symbol,
                            int short nb_rb,
                            int dmrs_symbol_flag,
                            int output_shift)
{
  int add_shift = 0;
  if (length % 8)
    add_shift = 8 - length % 8;
  
  int32_t rxFext[nb_rx_ant][length + add_shift] __attribute__((aligned(32)));
  int32_t chFext[nb_layer*nb_rx_ant][length + add_shift] __attribute__((aligned(32)));
  for (int aarx = 0; aarx < nb_rx_ant; aarx++) 
  {
    for (int aatx = 0; aatx < nb_layer; aatx++) 
    {
      nr_ulsch_extract_rbs0((c16_t *)rxF[aarx],
                            pusch_vars->ul_ch_estimates[aatx * nb_rx_ant + aarx],
                            rxFext[aarx],
                            chFext[aatx * nb_rx_ant + aarx],
                            soffset+(symbol * frame_parms->ofdm_symbol_size),
                            pusch_vars->dmrs_symbol * frame_parms->ofdm_symbol_size,
                            aarx,
                            dmrs_symbol_flag, 
                            rel15_ul,
                            frame_parms);
    }
  }
  
  int32_t rho[nb_layer*nb_layer][length + add_shift] __attribute__((aligned(32)));
  int32_t rxFext_comp[nb_layer][length + add_shift] __attribute__((aligned(32)));
  int32_t ul_ch_mag[nb_layer][length + add_shift] __attribute__((aligned(32)));
  for (int aatx = 0; aatx < nb_layer; aatx++) 
  {
    for (int aarx = 0; aarx < nb_rx_ant; aarx++) 
    {
      for (int atx = 0; atx < nb_layer; atx++) 
      {
#ifdef USE_128BIT
        simde__m128i mmtmpD0, mmtmpD1, mmtmpD2, mmtmpD3;
        simde__m128i *rho128        = (simde__m128i *)rho[aatx*nb_layer+atx];
        simde__m128i *ul_ch128      = (simde__m128i *)chFext[aatx * nb_rx_ant + aarx];
        simde__m128i *ul_ch128_2    = (simde__m128i *)chFext[atx * nb_rx_ant + aarx];
        for (int i = 0; i < (length>>2)+((length&3)?1:0); i++) 
        {
          // multiply by conjugated channel
          mmtmpD0 = simde_mm_madd_epi16(ul_ch128[i], ul_ch128_2[i]);
          // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
          mmtmpD1 = simde_mm_shufflelo_epi16(ul_ch128[i], SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_sign_epi16(mmtmpD1, *(simde__m128i*)&conjugate[0]);
          mmtmpD1 = simde_mm_madd_epi16(mmtmpD1, ul_ch128_2[i]);
          // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
          mmtmpD0 = simde_mm_srai_epi32(mmtmpD0, output_shift);
          mmtmpD1 = simde_mm_srai_epi32(mmtmpD1, output_shift);
          mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0, mmtmpD1);
          mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0, mmtmpD1);
          if (aarx == 0)
            rho128[i] = simde_mm_packs_epi32(mmtmpD2, mmtmpD3);
          else
            rho128[i] = simde_mm_adds_epi16(rho128[i], simde_mm_packs_epi32(mmtmpD2, mmtmpD3));
        }
#else
        simde__m256i mmtmpD0, mmtmpD1, mmtmpD2, mmtmpD3;
        simde__m256i *rho256        = (simde__m256i *)rho[aatx*nb_layer+atx];
        simde__m256i *ul_ch256      = (simde__m256i *)chFext[aatx * nb_rx_ant + aarx];
        simde__m256i *ul_ch256_2    = (simde__m256i *)chFext[atx * nb_rx_ant + aarx];
        register simde__m256i complex_shuffle256 = simde_mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);
        register simde__m256i conj256 = simde_mm256_set_epi16(1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1);
        for (int i = 0; i < (length >> 3)+((length&7)?1:0); i++) 
        {
          // multiply by conjugated channel
          mmtmpD0 = simde_mm256_madd_epi16(ul_ch256[i], ul_ch256_2[i]);
          // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
          mmtmpD1 = simde_mm256_shuffle_epi8(ul_ch256[i], complex_shuffle256);
          mmtmpD1 = simde_mm256_sign_epi16(mmtmpD1, conj256);
          mmtmpD1 = simde_mm256_madd_epi16(mmtmpD1, ul_ch256_2[i]);
          // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
          mmtmpD0 = simde_mm256_srai_epi32(mmtmpD0, output_shift);
          mmtmpD1 = simde_mm256_srai_epi32(mmtmpD1, output_shift);
          mmtmpD2 = simde_mm256_unpacklo_epi32(mmtmpD0, mmtmpD1);
          mmtmpD3 = simde_mm256_unpackhi_epi32(mmtmpD0, mmtmpD1);
          if (aarx == 0)
            rho256[i] = simde_mm256_packs_epi32(mmtmpD2, mmtmpD3);
          else
            rho256[i] = simde_mm256_adds_epi16(rho256[i], simde_mm256_packs_epi32(mmtmpD2, mmtmpD3));
        }
#endif
      }
      // compensation
#ifdef USE_128BIT
      simde__m128i xmmp0, xmmp1, xmmp2, xmmp3, xmmp4;
      register simde__m128i complex_shuffle128 = simde_mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
      register simde__m128i conj128 = simde_mm_set_epi16(1, -1, 1, -1, 1, -1, 1, -1);
      register simde__m128i QAM_amp128  = simde_mm_set1_epi16(QAM16_n1);  // 2/sqrt(10)
      simde__m128i *rxF128  = (simde__m128i*)rxFext[aarx];
      simde__m128i *ulch128 = (simde__m128i*)chFext[aatx * nb_rx_ant + aarx];
      simde__m128i *rxF_comp128 = (simde__m128i*)rxFext_comp[aatx];
      simde__m128i *ul_ch_mag128 = (simde__m128i*)ul_ch_mag[aatx];
      for (int i = 0; i < (length>>2)+((length&3)?1:0); i++) 
      {
        xmmp0  = simde_mm_madd_epi16(ulch128[i], rxF128[i]);
        // xmmp0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
        xmmp1  = simde_mm_shuffle_epi8(ulch128[i], complex_shuffle128);
        xmmp1  = simde_mm_sign_epi16(xmmp1, conj128);
        xmmp1  = simde_mm_madd_epi16(xmmp1, rxF128[i]);
        // xmmp1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
        xmmp0  = simde_mm_srai_epi32(xmmp0, output_shift);
        xmmp1  = simde_mm_srai_epi32(xmmp1, output_shift);
        xmmp2  = simde_mm_unpacklo_epi32(xmmp0, xmmp1);
        xmmp3  = simde_mm_unpackhi_epi32(xmmp0, xmmp1);
        xmmp4  = simde_mm_packs_epi32(xmmp2, xmmp3);
 
        xmmp0 = simde_mm_madd_epi16(ulch128[i], ulch128[i]); // |h|^2
        xmmp0 = simde_mm_srai_epi32(xmmp0, output_shift); 
        xmmp0 = simde_mm_packs_epi32(xmmp0, xmmp0);
        xmmp1 = simde_mm_unpacklo_epi16(xmmp0, xmmp0);
        xmmp1 = simde_mm_mulhrs_epi16(xmmp1, QAM_amp128);

        if (aarx == 0) 
        {
          *rxF_comp128 = xmmp4;
          *ul_ch_mag128 = xmmp1;
        }
        else
        {
          *rxF_comp128 = simde_mm_adds_epi16(*rxF_comp128, xmmp4);
          *ul_ch_mag128 = simde_mm_adds_epi16(*ul_ch_mag128, xmmp1);
        }
        rxF_comp128++;
        ul_ch_mag128++;
      }
#else
      simde__m256i xmmp0, xmmp1, xmmp2, xmmp3, xmmp4;
      register simde__m256i complex_shuffle256 = simde_mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);
      register simde__m256i conj256 = simde_mm256_set_epi16(1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1);
      register simde__m256i QAM_amp256  = simde_mm256_set1_epi16(QAM16_n1);  // 2/sqrt(10)
      simde__m256i *rxF256  = (simde__m256i*)rxFext[aarx];
      simde__m256i *ulch256 = (simde__m256i*)chFext[aatx * nb_rx_ant + aarx];
      simde__m256i *rxF_comp256 = (simde__m256i*)rxFext_comp[aatx];
      simde__m256i *ul_ch_mag256 = (simde__m256i*)ul_ch_mag[aatx];
      for (int i = 0; i < (length>>3) + ((length&7)?1:0); i++) 
      {
        xmmp0  = simde_mm256_madd_epi16(ulch256[i], rxF256[i]);
        // xmmp0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
        xmmp1  = simde_mm256_shuffle_epi8(ulch256[i], complex_shuffle256);
        xmmp1  = simde_mm256_sign_epi16(xmmp1, conj256);
        xmmp1  = simde_mm256_madd_epi16(xmmp1, rxF256[i]);
        // xmmp1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
        xmmp0  = simde_mm256_srai_epi32(xmmp0, output_shift);
        xmmp1  = simde_mm256_srai_epi32(xmmp1, output_shift);
        xmmp2  = simde_mm256_unpacklo_epi32(xmmp0, xmmp1);
        xmmp3  = simde_mm256_unpackhi_epi32(xmmp0, xmmp1);
        xmmp4  = simde_mm256_packs_epi32(xmmp2, xmmp3);

        xmmp0 = simde_mm256_madd_epi16(ulch256[i], ulch256[i]); // |h|^2
        xmmp0 = simde_mm256_srai_epi32(xmmp0, output_shift); 
        xmmp0 = simde_mm256_packs_epi32(xmmp0, xmmp0);
        xmmp1 = simde_mm256_unpacklo_epi16(xmmp0, xmmp0);
        xmmp1 = simde_mm256_mulhrs_epi16(xmmp1, QAM_amp256);

        if (aarx == 0) 
        {
          *rxF_comp256 = xmmp4;
          *ul_ch_mag256 = xmmp1;
        }
        else
        {
          *rxF_comp256 = simde_mm256_adds_epi16(*rxF_comp256, xmmp4);
          *ul_ch_mag256 = simde_mm256_adds_epi16(*ul_ch_mag256, xmmp1);
        }
        rxF_comp256++;
        ul_ch_mag256++;
      }
#endif
    }
  }
  c16_t *rho0 = (c16_t *)rho[1];
  c16_t *rho1 = (c16_t *)rho[2];
  c16_t *llr_0 = (c16_t *)&llr[0][pusch_vars->llr_offset[symbol]];
  c16_t *llr_1 = (c16_t *)&llr[1][pusch_vars->llr_offset[symbol]];
  c16_t *ul_ch_mag0 = (c16_t *)ul_ch_mag[0];
  c16_t *ul_ch_mag1 = (c16_t *)ul_ch_mag[1];
  nr_ulsch_qam16_qam16((c16_t *)rxFext_comp[0], (c16_t *)rxFext_comp[1], ul_ch_mag0, ul_ch_mag1, llr_0, rho0, length);
  nr_ulsch_qam16_qam16((c16_t *)rxFext_comp[1], (c16_t *)rxFext_comp[0], ul_ch_mag1, ul_ch_mag0, llr_1, rho1, length);
}

void inner_rx_64qam_2layer (NR_DL_FRAME_PARMS *frame_parms,
                            NR_gNB_PUSCH *pusch_vars, 
                            nfapi_nr_pusch_pdu_t *rel15_ul,
                            int **rxF, 
                            int **ul_ch, 
                            int16_t **llr,
                            int nb_layer,
                            int nb_rx_ant, 
                            int soffset,
                            int length, 
                            int symbol,
                            int short nb_rb,
                            int dmrs_symbol_flag,
                            int output_shift)
{
  int add_shift = 0;
  if (length % 8)
    add_shift = 8 - length % 8;
  
  int32_t rxFext[nb_rx_ant][length + add_shift] __attribute__((aligned(32)));
  int32_t chFext[nb_layer*nb_rx_ant][length + add_shift] __attribute__((aligned(32)));
  for (int aarx = 0; aarx < nb_rx_ant; aarx++) 
  {
    for (int aatx = 0; aatx < nb_layer; aatx++) 
    {
      nr_ulsch_extract_rbs0((c16_t *)rxF[aarx],
                            pusch_vars->ul_ch_estimates[aatx * nb_rx_ant + aarx],
                            rxFext[aarx],
                            chFext[aatx * nb_rx_ant + aarx],
                            soffset+(symbol * frame_parms->ofdm_symbol_size),
                            pusch_vars->dmrs_symbol * frame_parms->ofdm_symbol_size,
                            aarx,
                            dmrs_symbol_flag, 
                            rel15_ul,
                            frame_parms);
    }
  }
  int32_t rho[nb_layer*nb_layer][length + add_shift] __attribute__((aligned(32)));
  int32_t rxFext_comp[nb_layer][length + add_shift] __attribute__((aligned(32)));
  int32_t ul_ch_mag[nb_layer][length + add_shift] __attribute__((aligned(32)));
  for (int aatx = 0; aatx < nb_layer; aatx++) 
  {
    for (int aarx = 0; aarx < nb_rx_ant; aarx++) 
    {
      for (int atx = 0; atx < nb_layer; atx++) 
      {
#ifdef USE_128BIT
        simde__m128i mmtmpD0, mmtmpD1, mmtmpD2, mmtmpD3;
        simde__m128i *rho128        = (simde__m128i *)rho[aatx*nb_layer+atx];
        simde__m128i *ul_ch128      = (simde__m128i *)chFext[aatx * nb_rx_ant + aarx];
        simde__m128i *ul_ch128_2    = (simde__m128i *)chFext[atx * nb_rx_ant + aarx];
        for (int i = 0; i < (length>>2)+((length&3)?1:0); i++) 
        {
          // multiply by conjugated channel
          mmtmpD0 = simde_mm_madd_epi16(ul_ch128[i], ul_ch128_2[i]);
          // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
          mmtmpD1 = simde_mm_shufflelo_epi16(ul_ch128[i], SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_sign_epi16(mmtmpD1, *(simde__m128i*)&conjugate[0]);
          mmtmpD1 = simde_mm_madd_epi16(mmtmpD1, ul_ch128_2[i]);
          // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
          mmtmpD0 = simde_mm_srai_epi32(mmtmpD0, output_shift);
          mmtmpD1 = simde_mm_srai_epi32(mmtmpD1, output_shift);
          mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0, mmtmpD1);
          mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0, mmtmpD1);
          if (aarx == 0)
            rho128[i] = simde_mm_packs_epi32(mmtmpD2, mmtmpD3);
          else
            rho128[i] = simde_mm_adds_epi16(rho128[i], simde_mm_packs_epi32(mmtmpD2, mmtmpD3));
        }
#else
        simde__m256i mmtmpD0, mmtmpD1, mmtmpD2, mmtmpD3;
        simde__m256i *rho256        = (simde__m256i *)rho[aatx*nb_layer+atx];
        simde__m256i *ul_ch256      = (simde__m256i *)chFext[aatx * nb_rx_ant + aarx];
        simde__m256i *ul_ch256_2    = (simde__m256i *)chFext[atx * nb_rx_ant + aarx];
        register simde__m256i complex_shuffle256 = simde_mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);
        register simde__m256i conj256 = simde_mm256_set_epi16(1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1);
        for (int i = 0; i < (length >> 3)+((length&7)?1:0); i++) 
        {
          // multiply by conjugated channel
          mmtmpD0 = simde_mm256_madd_epi16(ul_ch256[i], ul_ch256_2[i]);
          // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
          mmtmpD1 = simde_mm256_shuffle_epi8(ul_ch256[i], complex_shuffle256);
          mmtmpD1 = simde_mm256_sign_epi16(mmtmpD1, conj256);
          mmtmpD1 = simde_mm256_madd_epi16(mmtmpD1, ul_ch256_2[i]);
          // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
          mmtmpD0 = simde_mm256_srai_epi32(mmtmpD0, output_shift);
          mmtmpD1 = simde_mm256_srai_epi32(mmtmpD1, output_shift);
          mmtmpD2 = simde_mm256_unpacklo_epi32(mmtmpD0, mmtmpD1);
          mmtmpD3 = simde_mm256_unpackhi_epi32(mmtmpD0, mmtmpD1);
          if (aarx == 0)
            rho256[i] = simde_mm256_packs_epi32(mmtmpD2, mmtmpD3);
          else
            rho256[i] = simde_mm256_adds_epi16(rho256[i], simde_mm256_packs_epi32(mmtmpD2, mmtmpD3));
        }
#endif
      }
      // compensation
#ifdef USE_128BIT
      simde__m128i xmmp0, xmmp1, xmmp2, xmmp3, xmmp4;
      register simde__m128i complex_shuffle128 = simde_mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
      register simde__m128i conj128 = simde_mm_set_epi16(1, -1, 1, -1, 1, -1, 1, -1);
      register simde__m128i QAM_amp128  = simde_mm_set1_epi16(QAM64_n1);  // 2/sqrt(10)
      simde__m128i *rxF128  = (simde__m128i*)rxFext[aarx];
      simde__m128i *ulch128 = (simde__m128i*)chFext[aatx * nb_rx_ant + aarx];
      simde__m128i *rxF_comp128 = (simde__m128i*)rxFext_comp[aatx];
      simde__m128i *ul_ch_mag128 = (simde__m128i*)ul_ch_mag[aatx];
      for (int i = 0; i < (length>>2)+((length&3)?1:0); i++) 
      {
        xmmp0  = simde_mm_madd_epi16(ulch128[i], rxF128[i]);
        // xmmp0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
        xmmp1  = simde_mm_shuffle_epi8(ulch128[i], complex_shuffle128);
        xmmp1  = simde_mm_sign_epi16(xmmp1, conj128);
        xmmp1  = simde_mm_madd_epi16(xmmp1, rxF128[i]);
        // xmmp1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
        xmmp0  = simde_mm_srai_epi32(xmmp0, output_shift);
        xmmp1  = simde_mm_srai_epi32(xmmp1, output_shift);
        xmmp2  = simde_mm_unpacklo_epi32(xmmp0, xmmp1);
        xmmp3  = simde_mm_unpackhi_epi32(xmmp0, xmmp1);
        xmmp4  = simde_mm_packs_epi32(xmmp2, xmmp3);

        xmmp0 = simde_mm_madd_epi16(ulch128[i], ulch128[i]); // |h|^2
        xmmp0 = simde_mm_srai_epi32(xmmp0, output_shift); 
        xmmp0 = simde_mm_packs_epi32(xmmp0, xmmp0);
        xmmp1 = simde_mm_unpacklo_epi16(xmmp0, xmmp0);
        xmmp1 = simde_mm_mulhrs_epi16(xmmp1, QAM_amp128);

        if (aarx == 0) 
        {
          *rxF_comp128 = xmmp4;
          *ul_ch_mag128 = xmmp1;
        }
        else
        {
          *rxF_comp128 = simde_mm_adds_epi16(*rxF_comp128, xmmp4);
          *ul_ch_mag128 = simde_mm_adds_epi16(*ul_ch_mag128, xmmp1);
        }
        rxF_comp128++;
        ul_ch_mag128++;
      }
#else
      simde__m256i xmmp0, xmmp1, xmmp2, xmmp3, xmmp4;
      register simde__m256i complex_shuffle256 = simde_mm256_set_epi8(29,28,31,30,25,24,27,26,21,20,23,22,17,16,19,18,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);
      register simde__m256i conj256 = simde_mm256_set_epi16(1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1);
      register simde__m256i QAM_amp256  = simde_mm256_set1_epi16(QAM64_n1);  // 2/sqrt(10)
      simde__m256i *rxF256  = (simde__m256i*)rxFext[aarx];
      simde__m256i *ulch256 = (simde__m256i*)chFext[aatx * nb_rx_ant + aarx];
      simde__m256i *rxF_comp256 = (simde__m256i*)rxFext_comp[aatx];
      simde__m256i *ul_ch_mag256 = (simde__m256i*)ul_ch_mag[aatx];
      for (int i = 0; i < (length>>3) + ((length&7)?1:0); i++) 
      {
        xmmp0  = simde_mm256_madd_epi16(ulch256[i], rxF256[i]);
        // xmmp0 contains real part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
        xmmp1  = simde_mm256_shuffle_epi8(ulch256[i], complex_shuffle256);
        xmmp1  = simde_mm256_sign_epi16(xmmp1, conj256);
        xmmp1  = simde_mm256_madd_epi16(xmmp1, rxF256[i]);
        // xmmp1 contains imag part of 4 consecutive outputs (32-bit) of conj(H_m[i])*R_m[i]
        xmmp0  = simde_mm256_srai_epi32(xmmp0, output_shift);
        xmmp1  = simde_mm256_srai_epi32(xmmp1, output_shift);
        xmmp2  = simde_mm256_unpacklo_epi32(xmmp0, xmmp1);
        xmmp3  = simde_mm256_unpackhi_epi32(xmmp0, xmmp1);
        xmmp4  = simde_mm256_packs_epi32(xmmp2, xmmp3);

        xmmp0 = simde_mm256_madd_epi16(ulch256[i], ulch256[i]); // |h|^2
        xmmp0 = simde_mm256_srai_epi32(xmmp0, output_shift); 
        xmmp0 = simde_mm256_packs_epi32(xmmp0, xmmp0);
        xmmp1 = simde_mm256_unpacklo_epi16(xmmp0, xmmp0);
        xmmp1 = simde_mm256_mulhrs_epi16(xmmp1, QAM_amp256);

        if (aarx == 0) 
        {
          *rxF_comp256 = xmmp4;
          *ul_ch_mag256 = xmmp1;
        }
        else
        {
          *rxF_comp256 = simde_mm256_adds_epi16(*rxF_comp256, xmmp4);
          *ul_ch_mag256 = simde_mm256_adds_epi16(*ul_ch_mag256, xmmp1);
        }
        rxF_comp256++;
        ul_ch_mag256++;
      }
#endif
    }
  }
  c16_t *rho0 = (c16_t *)rho[1];
  c16_t *rho1 = (c16_t *)rho[2];
  c16_t *llr_0 = (c16_t *)&llr[0][pusch_vars->llr_offset[symbol]];
  c16_t *llr_1 = (c16_t *)&llr[1][pusch_vars->llr_offset[symbol]];
  c16_t *ul_ch_mag0 = (c16_t *)ul_ch_mag[0];
  c16_t *ul_ch_mag1 = (c16_t *)ul_ch_mag[1];
  nr_ulsch_qam64_qam64((c16_t *)rxFext_comp[0], (c16_t *)rxFext_comp[1], ul_ch_mag0, ul_ch_mag1, llr_0, rho0, length);
  nr_ulsch_qam64_qam64((c16_t *)rxFext_comp[1], (c16_t *)rxFext_comp[0], ul_ch_mag1, ul_ch_mag0, llr_1, rho1, length);
}

void nr_pusch_symbol_processing_noprecoding(void *arg)
{

  puschSymbolProc_t *rdata=(puschSymbolProc_t*)arg;

  PHY_VARS_gNB *gNB = rdata->gNB;
  NR_DL_FRAME_PARMS *frame_parms = rdata->frame_parms;
  nfapi_nr_pusch_pdu_t *rel15_ul = rdata->rel15_ul;
  int ulsch_id = rdata->ulsch_id;
  int slot = rdata->slot;
  NR_gNB_PUSCH *pusch_vars = &gNB->pusch_vars[ulsch_id];
  int16_t *s = rdata->s;
  for (int symbol = rdata->startSymbol; symbol < rdata->startSymbol+rdata->numSymbols; symbol++) 
  {
    int dmrs_symbol_flag = (rel15_ul->ul_dmrs_symb_pos >> symbol) & 0x01;
    int nb_re_pusch = gNB->pusch_vars[ulsch_id].ul_valid_re_per_slot[symbol];
    // this needs to be reworded for parrellization, we need a table which give dmrs symbol location
    // used for chennel estimate, they are being run in parallel!
    if (dmrs_symbol_flag == 1) 
    {
      if ((rel15_ul->ul_dmrs_symb_pos >> ((symbol + 1) % frame_parms->symbols_per_slot)) & 0x01)
        AssertFatal(1==0,"Double DMRS configuration is not yet supported\n");

      gNB->pusch_vars[ulsch_id].dmrs_symbol = symbol;
    }

    LOG_I(PHY,"symbol %d: nb_re_pusch %d, DMRS symbl used for Chest :%d \n", symbol, nb_re_pusch, gNB->pusch_vars[ulsch_id].dmrs_symbol);

    if (nb_re_pusch == 0) continue;
    if (rel15_ul->nrOfLayers == 1)
    {
      int16_t *llr = &rdata->llr[pusch_vars->llr_offset[symbol]];
      void (*inner_rx)(int *,int *,int16_t *,int,int,int);
      if      (rel15_ul->qam_mod_order == 2) inner_rx = inner_rx_qpsk;
      else if (rel15_ul->qam_mod_order == 4) inner_rx = inner_rx_16qam;
      else if (rel15_ul->qam_mod_order == 6) inner_rx = inner_rx_64qam;
      else if (rel15_ul->qam_mod_order == 8) inner_rx = inner_rx_256qam;
      else    AssertFatal(1==0,"rel15_ul->qam_mod_order %d, pusch_pdu->dmrs_config_type %d\n",
      rel15_ul->qam_mod_order,rel15_ul->dmrs_config_type);

      int soffset   = (slot&3)*frame_parms->symbols_per_slot*frame_parms->ofdm_symbol_size;
      int32_t rxFext[nb_re_pusch+8] __attribute__((aligned(32)));
      int32_t chFext[nb_re_pusch+8] __attribute__((aligned(32)));
      int16_t llr_temp[(nb_re_pusch*rel15_ul->qam_mod_order)+16] __attribute__((aligned(32)));
      for (int aa = 0; aa < frame_parms->nb_antennas_rx; aa++) 
      {
        nr_ulsch_extract_rbs0(gNB->common_vars.rxdataF[aa],
                              gNB->pusch_vars[ulsch_id].ul_ch_estimates[aa],
                              rxFext,
                              chFext,
                              soffset+(symbol * frame_parms->ofdm_symbol_size),
                              gNB->pusch_vars[ulsch_id].dmrs_symbol*frame_parms->ofdm_symbol_size,
                              aa,
                              dmrs_symbol_flag, 
                              rel15_ul,
                              frame_parms);
        // demodulation
        inner_rx(rxFext, chFext, llr_temp, aa, nb_re_pusch, gNB->pusch_vars[ulsch_id].log2_maxh);
      }

      // unscrambling
      simde__m64 *llr64 = (simde__m64 *) llr;
      for (int i=0;i<(nb_re_pusch*rel15_ul->qam_mod_order)>>2;i++) 
        llr64[i] = simde_mm_mullo_pi16(((simde__m64 *)llr_temp)[i],((simde__m64 *)s)[i]);
      
      s += nb_re_pusch * rel15_ul->qam_mod_order;
      llr += nb_re_pusch * rel15_ul->qam_mod_order;
    }
    else // MIMO for 2x2
    {
      int soffset   = (slot&3)*frame_parms->symbols_per_slot*frame_parms->ofdm_symbol_size;
      void (*inner_rx)(NR_DL_FRAME_PARMS *, 
                       NR_gNB_PUSCH *,
                       nfapi_nr_pusch_pdu_t *,
                       int32_t **, 
                       int32_t **, 
                       int16_t **, 
                       int32_t, 
                       int32_t, 
                       int32_t, 
                       int32_t, 
                       int32_t, 
                       int16_t, 
                       int32_t, 
                       int32_t);
      if      (rel15_ul->qam_mod_order == 2) inner_rx = inner_rx_qpsk_2layer;
      else if (rel15_ul->qam_mod_order == 4) inner_rx = inner_rx_16qam_2layer;
      else if (rel15_ul->qam_mod_order == 6) inner_rx = inner_rx_64qam_2layer;
      else    AssertFatal(1==0,"rel15_ul->qam_mod_order %d, pusch_pdu->dmrs_config_type %d\n",
      rel15_ul->qam_mod_order,rel15_ul->dmrs_config_type);

      inner_rx(frame_parms,
               pusch_vars, 
               rel15_ul,
               (int32_t**)gNB->common_vars.rxdataF, 
               gNB->pusch_vars[ulsch_id].ul_ch_estimates, 
               rdata->llr_layers,
               rel15_ul->nrOfLayers, 
               frame_parms->nb_antennas_rx, 
               soffset,
               nb_re_pusch, // length
               symbol, // symbol index
               rel15_ul->rb_size, // ofdm size
               dmrs_symbol_flag,
               gNB->pusch_vars[ulsch_id].log2_maxh);

      // layer de-mapping
      int16_t* llr_cw = &rdata->llr[pusch_vars->llr_offset[symbol] * rel15_ul->nrOfLayers];
      for (int i = 0; i < (nb_re_pusch); i++) 
        for (int l = 0; l < rel15_ul->nrOfLayers; l++) 
          for (int m = 0; m < rel15_ul->qam_mod_order; m++) 
            llr_cw[i*rel15_ul->nrOfLayers*rel15_ul->qam_mod_order+l*rel15_ul->qam_mod_order+m] = rdata->llr_layers[l][pusch_vars->llr_offset[symbol] + i*rel15_ul->qam_mod_order+m];

      // unscrambling
      simde__m64 *llr64 = (simde__m64 *) &rdata->llr[pusch_vars->llr_offset[symbol] * rel15_ul->nrOfLayers];
      for (int i = 0; i < (nb_re_pusch*rel15_ul->qam_mod_order*rel15_ul->nrOfLayers)>>2; i++) 
        llr64[i] = simde_mm_mullo_pi16(((simde__m64 *)llr64)[i], ((simde__m64 *)s)[i]);

      s += (nb_re_pusch*rel15_ul->qam_mod_order*rel15_ul->nrOfLayers);
    }
  }
}

//compute average channel_level on each (TX,RX) antenna pair
void nr_ulsch_channel_level(int **ul_ch_estimates_ext,
                            NR_DL_FRAME_PARMS *frame_parms,
                            int32_t *avg,
                            uint8_t symbol,
                            uint32_t len,
                            uint8_t nrOfLayers,
                            unsigned short nb_rb)
{


  short rb;
  unsigned char aatx, aarx;
  simde__m128i *ul_ch128, avg128U;

  int16_t x = factor2(len);
  int16_t y = (len)>>x;
  
  uint32_t nb_rb_0 = len/12 + ((len%12)?1:0);

  int off = ((nb_rb&1) == 1)? 4:0;

  for (aatx = 0; aatx < nrOfLayers; aatx++) {
    for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      //clear average level
      avg128U = simde_mm_setzero_si128();

      ul_ch128=(simde__m128i *)&ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];

      for (rb = 0; rb < nb_rb_0; rb++) {
        avg128U = simde_mm_add_epi32(avg128U, simde_mm_srai_epi32(simde_mm_madd_epi16(ul_ch128[0], ul_ch128[0]), x));
        avg128U = simde_mm_add_epi32(avg128U, simde_mm_srai_epi32(simde_mm_madd_epi16(ul_ch128[1], ul_ch128[1]), x));
        avg128U = simde_mm_add_epi32(avg128U, simde_mm_srai_epi32(simde_mm_madd_epi16(ul_ch128[2], ul_ch128[2]), x));
        ul_ch128+=3;
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

static simde__m128i a_mult_conjb(simde__m128i a, simde__m128i b, unsigned char output_shift)
{
  simde__m128i mmtmpD0 = simde_mm_madd_epi16(b, a);
  simde__m128i mmtmpD1 = simde_mm_shufflelo_epi16(b, SIMDE_MM_SHUFFLE(2, 3, 0, 1));
  mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2, 3, 0, 1));
  mmtmpD1 = simde_mm_sign_epi16(mmtmpD1, *(simde__m128i *)&conjugate[0]);
  mmtmpD1 = simde_mm_madd_epi16(mmtmpD1, a);
  mmtmpD0 = simde_mm_srai_epi32(mmtmpD0, output_shift);
  mmtmpD1 = simde_mm_srai_epi32(mmtmpD1, output_shift);
  simde__m128i mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0, mmtmpD1);
  simde__m128i mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0, mmtmpD1);
  return simde_mm_packs_epi32(mmtmpD2, mmtmpD3);
}

//==============================================================================================
// Pre-processing for LLR computation
//==============================================================================================
void nr_ulsch_channel_compensation(int **rxdataF_ext,
                                   int **ul_ch_estimates_ext,
                                   int **ul_ch_mag,
                                   int **ul_ch_magb,
                                   int **ul_ch_magc,
                                   int **rxdataF_comp,
                                   int ***rho,
                                   NR_DL_FRAME_PARMS *frame_parms,
                                   unsigned char symbol,
                                   int length,
                                   uint8_t is_dmrs_symbol,
                                   unsigned char mod_order,
                                   uint8_t  nrOfLayers,
                                   unsigned short nb_rb,
                                   unsigned char output_shift) {

  int off = ((nb_rb&1) == 1)? 4:0;

#ifdef DEBUG_CH_COMP
  int16_t *rxF, *ul_ch;
  int prnt_idx;
  for (int nl=0; nl<nrOfLayers; nl++) {
    for (int aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      rxF = (int16_t *) &rxdataF_ext[aarx][symbol * (off + (nb_rb * 12))];
      ul_ch = (int16_t *) &ul_ch_estimates_ext[nl * frame_parms->nb_antennas_rx + aarx][symbol * (off + (nb_rb * 12))];

      printf("--------symbol = %d, mod_order = %d, output_shift = %d, layer %i, antenna rx = %d -----------\n",
             symbol, mod_order, output_shift, nl, aarx);
      printf("----------------Before compensation------------------\n");

      for (prnt_idx = 0; prnt_idx < 12 * 5 * 2; prnt_idx += 2) {
        printf("rxF[%d] = (%d,%d)\n", prnt_idx >> 1, rxF[prnt_idx], rxF[prnt_idx + 1]);
        printf("ul_ch[%d] = (%d,%d)\n", prnt_idx >> 1, ul_ch[prnt_idx], ul_ch[prnt_idx + 1]);
      }
    }
  }
#endif

#ifdef DEBUG_CH_MAG
  int16_t *ch_mag;
  int print_idx;


  for (int ant=0; ant<frame_parms->nb_antennas_rx; ant++) {
    ch_mag   = (int16_t *)&ul_ch_mag[ant][symbol*(off+(nb_rb*12))];

    printf("--------------------symbol = %d, mod_order = %d-----------------------\n", symbol, mod_order);
    printf("----------------Before computation------------------\n");

    for (print_idx=0;print_idx<5;print_idx++){

      printf("ch_mag[%d] = %d\n", print_idx, ch_mag[print_idx]);

    }
  }

#endif


  unsigned short rb;
  unsigned char aatx,aarx;
  simde__m128i *ul_ch128,*ul_ch128_2,*ul_ch_mag128,*ul_ch_mag128b,*ul_ch_mag128c,*rxdataF128,*rxdataF_comp128,*rho128;
  simde__m128i mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3,QAM_amp128={0},QAM_amp128b={0},QAM_amp128c={0};
  QAM_amp128b = simde_mm_setzero_si128();

  uint32_t nb_rb_0 = length/12 + ((length%12)?1:0);
  for (aatx=0; aatx<nrOfLayers; aatx++) {
    if (mod_order == 4) {
      QAM_amp128 = simde_mm_set1_epi16(QAM16_n1);  // 2/sqrt(10)
      QAM_amp128b = simde_mm_setzero_si128();
      QAM_amp128c = simde_mm_setzero_si128();
    } 
    else if (mod_order == 6) {
      QAM_amp128  = simde_mm_set1_epi16(QAM64_n1); //
      QAM_amp128b = simde_mm_set1_epi16(QAM64_n2);
      QAM_amp128c = simde_mm_setzero_si128();
    }
    else if (mod_order == 8) {
      QAM_amp128  = simde_mm_set1_epi16(QAM256_n1); //
      QAM_amp128b = simde_mm_set1_epi16(QAM256_n2);
      QAM_amp128c = simde_mm_set1_epi16(QAM256_n3);
    }

    //    printf("comp: rxdataF_comp %p, symbol %d\n",rxdataF_comp[0],symbol);

    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++)  {
      ul_ch128          = (simde__m128i *)&ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];
      ul_ch_mag128      = (simde__m128i *)&ul_ch_mag[aatx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];
      ul_ch_mag128b     = (simde__m128i *)&ul_ch_magb[aatx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];
      ul_ch_mag128c     = (simde__m128i *)&ul_ch_magc[aatx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];
      rxdataF128        = (simde__m128i *)&rxdataF_ext[aarx][symbol*(off+(nb_rb*12))];
      rxdataF_comp128   = (simde__m128i *)&rxdataF_comp[aatx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];


      for (rb=0; rb<nb_rb_0; rb++) {
        if (mod_order>2) {
          // get channel amplitude if not QPSK

          //print_shorts("ch:",(int16_t*)&ul_ch128[0]);

          mmtmpD0 = simde_mm_madd_epi16(ul_ch128[0],ul_ch128[0]);
          mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);

          mmtmpD1 = simde_mm_madd_epi16(ul_ch128[1],ul_ch128[1]);
          mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);

          mmtmpD0 = simde_mm_packs_epi32(mmtmpD0,mmtmpD1);

          // store channel magnitude here in a new field of ulsch

          ul_ch_mag128[0] = simde_mm_unpacklo_epi16(mmtmpD0,mmtmpD0);
          ul_ch_mag128b[0] = ul_ch_mag128[0];
          ul_ch_mag128c[0] = ul_ch_mag128[0];
          ul_ch_mag128[0] = simde_mm_mulhrs_epi16(ul_ch_mag128[0],QAM_amp128);
          ul_ch_mag128b[0] = simde_mm_mulhrs_epi16(ul_ch_mag128b[0],QAM_amp128b);
          ul_ch_mag128c[0] = simde_mm_mulhrs_epi16(ul_ch_mag128c[0],QAM_amp128c);
          // print_ints("ch: = ",(int32_t*)&mmtmpD0);
          // print_shorts("QAM_amp:",(int16_t*)&QAM_amp128);
          // print_shorts("mag:",(int16_t*)&ul_ch_mag128[0]);

          ul_ch_mag128[1]  = simde_mm_unpackhi_epi16(mmtmpD0,mmtmpD0);
          ul_ch_mag128b[1] = ul_ch_mag128[1];
          ul_ch_mag128c[1] = ul_ch_mag128[1];
          ul_ch_mag128[1]  = simde_mm_mulhrs_epi16(ul_ch_mag128[1],QAM_amp128);
          ul_ch_mag128b[1] = simde_mm_mulhrs_epi16(ul_ch_mag128b[1],QAM_amp128b);
          ul_ch_mag128c[1] = simde_mm_mulhrs_epi16(ul_ch_mag128c[1],QAM_amp128c);

          mmtmpD0 = simde_mm_madd_epi16(ul_ch128[2],ul_ch128[2]);
          mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
          mmtmpD1 = simde_mm_packs_epi32(mmtmpD0,mmtmpD0);

          ul_ch_mag128[2]  = simde_mm_unpacklo_epi16(mmtmpD1,mmtmpD1);
          ul_ch_mag128b[2] = ul_ch_mag128[2];
          ul_ch_mag128c[2] = ul_ch_mag128[2];

          ul_ch_mag128[2]  = simde_mm_mulhrs_epi16(ul_ch_mag128[2],QAM_amp128);
          ul_ch_mag128b[2] = simde_mm_mulhrs_epi16(ul_ch_mag128b[2],QAM_amp128b);
          ul_ch_mag128c[2] = simde_mm_mulhrs_epi16(ul_ch_mag128c[2],QAM_amp128c);
        }

        // Multiply received data by conjugated channel
        rxdataF_comp128[0] = a_mult_conjb(rxdataF128[0], ul_ch128[0], output_shift);
        rxdataF_comp128[1] = a_mult_conjb(rxdataF128[1], ul_ch128[1], output_shift);
        rxdataF_comp128[2] = a_mult_conjb(rxdataF128[2], ul_ch128[2], output_shift);

        ul_ch128 += 3;
        ul_ch_mag128 += 3;
        ul_ch_mag128b += 3;
        ul_ch_mag128c += 3;
        rxdataF128 += 3;
        rxdataF_comp128 += 3;
      }
    }
  }

  if (rho) {
    //we compute the Tx correlation matrix for each Rx antenna
    //As an example the 2x2 MIMO case requires
    //rho[aarx][nb_aatx*nb_aatx] = [cov(H_aarx_0,H_aarx_0) cov(H_aarx_0,H_aarx_1)
    //                              cov(H_aarx_1,H_aarx_0) cov(H_aarx_1,H_aarx_1)], aarx=0,...,nb_antennas_rx-1

    int avg_rho_re[frame_parms->nb_antennas_rx][nrOfLayers*nrOfLayers];
    int avg_rho_im[frame_parms->nb_antennas_rx][nrOfLayers*nrOfLayers];

    for (aarx=0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      for (aatx=0; aatx < nrOfLayers; aatx++) {
        for (int atx=0; atx< nrOfLayers; atx++) {

          avg_rho_re[aarx][aatx*nrOfLayers+atx] = 0;
          avg_rho_im[aarx][aatx*nrOfLayers+atx] = 0;
          rho128        = (simde__m128i *)&rho[aarx][aatx*nrOfLayers+atx][symbol*(off+(nb_rb*12))];
          ul_ch128      = (simde__m128i *)&ul_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];
          ul_ch128_2    = (simde__m128i *)&ul_ch_estimates_ext[atx*frame_parms->nb_antennas_rx+aarx][symbol*(off+(nb_rb*12))];

          for (rb=0; rb<nb_rb_0; rb++) {
            // multiply by conjugated channel
            mmtmpD0 = simde_mm_madd_epi16(ul_ch128[0],ul_ch128_2[0]);
            //  print_ints("re",&mmtmpD0);

            // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
            mmtmpD1 = simde_mm_shufflelo_epi16(ul_ch128[0], SIMDE_MM_SHUFFLE(2,3,0,1));
            mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
            mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i*)&conjugate[0]);
            //  print_ints("im",&mmtmpD1);
            mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,ul_ch128_2[0]);
            // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
            mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
            //  print_ints("re(shift)",&mmtmpD0);
            mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
            //  print_ints("im(shift)",&mmtmpD1);
            mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
            mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
            //        print_ints("c0",&mmtmpD2);
            //  print_ints("c1",&mmtmpD3);
            rho128[0] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);

            //print_shorts("rx:",ul_ch128_2);
            //print_shorts("ch:",ul_ch128);
            //print_shorts("pack:",rho128);

            avg_rho_re[aarx][aatx*nrOfLayers+atx] +=(((int16_t*)&rho128[0])[0]+
              ((int16_t*)&rho128[0])[2] +
              ((int16_t*)&rho128[0])[4] +
              ((int16_t*)&rho128[0])[6])/16;//

            avg_rho_im[aarx][aatx*nrOfLayers+atx] +=(((int16_t*)&rho128[0])[1]+
              ((int16_t*)&rho128[0])[3] +
              ((int16_t*)&rho128[0])[5] +
              ((int16_t*)&rho128[0])[7])/16;//
            // multiply by conjugated channel
            mmtmpD0 = simde_mm_madd_epi16(ul_ch128[1],ul_ch128_2[1]);
            // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
            mmtmpD1 = simde_mm_shufflelo_epi16(ul_ch128[1], SIMDE_MM_SHUFFLE(2,3,0,1));
            mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
            mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i*)conjugate);
            mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,ul_ch128_2[1]);
            // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
            mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
            mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
            mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
            mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
            rho128[1] =simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
            //print_shorts("rx:",ul_ch128_2+1);
            //print_shorts("ch:",ul_ch128+1);
            //print_shorts("pack:",rho128+1);

            // multiply by conjugated channel
            avg_rho_re[aarx][aatx*nrOfLayers+atx] +=(((int16_t*)&rho128[1])[0]+
              ((int16_t*)&rho128[1])[2] +
              ((int16_t*)&rho128[1])[4] +
              ((int16_t*)&rho128[1])[6])/16;

            avg_rho_im[aarx][aatx*nrOfLayers+atx] +=(((int16_t*)&rho128[1])[1]+
              ((int16_t*)&rho128[1])[3] +
              ((int16_t*)&rho128[1])[5] +
              ((int16_t*)&rho128[1])[7])/16;

            mmtmpD0 = simde_mm_madd_epi16(ul_ch128[2],ul_ch128_2[2]);
            // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
            mmtmpD1 = simde_mm_shufflelo_epi16(ul_ch128[2], SIMDE_MM_SHUFFLE(2,3,0,1));
            mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
            mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i*)conjugate);
            mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,ul_ch128_2[2]);
            // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
            mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
            mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
            mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
            mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

            rho128[2] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
            //print_shorts("rx:",ul_ch128_2+2);
            //print_shorts("ch:",ul_ch128+2);
            //print_shorts("pack:",rho128+2);
            avg_rho_re[aarx][aatx*nrOfLayers+atx] +=(((int16_t*)&rho128[2])[0]+
              ((int16_t*)&rho128[2])[2] +
              ((int16_t*)&rho128[2])[4] +
              ((int16_t*)&rho128[2])[6])/16;

            avg_rho_im[aarx][aatx*nrOfLayers+atx] +=(((int16_t*)&rho128[2])[1]+
              ((int16_t*)&rho128[2])[3] +
              ((int16_t*)&rho128[2])[5] +
              ((int16_t*)&rho128[2])[7])/16;

            ul_ch128+=3;
            ul_ch128_2+=3;
            rho128+=3;
          }
          if (is_dmrs_symbol==1) {
            //measurements->rx_correlation[0][0][aarx] = signal_energy(&rho[aarx][aatx*nb_aatx+atx][symbol*nb_rb*12],rb*12);
            avg_rho_re[aarx][aatx*nrOfLayers+atx] = 16*avg_rho_re[aarx][aatx*nrOfLayers+atx]/(nb_rb*12);
            avg_rho_im[aarx][aatx*nrOfLayers+atx] = 16*avg_rho_im[aarx][aatx*nrOfLayers+atx]/(nb_rb*12);
            //printf("rho[rx]%d tx%d tx%d = Re: %d Im: %d\n",aarx, aatx,atx, avg_rho_re[aarx][aatx*nb_aatx+atx], avg_rho_im[aarx][aatx*nb_aatx+atx]);
          }
        }
      }
    }
  }

  simde_mm_empty();
  simde_m_empty();


#ifdef DEBUG_CH_COMP
  for (int nl2=0; nl2<nrOfLayers; nl2++) {
    for (int aarx2=0; aarx2<frame_parms->nb_antennas_rx; aarx2++) {
      rxF   = (int16_t *)&rxdataF_comp[nl2*frame_parms->nb_antennas_rx+aarx2][(symbol*(off+(nb_rb*12)))];

      printf("--------After compansation, layer %i, antenna rx %i----------\n", nl2, aarx2);

      for (prnt_idx=0;prnt_idx<12*5*2;prnt_idx+=2){
        printf("rxF[%d] = (%d,%d)\n", prnt_idx>>1, rxF[prnt_idx],rxF[prnt_idx+1]);
      }
    }
  }
#endif

#ifdef DEBUG_CH_MAG


  for (int ant=0; ant<frame_parms->nb_antennas_rx; ant++) {
    ch_mag   = (int16_t *)&ul_ch_mag[ant][(symbol*(off+(nb_rb*12)))];

    printf("----------------After computation------------------\n");

    for (print_idx=0;print_idx<12*5*2;print_idx+=2){

      printf("ch_mag[%d] = (%d,%d)\n", print_idx>>1, ch_mag[print_idx],ch_mag[print_idx+1]);

    }
  }

#endif

}

void nr_ulsch_detection_mrc(NR_DL_FRAME_PARMS *frame_parms,
                int32_t **rxdataF_comp,
                int32_t **ul_ch_mag,
                int32_t **ul_ch_magb,
                int32_t **ul_ch_magc,
                int32_t ***rho,
                uint8_t  nrOfLayers,
                uint8_t symbol,
                uint16_t nb_rb,
                int length) {
  int n_rx = frame_parms->nb_antennas_rx;
  simde__m128i *rxdataF_comp128[2],*ul_ch_mag128[2],*ul_ch_mag128b[2],*ul_ch_mag128c[2];
  int32_t i;
  uint32_t nb_rb_0 = length/12 + ((length%12)?1:0);

  int off = ((nb_rb&1) == 1)? 4:0;

  if (n_rx > 1) {

    int nb_re = nb_rb * 12;

    for (int aatx = 0; aatx < nrOfLayers; aatx++) {

      rxdataF_comp128[0]   = (simde__m128i *)&rxdataF_comp[aatx*frame_parms->nb_antennas_rx][(symbol*(nb_re + off))];
      ul_ch_mag128[0]      = (simde__m128i *)&ul_ch_mag[aatx*frame_parms->nb_antennas_rx][(symbol*(nb_re + off))];
      ul_ch_mag128b[0]     = (simde__m128i *)&ul_ch_magb[aatx*frame_parms->nb_antennas_rx][(symbol*(nb_re + off))];
      ul_ch_mag128c[0]     = (simde__m128i *)&ul_ch_magc[aatx*frame_parms->nb_antennas_rx][(symbol*(nb_re + off))];

      for (int aa=1;aa < n_rx;aa++) {
        rxdataF_comp128[1]   = (simde__m128i *)&rxdataF_comp[aatx*frame_parms->nb_antennas_rx+aa][(symbol*(nb_re + off))];
        ul_ch_mag128[1]      = (simde__m128i *)&ul_ch_mag[aatx*frame_parms->nb_antennas_rx+aa][(symbol*(nb_re + off))];
        ul_ch_mag128b[1]     = (simde__m128i *)&ul_ch_magb[aatx*frame_parms->nb_antennas_rx+aa][(symbol*(nb_re + off))];
        ul_ch_mag128c[1]     = (simde__m128i *)&ul_ch_magc[aatx*frame_parms->nb_antennas_rx+aa][(symbol*(nb_re + off))];

        // MRC on each re of rb, both on MF output and magnitude (for 16QAM/64QAM llr computation)
        for (i=0; i<nb_rb_0*3; i++) {
            rxdataF_comp128[0][i] = simde_mm_adds_epi16(rxdataF_comp128[0][i],rxdataF_comp128[1][i]);
            ul_ch_mag128[0][i]    = simde_mm_adds_epi16(ul_ch_mag128[0][i],ul_ch_mag128[1][i]);
            ul_ch_mag128b[0][i]   = simde_mm_adds_epi16(ul_ch_mag128b[0][i],ul_ch_mag128b[1][i]);
            ul_ch_mag128c[0][i]   = simde_mm_adds_epi16(ul_ch_mag128c[0][i],ul_ch_mag128c[1][i]);
            //rxdataF_comp128[0][i] = _mm_add_epi16(rxdataF_comp128_0[i],(*(__m128i *)&jitterc[0]));
        }
      }

      if (rho) {
        simde__m128i *rho128[2];
        for (int aatx2 = 0; aatx2 < nrOfLayers; aatx2++) {
          rho128[0] = (simde__m128i *) &rho[0][aatx * nrOfLayers + aatx2][(symbol * (nb_re + off))];
          for (int aa = 1; aa < n_rx; aa++) {
            rho128[1] = (simde__m128i *) &rho[aa][aatx * nrOfLayers + aatx2][(symbol * (nb_re + off))];
            for (i = 0; i < nb_rb_0 * 3; i++) {
              rho128[0][i] = simde_mm_adds_epi16(rho128[0][i], rho128[1][i]);
            }
          }
        }
      }

    }
  }
}

/* Zero Forcing Rx function: nr_det_HhH()
 *
 *
 * */
void nr_ulsch_det_HhH(int32_t *after_mf_00,//a
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

/* Zero Forcing Rx function: nr_inv_comp_muli
 * Complex number multi: z = x*y
 *                         = (x_re*y_re - x_im*y_im) + j(x_im*y_re + x_re*y_im)
 * */
simde__m128i nr_ulsch_inv_comp_muli(simde__m128i input_x,
                         simde__m128i input_y)
{
  int16_t nr_conjug2[8]__attribute__((aligned(16))) = {1,-1,1,-1,1,-1,1,-1} ;

  simde__m128i xy_re_128, xy_im_128;
  simde__m128i output_z, tmp_z0, tmp_z1;

  // complex multiplication (x_re + jx_im)*(y_re + jy_im) = (x_re*y_re - x_im*y_im) + j(x_im*y_re + x_re*y_im)

  // the real part
  xy_re_128 = simde_mm_sign_epi16(input_x,*(simde__m128i*)&nr_conjug2[0]);
  xy_re_128 = simde_mm_madd_epi16(xy_re_128,input_y); //Re: (x_re*y_re - x_im*y_im)

  // the imag part
  xy_im_128 = simde_mm_shufflelo_epi16(input_x, SIMDE_MM_SHUFFLE(2,3,0,1));//permutes IQs for the low 64 bits as [I_a0 Q_a1 I_a2 Q_a3]_64bits to [Q_a1 I_a0 Q_a3 I_a2]_64bits
  xy_im_128 = simde_mm_shufflehi_epi16(xy_im_128, SIMDE_MM_SHUFFLE(2,3,0,1));//permutes IQs for the high 64 bits as [I_a0 Q_a1 I_a2 Q_a3]_64bits to [Q_a1 I_a0 Q_a3 I_a2]_64bits
  xy_im_128 = simde_mm_madd_epi16(xy_im_128,input_y);//Im: (x_im*y_re + x_re*y_im)

  //convert back to Q15 before packing
  xy_re_128 = simde_mm_srai_epi32(xy_re_128,4);//(2^15/64*2*16)
  xy_im_128 = simde_mm_srai_epi32(xy_im_128,4);

  tmp_z0  = simde_mm_unpacklo_epi32(xy_re_128,xy_im_128);
  //print_ints("unpack lo:",&tmp_z0[0]);
  tmp_z1  = simde_mm_unpackhi_epi32(xy_re_128,xy_im_128);
  //print_ints("unpack hi:",&tmp_z1[0]);
  output_z = simde_mm_packs_epi32(tmp_z0,tmp_z1);

  simde_mm_empty();
  simde_m_empty();
  return(output_z);
}

/* Zero Forcing Rx function: nr_conjch0_mult_ch1()
 *
 *
 * */
void nr_ulsch_conjch0_mult_ch1(int *ch0,
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
simde__m128i nr_ulsch_comp_muli_sum(simde__m128i input_x,
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
void nr_ulsch_construct_HhH_elements(int *conjch00_ch00,
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

/*
 * MMSE Rx function: nr_ulsch_mmse_2layers()
 */
uint8_t nr_ulsch_mmse_2layers(NR_DL_FRAME_PARMS *frame_parms,
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
                              uint32_t noise_var)
{
  int *ch00, *ch01, *ch10, *ch11;
  int *ch20, *ch30, *ch21, *ch31;
  uint32_t nb_rb_0 = length/12 + ((length%12)?1:0);

  int off = ((nb_rb&1) == 1)? 4:0;

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
      ch00 = (int *)&ul_ch_estimates_ext[0][symbol*(off+nb_rb*12)];
      ch01 = (int *)&ul_ch_estimates_ext[2][symbol*(off+nb_rb*12)];
      ch10 = (int *)&ul_ch_estimates_ext[1][symbol*(off+nb_rb*12)];
      ch11 = (int *)&ul_ch_estimates_ext[3][symbol*(off+nb_rb*12)];
      ch20 = NULL;
      ch21 = NULL;
      ch30 = NULL;
      ch31 = NULL;
      break;

    case 4://
      ch00 = (int *)&ul_ch_estimates_ext[0][symbol*(off+nb_rb*12)];
      ch01 = (int *)&ul_ch_estimates_ext[4][symbol*(off+nb_rb*12)];
      ch10 = (int *)&ul_ch_estimates_ext[1][symbol*(off+nb_rb*12)];
      ch11 = (int *)&ul_ch_estimates_ext[5][symbol*(off+nb_rb*12)];
      ch20 = (int *)&ul_ch_estimates_ext[2][symbol*(off+nb_rb*12)];
      ch21 = (int *)&ul_ch_estimates_ext[6][symbol*(off+nb_rb*12)];
      ch30 = (int *)&ul_ch_estimates_ext[3][symbol*(off+nb_rb*12)];
      ch31 = (int *)&ul_ch_estimates_ext[7][symbol*(off+nb_rb*12)];
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

  simde__m128i *rxdataF_comp128_0 = (simde__m128i *)&rxdataF_comp[0][symbol * (off + nb_rb * 12)]; // aatx=0 @ aarx =0
  simde__m128i *rxdataF_comp128_1 = (simde__m128i *)&rxdataF_comp[n_rx][symbol * (off + nb_rb * 12)]; // aatx=1 @ aarx =0

  simde__m128i *after_mf_a_128 = (simde__m128i *)af_mf_00;
  simde__m128i *after_mf_b_128 = (simde__m128i *)af_mf_01;
  simde__m128i *after_mf_c_128 = (simde__m128i *)af_mf_10;
  simde__m128i *after_mf_d_128 = (simde__m128i *)af_mf_11;


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
      QAM_amp128 = simde_mm_set1_epi16(QAM256_n1);
      QAM_amp128b = simde_mm_set1_epi16(QAM256_n2);
      QAM_amp128c = simde_mm_set1_epi16(QAM256_n3);
    }
    ul_ch_mag128_0 = (simde__m128i *)&ul_ch_mag[0][symbol * (off + nb_rb * 12)];
    ul_ch_mag128b_0 = (simde__m128i *)&ul_ch_magb[0][symbol * (off + nb_rb * 12)];
    ul_ch_mag128c_0 = (simde__m128i *)&ul_ch_magc[0][symbol * (off + nb_rb * 12)];
    ul_ch_mag128_1 = (simde__m128i *)&ul_ch_mag[frame_parms->nb_antennas_rx][symbol * (off + nb_rb * 12)];
    ul_ch_mag128b_1 = (simde__m128i *)&ul_ch_magb[frame_parms->nb_antennas_rx][symbol * (off + nb_rb * 12)];
    ul_ch_mag128c_1 = (simde__m128i *)&ul_ch_magc[frame_parms->nb_antennas_rx][symbol * (off + nb_rb * 12)];
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

//==============================================================================================

/* Main Function */
void nr_rx_pusch(PHY_VARS_gNB *gNB,
                 uint8_t ulsch_id,
                 uint32_t frame,
                 uint8_t slot,
                 unsigned char harq_pid)
{

  uint8_t aarx, aatx;
  uint32_t nb_re_pusch, bwp_start_subcarrier;
  int avgs = 0;

  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  NR_gNB_ULSCH_t *ulsch = &gNB->ulsch[ulsch_id];
  nfapi_nr_pusch_pdu_t *rel15_ul = &ulsch->harq_process->ulsch_pdu;
  int avg[frame_parms->nb_antennas_rx*rel15_ul->nrOfLayers];

  NR_gNB_PUSCH *pusch_vars = &gNB->pusch_vars[ulsch_id];
  pusch_vars->dmrs_symbol = INVALID_VALUE;
  pusch_vars->cl_done = 0;

  bwp_start_subcarrier = ((rel15_ul->rb_start + rel15_ul->bwp_start)*NR_NB_SC_PER_RB + frame_parms->first_carrier_offset) % frame_parms->ofdm_symbol_size;
  LOG_D(PHY,"pusch %d.%d : bwp_start_subcarrier %d, rb_start %d, first_carrier_offset %d\n", frame,slot,bwp_start_subcarrier, rel15_ul->rb_start, frame_parms->first_carrier_offset);
  LOG_D(PHY,"pusch %d.%d : ul_dmrs_symb_pos %x\n",frame,slot,rel15_ul->ul_dmrs_symb_pos);
  LOG_D(PHY,"ulsch RX %x : start_rb %d nb_rb %d mcs %d Nl %d Tpmi %d bwp_start %d start_sc %d start_symbol %d num_symbols %d cdmgrpsnodata %d num_dmrs %d dmrs_ports %d\n",
          rel15_ul->rnti,rel15_ul->rb_start,rel15_ul->rb_size,rel15_ul->mcs_index,
          rel15_ul->nrOfLayers,0,rel15_ul->bwp_start,0,rel15_ul->start_symbol_index,rel15_ul->nr_of_symbols,
          rel15_ul->num_dmrs_cdm_grps_no_data,rel15_ul->ul_dmrs_symb_pos,rel15_ul->dmrs_ports);
  //----------------------------------------------------------
  //--------------------- Channel estimation ---------------------
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

      nr_gnb_measurements(gNB, ulsch, pusch_vars, symbol, rel15_ul->nrOfLayers);
      allocCast2D(n0_subband_power,
                  unsigned int,
                  gNB->measurements.n0_subband_power,
                  frame_parms->nb_antennas_rx,
                  frame_parms->N_RB_UL,
                  false);
      for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
        if (symbol == rel15_ul->start_symbol_index) {
          pusch_vars->ulsch_power[aarx] = 0;
          pusch_vars->ulsch_noise_power[aarx] = 0;
        }
        for (aatx = 0; aatx < rel15_ul->nrOfLayers; aatx++) {
          pusch_vars->ulsch_power[aarx] += signal_energy_nodc(
              &pusch_vars->ul_ch_estimates[aatx * gNB->frame_parms.nb_antennas_rx + aarx][symbol * frame_parms->ofdm_symbol_size],
              rel15_ul->rb_size * 12);
        }
        for (int rb = 0; rb < rel15_ul->rb_size; rb++) {
          pusch_vars->ulsch_noise_power[aarx] +=
              n0_subband_power[aarx][rel15_ul->bwp_start + rel15_ul->rb_start + rb] / rel15_ul->rb_size;
        }
        LOG_D(PHY,
              "aa %d, bwp_start%d, rb_start %d, rb_size %d: ulsch_power %d, ulsch_noise_power %d\n",
              aarx,
              rel15_ul->bwp_start,
              rel15_ul->rb_start,
              rel15_ul->rb_size,
              pusch_vars->ulsch_power[aarx],
              pusch_vars->ulsch_noise_power[aarx]);
      }
    }
  }

  nvar /= (rel15_ul->nr_of_symbols * rel15_ul->nrOfLayers * frame_parms->nb_antennas_rx);

  if (gNB->chest_time == 1) { // averaging time domain channel estimates
    nr_chest_time_domain_avg(frame_parms,
                             pusch_vars->ul_ch_estimates,
                             rel15_ul->nr_of_symbols,
                             rel15_ul->start_symbol_index,
                             rel15_ul->ul_dmrs_symb_pos,
                             rel15_ul->rb_size);

    pusch_vars->dmrs_symbol =
        get_next_dmrs_symbol_in_slot(rel15_ul->ul_dmrs_symb_pos, rel15_ul->start_symbol_index, rel15_ul->nr_of_symbols);
  }
  stop_meas(&gNB->ulsch_channel_estimation_stats);

  int off = ((rel15_ul->rb_size&1) == 1)? 4:0;
  uint32_t rxdataF_ext_offset = 0;
  uint8_t shift_ch_ext = rel15_ul->nrOfLayers > 1 ? log2_approx(max_ch >> 11) : 0;

  // Flag to select the receiver: (true) Nonlinear ML receiver, (false) Linear MMSE receiver
  // By default, we are using the Nonlinear ML receiver, except
  //  - for 256QAM as Nonlinear ML receiver is not implemented for 256QAM
  //  - for 64QAM as Nonlinear ML receiver requires more processing time than MMSE, and many machines are not powerful enough
  bool ml_rx = true;
  if (rel15_ul->nrOfLayers != 2 || rel15_ul->qam_mod_order >= 6) {
    ml_rx = false;
  }

  int ad_shift = 0;
  if (rel15_ul->nrOfLayers == 1) {
    ad_shift = 1 + log2_approx(frame_parms->nb_antennas_rx >> 2);
  } else if (ml_rx == false) {
    ad_shift = -3; // For 2-layers, we are already doing a bit shift in the nr_ulsch_mmse_2layers() function, so we can use more bits
  }

  int num_re_total = 0;
  for(uint8_t symbol = rel15_ul->start_symbol_index; symbol < (rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols); symbol++) {
    uint8_t dmrs_symbol_flag = (rel15_ul->ul_dmrs_symb_pos >> symbol) & 0x01;
    if (dmrs_symbol_flag == 1) {
      if ((rel15_ul->ul_dmrs_symb_pos >> ((symbol + 1) % frame_parms->symbols_per_slot)) & 0x01)
        AssertFatal(1==0,"Double DMRS configuration is not yet supported\n");

      if (gNB->chest_time == 0) // Non averaging time domain channel estimates
        pusch_vars->dmrs_symbol = symbol;

      if (rel15_ul->dmrs_config_type == 0) {
        // if no data in dmrs cdm group is 1 only even REs have no data
        // if no data in dmrs cdm group is 2 both odd and even REs have no data
        nb_re_pusch = rel15_ul->rb_size *(12 - (rel15_ul->num_dmrs_cdm_grps_no_data*6));
      }
      else {
        nb_re_pusch = rel15_ul->rb_size *(12 - (rel15_ul->num_dmrs_cdm_grps_no_data*4));
      }
    } 
    else {
      nb_re_pusch = rel15_ul->rb_size * NR_NB_SC_PER_RB;
    }

    num_re_total += nb_re_pusch;
    pusch_vars->ul_valid_re_per_slot[symbol] = nb_re_pusch;
    LOG_D(PHY, "symbol %d: nb_re_pusch %d, DMRS symbl used for Chest :%d \n", symbol, nb_re_pusch, pusch_vars->dmrs_symbol);

    //----------------------------------------------------------
    //--------------------- RBs extraction ---------------------
    //----------------------------------------------------------
    if (nb_re_pusch > 0) {
      start_meas(&gNB->ulsch_rbs_extraction_stats);
      nr_ulsch_extract_rbs(gNB->common_vars.rxdataF, pusch_vars, slot, symbol, dmrs_symbol_flag, rel15_ul, frame_parms);
      stop_meas(&gNB->ulsch_rbs_extraction_stats);

      //----------------------------------------------------------
      //--------------------- Channel Scaling --------------------
      //----------------------------------------------------------
      nr_ulsch_scale_channel(pusch_vars->ul_ch_estimates_ext,
                             frame_parms,
                             ulsch,
                             symbol,
                             dmrs_symbol_flag,
                             nb_re_pusch,
                             rel15_ul->nrOfLayers,
                             rel15_ul->rb_size,
                             shift_ch_ext);

      if (pusch_vars->cl_done == 0) {
        nr_ulsch_channel_level(pusch_vars->ul_ch_estimates_ext,
                               frame_parms,
                               avg,
                               symbol,
                               nb_re_pusch,
                               rel15_ul->nrOfLayers,
                               rel15_ul->rb_size);

        avgs = 0;

        for (aatx=0;aatx<rel15_ul->nrOfLayers;aatx++)
          for (aarx=0;aarx<frame_parms->nb_antennas_rx;aarx++)
            avgs = cmax(avgs,avg[aatx*frame_parms->nb_antennas_rx+aarx]);

        pusch_vars->log2_maxh = (log2_approx(avgs) >> 1) + ad_shift;
        if (pusch_vars->log2_maxh < 0) {
          pusch_vars->log2_maxh = 0;
        }
        pusch_vars->cl_done = 1;
      }

      //----------------------------------------------------------
      //--------------------- Channel Compensation ---------------
      //----------------------------------------------------------
      start_meas(&gNB->ulsch_channel_compensation_stats);
      LOG_D(PHY, "Doing channel compensations log2_maxh %d, avgs %d (%d,%d)\n" ,pusch_vars->log2_maxh, avgs,avg[0], avg[1]);
      nr_ulsch_channel_compensation(pusch_vars->rxdataF_ext,
                                    pusch_vars->ul_ch_estimates_ext,
                                    pusch_vars->ul_ch_mag0,
                                    pusch_vars->ul_ch_magb0,
                                    pusch_vars->ul_ch_magc0,
                                    pusch_vars->rxdataF_comp,
                                    (rel15_ul->nrOfLayers > 1) ? pusch_vars->rho : NULL,
                                    frame_parms,
                                    symbol,
                                    nb_re_pusch,
                                    dmrs_symbol_flag,
                                    rel15_ul->qam_mod_order,
                                    rel15_ul->nrOfLayers,
                                    rel15_ul->rb_size,
                                    pusch_vars->log2_maxh);
      stop_meas(&gNB->ulsch_channel_compensation_stats);

      start_meas(&gNB->ulsch_mrc_stats);
      nr_ulsch_detection_mrc(frame_parms,
                             pusch_vars->rxdataF_comp,
                             pusch_vars->ul_ch_mag0,
                             pusch_vars->ul_ch_magb0,
                             pusch_vars->ul_ch_magc0,
                             (rel15_ul->nrOfLayers > 1) ? pusch_vars->rho : NULL,
                             rel15_ul->nrOfLayers,
                             symbol,
                             rel15_ul->rb_size,
                             nb_re_pusch);

      // Apply MMSE for 2 Tx layers
      if (ml_rx == false && rel15_ul->nrOfLayers == 2) {
        nr_ulsch_mmse_2layers(frame_parms,
                              pusch_vars->rxdataF_comp,
                              pusch_vars->ul_ch_mag0,
                              pusch_vars->ul_ch_magb0,
                              pusch_vars->ul_ch_magc0,
                              pusch_vars->ul_ch_estimates_ext,
                              rel15_ul->rb_size,
                              frame_parms->nb_antennas_rx,
                              rel15_ul->qam_mod_order,
                              pusch_vars->log2_maxh,
                              symbol,
                              nb_re_pusch,
                              nvar);
      }

      stop_meas(&gNB->ulsch_mrc_stats);

      if (rel15_ul->transform_precoding == transformPrecoder_enabled) {
        // For odd number of resource blocks need byte alignment to multiple of 8
        int nb_re_pusch2 = nb_re_pusch + (nb_re_pusch&7);

        // perform IDFT operation on the compensated rxdata if transform precoding is enabled
        nr_idft(&pusch_vars->rxdataF_comp[0][symbol * nb_re_pusch2], nb_re_pusch);
        LOG_D(PHY,"Transform precoding being done on data- symbol: %d, nb_re_pusch: %d\n", symbol, nb_re_pusch);
      }

      //----------------------------------------------------------
      //--------------------- PTRS Processing --------------------
      //----------------------------------------------------------
      /* In case PTRS is enabled then LLR will be calculated after PTRS symbols are processed *
      * otherwise LLR are calculated for each symbol based upon DMRS channel estimates only. */
      if (rel15_ul->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) {
        start_meas(&gNB->ulsch_ptrs_processing_stats);
        nr_pusch_ptrs_processing(gNB,
                                 frame_parms,
                                 rel15_ul,
                                 ulsch_id,
                                 slot,
                                 symbol,
                                 nb_re_pusch);
        stop_meas(&gNB->ulsch_ptrs_processing_stats);

        /*  Subtract total PTRS RE's in the symbol from PUSCH RE's */
        pusch_vars->ul_valid_re_per_slot[symbol] -= pusch_vars->ptrs_re_per_slot;
      }

      /*---------------------------------------------------------------------------------------------------- */
      /*--------------------  LLRs computation  -------------------------------------------------------------*/
      /*-----------------------------------------------------------------------------------------------------*/
      start_meas(&gNB->ulsch_llr_stats);
      if (ml_rx == false || rel15_ul->nrOfLayers == 1) {
        for (aatx=0; aatx < rel15_ul->nrOfLayers; aatx++) {
          nr_ulsch_compute_llr(&pusch_vars->rxdataF_comp[aatx * frame_parms->nb_antennas_rx][symbol * (off + rel15_ul->rb_size * NR_NB_SC_PER_RB)],
                               pusch_vars->ul_ch_mag0[aatx * frame_parms->nb_antennas_rx],
                               pusch_vars->ul_ch_magb0[aatx * frame_parms->nb_antennas_rx],
                               pusch_vars->ul_ch_magc0[aatx * frame_parms->nb_antennas_rx],
                               &pusch_vars->llr_layers[aatx][rxdataF_ext_offset * rel15_ul->qam_mod_order],
                               rel15_ul->rb_size,
                               pusch_vars->ul_valid_re_per_slot[symbol],
                               symbol,
                               rel15_ul->qam_mod_order);
        }
      } else {
        nr_ulsch_compute_ML_llr(pusch_vars->rxdataF_comp,
                                pusch_vars->ul_ch_mag0,
                                pusch_vars->rho,
                                pusch_vars->llr_layers,
                                frame_parms->nb_antennas_rx,
                                rel15_ul->rb_size,
                                nb_re_pusch,
                                symbol,
                                rxdataF_ext_offset,
                                rel15_ul->qam_mod_order);

        if (rel15_ul->qam_mod_order == 2) {
          nr_ulsch_shift_llr(pusch_vars->llr_layers, nb_re_pusch, rxdataF_ext_offset, rel15_ul->qam_mod_order, 4);
        }

#ifdef ML_DEBUG
        c16_t *llr_layers0 = (c16_t *)&pusch_vars->llr_layers[0][rxdataF_ext_offset * rel15_ul->qam_mod_order];
        c16_t *llr_layers1 = (c16_t *)&pusch_vars->llr_layers[1][rxdataF_ext_offset * rel15_ul->qam_mod_order];
        printf("===============================\n");
        printf("AFTER nr_ulsch_compute_ML_llr()\n");
        printf("===============================\n");
        for (int k = 0; k < nb_re_pusch; k++) {
          printf("[%3i] llr_layers0 = (%6i, %6i), llr_layers1 = (%6i, %6i)\n",
                 k, llr_layers0[k].r, llr_layers0[k].i, llr_layers1[k].r, llr_layers1[k].i);
        }
        printf("\n");
#endif
      }
      stop_meas(&gNB->ulsch_llr_stats);
      rxdataF_ext_offset += pusch_vars->ul_valid_re_per_slot[symbol];
    }
  } // symbol loop
  if (!(frame % 128)) {
    int num_llr = num_re_total*rel15_ul->qam_mod_order;
    GnbScopeUpdate(gNB, puschLLRe, num_llr);
    GnbScopeUpdate(gNB, puschIQe, num_re_total);
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
  pusch_vars->cl_done = 0;
  memset(pusch_vars->extraction_done,0,14*sizeof(int));
  gNB->nbSymb=0;
  bwp_start_subcarrier = ((rel15_ul->rb_start + rel15_ul->bwp_start)*NR_NB_SC_PER_RB + frame_parms->first_carrier_offset) % frame_parms->ofdm_symbol_size;
  LOG_D(PHY,"pusch %d.%d : bwp_start_subcarrier %d, rb_start %d, first_carrier_offset %d\n", frame,slot,bwp_start_subcarrier, rel15_ul->rb_start, frame_parms->first_carrier_offset);
  LOG_D(PHY,"pusch %d.%d : ul_dmrs_symb_pos %x\n",frame,slot,rel15_ul->ul_dmrs_symb_pos);

  //----------------------------------------------------------
  //------------------- Channel estimation -------------------
  //----------------------------------------------------------
  start_meas(&gNB->ulsch_channel_estimation_stats);
  int max_ch = 0;
  for (uint8_t symbol = rel15_ul->start_symbol_index; symbol < (rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols); symbol++) 
  {
    uint8_t dmrs_symbol_flag = (rel15_ul->ul_dmrs_symb_pos >> symbol) & 0x01;
    LOG_D(PHY, "symbol %d, dmrs_symbol_flag :%d\n", symbol, dmrs_symbol_flag);
    if (dmrs_symbol_flag == 1) 
    {
      if (pusch_vars->dmrs_symbol == INVALID_VALUE)
        pusch_vars->dmrs_symbol = symbol;
      for (int nl=0; nl<rel15_ul->nrOfLayers; nl++)
      {
        nr_pusch_channel_estimation(gNB,
                                    slot,
                                    get_dmrs_port(nl,rel15_ul->dmrs_ports),
                                    symbol,
                                    ulsch_id,
                                    bwp_start_subcarrier,
                                    rel15_ul,
                                    &max_ch,
                                    0 /* nvar*/);
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

  // get how many bit in a slot //
  int G = nr_get_G(rel15_ul->rb_size,
                   rel15_ul->nr_of_symbols,
                   nb_re_dmrs,
                   number_dmrs_symbols, // number of dmrs symbols irrespective of single or double symbol dmrs
                   rel15_ul->qam_mod_order,
                   rel15_ul->nrOfLayers);
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

  start_meas(&gNB->ulsch_rbs_extraction_stats);
  // extract the first dmrs for the channel level computation
  // extract the data in the OFDM frame, to the start of the array
  nr_ulsch_extract_rbs(gNB->common_vars.rxdataF,
                       pusch_vars,
                       slot,
                       meas_symbol,
                       (rel15_ul->ul_dmrs_symb_pos >> meas_symbol) & 0x01,
                       rel15_ul,
                       frame_parms);
  stop_meas(&gNB->ulsch_rbs_extraction_stats);

  int avgs = 0;
  int avg[frame_parms->nb_antennas_rx*rel15_ul->nrOfLayers];
  uint8_t shift_ch_ext = rel15_ul->nrOfLayers > 1 ? log2_approx(max_ch >> 11) : 0;

  //----------------------------------------------------------
  //--------------------- Channel Scaling --------------------
  //----------------------------------------------------------
  nr_ulsch_scale_channel(pusch_vars->ul_ch_estimates_ext,
                         frame_parms,
                         &gNB->ulsch[ulsch_id],
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
                         rel15_ul->nrOfLayers,
                         rel15_ul->rb_size);

  for (int aatx = 0; aatx < rel15_ul->nrOfLayers; aatx++)
    for (int aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++)
       avgs = cmax(avgs, avg[aatx*frame_parms->nb_antennas_rx+aarx]);
  
  if (rel15_ul->nrOfLayers == 1)
    pusch_vars->log2_maxh = (log2_approx(avgs) >> 1) + 2;
  else 
    pusch_vars->log2_maxh = (log2_approx(avgs) >> 1);

  pusch_vars->cl_done = 1;
  pusch_vars->extraction_done[meas_symbol] = 1;

  stop_meas(&gNB->rx_pusch_init_stats);

  start_meas(&gNB->rx_pusch_symbol_processing_stats);
  int numSymbols = gNB->num_pusch_symbols_per_thread;

  for(uint8_t symbol = rel15_ul->start_symbol_index; 
      symbol < (rel15_ul->start_symbol_index + rel15_ul->nr_of_symbols); 
      symbol += numSymbols) 
  {
    int total_res = 0;
    for (int s = 0; s < numSymbols;s++) 
    { 
      pusch_vars->ul_valid_re_per_slot[symbol+s] = get_nb_re_pusch(frame_parms,rel15_ul,symbol+s);
      pusch_vars->llr_offset[symbol+s] = ((symbol+s) == rel15_ul->start_symbol_index) ? 
                                         0 : 
                                         pusch_vars->llr_offset[symbol+s-1] + pusch_vars->ul_valid_re_per_slot[symbol+s-1] * rel15_ul->qam_mod_order;
      total_res+=pusch_vars->ul_valid_re_per_slot[symbol+s];
    }
    if (total_res > 0) 
    {
      union puschSymbolReqUnion id = {.s={ulsch_id,frame,slot,0}};
      id.p=1+symbol;
      notifiedFIFO_elt_t *req = newNotifiedFIFO_elt(sizeof(puschSymbolProc_t), id.p, gNB->respPuschSymb, &nr_pusch_symbol_processing_noprecoding); // create a job for Tpool
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

      pushTpool(&gNB->threadPool, req);
      gNB->nbSymb++;

      LOG_D(PHY,"%d.%d Added symbol %d (count %d) to process, in pipe\n",frame,slot,symbol,gNB->nbSymb);
    }
  } // symbol loop

  while (gNB->nbSymb > 0) 
  {
    notifiedFIFO_elt_t *req=pullTpool(gNB->respPuschSymb, &gNB->threadPool);
    gNB->nbSymb--;
    delNotifiedFIFO_elt(req);
  }

  stop_meas(&gNB->rx_pusch_symbol_processing_stats);
  return 0;
}
