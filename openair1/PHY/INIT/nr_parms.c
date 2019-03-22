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

  fp->eutra_band = config->nfapi_config.rf_bands.rf_band[0];
  fp->frame_type = !(config->subframe_config.duplex_mode.value);
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
  LOG_I(PHY,"fp->initial_bwp_dl.bwp_id=%d\n",fp->initial_bwp_dl.bwp_id);
  LOG_I(PHY,"fp->initial_bwp_dl.scs=%d\n",fp->initial_bwp_dl.scs);
  LOG_I(PHY,"fp->initial_bwp_dl.N_RB=%d\n",fp->initial_bwp_dl.N_RB);
  LOG_I(PHY,"fp->initial_bwp_dl.cyclic_prefix=%d\n",fp->initial_bwp_dl.cyclic_prefix);
  LOG_I(PHY,"fp->initial_bwp_dl.location=%d\n",fp->initial_bwp_dl.location);
  LOG_I(PHY,"fp->initial_bwp_dl.ofdm_symbol_size=%d\n",fp->initial_bwp_dl.ofdm_symbol_size);
}


nr_bandentry_t nr_bandtable[] = {
  {1,  1920000, 1980000, 2110000, 2170000, 20, 422000},
  {2,  1850000, 1910000, 1930000, 1990000, 20, 386000},
  {3,  1710000, 1785000, 1805000, 1880000, 20, 361000},
  {5,   824000,  849000,  869000,  894000, 20, 173800},
  {7,  2500000, 2570000, 2620000, 2690000, 20, 524000},
  {8,   880000,  915000,  925000,  960000, 20, 185000},
  {12,  698000,  716000,  728000,  746000, 20, 145800},
  {20,  832000,  862000,  791000,  821000, 20, 158200},
  {25, 1850000, 1915000, 1930000, 1995000, 20, 386000},
  {28,  703000,  758000,  758000,  813000, 20, 151600},
  {34, 2010000, 2025000, 2010000, 2025000, 20, 402000},
  {38, 2570000, 2620000, 2570000, 2630000, 20, 514000},
  {39, 1880000, 1920000, 1880000, 1920000, 20, 376000},
  {40, 2300000, 2400000, 2300000, 2400000, 20, 460000},
  {41, 2496000, 2690000, 2496000, 2690000,  3, 499200},
  {50, 1432000, 1517000, 1432000, 1517000, 20, 286400},
  {51, 1427000, 1432000, 1427000, 1432000, 20, 285400},
  {66, 1710000, 1780000, 2110000, 2200000, 20, 422000},
  {70, 1695000, 1710000, 1995000, 2020000, 20, 399000},
  {71,  663000,  698000,  617000,  652000, 20, 123400},
  {74, 1427000, 1470000, 1475000, 1518000, 20, 295000},
  {75,     000,     000, 1432000, 1517000, 20, 286400},
  {76,     000,     000, 1427000, 1432000, 20, 285400},
  {77, 3300000, 4200000, 3300000, 4200000,  1, 620000},
  {78, 3300000, 3800000, 3300000, 3800000,  1, 620000},
  {79, 4400000, 5000000, 4400000, 5000000,  2, 693334},
  {80, 1710000, 1785000,     000,     000, 20, 342000},
  {81,  860000,  915000,     000,     000, 20, 176000},
  {82,  832000,  862000,     000,     000, 20, 166400},
  {83,  703000,  748000,     000,     000, 20, 140600},
  {84, 1920000, 1980000,     000,     000, 20, 384000},
  {86, 1710000, 1785000,     000,     000, 20, 342000}
};

int get_band(uint32_t downlink_frequency,   uint8_t *current_band,   int32_t *current_offset, lte_frame_type_t *current_type) {

    int ind;
    int64_t dl_freq_khz = downlink_frequency/1000;
    for ( ind=0;
          ind < sizeof(nr_bandtable) / sizeof(nr_bandtable[0]);
          ind++) {
      *current_band = nr_bandtable[ind].band;
      LOG_I(PHY, "Scanning band %d, dl_min %"PRIu32", ul_min %"PRIu32"\n", ind, nr_bandtable[ind].dl_min,nr_bandtable[ind].ul_min);

      if ( nr_bandtable[ind].dl_min <= dl_freq_khz && nr_bandtable[ind].dl_max >= dl_freq_khz ) {
	*current_offset = (nr_bandtable[ind].ul_min - nr_bandtable[ind].dl_min)*1000;
	if (*current_offset == 0)
	  *current_type = TDD;
	else
	  *current_type = FDD;

	LOG_I( PHY, "DL frequency %"PRIu32": band %d, frame_type %d, UL frequency %"PRIu32"\n",
	       downlink_frequency, *current_band, *current_type, downlink_frequency+*current_offset);
        break;
      }
    }

    if( ind == sizeof(nr_bandtable) / sizeof(nr_bandtable[0])) {
      LOG_E(PHY,"Can't find EUTRA band for frequency %d\n", downlink_frequency);
      return(-1);
    }
}

