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
#ifndef __LTE_TRANSPORT_PROTO__H__
#define __LTE_TRANSPORT_PROTO__H__
#include "PHY/defs_eNB.h"
#include <math.h>
#include "nfapi_interface.h"
#include "transport_common_proto.h"

// Functions below implement 36-211 and 36-212

/** @addtogroup _PHY_TRANSPORT_
 * @{
 */

/** \fn free_eNB_dlsch(LTE_eNB_DLSCH_t *dlsch,unsigned char N_RB_DL)
    \brief This function frees memory allocated for a particular DLSCH at eNB
    @param dlsch Pointer to DLSCH to be removed
*/
void free_eNB_dlsch(LTE_eNB_DLSCH_t *dlsch);

void clean_eNb_dlsch(LTE_eNB_DLSCH_t *dlsch);

/** \fn new_eNB_dlsch(uint8_t Kmimo,uint8_t Mdlharq,uint32_t Nsoft,uint8_t abstraction_flag, LTE_DL_FRAME_PARMS* frame_parms)
    \brief This function allocates structures for a particular DLSCH at eNB
    @returns Pointer to DLSCH to be removed
    @param Kmimo Kmimo factor from 36-212/36-213
    @param Mdlharq Maximum number of HARQ rounds (36-212/36-213)
    @param Nsoft Soft-LLR buffer size from UE-Category
    @params N_RB_DL total number of resource blocks (determine the operating BW)
    @param abstraction_flag Flag to indicate abstracted interface
    @param frame_parms Pointer to frame descriptor structure
*/
LTE_eNB_DLSCH_t *new_eNB_dlsch(uint8_t Kmimo,uint8_t Mdlharq,uint32_t Nsoft,uint8_t N_RB_DL, uint8_t abstraction_flag, LTE_DL_FRAME_PARMS *frame_parms);

void clean_eNb_ulsch(LTE_eNB_ULSCH_t *ulsch);

/** \fn free_eNB_ulsch(LTE_eNB_DLSCH_t *dlsch)
    \brief This function frees memory allocated for a particular ULSCH at eNB
    @param ulsch Pointer to ULSCH to be removed
*/
void free_eNB_ulsch(LTE_eNB_ULSCH_t *ulsch);

LTE_eNB_ULSCH_t *new_eNB_ulsch(uint8_t max_turbo_iterations,uint8_t N_RB_UL, uint8_t abstraction_flag);

/** \fn dlsch_encoding(PHY_VARS_eNB *eNB,
    uint8_t *input_buffer,
    LTE_DL_FRAME_PARMS *frame_parms,
    uint8_t num_pdcch_symbols,
    LTE_eNB_DLSCH_t *dlsch,
    int frame,
    uint8_t subframe)
    \brief This function performs a subset of the bit-coding functions for LTE as described in 36-212, Release 8.Support is limited to turbo-coded channels (DLSCH/ULSCH). The implemented functions are:
    - CRC computation and addition
    - Code block segmentation and sub-block CRC addition
    - Channel coding (Turbo coding)
    - Rate matching (sub-block interleaving, bit collection, selection and transmission
    - Code block concatenation
    @param eNB Pointer to eNB PHY context
    @param input_buffer Pointer to input buffer for sub-frame
    @param frame_parms Pointer to frame descriptor structure
    @param num_pdcch_symbols Number of PDCCH symbols in this subframe
    @param dlsch Pointer to dlsch to be encoded
    @param frame Frame number
    @param subframe Subframe number
    @param rm_stats Time statistics for rate-matching
    @param te_stats Time statistics for turbo-encoding
    @param i_stats Time statistics for interleaving
    @returns status
*/
int32_t dlsch_encoding(PHY_VARS_eNB *eNB,
                      L1_rxtx_proc_t *proc,
                       uint8_t *a,
                       uint8_t num_pdcch_symbols,
                       LTE_eNB_DLSCH_t *dlsch,
                       int frame,
                       uint8_t subframe,
                       time_stats_t *rm_stats,
                       time_stats_t *te_stats,
                       time_stats_t *i_stats);

/** \fn dlsch_encoding(PHY_VARS_eNB *eNB,
    uint8_t *input_buffer,
    LTE_DL_FRAME_PARMS *frame_parms,
    uint8_t num_pdcch_symbols,
    LTE_eNB_DLSCH_t *dlsch,
    int frame,
    uint8_t subframe)
    \brief This function performs a subset of the bit-coding functions for LTE as described in 36-212, Release 8.Support is limited to turbo-coded channels (DLSCH/ULSCH). The implemented functions are:
    - CRC computation and addition
    - Code block segmentation and sub-block CRC addition
    - Channel coding (Turbo coding)
    - Rate matching (sub-block interleaving, bit collection, selection and transmission
    - Code block concatenation
    @param eNB Pointer to eNB PHY context
    @param input_buffer Pointer to input buffer for sub-frame
    @param frame_parms Pointer to frame descriptor structure
    @param num_pdcch_symbols Number of PDCCH symbols in this subframe
    @param dlsch Pointer to dlsch to be encoded
    @param frame Frame number
    @param subframe Subframe number
    @param rm_stats Time statistics for rate-matching
    @param te_stats Time statistics for turbo-encoding
    @param i_stats Time statistics for interleaving
    @returns status
*/
int32_t dlsch_encoding_fembms_pmch(PHY_VARS_eNB *eNB,
                       L1_rxtx_proc_t *proc,
                       uint8_t *a,
                       uint8_t num_pdcch_symbols,
                       LTE_eNB_DLSCH_t *dlsch,
                       int frame,
                       uint8_t subframe,
                       time_stats_t *rm_stats,
                       time_stats_t *te_stats,
                       time_stats_t *i_stats);




/** \fn dlsch_encoding_2threads(PHY_VARS_eNB *eNB,
    uint8_t *input_buffer,
    uint8_t num_pdcch_symbols,
    LTE_eNB_DLSCH_t *dlsch,
    int frame,
    uint8_t subframe)
    \brief This function performs a subset of the bit-coding functions for LTE as described in 36-212, Release 8.Support is limited to turbo-coded channels (DLSCH/ULSCH). This version spawns 1 worker thread. The implemented functions are:
    - CRC computation and addition
    - Code block segmentation and sub-block CRC addition
    - Channel coding (Turbo coding)
    - Rate matching (sub-block interleaving, bit collection, selection and transmission
    - Code block concatenation
    @param eNB Pointer to eNB PHY context
    @param input_buffer Pointer to input buffer for sub-frame
    @param num_pdcch_symbols Number of PDCCH symbols in this subframe
    @param dlsch Pointer to dlsch to be encoded
    @param frame Frame number
    @param subframe Subframe number
    @param rm_stats Time statistics for rate-matching
    @param te_stats Time statistics for turbo-encoding
    @param i_stats Time statistics for interleaving
    @returns status
*/
int32_t dlsch_encoding_2threads(PHY_VARS_eNB *eNB,
                                uint8_t *a,
                                uint8_t num_pdcch_symbols,
                                LTE_eNB_DLSCH_t *dlsch,
                                int frame,
                                uint8_t subframe,
                                time_stats_t *rm_stats,
                                time_stats_t *te_stats,
                                time_stats_t *te_wait_stats,
                                time_stats_t *te_main_stats,
                                time_stats_t *te_wakeup_stats0,
                                time_stats_t *te_wakeup_stats1,
                                time_stats_t *i_stats,
                                int worker_num);

// Functions below implement 36-211

/** \fn allocate_REs_in_RB(int32_t **txdataF,
    uint32_t *jj,
    uint32_t *jj2,
    uint16_t re_offset,
    uint32_t symbol_offset,
    LTE_DL_eNB_HARQ_t *dlsch0_harq,
    LTE_DL_eNB_HARQ_t *dlsch1_harq,
    uint8_t pilots,
    int16_t amp,
    int16_t *qam_table_s,
    uint32_t *re_allocated,
    uint8_t skip_dc,
    uint8_t skip_half,
    uint8_t use2ndpilots,
    LTE_DL_FRAME_PARMS *frame_parms);

    \brief Fills RB with data
    \param txdataF pointer to output data (frequency domain signal)
    \param jj index to output (from CW 1)
    \param jj index to output (from CW 2)
    \param re_offset index of the first RE of the RB
    \param symbol_offset index to the OFDM symbol
    \param dlsch0_harq Pointer to Transport block 0 HARQ structure
    \param dlsch0_harq Pointer to Transport block 1 HARQ structure
    \param pilots =1 if symbol_offset is an OFDM symbol that contains pilots, 0 otherwise
    \param amp Amplitude for symbols
    \param qam_table_s0 pointer to scaled QAM table for Transport Block 0 (by rho_a or rho_b)
    \param qam_table_s1 pointer to scaled QAM table for Transport Block 1 (by rho_a or rho_b)
    \param re_allocated pointer to allocation counter
    \param skip_dc offset for positive RBs
    \param skip_half indicate that first or second half of RB must be skipped for PBCH/PSS/SSS
    \param use2ndpilots Set to use the pilots from antenna port 1 for PDSCH
    \param frame_parms Frame parameter descriptor
*/

void init_modulation_LUTs(void);

int32_t allocate_REs_in_RB(PHY_VARS_eNB *phy_vars_eNB,
                           int32_t **txdataF,
                           uint32_t *jj,
                           uint32_t *jj2,
                           uint16_t re_offset,
                           uint32_t symbol_offset,
                           LTE_DL_eNB_HARQ_t *dlsch0_harq,
                           LTE_DL_eNB_HARQ_t *dlsch1_harq,
                           uint8_t pilots,
                           int16_t amp,
                           uint8_t precoder_index,
                           int16_t *qam_table_s0,
                           int16_t *qam_table_s1,
                           uint32_t *re_allocated,
                           uint8_t skip_dc,
                           uint8_t skip_half,
                           uint8_t lprime,
                           uint8_t mprime,
                           uint8_t Ns,
                           int *P1_SHIFT,
                           int *P2_SHIFT);


/** \fn int32_t dlsch_modulation(int32_t **txdataF,
    int16_t amp,
    uint32_t sub_frame_offset,
    LTE_DL_FRAME_PARMS *frame_parms,
    uint8_t num_pdcch_symbols,
    LTE_eNB_DLSCH_t *dlsch);

    \brief This function is the top-level routine for generation of the sub-frame signal (frequency-domain) for DLSCH.
    @param txdataF Table of pointers for frequency-domain TX signals
    @param amp Amplitude of signal
    @param sub_frame_offset Offset of this subframe in units of subframes (usually 0)
    @param frame_parms Pointer to frame descriptor
    @param num_pdcch_symbols Number of PDCCH symbols in this subframe
    @param dlsch0 Pointer to Transport Block 0 DLSCH descriptor for this allocation
    @param dlsch1 Pointer to Transport Block 0 DLSCH descriptor for this allocation
*/
int32_t dlsch_modulation(PHY_VARS_eNB *phy_vars_eNB,
                         int32_t **txdataF,
                         int16_t amp,
                         int frame,
                         uint32_t sub_frame_offset,
                         uint8_t num_pdcch_symbols,
                         LTE_eNB_DLSCH_t *dlsch0,
                         LTE_eNB_DLSCH_t *dlsch1);

int32_t dlsch_modulation_SIC(int32_t **sic_buffer,
                             uint32_t sub_frame_offset,
                             LTE_DL_FRAME_PARMS *frame_parms,
                             uint8_t num_pdcch_symbols,
                             LTE_eNB_DLSCH_t *dlsch0,
                             int G);
/*
  \brief This function is the top-level routine for generation of the sub-frame signal (frequency-domain) for MCH.
  @param txdataF Table of pointers for frequency-domain TX signals
  @param amp Amplitude of signal
  @param subframe_offset Offset of this subframe in units of subframes (usually 0)
  @param frame_parms Pointer to frame descriptor
  @param dlsch Pointer to DLSCH descriptor for this allocation
*/
int mch_modulation(int32_t **txdataF,
                   int16_t amp,
                   uint32_t subframe_offset,
                   LTE_DL_FRAME_PARMS *frame_parms,
                   LTE_eNB_DLSCH_t *dlsch);

/*
  \brief This function is the top-level routine for generation of the sub-frame signal (frequency-domain) for MCH.
  @param txdataF Table of pointers for frequency-domain TX signals
  @param amp Amplitude of signal
  @param subframe_offset Offset of this subframe in units of subframes (usually 0)
  @param frame_parms Pointer to frame descriptor
  @param dlsch Pointer to DLSCH descriptor for this allocation
*/
int mch_modulation_khz_1dot25(int32_t **txdataF,
                   int16_t amp,
                   uint32_t subframe_offset,
                   LTE_DL_FRAME_PARMS *frame_parms,
                   LTE_eNB_DLSCH_t *dlsch);

/** \brief Top-level generation function for eNB TX of MBSFN
    @param phy_vars_eNB Pointer to eNB variables
@param proc
    @param a Pointer to transport block

*/
void generate_mch_khz_1dot25(PHY_VARS_eNB *phy_vars_eNB,L1_rxtx_proc_t *proc,uint8_t *a);

/** \brief Top-level generation function for eNB TX of MBSFN
    @param phy_vars_eNB Pointer to eNB variables
@param proc
    @param a Pointer to transport block
*/
void generate_mch(PHY_VARS_eNB *phy_vars_eNB,L1_rxtx_proc_t *proc,uint8_t *a);

/** \brief This function generates the frequency-domain pilots (cell-specific downlink reference signals)
    @param phy_vars_eNB Pointer to eNB variables
    @param mcs MCS for MBSFN
    @param ndi new data indicator
    @param rvidx
*/
void fill_eNB_dlsch_MCH(PHY_VARS_eNB *phy_vars_eNB,int mcs,int ndi,int rvidx);

/** \brief This function generates the frequency-domain pilots (cell-specific downlink reference signals)
    for N subframes.
    @param phy_vars_eNB Pointer to eNB variables
    @param txdataF Table of pointers for frequency-domain TX signals
    @param amp Amplitude of signal
    @param N Number of sub-frames to generate
*/
void generate_pilots(PHY_VARS_eNB *phy_vars_eNB,
                     int32_t **txdataF,
                     int16_t amp,
                     uint16_t N);

/**
   \brief This function generates the frequency-domain pilots (cell-specific downlink reference signals) for one slot only
   @param phy_vars_eNB Pointer to eNB variables
   @param txdataF Table of pointers for frequency-domain TX signals
   @param amp Amplitude of signal
   @param slot index (0..19)
   @param first_pilot_only (0 no)
*/
int32_t generate_pilots_slot(PHY_VARS_eNB *phy_vars_eNB,
                             int32_t **txdataF,
                             int16_t amp,
                             uint16_t slot,
                             int first_pilot_only);

int32_t generate_mbsfn_pilot_khz_1dot25(PHY_VARS_eNB *phy_vars_eNB,
                             L1_rxtx_proc_t *proc,
                             int32_t **txdataF,
                             int16_t amp);


int32_t generate_mbsfn_pilot(PHY_VARS_eNB *phy_vars_eNB,
                             L1_rxtx_proc_t *proc,
                             int32_t **txdataF,
                             int16_t amp);

void generate_ue_spec_pilots(PHY_VARS_eNB *phy_vars_eNB,
                             uint8_t UE_id,
                             int32_t **txdataF,
                             int16_t amp,
                             uint16_t Ntti,
                             uint8_t beamforming_mode);

int32_t generate_pss(int32_t **txdataF,
                     int16_t amp,
                     LTE_DL_FRAME_PARMS *frame_parms,
                     uint16_t l,
                     uint16_t Ns);


int32_t generate_sss(int32_t **txdataF,
                     short amp,
                     LTE_DL_FRAME_PARMS *frame_parms,
                     unsigned short symbol,
                     unsigned short slot_offset);

int32_t generate_pbch(LTE_eNB_PBCH *eNB_pbch,
                      int32_t **txdataF,
                      int32_t amp,
                      LTE_DL_FRAME_PARMS *frame_parms,
                      uint8_t *pbch_pdu,
                      uint8_t frame_mod4);

int32_t generate_pbch_fembms(LTE_eNB_PBCH *eNB_pbch,
                      int32_t **txdataF,
                      int32_t amp,
                      LTE_DL_FRAME_PARMS *frame_parms,
                      uint8_t *pbch_pdu,
                      uint8_t frame_mod16);




/*! \brief DCI Encoding. This routine codes an arbitrary DCI PDU after appending the 8-bit 3GPP CRC.  It then applied sub-block interleaving and rate matching.
  \param a Pointer to DCI PDU (coded in bytes)
  \param A Length of DCI PDU in bits
  \param E Length of DCI PDU in coded bits
  \param e Pointer to sequence
  \param rnti RNTI for CRC scrambling*/
void dci_encoding(uint8_t *a,
                  uint8_t A,
                  uint16_t E,
                  uint8_t *e,
                  uint16_t rnti);

/*! \brief Top-level DCI entry point. This routine codes an set of DCI PDUs and performs PDCCH modulation, interleaving and mapping.
  \param num_pdcch_symbols
  \param num_dci  Number of DCI pdus to encode
  \param dci_alloc Allocation vectors for each DCI pdu
  \param n_rnti n_RNTI (see )
  \param amp Amplitude of QPSK symbols
  \param frame_parms Pointer to DL Frame parameter structure
  \param txdataF Pointer to tx signal buffers
  \param sub_frame_offset subframe offset in frame
  @returns Number of PDCCH symbols
*/

uint8_t generate_dci_top(uint8_t num_pdcch_symbols,
                         uint8_t num_dci,
                         DCI_ALLOC_t *dci_alloc,
                         uint32_t n_rnti,
                         int16_t amp,
                         LTE_DL_FRAME_PARMS *frame_parms,
                         int32_t **txdataF,
                         uint32_t sub_frame_offset);


void generate_mdci_top(PHY_VARS_eNB *eNB, int frame, int subframe, int16_t amp, int32_t **txdataF);

void generate_64qam_table(void);
void generate_16qam_table(void);








void ulsch_extract_rbs_single(int32_t **rxdataF,
                              int32_t **rxdataF_ext,
                              uint32_t first_rb,
                              uint32_t nb_rb,
                              uint8_t l,
                              uint8_t Ns,
                              LTE_DL_FRAME_PARMS *frame_parms);





void fill_dci_and_dlsch(PHY_VARS_eNB *eNB,
                        int frame,
                        int subframe,
                        L1_rxtx_proc_t *proc,
                        DCI_ALLOC_t *dci_alloc,
                        nfapi_dl_config_dci_dl_pdu *pdu);

void fill_mdci_and_dlsch(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc,mDCI_ALLOC_t *dci_alloc,nfapi_dl_config_mpdcch_pdu *pdu);

void fill_dci0(PHY_VARS_eNB *eNB,int frame,int subframe,L1_rxtx_proc_t *proc,DCI_ALLOC_t *dci_alloc,
               nfapi_hi_dci0_dci_pdu *pdu);

void fill_ulsch(PHY_VARS_eNB *eNB,int UE_id,nfapi_ul_config_ulsch_pdu *ulsch_pdu,int frame,int subframe);

void fill_mpdcch_dci0 (PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc, mDCI_ALLOC_t *dci_alloc, nfapi_hi_dci0_mpdcch_dci_pdu *pdu);

int generate_eNB_ulsch_params_from_rar(PHY_VARS_eNB *eNB,
                                       unsigned char *rar_pdu,
                                       uint32_t frame,
                                       unsigned char subframe);

int generate_eNB_ulsch_params_from_dci(PHY_VARS_eNB *PHY_vars_eNB,
                                       L1_rxtx_proc_t *proc,
                                       void *dci_pdu,
                                       rnti_t rnti,
                                       DCI_format_t dci_format,
                                       uint8_t UE_id,
                                       uint16_t si_rnti,
                                       uint16_t ra_rnti,
                                       uint16_t p_rnti,
                                       uint16_t cba_rnti,
                                       uint8_t use_srs);


void dump_ulsch(PHY_VARS_eNB *phy_vars_eNB,int frame, int subframe, uint8_t UE_id,int round);

void dump_ulsch_stats(FILE *fd,PHY_VARS_eNB *eNB,int frame);
void dump_uci_stats(FILE *fd,PHY_VARS_eNB *eNB,int frame);

void generate_pcfich(uint8_t num_pdcch_symbols,
                     int16_t amp,
                     LTE_DL_FRAME_PARMS *frame_parms,
                     int32_t **txdataF,
                     uint8_t subframe);

void rx_ulsch(PHY_VARS_eNB *eNB,
              L1_rxtx_proc_t *proc,
              uint8_t UE_id);

/*!
  \brief Decoding of PUSCH/ACK/RI/ACK from 36-212.
  @param phy_vars_eNB Pointer to eNB top-level descriptor
  @param proc Pointer to RXTX proc variables
  @param UE_id ID of UE transmitting this PUSCH
  @param control_only_flag Receive PUSCH with control information only
  @param Nbundled Nbundled parameter for ACK/NAK scrambling from 36-212/36-213
  @param llr8_flag If 1, indicate that the 8-bit turbo decoder should be used
  @returns 0 on success
*/
unsigned int  ulsch_decoding(PHY_VARS_eNB *phy_vars_eNB,
                             L1_rxtx_proc_t *proc,
                             uint8_t UE_id,
                             uint8_t control_only_flag,
                             uint8_t Nbundled,
                             uint8_t llr8_flag);

void generate_phich_top(PHY_VARS_eNB *phy_vars_eNB,
                        L1_rxtx_proc_t *proc,
                        int16_t amp);







void pdcch_interleaving(LTE_DL_FRAME_PARMS *frame_parms,int32_t **z, int32_t **wbar,uint8_t n_symbols_pdcch,uint8_t mi);

void pdcch_unscrambling(LTE_DL_FRAME_PARMS *frame_parms,
                        uint8_t subframe,
                        int8_t *llr,
                        uint32_t length);


void dlsch_scrambling(LTE_DL_FRAME_PARMS *frame_parms,
                      int mbsfn_flag,
                      LTE_eNB_DLSCH_t *dlsch,
                      int hard_pid,
                      int G,
                      uint8_t q,
                      uint16_t frame,
                      uint8_t Ns);

uint32_t calc_pucch_1x_interference(PHY_VARS_eNB *eNB,
                 int     frame,
                 uint8_t subframe,
                 uint8_t shortened_format
);

uint32_t rx_pucch(PHY_VARS_eNB *phy_vars_eNB,
                  PUCCH_FMT_t fmt,
                  uint8_t UE_id,
                  uint16_t n1_pucch,
                  uint16_t n2_pucch,
                  uint8_t shortened_format,
                  uint8_t *payload,
                  int     frame,
                  uint8_t subframe,
                  uint8_t pucch1_thres,
                  int     br_flag
                 );

/*!
  \brief Process PRACH waveform
  @param phy_vars_eNB Pointer to eNB top-level descriptor. If NULL, then this is an RRU
  @param ru Pointer to RU top-level descriptor. If NULL, then this is an eNB and we make use of the RU_list
  @param max_preamble most likely preamble
  @param max_preamble_energy Estimated Energy of most likely preamble
  @param max_preamble_delay Estimated Delay of most likely preamble
  @param avg_preamble_energy
  @param Nf System frame number
  @param tdd_mapindex Index of PRACH resource in Table 5.7.1-4 (TDD)
  @param br_flag indicator to act on eMTC PRACH
  @returns 0 on success

*/
void rx_prach(PHY_VARS_eNB *phy_vars_eNB,RU_t *ru,
              uint16_t *max_preamble,
              uint16_t *max_preamble_energy,
              uint16_t *max_preamble_delay,
              uint16_t *avg_preamble_energy,
              uint16_t Nf, uint8_t tdd_mapindex,
              uint8_t br_flag
             );


void init_unscrambling_lut(void);



uint8_t is_not_pilot(uint8_t pilots, uint8_t re, uint8_t nushift, uint8_t use2ndpilots);

uint8_t is_not_UEspecRS(int8_t lprime, uint8_t re, uint8_t nushift, uint8_t Ncp, uint8_t beamforming_mode);



double computeRhoA_eNB(uint8_t pa,
                       LTE_eNB_DLSCH_t *dlsch_eNB,
                       int dl_power_off,
                       uint8_t n_antenna_port);

double computeRhoB_eNB(uint8_t pa,
                       uint8_t pb,
                       uint8_t n_antenna_port,
                       LTE_eNB_DLSCH_t *dlsch_eNB,
                       int dl_power_off);



void conv_eMTC_rballoc(uint16_t resource_block_coding,
                       uint32_t N_RB_DL,
                       uint32_t *rb_alloc);

int find_dlsch(uint16_t rnti, PHY_VARS_eNB *eNB,find_type_t type);

int find_ulsch(uint16_t rnti, PHY_VARS_eNB *eNB,find_type_t type);

int find_uci(uint16_t rnti, int frame, int subframe, PHY_VARS_eNB *eNB,find_type_t type);

static inline  uint32_t lte_gold_generic(uint32_t *x1, uint32_t *x2, uint8_t reset)
{
  int32_t n;

  // 3GPP 3x.211
  // Nc = 1600
  // c(n)     = [x1(n+Nc) + x2(n+Nc)]mod2
  // x1(n+31) = [x1(n+3)                     + x1(n)]mod2
  // x2(n+31) = [x2(n+3) + x2(n+2) + x2(n+1) + x2(n)]mod2
  if (reset)
  {
      // Init value for x1: x1(0) = 1, x1(n) = 0, n=1,2,...,30
      // x1(31) = [x1(3) + x1(0)]mod2 = 1
      *x1 = 1 + (1U<<31);
      // Init value for x2: cinit = sum_{i=0}^30 x2*2^i
      // x2(31) = [x2(3)    + x2(2)    + x2(1)    + x2(0)]mod2
      //        =  (*x2>>3) ^ (*x2>>2) + (*x2>>1) + *x2
      *x2 = *x2 ^ ((*x2 ^ (*x2>>1) ^ (*x2>>2) ^ (*x2>>3))<<31);

      // x1 and x2 contain bits n = 0,1,...,31

      // Nc = 1600 bits are skipped at the beginning
      // i.e., 1600 / 32 = 50 32bit words

      for (n = 1; n < 50; n++)
      {
          // Compute x1(0),...,x1(27)
          *x1 = (*x1>>1) ^ (*x1>>4);
          // Compute x1(28),..,x1(31) and xor
          *x1 = *x1 ^ (*x1<<31) ^ (*x1<<28);
          // Compute x2(0),...,x2(27)
          *x2 = (*x2>>1) ^ (*x2>>2) ^ (*x2>>3) ^ (*x2>>4);
          // Compute x2(28),..,x2(31) and xor
          *x2 = *x2 ^ (*x2<<31) ^ (*x2<<30) ^ (*x2<<29) ^ (*x2<<28);
      }
  }

  *x1 = (*x1>>1) ^ (*x1>>4);
  *x1 = *x1 ^ (*x1<<31) ^ (*x1<<28);
  *x2 = (*x2>>1) ^ (*x2>>2) ^ (*x2>>3) ^ (*x2>>4);
  *x2 = *x2 ^ (*x2<<31) ^ (*x2<<30) ^ (*x2<<29) ^ (*x2<<28);

  // c(n) = [x1(n+Nc) + x2(n+Nc)]mod2
  return(*x1^*x2);
}


/**@}*/
#endif
