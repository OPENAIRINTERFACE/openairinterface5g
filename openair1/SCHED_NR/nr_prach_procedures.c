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

/*! \file nr_prach_procedures.c
 * \brief Implementation of gNB prach procedures from 38.213 LTE specifications
 * \author R. Knopp, 
 * \date 2019
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */

#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "PHY/NR_TRANSPORT/nr_transport.h"
#include "nfapi_nr_interface_scf.h"
#include "fapi_nr_l1.h"
#include "nfapi_pnf.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"


#include "assertions.h"
#include "msc.h"

#include <time.h>

#include "intertask_interface.h"

extern uint8_t nfapi_mode;

extern int oai_nfapi_nr_rach_ind(nfapi_rach_indication_t *rach_ind);


void L1_nr_prach_procedures(PHY_VARS_gNB *gNB,int frame,int slot,
			    nfapi_nr_prach_pdu_t *prach_pdu) {

  uint16_t max_preamble[4]={0},max_preamble_energy[4]={0},max_preamble_delay[4]={0};
  uint16_t i;


  gNB->UL_INFO.rach_ind.number_of_pdus=0;

  RU_t *ru;
  int aa=0;
  int ru_aa;

 
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PRACH_RX,1);



  for (i=0;i<gNB->num_RU;i++) {
    ru=gNB->RU_list[i];
    for (ru_aa=0,aa=0;ru_aa<ru->nb_rx;ru_aa++,aa++) {
      gNB->prach_vars.rxsigF[aa] = gNB->RU_list[i]->prach_rxsigF[ru_aa];
    }
  }


 /* rx_nr_prach(gNB,
	      prach_pdu,
	      frame,
	      slot,
	      &max_preamble[0],
	      &max_preamble_energy[0],
	      &max_preamble_delay[0]
	      );*/

  LOG_D(PHY,"[RAPROC] Frame %d, slot %d : Most likely preamble %d, energy %d dB delay %d (prach_energy counter %d)\n",
        frame,slot,
        max_preamble[0],
        max_preamble_energy[0]/10,
        max_preamble_delay[0],
	gNB->prach_energy_counter);

  if ((gNB->prach_energy_counter == 100) && 
      (max_preamble_energy[0] > gNB->measurements.prach_I0+100)) {
    
    LOG_I(PHY,"[gNB %d][RAPROC] Frame %d, slot %d Initiating RA procedure with preamble %d, energy %d.%d dB, delay %d\n",
	  gNB->Mod_id,
	  frame,
	  slot,
	  max_preamble[0],
	  max_preamble_energy[0]/10,
	  max_preamble_energy[0]%10,
	  max_preamble_delay[0]);
    
    T(T_ENB_PHY_INITIATE_RA_PROCEDURE, T_INT(gNB->Mod_id), T_INT(frame), T_INT(slot),
      T_INT(max_preamble[0]), T_INT(max_preamble_energy[0]), T_INT(max_preamble_delay[0]));
    
    
    gNB->UL_INFO.rach_ind.number_of_pdus  = 1;
    gNB->UL_INFO.rach_ind.pdu_list        = &gNB->prach_pdu_indication_list[0];
    gNB->UL_INFO.rach_ind.sfn                                    = frame;
    gNB->UL_INFO.rach_ind.slot            = slot;
    
    gNB->prach_pdu_indication_list[0].phy_cell_id  = gNB->gNB_config.cell_config.phy_cell_id.value;
    gNB->prach_pdu_indication_list[0].symbol_index = prach_pdu->prach_start_symbol;  // FIXME to be changed for multi-ssb (this is only the start symbol of first occasion)
    gNB->prach_pdu_indication_list[0].slot_index   = slot;
    gNB->prach_pdu_indication_list[0].freq_index   = prach_pdu->num_ra;
    gNB->prach_pdu_indication_list[0].avg_rssi     = (max_preamble_energy[0]<631) ? (128+(max_preamble_energy[0]/5)) : 254;
    gNB->prach_pdu_indication_list[0].avg_snr      = 0xff; // invalid for now


    gNB->prach_pdu_indication_list[0].num_preamble                        = 1;
    gNB->prach_pdu_indication_list[0].preamble_list                       = gNB->preamble_list;
    gNB->prach_pdu_indication_list[0].preamble_list[0].preamble_index     = max_preamble[0];
    gNB->prach_pdu_indication_list[0].preamble_list[0].timing_advance     = max_preamble_delay[0];
    gNB->prach_pdu_indication_list[0].preamble_list[0].preamble_pwr       = 0xffffffff;
  }    
  gNB->measurements.prach_I0 = ((gNB->measurements.prach_I0*900)>>10) + ((max_preamble_energy[0]*124)>>10); 
  if (frame==0) LOG_I(PHY,"prach_I0 = %d.%d dB\n",gNB->measurements.prach_I0/10,gNB->measurements.prach_I0%10);
  if (gNB->prach_energy_counter < 100) gNB->prach_energy_counter++;
  

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PRACH_RX,0);
}

