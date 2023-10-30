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

/*! \file prach_procedures.c
 * \brief Implementation of eNB procedures from 36.213 LTE specifications
 * \author R. Knopp, F. Kaltenberger, N. Nikaein, X. Foukas
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr,navid.nikaein@eurecom.fr, x.foukas@sms.ed.ac.uk
 * \note
 * \warning
 */

#include "PHY/defs_eNB.h"
#include "nfapi_interface.h"
#include "fapi_l1.h"
#include "nfapi_pnf.h"
#include "common/utils/LOG/log.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "common/utils/LOG/vcd_signal_dumper.h"


#include "assertions.h"

#include <time.h>



extern int oai_nfapi_rach_ind(nfapi_rach_indication_t *rach_ind);

void prach_procedures(PHY_VARS_eNB *eNB,
                      int br_flag ) {
  uint16_t max_preamble[4]={0},max_preamble_energy[4]={0},max_preamble_delay[4]={0},avg_preamble_energy[4]={0}; 
  uint16_t i;
  int frame,subframe;

  if (br_flag==1) {
    subframe = eNB->proc.subframe_prach_br;
    frame = eNB->proc.frame_prach_br;
    pthread_mutex_lock(&eNB->UL_INFO_mutex);
    eNB->UL_INFO.rach_ind_br.rach_indication_body.number_of_preambles=0;
    pthread_mutex_unlock(&eNB->UL_INFO_mutex);
  } else {
    pthread_mutex_lock(&eNB->UL_INFO_mutex);
    eNB->UL_INFO.rach_ind.rach_indication_body.number_of_preambles=0;
    pthread_mutex_unlock(&eNB->UL_INFO_mutex);
    subframe = eNB->proc.subframe_prach;
    frame = eNB->proc.frame_prach;
  }

  RU_t *ru;
  int aa=0;
  int ru_aa;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PRACH_RX,1);

  for (i=0; i<eNB->num_RU; i++) {
    ru=eNB->RU_list[i];

    for (ru_aa=0,aa=0; ru_aa<ru->nb_rx; ru_aa++,aa++) {
      eNB->prach_vars.rxsigF[0][aa] = eNB->RU_list[i]->prach_rxsigF[0][ru_aa];
      int ce_level;

      if (br_flag==1)
        for (ce_level=0; ce_level<4; ce_level++) eNB->prach_vars_br.rxsigF[ce_level][aa] = eNB->RU_list[i]->prach_rxsigF_br[ce_level][ru_aa];
    }
  }

  // run PRACH detection for CE-level 0 only for now when br_flag is set
  rx_prach(eNB,
           eNB->RU_list[0],
           &max_preamble[0],
           &max_preamble_energy[0],
           &max_preamble_delay[0],
           &avg_preamble_energy[0],
           frame,
           0,br_flag
          );
  LOG_D(PHY,"[RAPROC] Frame %d, subframe %d : BR %d  Most likely preamble %d, energy %d dB delay %d (prach_energy counter %d), threshold %d, I0 %d\n",
        frame,subframe,br_flag,
        max_preamble[0],
        max_preamble_energy[0]/10,
        max_preamble_delay[0],
        eNB->prach_energy_counter,
        eNB->measurements.prach_I0+eNB->prach_DTX_threshold,
        eNB->measurements.prach_I0);

  if (br_flag==1) {
    int             prach_mask;
    prach_mask = is_prach_subframe (&eNB->frame_parms, eNB->proc.frame_prach_br, eNB->proc.subframe_prach_br);
    eNB->UL_INFO.rach_ind_br.rach_indication_body.preamble_list = eNB->preamble_list_br;
    int             ind = 0;
    int             ce_level = 0;
    /* Save for later, it doesn't work
       for (int ind=0,ce_level=0;ce_level<4;ce_level++) {

       if ((eNB->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[ce_level]==1)&&
       (prach_mask&(1<<(1+ce_level)) > 0) && // prach is active and CE level has finished its repetitions
       (eNB->prach_vars_br.repetition_number[ce_level]==
       eNB->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[ce_level])) {

    */

    if (eNB->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[0] == 1) {
      if ((eNB->prach_energy_counter == 100) && (max_preamble_energy[0] > eNB->measurements.prach_I0 + eNB->prach_DTX_threshold_emtc[0])) {
        eNB->UL_INFO.rach_ind_br.rach_indication_body.number_of_preambles++;
        eNB->preamble_list_br[ind].preamble_rel8.timing_advance = max_preamble_delay[ind];      //
        eNB->preamble_list_br[ind].preamble_rel8.preamble = max_preamble[ind];
        // note: fid is implicitly 0 here, this is the rule for eMTC RA-RNTI from 36.321, Section 5.1.4
        eNB->preamble_list_br[ind].preamble_rel8.rnti = 1 + subframe + (60*(eNB->prach_vars_br.first_frame[ce_level] % 40));
        eNB->preamble_list_br[ind].instance_length = 0; //don't know exactly what this is
        eNB->preamble_list_br[ind].preamble_rel13.rach_resource_type = 1 + ce_level;    // CE Level
        LOG_I (PHY, "Filling NFAPI indication for RACH %d CELevel %d (mask %x) : TA %d, Preamble %d, rnti %x, rach_resource_type %d\n",
               ind,
               ce_level,
               prach_mask,
               eNB->preamble_list_br[ind].preamble_rel8.timing_advance,
               eNB->preamble_list_br[ind].preamble_rel8.preamble, eNB->preamble_list_br[ind].preamble_rel8.rnti, eNB->preamble_list_br[ind].preamble_rel13.rach_resource_type);
      }
    }

    /*
    ind++;
    }
    } */// ce_level
  } else {
    if ((eNB->prach_energy_counter == 100) &&
        (max_preamble_energy[0] > eNB->measurements.prach_I0+eNB->prach_DTX_threshold)) {
      LOG_D(PHY,"[eNB %d/%d][RAPROC] Frame %d, subframe %d Initiating RA procedure with preamble %d, energy %d.%d dB, delay %d\n",
            eNB->Mod_id,
            eNB->CC_id,
            frame,
            subframe,
            max_preamble[0],
            max_preamble_energy[0]/10,
            max_preamble_energy[0]%10,
            max_preamble_delay[0]);
      T(T_ENB_PHY_INITIATE_RA_PROCEDURE, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe),
        T_INT(max_preamble[0]), T_INT(max_preamble_energy[0]), T_INT(max_preamble_delay[0]));
      pthread_mutex_lock(&eNB->UL_INFO_mutex);
      eNB->UL_INFO.rach_ind.rach_indication_body.number_of_preambles  = 1;
      eNB->UL_INFO.rach_ind.rach_indication_body.preamble_list        = &eNB->preamble_list[0];
      eNB->UL_INFO.rach_ind.rach_indication_body.tl.tag               = NFAPI_RACH_INDICATION_BODY_TAG;
      eNB->UL_INFO.rach_ind.header.message_id                         = NFAPI_RACH_INDICATION;
      eNB->UL_INFO.rach_ind.sfn_sf                                    = frame<<4 | subframe;
      eNB->preamble_list[0].preamble_rel8.tl.tag                = NFAPI_PREAMBLE_REL8_TAG;
      eNB->preamble_list[0].preamble_rel8.timing_advance        = max_preamble_delay[0];
      eNB->preamble_list[0].preamble_rel8.preamble              = max_preamble[0];
      eNB->preamble_list[0].preamble_rel8.rnti                  = 1+subframe;  // note: fid is implicitly 0 here
      eNB->preamble_list[0].preamble_rel13.rach_resource_type   = 0;
      eNB->preamble_list[0].instance_length                     = 0; //don't know exactly what this is

      if (NFAPI_MODE==NFAPI_MODE_PNF) {  // If NFAPI PNF then we need to send the message to the VNF
        LOG_D(PHY,"Filling NFAPI indication for RACH : SFN_SF:%d TA %d, Preamble %d, rnti %x, rach_resource_type %d\n",
              NFAPI_SFNSF2DEC(eNB->UL_INFO.rach_ind.sfn_sf),
              eNB->preamble_list[0].preamble_rel8.timing_advance,
              eNB->preamble_list[0].preamble_rel8.preamble,
              eNB->preamble_list[0].preamble_rel8.rnti,
              eNB->preamble_list[0].preamble_rel13.rach_resource_type);
        oai_nfapi_rach_ind(&eNB->UL_INFO.rach_ind);
        eNB->UL_INFO.rach_ind.rach_indication_body.number_of_preambles = 0;
      }

      pthread_mutex_unlock(&eNB->UL_INFO_mutex);
    } // max_preamble_energy > prach_I0 + 100
    else {
      eNB->measurements.prach_I0 = ((eNB->measurements.prach_I0*900)>>10) + ((avg_preamble_energy[0]*124)>>10);

      if (frame==0) LOG_D(PHY,"prach_I0 = %d.%d dB\n",eNB->measurements.prach_I0/10,eNB->measurements.prach_I0%10);

      if (eNB->prach_energy_counter < 100) eNB->prach_energy_counter++;
    }
  } // else br_flag

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PRACH_RX,0);
}
