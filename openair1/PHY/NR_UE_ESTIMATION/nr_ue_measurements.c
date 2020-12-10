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

#include "PHY/defs_nr_UE.h"
#include "PHY/phy_extern_nr_ue.h"
#include "common/utils/LOG/log.h"
#include "PHY/sse_intrin.h"
#include "openair2/LAYER2/NR_MAC_UE/mac_proto.h"

//#define k1 1000
#define k1 ((long long int) 1000)
#define k2 ((long long int) (1024-k1))

//#define DEBUG_MEAS_RRC
//#define DEBUG_MEAS_UE
//#define DEBUG_RANK_EST

#if 0
int16_t cond_num_threshold = 0;

void print_shorts(char *s,short *x)
{


  printf("%s  : %d,%d,%d,%d,%d,%d,%d,%d\n",s,
         x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7]
        );

}
void print_ints(char *s,int *x)
{


  printf("%s  : %d,%d,%d,%d\n",s,
         x[0],x[1],x[2],x[3]
        );

}
#endif

int16_t get_nr_PL(uint8_t Mod_id, uint8_t CC_id, uint8_t gNB_index){

  PHY_VARS_NR_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];

  /*
  if (ue->frame_parms.mode1_flag == 1)
    RSoffset = 6;
  else
    RSoffset = 3;
  */

 /* LOG_D(PHY,"get_nr_PL : rsrp %f dBm/RE (%f), eNB power %d dBm/RE\n", 
        (1.0*dB_fixed_times10(ue->measurements.rsrp[eNB_index])-(10.0*ue->rx_total_gain_dB))/10.0,
        10*log10((double)ue->measurements.rsrp[eNB_index]),
        ue->frame_parms.pdsch_config_common.referenceSignalPower);*/

  return((int16_t)(((10*ue->rx_total_gain_dB) - dB_fixed_times10(ue->measurements.rsrp[gNB_index]))/10));
                    //        dB_fixed_times10(RSoffset*12*ue_g[Mod_id][CC_id]->frame_parms.N_RB_DL) +
                    //(ue->frame_parms.pdsch_config_common.referenceSignalPower*10))/10));
}

uint32_t get_nr_rx_total_gain_dB (module_id_t Mod_id,uint8_t CC_id)
{

  PHY_VARS_NR_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];

  if (ue)
    return ue->rx_total_gain_dB;

  return 0xFFFFFFFF;
}


float_t get_nr_RSRP(module_id_t Mod_id,uint8_t CC_id,uint8_t gNB_index)
{

  AssertFatal(PHY_vars_UE_g!=NULL,"PHY_vars_UE_g is null\n");
  AssertFatal(PHY_vars_UE_g[Mod_id]!=NULL,"PHY_vars_UE_g[%d] is null\n",Mod_id);
  AssertFatal(PHY_vars_UE_g[Mod_id][CC_id]!=NULL,"PHY_vars_UE_g[%d][%d] is null\n",Mod_id,CC_id);

  PHY_VARS_NR_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];

  if (ue)
    return (10*log10(ue->measurements.rsrp[gNB_index])-
	    get_nr_rx_total_gain_dB(Mod_id,0) -
	    10*log10(20*12));
  return -140.0;
}


void nr_ue_measurements(PHY_VARS_NR_UE *ue,
                        UE_nr_rxtx_proc_t *proc,
                        unsigned int subframe_offset,
                        unsigned char N0_symbol,
                        unsigned char abstraction_flag,
                        unsigned char rank_adaptation,
                        uint8_t subframe)
{
  int aarx,aatx,eNB_id=0; //,gain_offset=0;
  //int rx_power[NUMBER_OF_CONNECTED_eNB_MAX];

/*#if defined(__x86_64__) || defined(__i386__)
  __m128i *dl_ch0_128, *dl_ch1_128;
#elif defined(__arm__)
  int16x8_t *dl_ch0_128, *dl_ch1_128get_PL;
#endif*/

  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  //unsigned int limit, subband;
  //int nb_subbands, subband_size, last_subband_size, *dl_ch0, *dl_ch1, N_RB_DL = frame_parms->N_RB_DL;

  int ch_offset, rank_tm3_tm4 = 0;

  ue->measurements.nb_antennas_rx = frame_parms->nb_antennas_rx;
  
  /*int16_t *dl_ch;
  dl_ch = (int16_t *)&ue->pdsch_vars[proc->thread_id][0]->dl_ch_estimates[eNB_id][ch_offset];*/

  ch_offset = ue->frame_parms.ofdm_symbol_size*2;

//printf("testing measurements\n");
  // signal measurements

  for (eNB_id=0; eNB_id<ue->n_connected_eNB; eNB_id++) {
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      for (aatx=0; aatx<frame_parms->nb_antenna_ports_gNB; aatx++) {
        ue->measurements.rx_spatial_power[eNB_id][aatx][aarx] =
          (signal_energy_nodc(&ue->pdsch_vars[proc->thread_id][0]->dl_ch_estimates[eNB_id][ch_offset],
                              (50*12)));
        //- ue->measurements.n0_power[aarx];

        if (ue->measurements.rx_spatial_power[eNB_id][aatx][aarx]<0)
          ue->measurements.rx_spatial_power[eNB_id][aatx][aarx] = 0; //ue->measurements.n0_power[aarx];

        ue->measurements.rx_spatial_power_dB[eNB_id][aatx][aarx] = (unsigned short) dB_fixed(ue->measurements.rx_spatial_power[eNB_id][aatx][aarx]);

        if (aatx==0)
          ue->measurements.rx_power[eNB_id][aarx] = ue->measurements.rx_spatial_power[eNB_id][aatx][aarx];
        else
          ue->measurements.rx_power[eNB_id][aarx] += ue->measurements.rx_spatial_power[eNB_id][aatx][aarx];
      } //aatx

      ue->measurements.rx_power_dB[eNB_id][aarx] = (unsigned short) dB_fixed(ue->measurements.rx_power[eNB_id][aarx]);

      if (aarx==0)
        ue->measurements.rx_power_tot[eNB_id] = ue->measurements.rx_power[eNB_id][aarx];
      else
        ue->measurements.rx_power_tot[eNB_id] += ue->measurements.rx_power[eNB_id][aarx];
    } //aarx

    ue->measurements.rx_power_tot_dB[eNB_id] = (unsigned short) dB_fixed(ue->measurements.rx_power_tot[eNB_id]);

  } //eNB_id
  
  //printf("ue measurement addr dlch %p\n", dl_ch);

  eNB_id=0;

  if (ue->transmission_mode[eNB_id]!=4 && ue->transmission_mode[eNB_id]!=3)
    ue->measurements.rank[eNB_id] = 0;
  else
    ue->measurements.rank[eNB_id] = rank_tm3_tm4;
  //  printf ("tx mode %d\n", ue->transmission_mode[eNB_id]);
  //  printf ("rank %d\n", ue->PHY_measurements.rank[eNB_id]);

  // filter to remove jitter
  if (ue->init_averaging == 0) {
    for (eNB_id = 0; eNB_id < ue->n_connected_eNB; eNB_id++)
      ue->measurements.rx_power_avg[eNB_id] = (int)
          (((k1*((long long int)(ue->measurements.rx_power_avg[eNB_id]))) +
            (k2*((long long int)(ue->measurements.rx_power_tot[eNB_id]))))>>10);

    //LOG_I(PHY,"Noise Power Computation: k1 %d k2 %d n0 avg %d n0 tot %d\n", k1, k2, ue->measurements.n0_power_avg,
     //   ue->measurements.n0_power_tot);
    ue->measurements.n0_power_avg = (int)
        (((k1*((long long int) (ue->measurements.n0_power_avg))) +
          (k2*((long long int) (ue->measurements.n0_power_tot))))>>10);
  } else {
    for (eNB_id = 0; eNB_id < ue->n_connected_eNB; eNB_id++)
      ue->measurements.rx_power_avg[eNB_id] = ue->measurements.rx_power_tot[eNB_id];

    ue->measurements.n0_power_avg = ue->measurements.n0_power_tot;
    ue->init_averaging = 0;
  }

  for (eNB_id = 0; eNB_id < ue->n_connected_eNB; eNB_id++) {
    ue->measurements.rx_power_avg_dB[eNB_id] = dB_fixed( ue->measurements.rx_power_avg[eNB_id]);
    ue->measurements.wideband_cqi_tot[eNB_id] = dB_fixed2(ue->measurements.rx_power_tot[eNB_id],ue->measurements.n0_power_tot);
    ue->measurements.wideband_cqi_avg[eNB_id] = dB_fixed2(ue->measurements.rx_power_avg[eNB_id],ue->measurements.n0_power_avg);
    ue->measurements.rx_rssi_dBm[eNB_id] = ue->measurements.rx_power_avg_dB[eNB_id] - ue->rx_total_gain_dB;
//#ifdef DEBUG_MEAS_UE
      LOG_D(PHY,"[eNB %d] Subframe %d, RSSI %d dBm, RSSI (digital) %d dB, WBandCQI %d dB, rxPwrAvg %d, n0PwrAvg %d\n",
            eNB_id,
            subframe,
            ue->measurements.rx_rssi_dBm[eNB_id],
            ue->measurements.rx_power_avg_dB[eNB_id],
            ue->measurements.wideband_cqi_avg[eNB_id],
            ue->measurements.rx_power_avg[eNB_id],
            ue->measurements.n0_power_tot);
//#endif
  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}

void nr_ue_rsrp_measurements(PHY_VARS_NR_UE *ue,
    UE_nr_rxtx_proc_t *proc,
    uint8_t slot,
    uint8_t abstraction_flag)
{
	int aarx,rb, symbol_offset;
	int16_t *rxF;

	uint16_t Nid_cell = ue->frame_parms.Nid_cell;
	uint8_t eNB_offset=0,l,nushift;
	uint16_t off,nb_rb;
//	NR_UE_MAC_INST_t *mac = get_mac_inst(0);
	int **rxdataF=ue->common_vars.common_vars_rx_data_per_thread[proc->thread_id].rxdataF;

	nushift =  ue->frame_parms.Nid_cell%4;
	ue->frame_parms.nushift = nushift;
	unsigned int  ssb_offset = ue->frame_parms.first_carrier_offset + ue->frame_parms.ssb_start_subcarrier;
	if (ssb_offset>= ue->frame_parms.ofdm_symbol_size) ssb_offset-=ue->frame_parms.ofdm_symbol_size;
	
	symbol_offset = ue->frame_parms.ofdm_symbol_size*((ue->symbol_offset+1)%(ue->frame_parms.symbols_per_slot));

    ue->measurements.rsrp[eNB_offset] = 0;

    //if (mac->csirc->reportQuantity.choice.ssb_Index_RSRP){ 
		nb_rb = 20;
	//} else{
	//	LOG_E(PHY,"report quantity not supported \n");
	//}

    if (abstraction_flag == 0) {

      for (l=0; l<1; l++) {

        LOG_D(PHY,"[UE %d]  slot %d Doing ue_rrc_measurements rsrp/rssi (Nid_cell %d, nushift %d, eNB_offset %d, l %d)\n",ue->Mod_id,slot,Nid_cell,nushift,
              eNB_offset,l);

        for (aarx=0; aarx<ue->frame_parms.nb_antennas_rx; aarx++) {
          rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+ssb_offset+nushift)];
          off  = 0; 

          if (l==0) {
            for (rb=0; rb<nb_rb; rb++) {

              ue->measurements.rsrp[eNB_offset] += (((int32_t)(rxF[off])*rxF[off])+((int32_t)(rxF[off+1])*rxF[off+1]));
                      //printf("rb %d, off %d : %d\n",rb,off,((((int32_t)rxF[off])*rxF[off])+((int32_t)(rxF[off+1])*rxF[off+1])));

              off = (off+4) % ue->frame_parms.ofdm_symbol_size;
            }
          }
        }
      }

	  ue->measurements.rsrp[eNB_offset]/=nb_rb;

	} else { 

      ue->measurements.rsrp[eNB_offset] = -93 ;
    }


      if (eNB_offset == 0)

      LOG_I(PHY,"[UE %d] slot %d RRC Measurements (idx %d, Cell id %d) => rsrp: %3.1f dBm/RE (%d)\n",
            ue->Mod_id,
            slot,eNB_offset,
            (eNB_offset>0) ? ue->measurements.adj_cell_id[eNB_offset-1] : ue->frame_parms.Nid_cell,
            10*log10(ue->measurements.rsrp[eNB_offset])-ue->rx_total_gain_dB,
            ue->measurements.rsrp[eNB_offset]);

}
