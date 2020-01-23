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

#include "PHY/phy_extern.h"
#include "PHY/defs_gNB.h"
#include "sched_nr.h"
#include "PHY/NR_TRANSPORT/nr_transport.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_ESTIMATION/nr_ul_estimation.h"
#include "SCHED/sched_eNB.h"
#include "sched_nr.h"
#include "SCHED/sched_common_extern.h"
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_interface.h"
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_nr_interface.h"
#include "fapi_nr_l1.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "T.h"

#include "assertions.h"
#include "msc.h"

#include <time.h>

#if defined(ENABLE_ITTI)
  #include "intertask_interface.h"
#endif

uint8_t SSB_Table[38]={0,2,4,6,8,10,12,14,254,254,16,18,20,22,24,26,28,30,254,254,32,34,36,38,40,42,44,46,254,254,48,50,52,54,56,58,60,62};

extern uint8_t nfapi_mode;

void nr_set_ssb_first_subcarrier(nfapi_nr_config_request_scf_t *cfg, NR_DL_FRAME_PARMS *fp) {
  fp->ssb_start_subcarrier = (12 * cfg->ssb_table.ssb_offset_point_a.value + cfg->ssb_table.ssb_subcarrier_offset.value);
  LOG_D(PHY, "SSB first subcarrier %d (%d,%d)\n", fp->ssb_start_subcarrier,cfg->ssb_table.ssb_offset_point_a.value,cfg->ssb_table.ssb_subcarrier_offset.value);
}

void nr_common_signal_procedures (PHY_VARS_gNB *gNB,int frame, int slot) {

  NR_DL_FRAME_PARMS *fp=&gNB->frame_parms;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  int **txdataF = gNB->common_vars.txdataF;
  uint8_t ssb_index, n_hf;
  uint16_t ssb_start_symbol, rel_slot;
  int txdataF_offset = (slot%2)*fp->samples_per_slot_wCP;
  uint16_t slots_per_hf = (fp->slots_per_frame)>>1;

  n_hf = fp->half_frame_bit;

  // if SSB periodicity is 5ms, they are transmitted in both half frames
  if ( cfg->ssb_table.ssb_period.value == 0) {
    if (slot<slots_per_hf)
      n_hf=0;
    else
      n_hf=1;
  }

  // to set a effective slot number in the half frame where the SSB is supposed to be
  rel_slot = (n_hf)? (slot-slots_per_hf) : slot; 

  LOG_D(PHY,"common_signal_procedures: frame %d, slot %d\n",frame,slot);

  if(rel_slot<38 && rel_slot>=0)  { // there is no SSB beyond slot 37

    for (int i=0; i<2; i++)  {  // max two SSB per frame
      
      ssb_index = i + SSB_Table[rel_slot]; // computing the ssb_index

      if ((ssb_index<64) && ((fp->L_ssb >> ssb_index) & 0x01))  { // generating the ssb only if the bit of L_ssb at current ssb index is 1

        fp->ssb_index = ssb_index;
        int ssb_start_symbol_abs = nr_get_ssb_start_symbol(fp); // computing the starting symbol for current ssb
	ssb_start_symbol = ssb_start_symbol_abs % fp->symbols_per_slot;  // start symbol wrt slot

	nr_set_ssb_first_subcarrier(cfg, fp);  // setting the first subcarrier
	
	LOG_D(PHY,"SS TX: frame %d, slot %d, start_symbol %d\n",frame,slot, ssb_start_symbol);
	nr_generate_pss(gNB->d_pss, &txdataF[0][txdataF_offset], AMP, ssb_start_symbol, cfg, fp);
	nr_generate_sss(gNB->d_sss, &txdataF[0][txdataF_offset], AMP, ssb_start_symbol, cfg, fp);
	
        if (cfg->carrier_config.num_tx_ant.value <= 4)
	  nr_generate_pbch_dmrs(gNB->nr_gold_pbch_dmrs[n_hf][ssb_index&7],&txdataF[0][txdataF_offset], AMP, ssb_start_symbol, cfg, fp);
        else
	  nr_generate_pbch_dmrs(gNB->nr_gold_pbch_dmrs[0][ssb_index&7],&txdataF[0][txdataF_offset], AMP, ssb_start_symbol, cfg, fp);
	
	nr_generate_pbch(&gNB->pbch,
	                 &gNB->ssb_pdu,
	                 gNB->nr_pbch_interleaver,
			 &txdataF[0][txdataF_offset],
			 AMP,
			 ssb_start_symbol,
			 n_hf, frame, cfg, fp);

      }
    }
  }
}

void phy_procedures_gNB_TX(PHY_VARS_gNB *gNB,
                           int frame,int slot,
                           int do_meas) {
  int aa;
  NR_DL_FRAME_PARMS *fp=&gNB->frame_parms;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  int offset = gNB->CC_id;
  uint8_t ssb_frame_periodicity = 1;  // every how many frames SSB are generated
  int txdataF_offset = (slot%2)*fp->samples_per_slot_wCP;

  
  
  if (cfg->ssb_table.ssb_period.value > 1) 
    ssb_frame_periodicity = 1 <<(cfg->ssb_table.ssb_period.value -1) ; 

  if ((cfg->cell_config.frame_duplex_type.value == TDD) &&
      (nr_slot_select(cfg,frame,slot) == NR_UPLINK_SLOT)) return;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_TX+offset,1);

  if (do_meas==1) start_meas(&gNB->phy_proc_tx);

  // clear the transmit data array for the current subframe
  for (aa=0; aa<cfg->carrier_config.num_tx_ant.value; aa++) {
    memset(&gNB->common_vars.txdataF[aa][txdataF_offset],0,fp->samples_per_slot_wCP*sizeof(int32_t));
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_COMMON_TX,1);
  if (nfapi_mode == 0 || nfapi_mode == 1) { 
    if ((!(frame%ssb_frame_periodicity)))  // generate SSB only for given frames according to SSB periodicity
      nr_common_signal_procedures(gNB,frame, slot);
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_COMMON_TX,0);



  LOG_D(PHY, "[gNB %d] Frame %d slot %d \
    Calling nr_generate_dci_top (number of DCI %d)\n", gNB->Mod_id, frame, slot, gNB->pdcch_pdu->pdcch_pdu_rel15.numDlDci);
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PDCCH_TX,1);

  if (gNB->pdcch_pdu || gNB->ul_dci_pdu) nr_generate_dci_top(gNB->pdcch_pdu,
							     gNB->ul_dci_pdu,
							     gNB->nr_gold_pdcch_dmrs[slot],
							     &gNB->common_vars.txdataF[0][txdataF_offset],
							     AMP, *fp);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PDCCH_TX,0);
 
  LOG_D(PHY, "PDSCH generation started (%d)\n", gNB->num_pdsch_rnti);
  for (int i=0; i<gNB->num_pdsch_rnti; i++) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GENERATE_DLSCH,1);
    nr_generate_pdsch(gNB->dlsch[i][0],
		      gNB->nr_gold_pdsch_dmrs[slot],
		      gNB->common_vars.txdataF,
		      AMP, frame, slot, fp, 0,
		      &gNB->dlsch_encoding_stats,
		      &gNB->dlsch_scrambling_stats,
		      &gNB->dlsch_modulation_stats,
		      &gNB->tinput,
		      &gNB->tprep,
		      &gNB->tparity,
		      &gNB->toutput,
		      &gNB->dlsch_rate_matching_stats,
		      &gNB->dlsch_interleaving_stats,
		      &gNB->dlsch_segmentation_stats);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GENERATE_DLSCH,0);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_TX+offset,0);
}



/*

  if ((cfg->subframe_config.duplex_mode.value == TDD) && 
      ((nr_slot_select(fp,frame,slot)&NR_DOWNLINK_SLOT)==SF_DL)) return;

  //  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_ENB_RX,1);


  if (do_prach_rx(fp,frame,slot)) L1_nr_prach_procedures(gNB,frame,slot/fp->slots_per_subframe);
*/

void nr_ulsch_procedures(PHY_VARS_gNB *gNB, int frame_rx, int slot_rx, int ULSCH_id, uint8_t harq_pid)
{
  NR_DL_FRAME_PARMS                    *frame_parms           = &gNB->frame_parms;
  nfapi_nr_ul_config_ulsch_pdu         *rel15_ul              = &gNB->ulsch[ULSCH_id][0]->harq_processes[harq_pid]->ulsch_pdu;
  nfapi_nr_ul_config_ulsch_pdu_rel15_t *nfapi_ulsch_pdu_rel15 = &rel15_ul->ulsch_pdu_rel15;
  
  uint8_t ret;
  uint32_t G;
  int Nid_cell = 0; // [hna] shouldn't be a local variable (should be signaled)

  G = nr_get_G(nfapi_ulsch_pdu_rel15->number_rbs,
               nfapi_ulsch_pdu_rel15->number_symbols,
               nfapi_ulsch_pdu_rel15->nb_re_dmrs,
               nfapi_ulsch_pdu_rel15->length_dmrs,
               nfapi_ulsch_pdu_rel15->Qm,
               nfapi_ulsch_pdu_rel15->n_layers);

  //----------------------------------------------------------
  //------------------- ULSCH unscrambling -------------------
  //----------------------------------------------------------

  nr_ulsch_unscrambling(gNB->pusch_vars[ULSCH_id]->llr,
                        G,
                        0,
                        Nid_cell,
                        rel15_ul->rnti);

  //----------------------------------------------------------
  //--------------------- ULSCH decoding ---------------------
  //----------------------------------------------------------

  ret = nr_ulsch_decoding(gNB,
                    ULSCH_id,
                    gNB->pusch_vars[ULSCH_id]->llr,
                    frame_parms,
                    frame_rx,
                    nfapi_ulsch_pdu_rel15->number_symbols,
                    slot_rx,
                    harq_pid,
                    0);
        
  if (ret > gNB->ulsch[ULSCH_id][0]->max_ldpc_iterations)
    LOG_I(PHY, "ULSCH in error\n");
  else
    LOG_I(PHY, "ULSCH received ok\n");

}


void nr_fill_rx_indication(PHY_VARS_gNB *gNB, int frame, int slot_rx, int ULSCH_id, uint8_t harq_pid)
{
  // --------------------
  // [hna] TO BE CLEANED
  // --------------------

  // nfapi_rx_indication_pdu_t *pdu;

  int timing_advance_update;
  int sync_pos;

  // pthread_mutex_lock(&gNB->UL_INFO_mutex);

  // gNB->UL_INFO.rx_ind.sfn_sf                    = frame<<4| slot_rx;
  // gNB->UL_INFO.rx_ind.rx_indication_body.tl.tag = NFAPI_RX_INDICATION_BODY_TAG;

  // pdu                                    = &gNB->UL_INFO.rx_ind.rx_indication_body.rx_pdu_list[gNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus];

  // pdu->rx_ue_information.handle          = gNB->ulsch[UE_id+1][0]->handle;
  // pdu->rx_ue_information.tl.tag          = NFAPI_RX_UE_INFORMATION_TAG;
  // pdu->rx_ue_information.rnti            = gNB->ulsch[UE_id+1][0]->rnti;
  // pdu->rx_indication_rel8.tl.tag         = NFAPI_RX_INDICATION_REL8_TAG;
  // pdu->rx_indication_rel8.length         = gNB->ulsch[UE_id+1][0]->harq_processes[harq_pid]->TBS>>3;
  // pdu->rx_indication_rel8.offset         = 1;   // DJP - I dont understand - but broken unless 1 ????  0;  // filled in at the end of the UL_INFO formation
  // pdu->data                              = gNB->ulsch[UE_id+1][0]->harq_processes[harq_pid]->b;
  // estimate timing advance for MAC
  sync_pos                               = nr_est_timing_advance_pusch(gNB, ULSCH_id);
  timing_advance_update                  = sync_pos; // - gNB->frame_parms.nb_prefix_samples/4; //to check
  // printf("\x1B[33m" "timing_advance_update = %d\n" "\x1B[0m", timing_advance_update);

  switch (gNB->frame_parms.N_RB_DL) {
    // case 6:   /* nothing to do */          break;
    // case 15:  timing_advance_update /= 2;  break;
    // case 25:  timing_advance_update /= 4;  break;
    // case 50:  timing_advance_update /= 8;  break;
    // case 75:  timing_advance_update /= 12; break;
    case 106: timing_advance_update /= 16; break;
    case 217: timing_advance_update /= 32; break;
    case 273: timing_advance_update /= 32; break;
    case 66: timing_advance_update /= 12; break;
    default: abort();
  }

  // put timing advance command in 0..63 range
  timing_advance_update += 31;

  if (timing_advance_update < 0)  timing_advance_update = 0;
  if (timing_advance_update > 63) timing_advance_update = 63;

  // pdu->rx_indication_rel8.timing_advance = timing_advance_update;

  // estimate UL_CQI for MAC (from antenna port 0 only)
  // int SNRtimes10 = dB_fixed_times10(gNB->pusch_vars[UE_id]->ulsch_power[0]) - 300;//(10*gNB->measurements.n0_power_dB[0]);

  // if      (SNRtimes10 < -640) pdu->rx_indication_rel8.ul_cqi=0;
  // else if (SNRtimes10 >  635) pdu->rx_indication_rel8.ul_cqi=255;
  // else                        pdu->rx_indication_rel8.ul_cqi=(640+SNRtimes10)/5;

  // LOG_D(PHY,"[PUSCH %d] Frame %d Subframe %d Filling RX_indication with SNR %d (%d), timing_advance %d (update %d)\n",
  // harq_pid,frame,slot_rx,SNRtimes10,pdu->rx_indication_rel8.ul_cqi,pdu->rx_indication_rel8.timing_advance,
  // timing_advance_update);

  // gNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus++;
  // gNB->UL_INFO.rx_ind.sfn_sf = frame<<4 | slot_rx;

  // pthread_mutex_unlock(&gNB->UL_INFO_mutex);
}


void phy_procedures_gNB_common_RX(PHY_VARS_gNB *gNB, int frame_rx, int slot_rx) {

  uint8_t symbol;
  unsigned char aa;

  for(symbol = 0; symbol < NR_SYMBOLS_PER_SLOT; symbol++) {
    // nr_slot_fep_ul(gNB, symbol, proc->slot_rx, 0, 0);
    for (aa = 0; aa < gNB->frame_parms.nb_antennas_rx; aa++) {
      nr_slot_fep_ul(&gNB->frame_parms,
                     gNB->common_vars.rxdata[aa],
                     gNB->common_vars.rxdataF[aa],
                     symbol,
                     slot_rx,
                     0,
                     0);
    }
  }

}

void phy_procedures_gNB_uespec_RX(PHY_VARS_gNB *gNB, int frame_rx, int slot_rx) {

  nfapi_nr_ul_tti_request_t     *UL_tti_req  = &gNB->UL_tti_req;
  int num_pusch_pdu = UL_tti_req->n_pdus;

  LOG_D(PHY,"phy_procedures_gNB_uespec_RX frame %d, slot %d, num_pusch_pdu %d\n",frame_rx,slot_rx,num_pusch_pdu);
  
  for (int i = 0; i < num_pusch_pdu; i++) {

    switch (UL_tti_req->pdus_list[i].pdu_type) {
    case NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE:
      {
	LOG_D(PHY,"frame %d, slot %d, Got NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE\n",frame_rx,slot_rx);

	nfapi_nr_pusch_pdu_t  *pusch_pdu = &UL_tti_req->pdus_list[0].pusch_pdu;
	nr_fill_ulsch(gNB,frame_rx,slot_rx,pusch_pdu);      
	
	uint8_t ULSCH_id =  find_nr_ulsch(pusch_pdu->rnti,gNB,SEARCH_EXIST);
	uint8_t harq_pid = pusch_pdu->pusch_data.harq_process_id;
	uint8_t symbol_start = pusch_pdu->start_symbol_index;
	uint8_t symbol_end = symbol_start + pusch_pdu->nr_of_symbols;
	
	for(uint8_t symbol = symbol_start; symbol < symbol_end; symbol++) {
	  nr_rx_pusch(gNB, ULSCH_id, frame_rx, slot_rx, symbol, harq_pid);
	}
	//LOG_M("rxdataF_comp.m","rxF_comp",gNB->pusch_vars[UE_id]->rxdataF_comp[0],6900,1,1);
	//LOG_M("rxdataF_ext.m","rxF_ext",gNB->pusch_vars[UE_id]->rxdataF_ext[0],6900,1,1);
	nr_ulsch_procedures(gNB, frame_rx, slot_rx, ULSCH_id, harq_pid);
	nr_fill_rx_indication(gNB, frame_rx, slot_rx, ULSCH_id, harq_pid);  // indicate SDU to MAC
      }
    }
  }
}
