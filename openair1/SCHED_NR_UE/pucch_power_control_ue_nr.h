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

/************************************************************************
*
* MODULE      :  PUCCH power control for UE NR
*
* DESCRIPTION :  functions related to PUCCH Transmit power
*                TS 38.213 7.2.1 UE behaviour
*
**************************************************************************/

#ifndef PUCCH_POWER_CONTROL_H
#define PUCCH_POWER_CONTROL_H

/*************** INCLUDE *******************************************/

/*************** DEFINE ********************************************/

#define PUCCH_POWER_DEFAULT         (0)       /* in dBm */

/*************** VARIABLES *****************************************/

/*************** FUNCTIONS *****************************************/


/** \brief This function returns pucch power level in dBm
    @param ue context
    @param gNB_id identity
    @param slots for rx and tx
    @param pucch_format_nr_t pucch format
    @param nb_of_prbs number of prb allocated to pucch
    @param N_sc_ctrl_RB subcarrier control rb related to current pucch format
    @param N_symb_PUCCH number of pucch symbols excluding those reserved for dmrs
    @param O_UCI number of bits for UCI Uplink Control Information
    @param O_SR number of bits for SR scheduling Request
    @param int O_UCI number of  bits for CSI Channel State Information
    @param O_ACK number of bits for HARQ-ACK
    @param O_CRC number of bits for CRC
    @param n_HARQ_ACK use for obtaining a PUCCH transmission power
    @returns pucch power level in dBm */

int16_t get_pucch_tx_power_ue(PHY_VARS_NR_UE *ue, uint8_t gNB_id, UE_nr_rxtx_proc_t *proc, pucch_format_nr_t pucch_format,
                              int nb_of_prbs, int N_sc_ctrl_RB, int N_symb_PUCCH, int O_UCI, int O_SR, int O_CSI, int O_ACK,
                              int O_CRC, int n_HARQ_ACK);

#endif /* PUCCH_POWER_CONTROL_H */
