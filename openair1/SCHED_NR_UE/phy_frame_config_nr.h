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

/***********************************************************************
*
* FILENAME    :  phy_frame_configuration_nr.h
*
* DESCRIPTION :  functions related to FDD/TDD configuration  for NR
*                see TS 38.213 11.1 Slot configuration
*                and TS 38.331 for RRC configuration
*
************************************************************************/

#ifndef PHY_FRAME_CONFIG_NR_UE_H
#define PHY_FRAME_CONFIG_NR_UE_H

/************** DEFINE ********************************************/

/*************** FUNCTIONS *****************************************/


/** \brief This function adds a slot configuration to current dedicated configuration for nr
 *  @param frame_parms NR DL Frame parameters
 *  @param slotIndex
 *  @param nrofDownlinkSymbols
 *  @param nrofUplinkSymbols
    @returns none */

void add_tdd_dedicated_configuration_nr(NR_DL_FRAME_PARMS *frame_parms, int slotIndex,
                                        int nrofDownlinkSymbols, int nrofUplinkSymbols);

/** \brief This function processes tdd dedicated configuration for nr
 *  @param frame_parms nr frame parameters
 *  @param dl_UL_TransmissionPeriodicity periodicity
 *  @param nrofDownlinkSlots number of downlink slots
 *  @param nrofDownlinkSymbols number of downlink symbols
 *  @param nrofUplinkSlots number of uplink slots
 *  @param nrofUplinkSymbols number of uplink symbols
    @returns 0 if tdd dedicated configuration has been properly set or -1 on error with message */

int set_tdd_configuration_dedicated_nr(NR_DL_FRAME_PARMS *frame_parms);

/** \brief This function checks nr slot direction : downlink or uplink
 *  @param frame_parms NR DL Frame parameters
 *  @param nr_frame : frame number
 *  @param nr_slot  : slot number
    @returns int : downlink or uplink */

int slot_select_nr(NR_DL_FRAME_PARMS *frame_parms, int nr_frame, int nr_slot);

/** \brief This function checks nr UE slot direction : downlink or uplink
 *  @param cfg      : FAPI Config Request
 *  @param nr_frame : frame number
 *  @param nr_slot  : slot number
    @returns int : downlink, uplink or mixed slot type */

int nr_ue_slot_select(fapi_nr_config_request_t *cfg, int nr_frame, int nr_slot);

/** \brief This function frees tdd configuration for nr
 *  @param frame_parms NR DL Frame parameters
    @returns none */

void free_tdd_configuration_nr(NR_DL_FRAME_PARMS *frame_parms);

/** \brief This function frees tdd dedicated configuration for nr
 *  @param frame_parms NR DL Frame parameters
    @returns none */

void free_tdd_configuration_dedicated_nr(NR_DL_FRAME_PARMS *frame_parms);

#endif  /* PHY_FRAME_CONFIG_NR_H */

