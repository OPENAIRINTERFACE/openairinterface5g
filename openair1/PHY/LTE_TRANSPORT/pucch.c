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

/*! \file PHY/LTE_TRANSPORT/pucch.c
* \brief Top-level routines for generating and decoding the PUCCH physical channel V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/
#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h" 
#include "LAYER2/MAC/mac.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"


//uint8_t ncs_cell[20][7];
//#define DEBUG_PUCCH_TXS
//#define DEBUG_PUCCH_RX

#include "pucch_extern.h"

int16_t cfo_pucch_np[24*7] = {20787,-25330,27244,-18205,31356,-9512,32767,0,31356,9511,27244,18204,20787,25329,
                              27244,-18205,30272,-12540,32137,-6393,32767,0,32137,6392,30272,12539,27244,18204,
                              31356,-9512,32137,-6393,32609,-3212,32767,0,32609,3211,32137,6392,31356,9511,
                              32767,0,32767,0,32767,0,32767,0,32767,0,32767,0,32767,0,
                              31356,9511,32137,6392,32609,3211,32767,0,32609,-3212,32137,-6393,31356,-9512,
                              27244,18204,30272,12539,32137,6392,32767,0,32137,-6393,30272,-12540,27244,-18205,
                              20787,25329,27244,18204,31356,9511,32767,0,31356,-9512,27244,-18205,20787,-25330
                             };

int16_t cfo_pucch_ep[24*6] = {24278,-22005,29621,-14010,32412,-4808,32412,4807,29621,14009,24278,22004,
                              28897,-15447,31356,-9512,32609,-3212,32609,3211,31356,9511,28897,15446,
                              31785,-7962,32412,-4808,32727,-1608,32727,1607,32412,4807,31785,7961,
                              32767,0,32767,0,32767,0,32767,0,32767,0,32767,0,
                              31785,7961,32412,4807,32727,1607,32727,-1608,32412,-4808,31785,-7962,
                              28897,15446,31356,9511,32609,3211,32609,-3212,31356,-9512,28897,-15447,
                              24278,22004,29621,14009,32412,4807,32412,-4808,29621,-14010,24278,-22005
                             };




/* PUCCH format3 >> */
/* SubCarrier Demap */
uint16_t pucchfmt3_subCarrierDeMapping( PHY_VARS_eNB *eNB,
                                        int16_t SubCarrierDeMapData[NB_ANTENNAS_RX][14][12][2],
                                        uint16_t n3_pucch )
{
    LTE_eNB_COMMON *eNB_common_vars  = &eNB->common_vars;
    LTE_DL_FRAME_PARMS  *frame_parms = &eNB->frame_parms;
    int16_t             *rxptr;
    uint8_t             N_UL_symb    = D_NSYM1SLT;                      // only Normal CP format
    uint16_t            m;                                              // Mapping to physical resource blocks(m)
    uint32_t            aa;
    uint16_t            k, l;
    uint32_t            symbol_offset;
    uint16_t            carrier_offset;
    
    
    m = n3_pucch / D_NPUCCH_SF5;
    
    // Do detection
    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        
        for (l=0; l<D_NSYM1SF; l++) {
            
            if ((l<N_UL_symb) && ((m&1) == 0))
                carrier_offset = (m*6) + frame_parms->first_carrier_offset;
            else if ((l<N_UL_symb) && ((m&1) == 1))
                carrier_offset = frame_parms->first_carrier_offset + (frame_parms->N_RB_DL - (m>>1) - 1)*12;
            else if ((m&1) == 0)
                carrier_offset = frame_parms->first_carrier_offset + (frame_parms->N_RB_DL - (m>>1) - 1)*12;
            else
                carrier_offset = (((m-1)*6) + frame_parms->first_carrier_offset);
            
            if (carrier_offset > frame_parms->ofdm_symbol_size)
                carrier_offset -= (frame_parms->ofdm_symbol_size);
            
            symbol_offset = (unsigned int)frame_parms->ofdm_symbol_size*l;
            rxptr = (int16_t *)&eNB_common_vars->rxdataF[aa][symbol_offset];
            
            for (k=0; k<12; k++,carrier_offset++) {
                SubCarrierDeMapData[aa][l][k][0] = (int16_t)rxptr[carrier_offset<<1];      // DeMapping Data I
                SubCarrierDeMapData[aa][l][k][1] = (int16_t)rxptr[1+(carrier_offset<<1)];  // DeMapping Date Q
                
                if (carrier_offset==frame_parms->ofdm_symbol_size)
                    carrier_offset = 0;
                
		/*                #ifdef DEBUG_PUCCH_RX
                    LOG_D(PHY,"[eNB] PUCCH subframe %d (%d,%d,%d,%d) : (%d,%d)\n",subframe,l,k,carrier_offset,m,
                    SubCarrierDeMapData[aa][l][k][0],SubCarrierDeMapData[aa][l][k][1]);
		    #endif*/
            }
        }
    }
    return 0;
}

/* cyclic shift hopping remove */
uint16_t pucchfmt3_Baseseq_csh_remove( int16_t SubCarrierDeMapData[NB_ANTENNAS_RX][14][12][2],
                                       int16_t CshData_fmt3[NB_ANTENNAS_RX][14][12][2],
                                       LTE_DL_FRAME_PARMS *frame_parms,
                                       uint8_t subframe,
                                       uint8_t ncs_cell[20][7] )
{
    //int16_t     calctmp_baSeq[2];
    int16_t     calctmp_beta[2];
    int16_t     calctmp_alphak[2];
    int16_t     calctmp_SCDeMapData_alphak[2];
    int32_t     n_cell_cs_div64;
    int32_t     n_cell_cs_modNSC_RB;
    
    //int32_t     NSlot1subframe  = D_NSLT1SF;
    int32_t     NSym1slot       = D_NSYM1SLT; // Symbol per 1slot
    int32_t     NSym1subframe   = D_NSYM1SF;  // Symbol per 1subframe
    int32_t     aa, symNo, slotNo, sym, k;
    
    
    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {  // Antenna
        for (symNo=0; symNo<NSym1subframe; symNo++) {   // Symbol
            slotNo = symNo / NSym1slot;
            sym = symNo % NSym1slot;
            
            n_cell_cs_div64 = (int32_t)(ncs_cell[2*subframe+slotNo][sym]/64.0);
            n_cell_cs_modNSC_RB = ncs_cell[2*subframe+slotNo][sym] % 12;
            
            // for canceling e^(j*PI|_n_cs^cell(ns,l)/64_|/2).
            calctmp_beta[0] = RotTBL_re[(n_cell_cs_div64)&0x3];
            calctmp_beta[1] = RotTBL_im[(n_cell_cs_div64)&0x3];
            
            for (k=0; k<12; k++) {  // Sub Carrier
                
                // for canceling being cyclically shifted"(i+n_cs^cell(ns,l))".
                // e^((j*2PI(n_cs^cell(ns,l) mod N_SC)/N_SC)*k).
                calctmp_alphak[0] = alphaTBL_re[((n_cell_cs_modNSC_RB)*k)%12];
                calctmp_alphak[1] = alphaTBL_im[((n_cell_cs_modNSC_RB)*k)%12];
                
                // e^(-alphar*k)*r_l,m,n,k
                calctmp_SCDeMapData_alphak[0] = (((int32_t)SubCarrierDeMapData[aa][symNo][k][0] * calctmp_alphak[0] + (int32_t)SubCarrierDeMapData[aa][symNo][k][1] * calctmp_alphak[1])>>15);
                calctmp_SCDeMapData_alphak[1] = (((int32_t)SubCarrierDeMapData[aa][symNo][k][1] * calctmp_alphak[0] - (int32_t)SubCarrierDeMapData[aa][symNo][k][0] * calctmp_alphak[1])>>15);
                
                // (e^(-alphar*k)*r_l,m,n,k) * e^(-beta)
                CshData_fmt3[aa][symNo][k][0] = (((int32_t)calctmp_SCDeMapData_alphak[0] * calctmp_beta[0] + (int32_t)calctmp_SCDeMapData_alphak[1] * calctmp_beta[1])>>15);
                CshData_fmt3[aa][symNo][k][1] = (((int32_t)calctmp_SCDeMapData_alphak[1] * calctmp_beta[0] - (int32_t)calctmp_SCDeMapData_alphak[0] * calctmp_beta[1])>>15);
            }
        }
    }
    return 0;
}

#define MAXROW_TBL_SF5_OS_IDX    (5)    // Orthogonal sequence index
const int16_t TBL_3_SF5_GEN_N_DASH_NS[MAXROW_TBL_SF5_OS_IDX] = {0,3,6,8,10};

#define MAXROW_TBL_SF4_OS_IDX    (4)    // Orthogonal sequence index
const int16_t TBL_3_SF4_GEN_N_DASH_NS[MAXROW_TBL_SF4_OS_IDX] = {0,3,6,9};

/* Channel estimation */
uint16_t pucchfmt3_ChannelEstimation( int16_t SubCarrierDeMapData[NB_ANTENNAS_RX][14][12][2],
                                      double delta_theta[NB_ANTENNAS_RX][12],
                                      int16_t ChestValue[NB_ANTENNAS_RX][2][12][2],
                                      int16_t *Interpw,
                                      uint8_t subframe,
                                      uint8_t shortened_format,
                                      LTE_DL_FRAME_PARMS *frame_parms,
                                      uint16_t n3_pucch,
                                      uint16_t n3_pucch_array[NUMBER_OF_UE_MAX],
                                      uint8_t ncs_cell[20][7] )
{
    uint32_t        aa, symNo, k, slotNo, sym, i, j;
    int16_t         np, np_n, ip_ind;
    //int16_t         npucch_sf;
    int16_t         calctmp[2];
    int16_t         BsCshData[NB_ANTENNAS_RX][D_NSYM1SF][D_NSC1RB][2];
    //int16_t         delta_theta_calctmp[NB_ANTENNAS_RX][4][D_NSC1RB][2], delta_theta_comp[NB_ANTENNAS_RX][D_NSC1RB][2];
    int16_t         delta_theta_comp[NB_ANTENNAS_RX][D_NSC1RB][2];
    int16_t         CsData_allavg[NB_ANTENNAS_RX][14][2];
    int16_t         CsData_temp[NB_ANTENNAS_RX][D_NSYM1SF][D_NSC1RB][2];
    int32_t         IP_CsData_allsfavg[NB_ANTENNAS_RX][14][4][2];
    int32_t         IP_allavg[D_NPUCCH_SF5];
    //int16_t         temp_ch[2];
	int16_t         m[NUMBER_OF_UE_MAX], m_self=0, same_m_number;
	uint16_t        n3_pucch_sameRB[NUMBER_OF_UE_MAX];
	int16_t         n_oc0[NUMBER_OF_UE_MAX];
	int16_t         n_oc1[NUMBER_OF_UE_MAX];
	int16_t         np_n_array[2][NUMBER_OF_UE_MAX]; //Cyclic shift
	uint8_t N_PUCCH_SF0 = 5;
	uint8_t N_PUCCH_SF1 = (shortened_format==0)? 5:4;
    
    uint32_t u0 = (frame_parms->Nid_cell + frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.grouphop[subframe<<1]) % 30;
    uint32_t u1 = (frame_parms->Nid_cell + frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.grouphop[1+(subframe<<1)]) % 30;
    uint32_t v0=frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.seqhop[subframe<<1];
    uint32_t v1=frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.seqhop[1+(subframe<<1)];
    
    uint32_t u=u0;
    uint32_t v=v0;
    
    //double d_theta[32]={0.0};
    //int32_t temp_theta[32][2]={0};
    
    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        for (symNo=0; symNo<D_NSYM1SF; symNo++){
            for(ip_ind=0; ip_ind<D_NPUCCH_SF5-1; ip_ind++) {
                IP_CsData_allsfavg[aa][symNo][ip_ind][0] = 0;
                IP_CsData_allsfavg[aa][symNo][ip_ind][1] = 0;
            }
        }
    }
	
	// compute m[], m_self
	for(i=0; i<NUMBER_OF_UE_MAX; i++) {
		m[i] = n3_pucch_array[i] / N_PUCCH_SF0; // N_PUCCH_SF0 = 5
		if(n3_pucch_array[i] == n3_pucch) {
			m_self = i;
		}
	}
	
	for(i=0; i<NUMBER_OF_UE_MAX; i++) {
		//printf("n3_pucch_array[%d]=%d, m[%d]=%d \n", i, n3_pucch_array[i], i, m[i]);
	}
	//printf("m_self=%d \n", m_self);
	
	// compute n3_pucch_sameRB[] // Not 4 not be equally divided
	for(i=0, same_m_number=0; i<NUMBER_OF_UE_MAX; i++) {
		if(m[i] == m[m_self]) {
			n3_pucch_sameRB[same_m_number] = n3_pucch_array[i];
			same_m_number++;
		}
	}
	//printf("same_m_number = %d \n", same_m_number);
	for(i=0; i<same_m_number; i++) {
		//printf("n3_pucch_sameRB[%d]=%d \n", i, n3_pucch_sameRB[i]);
	}
	
	// compute n_oc1[], n_oc0[]
	for(i=0; i<same_m_number; i++) {
		n_oc0[i] = n3_pucch_sameRB[i] % N_PUCCH_SF1; //N_PUCCH_SF1 = (shortened_format==0)? 5:4;
		if (N_PUCCH_SF1 == 5) {
			n_oc1[i] = (3 * n_oc0[i]) % N_PUCCH_SF1;
		} else {
			n_oc1[i] = n_oc0[i] % N_PUCCH_SF1;
		}
	}
	for(i=0; i<same_m_number; i++) {
		//printf("n_oc0[%d]=%d, n_oc1[%d]=%d \n", i, n_oc0[i], i, n_oc1[i]);
	}
	
	
	// np_n_array[][]
	for(i=0; i<same_m_number; i++) {
		if (N_PUCCH_SF1 == 5) {
			np_n_array[0][i] = TBL_3_SF5_GEN_N_DASH_NS[n_oc0[i]]; //slot0
			np_n_array[1][i] = TBL_3_SF5_GEN_N_DASH_NS[n_oc1[i]]; //slot1
		} else {
			np_n_array[0][i] = TBL_3_SF4_GEN_N_DASH_NS[n_oc0[i]];
			np_n_array[1][i] = TBL_3_SF4_GEN_N_DASH_NS[n_oc1[i]];
		}
	}
	for(i=0; i<same_m_number; i++) {
		//printf("np_n_array[0][%d]=%d ,np_n_array[1][%d]=%d \n", i, np_n_array[0][i], i, np_n_array[1][i]);
	}
    
    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        for (symNo=0; symNo<D_NSYM1SF; symNo++){ // #define D_NSYM1SF       2*7
            slotNo = symNo / D_NSYM1SLT;
            sym = symNo % D_NSYM1SLT;
            
            for (k=0; k<D_NSC1RB; k++) { // #define D_NSC1RB        12
                
                // remove Base Sequence (c_r^*)*(r_l,m,m,n,k) = BsCshData
                BsCshData[aa][symNo][k][0] = (((int32_t)SubCarrierDeMapData[aa][symNo][k][0] * ul_ref_sigs[u][v][0][k<<1] + (int32_t)SubCarrierDeMapData[aa][symNo][k][1] * ul_ref_sigs[u][v][0][1+(k<<1)])>>15);
                BsCshData[aa][symNo][k][1] = (((int32_t)SubCarrierDeMapData[aa][symNo][k][1] * ul_ref_sigs[u][v][0][k<<1] - (int32_t)SubCarrierDeMapData[aa][symNo][k][0] * ul_ref_sigs[u][v][0][1+(k<<1)])>>15);
                
                if(shortened_format == 1) {
                    if (symNo < D_NSYM1SLT) { 
                        np = n3_pucch % D_NPUCCH_SF4;		// np = n_oc
                        np_n = TBL_3_SF4_GEN_N_DASH_NS[np]; //
                    } else {
                        np = n3_pucch % D_NPUCCH_SF4;		//
                        np_n = TBL_3_SF4_GEN_N_DASH_NS[np]; //
                    }
                    //npucch_sf = D_NPUCCH_SF4;// = 4
                } else {
                    if (symNo < D_NSYM1SLT) {
                        np = n3_pucch % D_NPUCCH_SF5;
                        np_n = TBL_3_SF5_GEN_N_DASH_NS[np];
                    } else {
                        np = (3 * n3_pucch) % D_NPUCCH_SF5;
                        np_n = TBL_3_SF5_GEN_N_DASH_NS[np];
                    }
                    //npucch_sf = D_NPUCCH_SF5;// = 5
                }
                // cyclic shift e^(-j * beta_n * k)
                calctmp[0] = alphaTBL_re[(((ncs_cell[2*subframe+slotNo][sym] + np_n)%D_NSC1RB)*k)%12];
                calctmp[1] = alphaTBL_im[(((ncs_cell[2*subframe+slotNo][sym] + np_n)%D_NSC1RB)*k)%12];
                
                // Channel Estimation 1A, g'(n_cs)_l,m,n
            	// CsData_temp = g_l,m,n,k
                // remove cyclic shift BsCshData * e^(-j * beta_n * k)
                CsData_temp[aa][symNo][k][0]=((((int32_t)BsCshData[aa][symNo][k][0] * calctmp[0] + (int32_t)BsCshData[aa][symNo][k][1] * calctmp[1])/ D_NSC1RB)>>15);
                CsData_temp[aa][symNo][k][1]=((((int32_t)BsCshData[aa][symNo][k][1] * calctmp[0] - (int32_t)BsCshData[aa][symNo][k][0] * calctmp[1])/ D_NSC1RB)>>15);
                
            	// Interference power for Channel Estimation 1A, No use Cyclic Shift g'(n_cs)_l,m,n
            	// Calculated by the cyclic shift that is not used  S(ncs)_est
            	ip_ind = 0;
	        	for(i=0; i<N_PUCCH_SF1; i++) {
					for(j=0; j<same_m_number; j++) {	//np_n_array Loop
						if(shortened_format == 1) {
							if(symNo < D_NSYM1SLT) { // if SF==1 slot0
								if(TBL_3_SF4_GEN_N_DASH_NS[i] == np_n_array[0][j]) {
									break;
								}
            				} else { // if SF==1 slot1
								if(TBL_3_SF4_GEN_N_DASH_NS[i] == np_n_array[1][j]) {
									break;
								}
            				}
						} else {
							if(symNo < D_NSYM1SLT) { // if SF==0 slot0
								if(TBL_3_SF5_GEN_N_DASH_NS[i] == np_n_array[0][j]) {
									break;
								}
            				} else { // if SF==0 slot1
								if(TBL_3_SF5_GEN_N_DASH_NS[i] == np_n_array[1][j]) {
									break;
								}
            				}
						}
						if(j == same_m_number - 1) { //when even once it has not been used
							if(shortened_format == 1) {
								calctmp[0] = alphaTBL_re[(((ncs_cell[2*subframe+slotNo][sym] + TBL_3_SF4_GEN_N_DASH_NS[i])%D_NSC1RB)*k)%12]; //D_NSC1RB =12
		                        calctmp[1] = alphaTBL_im[(((ncs_cell[2*subframe+slotNo][sym] + TBL_3_SF4_GEN_N_DASH_NS[i])%D_NSC1RB)*k)%12];
							} else {
								calctmp[0] = alphaTBL_re[(((ncs_cell[2*subframe+slotNo][sym] + TBL_3_SF5_GEN_N_DASH_NS[i])%D_NSC1RB)*k)%12];
		                        calctmp[1] = alphaTBL_im[(((ncs_cell[2*subframe+slotNo][sym] + TBL_3_SF5_GEN_N_DASH_NS[i])%D_NSC1RB)*k)%12];
							}
							// IP_CsData_allsfavg = g'(n_cs)_l,m,n
	                        IP_CsData_allsfavg[aa][symNo][ip_ind][0] += ((((int32_t)BsCshData[aa][symNo][k][0] * calctmp[0] + (int32_t)BsCshData[aa][symNo][k][1] * calctmp[1]))>>15);
	                        IP_CsData_allsfavg[aa][symNo][ip_ind][1] += ((((int32_t)BsCshData[aa][symNo][k][1] * calctmp[0] - (int32_t)BsCshData[aa][symNo][k][0] * calctmp[1]))>>15);
							if((symNo == 1 || symNo == 5 || symNo == 8 || symNo == 12)) {
							}
	                        ip_ind++;
		        		}
					}
	        	}
            }
            if(symNo > D_NSYM1SLT-1) {
                u=u1;
                v=v1;
            }
        }
    }
    
    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        for (symNo=0; symNo<D_NSYM1SF; symNo++){
            CsData_allavg[aa][symNo][0] = 0;
            CsData_allavg[aa][symNo][1] = 0;
            for (k=0; k<D_NSC1RB; k++) {
              CsData_allavg[aa][symNo][0] += (int16_t)((double)CsData_temp[aa][symNo][k][0]);
              CsData_allavg[aa][symNo][1] += (int16_t)((double)CsData_temp[aa][symNo][k][1]);
            }
        }
    }
    
    // Frequency deviation estimation
    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        for (k=0; k<12; k++) {
            delta_theta_comp[aa][k][0] = 0;
            delta_theta_comp[aa][k][1] = 0;
            
            delta_theta_comp[aa][k][0] += (((int32_t)CsData_temp[aa][1][k][0] * CsData_temp[aa][5][k][0] + (int32_t)((CsData_temp[aa][1][k][1])*CsData_temp[aa][5][k][1]))>>8);
            delta_theta_comp[aa][k][1] += (((int32_t)CsData_temp[aa][1][k][0]*CsData_temp[aa][5][k][1] - (int32_t)((CsData_temp[aa][1][k][1])*CsData_temp[aa][5][k][0]) )>>8);
            
            delta_theta_comp[aa][k][0] += (((int32_t)CsData_temp[aa][8][k][0] * CsData_temp[aa][12][k][0] + (int32_t)((CsData_temp[aa][8][k][1])*CsData_temp[aa][12][k][1]))>>8);
            delta_theta_comp[aa][k][1] += (((int32_t)CsData_temp[aa][8][k][0]*CsData_temp[aa][12][k][1] - (int32_t)((CsData_temp[aa][8][k][1])*CsData_temp[aa][12][k][0]))>>8);
            
            delta_theta[aa][k] = atan2((double)delta_theta_comp[aa][k][1], (double)delta_theta_comp[aa][k][0]) / 4.0;
        }
    }
    
    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        for (k=0; k<D_NSC1RB; k++) {
            ChestValue[aa][0][k][0] = (int16_t)((CsData_allavg[aa][1][0] + (int16_t)(((double)CsData_allavg[aa][5][0] * cos(delta_theta[aa][k]*4)) + ((double)CsData_allavg[aa][5][1] * sin(delta_theta[aa][k]*4)))) /(2*D_NSC1RB)) ;
            ChestValue[aa][0][k][1] = (int16_t)((CsData_allavg[aa][1][1] + (int16_t)(((double)CsData_allavg[aa][5][1] * cos(delta_theta[aa][k]*4)) - ((double)CsData_allavg[aa][5][0] * sin(delta_theta[aa][k]*4)))) /(2*D_NSC1RB)) ;
            ChestValue[aa][1][k][0] = (int16_t)((CsData_allavg[aa][8][0] + (int16_t)(((double)CsData_allavg[aa][12][0] * cos(delta_theta[aa][k]*4)) + ((double)CsData_allavg[aa][12][1] * sin(delta_theta[aa][k]*4)))) /(2*D_NSC1RB)) ;
            ChestValue[aa][1][k][1] = (int16_t)((CsData_allavg[aa][8][1] + (int16_t)(((double)CsData_allavg[aa][12][1] * cos(delta_theta[aa][k]*4)) - ((double)CsData_allavg[aa][12][0] * sin(delta_theta[aa][k]*4)))) /(2*D_NSC1RB)) ;
        }
    }
    
    *Interpw = 0;
    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
    	if(ip_ind == 0) {//ip_ind= The total number of cyclic shift of non-use
    		*Interpw = 1;
    		break;
    	}
        for(i=0; i<ip_ind; i++) {
            IP_allavg[i] = 0;
            
            IP_allavg[i] += (((int32_t)IP_CsData_allsfavg[aa][1][i][0] * IP_CsData_allsfavg[aa][1][i][0] + (int32_t)IP_CsData_allsfavg[aa][1][i][1]*IP_CsData_allsfavg[aa][1][i][1])>>8);
        	IP_allavg[i] += (((int32_t)IP_CsData_allsfavg[aa][5][i][0] * IP_CsData_allsfavg[aa][5][i][0] + (int32_t)IP_CsData_allsfavg[aa][5][i][1]*IP_CsData_allsfavg[aa][5][i][1])>>8);
            IP_allavg[i] += (((int32_t)IP_CsData_allsfavg[aa][8][i][0] * IP_CsData_allsfavg[aa][8][i][0] + (int32_t)IP_CsData_allsfavg[aa][8][i][1]*IP_CsData_allsfavg[aa][8][i][1])>>8);
            IP_allavg[i] += (((int32_t)IP_CsData_allsfavg[aa][12][i][0] * IP_CsData_allsfavg[aa][12][i][0] + (int32_t)IP_CsData_allsfavg[aa][12][i][1]*IP_CsData_allsfavg[aa][12][i][1])>>8);
        	*Interpw += IP_allavg[i]/(2*D_NSLT1SF*frame_parms->nb_antennas_rx*ip_ind*12);
        }
    }
    return 0;
}

/* Channel Equalization */
uint16_t pucchfmt3_Equalization( int16_t CshData_fmt3[NB_ANTENNAS_RX][14][12][2],
                                 int16_t ChdetAfterValue_fmt3[NB_ANTENNAS_RX][14][12][2],
                                 int16_t ChestValue[NB_ANTENNAS_RX][2][12][2],
                                 LTE_DL_FRAME_PARMS *frame_parms)
{
    int16_t aa, sltNo, symNo, k;
    
    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        sltNo = 0;
        for (symNo=0; symNo<D_NSYM1SF; symNo++){
            if(symNo >= D_NSYM1SLT) {
                sltNo = 1;
            }
            for (k=0; k<D_NSC1RB; k++){
                ChdetAfterValue_fmt3[aa][symNo][k][0] = (((int32_t)CshData_fmt3[aa][symNo][k][0] * ChestValue[aa][sltNo][k][0] + (int32_t)CshData_fmt3[aa][symNo][k][1] * ChestValue[aa][sltNo][k][1])>>8);
                ChdetAfterValue_fmt3[aa][symNo][k][1] = (((int32_t)CshData_fmt3[aa][symNo][k][1] * ChestValue[aa][sltNo][k][0] - (int32_t)CshData_fmt3[aa][symNo][k][0] * ChestValue[aa][sltNo][k][1])>>8);
            }
        }
    }
    return 0;
}

/* Frequency deviation remove AFC */
uint16_t pucchfmt3_FrqDevRemove( int16_t ChdetAfterValue_fmt3[NB_ANTENNAS_RX][14][12][2],
                             double delta_theta[NB_ANTENNAS_RX][12],
                             int16_t RemoveFrqDev_fmt3[NB_ANTENNAS_RX][2][5][12][2],
                             LTE_DL_FRAME_PARMS *frame_parms )
{
    int16_t aa, sltNo, symNo1slt, k, n;
    double calctmp[2];
    
    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        for(sltNo = 0; sltNo<D_NSLT1SF; sltNo++)
        {
            n=0;
            for (symNo1slt=0, n=0; symNo1slt<D_NSYM1SLT; symNo1slt++){
                if(!((symNo1slt==1) || (symNo1slt==5))) {
                    for (k=0; k<D_NSC1RB; k++) {
                        calctmp[0] = cos(delta_theta[aa][k] * (n-1));
                        calctmp[1] = sin(delta_theta[aa][k] * (n-1));
                        
                        RemoveFrqDev_fmt3[aa][sltNo][n][k][0] = (int16_t)((double)ChdetAfterValue_fmt3[aa][(sltNo*D_NSYM1SLT)+symNo1slt][k][0] * calctmp[0] 
                                                                + (double)ChdetAfterValue_fmt3[aa][(sltNo*D_NSYM1SLT)+symNo1slt][k][1] * calctmp[1]);
                        RemoveFrqDev_fmt3[aa][sltNo][n][k][1] = (int16_t)((double)ChdetAfterValue_fmt3[aa][(sltNo*D_NSYM1SLT)+symNo1slt][k][1] * calctmp[0] 
                                                                - (double)ChdetAfterValue_fmt3[aa][(sltNo*D_NSYM1SLT)+symNo1slt][k][0] * calctmp[1]);
                    }
                    n++;
                }
            }
        }
    }
    return 0;
}

//for opt.Lev.2 
#define  MAXROW_TBL_SF5  5
#define  MAXCLM_TBL_SF5  5
const int16_t TBL_3_SF5[MAXROW_TBL_SF5][MAXCLM_TBL_SF5][2] = 
                         {{ {32767,0}, {32767,0}, {32767,0}, {32767,0}, {32767,0}},
                          { {32767,0}, {10126, 31163}, {-26509, 19260}, {-26509, -19260}, {10126, -31163}},
                          { {32767,0}, {-26509, 19260}, {10126, -31163}, {10126, 31163}, {-26509, -19260}},
                          { {32767,0}, {-26509, -19260}, {10126, 31163}, {10126, -31163}, {-26509, 19260}},
                          { {32767,0}, {10126, -31163}, {-26509, -19260}, {-26509, 19260}, {10126, 31163}}};

#define  MAXROW_TBL_SF4_fmt3 4
#define  MAXCLM_TBL_SF4      4
const int16_t TBL_3_SF4[MAXROW_TBL_SF4_fmt3][MAXCLM_TBL_SF4][2] = 
                         {{ {32767,0}, {32767,0}, {32767,0}, {32767,0}},
                          { {32767,0}, {-32767,0}, {32767,0}, {-32767,0}},
                          { {32767,0}, {32767,0}, {-32767,0}, {-32767,0}},
                          { {32767,0}, {-32767,0}, {-32767,0}, {32767,0}}};

/* orthogonal sequence remove */
uint16_t pucchfmt3_OrthSeqRemove( int16_t RemoveFrqDev_fmt3[NB_ANTENNAS_RX][2][5][12][2],
                                  int16_t Fmt3xDataRmvOrth[NB_ANTENNAS_RX][2][5][12][2],
                                  uint8_t shortened_format,
                                  uint16_t n3_pucch,
                                  LTE_DL_FRAME_PARMS *frame_parms )
{
    int16_t aa, sltNo, n, k;
    int16_t Npucch_sf;
    int16_t noc;
    
    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        for (sltNo=0; sltNo<D_NSLT1SF; sltNo++){
            if(shortened_format == 1) {
                if(sltNo == 0) {
                    noc = n3_pucch % D_NPUCCH_SF4;
                    Npucch_sf = D_NPUCCH_SF5;
                } else {
                    noc = n3_pucch % D_NPUCCH_SF4;
                    Npucch_sf = D_NPUCCH_SF4;
                }
            } else {
                if(sltNo == 0) {
                    noc = n3_pucch % D_NPUCCH_SF5;
                    Npucch_sf = D_NPUCCH_SF5;
                } else {
                    noc = (3 * n3_pucch) % D_NPUCCH_SF5;
                    Npucch_sf = D_NPUCCH_SF5;
                }
            }
            for (n=0; n<Npucch_sf; n++){
                for (k=0; k<D_NSC1RB; k++) {
                    if ((sltNo == 1) && (shortened_format == 1)) {
                      Fmt3xDataRmvOrth[aa][sltNo][n][k][0] = (((int32_t)RemoveFrqDev_fmt3[aa][sltNo][n][k][0] * TBL_3_SF4[noc][n][0] + (int32_t)RemoveFrqDev_fmt3[aa][sltNo][n][k][1] * TBL_3_SF4[noc][n][1])>>15);
                      Fmt3xDataRmvOrth[aa][sltNo][n][k][1] = (((int32_t)RemoveFrqDev_fmt3[aa][sltNo][n][k][1] * TBL_3_SF4[noc][n][0] - (int32_t)RemoveFrqDev_fmt3[aa][sltNo][n][k][0] * TBL_3_SF4[noc][n][1])>>15);
                    } else {
                      Fmt3xDataRmvOrth[aa][sltNo][n][k][0] = (((int32_t)RemoveFrqDev_fmt3[aa][sltNo][n][k][0] * TBL_3_SF5[noc][n][0] + (int32_t)RemoveFrqDev_fmt3[aa][sltNo][n][k][1] * TBL_3_SF5[noc][n][1])>>15);
                      Fmt3xDataRmvOrth[aa][sltNo][n][k][1] = (((int32_t)RemoveFrqDev_fmt3[aa][sltNo][n][k][1] * TBL_3_SF5[noc][n][0] - (int32_t)RemoveFrqDev_fmt3[aa][sltNo][n][k][0] * TBL_3_SF5[noc][n][1])>>15);
                    }
                }
            }
        }
    }
    return 0;
}

/* averaging antenna */
uint16_t pucchfmt3_AvgAnt( int16_t Fmt3xDataRmvOrth[NB_ANTENNAS_RX][2][5][12][2],
                           int16_t Fmt3xDataAvgAnt[2][5][12][2],
                           uint8_t shortened_format,
                           LTE_DL_FRAME_PARMS *frame_parms )
{
    int16_t aa, sltNo, n, k;
    int16_t Npucch_sf;
    
    for (sltNo=0; sltNo<D_NSLT1SF; sltNo++){
        if((sltNo == 1) && (shortened_format == 1)) {
            Npucch_sf = D_NPUCCH_SF4;
        } else {
            Npucch_sf = D_NPUCCH_SF5;
        }
        for (n=0; n<Npucch_sf; n++){
            for (k=0; k<D_NSC1RB; k++) {
                Fmt3xDataAvgAnt[sltNo][n][k][0] = 0;
                Fmt3xDataAvgAnt[sltNo][n][k][1] = 0;
                for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
                    Fmt3xDataAvgAnt[sltNo][n][k][0] += Fmt3xDataRmvOrth[aa][sltNo][n][k][0]  / frame_parms->nb_antennas_rx;
                    Fmt3xDataAvgAnt[sltNo][n][k][1] += Fmt3xDataRmvOrth[aa][sltNo][n][k][1]  / frame_parms->nb_antennas_rx;
                }
            }
        }
    }
    return 0;
}

/* averaging symbol */
uint16_t pucchfmt3_AvgSym( int16_t Fmt3xDataAvgAnt[2][5][12][2],
                           int16_t Fmt3xDataAvgSym[2][12][2],
                           uint8_t shortened_format )
{
    int16_t sltNo, n, k;
    int16_t Npucch_sf;
    
    for (sltNo=0; sltNo<D_NSLT1SF; sltNo++){
        if((sltNo == 1) && (shortened_format == 1)) {
            Npucch_sf = D_NPUCCH_SF4;
        } else {
            Npucch_sf = D_NPUCCH_SF5;
        }
        for (k=0; k<D_NSC1RB; k++) {
            Fmt3xDataAvgSym[sltNo][k][0] = 0;
            Fmt3xDataAvgSym[sltNo][k][1] = 0;
            for (n=0; n<Npucch_sf; n++){
                Fmt3xDataAvgSym[sltNo][k][0] += Fmt3xDataAvgAnt[sltNo][n][k][0] / Npucch_sf;
                Fmt3xDataAvgSym[sltNo][k][1] += Fmt3xDataAvgAnt[sltNo][n][k][1] / Npucch_sf;
            }
        }
    }
    return 0;
}

/* iDFT */
void pucchfmt3_IDft2( int16_t *x, int16_t *y )
{
  int16_t i, k;
  int16_t tmp[2];
  int16_t calctmp[D_NSC1RB*2]={0};
  
  for(k=0; k<D_NSC1RB; k++) {
    for (i=0; i<D_NSC1RB; i++) {
      tmp[0] = alphaTBL_re[((i*k)%12)];
      tmp[1] = alphaTBL_im[((i*k)%12)];
      
      calctmp[2*k] += (((int32_t)x[2*i] * tmp[0] - (int32_t)x[2*i+1] * tmp[1])>>15);
      calctmp[2*k+1] += (((int32_t)x[2*i+1] * tmp[0] + (int32_t)x[2*i] * tmp[1])>>15);
    }
    y[2*k] = (int16_t)( (double) calctmp[2*k] / sqrt(D_NSC1RB));
    y[2*k+1] = (int16_t)((double) calctmp[2*k+1] / sqrt(D_NSC1RB));
  }
}

/* descramble */
uint16_t pucchfmt3_Descramble( int16_t IFFTOutData_Fmt3[2][12][2],
                               int16_t b[48],
                               uint8_t subframe,
                               uint32_t Nid_cell,
                               uint32_t rnti
                              )
{
    int16_t m, k, c,i,j;
    uint32_t cinit = 0;
    uint32_t x1;
    uint32_t s,s0,s1;
    cinit = (subframe + 1) * ((2 * Nid_cell + 1)<<16) + rnti;
    s0 = lte_gold_generic(&x1,&cinit,1);
    s1 = lte_gold_generic(&x1,&cinit,0);
    i=0;
    for (m=0; m<D_NSLT1SF; m++){
        for(k=0; k<D_NSC1RB; k++) {
            s = (i<32)? s0:s1;
            j = (i<32)? i:(i-32);
            c=((s>>j)&1);
            b[i] = (IFFTOutData_Fmt3[m][k][0] * (1 - 2*c));
            i++;
          
            s = (i<32)? s0:s1;
            j = (i<32)? i:(i-32);
            c=((s>>j)&1);
            b[i] = (IFFTOutData_Fmt3[m][k][1] * (1 - 2*c));
            i++;
        }
    }
    return 0;
}

int16_t pucchfmt3_Decode( int16_t b[48],
                          uint8_t subframe,
                          int16_t DTXthreshold,
                          int16_t Interpw,
                          uint8_t do_sr)
{
    int16_t c, i;
    int32_t Rho_tmp;
    int16_t c_max;
    int32_t Rho_max;
    int16_t bit_pattern;
    
    /* Is payload 6bit or 7bit? */
    if( do_sr == 1 ) {
        bit_pattern = 128;
    } else {
        bit_pattern = 64;
    }
    
    c=0;
    Rho_tmp = 0;
    for (i=0;i<48;i++) {
        Rho_tmp += b[i] * (1-2*chcod_tbl[c][i]);
    }
    c_max = c;
    Rho_max = Rho_tmp;
    
    for(c=1; c<bit_pattern; c++) {
        Rho_tmp = 0;
        for (i=0;i<48;i++) {
            Rho_tmp += b[i] * (1-2*chcod_tbl[c][i]);
        }
        if (Rho_tmp > Rho_max) {
            c_max = c;
            Rho_max = Rho_tmp;
        }
    }
    if(Interpw<1){
      Interpw=1;
    }
    if((Rho_max/Interpw) > DTXthreshold) {
        // ***Log
        return c_max;
    } else {
        // ***Log
        return -1;
    }
}

/* PUCCH format3 << */

uint32_t rx_pucch(PHY_VARS_eNB *eNB,
		  PUCCH_FMT_t fmt,
		  uint8_t UE_id,
		  uint16_t n1_pucch,
		  uint16_t n2_pucch,
		  uint8_t shortened_format,
		  uint8_t *payload,
		  int     frame,
		  uint8_t subframe,
		  uint8_t pucch1_thres
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0)) 
		  ,uint8_t br_flag
#endif
)
//-----------------------------------------------------------------------------
{
  static int first_call = 1;
  LTE_eNB_COMMON *common_vars = &eNB->common_vars;
  LTE_DL_FRAME_PARMS *frame_parms = &eNB->frame_parms;
  int8_t sigma2_dB = max(eNB->measurements.n0_subband_power_tot_dB[0], eNB->measurements.n0_subband_power_tot_dB[eNB->frame_parms.N_RB_UL-1]);

  uint32_t u,v,n,aa;
  uint32_t z[12*14];
  int16_t *zptr;
  int16_t rxcomp[NB_ANTENNAS_RX][2*12*14];
  uint8_t ns,N_UL_symb,nsymb,n_oc,n_oc0,n_oc1;
  uint8_t c = (frame_parms->Ncp==0) ? 3 : 2;
  uint16_t nprime,nprime0,nprime1;
  uint16_t i,j,re_offset,thres,h,off;
  uint8_t Nprime_div_deltaPUCCH_Shift,Nprime,d;
  uint8_t m,l,refs,phase,re,l2,phase_max=0;
  uint8_t n_cs,S,alpha_ind,rem;
  int16_t tmp_re,tmp_im,W_re=0,W_im=0;
  int16_t *rxptr;
  uint32_t symbol_offset;
  int16_t stat_ref_re,stat_ref_im,*cfo;
  int16_t chest0_re[NB_ANTENNAS_RX][12],chest0_im[NB_ANTENNAS_RX][12];
  int16_t chest1_re[NB_ANTENNAS_RX][12],chest1_im[NB_ANTENNAS_RX][12];
  int32_t chest_mag;
  int32_t stat_re=0,stat_im=0;
  uint32_t stat,stat_max=0;
  uint8_t log2_maxh;

  uint8_t deltaPUCCH_Shift          = frame_parms->pucch_config_common.deltaPUCCH_Shift;
  uint8_t NRB2                      = frame_parms->pucch_config_common.nRB_CQI;
  uint8_t Ncs1_div_deltaPUCCH_Shift = frame_parms->pucch_config_common.nCS_AN;

  uint32_t u0 = (frame_parms->Nid_cell + frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.grouphop[subframe<<1]) % 30;
  uint32_t u1 = (frame_parms->Nid_cell + frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.grouphop[1+(subframe<<1)]) % 30;
  uint32_t v0=frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.seqhop[subframe<<1];
  uint32_t v1=frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.seqhop[1+(subframe<<1)];
  int chL;

  /* PUCCH format3 >> */
  uint16_t Ret = 0;
  int16_t SubCarrierDeMapData[NB_ANTENNAS_RX][14][12][2];       //[Antenna][Symbol][Subcarrier][Complex]
  int16_t CshData_fmt3[NB_ANTENNAS_RX][14][12][2];              //[Antenna][Symbol][Subcarrier][Complex]
  double delta_theta[NB_ANTENNAS_RX][12];                       //[Antenna][Subcarrier][Complex]
  int16_t ChestValue[NB_ANTENNAS_RX][2][12][2];                 //[Antenna][Slot][Subcarrier][Complex]
  int16_t ChdetAfterValue_fmt3[NB_ANTENNAS_RX][14][12][2];      //[Antenna][Symbol][Subcarrier][Complex]
  int16_t RemoveFrqDev_fmt3[NB_ANTENNAS_RX][2][5][12][2];       //[Antenna][Slot][PUCCH_Symbol][Subcarrier][Complex]
  int16_t Fmt3xDataRmvOrth[NB_ANTENNAS_RX][2][5][12][2];        //[Antenna][Slot][PUCCH_Symbol][Subcarrier][Complex]
  int16_t Fmt3xDataAvgAnt[2][5][12][2];                         //[Slot][PUCCH_Symbol][Subcarrier][Complex]
  int16_t Fmt3xDataAvgSym[2][12][2];                            //[Slot][Subcarrier][Complex]
  int16_t IFFTOutData_Fmt3[2][12][2];                           //[Slot][Subcarrier][Complex]
  int16_t b[48];                                                //[bit]
  int16_t payload_entity = -1;
  int16_t Interpw;
  int16_t payload_max;
  
  // TODO
  // When using PUCCH format3, it must be an argument of rx_pucch function
  uint16_t n3_pucch = 20;
  uint16_t n3_pucch_array[NUMBER_OF_UE_MAX]={1};
  n3_pucch_array[0]=n3_pucch;
  uint8_t do_sr = 1;
  uint16_t crnti=0x1234;
  int16_t DTXthreshold = 10;
  /* PUCCH format3 << */

  if (first_call == 1) {
    for (i=0;i<10;i++) {
      for (j=0;j<NUMBER_OF_UE_MAX;j++) {
	eNB->pucch1_stats_cnt[j][i]=0;
	eNB->pucch1ab_stats_cnt[j][i]=0;
#if defined(USRP_REC_PLAY) // not 100% sure
	eNB->pucch1_stats_thres[j][i]=0;
#endif	
      }
    }
    first_call=0;
  }


  if(fmt!=pucch_format3) {  /* PUCCH format3 */
  
  // TODO
  // "SR+ACK/NACK" length is only 7 bits.
  // This restriction will be lifted in the future.
  // "CQI/PMI/RI+ACK/NACK" will be supported in the future.

  if ((deltaPUCCH_Shift==0) || (deltaPUCCH_Shift>3)) {
    LOG_E(PHY,"[eNB] rx_pucch: Illegal deltaPUCCH_shift %d (should be 1,2,3)\n",deltaPUCCH_Shift);
    return(-1);
  }

  if (Ncs1_div_deltaPUCCH_Shift > 7) {
    LOG_E(PHY,"[eNB] rx_pucch: Illegal Ncs1_div_deltaPUCCH_Shift %d (should be 0...7)\n",Ncs1_div_deltaPUCCH_Shift);
    return(-1);
  }

  zptr = (int16_t *)z;
  thres = (c*Ncs1_div_deltaPUCCH_Shift);
  Nprime_div_deltaPUCCH_Shift = (n1_pucch < thres) ? Ncs1_div_deltaPUCCH_Shift : (12/deltaPUCCH_Shift);
  Nprime = Nprime_div_deltaPUCCH_Shift * deltaPUCCH_Shift;

#ifdef DEBUG_PUCCH_RX
  printf("[eNB] PUCCH: cNcs1/deltaPUCCH_Shift %d, Nprime %d, n1_pucch %d\n",thres,Nprime,n1_pucch);
#endif

  N_UL_symb = (frame_parms->Ncp==NORMAL) ? 7 : 6;

  if (n1_pucch < thres)
    nprime0=n1_pucch;
  else
    nprime0 = (n1_pucch - thres)%(12*c/deltaPUCCH_Shift);

  if (n1_pucch >= thres)
    nprime1= ((c*(nprime0+1))%((12*c/deltaPUCCH_Shift)+1))-1;
  else {
    d = (frame_parms->Ncp==0) ? 2 : 0;
    h= (nprime0+d)%(c*Nprime_div_deltaPUCCH_Shift);
    nprime1 = (h/c) + (h%c)*Nprime_div_deltaPUCCH_Shift;
  }

#ifdef DEBUG_PUCCH_RX
  printf("PUCCH: nprime0 %d nprime1 %d\n",nprime0,nprime1);
#endif

  n_oc0 = nprime0/Nprime_div_deltaPUCCH_Shift;

  if (frame_parms->Ncp==1)
    n_oc0<<=1;

  n_oc1 = nprime1/Nprime_div_deltaPUCCH_Shift;

  if (frame_parms->Ncp==1)  // extended CP
    n_oc1<<=1;

#ifdef DEBUG_PUCCH_RX
  printf("[eNB] PUCCH: noc0 %d noc11 %d\n",n_oc0,n_oc1);
#endif

  nprime=nprime0;
  n_oc  =n_oc0;

  // loop over 2 slots
  for (ns=(subframe<<1),u=u0,v=v0; ns<(2+(subframe<<1)); ns++,u=u1,v=v1) {

    if ((nprime&1) == 0)
      S=0;  // 1
    else
      S=1;  // j
    /*
    if (fmt==pucch_format1)
      LOG_I(PHY,"[eNB] subframe %d => PUCCH1: u%d %d, v%d %d : ", subframe,ns&1,u,ns&1,v);
    else
      LOG_I(PHY,"[eNB] subframe %d => PUCCH1a/b: u%d %d, v%d %d : ", subframe,ns&1,u,ns&1,v);
    */

    //loop over symbols in slot
    for (l=0; l<N_UL_symb; l++) {
      // Compute n_cs (36.211 p. 18)
      n_cs = eNB->ncs_cell[ns][l];

      if (frame_parms->Ncp==0) { // normal CP
        n_cs = ((uint16_t)n_cs + (nprime*deltaPUCCH_Shift + (n_oc%deltaPUCCH_Shift))%Nprime)%12;
      } else {
        n_cs = ((uint16_t)n_cs + (nprime*deltaPUCCH_Shift + (n_oc>>1))%Nprime)%12;
      }



      refs=0;

      // Comput W_noc(m) (36.211 p. 19)
      if ((ns==(1+(subframe<<1))) && (shortened_format==1)) {  // second slot and shortened format

        if (l<2) {                                         // data
          W_re=W3_re[n_oc][l];
          W_im=W3_im[n_oc][l];
        } else if ((l<N_UL_symb-2)&&(frame_parms->Ncp==0)) { // reference and normal CP
          W_re=W3_re[n_oc][l-2];
          W_im=W3_im[n_oc][l-2];
          refs=1;
        } else if ((l<N_UL_symb-2)&&(frame_parms->Ncp==1)) { // reference and extended CP
          W_re=W4[n_oc][l-2];
          W_im=0;
          refs=1;
        } else if ((l>=N_UL_symb-2)) {                      // data
          W_re=W3_re[n_oc][l-N_UL_symb+4];
          W_im=W3_im[n_oc][l-N_UL_symb+4];
        }
      } else {
        if (l<2) {                                         // data
          W_re=W4[n_oc][l];
          W_im=0;
        } else if ((l<N_UL_symb-2)&&(frame_parms->Ncp==NORMAL)) { // reference and normal CP
          W_re=W3_re[n_oc][l-2];
          W_im=W3_im[n_oc][l-2];
          refs=1;
        } else if ((l<N_UL_symb-2)&&(frame_parms->Ncp==EXTENDED)) { // reference and extended CP
          W_re=W4[n_oc][l-2];
          W_im=0;
          refs=1;
        } else if ((l>=N_UL_symb-2)) {                     // data
          W_re=W4[n_oc][l-N_UL_symb+4];
          W_im=0;
        }
      }

      // multiply W by S(ns) (36.211 p.17). only for data, reference symbols do not have this factor
      if ((S==1)&&(refs==0)) {
        tmp_re = W_re;
        W_re = -W_im;
        W_im = tmp_re;
      }

#ifdef DEBUG_PUCCH_RX
      printf("[eNB] PUCCH: ncs[%d][%d]=%d, W_re %d, W_im %d, S %d, refs %d\n",ns,l,n_cs,W_re,W_im,S,refs);
#endif
      alpha_ind=0;
      // compute output sequence

      for (n=0; n<12; n++) {

        // this is r_uv^alpha(n)
        tmp_re = (int16_t)(((int32_t)alpha_re[alpha_ind] * ul_ref_sigs[u][v][0][n<<1] - (int32_t)alpha_im[alpha_ind] * ul_ref_sigs[u][v][0][1+(n<<1)])>>15);
        tmp_im = (int16_t)(((int32_t)alpha_re[alpha_ind] * ul_ref_sigs[u][v][0][1+(n<<1)] + (int32_t)alpha_im[alpha_ind] * ul_ref_sigs[u][v][0][n<<1])>>15);

        // this is S(ns)*w_noc(m)*r_uv^alpha(n)
        zptr[n<<1] = (tmp_re*W_re - tmp_im*W_im)>>15;
        zptr[1+(n<<1)] = -(tmp_re*W_im + tmp_im*W_re)>>15;

#ifdef DEBUG_PUCCH_RX
        printf("[eNB] PUCCH subframe %d z(%d,%d) => %d,%d, alpha(%d) => %d,%d\n",subframe,l,n,zptr[n<<1],zptr[(n<<1)+1],
              alpha_ind,alpha_re[alpha_ind],alpha_im[alpha_ind]);
#endif

        alpha_ind = (alpha_ind + n_cs)%12;
      } // n

      zptr+=24;
    } // l

    nprime=nprime1;
    n_oc  =n_oc1;
  } // ns

  rem = ((((deltaPUCCH_Shift*Ncs1_div_deltaPUCCH_Shift)>>3)&7)>0) ? 1 : 0;

  m = (n1_pucch < thres) ? NRB2 : (((n1_pucch-thres)/(12*c/deltaPUCCH_Shift))+NRB2+((deltaPUCCH_Shift*Ncs1_div_deltaPUCCH_Shift)>>3)+rem);

#ifdef DEBUG_PUCCH_RX
  printf("[eNB] PUCCH: m %d, thres %d, NRB2 %d\n",m,thres,NRB2);
#endif
  nsymb = N_UL_symb<<1;

  zptr = (int16_t*)z;

  // Do detection
  for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {

    //for (j=0,l=0;l<(nsymb-1);l++) {
    for (j=0,l=0; l<nsymb; l++) {
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
      if (br_flag > 0 ) {
        if ((m&1) == 0)
          re_offset = (m*6) + frame_parms->first_carrier_offset;
	else
	  re_offset = frame_parms->first_carrier_offset + (frame_parms->N_RB_DL - (m>>1) - 1)*12;
      }
      else
#endif
      {
        if ((l<(nsymb>>1)) && ((m&1) == 0))
          re_offset = (m*6) + frame_parms->first_carrier_offset;
        else if ((l<(nsymb>>1)) && ((m&1) == 1))
          re_offset = frame_parms->first_carrier_offset + (frame_parms->N_RB_DL - (m>>1) - 1)*12;
        else if ((m&1) == 0)
          re_offset = frame_parms->first_carrier_offset + (frame_parms->N_RB_DL - (m>>1) - 1)*12;
        else
          re_offset = ((m-1)*6) + frame_parms->first_carrier_offset;
      }

      if (re_offset > frame_parms->ofdm_symbol_size)
        re_offset -= (frame_parms->ofdm_symbol_size);

      symbol_offset = (unsigned int)frame_parms->ofdm_symbol_size*l;
      rxptr = (int16_t *)&common_vars->rxdataF[aa][symbol_offset];

      for (i=0; i<12; i++,j+=2,re_offset++) {
        if (re_offset==frame_parms->ofdm_symbol_size)
          re_offset = 0;

        rxcomp[aa][j]   = (int16_t)((rxptr[re_offset<<1]*(int32_t)zptr[j])>>15)   - ((rxptr[1+(re_offset<<1)]*(int32_t)zptr[1+j])>>15);
        rxcomp[aa][1+j] = (int16_t)((rxptr[re_offset<<1]*(int32_t)zptr[1+j])>>15) + ((rxptr[1+(re_offset<<1)]*(int32_t)zptr[j])>>15);

#ifdef DEBUG_PUCCH_RX
        printf("[eNB] PUCCH subframe %d (%d,%d,%d,%d,%d) => (%d,%d) x (%d,%d) : (%d,%d)\n",subframe,l,i,re_offset,m,j,
              rxptr[re_offset<<1],rxptr[1+(re_offset<<1)],
              zptr[j],zptr[1+j],
              rxcomp[aa][j],rxcomp[aa][1+j]);
#endif
      } //re
    } // symbol
  }  // antenna


  // PUCCH Format 1
  // Do cfo correction and MRC across symbols

  if (fmt == pucch_format1) {
#ifdef DEBUG_PUCCH_RX
    printf("Doing PUCCH detection for format 1\n");
#endif

    stat_max = 0;


    for (phase=0; phase<7; phase++) {
      stat=0;

      for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        for (re=0; re<12; re++) {
          stat_re=0;
          stat_im=0;
          off=re<<1;
          cfo =  (frame_parms->Ncp==0) ? &cfo_pucch_np[14*phase] : &cfo_pucch_ep[12*phase];

          for (l=0; l<(nsymb>>1); l++) {
            stat_re += (((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15))/nsymb;
            stat_im += (((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15))/nsymb;
            off+=2;

		    
#ifdef DEBUG_PUCCH_RX
            printf("[eNB] PUCCH subframe %d (%d,%d,%d) => (%d,%d) x (%d,%d) : (%d,%d) , stat %d\n",subframe,phase,l,re,
                  rxcomp[aa][off],rxcomp[aa][1+off],
                  cfo[l<<1],cfo[1+(l<<1)],
                  stat_re,stat_im,stat);
#endif
          }

          for (l2=0,l=(nsymb>>1); l < nsymb; l++,l2++) {
            stat_re += (((rxcomp[aa][off]*(int32_t)cfo[l2<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l2<<1)])>>15))/nsymb;
            stat_im += (((rxcomp[aa][off]*(int32_t)cfo[1+(l2<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l2<<1)])>>15))/nsymb;
            off+=2;

#ifdef DEBUG_PUCCH_RX
            printf("[eNB] PUCCH subframe %d (%d,%d,%d) => (%d,%d) x (%d,%d) : (%d,%d), stat %d\n",subframe,phase,l2,re,
                  rxcomp[aa][off],rxcomp[aa][1+off],
                  cfo[l2<<1],cfo[1+(l2<<1)],
                  stat_re,stat_im,stat);
#endif


          }
	  stat += ((stat_re*stat_re) + (stat_im*stat_im));

       } //re
      } // aa

 
      if (stat>stat_max) {
        stat_max = stat;
        phase_max = phase;
      }

    } //phase

//    stat_max *= nsymb;  // normalize to energy per symbol
//    stat_max /= (frame_parms->N_RB_UL*12); //
    stat_max /= 12;
    
#ifdef DEBUG_PUCCH_RX
    printf("[eNB] PUCCH: stat %d, stat_max %d, phase_max %d\n", stat,stat_max,phase_max);
#endif

#ifdef DEBUG_PUCCH_RX
    LOG_I(PHY,"[eNB] PUCCH fmt1:  stat_max : %d, sigma2_dB %d (%d, %d), phase_max : %d\n",dB_fixed(stat_max),sigma2_dB,eNB->measurements.n0_subband_power_tot_dBm[6],pucch1_thres,phase_max);
#endif

    
    eNB->pucch1_stats[UE_id][(subframe<<10)+eNB->pucch1_stats_cnt[UE_id][subframe]] = stat_max;
    eNB->pucch1_stats_thres[UE_id][(subframe<<10)+eNB->pucch1_stats_cnt[UE_id][subframe]] = sigma2_dB+pucch1_thres;
    eNB->pucch1_stats_cnt[UE_id][subframe] = (eNB->pucch1_stats_cnt[UE_id][subframe]+1)&1023;

    T(T_ENB_PHY_PUCCH_1_ENERGY, T_INT(eNB->Mod_id), T_INT(eNB->uci_vars[UE_id].rnti), T_INT(frame), T_INT(subframe),
      T_INT(stat_max), T_INT(sigma2_dB+pucch1_thres));

    /*
    if (eNB->pucch1_stats_cnt[UE_id][subframe] == 0) {
      LOG_M("pucch_debug.m","pucch_energy",
		   &eNB->pucch1_stats[UE_id][(subframe<<10)],
		   1024,1,2);
      AssertFatal(0,"Exiting for PUCCH 1 debug\n");

    }
    */

    // This is a moving average of the PUCCH1 statistics conditioned on being above or below the threshold
    if (sigma2_dB<(dB_fixed(stat_max)-pucch1_thres))  {
      *payload = 1;
    }
    else {
      *payload = 0;
    }
    if (UE_id==0) {
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_SR_ENERGY,dB_fixed(stat_max));
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_SR_THRES,sigma2_dB+pucch1_thres);
    }
  } else if ((fmt == pucch_format1a)||(fmt == pucch_format1b)) {
    stat_max = 0;
#ifdef DEBUG_PUCCH_RX
    LOG_D(PHY,"Doing PUCCH detection for format 1a/1b\n");
#endif

    for (phase=0; phase<7; phase++) {
      stat=0;

      for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        for (re=0; re<12; re++) {

	  // compute received energy first slot, seperately for data and reference
	  // by coherent combining across symbols but not resource elements
	  // Note: assumption is that channel is stationary over symbols in slot after CFO
          stat_re=0;
          stat_im=0;
          stat_ref_re=0;
          stat_ref_im=0;
          off=re<<1;
          cfo =  (frame_parms->Ncp==0) ? &cfo_pucch_np[14*phase] : &cfo_pucch_ep[12*phase];


          for (l=0; l<(nsymb>>1); l++) {
            if ((l<2)||(l>(nsymb>>1) - 3)) {  //data symbols
              stat_re += ((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15);
              stat_im += ((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15);
            } else { //reference symbols
              stat_ref_re += ((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15);
              stat_ref_im += ((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15);
            }

            off+=2;
#ifdef DEBUG_PUCCH_RX
            printf("[eNB] PUCCH subframe %d (%d,%d) => (%d,%d) x (%d,%d) : (%d,%d)\n",subframe,l,re,
                  rxcomp[aa][off],rxcomp[aa][1+off],
                  cfo[l<<1],cfo[1+(l<<1)],
                  stat_re,stat_im);
#endif
          }
	  // this is total energy received, summed over data and reference
	  stat += ((((stat_re*stat_re)) + ((stat_im*stat_im)) +
		    ((stat_ref_re*stat_ref_re)) + ((stat_ref_im*stat_ref_im)))/nsymb);

	  // now second slot
	  stat_re=0;
	  stat_im=0;
          stat_ref_re=0;
          stat_ref_im=0;

          for (l2=0,l=(nsymb>>1); l< nsymb; l++,l2++) {
            if ((l2<2) || ((l2>(nsymb>>1) - 3)) ) {  // data symbols
              stat_re += ((rxcomp[aa][off]*(int32_t)cfo[l2<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l2<<1)])>>15);
              stat_im += ((rxcomp[aa][off]*(int32_t)cfo[1+(l2<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l2<<1)])>>15);
            } else { //reference_symbols
              stat_ref_re += ((rxcomp[aa][off]*(int32_t)cfo[l2<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l2<<1)])>>15);
              stat_ref_im += ((rxcomp[aa][off]*(int32_t)cfo[1+(l2<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l2<<1)])>>15);
            }

            off+=2;
#ifdef DEBUG_PUCCH_RX
            printf("[eNB] PUCCH subframe %d (%d,%d) => (%d,%d) x (%d,%d) : (%d,%d)\n",subframe,l2,re,
                  rxcomp[aa][off],rxcomp[aa][1+off],
                  cfo[l2<<1],cfo[1+(l2<<1)],
                  stat_re,stat_im);
#endif

          }

#ifdef DEBUG_PUCCH_RX
          printf("aa%d re %d : phase %d : stat %d\n",aa,re,phase,stat);
#endif

	  stat += ((((stat_re*stat_re)) + ((stat_im*stat_im)) +
		    ((stat_ref_re*stat_ref_re)) + ((stat_ref_im*stat_ref_im)))/nsymb);


        } //re
      } // aa

#ifdef DEBUG_PUCCH_RX
      LOG_D(PHY,"Format 1A: phase %d : stat %d\n",phase,stat);
#endif
      if (stat>stat_max) {
        stat_max = stat;
        phase_max = phase;
      }
    } //phase

    stat_max/=(12);  //normalize to energy per symbol and RE
#ifdef DEBUG_PUCCH_RX
    LOG_I(PHY,"[eNB] PUCCH fmt1a/b:  stat_max : %d (%d : sigma2 %d), phase_max : %d\n",stat_max,dB_fixed(stat_max),sigma2_dB,phase_max);
#endif

    stat_re=0;
    stat_im=0;

    // Do detection now
#if defined(USRP_REC_PLAY)
    // It looks like the value is a bit messy when RF is replayed.
    // For instance i assume to skip pucch1_thres from the test below.
    // Not 100% sure
    if (sigma2_dB<(dB_fixed(stat_max)))  {//
#else
    if (sigma2_dB<(dB_fixed(stat_max)-pucch1_thres))  {//
#endif
      chL = (nsymb>>1)-4;
      chest_mag=0;
      cfo =  (frame_parms->Ncp==0) ? &cfo_pucch_np[14*phase_max] : &cfo_pucch_ep[12*phase_max];

      for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
	// channel estimates first
        for (re=0; re<12; re++) {

          // channel estimate for first slot
          chest0_re[aa][re]=0;
          chest0_im[aa][re]=0;
	  for (l=2; l<(nsymb>>1)-2; l++) {
	    off=(re<<1) + (24*l);
	    chest0_re[aa][re] += (((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15))/chL;
	    chest0_im[aa][re] += (((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15))/chL;
	  }	    
	  

          // channel estimate for second slot
	  chest1_re[aa][re]=0;
	  chest1_im[aa][re]=0;
          for (l=2; l<(nsymb>>1)-2; l++) {
            off=(re<<1) + (24*l) + (nsymb>>1)*24;
            chest1_re[aa][re] += (((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15))/chL;
	    chest1_im[aa][re] += (((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15))/chL;
          }
	  chest_mag = max(chest_mag,(chest0_re[aa][re]*chest0_re[aa][re]) + (chest0_im[aa][re]*chest0_im[aa][re]));
	  chest_mag = max(chest_mag,(chest1_re[aa][re]*chest1_re[aa][re]) + (chest1_im[aa][re]*chest1_im[aa][re]));

	}
      }
      log2_maxh = log2_approx(chest_mag)/2;
#ifdef DEBUG_PUCCH_RX
      printf("PUCCH 1A: log2_maxh %d\n",log2_maxh);
#endif
	// now do channel matched filter
      for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
	for (re=0; re<12; re++) {

#ifdef DEBUG_PUCCH_RX
          printf("[eNB] PUCCH subframe %d chest0[%d][%d] => (%d,%d)\n",subframe,aa,re,
                chest0_re[aa][re],chest0_im[aa][re]);
#endif
	  // first slot, left of RS    
          for (l=0; l<2; l++) {
            off=(re<<1) + (24*l);
            tmp_re = ((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15);
            tmp_im = ((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15);
            stat_re += (((tmp_re*chest0_re[aa][re])>>log2_maxh) + ((tmp_im*chest0_im[aa][re])>>log2_maxh));
            stat_im += (((tmp_re*chest0_im[aa][re])>>log2_maxh) - ((tmp_im*chest0_re[aa][re])>>log2_maxh));
            off+=2;
#ifdef DEBUG_PUCCH_RX
            printf("[eNB] PUCCH subframe %d (%d,%d) => (%d,%d) x (%d,%d) : (%d,%d) => (%d,%d)\n",subframe,l,re,
		   rxcomp[aa][off],rxcomp[aa][1+off],
		   cfo[l<<1],cfo[1+(l<<1)],
		   tmp_re,tmp_im,
		   stat_re,stat_im);
#endif
	  }
	  // first slot, right of RS
          for (l=(nsymb>>1)-2; l<(nsymb>>1); l++) {
            off=(re<<1) + (24*l);
            tmp_re = ((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15);
            tmp_im = ((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15);
            stat_re += (((tmp_re*chest0_re[aa][re])>>log2_maxh) + ((tmp_im*chest0_im[aa][re])>>log2_maxh));
            stat_im += (((tmp_re*chest0_im[aa][re])>>log2_maxh) - ((tmp_im*chest0_re[aa][re])>>log2_maxh));
            off+=2;
#ifdef DEBUG_PUCCH_RX
            printf("[eNB] PUCCH subframe %d (%d,%d) => (%d,%d) x (%d,%d) : (%d,%d) => (%d,%d)\n",subframe,l,re,
		   rxcomp[aa][off],rxcomp[aa][1+off],
		   cfo[l<<1],cfo[1+(l<<1)],
		   tmp_re,tmp_im,
		   stat_re,stat_im);
#endif
	  }
	  
	  

#ifdef DEBUG_PUCCH_RX
          printf("[eNB] PUCCH subframe %d chest1[%d][%d] => (%d,%d)\n",subframe,aa,re,
                chest0_re[aa][re],chest0_im[aa][re]);
#endif
	  // second slot, left of RS    	  
          for (l=0; l<2; l++) {
	    off=(re<<1) + (24*l) + (nsymb>>1)*24;
	    tmp_re = ((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15);
	    tmp_im = ((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15);
	    stat_re += (((tmp_re*chest1_re[aa][re])>>log2_maxh) + ((tmp_im*chest1_im[aa][re])>>log2_maxh));
            stat_im += (((tmp_re*chest1_im[aa][re])>>log2_maxh) - ((tmp_im*chest1_re[aa][re])>>log2_maxh));
            off+=2;
#ifdef DEBUG_PUCCH_RX
            printf("[PHY][eNB] PUCCH subframe %d (%d,%d) => (%d,%d) x (%d,%d) : (%d,%d) => (%d,%d)\n",subframe,l,re,
		   rxcomp[aa][off],rxcomp[aa][1+off],
		   cfo[l<<1],cfo[1+(l<<1)],
		   tmp_re,tmp_im,
		   stat_re,stat_im);
#endif
          }

	  // second slot, right of RS    	  
          for (l=(nsymb>>1)-2; l<(nsymb>>1)-1; l++) {
            off=(re<<1) + (24*l) + (nsymb>>1)*24;
            tmp_re = ((rxcomp[aa][off]*(int32_t)cfo[l<<1])>>15)     - ((rxcomp[aa][1+off]*(int32_t)cfo[1+(l<<1)])>>15);
            tmp_im = ((rxcomp[aa][off]*(int32_t)cfo[1+(l<<1)])>>15) + ((rxcomp[aa][1+off]*(int32_t)cfo[(l<<1)])>>15);
            stat_re += (((tmp_re*chest1_re[aa][re])>>log2_maxh) + ((tmp_im*chest1_im[aa][re])>>log2_maxh));
            stat_im += (((tmp_re*chest1_im[aa][re])>>log2_maxh) - ((tmp_im*chest1_re[aa][re])>>log2_maxh));
            off+=2;
#ifdef DEBUG_PUCCH_RX
            printf("[PHY][eNB] PUCCH subframe %d (%d,%d) => (%d,%d) x (%d,%d) : (%d,%d) => (%d,%d)\n",subframe,l,re,
		   rxcomp[aa][off],rxcomp[aa][1+off],
		   cfo[l<<1],cfo[1+(l<<1)],
		   tmp_re,tmp_im,
		   stat_re,stat_im);
#endif
          }

#ifdef DEBUG_PUCCH_RX
          printf("aa%d re %d : stat %d,%d\n",aa,re,stat_re,stat_im);
#endif

        } //re
      } // aa


      LOG_D(PHY,"PUCCH 1a/b: subframe %d : stat %d,%d (pos %d)\n",subframe,stat_re,stat_im,
	    (subframe<<10) + (eNB->pucch1ab_stats_cnt[UE_id][subframe]));
      LOG_D(PHY,"In pucch.c PUCCH 1a/b: ACK subframe %d : sigma2_dB %d, stat_max %d, pucch1_thres %d\n",subframe,sigma2_dB,dB_fixed(stat_max),pucch1_thres);
      
      eNB->pucch1ab_stats[UE_id][(subframe<<11) + 2*(eNB->pucch1ab_stats_cnt[UE_id][subframe])] = (stat_re);
      eNB->pucch1ab_stats[UE_id][(subframe<<11) + 1+2*(eNB->pucch1ab_stats_cnt[UE_id][subframe])] = (stat_im);
      eNB->pucch1ab_stats_cnt[UE_id][subframe] = (eNB->pucch1ab_stats_cnt[UE_id][subframe]+1)&1023;

      /* frame not available here - set to -1 for the moment */
      T(T_ENB_PHY_PUCCH_1AB_IQ, T_INT(eNB->Mod_id), T_INT(eNB->uci_vars[UE_id].rnti), T_INT(-1), T_INT(subframe), T_INT(stat_re), T_INT(stat_im));

      *payload = (stat_re<0) ? 1 : 2; // 1 == ACK, 2 == NAK

      if (fmt==pucch_format1b)
        *(1+payload) = (stat_im<0) ? 1 : 2;
    } else { // insufficient energy on PUCCH so NAK
#if defined(USRP_REC_PLAY)
      LOG_D(PHY,"PUCCH 1a/b: NAK subframe %d : sigma2_dB %d, stat_max %d, pucch1_thres %d\n",subframe,sigma2_dB,dB_fixed(stat_max),pucch1_thres);
#else
      LOG_D(PHY,"In pucch.c PUCCH 1a/b: NAK subframe %d : sigma2_dB %d, stat_max %d, pucch1_thres %d\n",subframe,sigma2_dB,dB_fixed(stat_max),pucch1_thres);
#endif

      *payload = 4;  // DTX
      ((int16_t*)&eNB->pucch1ab_stats[UE_id][(subframe<<10) + (eNB->pucch1ab_stats_cnt[UE_id][subframe])])[0] = (int16_t)(stat_re);
      ((int16_t*)&eNB->pucch1ab_stats[UE_id][(subframe<<10) + (eNB->pucch1ab_stats_cnt[UE_id][subframe])])[1] = (int16_t)(stat_im);
      eNB->pucch1ab_stats_cnt[UE_id][subframe] = (eNB->pucch1ab_stats_cnt[UE_id][subframe]+1)&1023;

      if (fmt==pucch_format1b)
        *(1+payload) = 4;
    }
  } else {
    LOG_E(PHY,"[eNB] PUCCH fmt2/2a/2b not supported\n");
  }
  
  /* PUCCH format3 >> */
  } else {
    /* SubCarrier Demap */
    Ret = pucchfmt3_subCarrierDeMapping( eNB, SubCarrierDeMapData, n3_pucch );
    if(Ret != 0) {
        //***log pucchfmt3_subCarrierDeMapping Error!
        return(-1);
    }
    
    /* cyclic shift hopping remove */
    Ret = pucchfmt3_Baseseq_csh_remove( SubCarrierDeMapData, CshData_fmt3, frame_parms, subframe, eNB->ncs_cell );
    if(Ret != 0) {
        //***log pucchfmt3_Baseseq_csh_remove Error!
        return(-1);
    }
    
    /* Channel Estimation */
    Ret = pucchfmt3_ChannelEstimation( SubCarrierDeMapData, delta_theta, ChestValue, &Interpw, subframe, shortened_format, frame_parms, n3_pucch, n3_pucch_array, eNB->ncs_cell );
    if(Ret != 0) {
        //***log pucchfmt3_ChannelEstimation Error!
        return(-1);
    }
    
    /* Channel Equalization */
    Ret = pucchfmt3_Equalization( CshData_fmt3, ChdetAfterValue_fmt3, ChestValue, frame_parms );
    if(Ret != 0) {
        //***log pucchfmt3_Equalization Error!
        return(-1);
    }
    
    /* Frequency deviation remove AFC */
    Ret = pucchfmt3_FrqDevRemove( ChdetAfterValue_fmt3, delta_theta, RemoveFrqDev_fmt3, frame_parms );
    if(Ret != 0) {
        //***log pucchfmt3_FrqDevRemove Error!
        return(-1);
    }
    
    /* orthogonal sequence remove */
    Ret = pucchfmt3_OrthSeqRemove( RemoveFrqDev_fmt3, Fmt3xDataRmvOrth, shortened_format, n3_pucch, frame_parms );
    if(Ret != 0) {
        //***log pucchfmt3_OrthSeqRemove Error!
        return(-1);
    }
    
    /* averaging antenna */
    pucchfmt3_AvgAnt( Fmt3xDataRmvOrth, Fmt3xDataAvgAnt, shortened_format, frame_parms );
    
    /* averaging symbol */
    pucchfmt3_AvgSym( Fmt3xDataAvgAnt, Fmt3xDataAvgSym, shortened_format );
    
    /* IDFT */
    pucchfmt3_IDft2( (int16_t*)Fmt3xDataAvgSym[0], (int16_t*)IFFTOutData_Fmt3[0] );
    pucchfmt3_IDft2( (int16_t*)Fmt3xDataAvgSym[1], (int16_t*)IFFTOutData_Fmt3[1] );
    
    /* descramble */
    pucchfmt3_Descramble(IFFTOutData_Fmt3, b, subframe, frame_parms->Nid_cell, crnti);

    /* Is payload 6bit or 7bit? */
    if( do_sr == 1 ) {
        payload_max = 7;
    } else {
        payload_max = 6;
    }
    
    /* decode */
    payload_entity = pucchfmt3_Decode( b, subframe, DTXthreshold, Interpw, do_sr );
    if (payload_entity == -1) {
        //***log pucchfmt3_Decode Error!
        return(-1);
    }
    
    for(i=0; i<payload_max; i++) {
      *(payload+i) = (uint8_t)((payload_entity>>i) & 0x01);
    }
  }
  /* PUCCH format3 << */

  return((int32_t)stat_max);

}
