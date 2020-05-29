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

#include "PHY/defs_nr_UE.h"
#include "PHY/defs_gNB.h"
#include "modulation_UE.h"
#include "nr_modulation.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"
#include <common/utils/LOG/log.h>

//#define DEBUG_FEP

#define SOFFSET 0

/*#ifdef LOG_I
#undef LOG_I
#define LOG_I(A,B...) printf(A)
#endif*/

int nr_slot_fep(PHY_VARS_NR_UE *ue,
                unsigned char symbol,
                unsigned char Ns,
                int sample_offset,
                int no_prefix)
{
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  NR_UE_COMMON *common_vars   = &ue->common_vars;
  unsigned char aa;
  unsigned int nb_prefix_samples;
  unsigned int nb_prefix_samples0;
  unsigned int abs_symbol;
  if (ue->is_synchronized) {
    nb_prefix_samples = (no_prefix ? 0 : frame_parms->nb_prefix_samples);
    nb_prefix_samples0 = (no_prefix ? 0 : frame_parms->nb_prefix_samples0);
  }
  else {
    nb_prefix_samples = (no_prefix ? 0 : frame_parms->nb_prefix_samples);
    nb_prefix_samples0 = (no_prefix ? 0 : frame_parms->nb_prefix_samples);
  }
  //unsigned int subframe_offset;//,subframe_offset_F;
  unsigned int slot_offset;
  //int i;
  unsigned int frame_length_samples = frame_parms->samples_per_subframe * 10;
  unsigned int rx_offset;


  dft_size_idx_t dftsize;
  int tmp_dft_in[8192] __attribute__ ((aligned (32)));  // This is for misalignment issues for 6 and 15 PRBs

  switch (frame_parms->ofdm_symbol_size) {
  case 128:
    dftsize = DFT_128;
    break;

  case 256:
    dftsize = DFT_256;
    break;

  case 512:
    dftsize = DFT_512;
    break;

  case 1024:
    dftsize = DFT_1024;
    break;

  case 1536:
    dftsize = DFT_1536;
    break;

  case 2048:
    dftsize = DFT_2048;
    break;

  case 3072:
    dftsize = DFT_3072;
    break;

  case 4096:
    dftsize = DFT_4096;
    break;

  case 8192:
    dftsize = DFT_8192;
    break;

  default:
    printf("unsupported ofdm symbol size \n");
    assert(0);
  }

  if (no_prefix) {
    slot_offset = frame_parms->ofdm_symbol_size * (frame_parms->symbols_per_slot) * (Ns);
  } else {
    slot_offset = frame_parms->get_samples_slot_timestamp(Ns,frame_parms,0);
  }

  /*if (l<0 || l>=7-frame_parms->Ncp) {
    printf("slot_fep: l must be between 0 and %d\n",7-frame_parms->Ncp);
    return(-1);
    }*/

  if (Ns<0 || Ns>(frame_parms->slots_per_frame-1)) {
    printf("slot_fep: Ns must be between 0 and %d\n",frame_parms->slots_per_frame-1);
    return(-1);
  }

  for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
    memset(&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],0,frame_parms->ofdm_symbol_size*sizeof(int));

    rx_offset = sample_offset + slot_offset - SOFFSET;
    // Align with 256 bit
    //    rx_offset = rx_offset&0xfffffff8;

#ifdef DEBUG_FEP
      //  if (ue->frame <100)
    /*LOG_I(PHY,*/printf("slot_fep: frame %d: slot %d, symbol %d, nb_prefix_samples %u, nb_prefix_samples0 %u, slot_offset %u, sample_offset %d,rx_offset %u, frame_length_samples %u\n",
    		ue->proc.proc_rxtx[(Ns)&1].frame_rx, Ns, symbol, nb_prefix_samples, nb_prefix_samples0, slot_offset, sample_offset, rx_offset, frame_length_samples);
#endif

    abs_symbol = Ns * frame_parms->symbols_per_slot + symbol;
    
    for (int idx_symb = Ns*frame_parms->symbols_per_slot; idx_symb < abs_symbol; idx_symb++)
      rx_offset += (idx_symb%(0x7<<frame_parms->numerology_index)) ? nb_prefix_samples : nb_prefix_samples0;

    rx_offset += frame_parms->ofdm_symbol_size * symbol;

    if (abs_symbol%(0x7<<frame_parms->numerology_index)) {

      rx_offset += nb_prefix_samples;
      if (rx_offset > (frame_length_samples - frame_parms->ofdm_symbol_size))
        memcpy((short*) &common_vars->rxdata[aa][frame_length_samples],
               (short*) &common_vars->rxdata[aa][0],
               frame_parms->ofdm_symbol_size*sizeof(int));

      if ((rx_offset&7)!=0) {  // if input to dft is not 256-bit aligned, issue for size 6,15 and 25 PRBs
        memcpy((void *)tmp_dft_in,
               (void *) &common_vars->rxdata[aa][rx_offset % frame_length_samples],
               frame_parms->ofdm_symbol_size*sizeof(int));
        dft(dftsize,(int16_t *)tmp_dft_in,
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      } else { // use dft input from RX buffer directly
#if UE_TIMING_TRACE
          start_meas(&ue->rx_dft_stats);
#endif

        dft(dftsize,(int16_t *) &common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
#if UE_TIMING_TRACE
        stop_meas(&ue->rx_dft_stats);
#endif
      }
    } else {

      rx_offset += nb_prefix_samples0;
      if (rx_offset > (frame_length_samples - frame_parms->ofdm_symbol_size))
        memcpy((void *) &common_vars->rxdata[aa][frame_length_samples],
               (void *) &common_vars->rxdata[aa][0],
               frame_parms->ofdm_symbol_size*sizeof(int));
#if UE_TIMING_TRACE
      start_meas(&ue->rx_dft_stats);
#endif

      if ((rx_offset&7)!=0) {  // if input to dft is not 128-bit aligned, issue for size 6 and 15 PRBs
        memcpy((void *)tmp_dft_in,
               (void *) &common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
               frame_parms->ofdm_symbol_size*sizeof(int));
        dft(dftsize,(int16_t *)tmp_dft_in,
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      } else { // use dft input from RX buffer directly

        dft(dftsize,(int16_t *) &common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      }
#if UE_TIMING_TRACE
      stop_meas(&ue->rx_dft_stats);
#endif


    }

    #ifdef DEBUG_FEP
        //  if (ue->frame <100)
        printf("slot_fep: frame %d: symbol %d rx_offset %u\n", ue->proc.proc_rxtx[(Ns)&1].frame_rx, symbol, rx_offset);
    #endif
  }


#ifdef DEBUG_FEP
  printf("slot_fep: done\n");
#endif
  return(0);
}

int nr_slot_fep_init_sync(PHY_VARS_NR_UE *ue,
                unsigned char symbol,
                unsigned char Ns,
                int sample_offset,
                int no_prefix)
{
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  NR_UE_COMMON *common_vars   = &ue->common_vars;
  unsigned char aa;
  unsigned int nb_prefix_samples;
  unsigned int nb_prefix_samples0;
  unsigned int abs_symbol;
  if (ue->is_synchronized) {
    nb_prefix_samples = (no_prefix ? 0 : frame_parms->nb_prefix_samples);
    nb_prefix_samples0 = (no_prefix ? 0 : frame_parms->nb_prefix_samples0);
  }
  else {
    nb_prefix_samples = (no_prefix ? 0 : frame_parms->nb_prefix_samples);
    nb_prefix_samples0 = (no_prefix ? 0 : frame_parms->nb_prefix_samples);
  }
  //unsigned int subframe_offset;//,subframe_offset_F;
  unsigned int slot_offset;
  //int i;
  unsigned int frame_length_samples = frame_parms->samples_per_subframe * 10;
  unsigned int rx_offset;


  dft_size_idx_t dftsize;
  int tmp_dft_in[8192] __attribute__ ((aligned (32)));  // This is for misalignment issues for 6 and 15 PRBs

  switch (frame_parms->ofdm_symbol_size) {
  case 128:
    dftsize = DFT_128;
    break;

  case 256:
    dftsize = DFT_256;
    break;

  case 512:
    dftsize = DFT_512;
    break;

  case 1024:
    dftsize = DFT_1024;
    break;

  case 1536:
    dftsize = DFT_1536;
    break;

  case 2048:
    dftsize = DFT_2048;
    break;

  case 3072:
    dftsize = DFT_3072;
    break;

  case 4096:
    dftsize = DFT_4096;
    break;

  case 8192:
    dftsize = DFT_8192;
    break;

  default:
    printf("unsupported ofdm symbol size \n");
    assert(0);
  }

  if (no_prefix) {
    slot_offset = frame_parms->ofdm_symbol_size * (frame_parms->symbols_per_slot) * (Ns);
  } else {
    slot_offset = frame_parms->get_samples_slot_timestamp(Ns,frame_parms,0);
  }

  /*if (l<0 || l>=7-frame_parms->Ncp) {
    printf("slot_fep: l must be between 0 and %d\n",7-frame_parms->Ncp);
    return(-1);
    }*/

  if (Ns<0 || Ns>(frame_parms->slots_per_frame-1)) {
    printf("slot_fep: Ns must be between 0 and %d\n",frame_parms->slots_per_frame-1);
    return(-1);
  }

  for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
    memset(&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],0,frame_parms->ofdm_symbol_size*sizeof(int));

    rx_offset = sample_offset + slot_offset - SOFFSET;
    // Align with 256 bit
    //    rx_offset = rx_offset&0xfffffff8;

#ifdef DEBUG_FEP
      //  if (ue->frame <100)
    /*LOG_I(PHY,*/printf("slot_fep: frame %d: slot %d, symbol %d, nb_prefix_samples %u, nb_prefix_samples0 %u, slot_offset %u, sample_offset %d,rx_offset %u, frame_length_samples %u\n",
    		ue->proc.proc_rxtx[(Ns)&1].frame_rx, Ns, symbol, nb_prefix_samples, nb_prefix_samples0, slot_offset, sample_offset, rx_offset, frame_length_samples);
#endif

    abs_symbol = Ns * frame_parms->symbols_per_slot + symbol;
    
    for (int idx_symb = Ns*frame_parms->symbols_per_slot; idx_symb < abs_symbol; idx_symb++)
      rx_offset += (abs_symbol%(0x7<<frame_parms->numerology_index)) ? nb_prefix_samples : nb_prefix_samples0;

    rx_offset += frame_parms->ofdm_symbol_size * symbol;

    if (abs_symbol%(0x7<<frame_parms->numerology_index)) {

      rx_offset += nb_prefix_samples;
      if (rx_offset > (frame_length_samples - frame_parms->ofdm_symbol_size))
        memcpy((short*) &common_vars->rxdata[aa][frame_length_samples],
               (short*) &common_vars->rxdata[aa][0],
               frame_parms->ofdm_symbol_size*sizeof(int));

      if ((rx_offset&7)!=0) {  // if input to dft is not 256-bit aligned, issue for size 6,15 and 25 PRBs
        memcpy((void *)tmp_dft_in,
               (void *) &common_vars->rxdata[aa][rx_offset],
               frame_parms->ofdm_symbol_size*sizeof(int));
        dft(dftsize,(int16_t *)tmp_dft_in,
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      } else { // use dft input from RX buffer directly
#if UE_TIMING_TRACE
          start_meas(&ue->rx_dft_stats);
#endif


        dft(dftsize,(int16_t *) &common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
#if UE_TIMING_TRACE
        stop_meas(&ue->rx_dft_stats);
#endif
      }
    } else {

      rx_offset += nb_prefix_samples0;
      if (rx_offset > (frame_length_samples - frame_parms->ofdm_symbol_size))
        memcpy((void *) &common_vars->rxdata[aa][frame_length_samples],
               (void *) &common_vars->rxdata[aa][0],
               frame_parms->ofdm_symbol_size*sizeof(int));
#if UE_TIMING_TRACE
      start_meas(&ue->rx_dft_stats);
#endif

      if ((rx_offset&7)!=0) {  // if input to dft is not 128-bit aligned, issue for size 6 and 15 PRBs
        memcpy((void *)tmp_dft_in,
               (void *) &common_vars->rxdata[aa][rx_offset],
               frame_parms->ofdm_symbol_size*sizeof(int));
        dft(dftsize,(int16_t *)tmp_dft_in,
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      } else { // use dft input from RX buffer directly
        dft(dftsize,(int16_t *) &common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      }
#if UE_TIMING_TRACE
      stop_meas(&ue->rx_dft_stats);
#endif


    }

    #ifdef DEBUG_FEP
        //  if (ue->frame <100)
        printf("slot_fep: frame %d: symbol %d rx_offset %u\n", ue->proc.proc_rxtx[(Ns)&1].frame_rx, symbol, rx_offset);
    #endif
  }


#ifdef DEBUG_FEP
  printf("slot_fep: done\n");
#endif
  return(0);
}


int nr_slot_fep_ul(NR_DL_FRAME_PARMS *frame_parms,
                   int32_t *rxdata,
                   int32_t *rxdataF,
                   unsigned char symbol,
                   unsigned char Ns,
                   int sample_offset,
                   int no_prefix)
{
  uint32_t slot_offset;
  uint32_t rxdata_offset;

  unsigned int nb_prefix_samples  = (no_prefix ? 0 : frame_parms->nb_prefix_samples);
  unsigned int nb_prefix_samples0 = (no_prefix ? 0 : frame_parms->nb_prefix_samples0);
  
  dft_size_idx_t dftsize;

  switch (frame_parms->ofdm_symbol_size) {
    case 128:
      dftsize = DFT_128;
      break;

    case 256:
      dftsize = DFT_256;
      break;

    case 512:
      dftsize = DFT_512;
      break;

    case 1024:
      dftsize = DFT_1024;
      break;

    case 1536:
      dftsize = DFT_1536;
      break;

    case 2048:
      dftsize = DFT_2048;
      break;

    case 4096:
      dftsize = DFT_4096;
      break;

    case 8192:
      dftsize = DFT_8192;
      break;

    default:
      dftsize = DFT_512;
      break;
  }
  
  slot_offset   = frame_parms->get_samples_slot_timestamp(Ns,frame_parms,0);

  
  if(symbol == 0)
    rxdata_offset = slot_offset + nb_prefix_samples0 - SOFFSET;
  else
    rxdata_offset = slot_offset + nb_prefix_samples0 + (symbol * (frame_parms->ofdm_symbol_size + nb_prefix_samples)) - SOFFSET;

  dft(dftsize,(int16_t *)&rxdata[rxdata_offset],
       (int16_t *)&rxdataF[symbol * frame_parms->ofdm_symbol_size], 1);

  return(0);
}
