/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
#include "PHY/defs.h"
#include "PHY/impl_defs_lte.h"

//#define DEBUG_PC 1

/*
double ratioPB[2][4]={{ 1.0,4.0/5.0,3.0/5.0,2.0/5.0},
          { 5.0/4.0,1.0,3.0/4.0,1.0/2.0}};
*/

double ratioPB[2][4]= {{ 0.00000,  -0.96910,  -2.21849,  -3.97940},
  { 0.96910,   0.00000,  -1.24939,  -3.01030}
};

double pa_values[8]= {-6.0,-4.77,-3.0,-1.77,0.0,1.0,2.0,3.0};

double get_pa_dB(PDSCH_CONFIG_DEDICATED *pdsch_config_dedicated)
{
  return(pa_values[pdsch_config_dedicated->p_a]);
}

double computeRhoA_eNB(PDSCH_CONFIG_DEDICATED *pdsch_config_dedicated,
                       LTE_eNB_DLSCH_t *dlsch_eNB,int dl_power_off)
{
  double rho_a_dB;
  double sqrt_rho_a_lin;

  rho_a_dB = pa_values[ pdsch_config_dedicated->p_a];

  if(!dl_power_off)
    rho_a_dB-=10*log10(2);

  sqrt_rho_a_lin= pow(10,(0.05*rho_a_dB));

  dlsch_eNB->sqrt_rho_a= (short) (sqrt_rho_a_lin*pow(2,13));

#ifdef DEBUG_PC
  printf("sqrt_rho_a(eNB):%d\n",dlsch_eNB->sqrt_rho_a);
#endif

  return(rho_a_dB);
}

double computeRhoB_eNB(PDSCH_CONFIG_DEDICATED  *pdsch_config_dedicated,
                       PDSCH_CONFIG_COMMON *pdsch_config_common,
                       uint8_t n_antenna_port,
                       LTE_eNB_DLSCH_t *dlsch_eNB,
                       int dl_power_off)
{

  double rho_a_dB, rho_b_dB;
  double sqrt_rho_b_lin;

  rho_a_dB= computeRhoA_eNB(pdsch_config_dedicated,dlsch_eNB,dl_power_off);

  if(n_antenna_port>1)
    rho_b_dB= ratioPB[1][pdsch_config_common->p_b] + rho_a_dB;
  else
    rho_b_dB= ratioPB[0][pdsch_config_common->p_b] + rho_a_dB;

  sqrt_rho_b_lin= pow(10,(0.05*rho_b_dB));

  dlsch_eNB->sqrt_rho_b= (short) (sqrt_rho_b_lin*pow(2,13));

#ifdef DEBUG_PC
  printf("sqrt_rho_b(eNB):%d\n",dlsch_eNB->sqrt_rho_b);
#endif
  return(rho_b_dB);
}


double computeRhoA_UE(PDSCH_CONFIG_DEDICATED *pdsch_config_dedicated,
                      LTE_UE_DLSCH_t *dlsch_ue,
                      unsigned char dl_power_off)
{

  double rho_a_dB;
  double sqrt_rho_a_lin;

  rho_a_dB = pa_values[ pdsch_config_dedicated->p_a];

  if(!dl_power_off)
    rho_a_dB-=10*log10(2);

  sqrt_rho_a_lin= pow(10,(0.05*rho_a_dB));

  dlsch_ue->sqrt_rho_a= (short) (sqrt_rho_a_lin*pow(2,13));

#ifdef DEBUG_PC
  printf("sqrt_rho_a(ue):%d\n",dlsch_ue->sqrt_rho_a);
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

  rho_a_dB= computeRhoA_UE(pdsch_config_dedicated,dlsch_ue,dl_power_off);

  if(n_antenna_port>1)
    rho_b_dB= ratioPB[1][pdsch_config_common->p_b] + rho_a_dB;
  else
    rho_b_dB= ratioPB[0][pdsch_config_common->p_b] + rho_a_dB;

  sqrt_rho_b_lin= pow(10,(0.05*rho_b_dB));

  dlsch_ue->sqrt_rho_b= (short) (sqrt_rho_b_lin*pow(2,13));

#ifdef DEBUG_PC
  printf("sqrt_rho_b(ue):%d\n",dlsch_ue->sqrt_rho_b);
#endif
  return(rho_b_dB);
}


