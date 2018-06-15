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

/*! \file PHY/LTE_TRANSPORT/transport_proto.h
 * \brief Function prototypes for eNB PHY physical/transport channel processing and generation V8.6 2009-03
 * \author R. Knopp, F. Kaltenberger
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */
#ifndef __LTE_TRANSPORT_COMMON_PROTO__H__
#define __LTE_TRANSPORT_COMMON_PROTO__H__
#include "PHY/defs_common.h"




// Functions below implement minor procedures from 36-211 and 36-212

/** \brief Compute Q (modulation order) based on I_MCS PDSCH.  Implements table 7.1.7.1-1 from 36.213.
    @param I_MCS */
uint8_t get_Qm(uint8_t I_MCS);

/** \brief Compute Q (modulation order) based on I_MCS for PUSCH.  Implements table 8.6.1-1 from 36.213.
    @param I_MCS */
uint8_t get_Qm_ul(uint8_t I_MCS);

/** \brief Compute I_TBS (transport-block size) based on I_MCS for PDSCH.  Implements table 7.1.7.1-1 from 36.213.
    @param I_MCS */
uint8_t get_I_TBS(uint8_t I_MCS);

/** \brief Compute I_TBS (transport-block size) based on I_MCS for PUSCH.  Implements table 8.6.1-1 from 36.213.
    @param I_MCS */
unsigned char get_I_TBS_UL(unsigned char I_MCS);

/** \brief Compute Q (modulation order) based on downlink I_MCS. Implements table 7.1.7.1-1 from 36.213.
    @param I_MCS
    @param nb_rb
    @return Transport block size */
uint32_t get_TBS_DL(uint8_t mcs, uint16_t nb_rb);

/** \brief Compute Q (modulation order) based on uplink I_MCS. Implements table 7.1.7.1-1 from 36.213.
    @param I_MCS
    @param nb_rb
    @return Transport block size */
uint32_t get_TBS_UL(uint8_t mcs, uint16_t nb_rb);

/* \brief Return bit-map of resource allocation for a given DCI rballoc (RIV format) and vrb type
   @param N_RB_DL number of PRB on DL
   @param indicator for even/odd slot
   @param vrb vrb index
   @param Ngap Gap indicator
*/
uint32_t get_prb(int N_RB_DL,int odd_slot,int vrb,int Ngap);

/* \brief Return prb for a given vrb index
   @param vrb_type VRB type (0=localized,1=distributed)
   @param rb_alloc_dci rballoc field from DCI
*/
uint32_t get_rballoc(vrb_t vrb_type,uint16_t rb_alloc_dci);


/* \brief Return bit-map of resource allocation for a given DCI rballoc (RIV format) and vrb type
   @returns Transmission mode (1-7)
*/
uint8_t get_transmission_mode(module_id_t Mod_id, uint8_t CC_id, rnti_t rnti);


/* \brief
   @param ra_header Header of resource allocation (0,1) (See sections 7.1.6.1/7.1.6.2 of 36.213 Rel8.6)
   @param rb_alloc Bitmap allocation from DCI (format 1,2)
   @returns number of physical resource blocks
*/
uint32_t conv_nprb(uint8_t ra_header,uint32_t rb_alloc,int N_RB_DL);

int get_G(LTE_DL_FRAME_PARMS *frame_parms,uint16_t nb_rb,uint32_t *rb_alloc,uint8_t mod_order,uint8_t Nl,uint8_t num_pdcch_symbols,int frame,uint8_t subframe, uint8_t beamforming_mode);

int adjust_G(LTE_DL_FRAME_PARMS *frame_parms,uint32_t *rb_alloc,uint8_t mod_order,uint8_t subframe);
int adjust_G2(LTE_DL_FRAME_PARMS *frame_parms,uint32_t *rb_alloc,uint8_t mod_order,uint8_t subframe,uint8_t symbol);


#ifndef modOrder
#define modOrder(I_MCS,I_TBS) ((I_MCS-I_TBS)*2+2) // Find modulation order from I_TBS and I_MCS
#endif

/** \fn uint8_t I_TBS2I_MCS(uint8_t I_TBS);
    \brief This function maps I_tbs to I_mcs according to Table 7.1.7.1-1 in 3GPP TS 36.213 V8.6.0. Where there is two supported modulation orders for the same I_TBS then either high or low modulation is chosen by changing the equality of the two first comparisons in the if-else statement.
    \param I_TBS Index of Transport Block Size
    \return I_MCS given I_TBS
*/
uint8_t I_TBS2I_MCS(uint8_t I_TBS);

/** \fn uint8_t SE2I_TBS(float SE,
    uint8_t N_PRB,
    uint8_t symbPerRB);
    \brief This function maps a requested throughput in number of bits to I_tbs. The throughput is calculated as a function of modulation order, RB allocation and number of symbols per RB. The mapping orginates in the "Transport block size table" (Table 7.1.7.2.1-1 in 3GPP TS 36.213 V8.6.0)
    \param SE Spectral Efficiency (before casting to integer, multiply by 1024, remember to divide result by 1024!)
    \param N_PRB Number of PhysicalResourceBlocks allocated \sa lte_frame_parms->N_RB_DL
    \param symbPerRB Number of symbols per resource block allocated to this channel
    \return I_TBS given an SE and an N_PRB
*/
uint8_t SE2I_TBS(float SE,
                 uint8_t N_PRB,
                 uint8_t symbPerRB);
/** \brief This function generates the sounding reference symbol (SRS) for the uplink according to 36.211 v8.6.0. If IFFT_FPGA is defined, the SRS is quantized to a QPSK sequence.
    @param frame_parms LTE DL Frame Parameters
    @param soundingrs_ul_config_dedicated Dynamic configuration from RRC during Connection Establishment
    @param txdataF pointer to the frequency domain TX signal
    @returns 0 on success*/

int generate_srs(LTE_DL_FRAME_PARMS *frame_parms,
		 SOUNDINGRS_UL_CONFIG_DEDICATED *soundingrs_ul_config_dedicated,
		 int *txdataF,
		 int16_t amp,
		 uint32_t subframe);


/*!
  \brief This function generates the downlink reference signal for the PUSCH according to 36.211 v8.6.0. The DRS occuies the RS defined by rb_alloc and the symbols 2 and 8 for extended CP and 3 and 10 for normal CP.
*/


/*!
  \brief This function initializes the Group Hopping, Sequence Hopping and nPRS sequences for PUCCH/PUSCH according to 36.211 v8.6.0. It should be called after configuration of UE (reception of SIB2/3) and initial configuration of eNB (or after reconfiguration of cell-specific parameters).
  @param frame_parms Pointer to a LTE_DL_FRAME_PARMS structure (eNB or UE)*/
void init_ul_hopping(LTE_DL_FRAME_PARMS *frame_parms);


int32_t compareints (const void * a, const void * b);

uint8_t subframe2harq_pid(LTE_DL_FRAME_PARMS *frame_parms,frame_t frame,uint8_t subframe);

int dump_dci(LTE_DL_FRAME_PARMS *frame_parms, DCI_ALLOC_t *dci);

void generate_pcfich_reg_mapping(LTE_DL_FRAME_PARMS *frame_parms);

void generate_phich_reg_mapping(LTE_DL_FRAME_PARMS *frame_parms);

uint32_t check_phich_reg(LTE_DL_FRAME_PARMS *frame_parms,uint32_t kprime,uint8_t lprime,uint8_t mi);

void generate_RIV_tables(void);

/** \brief  This routine provides the relationship between a PHICH TXOp and its corresponding PUSCH subframe (Table 8.3.-1 from 36.213).
    @param frame_parms Pointer to DL frame configuration parameters
    @param subframe Subframe of received/transmitted PHICH
    @returns subframe of PUSCH transmission
*/
uint8_t phich_subframe2_pusch_subframe(LTE_DL_FRAME_PARMS *frame_parms,uint8_t subframe);

/** \brief  This routine provides the relationship between a PHICH TXOp and its corresponding PUSCH frame (Table 8.3.-1 from 36.213).
    @param frame_parms Pointer to DL frame configuration parameters
    @param frame Frame of received/transmitted PHICH
    @param subframe Subframe of received/transmitted PHICH
    @returns frame of PUSCH transmission
*/
int phich_frame2_pusch_frame(LTE_DL_FRAME_PARMS *frame_parms, int frame, int subframe);

uint16_t computeRIV(uint16_t N_RB_DL,uint16_t RBstart,uint16_t Lcrbs);

int get_nCCE_offset_l1(int *CCE_table,
                       const unsigned char L,
                       const int nCCE,
                       const int common_dci,
                       const unsigned short rnti,
                       const unsigned char subframe);

uint16_t get_nCCE(uint8_t num_pdcch_symbols,LTE_DL_FRAME_PARMS *frame_parms,uint8_t mi);

uint16_t get_nquad(uint8_t num_pdcch_symbols,LTE_DL_FRAME_PARMS *frame_parms,uint8_t mi);

uint8_t get_mi(LTE_DL_FRAME_PARMS *frame,uint8_t subframe);

uint16_t get_nCCE_mac(uint8_t Mod_id,uint8_t CC_id,int num_pdcch_symbols,int subframe);

uint8_t get_num_pdcch_symbols(uint8_t num_dci,DCI_ALLOC_t *dci_alloc,LTE_DL_FRAME_PARMS *frame_parms,uint8_t subframe);

void init_ncs_cell(LTE_DL_FRAME_PARMS *frame_parms,uint8_t ncs_cell[20][7]);

/*!
  \brief Check for PRACH TXop in subframe
  @param frame_parms Pointer to LTE_DL_FRAME_PARMS
  @param frame frame index to check
  @param subframe subframe index to check
  @returns 0 on success
*/
int is_prach_subframe(LTE_DL_FRAME_PARMS *frame_parms,frame_t frame, uint8_t subframe);

/*!
  \brief Helper for MAC, returns number of available PRACH in TDD for a particular configuration index
  @param frame_parms Pointer to LTE_DL_FRAME_PARMS structure
  @returns 0-5 depending on number of available prach
*/
uint8_t get_num_prach_tdd(module_id_t Mod_id);

/*!
  \brief Return the PRACH format as a function of the Configuration Index and Frame type.
  @param prach_ConfigIndex PRACH Configuration Index
  @param frame_type 0-FDD, 1-TDD
  @returns 0-1 accordingly
*/
uint8_t get_prach_fmt(uint8_t prach_ConfigIndex,lte_frame_type_t frame_type);

/*!
  \brief Helper for MAC, returns frequency index of PRACH resource in TDD for a particular configuration index
  @param frame_parms Pointer to LTE_DL_FRAME_PARMS structure
  @returns 0-5 depending on number of available prach
*/
uint8_t get_fid_prach_tdd(module_id_t Mod_id,uint8_t tdd_map_index);

/*!
  \brief Comp ute DFT of PRACH ZC sequences.  Used for generation of prach in UE and reception of PRACH in eNB.
  @param rootSequenceIndex PRACH root sequence
  #param prach_ConfigIndex PRACH Configuration Index
  @param zeroCorrelationZoneConfig PRACH ncs_config
  @param highSpeedFlat PRACH High-Speed Flag
  @param frame_type TDD/FDD flag
  @param Xu DFT output
*/
void compute_prach_seq(uint16_t rootSequenceIndex,
		       uint8_t prach_ConfigIndex,
		       uint8_t zeroCorrelationZoneConfig,
		       uint8_t highSpeedFlag,
		       lte_frame_type_t frame_type,
		       uint32_t X_u[64][839]);


void init_prach_tables(int N_ZC);

/*!
  \brief Return the status of MBSFN in this frame/subframe
  @param frame Frame index
  @param subframe Subframe index
  @param frame_parms Pointer to frame parameters
  @returns 1 if subframe is for MBSFN
*/
int is_pmch_subframe(frame_t frame, int subframe, LTE_DL_FRAME_PARMS *frame_parms);


/** \brief  This routine expands a single (wideband) PMI to subband PMI bitmap similar to the one used in the UCI and in the dlsch_modulation routine
    @param frame_parms Pointer to DL frame configuration parameters
    @param wideband_pmi (0,1,2,3 for rank 0 and 0,1 for rank 1)
    @param rank (0 or 1)
    @returns subband PMI bitmap
*/
uint32_t pmi_extend(LTE_DL_FRAME_PARMS *frame_parms,uint8_t wideband_pmi, uint8_t rank);

uint64_t pmi2hex_2Ar1(uint32_t pmi);

uint64_t pmi2hex_2Ar2(uint32_t pmi);

uint8_t get_pmi(uint8_t N_RB_DL,MIMO_mode_t mode, uint32_t pmi_alloc,uint16_t rb);

// DL power control functions
double get_pa_dB(uint8_t pa);

void init_scrambling_lut(void);

void init_unscrambling_lut(void);


uint8_t get_prach_prb_offset(LTE_DL_FRAME_PARMS *frame_parms, 
			     uint8_t prach_ConfigIndex, 
			     uint8_t n_ra_prboffset,
			     uint8_t tdd_mapindex, uint16_t Nf);

uint8_t subframe2harq_pid(LTE_DL_FRAME_PARMS *frame_parms,frame_t frame,uint8_t subframe);
uint8_t ul_subframe2pdcch_alloc_subframe(LTE_DL_FRAME_PARMS *frame_parms,uint8_t n);

uint32_t conv_1C_RIV(int32_t rballoc,uint32_t N_RB_DL);

void conv_rballoc(uint8_t ra_header,uint32_t rb_alloc,uint32_t N_RB_DL,uint32_t *rb_alloc2);

int16_t estimate_ue_tx_power(uint32_t tbs, uint32_t nb_rb, uint8_t control_only, lte_prefix_type_t ncp, uint8_t use_srs);

#endif
