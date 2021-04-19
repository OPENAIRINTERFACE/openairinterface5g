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
#include "openair2/LAYER2/NR_MAC_UE/mac_proto.h"
#include "openair2/LAYER2/NR_MAC_UE/mac_defs.h"
#include "RRC/NR_UE/rrc_proto.h"

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

#define  NB_SYMBOL_MINUS_FOUR             (11)
#define  I_PUCCH_NO_ADDITIONAL_DMRS        (0)
#define  I_PUCCH_ADDITIONAL_DMRS           (1)
#define  I_PUCCH_NO_HOPPING                (0)
#define  I_PUCCH_HOPING                    (1)


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

boolean_t select_pucch_resource(PHY_VARS_NR_UE *ue, NR_UE_MAC_INST_t *mac, uint8_t gNB_id, int uci_size, int pucch_resource_indicator, 
                                int *initial_pucch_id, int *resource_set_id, int *resource_id, NR_UE_HARQ_STATUS_t *harq_status);

/** \brief This function select a pucch resource set
    @param ue context
    @param gNB_id identity
    @param uci size number of uci bits
    @returns number of the pucch resource set */

int find_pucch_resource_set(NR_UE_MAC_INST_t *mac, uint8_t gNB_id, int uci_size);

/** \brief This function check pucch format
    @param ue context
    @param gNB_id identity
    @param format_pucch pucch format
    @param nb_symbols_for_tx number of symbols for pucch transmission
    @param uci size number of uci bits
    @returns TRUE pucch format matched uci size and constraints, FALSE invalid pucch format */

boolean_t check_pucch_format(NR_UE_MAC_INST_t *mac, uint8_t gNB_id, pucch_format_nr_t format_pucch, int nb_symbols_for_tx, 
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

int get_csi_nr(NR_UE_MAC_INST_t *mac, PHY_VARS_NR_UE *ue, uint8_t gNB_id, uint32_t *csi_payload);

/** \brief This dummy function sets current CSI for simulation
    @param csi_status
    @param csi_payload is updated with CSI
    @returns none */
    
uint16_t get_nr_csi_bitlen(NR_UE_MAC_INST_t *mac);

void set_csi_nr(int csi_status, uint32_t csi_payload);

uint8_t get_nb_symbols_pucch(NR_PUCCH_Resource_t *pucch_resource, pucch_format_nr_t format_type);

uint16_t get_starting_symb_idx(NR_PUCCH_Resource_t *pucch_resource, pucch_format_nr_t format_type);

int get_ics_pucch(NR_PUCCH_Resource_t *pucch_resource, pucch_format_nr_t format_type);

NR_PUCCH_Resource_t *select_resource_by_id(int resource_id, NR_PUCCH_Config_t *pucch_config);
#endif /* PUCCH_UCI_UE_NR_H */
