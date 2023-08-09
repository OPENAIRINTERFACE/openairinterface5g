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

/**********************************************************************
*
* FILENAME    :  ss_pbch_nr.h
*
* MODULE      : this file contains define only
*
* DESCRIPTION : define elements related to SS/PBCH block ie synchronisation (pss/sss) and pbch
*
*               see TS 38.211  7.4.2 Synchronisation Signals
*               see TS 38.213  4 Synchronisation procedures
*
************************************************************************/

#ifndef SS_PBCH_NR_H
#define SS_PBCH_NR_H

/************** DEFINE ********************************************/

#define VOID_PARAMETER                (void)   /* avoid a compiler warning for unused parameters of function */

/* PSS parameters */
#define  NUMBER_PSS_SEQUENCE          (3)
#define  NUMBER_PSS_SEQUENCE_SL       (2)
#define  PSS_SSS_SUB_CARRIER_START    (56)
#define  PSS_SSS_SUB_CARRIER_START_SL (2)
#define  INVALID_PSS_SEQUENCE         (NUMBER_PSS_SEQUENCE)
#define  LENGTH_PSS_NR                (127)
#define  N_SC_RB                      (12)     /* Resource block size in frequency domain expressed as a number if subcarriers */
#define  SCALING_PSS_NR               (3)
#define  SCALING_CE_PSS_NR            (13)     /* scaling channel estimation based on ps */
#define  PSS_IFFT_SIZE                (256)

#define  PSS_SC_START_NR              (52)     /* see from TS 38.211 table 7.4.3.1-1: Resources within an SS/PBCH block for PSS... */

/* define ofdm symbol offset in the SS/PBCH block of NR synchronisation */
#ifdef NR_UNIT_TEST
#define OFFSET_SS_PBCH                (0)
#else
#define OFFSET_SS_PBCH                (4)
#endif

#define  PSS_SYMBOL_NB                ((0) + OFFSET_SS_PBCH)   /* symbol numbers for each element */
#define  PBCH_SYMBOL_NB               ((1) + OFFSET_SS_PBCH)
#define  SSS_SYMBOL_NB                ((2) + OFFSET_SS_PBCH)
#define  PBCH_LAST_SYMBOL_NB          ((3) + OFFSET_SS_PBCH)


#define  OFFSET_SS_PSBCH              -1
#define  PSS0_SL_SYMBOL_NB            ((1) + OFFSET_SS_PSBCH)
#define  PSS1_SL_SYMBOL_NB            ((2) + OFFSET_SS_PSBCH)
#define  SSS0_SL_SYMBOL_NB            ((3) + OFFSET_SS_PSBCH)
#define  SSS1_SL_SYMBOL_NB            ((4) + OFFSET_SS_PSBCH)

/* SS/PBCH parameters */
#define  N_RB_SS_PBCH_BLOCK           (20)
#define  NB_SYMBOLS_PBCH              (3)
#define  NR_N_SYMBOLS_SSB             (4)

#define IQ_SIZE sizeof(c16_t) /* I and Q are alternatively stored into buffers */
#define  N_SYMB_SLOT                  (14)

/* SS/PBCH parameters :  see from TS 38.211 table 7.4.3.1-1: Resources within an SS/PBCH block for PSS... */
#define  DMRS_PBCH_PER_RB             (N_SC_RB >> 4)               /* at 0+v, 4+v, 8+v for a resource block with v = NcellID modulo 4 */
#define  DMRS_END_FIRST_PART          (44)
#define  DMRS_START_SECOND_PART       (192)
#define  DMRS_END_SECOND_PART         (236)
#define  DMRS_PBCH_NUMBER             (NB_SYMBOLS_PBCH*(N_RB_SS_PBCH_BLOCK * DMRS_PBCH_PER_RB))    /* there are both PBCH and SSS/(Set to 0) at the second OFDM symbol of SS/PBCH so size is increased */

/* see TS 38211 7.4.1.4 Demodulation reference signals for PBCH */
#define  DMRS_PBCH_I_SSB              (8)         /* maximum index value for SSB/PBCH which can have alength of L=4 or L=8 */
#define  DMRS_PBCH_N_HF               (2)         /* half frame indication - 0 for first part of frame and 1 for second part of frame */

#endif /* SS_PBCH_NR_H */


