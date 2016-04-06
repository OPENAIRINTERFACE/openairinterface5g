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

/*! \file pusch_pc.c
 * \brief Implementation of UE PUSCH Power Control procedures from 36.213 LTE specifications (Section
 * \author R. Knopp
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */

#include "defs.h"
#include "PHY/defs.h"
#include "PHY/LTE_TRANSPORT/proto.h"
#include "PHY/extern.h"

// This is the formula from Section 5.1.1.1 in 36.213 100*10*log10((2^(MPR*Ks)-1)), where MPR is in the range [0,6] and Ks=1.25
int16_t hundred_times_delta_TF[100] = {-32768,-1268,-956,-768,-631,-523,-431,-352,-282,-219,-161,-107,-57,-9,36,79,120,159,197,234,269,304,337,370,402,434,465,495,525,555,583,612,640,668,696,723,750,777,803,829,856,881,907,933,958,983,1008,1033,1058,1083,1108,1132,1157,1181,1205,1229,1254,1278,1302,1325,1349,1373,1397,1421,1444,1468,1491,1515,1538,1562,1585,1609,1632,1655,1679,1702,1725,1748,1772,1795,1818,1841,1864,1887,1910,1933,1956,1980,2003,2026,2049,2072,2095,2118,2141,2164,2186,2209,2232,2255};
uint16_t hundred_times_log10_NPRB[100] = {0,301,477,602,698,778,845,903,954,1000,1041,1079,1113,1146,1176,1204,1230,1255,1278,1301,1322,1342,1361,1380,1397,1414,1431,1447,1462,1477,1491,1505,1518,1531,1544,1556,1568,1579,1591,1602,1612,1623,1633,1643,1653,1662,1672,1681,1690,1698,1707,1716,1724,1732,1740,1748,1755,1763,1770,1778,1785,1792,1799,1806,1812,1819,1826,1832,1838,1845,1851,1857,1863,1869,1875,1880,1886,1892,1897,1903,1908,1913,1919,1924,1929,1934,1939,1944,1949,1954,1959,1963,1968,1973,1977,1982,1986,1991,1995,2000};

int16_t get_hundred_times_delta_IF_eNB(PHY_VARS_eNB *phy_vars_eNB,uint8_t UE_id,uint8_t harq_pid, uint8_t bw_factor)
{

  uint32_t Nre,sumKr,MPR_x100,Kr,r;
  uint16_t beta_offset_pusch;

  DevAssert( UE_id < NUMBER_OF_UE_MAX+1 );
  DevAssert( harq_pid < 8 );

  Nre = phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->Nsymb_initial *
        phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->nb_rb*12;

  sumKr = 0;

  for (r=0; r<phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->C; r++) {
    if (r<phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->Cminus)
      Kr = phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->Kminus;
    else
      Kr = phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->Kplus;

    sumKr += Kr;
  }

  if (Nre==0)
    return(0);

  MPR_x100 = 100*sumKr/Nre;
  // Note: MPR=is the effective spectral efficiency of the PUSCH
  // FK 20140908 sumKr is only set after the ulsch_encoding

  beta_offset_pusch = 8;
  //(phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->control_only == 1) ? phy_vars_eNB->ulsch_eNB[UE_id]->beta_offset_cqi_times8:8;

  DevAssert( UE_id < NUMBER_OF_UE_MAX );
//#warning "This condition happens sometimes. Need more investigation" // navid
  //DevAssert( MPR_x100/6 < 100 );

  if (phy_vars_eNB->ul_power_control_dedicated[UE_id].deltaMCS_Enabled == 1) {
    // This is the formula from Section 5.1.1.1 in 36.213 10*log10(deltaIF_PUSCH = (2^(MPR*Ks)-1)*beta_offset_pusch)
    if (bw_factor == 1) {
      uint8_t nb_rb = phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->nb_rb;
      return(hundred_times_delta_TF[MPR_x100/6]+10*dB_fixed_times10((beta_offset_pusch)>>3)) + hundred_times_log10_NPRB[nb_rb-1];
    } else
      return(hundred_times_delta_TF[MPR_x100/6]+10*dB_fixed_times10((beta_offset_pusch)>>3));
  } else {
    return(0);
  }
}

int16_t get_hundred_times_delta_IF_mac(module_id_t module_idP, uint8_t CC_id, rnti_t rnti, uint8_t harq_pid)
{
  int8_t UE_id = find_ue( rnti, PHY_vars_eNB_g[module_idP][CC_id] );

  if (UE_id == -1) {
    // not found
    return 0;
  }

  return get_hundred_times_delta_IF_eNB( PHY_vars_eNB_g[module_idP][CC_id], UE_id, harq_pid, 0 );
}

int16_t get_hundred_times_delta_IF(PHY_VARS_UE *phy_vars_ue,uint8_t eNB_id,uint8_t harq_pid)
{

  uint32_t Nre = phy_vars_ue->ulsch_ue[eNB_id]->harq_processes[harq_pid]->Nsymb_initial *
                 phy_vars_ue->ulsch_ue[eNB_id]->harq_processes[harq_pid]->nb_rb*12;

  if (Nre==0)
    return(0);

  uint32_t MPR_x100 = 100*phy_vars_ue->ulsch_ue[eNB_id]->harq_processes[harq_pid]->sumKr/Nre;
  // Note: MPR=is the effective spectral efficiency of the PUSCH
  // FK 20140908 sumKr is only set after the ulsch_encoding

  uint16_t beta_offset_pusch = (phy_vars_ue->ulsch_ue[eNB_id]->harq_processes[harq_pid]->control_only == 1) ?
                               phy_vars_ue->ulsch_ue[eNB_id]->beta_offset_cqi_times8:8;

  if (phy_vars_ue->ul_power_control_dedicated[eNB_id].deltaMCS_Enabled == 1) {
    // This is the formula from Section 5.1.1.1 in 36.213 10*log10(deltaIF_PUSCH = (2^(MPR*Ks)-1)*beta_offset_pusch)
    return(hundred_times_delta_TF[MPR_x100/6]+10*dB_fixed_times10((beta_offset_pusch)>>3));
  } else {
    return(0);
  }
}



uint8_t alpha_lut[8] = {0,40,50,60,70,80,90,100};

void pusch_power_cntl(PHY_VARS_UE *phy_vars_ue,uint8_t subframe,uint8_t eNB_id,uint8_t j, uint8_t abstraction_flag)
{


  uint8_t harq_pid = subframe2harq_pid(&phy_vars_ue->lte_frame_parms,
                                       phy_vars_ue->frame_tx,
                                       subframe);

  uint8_t nb_rb = phy_vars_ue->ulsch_ue[eNB_id]->harq_processes[harq_pid]->nb_rb;
  int8_t PL;


  // P_pusch = 10*log10(nb_rb + P_opusch(j)+ alpha(u)*PL + delta_TF(i) + f(i))
  //
  // P_opusch(0) = P_oPTR + deltaP_Msg3 if PUSCH is transporting Msg3
  // else
  // P_opusch(0) = PO_NOMINAL_PUSCH(j) + P_O_UE_PUSCH(j)
  PL = get_PL(phy_vars_ue->Mod_id,phy_vars_ue->CC_id,eNB_id);

  phy_vars_ue->ulsch_ue[eNB_id]->Po_PUSCH = (hundred_times_log10_NPRB[nb_rb-1]+
      get_hundred_times_delta_IF(phy_vars_ue,eNB_id,harq_pid) +
      100*phy_vars_ue->ulsch_ue[eNB_id]->f_pusch)/100;

  if( phy_vars_ue->ulsch_ue_Msg3_active[eNB_id] == 1) {  // Msg3 PUSCH

    phy_vars_ue->ulsch_ue[eNB_id]->Po_PUSCH += (mac_xface->get_Po_NOMINAL_PUSCH(phy_vars_ue->Mod_id,0) + PL);

    LOG_I(PHY,"[UE  %d][RAPROC] frame %d, subframe %d: Msg3 Po_PUSCH %d dBm (%d,%d,100*PL=%d,%d,%d)\n",
          phy_vars_ue->Mod_id,phy_vars_ue->frame_tx,subframe,phy_vars_ue->ulsch_ue[eNB_id]->Po_PUSCH,
          100*mac_xface->get_Po_NOMINAL_PUSCH(phy_vars_ue->Mod_id,0),
          hundred_times_log10_NPRB[nb_rb-1],
          100*PL,
          get_hundred_times_delta_IF(phy_vars_ue,eNB_id,harq_pid),
          100*phy_vars_ue->ulsch_ue[eNB_id]->f_pusch);
  } else if (j==0) { // SPS PUSCH
  } else if (j==1) { // Normal PUSCH

    phy_vars_ue->ulsch_ue[eNB_id]->Po_PUSCH +=  ((alpha_lut[phy_vars_ue->lte_frame_parms.ul_power_control_config_common.alpha]*PL)/100);
    phy_vars_ue->ulsch_ue[eNB_id]->Po_PUSCH +=  phy_vars_ue->lte_frame_parms.ul_power_control_config_common.p0_NominalPUSCH;
    phy_vars_ue->ulsch_ue[eNB_id]->PHR       =  phy_vars_ue->tx_power_max_dBm-phy_vars_ue->ulsch_ue[eNB_id]->Po_PUSCH;  

    if (phy_vars_ue->ulsch_ue[eNB_id]->PHR < -23)
      phy_vars_ue->ulsch_ue[eNB_id]->PHR = -23;
    else if (phy_vars_ue->ulsch_ue[eNB_id]->PHR > 40)
      phy_vars_ue->ulsch_ue[eNB_id]->PHR = 40;

    LOG_D(PHY,"[UE  %d][PUSCH %d] frame %d, subframe %d: Po_PUSCH %d dBm : tx power %d, Po_NOMINAL_PUSCH %d,log10(NPRB) %f,PHR %d, PL %d, alpha*PL %f,delta_IF %f,f_pusch %d\n",
          phy_vars_ue->Mod_id,harq_pid,phy_vars_ue->frame_tx,subframe,
          phy_vars_ue->ulsch_ue[eNB_id]->Po_PUSCH,
          phy_vars_ue->tx_power_max_dBm,
          phy_vars_ue->lte_frame_parms.ul_power_control_config_common.p0_NominalPUSCH,
          hundred_times_log10_NPRB[nb_rb-1]/100.0,
          phy_vars_ue->ulsch_ue[eNB_id]->PHR,
          PL,
          alpha_lut[phy_vars_ue->lte_frame_parms.ul_power_control_config_common.alpha]*PL/100.0,
          get_hundred_times_delta_IF(phy_vars_ue,eNB_id,harq_pid)/100.0,
          phy_vars_ue->ulsch_ue[eNB_id]->f_pusch);
  }

}

int8_t get_PHR(uint8_t Mod_id, uint8_t CC_id,uint8_t eNB_index)
{

  return PHY_vars_UE_g[Mod_id][CC_id]->ulsch_ue[eNB_index]->PHR;
}

// uint8_t eNB_id,uint8_t harq_pid, uint8_t UE_id,
int16_t estimate_ue_tx_power(uint32_t tbs, uint32_t nb_rb, uint8_t control_only, lte_prefix_type_t ncp, uint8_t use_srs)
{

  /// The payload + CRC size in bits, "B"
  uint32_t B;
  /// Number of code segments
  uint32_t C;
  /// Number of "small" code segments
  uint32_t Cminus;
  /// Number of "large" code segments
  uint32_t Cplus;
  /// Number of bits in "small" code segments (<6144)
  uint32_t Kminus;
  /// Number of bits in "large" code segments (<6144)
  uint32_t Kplus;
  /// Total number of bits across all segments
  uint32_t sumKr;
  /// Number of "Filler" bits
  uint32_t F;
  // num resource elements
  uint32_t num_re=0.0;
  // num symbols
  uint32_t num_symb=0.0;
  /// effective spectral efficiency of the PUSCH
  uint32_t MPR_x100=0;
  /// beta_offset
  uint16_t beta_offset_pusch_x8=8;
  /// delta mcs
  float delta_mcs=0.0;
  /// bandwidth factor
  float bw_factor=0.0;

  B= tbs+24;
  lte_segmentation(NULL,
                   NULL,
                   B,
                   &C,
                   &Cplus,
                   &Cminus,
                   &Kplus,
                   &Kminus,
                   &F);


  sumKr = Cminus*Kminus + Cplus*Kplus;
  num_symb = 12-(ncp<<1)-(use_srs==0?0:1);
  num_re = num_symb * nb_rb * 12;

  if (num_re == 0)
    return(0);

  MPR_x100 = 100*sumKr/num_re;

  if (control_only == 1 )
    beta_offset_pusch_x8=8; // fixme

  //(beta_offset_pusch_x8=phy_vars_ue->ulsch_ue[eNB_id]->harq_processes[harq_pid]->control_only == 1) ? phy_vars_ue->ulsch_ue[eNB_id]->beta_offset_cqi_times8:8;

  // if deltamcs_enabledm
  delta_mcs = ((hundred_times_delta_TF[MPR_x100/6]+10*dB_fixed_times10((beta_offset_pusch_x8)>>3))/100.0);
  bw_factor = (hundred_times_log10_NPRB[nb_rb-1]/100.0);
#ifdef DEBUG_SEGMENTATION
  printf("estimated ue tx power %d (num_re %d, sumKr %d, mpr_x100 %d, delta_mcs %f, bw_factor %f)\n",
         (int16_t)ceil(delta_mcs + bw_factor), num_re, sumKr, MPR_x100, delta_mcs, bw_factor);
#endif
  return (int16_t)ceil(delta_mcs + bw_factor);

}
