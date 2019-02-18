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
      break;

    case NR_MU_1: //30kHz scs
      fp->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_1];
      fp->slots_per_subframe = nr_slots_per_subframe[NR_MU_1];

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
            fp->first_carrier_offset = 900; //1536 - 636
            fp->nb_prefix_samples0 = 132;
            fp->nb_prefix_samples = 108;
          }
          else {
            fp->ofdm_symbol_size = 2048;
            fp->first_carrier_offset = 1412; //2048 - 636
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
            fp->first_carrier_offset = 1770; //3072 - 1302
            fp->nb_prefix_samples0 = 264;
            fp->nb_prefix_samples = 216;
          }
	  else {
	    fp->ofdm_symbol_size = 4096;
	    fp->first_carrier_offset = 2794; //4096 - 1302
	    fp->nb_prefix_samples0 = 352;
	    fp->nb_prefix_samples = 288;
	  }
          break;

        case 245:
	  AssertFatal(fp->threequarter_fs==0,"3/4 sampling impossible for N_RB %d and MU %d\n",N_RB_DL,mu); 
	  fp->ofdm_symbol_size = 4096;
	  fp->first_carrier_offset = 2626; //4096 - 1478
	  fp->nb_prefix_samples0 = 352;
	  fp->nb_prefix_samples = 288;
	  break;
        case 273:
	  AssertFatal(fp->threequarter_fs==0,"3/4 sampling impossible for N_RB %d and MU %d\n",N_RB_DL,mu); 
	  fp->ofdm_symbol_size = 4096;
	  fp->first_carrier_offset = 2458; //4096 - 1638
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
      break;

    case NR_MU_4:
      fp->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_4];
      fp->slots_per_subframe = nr_slots_per_subframe[NR_MU_4];
      break;

  default:
    AssertFatal(1==0,"Invalid numerology index %d", mu);
  }

  fp->slots_per_frame = 10* fp->slots_per_subframe;

  fp->nb_antenna_ports_eNB = 1; // default value until overwritten by RRCConnectionReconfiguration

  fp->symbols_per_slot = ((Ncp == NORMAL)? 14 : 12); // to redefine for different slot formats
  fp->samples_per_subframe_wCP = fp->ofdm_symbol_size * fp->symbols_per_slot * fp->slots_per_subframe;
  fp->samples_per_frame_wCP = 10 * fp->samples_per_subframe_wCP;
  fp->samples_per_slot_wCP = fp->symbols_per_slot*fp->ofdm_symbol_size; 
  fp->samples_per_slot = fp->nb_prefix_samples0 + ((fp->symbols_per_slot-1)*fp->nb_prefix_samples) + (fp->symbols_per_slot*fp->ofdm_symbol_size); 
  fp->samples_per_subframe = (fp->samples_per_subframe_wCP + (fp->nb_prefix_samples0 * fp->slots_per_subframe) +
                                      (fp->nb_prefix_samples * fp->slots_per_subframe * (fp->symbols_per_slot - 1)));
  fp->samples_per_frame = 10 * fp->samples_per_subframe;
  fp->freq_range = (fp->dl_CarrierFreq < 6e9)? nr_FR1 : nr_FR2;

  // Initial bandwidth part configuration -- full carrier bandwidth
  fp->initial_bwp_dl.bwp_id = 0;
  fp->initial_bwp_dl.scs = fp->subcarrier_spacing;
  fp->initial_bwp_dl.location = 0;
  fp->initial_bwp_dl.N_RB = fp->N_RB_DL;
  fp->initial_bwp_dl.cyclic_prefix = fp->Ncp;
  fp->initial_bwp_dl.ofdm_symbol_size = fp->ofdm_symbol_size;

  return 0;
}

int nr_init_frame_parms(nfapi_nr_config_request_t* config,
                        NR_DL_FRAME_PARMS *fp)
{


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
  LOG_I(PHY,"fp->initial_bwp_dl.bwp_id=%d\n",fp->initial_bwp_dl.bwp_id);
  LOG_I(PHY,"fp->initial_bwp_dl.scs=%d\n",fp->initial_bwp_dl.scs);
  LOG_I(PHY,"fp->initial_bwp_dl.N_RB=%d\n",fp->initial_bwp_dl.N_RB);
  LOG_I(PHY,"fp->initial_bwp_dl.cyclic_prefix=%d\n",fp->initial_bwp_dl.cyclic_prefix);
  LOG_I(PHY,"fp->initial_bwp_dl.location=%d\n",fp->initial_bwp_dl.location);
  LOG_I(PHY,"fp->initial_bwp_dl.ofdm_symbol_size=%d\n",fp->initial_bwp_dl.ofdm_symbol_size);
}
