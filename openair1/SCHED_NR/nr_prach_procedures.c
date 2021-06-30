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
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
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


void L1_nr_prach_procedures(PHY_VARS_gNB *gNB,int frame,int slot) {

  uint16_t max_preamble[4]={0},max_preamble_energy[4]={0},max_preamble_delay[4]={0};


  uint8_t pdu_index = 0;
  RU_t *ru;
  int aa=0;
  int ru_aa;

 
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PRACH_RX,1);
  gNB->UL_INFO.rach_ind.sfn             = frame;
  gNB->UL_INFO.rach_ind.slot            = slot;
  gNB->UL_INFO.rach_ind.pdu_list        = gNB->prach_pdu_indication_list;
  gNB->UL_INFO.rach_ind.number_of_pdus  = 0;

  ru=gNB->RU_list[0];

  int prach_id=find_nr_prach(gNB,frame,slot,SEARCH_EXIST);
  
  if (prach_id>=0) {
    nfapi_nr_prach_pdu_t *prach_pdu = &gNB->prach_vars.list[prach_id].pdu;
    uint8_t prachStartSymbol;
    uint8_t N_dur = get_nr_prach_duration(prach_pdu->prach_format);
    
    /*
    uint16_t format,RA_sfn_index;
    uint8_t start_symbol,N_t_slot,N_dur,N_RA_slot,config_period;

    get_nr_prach_info_from_index(gNB->gNB_config.prach_config.prach_ConfigurationIndex.value,
				 frame,slot,
				 gNB->gNB_config.carrier_config.dl_frequency.value,
				 gNB->frame_parms.numerology_index,
				 gNB->frame_parms.frame_type,
				 &format,
				 &start_symbol,
				 &N_t_slot,
				 &N_dur,
				 &RA_sfn_index,
				 &N_RA_slot,
				 &config_period);
    */
    
    for(int prach_oc = 0; prach_oc < prach_pdu->num_prach_ocas; prach_oc++) {
      for (ru_aa=0,aa=0;ru_aa<ru->nb_rx;ru_aa++,aa++) {
	gNB->prach_vars.rxsigF[aa] = ru->prach_rxsigF[prach_oc][ru_aa];
      }

      prachStartSymbol = prach_pdu->prach_start_symbol+prach_oc*N_dur;
      //comment FK: the standard 38.211 section 5.3.2 has one extra term +14*N_RA_slot. This is because there prachStartSymbol is given wrt to start of the 15kHz slot or 60kHz slot. Here we work slot based, so this function is anyway only called in slots where there is PRACH. Its up to the MAC to schedule another PRACH PDU in the case there are there N_RA_slot \in {0,1}. 
     
      rx_nr_prach(gNB,
		  prach_pdu,
		  prach_oc,
		  frame,
		  slot,
		  &max_preamble[0],
		  &max_preamble_energy[0],
		  &max_preamble_delay[0]
		  );
      free_nr_prach_entry(gNB,prach_id);
      LOG_D(PHY,"[RAPROC] Frame %d, slot %d, occasion %d (prachStartSymbol %d) : Most likely preamble %d, energy %d.%d dB delay %d (prach_energy counter %d)\n",
	    frame,slot,prach_oc,prachStartSymbol,
	    max_preamble[0],
	    max_preamble_energy[0]/10,
            max_preamble_energy[0]%10,
	    max_preamble_delay[0],
	    gNB->prach_energy_counter);

      if ((gNB->prach_energy_counter == 100) && (max_preamble_energy[0] > gNB->measurements.prach_I0+gNB->prach_thres)) {
	
	LOG_I(PHY,"[gNB %d][RAPROC] Frame %d, slot %d Initiating RA procedure with preamble %d, energy %d.%d dB (I0 %d, thres %d), delay %d start symbol %u freq index %u\n",
	      gNB->Mod_id,
	      frame,
	      slot,
	      max_preamble[0],
	      max_preamble_energy[0]/10,
	      max_preamble_energy[0]%10,
              gNB->measurements.prach_I0,gNB->prach_thres,
	      max_preamble_delay[0],
	      prachStartSymbol,
	      prach_pdu->num_ra);
	
	T(T_ENB_PHY_INITIATE_RA_PROCEDURE, T_INT(gNB->Mod_id), T_INT(frame), T_INT(slot),
	  T_INT(max_preamble[0]), T_INT(max_preamble_energy[0]), T_INT(max_preamble_delay[0]));
	
	
	gNB->UL_INFO.rach_ind.number_of_pdus  += 1;
	
	gNB->prach_pdu_indication_list[pdu_index].phy_cell_id  = gNB->gNB_config.cell_config.phy_cell_id.value;
	gNB->prach_pdu_indication_list[pdu_index].symbol_index = prachStartSymbol; 
	gNB->prach_pdu_indication_list[pdu_index].slot_index   = slot;
	gNB->prach_pdu_indication_list[pdu_index].freq_index   = prach_pdu->num_ra;
	gNB->prach_pdu_indication_list[pdu_index].avg_rssi     = (max_preamble_energy[0]<631) ? (128+(max_preamble_energy[0]/5)) : 254;
	gNB->prach_pdu_indication_list[pdu_index].avg_snr      = 0xff; // invalid for now
	
	gNB->prach_pdu_indication_list[pdu_index].num_preamble                        = 1;
	gNB->prach_pdu_indication_list[pdu_index].preamble_list                       = gNB->preamble_list;
	gNB->prach_pdu_indication_list[pdu_index].preamble_list[0].preamble_index     = max_preamble[0];
	gNB->prach_pdu_indication_list[pdu_index].preamble_list[0].timing_advance     = max_preamble_delay[0];
	gNB->prach_pdu_indication_list[pdu_index].preamble_list[0].preamble_pwr       = 0xffffffff;
	pdu_index++;
      }
      gNB->measurements.prach_I0 = ((gNB->measurements.prach_I0*900)>>10) + ((max_preamble_energy[0]*124)>>10); 
      if (frame==0) LOG_I(PHY,"prach_I0 = %d.%d dB\n",gNB->measurements.prach_I0/10,gNB->measurements.prach_I0%10);
      if (gNB->prach_energy_counter < 100) gNB->prach_energy_counter++;
    } //if prach_id>0
  } //for NUMBER_OF_NR_PRACH_OCCASION_MAX
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PRACH_RX,0);
}

