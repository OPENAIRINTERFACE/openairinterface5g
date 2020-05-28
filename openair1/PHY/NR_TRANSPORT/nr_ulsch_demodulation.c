#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "nr_transport_proto.h"
#include "PHY/impl_defs_top.h"
#include "PHY/NR_TRANSPORT/nr_sch_dmrs.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_REFSIG/ptrs_nr.h"
#include "PHY/NR_ESTIMATION/nr_ul_estimation.h"
#include "PHY/defs_nr_common.h"

//#define DEBUG_CH_COMP
//#define DEBUG_RB_EXT
//#define DEBUG_CH_MAG

void nr_idft(uint32_t *z, uint32_t Msc_PUSCH)
{

#if defined(__x86_64__) || defined(__i386__)
  __m128i idft_in128[1][1200], idft_out128[1][1200];
  __m128i norm128;
#elif defined(__arm__)
  int16x8_t idft_in128[1][1200], idft_out128[1][1200];
  int16x8_t norm128;
#endif
  int16_t *idft_in0 = (int16_t*)idft_in128[0], *idft_out0 = (int16_t*)idft_out128[0];

  int i, ip;

  LOG_T(PHY,"Doing lte_idft for Msc_PUSCH %d\n",Msc_PUSCH);

  // conjugate input
  for (i = 0; i < (Msc_PUSCH>>2); i++) {
#if defined(__x86_64__)||defined(__i386__)
    *&(((__m128i*)z)[i]) = _mm_sign_epi16(*&(((__m128i*)z)[i]), *(__m128i*)&conjugate2[0]);
#elif defined(__arm__)
    *&(((int16x8_t*)z)[i]) = vmulq_s16(*&(((int16x8_t*)z)[i]), *(int16x8_t*)&conjugate2[0]);
#endif
  }

  for (i=0,ip=0; i<Msc_PUSCH; i++, ip+=4) {
    ((uint32_t*)idft_in0)[ip+0] = z[i];
  }


  switch (Msc_PUSCH) {
    case 12:
      dft(DFT_12,(int16_t *)idft_in0, (int16_t *)idft_out0,0);

#if defined(__x86_64__)||defined(__i386__)
      norm128 = _mm_set1_epi16(9459);
#elif defined(__arm__)
      norm128 = vdupq_n_s16(9459);
#endif

      for (i = 0; i < 12; i++) {
#if defined(__x86_64__)||defined(__i386__)
        ((__m128i*)idft_out0)[i] = _mm_slli_epi16(_mm_mulhi_epi16(((__m128i*)idft_out0)[i], norm128), 1);
#elif defined(__arm__)
        ((int16x8_t*)idft_out0)[i] = vqdmulhq_s16(((int16x8_t*)idft_out0)[i], norm128);
#endif
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

    default:
      // should not be reached
      LOG_E( PHY, "Unsupported Msc_PUSCH value of %"PRIu16"\n", Msc_PUSCH );
      return;
  }



  for (i = 0, ip = 0; i < Msc_PUSCH; i++, ip+=4) {
    z[i] = ((uint32_t*)idft_out0)[ip];
  }

  // conjugate output
  for (i = 0; i < (Msc_PUSCH>>2); i++) {
#if defined(__x86_64__) || defined(__i386__)
    ((__m128i*)z)[i] = _mm_sign_epi16(((__m128i*)z)[i], *(__m128i*)&conjugate2[0]);
#elif defined(__arm__)
    *&(((int16x8_t*)z)[i]) = vmulq_s16(*&(((int16x8_t*)z)[i]), *(int16x8_t*)&conjugate2[0]);
#endif
  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif

}


void nr_ulsch_extract_rbs_single(int32_t **rxdataF,
                                 NR_gNB_PUSCH *pusch_vars,
                                 unsigned char symbol,
                                 uint8_t is_dmrs_symbol,
                                 nfapi_nr_pusch_pdu_t *pusch_pdu,
                                 NR_DL_FRAME_PARMS *frame_parms)
{
  unsigned short start_re, re, nb_re_pusch;
  unsigned char aarx;
  uint8_t K_ptrs;
  uint32_t rxF_ext_index = 0;
  uint32_t ul_ch0_ext_index = 0;
  uint32_t ul_ch0_index = 0;
  uint32_t ul_ch0_ptrs_ext_index = 0;
  uint32_t ul_ch0_ptrs_index = 0;
  uint8_t is_ptrs_symbol_flag,k_prime;
  uint16_t n=0, num_ptrs_symbols;
  int16_t *rxF,*rxF_ext;
  int *ul_ch0,*ul_ch0_ext;
  int *ul_ch0_ptrs,*ul_ch0_ptrs_ext;
  uint16_t n_rnti = pusch_pdu->rnti;
  uint8_t delta = 0;

#ifdef DEBUG_RB_EXT

  printf("--------------------symbol = %d-----------------------\n", symbol);
  printf("--------------------ch_ext_index = %d-----------------------\n", symbol*NR_NB_SC_PER_RB * pusch_pdu->rb_size);

#endif
  
  uint8_t is_dmrs_re;
  start_re = (frame_parms->first_carrier_offset + (pusch_pdu->rb_start * NR_NB_SC_PER_RB))%frame_parms->ofdm_symbol_size;
  nb_re_pusch = NR_NB_SC_PER_RB * pusch_pdu->rb_size;
  is_ptrs_symbol_flag = 0;
  num_ptrs_symbols = 0;

  K_ptrs = (pusch_pdu->pusch_ptrs.ptrs_freq_density)?4:2;

  for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
    
    rxF       = (int16_t *)&rxdataF[aarx][symbol * frame_parms->ofdm_symbol_size];
    rxF_ext   = (int16_t *)&pusch_vars->rxdataF_ext[aarx][symbol * nb_re_pusch]; // [hna] rxdataF_ext isn't contiguous in order to solve an alignment problem ib llr computation in case of mod_order = 4, 6

    ul_ch0     = &pusch_vars->ul_ch_estimates[aarx][pusch_vars->dmrs_symbol*frame_parms->ofdm_symbol_size]; // update channel estimates if new dmrs symbol are available

    ul_ch0_ext = &pusch_vars->ul_ch_estimates_ext[aarx][symbol*nb_re_pusch];

    ul_ch0_ptrs     = &pusch_vars->ul_ch_ptrs_estimates[aarx][pusch_vars->ptrs_symbol_index*frame_parms->ofdm_symbol_size]; // update channel estimates if new dmrs symbol are available

    ul_ch0_ptrs_ext = &pusch_vars->ul_ch_ptrs_estimates_ext[aarx][symbol*nb_re_pusch];

    n = 0;
    k_prime = 0;

    for (re = 0; re < nb_re_pusch; re++) {

      if (is_dmrs_symbol)
        is_dmrs_re = (re == get_dmrs_freq_idx_ul(n, k_prime, delta, pusch_pdu->dmrs_config_type));
      else
        is_dmrs_re = 0;

      if (pusch_pdu->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) {
        if(is_ptrs_symbol(symbol, pusch_vars->ptrs_symbols))
            is_ptrs_symbol_flag = is_ptrs_subcarrier((start_re + re) % frame_parms->ofdm_symbol_size,
                                                     n_rnti,
                                                     aarx,
                                                     pusch_pdu->dmrs_config_type,
                                                     K_ptrs,
                                                     pusch_pdu->rb_size,
                                                     pusch_pdu->pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset,
                                                     start_re,
                                                     frame_parms->ofdm_symbol_size);

        if (is_ptrs_symbol_flag == 1)
          num_ptrs_symbols++;

      }

  #ifdef DEBUG_RB_EXT
      printf("re = %d, is_ptrs_symbol_flag = %d, symbol = %d\n", re, is_ptrs_symbol_flag, symbol);
  #endif

      if ( is_dmrs_re == 0 && is_ptrs_symbol_flag == 0) {

        rxF_ext[rxF_ext_index]     = (rxF[ ((start_re + re)*2)      % (frame_parms->ofdm_symbol_size*2)]);
        rxF_ext[rxF_ext_index + 1] = (rxF[(((start_re + re)*2) + 1) % (frame_parms->ofdm_symbol_size*2)]);
        ul_ch0_ext[ul_ch0_ext_index] = ul_ch0[ul_ch0_index];
        ul_ch0_ptrs_ext[ul_ch0_ptrs_ext_index] = ul_ch0_ptrs[ul_ch0_ptrs_index];

  #ifdef DEBUG_RB_EXT
        printf("rxF_ext[%d] = %d, %d\n", rxF_ext_index, rxF_ext[rxF_ext_index], rxF_ext[rxF_ext_index+1]);
  #endif

        ul_ch0_ext_index++;
        ul_ch0_ptrs_ext_index++;
        rxF_ext_index +=2;
      } else {
        k_prime++;
        k_prime&=1;
        n+=(k_prime)?0:1;
      }
      ul_ch0_index++;
      ul_ch0_ptrs_index++;
    }
  }

  pusch_vars->ptrs_sc_per_ofdm_symbol = num_ptrs_symbols;

}

void nr_ulsch_scale_channel(int **ul_ch_estimates_ext,
                            NR_DL_FRAME_PARMS *frame_parms,
                            NR_gNB_ULSCH_t **ulsch_gNB,
                            uint8_t symbol,
                            uint8_t is_dmrs_symbol,
                            unsigned short nb_rb,
                            pusch_dmrs_type_t pusch_dmrs_type)
{

#if defined(__x86_64__)||defined(__i386__)

  short rb, ch_amp;
  unsigned char aatx,aarx;
  __m128i *ul_ch128, ch_amp128;

  // Determine scaling amplitude based the symbol

  ch_amp = 1024*8; //((pilots) ? (ulsch_gNB[0]->sqrt_rho_b) : (ulsch_gNB[0]->sqrt_rho_a));

  LOG_D(PHY,"Scaling PUSCH Chest in OFDM symbol %d by %d, pilots %d nb_rb %d NCP %d symbol %d\n", symbol, ch_amp, is_dmrs_symbol, nb_rb, frame_parms->Ncp, symbol);
   // printf("Scaling PUSCH Chest in OFDM symbol %d by %d\n",symbol_mod,ch_amp);

  ch_amp128 = _mm_set1_epi16(ch_amp); // Q3.13

  for (aatx=0; aatx < frame_parms->nb_antenna_ports_gNB; aatx++) {
    for (aarx=0; aarx < frame_parms->nb_antennas_rx; aarx++) {

      ul_ch128 = (__m128i *)&ul_ch_estimates_ext[aarx][symbol*nb_rb*NR_NB_SC_PER_RB];

      if (is_dmrs_symbol==1){
        if (pusch_dmrs_type == pusch_dmrs_type1)
          nb_rb = nb_rb>>1;
        else
          nb_rb = (2*nb_rb)/3;
      }


      for (rb=0;rb<nb_rb;rb++) {

        ul_ch128[0] = _mm_mulhi_epi16(ul_ch128[0], ch_amp128);
        ul_ch128[0] = _mm_slli_epi16(ul_ch128[0], 3);

        ul_ch128[1] = _mm_mulhi_epi16(ul_ch128[1], ch_amp128);
        ul_ch128[1] = _mm_slli_epi16(ul_ch128[1], 3);

        if (is_dmrs_symbol) {
          ul_ch128+=2;
        } else {
          ul_ch128[2] = _mm_mulhi_epi16(ul_ch128[2], ch_amp128);
          ul_ch128[2] = _mm_slli_epi16(ul_ch128[2], 3);
          ul_ch128+=3;

        }
      }
    }
  }
#endif
}

//compute average channel_level on each (TX,RX) antenna pair
void nr_ulsch_channel_level(int **ul_ch_estimates_ext,
                            NR_DL_FRAME_PARMS *frame_parms,
                            int32_t *avg,
                            uint8_t symbol,
                            uint32_t len,
                            unsigned short nb_rb)
{

#if defined(__x86_64__)||defined(__i386__)

  short rb;
  unsigned char aatx, aarx;
  __m128i *ul_ch128, avg128U;

  int16_t x = factor2(len);
  int16_t y = (len)>>x;

  for (aatx = 0; aatx < frame_parms->nb_antennas_tx; aatx++)
    for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      //clear average level
      avg128U = _mm_setzero_si128();

      ul_ch128=(__m128i *)&ul_ch_estimates_ext[(aatx<<1)+aarx][symbol*nb_rb*12];

      for (rb = 0; rb < len/12; rb++) {
        avg128U = _mm_add_epi32(avg128U, _mm_srai_epi32(_mm_madd_epi16(ul_ch128[0], ul_ch128[0]), x));
        avg128U = _mm_add_epi32(avg128U, _mm_srai_epi32(_mm_madd_epi16(ul_ch128[1], ul_ch128[1]), x));
        avg128U = _mm_add_epi32(avg128U, _mm_srai_epi32(_mm_madd_epi16(ul_ch128[2], ul_ch128[2]), x));
        ul_ch128+=3;
      }

      avg[(aatx<<1)+aarx] = (((int32_t*)&avg128U)[0] +
                             ((int32_t*)&avg128U)[1] +
                             ((int32_t*)&avg128U)[2] +
                             ((int32_t*)&avg128U)[3]   ) / y;

    }

  _mm_empty();
  _m_empty();

#elif defined(__arm__)

  short rb;
  unsigned char aatx, aarx, nre = 12, symbol_mod;
  int32x4_t avg128U;
  int16x4_t *ul_ch128;

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_gNB; aatx++)
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      //clear average level
      avg128U = vdupq_n_s32(0);
      // 5 is always a symbol with no pilots for both normal and extended prefix

      ul_ch128 = (int16x4_t *)&ul_ch_estimates_ext[(aatx<<1)+aarx][symbol*frame_parms->N_RB_UL*12];

      for (rb = 0; rb < nb_rb; rb++) {
        //  printf("rb %d : ",rb);
        //  print_shorts("ch",&ul_ch128[0]);
        avg128U = vqaddq_s32(avg128U, vmull_s16(ul_ch128[0], ul_ch128[0]));
        avg128U = vqaddq_s32(avg128U, vmull_s16(ul_ch128[1], ul_ch128[1]));
        avg128U = vqaddq_s32(avg128U, vmull_s16(ul_ch128[2], ul_ch128[2]));
        avg128U = vqaddq_s32(avg128U, vmull_s16(ul_ch128[3], ul_ch128[3]));

        if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_gNB!=1)) {
          ul_ch128+=4;
        } else {
          avg128U = vqaddq_s32(avg128U, vmull_s16(ul_ch128[4], ul_ch128[4]));
          avg128U = vqaddq_s32(avg128U, vmull_s16(ul_ch128[5], ul_ch128[5]));
          ul_ch128+=6;
        }

        /*
          if (rb==0) {
          print_shorts("ul_ch128",&ul_ch128[0]);
          print_shorts("ul_ch128",&ul_ch128[1]);
          print_shorts("ul_ch128",&ul_ch128[2]);
          }
        */
      }

      if (symbol==2) //assume start symbol 2
          nre=6;
      else
          nre=12;

      avg[(aatx<<1)+aarx] = (((int32_t*)&avg128U)[0] +
                             ((int32_t*)&avg128U)[1] +
                             ((int32_t*)&avg128U)[2] +
                             ((int32_t*)&avg128U)[3]   ) / (nb_rb*nre);
    }


#endif
}

void nr_ulsch_channel_compensation(int **rxdataF_ext,
                                   int **ul_ch_estimates_ext,
                                   int **ul_ch_mag,
                                   int **ul_ch_magb,
                                   int **rxdataF_comp,
                                   int **rho,
                                   NR_DL_FRAME_PARMS *frame_parms,
                                   unsigned char symbol,
                                   uint8_t is_dmrs_symbol,
                                   unsigned char mod_order,
                                   unsigned short nb_rb,
                                   unsigned char output_shift) {

#ifdef DEBUG_CH_COMP
  int16_t *rxF, *ul_ch;
  int prnt_idx;


  rxF   = (int16_t *)&rxdataF_ext[0][(symbol*nb_rb*12)];
  ul_ch = (int16_t *)&ul_ch_estimates_ext[0][symbol*nb_rb*12];

  printf("--------------------symbol = %d, mod_order = %d, output_shift = %d-----------------------\n", symbol, mod_order, output_shift);
  printf("----------------Before compansation------------------\n");

  for (prnt_idx=0;prnt_idx<12*nb_rb*2;prnt_idx++){

    printf("rxF[%d] = %d\n", prnt_idx, rxF[prnt_idx]);
    printf("ul_ch[%d] = %d\n", prnt_idx, ul_ch[prnt_idx]);

  }

#endif

#ifdef DEBUG_CH_MAG
  int16_t *ch_mag;
  int print_idx;


  ch_mag   = (int16_t *)&ul_ch_mag[0][(symbol*nb_rb*12)];

  printf("--------------------symbol = %d, mod_order = %d-----------------------\n", symbol, mod_order);
  printf("----------------Before computation------------------\n");

  for (print_idx=0;print_idx<50;print_idx++){

    printf("ch_mag[%d] = %d\n", print_idx, ch_mag[print_idx]);

  }

#endif

#if defined(__i386) || defined(__x86_64)

  unsigned short rb;
  unsigned char aatx,aarx;
  __m128i *ul_ch128,*ul_ch128_2,*ul_ch_mag128,*ul_ch_mag128b,*rxdataF128,*rxdataF_comp128,*rho128;
  __m128i mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3,QAM_amp128,QAM_amp128b;
  QAM_amp128b = _mm_setzero_si128();

  for (aatx=0; aatx<frame_parms->nb_antennas_tx; aatx++) {

    if (mod_order == 4) {
      QAM_amp128 = _mm_set1_epi16(QAM16_n1);  // 2/sqrt(10)
      QAM_amp128b = _mm_setzero_si128();
    } else if (mod_order == 6) {
      QAM_amp128  = _mm_set1_epi16(QAM64_n1); //
      QAM_amp128b = _mm_set1_epi16(QAM64_n2);
    }

    //    printf("comp: rxdataF_comp %p, symbol %d\n",rxdataF_comp[0],symbol);

    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

      ul_ch128          = (__m128i *)&ul_ch_estimates_ext[(aatx<<1)+aarx][symbol*nb_rb*12];
      ul_ch_mag128      = (__m128i *)&ul_ch_mag[(aatx<<1)+aarx][symbol*nb_rb*12];
      ul_ch_mag128b     = (__m128i *)&ul_ch_magb[(aatx<<1)+aarx][symbol*nb_rb*12];
      rxdataF128        = (__m128i *)&rxdataF_ext[aarx][symbol*nb_rb*12];
      rxdataF_comp128   = (__m128i *)&rxdataF_comp[(aatx<<1)+aarx][symbol*nb_rb*12];


      for (rb=0; rb<nb_rb; rb++) {
        if (mod_order>2) {
          // get channel amplitude if not QPSK

          //print_shorts("ch:",(int16_t*)&ul_ch128[0]);

          mmtmpD0 = _mm_madd_epi16(ul_ch128[0],ul_ch128[0]);
          mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);

          mmtmpD1 = _mm_madd_epi16(ul_ch128[1],ul_ch128[1]);
          mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);

          mmtmpD0 = _mm_packs_epi32(mmtmpD0,mmtmpD1);

          // store channel magnitude here in a new field of ulsch

          ul_ch_mag128[0] = _mm_unpacklo_epi16(mmtmpD0,mmtmpD0);
          ul_ch_mag128b[0] = ul_ch_mag128[0];
          ul_ch_mag128[0] = _mm_mulhi_epi16(ul_ch_mag128[0],QAM_amp128);
          ul_ch_mag128[0] = _mm_slli_epi16(ul_ch_mag128[0],1);

          // print_ints("ch: = ",(int32_t*)&mmtmpD0);
          // print_shorts("QAM_amp:",(int16_t*)&QAM_amp128);
          // print_shorts("mag:",(int16_t*)&ul_ch_mag128[0]);

          ul_ch_mag128[1] = _mm_unpackhi_epi16(mmtmpD0,mmtmpD0);
          ul_ch_mag128b[1] = ul_ch_mag128[1];
          ul_ch_mag128[1] = _mm_mulhi_epi16(ul_ch_mag128[1],QAM_amp128);
          ul_ch_mag128[1] = _mm_slli_epi16(ul_ch_mag128[1],1);

          if (is_dmrs_symbol==0) {
            mmtmpD0 = _mm_madd_epi16(ul_ch128[2],ul_ch128[2]);
            mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
            mmtmpD1 = _mm_packs_epi32(mmtmpD0,mmtmpD0);

            ul_ch_mag128[2] = _mm_unpacklo_epi16(mmtmpD1,mmtmpD1);
            ul_ch_mag128b[2] = ul_ch_mag128[2];

            ul_ch_mag128[2] = _mm_mulhi_epi16(ul_ch_mag128[2],QAM_amp128);
            ul_ch_mag128[2] = _mm_slli_epi16(ul_ch_mag128[2],1);
          }

          ul_ch_mag128b[0] = _mm_mulhi_epi16(ul_ch_mag128b[0],QAM_amp128b);
          ul_ch_mag128b[0] = _mm_slli_epi16(ul_ch_mag128b[0],1);


          ul_ch_mag128b[1] = _mm_mulhi_epi16(ul_ch_mag128b[1],QAM_amp128b);
          ul_ch_mag128b[1] = _mm_slli_epi16(ul_ch_mag128b[1],1);

          if (is_dmrs_symbol==0) {
            ul_ch_mag128b[2] = _mm_mulhi_epi16(ul_ch_mag128b[2],QAM_amp128b);
            ul_ch_mag128b[2] = _mm_slli_epi16(ul_ch_mag128b[2],1);
          }
        }

        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(ul_ch128[0],rxdataF128[0]);
        //  print_ints("re",&mmtmpD0);

        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(ul_ch128[0],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)&conjugate[0]);
        //  print_ints("im",&mmtmpD1);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,rxdataF128[0]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        //  print_ints("re(shift)",&mmtmpD0);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        //  print_ints("im(shift)",&mmtmpD1);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        //        print_ints("c0",&mmtmpD2);
        //  print_ints("c1",&mmtmpD3);
        rxdataF_comp128[0] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
        //  print_shorts("rx:",rxdataF128);
        //  print_shorts("ch:",ul_ch128);
        //  print_shorts("pack:",rxdataF_comp128);

        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(ul_ch128[1],rxdataF128[1]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(ul_ch128[1],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,rxdataF128[1]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

        rxdataF_comp128[1] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
        //  print_shorts("rx:",rxdataF128+1);
        //  print_shorts("ch:",ul_ch128+1);
        //  print_shorts("pack:",rxdataF_comp128+1);

        if (is_dmrs_symbol==0) {
          // multiply by conjugated channel
          mmtmpD0 = _mm_madd_epi16(ul_ch128[2],rxdataF128[2]);
          // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
          mmtmpD1 = _mm_shufflelo_epi16(ul_ch128[2],_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
          mmtmpD1 = _mm_madd_epi16(mmtmpD1,rxdataF128[2]);
          // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
          mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
          mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
          mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
          mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

          rxdataF_comp128[2] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
          //  print_shorts("rx:",rxdataF128+2);
          //  print_shorts("ch:",ul_ch128+2);
          //        print_shorts("pack:",rxdataF_comp128+2);

          ul_ch128+=3;
          ul_ch_mag128+=3;
          ul_ch_mag128b+=3;
          rxdataF128+=3;
          rxdataF_comp128+=3;
        } else { // we have a smaller PUSCH in symbols with pilots so skip last group of 4 REs and increment less
          ul_ch128+=2;
          ul_ch_mag128+=2;
          ul_ch_mag128b+=2;
          rxdataF128+=2;
          rxdataF_comp128+=2;
        }

      }
    }
  }

  if (rho) {


    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      rho128        = (__m128i *)&rho[aarx][symbol*frame_parms->N_RB_UL*12];
      ul_ch128      = (__m128i *)&ul_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_UL*12];
      ul_ch128_2    = (__m128i *)&ul_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_UL*12];

      for (rb=0; rb<nb_rb; rb++) {
        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(ul_ch128[0],ul_ch128_2[0]);
        //  print_ints("re",&mmtmpD0);

        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(ul_ch128[0],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)&conjugate[0]);
        //  print_ints("im",&mmtmpD1);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,ul_ch128_2[0]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        //  print_ints("re(shift)",&mmtmpD0);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        //  print_ints("im(shift)",&mmtmpD1);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        //        print_ints("c0",&mmtmpD2);
        //  print_ints("c1",&mmtmpD3);
        rho128[0] = _mm_packs_epi32(mmtmpD2,mmtmpD3);

        //print_shorts("rx:",ul_ch128_2);
        //print_shorts("ch:",ul_ch128);
        //print_shorts("pack:",rho128);

        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(ul_ch128[1],ul_ch128_2[1]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(ul_ch128[1],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,ul_ch128_2[1]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);


        rho128[1] =_mm_packs_epi32(mmtmpD2,mmtmpD3);
        //print_shorts("rx:",ul_ch128_2+1);
        //print_shorts("ch:",ul_ch128+1);
        //print_shorts("pack:",rho128+1);
        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(ul_ch128[2],ul_ch128_2[2]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(ul_ch128[2],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,ul_ch128_2[2]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

        rho128[2] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
        //print_shorts("rx:",ul_ch128_2+2);
        //print_shorts("ch:",ul_ch128+2);
        //print_shorts("pack:",rho128+2);

        ul_ch128+=3;
        ul_ch128_2+=3;
        rho128+=3;

      }

    }
  }

  _mm_empty();
  _m_empty();

#elif defined(__arm__)


  unsigned short rb;
  unsigned char aatx,aarx,symbol_mod,is_dmrs_symbol=0;

  int16x4_t *ul_ch128,*ul_ch128_2,*rxdataF128;
  int32x4_t mmtmpD0,mmtmpD1,mmtmpD0b,mmtmpD1b;
  int16x8_t *ul_ch_mag128,*ul_ch_mag128b,mmtmpD2,mmtmpD3,mmtmpD4;
  int16x8_t QAM_amp128,QAM_amp128b;
  int16x4x2_t *rxdataF_comp128,*rho128;

  int16_t conj[4]__attribute__((aligned(16))) = {1,-1,1,-1};
  int32x4_t output_shift128 = vmovq_n_s32(-(int32_t)output_shift);

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp))) {
    if (frame_parms->nb_antenna_ports_gNB==1) { // 10 out of 12 so don't reduce size
      nb_rb=1+(5*nb_rb/6);
    }
    else {
      is_dmrs_symbol=1;
    }
  }

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_gNB; aatx++) {
    if (mod_order == 4) {
      QAM_amp128  = vmovq_n_s16(QAM16_n1);  // 2/sqrt(10)
      QAM_amp128b = vmovq_n_s16(0);
    } else if (mod_order == 6) {
      QAM_amp128  = vmovq_n_s16(QAM64_n1); //
      QAM_amp128b = vmovq_n_s16(QAM64_n2);
    }
    //    printf("comp: rxdataF_comp %p, symbol %d\n",rxdataF_comp[0],symbol);

    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      ul_ch128          = (int16x4_t*)&ul_ch_estimates_ext[(aatx<<1)+aarx][symbol*frame_parms->N_RB_UL*12];
      ul_ch_mag128      = (int16x8_t*)&ul_ch_mag[(aatx<<1)+aarx][symbol*frame_parms->N_RB_UL*12];
      ul_ch_mag128b     = (int16x8_t*)&ul_ch_magb[(aatx<<1)+aarx][symbol*frame_parms->N_RB_UL*12];
      rxdataF128        = (int16x4_t*)&rxdataF_ext[aarx][symbol*frame_parms->N_RB_UL*12];
      rxdataF_comp128   = (int16x4x2_t*)&rxdataF_comp[(aatx<<1)+aarx][symbol*frame_parms->N_RB_UL*12];

      for (rb=0; rb<nb_rb; rb++) {
  if (mod_order>2) {
    // get channel amplitude if not QPSK
    mmtmpD0 = vmull_s16(ul_ch128[0], ul_ch128[0]);
    // mmtmpD0 = [ch0*ch0,ch1*ch1,ch2*ch2,ch3*ch3];
    mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
    // mmtmpD0 = [ch0*ch0 + ch1*ch1,ch0*ch0 + ch1*ch1,ch2*ch2 + ch3*ch3,ch2*ch2 + ch3*ch3]>>output_shift128 on 32-bits
    mmtmpD1 = vmull_s16(ul_ch128[1], ul_ch128[1]);
    mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
    mmtmpD2 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
    // mmtmpD2 = [ch0*ch0 + ch1*ch1,ch0*ch0 + ch1*ch1,ch2*ch2 + ch3*ch3,ch2*ch2 + ch3*ch3,ch4*ch4 + ch5*ch5,ch4*ch4 + ch5*ch5,ch6*ch6 + ch7*ch7,ch6*ch6 + ch7*ch7]>>output_shift128 on 16-bits
    mmtmpD0 = vmull_s16(ul_ch128[2], ul_ch128[2]);
    mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
    mmtmpD1 = vmull_s16(ul_ch128[3], ul_ch128[3]);
    mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
    mmtmpD3 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
    if (is_dmrs_symbol==0) {
      mmtmpD0 = vmull_s16(ul_ch128[4], ul_ch128[4]);
      mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
      mmtmpD1 = vmull_s16(ul_ch128[5], ul_ch128[5]);
      mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
      mmtmpD4 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
    }

    ul_ch_mag128b[0] = vqdmulhq_s16(mmtmpD2,QAM_amp128b);
    ul_ch_mag128b[1] = vqdmulhq_s16(mmtmpD3,QAM_amp128b);
    ul_ch_mag128[0] = vqdmulhq_s16(mmtmpD2,QAM_amp128);
    ul_ch_mag128[1] = vqdmulhq_s16(mmtmpD3,QAM_amp128);

    if (is_dmrs_symbol==0) {
      ul_ch_mag128b[2] = vqdmulhq_s16(mmtmpD4,QAM_amp128b);
      ul_ch_mag128[2]  = vqdmulhq_s16(mmtmpD4,QAM_amp128);
    }
  }

  mmtmpD0 = vmull_s16(ul_ch128[0], rxdataF128[0]);
  //mmtmpD0 = [Re(ch[0])Re(rx[0]) Im(ch[0])Im(ch[0]) Re(ch[1])Re(rx[1]) Im(ch[1])Im(ch[1])]
  mmtmpD1 = vmull_s16(ul_ch128[1], rxdataF128[1]);
  //mmtmpD1 = [Re(ch[2])Re(rx[2]) Im(ch[2])Im(ch[2]) Re(ch[3])Re(rx[3]) Im(ch[3])Im(ch[3])]
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  //mmtmpD0 = [Re(ch[0])Re(rx[0])+Im(ch[0])Im(ch[0]) Re(ch[1])Re(rx[1])+Im(ch[1])Im(ch[1]) Re(ch[2])Re(rx[2])+Im(ch[2])Im(ch[2]) Re(ch[3])Re(rx[3])+Im(ch[3])Im(ch[3])]

  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[0],*(int16x4_t*)conj)), rxdataF128[0]);
  //mmtmpD0 = [-Im(ch[0])Re(rx[0]) Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1]) Re(ch[1])Im(rx[1])]
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[1],*(int16x4_t*)conj)), rxdataF128[1]);
  //mmtmpD0 = [-Im(ch[2])Re(rx[2]) Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3]) Re(ch[3])Im(rx[3])]
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
  //mmtmpD1 = [-Im(ch[0])Re(rx[0])+Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1])+Re(ch[1])Im(rx[1]) -Im(ch[2])Re(rx[2])+Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3])+Re(ch[3])Im(rx[3])]

  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rxdataF_comp128[0] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
  mmtmpD0 = vmull_s16(ul_ch128[2], rxdataF128[2]);
  mmtmpD1 = vmull_s16(ul_ch128[3], rxdataF128[3]);
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[2],*(int16x4_t*)conj)), rxdataF128[2]);
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[3],*(int16x4_t*)conj)), rxdataF128[3]);
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rxdataF_comp128[1] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));

  if (is_dmrs_symbol==0) {
    mmtmpD0 = vmull_s16(ul_ch128[4], rxdataF128[4]);
    mmtmpD1 = vmull_s16(ul_ch128[5], rxdataF128[5]);
    mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
         vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));

    mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[4],*(int16x4_t*)conj)), rxdataF128[4]);
    mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[5],*(int16x4_t*)conj)), rxdataF128[5]);
    mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
         vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));


    mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
    mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
    rxdataF_comp128[2] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));


    ul_ch128+=6;
    ul_ch_mag128+=3;
    ul_ch_mag128b+=3;
    rxdataF128+=6;
    rxdataF_comp128+=3;

  } else { // we have a smaller PUSCH in symbols with pilots so skip last group of 4 REs and increment less
    ul_ch128+=4;
    ul_ch_mag128+=2;
    ul_ch_mag128b+=2;
    rxdataF128+=4;
    rxdataF_comp128+=2;
  }
      }
    }
  }

  if (rho) {
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      rho128        = (int16x4x2_t*)&rho[aarx][symbol*frame_parms->N_RB_UL*12];
      ul_ch128      = (int16x4_t*)&ul_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_UL*12];
      ul_ch128_2    = (int16x4_t*)&ul_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_UL*12];
      for (rb=0; rb<nb_rb; rb++) {
  mmtmpD0 = vmull_s16(ul_ch128[0], ul_ch128_2[0]);
  mmtmpD1 = vmull_s16(ul_ch128[1], ul_ch128_2[1]);
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[0],*(int16x4_t*)conj)), ul_ch128_2[0]);
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[1],*(int16x4_t*)conj)), ul_ch128_2[1]);
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));

  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rho128[0] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));

  mmtmpD0 = vmull_s16(ul_ch128[2], ul_ch128_2[2]);
  mmtmpD1 = vmull_s16(ul_ch128[3], ul_ch128_2[3]);
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[2],*(int16x4_t*)conj)), ul_ch128_2[2]);
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[3],*(int16x4_t*)conj)), ul_ch128_2[3]);
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));

  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rho128[1] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));

  mmtmpD0 = vmull_s16(ul_ch128[0], ul_ch128_2[0]);
  mmtmpD1 = vmull_s16(ul_ch128[1], ul_ch128_2[1]);
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[4],*(int16x4_t*)conj)), ul_ch128_2[4]);
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(ul_ch128[5],*(int16x4_t*)conj)), ul_ch128_2[5]);
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));

  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rho128[2] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));


  ul_ch128+=6;
  ul_ch128_2+=6;
  rho128+=3;
      }
    }
  }
#endif


#ifdef DEBUG_CH_COMP

  rxF   = (int16_t *)&rxdataF_comp[0][(symbol*nb_rb*12)];

  printf("----------------After compansation------------------\n");

  for (prnt_idx=0;prnt_idx<12*nb_rb*2;prnt_idx++){

    printf("rxF[%d] = %d\n", prnt_idx, rxF[prnt_idx]);

  }

#endif

#ifdef DEBUG_CH_MAG


  ch_mag   = (int16_t *)&ul_ch_mag[0][(symbol*nb_rb*12)];

  printf("----------------After computation------------------\n");

  for (print_idx=0;print_idx<12*nb_rb*2;print_idx++){

    printf("ch_mag[%d] = %d\n", print_idx, ch_mag[print_idx]);

  }

#endif

}

void nr_rx_pusch(PHY_VARS_gNB *gNB,
                 uint8_t UE_id,
                 uint32_t frame,
                 uint8_t nr_tti_rx,
                 unsigned char symbol,
                 unsigned char harq_pid)
{

  uint8_t first_symbol_flag, aarx, aatx, dmrs_symbol_flag; // dmrs_symbol_flag, a flag to indicate DMRS REs in current symbol
  uint32_t nb_re_pusch, bwp_start_subcarrier;
  uint8_t L_ptrs = 0; // PTRS parameter
  int avgs;
  int avg[4];
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  nfapi_nr_pusch_pdu_t *rel15_ul = &gNB->ulsch[UE_id][0]->harq_processes[harq_pid]->ulsch_pdu;

  dmrs_symbol_flag = 0;
  first_symbol_flag = 0;
  gNB->pusch_vars[UE_id]->ptrs_sc_per_ofdm_symbol = 0;

  if(symbol == rel15_ul->start_symbol_index){
    gNB->pusch_vars[UE_id]->rxdataF_ext_offset = 0;
    gNB->pusch_vars[UE_id]->dmrs_symbol = 0;
    gNB->pusch_vars[UE_id]->cl_done = 0;
    gNB->pusch_vars[UE_id]->ptrs_symbols = 0;
    first_symbol_flag = 1;

    if (rel15_ul->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) {  // if there is ptrs pdu
      L_ptrs = 1<<(rel15_ul->pusch_ptrs.ptrs_time_density);

      set_ptrs_symb_idx(&gNB->pusch_vars[UE_id]->ptrs_symbols,
                        rel15_ul->nr_of_symbols,
                        rel15_ul->start_symbol_index,
                        L_ptrs,
                        rel15_ul->ul_dmrs_symb_pos);
    }
  }

  bwp_start_subcarrier = (rel15_ul->rb_start*NR_NB_SC_PER_RB + frame_parms->first_carrier_offset) % frame_parms->ofdm_symbol_size;

  dmrs_symbol_flag = ((rel15_ul->ul_dmrs_symb_pos)>>symbol)&0x01;

  if (dmrs_symbol_flag == 1){
    nb_re_pusch = rel15_ul->rb_size * ((rel15_ul->dmrs_config_type==pusch_dmrs_type1)?6:8);
    gNB->pusch_vars[UE_id]->dmrs_symbol = symbol;
  } else {
    nb_re_pusch = rel15_ul->rb_size * NR_NB_SC_PER_RB;
  }

  if (rel15_ul->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) {  // if there is ptrs pdu
    if(is_ptrs_symbol(symbol, gNB->pusch_vars[UE_id]->ptrs_symbols))
      gNB->pusch_vars[UE_id]->ptrs_symbol_index = symbol;
  }


  //----------------------------------------------------------
  //--------------------- Channel estimation ---------------------
  //----------------------------------------------------------
  start_meas(&gNB->ulsch_channel_estimation_stats);
  if (dmrs_symbol_flag == 1)
    nr_pusch_channel_estimation(gNB,
                                nr_tti_rx,
                                0, // p
                                symbol,
                                bwp_start_subcarrier,
                                rel15_ul);
  stop_meas(&gNB->ulsch_channel_estimation_stats);
  //----------------------------------------------------------
  //--------------------- RBs extraction ---------------------
  //----------------------------------------------------------

  start_meas(&gNB->ulsch_rbs_extraction_stats);
  nr_ulsch_extract_rbs_single(gNB->common_vars.rxdataF,
                              gNB->pusch_vars[UE_id],
                              symbol,
                              dmrs_symbol_flag,
                              rel15_ul,
                              frame_parms);
  stop_meas(&gNB->ulsch_rbs_extraction_stats);

  nr_ulsch_scale_channel(gNB->pusch_vars[UE_id]->ul_ch_estimates_ext,
                         frame_parms,
                         gNB->ulsch[UE_id],
                         symbol,
                         dmrs_symbol_flag,
                         rel15_ul->rb_size,
                         rel15_ul->dmrs_config_type);


  if (first_symbol_flag==1) {

    nr_ulsch_channel_level(gNB->pusch_vars[UE_id]->ul_ch_estimates_ext,
                           frame_parms,
                           avg,
                           symbol,
                           nb_re_pusch,
                           rel15_ul->rb_size);
     avgs = 0;

     for (aatx=0;aatx<frame_parms->nb_antennas_tx;aatx++)
       for (aarx=0;aarx<frame_parms->nb_antennas_rx;aarx++)
         avgs = cmax(avgs,avg[(aatx<<1)+aarx]);

     gNB->pusch_vars[UE_id]->log2_maxh = (log2_approx(avgs)/2)+1;

  }

  start_meas(&gNB->ulsch_channel_compensation_stats);
  nr_ulsch_channel_compensation(gNB->pusch_vars[UE_id]->rxdataF_ext,
                                gNB->pusch_vars[UE_id]->ul_ch_estimates_ext,
                                gNB->pusch_vars[UE_id]->ul_ch_mag0,
                                gNB->pusch_vars[UE_id]->ul_ch_magb0,
                                gNB->pusch_vars[UE_id]->rxdataF_comp,
                                (frame_parms->nb_antennas_tx>1) ? gNB->pusch_vars[UE_id]->rho : NULL,
                                frame_parms,
                                symbol,
                                dmrs_symbol_flag,
                                rel15_ul->qam_mod_order,
                                rel15_ul->rb_size,
                                gNB->pusch_vars[UE_id]->log2_maxh);
  stop_meas(&gNB->ulsch_channel_compensation_stats);

#ifdef NR_SC_FDMA
  nr_idft(&((uint32_t*)gNB->pusch_vars[UE_id]->rxdataF_ext[0])[symbol * rel15_ul->rb_size * NR_NB_SC_PER_RB], nb_re_pusch);
#endif

  //----------------------------------------------------------
  //-------------------- LLRs computation --------------------
  //----------------------------------------------------------
  start_meas(&gNB->ulsch_llr_stats);
  nr_ulsch_compute_llr(&gNB->pusch_vars[UE_id]->rxdataF_comp[0][symbol * rel15_ul->rb_size * NR_NB_SC_PER_RB],
                       gNB->pusch_vars[UE_id]->ul_ch_mag0,
                       gNB->pusch_vars[UE_id]->ul_ch_magb0,
                       &gNB->pusch_vars[UE_id]->llr[gNB->pusch_vars[UE_id]->rxdataF_ext_offset * rel15_ul->qam_mod_order],
                       rel15_ul->rb_size,
                       nb_re_pusch,
                       symbol,
                       rel15_ul->qam_mod_order);
  stop_meas(&gNB->ulsch_llr_stats);

  gNB->pusch_vars[UE_id]->rxdataF_ext_offset = gNB->pusch_vars[UE_id]->rxdataF_ext_offset +  nb_re_pusch - gNB->pusch_vars[UE_id]->ptrs_sc_per_ofdm_symbol;
  
}
