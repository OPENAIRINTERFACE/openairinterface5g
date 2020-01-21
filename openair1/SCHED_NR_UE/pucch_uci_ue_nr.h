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
* FILENAME    :  pucch_uci_ue_nr.h
*
* MODULE      :  Packed Uplink Control Channel aka PUCCH
*                PUCCH is used to trasnmit Uplink Control Information UCI
*                which is composed of:
*                - SR Scheduling Request
*                - HARQ ACK/NACK
*                - CSI Channel State Information
*
* DESCRIPTION :  functions related to PUCCH management
*                TS 38.213 9  UE procedure for reporting control information
*
************************************************************************/

#ifndef PUCCH_UCI_UE_NR_H
#define PUCCH_UCI_UE_NR_H

/************** INCLUDE *******************************************/

#include "PHY/defs_nr_UE.h"

#ifdef DEFINE_VARIABLES_PUCCH_UE_NR_H
#define EXTERN
#define INIT_VARIABLES_PUCCH_UE_NR_H
#else
#define EXTERN extern
#undef INIT_VARIABLES_PUCCH_UE_NR_H
#endif

/************** DEFINE ********************************************/

#define BITS_PER_SYMBOL_BPSK  (1)     /* 1 bit per symbol for bpsk modulation */
#define BITS_PER_SYMBOL_QPSK  (2)     /* 2 bits per symbol for bpsk modulation */

/************** VARIABLES *****************************************/

/* TS 36.213 Table 9.2.1-1: PUCCH resource sets before dedicated PUCCH resource configuration */
const initial_pucch_resource_t initial_pucch_resource[NB_INITIAL_PUCCH_RESOURCE]
#ifdef INIT_VARIABLES_PUCCH_UE_NR_H
=
{
/*              format           first symbol     Number of symbols        PRB offset    nb index for       set of initial CS */
/*  0  */ {  pucch_format0_nr,      12,                  2,                   0,            2,       {    0,   3,    0,    0  }   },
/*  1  */ {  pucch_format0_nr,      12,                  2,                   0,            3,       {    0,   4,    8,    0  }   },
/*  2  */ {  pucch_format0_nr,      12,                  2,                   3,            3,       {    0,   4,    8,    0  }   },
/*  3  */ {  pucch_format1_nr,      10,                  4,                   0,            2,       {    0,   6,    0,    0  }   },
/*  4  */ {  pucch_format1_nr,      10,                  4,                   0,            4,       {    0,   3,    6,    9  }   },
/*  5  */ {  pucch_format1_nr,      10,                  4,                   2,            4,       {    0,   3,    6,    9  }   },
/*  6  */ {  pucch_format1_nr,      10,                  4,                   4,            4,       {    0,   3,    6,    9  }   },
/*  7  */ {  pucch_format1_nr,       4,                 10,                   0,            2,       {    0,   6,    0,    0  }   },
/*  8  */ {  pucch_format1_nr,       4,                 10,                   0,            4,       {    0,   3,    6,    9  }   },
/*  9  */ {  pucch_format1_nr,       4,                 10,                   2,            4,       {    0,   3,    6,    9  }   },
/* 10  */ {  pucch_format1_nr,       4,                 10,                   4,            4,       {    0,   3,    6,    9  }   },
/* 11  */ {  pucch_format1_nr,       0,                 14,                   0,            2,       {    0,   6,    0,    0  }   },
/* 12  */ {  pucch_format1_nr,       0,                 14,                   0,            4,       {    0,   3,    6,    9  }   },
/* 13  */ {  pucch_format1_nr,       0,                 14,                   2,            4,       {    0,   3,    6,    9  }   },
/* 14  */ {  pucch_format1_nr,       0,                 14,                   4,            4,       {    0,   3,    6,    9  }   },
/* 15  */ {  pucch_format1_nr,       0,                 14,                   0,            4,       {    0,   3,    6,    9  }   },
}
#endif
;

/* TS 36.213 Table 9.2.3-3: Mapping of values for one HARQ-ACK bit to sequences */
const int sequence_cyclic_shift_1_harq_ack_bit[2]
#ifdef INIT_VARIABLES_PUCCH_UE_NR_H
/*        HARQ-ACK Value        0    1 */
/* Sequence cyclic shift */ = { 0,   6 }
#endif
;

/* TS 38.213 Table 9.2.3-4: Mapping of values for two HARQ-ACK bits to sequences */
const int sequence_cyclic_shift_2_harq_ack_bits[4]
#ifdef INIT_VARIABLES_PUCCH_UE_NR_H
/*        HARQ-ACK Value       (0,0)  (0,1)  (1,0)  (1,1) */
/* Sequence cyclic shift */ = {   0,     3,     9,     6 }
#endif
;

/* TS 36.213 Table 9.2.5-1: Mapping of values for one HARQ-ACK bit and positive SR to sequences */
const int sequence_cyclic_shift_1_harq_ack_bit_positive_sr[2]
#ifdef INIT_VARIABLES_PUCCH_UE_NR_H
/*        HARQ-ACK Value        0    1 */
/* Sequence cyclic shift */ = { 3,   9 }
#endif
;

/* TS 36.213 Table 9.2.5-2: Mapping of values for two HARQ-ACK bits and positive SR to sequences */
const int sequence_cyclic_shift_2_harq_ack_bits_positive_sr[4]
#ifdef INIT_VARIABLES_PUCCH_UE_NR_H
/*        HARQ-ACK Value      (0,0)  (0,1)   (1,0)  (1,1) */
/* Sequence cyclic shift */ = {  1,     4,     10,     7 }
#endif
;

/* TS 36.213 Table 9.2.5.2-1: Code rate  corresponding to higher layer parameter PUCCH-F2-maximum-coderate, */
/* or PUCCH-F3-maximum-coderate, or PUCCH-F4-maximum-coderate */
/* add one additional element set to 0 for parsing the array until this end */
/* stored values are code rates * 100 */
const int code_rate_r_time_100[8]
#ifdef INIT_VARIABLES_PUCCH_UE_NR_H
= { (0.08 * 100), (0.15 * 100), (0.25*100), (0.35*100), (0.45*100), (0.60*100), (0.80*100), 0 }
#endif
;

#define  NB_SYMBOL_MINUS_FOUR             (11)
#define  I_PUCCH_NO_ADDITIONAL_DMRS        (0)
#define  I_PUCCH_ADDITIONAL_DMRS           (1)
#define  I_PUCCH_NO_HOPPING                (0)
#define  I_PUCCH_HOPING                    (1)

/* TS 38.211 Table 6.4.1.3.3.2-1: DM-RS positions for PUCCH format 3 and 4 */
const int nb_symbols_excluding_dmrs[NB_SYMBOL_MINUS_FOUR][2][2]
#ifdef INIT_VARIABLES_PUCCH_UE_NR_H
= {
/*                     No additional DMRS            Additional DMRS   */
/* PUCCH length      No hopping   hopping         No hopping   hopping */
/* index                  0          1                 0          1    */
/*    4     */    {{      3    ,     2   }   ,  {      3     ,    2    }},
/*    5     */    {{      3    ,     3   }   ,  {      3     ,    3    }},
/*    6     */    {{      4    ,     4   }   ,  {      4     ,    4    }},
/*    7     */    {{      5    ,     5   }   ,  {      5     ,    5    }},
/*    8     */    {{      6    ,     6   }   ,  {      6     ,    6    }},
/*    9     */    {{      7    ,     7   }   ,  {      7     ,    7    }},
/*   10     */    {{      8    ,     8   }   ,  {      6     ,    6    }},
/*   11     */    {{      9    ,     9   }   ,  {      7     ,    7    }},
/*   12     */    {{     10    ,    10   }   ,  {      8     ,    8    }},
/*   13     */    {{     11    ,    11   }   ,  {      9     ,    9    }},
/*   14     */    {{     12    ,    12   }   ,  {     10     ,   10    }},
}
#endif
;

/* TS 38.213 9.2.5.2 UE procedure for multiplexing HARQ-ACK/SR and CSI in a PUCCH */
/* this is a counter of number of pucch format 4 per subframe */
int nb_pucch_format_4_in_subframes[LTE_NUMBER_OF_SUBFRAMES_PER_FRAME]
#ifdef INIT_VARIABLES_PUCCH_UE_NR_H
= { 0 }
#endif
;

/*************** FUNCTIONS ****************************************/

bool pucch_procedures_ue_nr(PHY_VARS_NR_UE *ue, uint8_t gNB_id, UE_nr_rxtx_proc_t *proc, bool reset_harq);

/** \brief This function return number of downlink acknowledgement and its bitmap
    @param ue context
    @param gNB_id identity
    @param slots for rx and tx
    @param o_ACK HARQ-ACK information bits
    @param n_HARQ_ACK use for obtaining a PUCCH transmission power
    @param do_reset reset downlink HARQ context
    @returns number of bits of o_ACK */

uint8_t get_downlink_ack(PHY_VARS_NR_UE *ue, uint8_t gNB_id,  UE_nr_rxtx_proc_t *proc, uint32_t *o_ACK,
                         int *n_HARQ_ACK, bool do_reset);

/** \brief This function selects a pucch resource
    @param ue context
    @param gNB_id identity
    @param uci size number of uci bits
    @param pucch_resource_indicator is from downlink DCI
    @param initial_pucch_id  pucch resource id for initial phase
    @param resource_set_id   pucch resource set if any
    @param resource_id       pucch resource id if any
    @returns TRUE  a pucch resource has been found FALSE no valid pucch resource */

boolean_t select_pucch_resource(PHY_VARS_NR_UE *ue, uint8_t gNB_id, int uci_size, int pucch_resource_indicator, 
                                int *initial_pucch_id, int *resource_set_id, int *resource_id, NR_UE_HARQ_STATUS_t *harq_status);

/** \brief This function select a pucch resource set
    @param ue context
    @param gNB_id identity
    @param uci size number of uci bits
    @returns number of the pucch resource set */

int find_pucch_resource_set(PHY_VARS_NR_UE *ue, uint8_t gNB_id, int uci_size);

/** \brief This function check pucch format
    @param ue context
    @param gNB_id identity
    @param format_pucch pucch format
    @param nb_symbols_for_tx number of symbols for pucch transmission
    @param uci size number of uci bits
    @returns TRUE pucch format matched uci size and constraints, FALSE invalid pucch format */

boolean_t check_pucch_format(PHY_VARS_NR_UE *ue, uint8_t gNB_id, pucch_format_nr_t format_pucch, int nb_symbols_for_tx, 
                             int uci_size);

/** \brief This function selects a pucch resource
    @param ue context
    @param gNB_id identity
    @param slots for rx and tx
    @returns TRUE  a scheduling request is triggered */
                             
int trigger_periodic_scheduling_request(PHY_VARS_NR_UE *ue, uint8_t gNB_id, UE_nr_rxtx_proc_t *proc);

/** \brief This function reads current CSI
    @param ue context
    @param gNB_id identity
    @param csi_payload is updated with CSI
    @returns number of bits of CSI */

int get_csi_nr(PHY_VARS_NR_UE *ue, uint8_t gNB_id, uint32_t *csi_payload);

/** \brief This dummy function sets current CSI for simulation
    @param csi_status
    @param csi_payload is updated with CSI
    @returns none */

void set_csi_nr(int csi_status, uint32_t csi_payload);

#endif /* PUCCH_UCI_UE_NR_H */
