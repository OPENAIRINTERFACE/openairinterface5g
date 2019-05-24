#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "nr_transport_proto.h"
#include "PHY/impl_defs_top.h"
#include "PHY/NR_TRANSPORT/nr_sch_dmrs.h"
#include "PHY/defs_nr_common.h"

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
      dft12((int16_t *)idft_in0, (int16_t *)idft_out0);

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
      dft24(idft_in0, idft_out0, 1);
      break;

    case 36:
      dft36(idft_in0, idft_out0, 1);
      break;

    case 48:
      dft48(idft_in0, idft_out0, 1);
      break;

    case 60:
      dft60(idft_in0, idft_out0, 1);
      break;

    case 72:
      dft72(idft_in0, idft_out0, 1);
      break;

    case 96:
      dft96(idft_in0, idft_out0, 1);
      break;

    case 108:
      dft108(idft_in0, idft_out0, 1);
      break;

    case 120:
      dft120(idft_in0, idft_out0, 1);
      break;

    case 144:
      dft144(idft_in0, idft_out0, 1);
      break;

    case 180:
      dft180(idft_in0, idft_out0, 1);
      break;

    case 192:
      dft192(idft_in0, idft_out0, 1);
      break;

    case 216:
      dft216(idft_in0, idft_out0, 1);
      break;

    case 240:
      dft240(idft_in0, idft_out0, 1);
      break;

    case 288:
      dft288(idft_in0, idft_out0, 1);
      break;

    case 300:
      dft300(idft_in0, idft_out0, 1);
      break;

    case 324:
      dft324((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 360:
      dft360((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 384:
      dft384((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 432:
      dft432((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 480:
      dft480((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 540:
      dft540((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 576:
      dft576((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 600:
      dft600((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 648:
      dft648((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 720:
      dft720((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 768:
      dft768((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 864:
      dft864((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 900:
      dft900((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 960:
      dft960((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 972:
      dft972((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 1080:
      dft1080((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 1152:
      dft1152((int16_t*)idft_in0, (int16_t*)idft_out0, 1);
      break;

    case 1200:
      dft1200(idft_in0, idft_out0, 1);
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

void nr_ulsch_extract_rbs_single(int **rxdataF,
                                 int **rxdataF_ext,
                                 uint32_t rxdataF_ext_offset,
                                 // unsigned int *rb_alloc, [hna] Resource Allocation Type 1 is assumed only for the moment
                                 unsigned char symbol,
                                 unsigned short start_rb,
                                 unsigned short nb_rb_pusch,
                                 NR_DL_FRAME_PARMS *frame_parms) 
{
  unsigned short start_re, re, nb_re_pusch;
  unsigned char aarx, is_dmrs_symbol = 0;
  uint32_t rxF_ext_index = 0;

  int16_t *rxF,*rxF_ext;

  is_dmrs_symbol = (symbol == 2) ? 1 : 0; //to be updated from config
  
  start_re = frame_parms->first_carrier_offset + (start_rb * NR_NB_SC_PER_RB);
  
  nb_re_pusch = NR_NB_SC_PER_RB * nb_rb_pusch;

  for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
    
    rxF       = (int16_t *)&rxdataF[aarx][symbol * frame_parms->ofdm_symbol_size];
    rxF_ext   = (int16_t *)&rxdataF_ext[aarx][symbol * nb_re_pusch]; // [hna] rxdataF_ext isn't contiguous in order to solve an alignment problem ib llr computation in case of mod_order = 4, 6

    for (re = 0; re < nb_re_pusch; re++) {

      if ( (is_dmrs_symbol && ((re&1) != 0))    ||    (is_dmrs_symbol == 0) ) { // [hna] (re&1) != frame_parms->nushift) assuming only dmrs type 1 and mapping type A
                                                                                // frame_parms->nushift should be initialized with 0
        rxF_ext[rxF_ext_index]     = (rxF[ ((start_re + re)*2)      % (frame_parms->ofdm_symbol_size*2)]);
        rxF_ext[rxF_ext_index + 1] = (rxF[(((start_re + re)*2) + 1) % (frame_parms->ofdm_symbol_size*2)]);
        rxF_ext_index = rxF_ext_index + 2;
    	}
    }
  }
}



void nr_rx_pusch(PHY_VARS_gNB *gNB,
                 uint8_t UE_id,
                 uint32_t frame,
                 uint8_t nr_tti_rx,
                 unsigned char symbol,
                 unsigned char harq_pid)
{

  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  nfapi_nr_ul_config_ulsch_pdu_rel15_t *rel15_ul = &gNB->ulsch[UE_id+1][0]->harq_processes[harq_pid]->ulsch_pdu.ulsch_pdu_rel15;
  uint32_t nb_re_pusch;
  
  if(symbol == rel15_ul->start_symbol)
    gNB->pusch_vars[UE_id]->rxdataF_ext_offset = 0;

  if (symbol == 2)  // [hna] here it is assumed that symbol 2 carries 6 DMRS REs (dmrs-type 1)
    nb_re_pusch = rel15_ul->number_rbs * 6;
  else
    nb_re_pusch = rel15_ul->number_rbs * NR_NB_SC_PER_RB;

  //----------------------------------------------------------
  //--------------------- RBs extraction ---------------------
  //----------------------------------------------------------
  
  nr_ulsch_extract_rbs_single(gNB->common_vars.rxdataF,
                              gNB->pusch_vars[UE_id]->rxdataF_ext,
                              gNB->pusch_vars[UE_id]->rxdataF_ext_offset,
                              // rb_alloc, [hna] Resource Allocation Type 1 is assumed only for the moment
                              symbol,
                              rel15_ul->start_rb,
                              rel15_ul->number_rbs,
                              frame_parms);

#ifdef NR_SC_FDMA
  nr_idft(&((uint32_t*)gNB->pusch_vars[UE_id]->rxdataF_ext[0])[symbol * rel15_ul->number_rbs * NR_NB_SC_PER_RB], nb_re_pusch);
#endif

  //----------------------------------------------------------
  //-------------------- LLRs computation --------------------
  //----------------------------------------------------------
  
  nr_ulsch_compute_llr(&gNB->pusch_vars[UE_id]->rxdataF_ext[0][symbol * rel15_ul->number_rbs * NR_NB_SC_PER_RB],
                       gNB->pusch_vars[UE_id]->ul_ch_mag,
                       gNB->pusch_vars[UE_id]->ul_ch_magb,
                       &gNB->pusch_vars[UE_id]->llr[gNB->pusch_vars[UE_id]->rxdataF_ext_offset * rel15_ul->Qm],
                       nb_re_pusch,
                       symbol,
                       rel15_ul->Qm);

  gNB->pusch_vars[UE_id]->rxdataF_ext_offset = gNB->pusch_vars[UE_id]->rxdataF_ext_offset +  nb_re_pusch;
  
}
