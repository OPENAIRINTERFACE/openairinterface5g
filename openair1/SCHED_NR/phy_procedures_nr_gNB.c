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
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_TRANSPORT/nr_transport.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_ESTIMATION/nr_ul_estimation.h"
#include "PHY/NR_UE_TRANSPORT/pucch_nr.h"
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
#include "executables/nr-softmodem.h"
#include "executables/softmodem-common.h"

#include "assertions.h"
#include "msc.h"

#include <time.h>

#include "intertask_interface.h"

uint8_t SSB_Table[38]={0,2,4,6,8,10,12,14,254,254,16,18,20,22,24,26,28,30,254,254,32,34,36,38,40,42,44,46,254,254,48,50,52,54,56,58,60,62};

extern uint8_t nfapi_mode;

void nr_set_ssb_first_subcarrier(nfapi_nr_config_request_scf_t *cfg, NR_DL_FRAME_PARMS *fp) {

  uint8_t sco = 0;
  if (((fp->freq_range == nr_FR1) && (cfg->ssb_table.ssb_subcarrier_offset.value<24)) ||
      ((fp->freq_range == nr_FR2) && (cfg->ssb_table.ssb_subcarrier_offset.value<12)) )
    sco = cfg->ssb_table.ssb_subcarrier_offset.value;

  fp->ssb_start_subcarrier = (12 * cfg->ssb_table.ssb_offset_point_a.value + sco);
  LOG_D(PHY, "SSB first subcarrier %d (%d,%d)\n", fp->ssb_start_subcarrier,cfg->ssb_table.ssb_offset_point_a.value,sco);
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

      if ((ssb_index<64) && ((fp->L_ssb >> (63-ssb_index)) & 0x01))  { // generating the ssb only if the bit of L_ssb at current ssb index is 1
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


  if (gNB->pdcch_pdu || gNB->ul_dci_pdu) {
    LOG_D(PHY, "[gNB %d] Frame %d slot %d Calling nr_generate_dci_top (number of UL/DL DCI %d/%d)\n",
	  gNB->Mod_id, frame, slot,
	  gNB->ul_dci_pdu==NULL?0:gNB->ul_dci_pdu->pdcch_pdu.pdcch_pdu_rel15.numDlDci,
	  gNB->pdcch_pdu==NULL?0:gNB->pdcch_pdu->pdcch_pdu_rel15.numDlDci);
  
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PDCCH_TX,1);

    nr_generate_dci_top(gNB->pdcch_pdu,
			gNB->ul_dci_pdu,
			gNB->nr_gold_pdcch_dmrs[slot],
			&gNB->common_vars.txdataF[0][txdataF_offset],
			AMP, *fp);
  
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_PDCCH_TX,0);
  }
 
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
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  nfapi_nr_pusch_pdu_t *pusch_pdu = &gNB->ulsch[ULSCH_id][0]->harq_processes[harq_pid]->ulsch_pdu;
  
  uint8_t ret;
  uint8_t l, number_dmrs_symbols = 0;
  uint32_t G;
  uint16_t start_symbol, number_symbols, nb_re_dmrs;

  start_symbol = pusch_pdu->start_symbol_index;
  number_symbols = pusch_pdu->nr_of_symbols;

  for (l = start_symbol; l < start_symbol + number_symbols; l++)
      number_dmrs_symbols += ((pusch_pdu->ul_dmrs_symb_pos)>>l)&0x01;

  nb_re_dmrs = ((pusch_pdu->dmrs_config_type == pusch_dmrs_type1)?6:4);

  G = nr_get_G(pusch_pdu->rb_size,
               number_symbols,
               nb_re_dmrs,
               number_dmrs_symbols, // number of dmrs symbols irrespective of single or double symbol dmrs
               pusch_pdu->qam_mod_order,
               pusch_pdu->nrOfLayers);


  //----------------------------------------------------------
  //------------------- ULSCH unscrambling -------------------
  //----------------------------------------------------------

  start_meas(&gNB->ulsch_unscrambling_stats);
  nr_ulsch_unscrambling(gNB->pusch_vars[ULSCH_id]->llr,
                        G,
                        0,
                        pusch_pdu->data_scrambling_id,
                        pusch_pdu->rnti);
  stop_meas(&gNB->ulsch_unscrambling_stats);
  //----------------------------------------------------------
  //--------------------- ULSCH decoding ---------------------
  //----------------------------------------------------------

  start_meas(&gNB->ulsch_decoding_stats);
  ret = nr_ulsch_decoding(gNB,
                          ULSCH_id,
                          gNB->pusch_vars[ULSCH_id]->llr,
                          frame_parms,
                          pusch_pdu,
                          frame_rx,
                          slot_rx,
                          harq_pid,
                          G);
  stop_meas(&gNB->ulsch_decoding_stats);

  if (ret > gNB->ulsch[ULSCH_id][0]->max_ldpc_iterations){
    LOG_I(PHY, "ULSCH %d in error\n",ULSCH_id);
    nr_fill_indication(gNB,frame_rx, slot_rx, ULSCH_id, harq_pid, 1);
  }
  else if(gNB->ulsch[ULSCH_id][0]->harq_processes[harq_pid]->b!=NULL){
    LOG_I(PHY, "ULSCH received ok \n");
    nr_fill_indication(gNB,frame_rx, slot_rx, ULSCH_id, harq_pid, 0);
  }
}


void nr_fill_indication(PHY_VARS_gNB *gNB, int frame, int slot_rx, int ULSCH_id, uint8_t harq_pid, uint8_t crc_flag) {

  pthread_mutex_lock(&gNB->UL_INFO_mutex);

  int timing_advance_update, cqi;
  int sync_pos;
  uint16_t mu = gNB->frame_parms.numerology_index;
  NR_gNB_ULSCH_t                       *ulsch                 = gNB->ulsch[ULSCH_id][0];
  NR_UL_gNB_HARQ_t                     *harq_process          = ulsch->harq_processes[harq_pid];

  nfapi_nr_pusch_pdu_t *pusch_pdu = &harq_process->ulsch_pdu;

  //  pdu->data                              = gNB->ulsch[ULSCH_id+1][0]->harq_processes[harq_pid]->b;
  sync_pos                               = nr_est_timing_advance_pusch(gNB, ULSCH_id); // estimate timing advance for MAC
  timing_advance_update                  = sync_pos * (1 << mu);                    // scale by the used scs numerology

  // scale the 16 factor in N_TA calculation in 38.213 section 4.2 according to the used FFT size
  switch (gNB->frame_parms.N_RB_DL) {
    case 106: timing_advance_update /= 16; break;
    case 217: timing_advance_update /= 32; break;
    case 245: timing_advance_update /= 32; break;
    case 273: timing_advance_update /= 32; break;
    case 66:  timing_advance_update /= 12; break;
    default: abort();
  }

  // put timing advance command in 0..63 range
  timing_advance_update += 31;

  if (timing_advance_update < 0)  timing_advance_update = 0;
  if (timing_advance_update > 63) timing_advance_update = 63;

  LOG_D(PHY, "Estimated timing advance PUSCH is  = %d, timing_advance_update is %d \n", sync_pos,timing_advance_update);

  // estimate UL_CQI for MAC (from antenna port 0 only)
  int SNRtimes10 = dB_fixed_times10(gNB->pusch_vars[ULSCH_id]->ulsch_power[0]) - 300;//(10*gNB->measurements.n0_power_dB[0]);

  if      (SNRtimes10 < -640) cqi=0;
  else if (SNRtimes10 >  635) cqi=255;
  else                        cqi=(640+SNRtimes10)/5;

  // crc indication
  uint16_t num_crc = gNB->UL_INFO.crc_ind.number_crcs;
  gNB->UL_INFO.crc_ind.crc_list = &gNB->crc_pdu_list[0];
  gNB->UL_INFO.crc_ind.sfn = frame;
  gNB->UL_INFO.crc_ind.slot = slot_rx;

  gNB->crc_pdu_list[num_crc].handle = pusch_pdu->handle;
  gNB->crc_pdu_list[num_crc].rnti = pusch_pdu->rnti;
  gNB->crc_pdu_list[num_crc].harq_id = harq_pid;
  gNB->crc_pdu_list[num_crc].tb_crc_status = crc_flag;
  gNB->crc_pdu_list[num_crc].num_cb = pusch_pdu->pusch_data.num_cb;
  gNB->crc_pdu_list[num_crc].ul_cqi = cqi;
  gNB->crc_pdu_list[num_crc].timing_advance = timing_advance_update;
  gNB->crc_pdu_list[num_crc].rssi = 0xffff; // invalid value as this is not yet computed

  gNB->UL_INFO.crc_ind.number_crcs++;

  // rx indication
  uint16_t num_rx = gNB->UL_INFO.rx_ind.number_of_pdus;
  gNB->UL_INFO.rx_ind.pdu_list = &gNB->rx_pdu_list[0];
  gNB->UL_INFO.rx_ind.sfn = frame;
  gNB->UL_INFO.rx_ind.slot = slot_rx;
  gNB->rx_pdu_list[num_rx].handle = pusch_pdu->handle;
  gNB->rx_pdu_list[num_rx].rnti = pusch_pdu->rnti;
  gNB->rx_pdu_list[num_rx].harq_id = harq_pid;
  gNB->rx_pdu_list[num_rx].ul_cqi = cqi;
  gNB->rx_pdu_list[num_rx].timing_advance = timing_advance_update;
  gNB->rx_pdu_list[num_rx].rssi = 0xffff; // invalid value as this is not yet computed
  if (crc_flag)
    gNB->rx_pdu_list[num_rx].pdu_length = 0;
  else {
    gNB->rx_pdu_list[num_rx].pdu_length = harq_process->TBS;
    gNB->rx_pdu_list[num_rx].pdu = harq_process->b;
  }

  gNB->UL_INFO.rx_ind.number_of_pdus++;

  pthread_mutex_unlock(&gNB->UL_INFO_mutex);
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

  nfapi_nr_ul_tti_request_t *UL_tti_req  = &gNB->UL_tti_req;
  int num_pdus = UL_tti_req->n_pdus;

  nfapi_nr_uci_indication_t *uci_indication = &gNB->uci_indication;
  uci_indication->sfn = frame_rx;
  uci_indication->slot = slot_rx;
  uci_indication->num_ucis = 0;


  LOG_D(PHY,"phy_procedures_gNB_uespec_RX frame %d, slot %d, num_pdus %d\n",frame_rx,slot_rx,num_pdus);

  gNB->UL_INFO.rx_ind.number_of_pdus = 0;
  gNB->UL_INFO.crc_ind.number_crcs = 0;

  for (int i = 0; i < num_pdus; i++) {
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
	//LOG_M("rxdataF_comp.m","rxF_comp",gNB->pusch_vars[0]->rxdataF_comp[0],6900,1,1);
	//LOG_M("rxdataF_ext.m","rxF_ext",gNB->pusch_vars[0]->rxdataF_ext[0],6900,1,1);
	nr_ulsch_procedures(gNB, frame_rx, slot_rx, ULSCH_id, harq_pid);
      }
      break;
    case NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE:
      {
	LOG_D(PHY,"frame %d, slot %d, Got NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE\n",frame_rx,slot_rx);
	
	nfapi_nr_pucch_pdu_t  *pucch_pdu = &UL_tti_req->pdus_list[i].pucch_pdu;
	switch (pucch_pdu->format_type) {
	case 0:
	  uci_indication->uci_list[uci_indication->num_ucis].pdu_type = NFAPI_NR_UCI_FORMAT_0_1_PDU_TYPE;
	  uci_indication->uci_list[uci_indication->num_ucis].pdu_size = sizeof(nfapi_nr_uci_pucch_pdu_format_0_1_t);
	  nfapi_nr_uci_pucch_pdu_format_0_1_t *uci_pdu_format0 = &uci_indication->uci_list[uci_indication->num_ucis].pucch_pdu_format_0_1;
	  
	  nr_decode_pucch0(gNB,
			   slot_rx,
			   uci_pdu_format0,
			   pucch_pdu);
	  
	  uci_indication->num_ucis += 1;
	  break;
	case 1:
	  break;
	case 2:
	  break;
	default:
	  AssertFatal(1==0,"Only PUCCH format 0,1 and 2 are currently supported\n");
	}
      }
    }
  }
}
