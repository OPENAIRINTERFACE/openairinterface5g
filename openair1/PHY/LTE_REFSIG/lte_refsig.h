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

/* Definitions for LTE Reference signals */
/* Author R. Knopp / EURECOM / OpenAirInterface.org */
#ifndef __LTE_REFSIG_DEFS__H__
#define __LTE_REFSIG_DEFS__H__
#include "PHY/defs_eNB.h"
#include "PHY/defs_UE.h"

/** @ingroup _PHY_REF_SIG
 * @{
*/

/*! \brief gold sequenquence generator
\param x1
\param x2 this should be set to c_init if reset=1
\param reset resets the generator
\return 32 bits of the gold sequence
*/
uint32_t lte_gold_generic(uint32_t *x1, uint32_t *x2, uint8_t reset);


/*!\brief This function generates the LTE Gold sequence (36-211, Sec 7.2), specifically for DL reference signals.
@param frame_parms LTE DL Frame parameters
@param lte_gold_table pointer to table where sequences are stored
@param Nid_cell Cell Id (to compute sequences for local and adjacent cells) */

void lte_gold(LTE_DL_FRAME_PARMS *frame_parms,uint32_t lte_gold_table[20][2][14],uint16_t Nid_cell);
void lte_gold_ue_spec(uint32_t lte_gold_uespec_table[2][20][2][21],uint16_t Nid_cell, uint16_t *n_idDMRS);

void lte_gold_ue_spec_port5(uint32_t lte_gold_uespec_port5_table[20][38],uint16_t Nid_cell, uint16_t n_rnti);

/*!\brief This function generates the LTE Gold sequence (36-211, Sec 7.2), specifically for DL UE-specific reference signals for antenna ports 7..14.
@param frame_parms LTE DL Frame parameters
@param lte_gold_uespec_table pointer to table where sequences are stored
@param Nid_cell Cell Id (to compute sequences for local and adjacent cells)
@param n_idDMRS Scrambling identity for TM10*/

void lte_gold_mbsfn(LTE_DL_FRAME_PARMS *frame_parms,uint32_t lte_gold_mbsfn_table[10][3][42],uint16_t Nid_MBSFN);


/*! \brief This function generates the cell-specific reference signal sequence (36-211, Sec 6.10.1.1)
@param phy_vars_eNB Pointer to eNB variables
@param output Output vector for OFDM symbol (Frequency Domain)
@param amp Q15 amplitude
@param Ns Slot number (0..19)
@param l symbol (0,1) - Note 1 means 3!
@param p antenna index
*/


int lte_dl_cell_spec(PHY_VARS_eNB *phy_vars_eNB,
                     int32_t *output,
                     short amp,
                     unsigned char Ns,
                     unsigned char l,
                     unsigned char p);

/*! \brief This function generates the UE-specific reference signal sequence (36-211, Sec 6.10.3.2)
@param phy_vars_eNB Pointer to eNB variables
@param output Output vector for OFDM symbol (Frequency Domain)
@param amp Q15 amplitude
@param Ns Slot number (0..19)
@param lprime symbol (0,1)
@param p antenna index
@param SS_flag Flag to indicate special subframe
*/
/*int lte_dl_ue_spec(PHY_VARS_eNB *phy_vars_eNB,
                   uint8_t UE_id,
                   int32_t *output,
                   short amp,
                   uint8_t Ns,
		   uint8_t lprime,
                   uint8_t p,
                   int SS_flag);*/

/*! \brief This function generates the MBSFN reference signal sequence (36-211, Sec 6.10.1.2)
@param phy_vars_eNB Pointer to eNB variables
@param output Output vector for OFDM symbol (Frequency Domain)
@param amp Q15 amplitude
@param Ns Slot number (0..19)
@param l symbol (0,1,2)
*/
int lte_dl_mbsfn(PHY_VARS_eNB *phy_vars_eNB, int32_t *output,
                 short amp,
                 int subframe,
                 unsigned char l);


/*!\brief This function generates the cell-specific reference signal sequence (36-211, Sec 6.10.1.1) for channel estimation upon reception
@param phy_vars_ue Pointer to UE variables
@param eNB_offset offset with respect to Nid_cell in frame_parms of current eNB (to estimate channels of adjacent eNBs)
@param output Output vector for OFDM symbol (Frequency Domain)
@param Ns Slot number (0..19)
@param l symbol (0,1) - Note 1 means 3!
@param p antenna intex
*/
int lte_dl_cell_spec_rx(PHY_VARS_UE *phy_vars_ue,
                        uint8_t eNB_offset,
                        int *output,
                        unsigned char Ns,
                        unsigned char l,
                        unsigned char p);

/*!\brief This function generates the ue-specific reference signal
 * sequence (36-211, Sec 6.10.3.1) for beamforming channel estimation upon reception
@param phy_vars_ue Pointer to UE variables
@param output Output vector for OFDM symbol (Frequency Domain)
@param Ns Slot number (0..19)
@param p antenna port intex
@param lprime symbol (0,1)
@param SS_flag Flag to indicate special subframe
@param nRB_PDSCH number of allocated PDSCH RBs
*/
int lte_dl_ue_spec_rx(PHY_VARS_UE *phy_vars_ue,
                      int32_t *output,
                      unsigned char Ns,
                      unsigned char p,
                      int lprime,
                      int SS_flag,
                      uint16_t nRB_PDSCH);

int lte_dl_mbsfn_rx(PHY_VARS_UE *phy_vars_ue,
                    int *output,
                    int subframe,
                    unsigned char l);




void generate_ul_ref_sigs(void);
void generate_ul_ref_sigs_rx(void);

void free_ul_ref_sigs(void);

/*!
\brief This function generate the sounding reference symbol (SRS) for the uplink. The SRS is always transmitted in the last symbol of the slot and uses the full bandwidth. This function makes the following simplifications wrt LTE Rel.8:
 1) the SRS in OpenAir is quantized to a QPSK sequence.
 2) no group hopping, no sequence hopping
 3) u = N_id_cell%30, v=0, alpha=0,
 4) Msc_RS = 300, k_0=0
@param txdataF pointer to the frequency domain TX signal
@param amp amplitudte of the transmit signal (irrelevant for #ifdef IFFT_FPGA)
@param frame_parms LTE DL Frame Parameters
@sub_frame_offset  Offset of this subframe in units of subframes
*/

int lte_generate_srs(int32_t **txdataF,
                     short amp,
                     LTE_DL_FRAME_PARMS *frame_parms,
                     unsigned int sub_frame_offset);


#endif
