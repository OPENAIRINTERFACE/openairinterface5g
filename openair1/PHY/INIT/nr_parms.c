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
#include "common/utils/LOG/log.h"

/// Subcarrier spacings in Hz indexed by numerology index
uint32_t nr_subcarrier_spacing[MAX_NUM_SUBCARRIER_SPACING] = {15e3, 30e3, 60e3, 120e3, 240e3};
uint16_t nr_slots_per_subframe[MAX_NUM_SUBCARRIER_SPACING] = {1, 2, 4, 16, 32};


int nr_get_ssb_start_symbol(NR_DL_FRAME_PARMS *fp, uint8_t i_ssb, uint8_t half_frame_index)
{

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

  if (half_frame_index)
    symbol += (5 * fp->symbols_per_slot * fp->slots_per_subframe);

  return symbol;
}

int nr_is_ssb_slot(nfapi_nr_config_request_t *cfg, int slot, int frame)
{

  uint8_t n_hf;
  uint16_t p,mu,hf_slots;
  uint64_t ssb_map;
  int rel_slot;

  mu = cfg->subframe_config.numerology_index_mu.value;

  ssb_map = cfg->sch_config.ssb_scg_position_in_burst.value;
  p = cfg->sch_config.ssb_periodicity.value;
  n_hf = cfg->sch_config.half_frame_index.value;

  // if SSB periodicity is 5ms, they are transmitted in both half frames
  if ( (p>10) && (frame%(p/10)) )  
    return 0;
  else {
    hf_slots = (10<<mu)>>1; // number of slots per half frame
    // if SSB periodicity is 5ms, they are transmitted in both half frames
    if ( p == 5) {
      if (slot<hf_slots) 
      n_hf=0;
    else
      n_hf=1;
  }

  // to set a effective slot number between 0 to 9 in the half frame where the SSB is supposed to be
    rel_slot = (n_hf)? (slot-hf_slots) : slot;


    return ( ((ssb_map >> rel_slot*2) & 0x01) || ((ssb_map >> (1+rel_slot*2)) & 0x01) ); 
  }
}


int nr_init_frame_parms0(NR_DL_FRAME_PARMS *fp,
			 int mu,
			 int Ncp,
			 int N_RB_DL)

{

#if DISABLE_LOG_X
  printf("Initializing frame parms for mu %d, N_RB %d, Ncp %d\n",mu, N_RB_DL, Ncp);
#else
  LOG_I(PHY,"Initializing frame parms for mu %d, N_RB %d, Ncp %d\n",mu, N_RB_DL, Ncp);
#endif

  if (Ncp == NFAPI_CP_EXTENDED)
    AssertFatal(mu == NR_MU_2,"Invalid cyclic prefix %d for numerology index %d\n", Ncp, mu);

  fp->numerology_index = mu;
  fp->Ncp = Ncp;
  fp->N_RB_DL = N_RB_DL;

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
      if (fp->eutra_band == 5 || fp->eutra_band == 66) 
	      fp->ssb_type = nr_ssb_type_B;
      else{  
      	if (fp->eutra_band == 41 || ( fp->eutra_band > 76 && fp->eutra_band < 80) )
		fp->ssb_type = nr_ssb_type_C;
	else
		AssertFatal(1==0,"NR Operating Band n%d not available for SS block SCS with mu=%d\n", fp->eutra_band, mu);
      }

      switch(N_RB_DL){
        case 11:
        case 24:
        case 38:
        case 78:
        case 51:
        case 65:

        case 106: //40 MHz
          if (fp->threequarter_fs) {
            fp->ofdm_symbol_size = 1536;
            fp->first_carrier_offset = 900; //1536 - ( (106*12) / 2 )
            fp->nb_prefix_samples0 = 132;
            fp->nb_prefix_samples = 108;
          }
          else {
            fp->ofdm_symbol_size = 2048;
            fp->first_carrier_offset = 1412; //2048 - ( (106*12) / 2 )
            fp->nb_prefix_samples0 = 176;
            fp->nb_prefix_samples = 144;
          }
          break;

        case 133:
        case 162:
        case 189:

        case 217: //80 MHz
          if (fp->threequarter_fs) {
            fp->ofdm_symbol_size = 3072;
            fp->first_carrier_offset = 1770; //3072 - ( (217*12) / 2 )
            fp->nb_prefix_samples0 = 264;
            fp->nb_prefix_samples = 216;
          }
	  else {
	    fp->ofdm_symbol_size = 4096;
	    fp->first_carrier_offset = 2794; //4096 - ( (217*12) / 2 )
	    fp->nb_prefix_samples0 = 352;
	    fp->nb_prefix_samples = 288;
	  }
          break;

        case 245:
	  AssertFatal(fp->threequarter_fs==0,"3/4 sampling impossible for N_RB %d and MU %d\n",N_RB_DL,mu); 
	  fp->ofdm_symbol_size = 4096;
	  fp->first_carrier_offset = 2626; //4096 - ( (245*12) / 2 )
	  fp->nb_prefix_samples0 = 352;
	  fp->nb_prefix_samples = 288;
	  break;
        case 273:
	  AssertFatal(fp->threequarter_fs==0,"3/4 sampling impossible for N_RB %d and MU %d\n",N_RB_DL,mu); 
	  fp->ofdm_symbol_size = 4096;
	  fp->first_carrier_offset = 2458; //4096 - ( (273*12) / 2 )
	  fp->nb_prefix_samples0 = 352;
	  fp->nb_prefix_samples = 288;
	  break;
      default:
        AssertFatal(1==0,"Number of resource blocks %d undefined for mu %d, frame parms = %p\n", N_RB_DL, mu, fp);
      }
      break;

    case NR_MU_2: //60kHz scs
      fp->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_2];
      fp->slots_per_subframe = nr_slots_per_subframe[NR_MU_2];

      switch(N_RB_DL){ //FR1 bands only
        case 11:
        case 18:
        case 38:
        case 24:
        case 31:
        case 51:
        case 65:
        case 79:
        case 93:
        case 107:
        case 121:
        case 135:
      default:
        AssertFatal(1==0,"Number of resource blocks %d undefined for mu %d, frame parms = %p\n", N_RB_DL, mu, fp);
      }
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

  fp->slots_per_frame = 10* fp->slots_per_subframe;

  fp->nb_antenna_ports_eNB = 1; // default value until overwritten by RRCConnectionReconfiguration
  fp->nb_antennas_rx = 1; // default value until overwritten by RRCConnectionReconfiguration
  fp->nb_antennas_tx = 1; // default value until overwritten by RRCConnectionReconfiguration

  fp->symbols_per_slot = ((Ncp == NORMAL)? 14 : 12); // to redefine for different slot formats
  fp->samples_per_subframe_wCP = fp->ofdm_symbol_size * fp->symbols_per_slot * fp->slots_per_subframe;
  fp->samples_per_frame_wCP = 10 * fp->samples_per_subframe_wCP;
  fp->samples_per_slot_wCP = fp->symbols_per_slot*fp->ofdm_symbol_size; 
  fp->samples_per_slot = fp->nb_prefix_samples0 + ((fp->symbols_per_slot-1)*fp->nb_prefix_samples) + (fp->symbols_per_slot*fp->ofdm_symbol_size); 
  fp->samples_per_subframe = (fp->samples_per_subframe_wCP + (fp->nb_prefix_samples0 * fp->slots_per_subframe) +
                                      (fp->nb_prefix_samples * fp->slots_per_subframe * (fp->symbols_per_slot - 1)));
  fp->samples_per_frame = 10 * fp->samples_per_subframe;
  fp->freq_range = (fp->dl_CarrierFreq < 6e9)? nr_FR1 : nr_FR2;

  // definition of Lmax according to ts 38.213 section 4.1
  if (fp->dl_CarrierFreq < 6e9){
	if(fp->frame_type && (fp->ssb_type==2))
		fp->Lmax = (fp->dl_CarrierFreq < 2.4e9)? 4 : 8;
	else
		fp->Lmax = (fp->dl_CarrierFreq < 3e9)? 4 : 8;
  }  
  else
    fp->Lmax = 64;


  return 0;
}

int nr_init_frame_parms(nfapi_nr_config_request_t* config,
                        NR_DL_FRAME_PARMS *fp)
{

  fp->eutra_band = config->nfapi_config.rf_bands.rf_band[0];
  fp->frame_type = config->subframe_config.duplex_mode.value;
  fp->L_ssb = config->sch_config.ssb_scg_position_in_burst.value;
  return nr_init_frame_parms0(fp,
			      config->subframe_config.numerology_index_mu.value,
			      config->subframe_config.dl_cyclic_prefix_type.value,
			      config->rf_config.dl_carrier_bandwidth.value);
}

int nr_init_frame_parms_ue(NR_DL_FRAME_PARMS *fp,
			   int mu, 
			   int Ncp,
			   int N_RB_DL,
			   int n_ssb_crb,
			   int ssb_subcarrier_offset) 
{
  /*n_ssb_crb and ssb_subcarrier_offset are given in 15kHz SCS*/
  nr_init_frame_parms0(fp,mu,Ncp,N_RB_DL);
  fp->ssb_start_subcarrier = (12 * n_ssb_crb + ssb_subcarrier_offset)/(1<<mu);
  return 0;
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
  LOG_I(PHY,"fp->dl_CarrierFreq=%u\n",fp->dl_CarrierFreq);
  LOG_I(PHY,"fp->ul_CarrierFreq=%u\n",fp->ul_CarrierFreq);
}



