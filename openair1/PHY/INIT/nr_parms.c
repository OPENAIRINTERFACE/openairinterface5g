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
#include "log.h"

/// Subcarrier spacings in Hz indexed by numerology index
uint32_t nr_subcarrier_spacing[MAX_NUM_SUBCARRIER_SPACING] = {15e3, 30e3, 60e3, 120e3, 240e3};
uint16_t nr_slots_per_subframe[MAX_NUM_SUBCARRIER_SPACING] = {1, 2, 4, 16, 32};

int nr_init_frame_parms(nfapi_nr_config_request_t* config,
                        NR_DL_FRAME_PARMS *frame_parms)
{

  int N_RB = config->rf_config.dl_carrier_bandwidth.value;
  int Ncp = config->subframe_config.dl_cyclic_prefix_type.value;
  int mu = config->subframe_config.numerology_index_mu.value;

#if DISABLE_LOG_X
  printf("Initializing frame parms for mu %d, N_RB %d, Ncp %d\n",mu, N_RB, Ncp);
#else
  LOG_I(PHY,"Initializing frame parms for mu %d, N_RB %d, Ncp %d\n",mu, N_RB, Ncp);
#endif

  if (Ncp == NFAPI_CP_EXTENDED)
    AssertFatal(mu == NR_MU_2,"Invalid cyclic prefix %d for numerology index %d\n", Ncp, mu);

  switch(mu) {

    case NR_MU_0: //15kHz scs
      frame_parms->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_0];
      frame_parms->slots_per_subframe = nr_slots_per_subframe[NR_MU_0];
      break;

    case NR_MU_1: //30kHz scs
      frame_parms->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_1];
      frame_parms->slots_per_subframe = nr_slots_per_subframe[NR_MU_1];

      switch(N_RB){
        case 11:
        case 24:
        case 38:
        case 78:
        case 51:
        case 65:

        case 106: //40 MHz
          if (frame_parms->threequarter_fs) {
            frame_parms->ofdm_symbol_size = 1536;
            frame_parms->first_carrier_offset = 900; //1536 - 636
            frame_parms->nb_prefix_samples0 = 132;
            frame_parms->nb_prefix_samples = 108;
          }
          else {
            frame_parms->ofdm_symbol_size = 2048;
            frame_parms->first_carrier_offset = 1412; //2048 - 636
            frame_parms->nb_prefix_samples0 = 176;
            frame_parms->nb_prefix_samples = 144;
          }
          break;

        case 133:
        case 162:
        case 189:

        case 217: //80 MHz
          if (frame_parms->threequarter_fs) {
            frame_parms->ofdm_symbol_size = 3072;
            frame_parms->first_carrier_offset = 1770; //3072 - 1302
            frame_parms->nb_prefix_samples0 = 264;
            frame_parms->nb_prefix_samples = 216;
          }
          else {
            frame_parms->ofdm_symbol_size = 4096;
            frame_parms->first_carrier_offset = 2794; //4096 - 1302
            frame_parms->nb_prefix_samples0 = 352;
            frame_parms->nb_prefix_samples = 288;
          }
          break;

        case 245:
        case 273:
      default:
        AssertFatal(1==0,"Number of resource blocks %d undefined for mu %d, frame parms = %p\n", N_RB, mu, frame_parms);
      }
      break;

    case NR_MU_2: //60kHz scs
      frame_parms->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_2];
      frame_parms->slots_per_subframe = nr_slots_per_subframe[NR_MU_2];

      switch(N_RB){ //FR1 bands only
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
        AssertFatal(1==0,"Number of resource blocks %d undefined for mu %d, frame parms = %p\n", N_RB, mu, frame_parms);
      }
      break;

    case NR_MU_3:
      frame_parms->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_3];
      frame_parms->slots_per_subframe = nr_slots_per_subframe[NR_MU_3];
      break;

    case NR_MU_4:
      frame_parms->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_4];
      frame_parms->slots_per_subframe = nr_slots_per_subframe[NR_MU_4];
      break;

  default:
    AssertFatal(1==0,"Invalid numerology index %d", mu);
  }

  frame_parms->slots_per_frame = 10* frame_parms->slots_per_subframe;
  frame_parms->symbols_per_slot = ((Ncp == NORMAL)? 14 : 12); // to redefine for different slot formats
  frame_parms->samples_per_subframe_wCP = frame_parms->ofdm_symbol_size * frame_parms->symbols_per_slot * frame_parms->slots_per_subframe;
  frame_parms->samples_per_frame_wCP = 10 * frame_parms->samples_per_subframe_wCP;
  frame_parms->samples_per_subframe = (frame_parms->samples_per_subframe_wCP + (frame_parms->nb_prefix_samples0 * frame_parms->slots_per_subframe) +
                                      (frame_parms->nb_prefix_samples * frame_parms->slots_per_subframe * (frame_parms->symbols_per_slot - 1)));
  frame_parms->samples_per_frame = 10 * frame_parms->samples_per_subframe;
  frame_parms->freq_range = (frame_parms->dl_CarrierFreq < 6e9)? nr_FR1 : nr_FR2;

  // Initial bandwidth part configuration -- full carrier bandwidth
  frame_parms->initial_bwp_dl.bwp_id = 0;
  frame_parms->initial_bwp_dl.scs = frame_parms->subcarrier_spacing;
  frame_parms->initial_bwp_dl.location = 0;
  frame_parms->initial_bwp_dl.N_RB = N_RB;
  frame_parms->initial_bwp_dl.cyclic_prefix = Ncp;
  frame_parms->initial_bwp_dl.ofdm_symbol_size = frame_parms->ofdm_symbol_size;

  return 0;
}

int nr_init_frame_parms_ue(NR_DL_FRAME_PARMS *frame_parms)
{

  int N_RB = 106;
  int Ncp = 0;
  int mu = 1;

#if DISABLE_LOG_X
  printf("Initializing frame parms for mu %d, N_RB %d, Ncp %d\n",mu, N_RB, Ncp);
#else
  LOG_I(PHY,"Initializing frame parms for mu %d, N_RB %d, Ncp %d\n",mu, N_RB, Ncp);
#endif


  if (Ncp == EXTENDED)
    AssertFatal(mu == NR_MU_2,"Invalid cyclic prefix %d for numerology index %d\n", Ncp, mu);

  switch(mu) {

    case NR_MU_0: //15kHz scs
      frame_parms->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_0];
      frame_parms->slots_per_subframe = nr_slots_per_subframe[NR_MU_0];
      break;

    case NR_MU_1: //30kHz scs
      frame_parms->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_1];
      frame_parms->slots_per_subframe = nr_slots_per_subframe[NR_MU_1];

      switch(N_RB){
        case 11:
        case 24:
        case 38:
        case 78:
        case 51:
        case 65:

        case 106: //40 MHz
          if (frame_parms->threequarter_fs) {
            frame_parms->ofdm_symbol_size = 1536;
            frame_parms->first_carrier_offset = 900; //1536 - 636
            frame_parms->nb_prefix_samples0 = 132;
            frame_parms->nb_prefix_samples = 108;
          }
          else {
            frame_parms->ofdm_symbol_size = 2048;
            frame_parms->first_carrier_offset = 1412; //2048 - 636
            frame_parms->nb_prefix_samples0 = 176;
            frame_parms->nb_prefix_samples = 144;
          }
          break;

        case 133:
        case 162:
        case 189:

        case 217: //80 MHz
          if (frame_parms->threequarter_fs) {
            frame_parms->ofdm_symbol_size = 3072;
            frame_parms->first_carrier_offset = 1770; //3072 - 1302
            frame_parms->nb_prefix_samples0 = 264;
            frame_parms->nb_prefix_samples = 216;
          }
          else {
            frame_parms->ofdm_symbol_size = 4096;
            frame_parms->first_carrier_offset = 2794; //4096 - 1302
            frame_parms->nb_prefix_samples0 = 352;
            frame_parms->nb_prefix_samples = 288;
          }
          break;

        case 245:
        case 273:
      default:
        AssertFatal(1==0,"Number of resource blocks %d undefined for mu %d, frame parms = %p\n", N_RB, mu, frame_parms);
      }
      break;

    case NR_MU_2: //60kHz scs
      frame_parms->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_2];
      frame_parms->slots_per_subframe = nr_slots_per_subframe[NR_MU_2];

      switch(N_RB){ //FR1 bands only
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
        AssertFatal(1==0,"Number of resource blocks %d undefined for mu %d, frame parms = %p\n", N_RB, mu, frame_parms);
      }
      break;

    case NR_MU_3:
      frame_parms->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_3];
      frame_parms->slots_per_subframe = nr_slots_per_subframe[NR_MU_3];
      break;

    case NR_MU_4:
      frame_parms->subcarrier_spacing = nr_subcarrier_spacing[NR_MU_4];
      frame_parms->slots_per_subframe = nr_slots_per_subframe[NR_MU_4];
      break;

  default:
    AssertFatal(1==0,"Invalid numerology index %d", mu);
  }

    frame_parms->nb_prefix_samples0 = 160;
    frame_parms->nb_prefix_samples = 144;
    frame_parms->symbols_per_tti = 14;
    frame_parms->numerology_index = 0;
    frame_parms->ttis_per_subframe = 1;
    frame_parms->slots_per_tti = 2; //only slot config 1 is supported     

    frame_parms->ofdm_symbol_size = 2048;
    frame_parms->samples_per_tti = 30720;
    frame_parms->samples_per_subframe = 30720 * frame_parms->ttis_per_subframe;
    //frame_parms->first_carrier_offset = 2048-600;

  frame_parms->slots_per_frame = 10* frame_parms->slots_per_subframe;
  frame_parms->symbols_per_slot = ((Ncp == NORMAL)? 14 : 12); // to redefine for different slot formats
  frame_parms->samples_per_subframe_wCP = frame_parms->ofdm_symbol_size * frame_parms->symbols_per_slot * frame_parms->slots_per_subframe;
  frame_parms->samples_per_frame_wCP = 10 * frame_parms->samples_per_subframe_wCP;
  //frame_parms->samples_per_subframe = (frame_parms->samples_per_subframe_wCP + (frame_parms->nb_prefix_samples0 * frame_parms->slots_per_subframe) +
  //                                    (frame_parms->nb_prefix_samples * frame_parms->slots_per_subframe * (frame_parms->symbols_per_slot - 1)));
  frame_parms->samples_per_frame = 10 * frame_parms->samples_per_subframe;
  frame_parms->freq_range = (frame_parms->dl_CarrierFreq < 6e9)? nr_FR1 : nr_FR2;

  return 0;
}

void nr_dump_frame_parms(NR_DL_FRAME_PARMS *frame_parms)
{
  LOG_I(PHY,"frame_parms->scs=%d\n",frame_parms->subcarrier_spacing);
  LOG_I(PHY,"frame_parms->ofdm_symbol_size=%d\n",frame_parms->ofdm_symbol_size);
  LOG_I(PHY,"frame_parms->nb_prefix_samples0=%d\n",frame_parms->nb_prefix_samples0);
  LOG_I(PHY,"frame_parms->nb_prefix_samples=%d\n",frame_parms->nb_prefix_samples);
  LOG_I(PHY,"frame_parms->slots_per_subframe=%d\n",frame_parms->slots_per_subframe);
  LOG_I(PHY,"frame_parms->samples_per_subframe_wCP=%d\n",frame_parms->samples_per_subframe_wCP);
  LOG_I(PHY,"frame_parms->samples_per_frame_wCP=%d\n",frame_parms->samples_per_frame_wCP);
  LOG_I(PHY,"frame_parms->samples_per_subframe=%d\n",frame_parms->samples_per_subframe);
  LOG_I(PHY,"frame_parms->samples_per_frame=%d\n",frame_parms->samples_per_frame);
  LOG_I(PHY,"frame_parms->initial_bwp_dl.bwp_id=%d\n",frame_parms->initial_bwp_dl.bwp_id);
  LOG_I(PHY,"frame_parms->initial_bwp_dl.scs=%d\n",frame_parms->initial_bwp_dl.scs);
  LOG_I(PHY,"frame_parms->initial_bwp_dl.N_RB=%d\n",frame_parms->initial_bwp_dl.N_RB);
  LOG_I(PHY,"frame_parms->initial_bwp_dl.cyclic_prefix=%d\n",frame_parms->initial_bwp_dl.cyclic_prefix);
  LOG_I(PHY,"frame_parms->initial_bwp_dl.location=%d\n",frame_parms->initial_bwp_dl.location);
  LOG_I(PHY,"frame_parms->initial_bwp_dl.ofdm_symbol_size=%d\n",frame_parms->initial_bwp_dl.ofdm_symbol_size);
}
