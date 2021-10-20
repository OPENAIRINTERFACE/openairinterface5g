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

#include "phy_init.h"
#include "common/utils/nr/nr_common.h"
#include "common/utils/LOG/log.h"

/// Subcarrier spacings in Hz indexed by numerology index
uint32_t nr_subcarrier_spacing[MAX_NUM_SUBCARRIER_SPACING] = {15e3, 30e3, 60e3, 120e3, 240e3};
uint16_t nr_slots_per_subframe[MAX_NUM_SUBCARRIER_SPACING] = {1, 2, 4, 8, 16};


void set_Lmax(NR_DL_FRAME_PARMS *fp) {
  // definition of Lmax according to ts 38.213 section 4.1
  if (fp->dl_CarrierFreq < 6e9) {
    if(fp->frame_type && (fp->ssb_type==2))
      fp->Lmax = (fp->dl_CarrierFreq < 2.4e9)? 4 : 8;
    else
      fp->Lmax = (fp->dl_CarrierFreq < 3e9)? 4 : 8;
  } else {
    fp->Lmax = 64;
  }
}


int nr_get_ssb_start_symbol(NR_DL_FRAME_PARMS *fp,uint8_t i_ssb) {

  int mu = fp->numerology_index;
  int symbol = 0;
  uint8_t n, n_temp;
  nr_ssb_type_e type = fp->ssb_type;
  int case_AC[2] = {2,8};
  int case_BD[4] = {4,8,16,20};
  int case_E[8] = {8, 12, 16, 20, 32, 36, 40, 44};

  switch(mu) {
    case NR_MU_0: // case A
      n = i_ssb >> 1;
      symbol = case_AC[i_ssb % 2] + 14*n;
      break;
    case NR_MU_1:
      if (type == 1){ // case B
        n = i_ssb >> 2;
        symbol = case_BD[i_ssb % 4] + 28*n;
       }
       if (type == 2){ // case C
         n = i_ssb >> 1;
         symbol = case_AC[i_ssb % 2] + 14*n;
       }
       break;
     case NR_MU_3: // case D
       n_temp = i_ssb >> 2;
       n = n_temp + (n_temp >> 2);
       symbol = case_BD[i_ssb % 4] + 28*n;
       break;
     case NR_MU_4:  // case E
       n_temp = i_ssb >> 3;
       n = n_temp + (n_temp >> 2);
       symbol = case_E[i_ssb % 8] + 56*n;
       break;
     default:
       AssertFatal(0==1, "Invalid numerology index %d for the synchronization block\n", mu);
  }

  return symbol;
}

void set_scs_parameters (NR_DL_FRAME_PARMS *fp, int mu, int N_RB_DL)
{
  switch(mu) {

    case NR_MU_0: //15kHz scs
      fp->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_0];
      fp->slots_per_subframe = nr_slots_per_subframe[NR_MU_0];
      fp->ssb_type = nr_ssb_type_A;
      break;

    case NR_MU_1: //30kHz scs
      fp->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_1];
      fp->slots_per_subframe = nr_slots_per_subframe[NR_MU_1];

      // selection of SS block pattern according to TS 38101-1 Table 5.4.3.3-1 for SCS 30kHz
      if (fp->nr_band == 5 || fp->nr_band == 66) 
        fp->ssb_type = nr_ssb_type_B;
      else{  
      	if (fp->nr_band == 41 || fp->nr_band == 38 || ( fp->nr_band > 76 && fp->nr_band < 80) )
	  fp->ssb_type = nr_ssb_type_C;
	else
	  AssertFatal(1==0,"NR Operating Band n%d not available for SS block SCS with mu=%d\n", fp->nr_band, mu);
      }
      break;

    case NR_MU_2: //60kHz scs
      fp->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_2];
      fp->slots_per_subframe = nr_slots_per_subframe[NR_MU_2];
      break;

    case NR_MU_3:
      fp->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_3];
      fp->slots_per_subframe = nr_slots_per_subframe[NR_MU_3];
      fp->ssb_type = nr_ssb_type_D;
      break;

    case NR_MU_4:
      fp->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_4];
      fp->slots_per_subframe = nr_slots_per_subframe[NR_MU_4];
      fp->ssb_type = nr_ssb_type_E;
      break;

    default:
      AssertFatal(1==0,"Invalid numerology index %d", mu);
  }

  if(fp->threequarter_fs)
    fp->ofdm_symbol_size = 3 * 128;
  else
    fp->ofdm_symbol_size = 4 * 128;

  while(fp->ofdm_symbol_size < N_RB_DL * 12)
    fp->ofdm_symbol_size <<= 1;

  fp->first_carrier_offset = fp->ofdm_symbol_size - (N_RB_DL * 12 / 2);
  fp->nb_prefix_samples    = fp->ofdm_symbol_size / 128 * 9;
  fp->nb_prefix_samples0   = fp->ofdm_symbol_size / 128 * (9 + (1 << mu));
  LOG_I(PHY,"Init: N_RB_DL %d, first_carrier_offset %d, nb_prefix_samples %d,nb_prefix_samples0 %d\n",
        N_RB_DL,fp->first_carrier_offset,fp->nb_prefix_samples,fp->nb_prefix_samples0);
}

uint32_t get_samples_per_slot(int slot, NR_DL_FRAME_PARMS* fp)
{
  uint32_t samp_count;

  if(fp->numerology_index == 0)
    samp_count = fp->samples_per_subframe;
  else
    samp_count = (slot%(fp->slots_per_subframe/2)) ? fp->samples_per_slotN0 : fp->samples_per_slot0;

  return samp_count;
}


uint32_t get_slot_from_timestamp(openair0_timestamp timestamp_rx, NR_DL_FRAME_PARMS* fp)
{
   uint32_t slot_idx = 0;
   int samples_till_the_slot = 0;
   timestamp_rx = timestamp_rx%fp->samples_per_frame;

    while (timestamp_rx > samples_till_the_slot) {
        samples_till_the_slot += fp->get_samples_per_slot(slot_idx,fp);
        slot_idx++;
     }
   return slot_idx; 
}

uint32_t get_samples_slot_timestamp(int slot, NR_DL_FRAME_PARMS* fp, uint8_t sl_ahead)
{
  uint32_t samp_count = 0;

  if(!sl_ahead) {
    for(uint8_t idx_slot = 0; idx_slot < slot; idx_slot++)
      samp_count += fp->get_samples_per_slot(idx_slot, fp);
  } else {
    for(uint8_t idx_slot = slot; idx_slot < slot+sl_ahead; idx_slot++)
      samp_count += fp->get_samples_per_slot(idx_slot, fp);
  }
  return samp_count;
}

int nr_init_frame_parms(nfapi_nr_config_request_scf_t* cfg,
                        NR_DL_FRAME_PARMS *fp)
{

  AssertFatal (cfg != NULL, "%s %s:%i Null pointer to cfg %p!\n", __FUNCTION__, __FILE__, __LINE__, cfg);

  fp->frame_type = cfg->cell_config.frame_duplex_type.value;
  fp->L_ssb = (((uint64_t) cfg->ssb_table.ssb_mask_list[0].ssb_mask.value)<<32) | cfg->ssb_table.ssb_mask_list[1].ssb_mask.value ;
  fp->N_RB_DL = cfg->carrier_config.dl_grid_size[cfg->ssb_config.scs_common.value].value;
  fp->N_RB_UL = cfg->carrier_config.ul_grid_size[cfg->ssb_config.scs_common.value].value;

  int Ncp = NFAPI_CP_NORMAL;
  int mu = cfg->ssb_config.scs_common.value;

#if DISABLE_LOG_X
  printf("Initializing frame parms for mu %d, N_RB %d, Ncp %d\n",mu, fp->N_RB_DL, Ncp);
#else
  LOG_I(PHY,"Initializing frame parms for mu %d, N_RB %d, Ncp %d\n",mu, fp->N_RB_DL, Ncp);
#endif

  if (Ncp == NFAPI_CP_EXTENDED)
    AssertFatal(mu == NR_MU_2,"Invalid cyclic prefix %d for numerology index %d\n", Ncp, mu);

  fp->half_frame_bit = 0;  // half frame bit initialized to 0 here
  fp->numerology_index = mu;

  set_scs_parameters(fp, mu, fp->N_RB_DL);

  fp->slots_per_frame = 10* fp->slots_per_subframe;

  fp->nb_antennas_rx = cfg->carrier_config.num_rx_ant.value;      // It denotes the number of rx antennas at gNB
  fp->nb_antennas_tx = cfg->carrier_config.num_tx_ant.value;      // It corresponds to pdsch_AntennaPorts (logical antenna ports)

  fp->symbols_per_slot = ((Ncp == NORMAL)? 14 : 12); // to redefine for different slot formats
  fp->samples_per_subframe_wCP = fp->ofdm_symbol_size * fp->symbols_per_slot * fp->slots_per_subframe;
  fp->samples_per_frame_wCP = 10 * fp->samples_per_subframe_wCP;
  fp->samples_per_slot_wCP = fp->symbols_per_slot*fp->ofdm_symbol_size; 
  fp->samples_per_slotN0 = (fp->nb_prefix_samples + fp->ofdm_symbol_size) * fp->symbols_per_slot;
  fp->samples_per_slot0 = fp->nb_prefix_samples0 + ((fp->symbols_per_slot-1)*fp->nb_prefix_samples) + (fp->symbols_per_slot*fp->ofdm_symbol_size); 
  fp->samples_per_subframe = (fp->nb_prefix_samples0 + fp->ofdm_symbol_size) * 2 + 
                             (fp->nb_prefix_samples + fp->ofdm_symbol_size) * (fp->symbols_per_slot * fp->slots_per_subframe - 2); 
  fp->get_samples_per_slot = &get_samples_per_slot;
  fp->get_samples_slot_timestamp = &get_samples_slot_timestamp;
  fp->get_slot_from_timestamp = &get_slot_from_timestamp;
  fp->samples_per_frame = 10 * fp->samples_per_subframe;
  fp->freq_range = (fp->dl_CarrierFreq < 6e9)? nr_FR1 : nr_FR2;

  fp->Ncp = Ncp;

  set_Lmax(fp);

  fp->N_ssb = 0;
  int num_tx_ant = cfg->carrier_config.num_tx_ant.value;

  for (int p=0; p<num_tx_ant; p++)
    fp->N_ssb += ((fp->L_ssb >> (63-p)) & 0x01);

  return 0;

}

int nr_init_frame_parms_ue(NR_DL_FRAME_PARMS *fp,
                           fapi_nr_config_request_t* config,
                           uint16_t nr_band)
{

  uint8_t nb_ant_ports_gNB  = 1;
  uint8_t tdd_cfg           = 3;
  uint8_t Nid_cell          = 0;
  int     Ncp               = NORMAL;

  if(fp->nb_antennas_rx == 0)
    fp->nb_antennas_rx = 1;
  if(fp->nb_antennas_tx == 0)
    fp->nb_antennas_tx = 1;

  // default values until overwritten by RRCConnectionReconfiguration
  fp->nb_antenna_ports_gNB = nb_ant_ports_gNB;
  fp->tdd_config           = tdd_cfg;
  fp->Nid_cell             = Nid_cell;
  fp->nr_band              = nr_band;

  LOG_I(PHY, "Initializing frame parms: set nb_antenna_ports_gNB %d, tdd_config, %d, Nid_cell %d\n", fp->nb_antenna_ports_gNB, fp->tdd_config, fp->Nid_cell);

  uint64_t dl_bw_khz = (12*config->carrier_config.dl_grid_size[config->ssb_config.scs_common])*(15<<config->ssb_config.scs_common);
  fp->dl_CarrierFreq = ((dl_bw_khz>>1) + config->carrier_config.dl_frequency)*1000 ;

  LOG_D(PHY,"dl_bw_kHz %lu\n",dl_bw_khz);
  LOG_D(PHY,"dl_CarrierFreq %lu\n",fp->dl_CarrierFreq);

  uint64_t ul_bw_khz = (12*config->carrier_config.ul_grid_size[config->ssb_config.scs_common])*(15<<config->ssb_config.scs_common);
  fp->ul_CarrierFreq = ((ul_bw_khz>>1) + config->carrier_config.uplink_frequency)*1000 ;

  fp->numerology_index = config->ssb_config.scs_common;
  fp->N_RB_UL = config->carrier_config.ul_grid_size[fp->numerology_index];
  fp->N_RB_DL = config->carrier_config.dl_grid_size[fp->numerology_index];

  fp->frame_type = get_frame_type(fp->nr_band, fp->numerology_index);
  int32_t uplink_frequency_offset = get_delta_duplex(fp->nr_band, fp->numerology_index);
  uplink_frequency_offset *= 1000;

  LOG_I(PHY, "Initializing frame parms: DL frequency %lu Hz, UL frequency %lu Hz: band %d, uldl offset %d Hz\n", fp->dl_CarrierFreq, fp->ul_CarrierFreq, fp->nr_band, uplink_frequency_offset);

  AssertFatal(fp->frame_type==config->cell_config.frame_duplex_type, "Invalid duplex type (frame_type %d,cell_config.frame_duplex_type %d) in config request file for band %d\n", fp->frame_type,config->cell_config.frame_duplex_type,fp->nr_band);

  AssertFatal(fp->ul_CarrierFreq == (fp->dl_CarrierFreq + uplink_frequency_offset), "Disagreement in uplink frequency for band %d: ul_CarrierFreq = %lu Hz vs expected %lu Hz\n", fp->nr_band, fp->ul_CarrierFreq, fp->dl_CarrierFreq + uplink_frequency_offset);

#if DISABLE_LOG_X
  printf("Initializing UE frame parms for mu %d, N_RB %d, Ncp %d\n",fp->numerology_index, fp->N_RB_DL, Ncp);
#else
  LOG_I(PHY,"Initializing frame parms for mu %d, N_RB %d, Ncp %d\n",fp->numerology_index, fp->N_RB_DL, Ncp);
#endif

  if (Ncp == NFAPI_CP_EXTENDED)
    AssertFatal(fp->numerology_index == NR_MU_2,"Invalid cyclic prefix %d for numerology index %d\n", Ncp, fp->numerology_index);

  fp->Ncp = Ncp;

  set_scs_parameters(fp, fp->numerology_index, fp->N_RB_DL);

  fp->slots_per_frame = 10* fp->slots_per_subframe;
  fp->symbols_per_slot = ((Ncp == NORMAL)? 14 : 12); // to redefine for different slot formats
  fp->samples_per_subframe_wCP = fp->ofdm_symbol_size * fp->symbols_per_slot * fp->slots_per_subframe;
  fp->samples_per_frame_wCP = 10 * fp->samples_per_subframe_wCP;
  fp->samples_per_slot_wCP = fp->symbols_per_slot*fp->ofdm_symbol_size; 
  fp->samples_per_slotN0 = (fp->nb_prefix_samples + fp->ofdm_symbol_size) * fp->symbols_per_slot;
  fp->samples_per_slot0 = fp->nb_prefix_samples0 + ((fp->symbols_per_slot-1)*fp->nb_prefix_samples) + (fp->symbols_per_slot*fp->ofdm_symbol_size); 
  fp->samples_per_subframe = (fp->nb_prefix_samples0 + fp->ofdm_symbol_size) * 2 + 
                             (fp->nb_prefix_samples + fp->ofdm_symbol_size) * (fp->symbols_per_slot * fp->slots_per_subframe - 2); 
  fp->get_samples_per_slot = &get_samples_per_slot;
  fp->get_samples_slot_timestamp = &get_samples_slot_timestamp;
  fp->samples_per_frame = 10 * fp->samples_per_subframe;
  fp->freq_range = (fp->dl_CarrierFreq < 6e9)? nr_FR1 : nr_FR2;

  uint8_t sco = 0;
  if (((fp->freq_range == nr_FR1) && (config->ssb_table.ssb_subcarrier_offset<24)) ||
      ((fp->freq_range == nr_FR2) && (config->ssb_table.ssb_subcarrier_offset<12)) )
    sco = config->ssb_table.ssb_subcarrier_offset;

  fp->ssb_start_subcarrier = (12 * config->ssb_table.ssb_offset_point_a + sco);
  set_Lmax(fp);

  fp->L_ssb = (((uint64_t) config->ssb_table.ssb_mask_list[0].ssb_mask)<<32) | config->ssb_table.ssb_mask_list[1].ssb_mask;
  
  fp->N_ssb = 0;
  for (int p=0; p<fp->Lmax; p++)
    fp->N_ssb += ((fp->L_ssb >> (63-p)) & 0x01);

  return 0;
}

void nr_init_frame_parms_ue_sa(NR_DL_FRAME_PARMS *frame_parms, uint64_t downlink_frequency, int32_t delta_duplex, uint8_t mu, uint16_t nr_band) {

  LOG_I(PHY,"SA init parameters. DL freq %lu UL offset %d SSB numerology %d N_RB_DL %d\n",
        downlink_frequency,
        delta_duplex,
        mu,
        frame_parms->N_RB_DL);

  frame_parms->numerology_index = mu;
  frame_parms->dl_CarrierFreq = downlink_frequency;
  frame_parms->ul_CarrierFreq = downlink_frequency + delta_duplex;
  frame_parms->freq_range = (frame_parms->dl_CarrierFreq < 6e9)? nr_FR1 : nr_FR2;
  frame_parms->N_RB_UL = frame_parms->N_RB_DL;

  frame_parms->nr_band = nr_band;
  frame_parms->frame_type = get_frame_type(frame_parms->nr_band, frame_parms->numerology_index);

  frame_parms->Ncp = NORMAL;
  set_scs_parameters(frame_parms, frame_parms->numerology_index, frame_parms->N_RB_DL);
  set_Lmax(frame_parms);

  frame_parms->slots_per_frame = 10* frame_parms->slots_per_subframe;
  frame_parms->symbols_per_slot = ((frame_parms->Ncp == NORMAL)? 14 : 12); // to redefine for different slot formats
  frame_parms->samples_per_subframe_wCP = frame_parms->ofdm_symbol_size * frame_parms->symbols_per_slot * frame_parms->slots_per_subframe;
  frame_parms->samples_per_frame_wCP = 10 * frame_parms->samples_per_subframe_wCP;
  frame_parms->samples_per_slot_wCP = frame_parms->symbols_per_slot*frame_parms->ofdm_symbol_size;
  frame_parms->samples_per_slotN0 = (frame_parms->nb_prefix_samples + frame_parms->ofdm_symbol_size) * frame_parms->symbols_per_slot;
  frame_parms->samples_per_slot0 = frame_parms->nb_prefix_samples0 + ((frame_parms->symbols_per_slot-1)*frame_parms->nb_prefix_samples) + (frame_parms->symbols_per_slot*frame_parms->ofdm_symbol_size);
  frame_parms->samples_per_subframe = (frame_parms->nb_prefix_samples0 + frame_parms->ofdm_symbol_size) * 2 +
                             (frame_parms->nb_prefix_samples + frame_parms->ofdm_symbol_size) * (frame_parms->symbols_per_slot * frame_parms->slots_per_subframe - 2);
  frame_parms->get_samples_per_slot = &get_samples_per_slot;
  frame_parms->get_samples_slot_timestamp = &get_samples_slot_timestamp;
  frame_parms->samples_per_frame = 10 * frame_parms->samples_per_subframe;

}


void nr_dump_frame_parms(NR_DL_FRAME_PARMS *fp)
{
  LOG_I(PHY,"fp->scs=%d\n",fp->subcarrier_spacing);
  LOG_I(PHY,"fp->ofdm_symbol_size=%d\n",fp->ofdm_symbol_size);
  LOG_I(PHY,"fp->nb_prefix_samples0=%d\n",fp->nb_prefix_samples0);
  LOG_I(PHY,"fp->nb_prefix_samples=%d\n",fp->nb_prefix_samples);
  LOG_I(PHY,"fp->slots_per_subframe=%d\n",fp->slots_per_subframe);
  LOG_I(PHY,"fp->samples_per_subframe_wCP=%d\n",fp->samples_per_subframe_wCP);
  LOG_I(PHY,"fp->samples_per_frame_wCP=%d\n",fp->samples_per_frame_wCP);
  LOG_I(PHY,"fp->samples_per_subframe=%d\n",fp->samples_per_subframe);
  LOG_I(PHY,"fp->samples_per_frame=%d\n",fp->samples_per_frame);
  LOG_I(PHY,"fp->dl_CarrierFreq=%lu\n",fp->dl_CarrierFreq);
  LOG_I(PHY,"fp->ul_CarrierFreq=%lu\n",fp->ul_CarrierFreq);
}



