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

/*! \file PHY/NR_TRANSPORT/srs_rx.c
 * \brief Top-level routines for getting the SRS physical channel
 * \date 2021
 * \version 1.0
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "PHY/impl_defs_nr.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_gNB.h"
#include <openair1/PHY/CODING/nrSmallBlock/nr_small_block_defs.h>
#include "common/utils/LOG/log.h"

#include "nfapi/oai_integration/vendor_ext.h"

#include "T.h"

//#define SRS_DEBUG

NR_gNB_SRS_t *new_gNB_srs(void){
  NR_gNB_SRS_t *srs;
  srs = (NR_gNB_SRS_t *)malloc16(sizeof(NR_gNB_SRS_t));
  srs->active = 0;
  return (srs);
}

int nr_find_srs(uint16_t rnti,
                int frame,
                int slot,
                PHY_VARS_gNB *gNB) {

  AssertFatal(gNB!=NULL,"gNB is null\n");
  int index = -1;

  for (int i=0; i<NUMBER_OF_NR_SRS_MAX; i++) {
    AssertFatal(gNB->srs[i]!=NULL,"gNB->srs[%d] is null\n",i);
    if ((gNB->srs[i]->active>0) &&
        (gNB->srs[i]->srs_pdu.rnti==rnti) &&
        (gNB->srs[i]->frame==frame) &&
        (gNB->srs[i]->slot==slot)) return(i);
    else if ((gNB->srs[i]->active == 0) && (index==-1)) index=i;
  }

  if (index==-1)
    LOG_E(MAC,"SRS list is full\n");

  return(index);
}

void nr_fill_srs(PHY_VARS_gNB *gNB,
                 int frame,
                 int slot,
                 nfapi_nr_srs_pdu_t *srs_pdu) {

  int id = nr_find_srs(srs_pdu->rnti,frame,slot,gNB);
  AssertFatal( (id>=0) && (id<NUMBER_OF_NR_SRS_MAX),
               "invalid id found for srs !!! rnti %04x id %d\n",srs_pdu->rnti,id);

  NR_gNB_SRS_t  *srs = gNB->srs[id];
  srs->frame = frame;
  srs->slot = slot;
  srs->active = 1;
  memcpy((void*)&srs->srs_pdu, (void*)srs_pdu, sizeof(nfapi_nr_srs_pdu_t));
}

int nr_get_srs_signal(PHY_VARS_gNB *gNB,
                      int frame,
                      int slot,
                      nfapi_nr_srs_pdu_t *srs_pdu,
                      nr_srs_info_t *nr_srs_info,
                      int32_t **srs_received_signal) {

  if(nr_srs_info->sc_list_length == 0) {
    LOG_E(NR_PHY, "(%d.%d) nr_srs_info was not generated yet!\n", frame, slot);
    return -1;
  }

  int32_t **rxdataF = gNB->common_vars.rxdataF;
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;

  uint16_t n_symbols = (slot&3)*frame_parms->symbols_per_slot;                    // number of symbols until this slot
  uint8_t l0 = frame_parms->symbols_per_slot - 1 - srs_pdu->time_start_position;  // starting symbol in this slot
  uint64_t symbol_offset = (n_symbols+l0)*frame_parms->ofdm_symbol_size;

  int32_t *rx_signal;
  for (int ant = 0; ant < frame_parms->nb_antennas_rx; ant++) {

    memset(srs_received_signal[ant], 0, frame_parms->ofdm_symbol_size*sizeof(int32_t));
    rx_signal = &rxdataF[ant][symbol_offset];

    for(int sc_idx = 0; sc_idx < nr_srs_info->sc_list_length; sc_idx++) {
      srs_received_signal[ant][nr_srs_info->sc_list[sc_idx]] = rx_signal[nr_srs_info->sc_list[sc_idx]];

#ifdef SRS_DEBUG
      uint64_t subcarrier_offset = frame_parms->first_carrier_offset + srs_pdu->bwp_start*12;
      int subcarrier_log = nr_srs_info->sc_list[sc_idx]-subcarrier_offset;
      if(subcarrier_log < 0) {
        subcarrier_log = subcarrier_log + frame_parms->ofdm_symbol_size;
      }
      if(sc_idx == 0) {
        LOG_I(NR_PHY,"________ Rx antenna %i ________\n", ant);
      }
      if(subcarrier_log%12 == 0) {
        LOG_I(NR_PHY,"::::::::::::: %i :::::::::::::\n", subcarrier_log/12);
      }
      LOG_I(NR_PHY,"(%i)  \t%i\t%i\n",
            subcarrier_log,
            (int16_t)(srs_received_signal[ant][nr_srs_info->sc_list[sc_idx]]&0xFFFF),
            (int16_t)((srs_received_signal[ant][nr_srs_info->sc_list[sc_idx]]>>16)&0xFFFF));
#endif
    }
  }
  return 0;
}