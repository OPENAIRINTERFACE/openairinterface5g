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

#include "PHY/defs_eNB.h"
#include "PHY/defs_UE.h"

//#define DEBUG_PC 0
/*
double ratioPB[2][4]={{ 1.0,4.0/5.0,3.0/5.0,2.0/5.0},
          { 5.0/4.0,1.0,3.0/4.0,1.0/2.0}};
*/

double ratioPB[2][4]={{ 0.00000,  -0.96910,  -2.21849,  -3.97940}, //in db
                      { 0.96910,   0.00000,  -1.24939,  -3.01030}};

double pa_values[8]={-6.0,-4.77,-3.0,-1.77,0.0,1.0,2.0,3.0}; //reported by higher layers

double get_pa_dB(uint8_t pa)
{
  AssertFatal(pa<8,"pa %d is not in (0...7)\n",pa);

  return(pa_values[pa]);

}

double computeRhoA_eNB(uint8_t pa,
                       LTE_eNB_DLSCH_t *dlsch_eNB, int dl_power_off, uint8_t n_antenna_port){
  double rho_a_dB;
  double sqrt_rho_a_lin;

  rho_a_dB = get_pa_dB(pa);

  if(!dl_power_off) //if dl_power_offset is 0, this is for MU-interference, TM5
    rho_a_dB-=10*log10(2);

  if(n_antenna_port==4) // see TS 36.213 Section 5.2
    rho_a_dB+=10*log10(2);

  sqrt_rho_a_lin= pow(10,(0.05*rho_a_dB));

  if (dlsch_eNB) {
     dlsch_eNB->sqrt_rho_a= (short) (sqrt_rho_a_lin*pow(2,13));
     dlsch_eNB->pa = pa;
  }
#if DEBUG_PC
  printf("eNB: p_a=%d, value=%f, sqrt_rho_a=%d\n",p_a,pa_values[ pdsch_config_dedicated->p_a],dlsch_eNB->sqrt_rho_a);
#endif

  return(rho_a_dB);
}

double computeRhoB_eNB(uint8_t pa,
                       uint8_t pb,
                       uint8_t n_antenna_port,
                       LTE_eNB_DLSCH_t *dlsch_eNB,
                       int dl_power_off)
{

  double rho_a_dB, rho_b_dB;
  double sqrt_rho_b_lin;

  AssertFatal(pa<8,"pa %d is not in (0...7)\n",pa);
  AssertFatal(pb<4,"pb %d is not in (0...3)\n",pb);
  rho_a_dB= computeRhoA_eNB(pa,dlsch_eNB,dl_power_off, n_antenna_port);

  if(n_antenna_port>1)
    rho_b_dB= ratioPB[1][pb] + rho_a_dB;
  else
    rho_b_dB= ratioPB[0][pb] + rho_a_dB;

  sqrt_rho_b_lin= pow(10,(0.05*rho_b_dB));

  if (dlsch_eNB) {
    dlsch_eNB->sqrt_rho_b= (short) (sqrt_rho_b_lin*pow(2,13));
    dlsch_eNB->pb = pb;
  }
#ifdef DEBUG_PC
  printf("eNB: n_ant=%d, p_b=%d -> rho_b/rho_a=%f -> sqrt_rho_b=%d\n",n_antenna_port,pb,ratioPB[1][pb],dlsch_eNB->sqrt_rho_b);
#endif
  return(rho_b_dB);
}

double computeRhoA_UE(PDSCH_CONFIG_DEDICATED *pdsch_config_dedicated,
                      LTE_UE_DLSCH_t *dlsch_ue,
                      unsigned char dl_power_off,
                      uint8_t n_antenna_port
                    ){

  double rho_a_dB;
  double sqrt_rho_a_lin;

  rho_a_dB = get_pa_dB(pdsch_config_dedicated->p_a);

  if(!dl_power_off)
    rho_a_dB-=10*log10(2);
  //if dl_power_offset is 0, this is for MU-interference, TM5. But in practice UE may assume 16 or 64QAM TM4 as multiuser

   if(n_antenna_port==4) // see TS 36.213 Section 5.2
    rho_a_dB=+10*log10(2);

  sqrt_rho_a_lin= pow(10,(0.05*rho_a_dB));

  dlsch_ue->sqrt_rho_a= (short) (sqrt_rho_a_lin*pow(2,13));

#ifdef DEBUG_PC
  printf("UE: p_a=%d, value=%f, dl_power_off=%d, sqrt_rho_a=%d\n",pdsch_config_dedicated->p_a,pa_values[ pdsch_config_dedicated->p_a],dl_power_off,dlsch_ue->sqrt_rho_a);
#endif

  return(rho_a_dB);
}

double computeRhoB_UE(PDSCH_CONFIG_DEDICATED  *pdsch_config_dedicated,
                      PDSCH_CONFIG_COMMON *pdsch_config_common,
                      uint8_t n_antenna_port,
                      LTE_UE_DLSCH_t *dlsch_ue,
                      unsigned char dl_power_off)
{

  double rho_a_dB, rho_b_dB;
  double sqrt_rho_b_lin;

  rho_a_dB= computeRhoA_UE(pdsch_config_dedicated,dlsch_ue,dl_power_off, n_antenna_port);

  if(n_antenna_port>1)
    rho_b_dB= ratioPB[1][pdsch_config_common->p_b] + rho_a_dB;
  else
    rho_b_dB= ratioPB[0][pdsch_config_common->p_b] + rho_a_dB;

  sqrt_rho_b_lin= pow(10,(0.05*rho_b_dB));

  dlsch_ue->sqrt_rho_b= (short) (sqrt_rho_b_lin*pow(2,13));

#ifdef DEBUG_PC
  printf("UE: p_b=%d, n_ant=%d -> ratio=%f -> sqrt_rho_b=%d\n",pdsch_config_common->p_b, n_antenna_port,ratioPB[1][pdsch_config_common->p_b],dlsch_ue->sqrt_rho_b);
#endif
  return(rho_b_dB);
}

