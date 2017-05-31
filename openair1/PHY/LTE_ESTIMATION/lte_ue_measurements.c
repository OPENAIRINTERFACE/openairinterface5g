/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

// this function fills the PHY_vars->PHY_measurement structure

#include "PHY/defs.h"
#include "PHY/extern.h"
#include "SCHED/defs.h"
#include "SCHED/extern.h"
#include "log.h"
#include "PHY/sse_intrin.h"

//#define k1 1000
#define k1 ((long long int) 1000)
#define k2 ((long long int) (1024-k1))

//#define DEBUG_MEAS_RRC
//#define DEBUG_MEAS_UE

#ifdef USER_MODE
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


int16_t get_PL(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index)
{

  PHY_VARS_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];
  /*
  int RSoffset;


  if (ue->frame_parms.mode1_flag == 1)
    RSoffset = 6;
  else
    RSoffset = 3;
  */

  LOG_D(PHY,"get_PL : Frame %d : rsrp %f dBm/RE (%f), eNB power %d dBm/RE\n", ue->proc.proc_rxtx[0].frame_rx,
        (1.0*dB_fixed_times10(ue->measurements.rsrp[eNB_index])-(10.0*ue->rx_total_gain_dB))/10.0,
        10*log10((double)ue->measurements.rsrp[eNB_index]),
        ue->frame_parms.pdsch_config_common.referenceSignalPower);

  return((int16_t)(((10*ue->rx_total_gain_dB) -
                    dB_fixed_times10(ue->measurements.rsrp[eNB_index])+
                    //        dB_fixed_times10(RSoffset*12*ue_g[Mod_id][CC_id]->frame_parms.N_RB_DL) +
                    (ue->frame_parms.pdsch_config_common.referenceSignalPower*10))/10));
}


uint8_t get_n_adj_cells (uint8_t Mod_id,uint8_t CC_id)
{

  PHY_VARS_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];

  if (ue)
    return ue->measurements.n_adj_cells;
  else
    return 0;
}

uint32_t get_rx_total_gain_dB (uint8_t Mod_id,uint8_t CC_id)
{

  PHY_VARS_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];

  if (ue)
    return ue->rx_total_gain_dB;

  return 0xFFFFFFFF;
}
uint32_t get_RSSI (uint8_t Mod_id,uint8_t CC_id)
{

  PHY_VARS_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];

  if (ue)
    return ue->measurements.rssi;

  return 0xFFFFFFFF;
}
uint32_t get_RSRP(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index)
{

  PHY_VARS_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];

  if (ue)
    return ue->measurements.rsrp[eNB_index];

  return 0xFFFFFFFF;
}

uint32_t get_RSRQ(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index)
{

  PHY_VARS_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];

  if (ue)
    return ue->measurements.rsrq[eNB_index];

  return 0xFFFFFFFF;
}

int8_t set_RSRP_filtered(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index,float rsrp)
{

  PHY_VARS_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];

  if (ue) {
    ue->measurements.rsrp_filtered[eNB_index]=rsrp;
    return 0;
  }

  LOG_W(PHY,"[UE%d] could not set the rsrp\n",Mod_id);
  return -1;
}

int8_t set_RSRQ_filtered(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index,float rsrq)
{

  PHY_VARS_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];

  if (ue) {
    ue->measurements.rsrq_filtered[eNB_index]=rsrq;
    return 0;
  }

  LOG_W(PHY,"[UE%d] could not set the rsrq\n",Mod_id);
  return -1;

}

void ue_rrc_measurements(PHY_VARS_UE *ue,
    uint8_t slot,
    uint8_t abstraction_flag)
{

  uint8_t subframe = slot>>1;
  int aarx,rb;
  uint8_t pss_symb;
  uint8_t sss_symb;

  int32_t **rxdataF;
  int16_t *rxF,*rxF_pss,*rxF_sss;

  uint16_t Nid_cell = ue->frame_parms.Nid_cell;
  uint8_t eNB_offset,nu,l,nushift,k;
  uint16_t off;

  //uint8_t isPss; // indicate if this is a slot for extracting PSS
  //uint8_t isSss; // indicate if this is a slot for extracting SSS
  //int32_t pss_ext[4][72]; // contain the extracted 6*12 REs for mapping the PSS
  //int32_t sss_ext[4][72]; // contain the extracted 6*12 REs for mapping the SSS
  //int32_t (*xss_ext)[72]; // point to either pss_ext or sss_ext for common calculation
  //int16_t *re,*im; // real and imag part of each 32-bit xss_ext[][] value

  //LOG_I(PHY,"UE RRC MEAS Start Subframe %d Frame Type %d slot %d \n",subframe,ue->frame_parms.frame_type,slot);
  for (eNB_offset = 0; eNB_offset<1+ue->measurements.n_adj_cells; eNB_offset++) {

    if (eNB_offset==0) {
      ue->measurements.rssi = 0;
      //ue->measurements.n0_power_tot = 0;

      if (abstraction_flag == 0) {
        if ( ((ue->frame_parms.frame_type == FDD) && ((subframe == 0) || (subframe == 5))) ||
             ((ue->frame_parms.frame_type == TDD) && ((subframe == 1) || (subframe == 6)))
                )
        {  // FDD PSS/SSS, compute noise in DTX REs

          if (ue->frame_parms.Ncp==NORMAL) {
            for (aarx=0; aarx<ue->frame_parms.nb_antennas_rx; aarx++) {

          if(ue->frame_parms.frame_type == FDD)
          {
	      rxF_sss = (int16_t *)&ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].rxdataF[aarx][(5*ue->frame_parms.ofdm_symbol_size)];
	      rxF_pss = (int16_t *)&ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].rxdataF[aarx][(6*ue->frame_parms.ofdm_symbol_size)];
          }
          else
          {
              rxF_sss = (int16_t *)&ue->common_vars.common_vars_rx_data_per_thread[(subframe+1)&0x1].rxdataF[aarx][(13*ue->frame_parms.ofdm_symbol_size)];
              rxF_pss = (int16_t *)&ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].rxdataF[aarx][(2*ue->frame_parms.ofdm_symbol_size)];
          }
              //-ve spectrum from SSS

              //+ve spectrum from SSS
	          ue->measurements.n0_power[aarx] += (((int32_t)rxF_sss[2+70]*rxF_sss[2+70])+((int32_t)rxF_sss[2+69]*rxF_sss[2+69]));
              ue->measurements.n0_power[aarx] += (((int32_t)rxF_sss[2+68]*rxF_sss[2+68])+((int32_t)rxF_sss[2+67]*rxF_sss[2+67]));
              ue->measurements.n0_power[aarx] += (((int32_t)rxF_sss[2+66]*rxF_sss[2+66])+((int32_t)rxF_sss[2+65]*rxF_sss[2+65]));
	      //	      ue->measurements.n0_power[aarx] += (((int32_t)rxF_sss[2+64]*rxF_sss[2+64])+((int32_t)rxF_sss[2+63]*rxF_sss[2+63]));
	      //	      printf("sssp32 %d\n",ue->measurements.n0_power[aarx]);
              //+ve spectrum from PSS
              ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[2+70]*rxF_pss[2+70])+((int32_t)rxF_pss[2+69]*rxF_pss[2+69]));
              ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[2+68]*rxF_pss[2+68])+((int32_t)rxF_pss[2+67]*rxF_pss[2+67]));
              ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[2+66]*rxF_pss[2+66])+((int32_t)rxF_pss[2+65]*rxF_pss[2+65]));
	      //              ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[2+64]*rxF_pss[2+64])+((int32_t)rxF_pss[2+63]*rxF_pss[2+63]));
	      //	      printf("pss32 %d\n",ue->measurements.n0_power[aarx]);              //-ve spectrum from PSS
              if(ue->frame_parms.frame_type == FDD)
              {
                  rxF_sss = (int16_t *)&ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].rxdataF[aarx][(6*ue->frame_parms.ofdm_symbol_size)];
                  rxF_pss = (int16_t *)&ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].rxdataF[aarx][(7*ue->frame_parms.ofdm_symbol_size)];
              }
              else
              {
                  rxF_sss = (int16_t *)&ue->common_vars.common_vars_rx_data_per_thread[(subframe+1)&0x1].rxdataF[aarx][(14*ue->frame_parms.ofdm_symbol_size)];
                  rxF_pss = (int16_t *)&ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].rxdataF[aarx][(3*ue->frame_parms.ofdm_symbol_size)];
              }
	      //              ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[-72]*rxF_pss[-72])+((int32_t)rxF_pss[-71]*rxF_pss[-71]));
	      //	      printf("pssm36 %d\n",ue->measurements.n0_power[aarx]);
              ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[-70]*rxF_pss[-70])+((int32_t)rxF_pss[-69]*rxF_pss[-69]));
              ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[-68]*rxF_pss[-68])+((int32_t)rxF_pss[-67]*rxF_pss[-67]));
              ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[-66]*rxF_pss[-66])+((int32_t)rxF_pss[-65]*rxF_pss[-65]));
              
              ue->measurements.n0_power[aarx] = (((int32_t)rxF_sss[-70]*rxF_sss[-70])+((int32_t)rxF_sss[-69]*rxF_sss[-69]));
              ue->measurements.n0_power[aarx] += (((int32_t)rxF_sss[-68]*rxF_sss[-68])+((int32_t)rxF_sss[-67]*rxF_sss[-67]));
              ue->measurements.n0_power[aarx] += (((int32_t)rxF_sss[-66]*rxF_sss[-66])+((int32_t)rxF_sss[-65]*rxF_sss[-65]));

	      //              ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[-64]*rxF_pss[-64])+((int32_t)rxF_pss[-63]*rxF_pss[-63]));
	      //	      printf("pssm32 %d\n",ue->measurements.n0_power[aarx]);
              ue->measurements.n0_power_dB[aarx] = (unsigned short) dB_fixed(ue->measurements.n0_power[aarx]/12);
              ue->measurements.n0_power_tot /*+=*/ = ue->measurements.n0_power[aarx];
            }

            //LOG_I(PHY,"Subframe %d RRC UE MEAS Noise Level %d \n", subframe, ue->measurements.n0_power_tot);

	    ue->measurements.n0_power_tot_dB = (unsigned short) dB_fixed(ue->measurements.n0_power_tot/(12*aarx));
	    ue->measurements.n0_power_tot_dBm = ue->measurements.n0_power_tot_dB - ue->rx_total_gain_dB - dB_fixed(ue->frame_parms.ofdm_symbol_size);
          } else {
            LOG_E(PHY, "Not yet implemented: noise power calculation when prefix length = EXTENDED\n");
          }
        }
        else if ((ue->frame_parms.frame_type == TDD) &&
                 ((subframe == 1) || (subframe == 6))) {  // TDD PSS/SSS, compute noise in DTX REs // 2016-09-29 wilson fix incorrect noise power calculation


          pss_symb = 2;
          sss_symb = ue->frame_parms.symbols_per_tti-1;
          if (ue->frame_parms.Ncp==NORMAL) {
            for (aarx=0; aarx<ue->frame_parms.nb_antennas_rx; aarx++) {

                rxdataF  =  ue->common_vars.common_vars_rx_data_per_thread[(subframe&0x1)].rxdataF;
                rxF_pss  = (int16_t *) &rxdataF[aarx][((pss_symb*(ue->frame_parms.ofdm_symbol_size)))];

                rxdataF  =  ue->common_vars.common_vars_rx_data_per_thread[(subframe+1)&0x1].rxdataF;
                rxF_sss  = (int16_t *) &rxdataF[aarx][((sss_symb*(ue->frame_parms.ofdm_symbol_size)))];

                //-ve spectrum from SSS
            //          printf("slot %d: SSS DTX: %d,%d, non-DTX %d,%d\n",slot,rxF_pss[-72],rxF_pss[-71],rxF_pss[-36],rxF_pss[-35]);

            //              ue->measurements.n0_power[aarx] = (((int32_t)rxF_pss[-72]*rxF_pss[-72])+((int32_t)rxF_pss[-71]*rxF_pss[-71]));
            //          printf("sssn36 %d\n",ue->measurements.n0_power[aarx]);
                ue->measurements.n0_power[aarx] = (((int32_t)rxF_pss[-70]*rxF_pss[-70])+((int32_t)rxF_pss[-69]*rxF_pss[-69]));
                ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[-68]*rxF_pss[-68])+((int32_t)rxF_pss[-67]*rxF_pss[-67]));
                ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[-66]*rxF_pss[-66])+((int32_t)rxF_pss[-65]*rxF_pss[-65]));
            //              ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[-64]*rxF_pss[-64])+((int32_t)rxF_pss[-63]*rxF_pss[-63]));
            //          printf("sssm32 %d\n",ue->measurements.n0_power[aarx]);
                //+ve spectrum from SSS
            ue->measurements.n0_power[aarx] += (((int32_t)rxF_sss[2+70]*rxF_sss[2+70])+((int32_t)rxF_sss[2+69]*rxF_sss[2+69]));
                ue->measurements.n0_power[aarx] += (((int32_t)rxF_sss[2+68]*rxF_sss[2+68])+((int32_t)rxF_sss[2+67]*rxF_sss[2+67]));
                ue->measurements.n0_power[aarx] += (((int32_t)rxF_sss[2+66]*rxF_sss[2+66])+((int32_t)rxF_sss[2+65]*rxF_sss[2+65]));
            //          ue->measurements.n0_power[aarx] += (((int32_t)rxF_sss[2+64]*rxF_sss[2+64])+((int32_t)rxF_sss[2+63]*rxF_sss[2+63]));
            //          printf("sssp32 %d\n",ue->measurements.n0_power[aarx]);
                //+ve spectrum from PSS
                ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[2+70]*rxF_pss[2+70])+((int32_t)rxF_pss[2+69]*rxF_pss[2+69]));
                ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[2+68]*rxF_pss[2+68])+((int32_t)rxF_pss[2+67]*rxF_pss[2+67]));
                ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[2+66]*rxF_pss[2+66])+((int32_t)rxF_pss[2+65]*rxF_pss[2+65]));
            //              ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[2+64]*rxF_pss[2+64])+((int32_t)rxF_pss[2+63]*rxF_pss[2+63]));
            //          printf("pss32 %d\n",ue->measurements.n0_power[aarx]);              //-ve spectrum from PSS
                rxF_pss = (int16_t *)&ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].rxdataF[aarx][(7*ue->frame_parms.ofdm_symbol_size)];
            //              ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[-72]*rxF_pss[-72])+((int32_t)rxF_pss[-71]*rxF_pss[-71]));
            //          printf("pssm36 %d\n",ue->measurements.n0_power[aarx]);
                ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[-70]*rxF_pss[-70])+((int32_t)rxF_pss[-69]*rxF_pss[-69]));
                ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[-68]*rxF_pss[-68])+((int32_t)rxF_pss[-67]*rxF_pss[-67]));
                ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[-66]*rxF_pss[-66])+((int32_t)rxF_pss[-65]*rxF_pss[-65]));
            //              ue->measurements.n0_power[aarx] += (((int32_t)rxF_pss[-64]*rxF_pss[-64])+((int32_t)rxF_pss[-63]*rxF_pss[-63]));
            //          printf("pssm32 %d\n",ue->measurements.n0_power[aarx]);
                ue->measurements.n0_power_dB[aarx] = (unsigned short) dB_fixed(ue->measurements.n0_power[aarx]/12);
                ue->measurements.n0_power_tot /*+=*/ = ue->measurements.n0_power[aarx];
	    }

        ue->measurements.n0_power_tot_dB = (unsigned short) dB_fixed(ue->measurements.n0_power_tot/(12*aarx));
        ue->measurements.n0_power_tot_dBm = ue->measurements.n0_power_tot_dB - ue->rx_total_gain_dB - dB_fixed(ue->frame_parms.ofdm_symbol_size);


        //LOG_I(PHY,"Subframe %d RRC UE MEAS Noise Level %d \n", subframe, ue->measurements.n0_power_tot);

          }
        }
      }
    }
    // recompute nushift with eNB_offset corresponding to adjacent eNB on which to perform channel estimation
    //    printf("[PHY][UE %d] Frame %d slot %d Doing ue_rrc_measurements rsrp/rssi (Nid_cell %d, Nid2 %d, nushift %d, eNB_offset %d)\n",ue->Mod_id,ue->frame,slot,Nid_cell,Nid2,nushift,eNB_offset);
    if (eNB_offset > 0)
      Nid_cell = ue->measurements.adj_cell_id[eNB_offset-1];


    nushift =  Nid_cell%6;



    ue->measurements.rsrp[eNB_offset] = 0;


    if (abstraction_flag == 0) {

      // compute RSRP using symbols 0 and 4-frame_parms->Ncp

      for (l=0,nu=0; l<=(4-ue->frame_parms.Ncp); l+=(4-ue->frame_parms.Ncp),nu=3) {
        k = (nu + nushift)%6;
#ifdef DEBUG_MEAS_RRC
        LOG_I(PHY,"[UE %d] Frame %d subframe %d Doing ue_rrc_measurements rsrp/rssi (Nid_cell %d, nushift %d, eNB_offset %d, k %d, l %d)\n",ue->Mod_id,ue->proc.proc_rxtx[subframe&1].frame_rx,subframe,Nid_cell,nushift,
              eNB_offset,k,l);
#endif

        for (aarx=0; aarx<ue->frame_parms.nb_antennas_rx; aarx++) {
          rxF = (int16_t *)&ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].rxdataF[aarx][(l*ue->frame_parms.ofdm_symbol_size)];
          off  = (ue->frame_parms.first_carrier_offset+k)<<1;

          if (l==(4-ue->frame_parms.Ncp)) {
            for (rb=0; rb<ue->frame_parms.N_RB_DL; rb++) {

              //    printf("rb %d, off %d, off2 %d\n",rb,off,off2);

              ue->measurements.rsrp[eNB_offset] += (((int32_t)(rxF[off])*rxF[off])+((int32_t)(rxF[off+1])*rxF[off+1]));
              //        printf("rb %d, off %d : %d\n",rb,off,((((int32_t)rxF[off])*rxF[off])+((int32_t)(rxF[off+1])*rxF[off+1])));
	      //	      if ((ue->frame_rx&0x3ff) == 0)
	      //                printf("rb %d, off %d : %d\n",rb,off,((rxF[off]*rxF[off])+(rxF[off+1]*rxF[off+1])));


              off+=12;

              if (off>=(ue->frame_parms.ofdm_symbol_size<<1))
                off = (1+k)<<1;

              ue->measurements.rsrp[eNB_offset] += (((int32_t)(rxF[off])*rxF[off])+((int32_t)(rxF[off+1])*rxF[off+1]));
              //    printf("rb %d, off %d : %d\n",rb,off,(((int32_t)(rxF[off])*rxF[off])+((int32_t)(rxF[off+1])*rxF[off+1])));
              /*
                if ((ue->frame_rx&0x3ff) == 0)
                printf("rb %d, off %d : %d\n",rb,off,((rxF[off]*rxF[off])+(rxF[off+1]*rxF[off+1])));
              */
              off+=12;

              if (off>=(ue->frame_parms.ofdm_symbol_size<<1))
                off = (1+k)<<1;

            }

            /*
            if ((eNB_offset==0)&&(l==0)) {
            for (i=0;i<6;i++,off2+=4)
            ue->measurements.rssi += ((rxF[off2]*rxF[off2])+(rxF[off2+1]*rxF[off2+1]));
            if (off2==(ue->frame_parms.ofdm_symbol_size<<2))
            off2=4;
            for (i=0;i<6;i++,off2+=4)
            ue->measurements.rssi += ((rxF[off2]*rxF[off2])+(rxF[off2+1]*rxF[off2+1]));
            }
            */
            //    printf("slot %d, rb %d => rsrp %d, rssi %d\n",slot,rb,ue->measurements.rsrp[eNB_offset],ue->measurements.rssi);
          }
        }
      }

      // 2 RE per PRB
      //      ue->measurements.rsrp[eNB_offset]/=(24*ue->frame_parms.N_RB_DL);
      ue->measurements.rsrp[eNB_offset]/=(2*ue->frame_parms.N_RB_DL*ue->frame_parms.ofdm_symbol_size);
      //      LOG_I(PHY,"eNB: %d, RSRP: %d \n",eNB_offset,ue->measurements.rsrp[eNB_offset]);
      if (eNB_offset == 0) {
        //  ue->measurements.rssi/=(24*ue->frame_parms.N_RB_DL);
        //  ue->measurements.rssi*=rx_power_correction;
        //  ue->measurements.rssi=ue->measurements.rsrp[0]*24/2;
        ue->measurements.rssi=ue->measurements.rsrp[0]*(12*ue->frame_parms.N_RB_DL);
      }

      if (ue->measurements.rssi>0)
        ue->measurements.rsrq[eNB_offset] = 100*ue->measurements.rsrp[eNB_offset]*ue->frame_parms.N_RB_DL/ue->measurements.rssi;
      else
        ue->measurements.rsrq[eNB_offset] = -12000;

      //((200*ue->measurements.rsrq[eNB_offset]) + ((1024-200)*100*ue->measurements.rsrp[eNB_offset]*ue->frame_parms.N_RB_DL/ue->measurements.rssi))>>10;
    } else { // Do abstraction of RSRP and RSRQ
      ue->measurements.rssi = ue->measurements.rx_power_avg[0];
      // dummay value for the moment
      ue->measurements.rsrp[eNB_offset] = -93 ;
      ue->measurements.rsrq[eNB_offset] = 3;

    }

#ifdef DEBUG_MEAS_RRC

    //    if (slot == 0) {

      if (eNB_offset == 0)
        LOG_I(PHY,"[UE %d] Frame %d, subframe %d RRC Measurements => rssi %3.1f dBm (digital: %3.1f dB, gain %d), N0 %d dBm\n",ue->Mod_id,
              ue->proc.proc_rxtx[subframe&1].frame_rx,subframe,10*log10(ue->measurements.rssi)-ue->rx_total_gain_dB,
              10*log10(ue->measurements.rssi),
              ue->rx_total_gain_dB,
              ue->measurements.n0_power_tot_dBm);

      LOG_I(PHY,"[UE %d] Frame %d, subframe %d RRC Measurements (idx %d, Cell id %d) => rsrp: %3.1f dBm/RE (%d), rsrq: %3.1f dB\n",
            ue->Mod_id,
            ue->proc.proc_rxtx[subframe&1].frame_rx,subframe,eNB_offset,
            (eNB_offset>0) ? ue->measurements.adj_cell_id[eNB_offset-1] : ue->frame_parms.Nid_cell,
            10*log10(ue->measurements.rsrp[eNB_offset])-ue->rx_total_gain_dB,
            ue->measurements.rsrp[eNB_offset],
            (10*log10(ue->measurements.rsrq[eNB_offset])));
      //LOG_D(PHY,"RSRP_total_dB: %3.2f \n",(dB_fixed_times10(ue->measurements.rsrp[eNB_offset])/10.0)-ue->rx_total_gain_dB-dB_fixed(ue->frame_parms.N_RB_DL*12));

      //LOG_D(PHY,"RSRP_dB: %3.2f \n",(dB_fixed_times10(ue->measurements.rsrp[eNB_offset])/10.0));
      //LOG_D(PHY,"gain_loss_dB: %d \n",ue->rx_total_gain_dB);
      //LOG_D(PHY,"gain_fixed_dB: %d \n",dB_fixed(ue->frame_parms.N_RB_DL*12));

      //    }

#endif
  }

}

void lte_ue_measurements(PHY_VARS_UE *ue,
                         unsigned int subframe_offset,
                         unsigned char N0_symbol,
                         unsigned char abstraction_flag,
						 uint8_t subframe)
{


  int aarx,aatx,eNB_id=0; //,gain_offset=0;
  //int rx_power[NUMBER_OF_CONNECTED_eNB_MAX];
  int i;
  unsigned int limit,subband;
#if defined(__x86_64__) || defined(__i386__)
  __m128i *dl_ch0_128,*dl_ch1_128;
#elif defined(__arm__)
  int16x8_t *dl_ch0_128, *dl_ch1_128;
#endif
  int *dl_ch0,*dl_ch1;
  LTE_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  int nb_subbands,subband_size,last_subband_size;
  int N_RB_DL = frame_parms->N_RB_DL;
  ue->measurements.nb_antennas_rx = frame_parms->nb_antennas_rx;

    if (ue->transmission_mode[eNB_id]!=4)
     ue->measurements.rank[eNB_id] = 0;
    else
    ue->measurements.rank[eNB_id] = 1;
  //  printf ("tx mode %d\n", ue->transmission_mode[eNB_id]);
  //  printf ("rank %d\n", ue->PHY_measurements.rank[eNB_id]);

  switch (N_RB_DL) {
  case 6:
    nb_subbands = 6;
    subband_size = 12;
    last_subband_size = 0;
    break;

  default:
  case 25:
    nb_subbands = 7;
    subband_size = 4*12;
    last_subband_size = 12;
    break;

  case 50:
    nb_subbands = 9;
    subband_size = 6*12;
    last_subband_size = 2*12;
    break;

  case 100:
    nb_subbands = 13;
    subband_size = 8*12;
    last_subband_size = 4*12;
    break;
  }

  // signal measurements
  for (eNB_id=0; eNB_id<ue->n_connected_eNB; eNB_id++) {
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++) {
        ue->measurements.rx_spatial_power[eNB_id][aatx][aarx] =
          (signal_energy_nodc(&ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[eNB_id][(aatx<<1) + aarx][0],
                              (N_RB_DL*12)));
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

  // filter to remove jitter
  if (ue->init_averaging == 0) {
    for (eNB_id = 0; eNB_id < ue->n_connected_eNB; eNB_id++)
      ue->measurements.rx_power_avg[eNB_id] = (int)
          (((k1*((long long int)(ue->measurements.rx_power_avg[eNB_id]))) +
            (k2*((long long int)(ue->measurements.rx_power_tot[eNB_id]))))>>10);

    //LOG_I(PHY,"Noise Power Computation: k1 %d k2 %d n0 avg %d n0 tot %d\n", k1, k2, ue->measurements.n0_power_avg,
  	//	  ue->measurements.n0_power_tot);
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
#ifdef DEBUG_MEAS_UE
      LOG_I(PHY,"[eNB %d] Subframe %d, RSSI %d dBm, RSSI (digital) %d dB, WBandCQI %d dB, rxPwrAvg %d, n0PwrAvg %d\n",
            eNB_id,
			subframe,
            ue->measurements.rx_rssi_dBm[eNB_id],
            ue->measurements.rx_power_avg_dB[eNB_id],
            ue->measurements.wideband_cqi_avg[eNB_id],
            ue->measurements.rx_power_avg[eNB_id],
            ue->measurements.n0_power_avg);
#endif
  }

  ue->measurements.n0_power_avg_dB = dB_fixed( ue->measurements.n0_power_avg);

  for (eNB_id = 0; eNB_id < ue->n_connected_eNB; eNB_id++) {
    if (frame_parms->mode1_flag==0) {
      // cqi/pmi information

      for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
        dl_ch0    = &ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[eNB_id][aarx][4];
        dl_ch1    = &ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[eNB_id][2+aarx][4];

        for (subband=0; subband<nb_subbands; subband++) {

          // cqi
          if (aarx==0)
            ue->measurements.subband_cqi_tot[eNB_id][subband]=0;

          if ((subband<(nb_subbands-1))||(N_RB_DL==6)) {
            /*for (i=0;i<48;i++)
            msg("subband %d (%d) : %d,%d\n",subband,i,((short *)dl_ch0)[2*i],((short *)dl_ch0)[1+(2*i)]);
            */
            ue->measurements.subband_cqi[eNB_id][aarx][subband] =
              (signal_energy_nodc(dl_ch0,subband_size) + signal_energy_nodc(dl_ch1,subband_size));

            if ( ue->measurements.subband_cqi[eNB_id][aarx][subband] < 0)
              ue->measurements.subband_cqi[eNB_id][aarx][subband]=0;

            /*
            else
            ue->measurements.subband_cqi[eNB_id][aarx][subband]-=ue->measurements.n0_power[aarx];
            */

            ue->measurements.subband_cqi_tot[eNB_id][subband] += ue->measurements.subband_cqi[eNB_id][aarx][subband];
            ue->measurements.subband_cqi_dB[eNB_id][aarx][subband] = dB_fixed2(ue->measurements.subband_cqi[eNB_id][aarx][subband],
                ue->measurements.n0_power[aarx]);
          } else { // this is for the last subband which is smaller in size
            //      for (i=0;i<12;i++)
            //        printf("subband %d (%d) : %d,%d\n",subband,i,((short *)dl_ch0)[2*i],((short *)dl_ch0)[1+(2*i)]);
            ue->measurements.subband_cqi[eNB_id][aarx][subband] = (signal_energy_nodc(dl_ch0,last_subband_size) +
                signal_energy_nodc(dl_ch1,last_subband_size)); // - ue->measurements.n0_power[aarx];
            ue->measurements.subband_cqi_tot[eNB_id][subband] += ue->measurements.subband_cqi[eNB_id][aarx][subband];
            ue->measurements.subband_cqi_dB[eNB_id][aarx][subband] = dB_fixed2(ue->measurements.subband_cqi[eNB_id][aarx][subband],
                ue->measurements.n0_power[aarx]);
          }

          dl_ch1+=subband_size;
          dl_ch0+=subband_size;
          //    msg("subband_cqi[%d][%d][%d] => %d (%d dB)\n",eNB_id,aarx,subband,ue->measurements.subband_cqi[eNB_id][aarx][subband],ue->measurements.subband_cqi_dB[eNB_id][aarx][subband]);
        }

      }

      for (subband=0; subband<nb_subbands; subband++) {
        ue->measurements.subband_cqi_tot_dB[eNB_id][subband] = dB_fixed2(ue->measurements.subband_cqi_tot[eNB_id][subband],ue->measurements.n0_power_tot);
        //    msg("subband_cqi_tot[%d][%d] => %d dB (n0 %d)\n",eNB_id,subband,ue->measurements.subband_cqi_tot_dB[eNB_id][subband],ue->measurements.n0_power_tot);
      }

      for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
	//printf("aarx=%d", aarx);
        // skip the first 4 RE due to interpolation filter length of 5 (not possible to skip 5 due to 128i alignment, must be multiple of 128bit)

#if defined(__x86_64__) || defined(__i386__)
       __m128i pmi128_re,pmi128_im,mmtmpPMI0,mmtmpPMI1 /* ,mmtmpPMI2,mmtmpPMI3 */ ;

        dl_ch0_128    = (__m128i *)&ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[eNB_id][aarx][4];
        dl_ch1_128    = (__m128i *)&ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[eNB_id][2+aarx][4];
#elif defined(__arm__)
        int32x4_t pmi128_re,pmi128_im,mmtmpPMI0,mmtmpPMI1,mmtmpPMI0b,mmtmpPMI1b;

        dl_ch0_128    = (int16x8_t *)&ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[eNB_id][aarx][4];
        dl_ch1_128    = (int16x8_t *)&ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[eNB_id][2+aarx][4];

#endif
        for (subband=0; subband<nb_subbands; subband++) {


          // pmi
#if defined(__x86_64__) || defined(__i386__)

	  pmi128_re = _mm_xor_si128(pmi128_re,pmi128_re);
          pmi128_im = _mm_xor_si128(pmi128_im,pmi128_im);
#elif defined(__arm__)

          pmi128_re = vdupq_n_s32(0);
	  pmi128_im = vdupq_n_s32(0);
#endif
          // limit is the number of groups of 4 REs in a subband (12 = 4 RBs, 3 = 1 RB)
          // for 5 MHz channelization, there are 7 subbands, 6 of size 4 RBs and 1 of size 1 RB
          if ((N_RB_DL==6) || (subband<(nb_subbands-1)))
            limit = subband_size>>2;
          else
            limit = last_subband_size>>2;

          for (i=0; i<limit; i++) {

#if defined(__x86_64__) || defined(__i386__)
	      mmtmpPMI0 = _mm_xor_si128(mmtmpPMI0,mmtmpPMI0);
              mmtmpPMI1 = _mm_xor_si128(mmtmpPMI1,mmtmpPMI1);

            // For each RE in subband perform ch0 * conj(ch1)
            // multiply by conjugated channel
		//  print_ints("ch0",&dl_ch0_128[0]);
		//  print_ints("ch1",&dl_ch1_128[0]);

	    mmtmpPMI0 = _mm_madd_epi16(dl_ch0_128[0],dl_ch1_128[0]);
	         //  print_ints("re",&mmtmpPMI0);
            mmtmpPMI1 = _mm_shufflelo_epi16(dl_ch1_128[0],_MM_SHUFFLE(2,3,0,1));
              //  print_ints("_mm_shufflelo_epi16",&mmtmpPMI1);
            mmtmpPMI1 = _mm_shufflehi_epi16(mmtmpPMI1,_MM_SHUFFLE(2,3,0,1));
	        //  print_ints("_mm_shufflehi_epi16",&mmtmpPMI1);
            mmtmpPMI1 = _mm_sign_epi16(mmtmpPMI1,*(__m128i*)&conjugate[0]);
	       //  print_ints("_mm_sign_epi16",&mmtmpPMI1);
            mmtmpPMI1 = _mm_madd_epi16(mmtmpPMI1,dl_ch0_128[0]);
	       //   print_ints("mm_madd_epi16",&mmtmpPMI1);
            // mmtmpPMI1 contains imag part of 4 consecutive outputs (32-bit)
            pmi128_re = _mm_add_epi32(pmi128_re,mmtmpPMI0);
	     //   print_ints(" pmi128_re 0",&pmi128_re);
            pmi128_im = _mm_add_epi32(pmi128_im,mmtmpPMI1);
	       //   print_ints(" pmi128_im 0 ",&pmi128_im);

	  /*  mmtmpPMI0 = _mm_xor_si128(mmtmpPMI0,mmtmpPMI0);
            mmtmpPMI1 = _mm_xor_si128(mmtmpPMI1,mmtmpPMI1);

	    mmtmpPMI0 = _mm_madd_epi16(dl_ch0_128[1],dl_ch1_128[1]);
	         //  print_ints("re",&mmtmpPMI0);
            mmtmpPMI1 = _mm_shufflelo_epi16(dl_ch1_128[1],_MM_SHUFFLE(2,3,0,1));
              //  print_ints("_mm_shufflelo_epi16",&mmtmpPMI1);
            mmtmpPMI1 = _mm_shufflehi_epi16(mmtmpPMI1,_MM_SHUFFLE(2,3,0,1));
	        //  print_ints("_mm_shufflehi_epi16",&mmtmpPMI1);
            mmtmpPMI1 = _mm_sign_epi16(mmtmpPMI1,*(__m128i*)&conjugate);
	       //  print_ints("_mm_sign_epi16",&mmtmpPMI1);
            mmtmpPMI1 = _mm_madd_epi16(mmtmpPMI1,dl_ch0_128[1]);
	       //   print_ints("mm_madd_epi16",&mmtmpPMI1);
            // mmtmpPMI1 contains imag part of 4 consecutive outputs (32-bit)
            pmi128_re = _mm_add_epi32(pmi128_re,mmtmpPMI0);
	        //  print_ints(" pmi128_re 1",&pmi128_re);
            pmi128_im = _mm_add_epi32(pmi128_im,mmtmpPMI1);
	    //print_ints(" pmi128_im 1 ",&pmi128_im);*/

#elif defined(__arm__)

            mmtmpPMI0 = vmull_s16(((int16x4_t*)dl_ch0_128)[0], ((int16x4_t*)dl_ch1_128)[0]);
            mmtmpPMI1 = vmull_s16(((int16x4_t*)dl_ch0_128)[1], ((int16x4_t*)dl_ch1_128)[1]);
            pmi128_re = vqaddq_s32(pmi128_re,vcombine_s32(vpadd_s32(vget_low_s32(mmtmpPMI0),vget_high_s32(mmtmpPMI0)),vpadd_s32(vget_low_s32(mmtmpPMI1),vget_high_s32(mmtmpPMI1))));

            mmtmpPMI0b = vmull_s16(vrev32_s16(vmul_s16(((int16x4_t*)dl_ch0_128)[0],*(int16x4_t*)conjugate)), ((int16x4_t*)dl_ch1_128)[0]);
            mmtmpPMI1b = vmull_s16(vrev32_s16(vmul_s16(((int16x4_t*)dl_ch0_128)[1],*(int16x4_t*)conjugate)), ((int16x4_t*)dl_ch1_128)[1]);
            pmi128_im = vqaddq_s32(pmi128_im,vcombine_s32(vpadd_s32(vget_low_s32(mmtmpPMI0b),vget_high_s32(mmtmpPMI0b)),vpadd_s32(vget_low_s32(mmtmpPMI1b),vget_high_s32(mmtmpPMI1b))));

#endif
            dl_ch0_128++;
            dl_ch1_128++;
          }

          ue->measurements.subband_pmi_re[eNB_id][subband][aarx] = (((int *)&pmi128_re)[0] + ((int *)&pmi128_re)[1] + ((int *)&pmi128_re)[2] + ((int *)&pmi128_re)[3])>>2;
          ue->measurements.subband_pmi_im[eNB_id][subband][aarx] = (((int *)&pmi128_im)[0] + ((int *)&pmi128_im)[1] + ((int *)&pmi128_im)[2] + ((int *)&pmi128_im)[3])>>2;
          ue->measurements.wideband_pmi_re[eNB_id][aarx] += ue->measurements.subband_pmi_re[eNB_id][subband][aarx];
          ue->measurements.wideband_pmi_im[eNB_id][aarx] += ue->measurements.subband_pmi_im[eNB_id][subband][aarx];
        } // subband loop
      } // rx antenna loop
    }  // if frame_parms->mode1_flag == 0
    else {
      // cqi information only for mode 1
      for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
        dl_ch0    = &ue->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[eNB_id][aarx][4];

        for (subband=0; subband<7; subband++) {

          // cqi
          if (aarx==0)
            ue->measurements.subband_cqi_tot[eNB_id][subband]=0;

          if (subband<6) {
            //      for (i=0;i<48;i++)
            //        printf("subband %d (%d) : %d,%d\n",subband,i,((short *)dl_ch0)[2*i],((short *)dl_ch0)[1+(2*i)]);
            ue->measurements.subband_cqi[eNB_id][aarx][subband] =
              (signal_energy_nodc(dl_ch0,48) ) - ue->measurements.n0_power[aarx];

            ue->measurements.subband_cqi_tot[eNB_id][subband] += ue->measurements.subband_cqi[eNB_id][aarx][subband];
            ue->measurements.subband_cqi_dB[eNB_id][aarx][subband] = dB_fixed2(ue->measurements.subband_cqi[eNB_id][aarx][subband],
                ue->measurements.n0_power[aarx]);
          } else {
            //      for (i=0;i<12;i++)
            //        printf("subband %d (%d) : %d,%d\n",subband,i,((short *)dl_ch0)[2*i],((short *)dl_ch0)[1+(2*i)]);
            ue->measurements.subband_cqi[eNB_id][aarx][subband] = (signal_energy_nodc(dl_ch0,12) ) - ue->measurements.n0_power[aarx];
            ue->measurements.subband_cqi_tot[eNB_id][subband] += ue->measurements.subband_cqi[eNB_id][aarx][subband];
            ue->measurements.subband_cqi_dB[eNB_id][aarx][subband] = dB_fixed2(ue->measurements.subband_cqi[eNB_id][aarx][subband],
                ue->measurements.n0_power[aarx]);
          }

          dl_ch1+=48;
          //    msg("subband_cqi[%d][%d][%d] => %d (%d dB)\n",eNB_id,aarx,subband,ue->measurements.subband_cqi[eNB_id][aarx][subband],ue->measurements.subband_cqi_dB[eNB_id][aarx][subband]);
        }
      }

      for (subband=0; subband<nb_subbands; subband++) {
        ue->measurements.subband_cqi_tot_dB[eNB_id][subband] = dB_fixed2(ue->measurements.subband_cqi_tot[eNB_id][subband],ue->measurements.n0_power_tot);
      }
    }

    //ue->measurements.rank[eNB_id] = 0;

    for (i=0; i<nb_subbands; i++) {
      ue->measurements.selected_rx_antennas[eNB_id][i] = 0;

      if (frame_parms->nb_antennas_rx>1) {
        if (ue->measurements.subband_cqi_dB[eNB_id][0][i] >= ue->measurements.subband_cqi_dB[eNB_id][1][i])
          ue->measurements.selected_rx_antennas[eNB_id][i] = 0;
        else
          ue->measurements.selected_rx_antennas[eNB_id][i] = 1;
      } else
        ue->measurements.selected_rx_antennas[eNB_id][i] = 0;
    }

    // if(eNB_id==0)
    // printf("in lte_ue_measurements: selected rx_antenna[eNB_id==0]:%u\n", ue->measurements.selected_rx_antennas[eNB_id][i]);
  }  // eNB_id loop

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}


void lte_ue_measurements_emul(PHY_VARS_UE *ue,uint8_t subframe,uint8_t eNB_id)
{

  msg("[PHY] EMUL UE lte_ue_measurements_emul subframe %d, eNB_id %d\n",subframe,eNB_id);
}

