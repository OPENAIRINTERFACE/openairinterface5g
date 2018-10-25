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
* FILENAME    :  phy_frame_configuration_nr.c
*
* DESCRIPTION :  functions related to FDD/TDD configuration for NR
*                see TS 38.213 11.1 Slot configuration
*                and TS 38.331 for RRC configuration
*
************************************************************************/

#include "SCHED_NR_UE/defs.h"
#include "PHY/defs_nr_UE.h"
#include "SCHED_NR_UE/phy_frame_config_nr.h"

/*******************************************************************
*
* NAME :         set_tdd_configuration
*
* PARAMETERS :   pointer to frame configuration
*
* OUTPUT:        table of uplink symbol for each slot for 2 frames
*
* RETURN :       0 if tdd has been properly configurated
*                -1 tdd configuration can not be done
*
* DESCRIPTION :  generate bit map for uplink symbol for each slot for several frames
*                see TS 38.213 11.1 Slot configuration
*
*********************************************************************/

int set_tdd_config_nr(NR_DL_FRAME_PARMS *frame_parms, int dl_UL_TransmissionPeriodicity,
                       int nrofDownlinkSlots, int nrofDownlinkSymbols,
                       int nrofUplinkSlots,   int nrofUplinkSymbols)
{
  TDD_UL_DL_configCommon_t  *p_tdd_ul_dl_configuration;
  int slot_number = 0;
  int nb_slots_to_set = TDD_CONFIG_NB_FRAMES*(frame_parms->ttis_per_subframe * LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);

  /* allocate buffer for configuration structure */
  p_tdd_ul_dl_configuration = calloc( 1, sizeof(TDD_UL_DL_configCommon_t));

  if (p_tdd_ul_dl_configuration == NULL) {
    printf("Error test_frame_configuration: memory allocation problem \n");
    assert(0);
  }
  else {
    frame_parms->frame_type = TDD;
  }

  p_tdd_ul_dl_configuration->dl_UL_TransmissionPeriodicity = dl_UL_TransmissionPeriodicity;
  p_tdd_ul_dl_configuration->nrofDownlinkSlots   = nrofDownlinkSlots;
  p_tdd_ul_dl_configuration->nrofDownlinkSymbols = nrofDownlinkSymbols;
  p_tdd_ul_dl_configuration->nrofUplinkSlots     = nrofUplinkSlots;
  p_tdd_ul_dl_configuration->nrofUplinkSymbols   = nrofUplinkSymbols;

  frame_parms->p_tdd_UL_DL_Configuration = p_tdd_ul_dl_configuration;

  int nb_periods_per_frame = (FRAME_DURATION_MICRO_SEC/dl_UL_TransmissionPeriodicity);

  int nb_slots_per_period = (frame_parms->ttis_per_subframe * LTE_NUMBER_OF_SUBFRAMES_PER_FRAME)/nb_periods_per_frame;

  if (nb_slots_per_period != (nrofDownlinkSlots + nrofUplinkSlots)) {
    LOG_E(PHY,"set_tdd_configuration_nr: given period is inconsistent with current tdd configuration \n");
    return (-1);
  }

  while(slot_number != nb_slots_to_set) {

    for (int number_of_slot = 0; number_of_slot < nrofDownlinkSlots; number_of_slot++) {
      frame_parms->tdd_uplink_nr[slot_number] = NR_TDD_DOWNLINK_SLOT;
      slot_number++;
    }

    if (p_tdd_ul_dl_configuration->nrofDownlinkSymbols != 0) {
      LOG_E(PHY,"set_tdd_configuration_nr: downlink symbol for slot is not supported for tdd configuration \n");
      return (-1);
    }

    for (int number_of_slot = 0; number_of_slot < nrofUplinkSlots; number_of_slot++) {
      frame_parms->tdd_uplink_nr[slot_number] = NR_TDD_UPLINK_SLOT;
      slot_number++;
    }

    if (p_tdd_ul_dl_configuration->nrofUplinkSymbols != 0) {
      LOG_E(PHY,"set_tdd_configuration_nr: uplink symbol for slot is not supported for tdd configuration \n");
      return (-1);
    }
  }

  if (frame_parms->p_tdd_UL_DL_ConfigurationCommon2 != NULL) {
    LOG_E(PHY,"set_tdd_configuration_nr: additionnal tdd configuration 2 is not supported for tdd configuration \n");
    return (-1);
  }

  return (0);
}

/*******************************************************************
*
* NAME :         add_tdd_dedicated_configuration_nr
*
* PARAMETERS :   pointer to frame configuration
*
* OUTPUT:        table of uplink symbol for each slot for several frames
*
* RETURN :       0 if tdd has been properly configurated
*                -1 tdd configuration can not be done
*
* DESCRIPTION :  generate bit map for uplink symbol for each slot for several frames
*                see TS 38.213 11.1 Slot configuration
*
*********************************************************************/

void add_tdd_dedicated_configuration_nr(NR_DL_FRAME_PARMS *frame_parms, int slotIndex, int nrofDownlinkSymbols, int nrofUplinkSymbols)
{
  TDD_UL_DL_SlotConfig_t *p_TDD_UL_DL_ConfigDedicated = frame_parms->p_TDD_UL_DL_ConfigDedicated;
  TDD_UL_DL_SlotConfig_t *p_previous_TDD_UL_DL_ConfigDedicated;
  int next = 0;

  while (p_TDD_UL_DL_ConfigDedicated != NULL) {
    p_previous_TDD_UL_DL_ConfigDedicated = p_TDD_UL_DL_ConfigDedicated;
    p_TDD_UL_DL_ConfigDedicated = (TDD_UL_DL_SlotConfig_t *)(p_TDD_UL_DL_ConfigDedicated->p_next_TDD_UL_DL_SlotConfig);
    next = 1;
  }

  p_TDD_UL_DL_ConfigDedicated = calloc( 1, sizeof(TDD_UL_DL_SlotConfig_t));
  //printf("allocate pt %p \n", p_TDD_UL_DL_ConfigDedicated);
  if (p_TDD_UL_DL_ConfigDedicated == NULL) {
    printf("Error test_frame_configuration: memory allocation problem \n");
    assert(0);
  }

  if (next == 0) {
    frame_parms->p_TDD_UL_DL_ConfigDedicated = p_TDD_UL_DL_ConfigDedicated;
  }
  else {
    p_previous_TDD_UL_DL_ConfigDedicated->p_next_TDD_UL_DL_SlotConfig = (struct TDD_UL_DL_SlotConfig_t *)p_TDD_UL_DL_ConfigDedicated;
  }

  p_TDD_UL_DL_ConfigDedicated->slotIndex = slotIndex;
  p_TDD_UL_DL_ConfigDedicated->nrofDownlinkSymbols = nrofDownlinkSymbols;
  p_TDD_UL_DL_ConfigDedicated->nrofUplinkSymbols = nrofUplinkSymbols;
}

/*******************************************************************
*
* NAME :         set_tdd_configuration_dedicated_nr
*
* PARAMETERS :   pointer to frame configuration
*
* OUTPUT:        table of uplink symbol for each slot for several frames
*
* RETURN :       0 if tdd has been properly configurated
*                -1 tdd configuration can not be done
*
* DESCRIPTION :  generate bit map for uplink symbol for each slot for several frames
*                see TS 38.213 11.1 Slot configuration
*
*********************************************************************/

int set_tdd_configuration_dedicated_nr(NR_DL_FRAME_PARMS *frame_parms)
{
  TDD_UL_DL_SlotConfig_t *p_current_TDD_UL_DL_SlotConfig;

  p_current_TDD_UL_DL_SlotConfig = frame_parms->p_TDD_UL_DL_ConfigDedicated;

  NR_TST_PHY_PRINTF("\nSet tdd dedicated configuration\n ");

  while(p_current_TDD_UL_DL_SlotConfig != NULL) {
    int slot_index = p_current_TDD_UL_DL_SlotConfig->slotIndex;
    if (slot_index < TDD_CONFIG_NB_FRAMES*(frame_parms->ttis_per_subframe * LTE_NUMBER_OF_SUBFRAMES_PER_FRAME)) {
      if (p_current_TDD_UL_DL_SlotConfig->nrofDownlinkSymbols != 0) {
        if (p_current_TDD_UL_DL_SlotConfig->nrofDownlinkSymbols == NR_TDD_SET_ALL_SYMBOLS) {
          if (p_current_TDD_UL_DL_SlotConfig->nrofUplinkSymbols == 0) {
            frame_parms->tdd_uplink_nr[slot_index] = NR_TDD_DOWNLINK_SLOT;
            NR_TST_PHY_PRINTF(" DL[%d] ", slot_index);
          }
          else {
            LOG_E(PHY,"set_tdd_configuration_dedicated_nr: tdd downlink & uplink symbol configuration is not supported \n");
            return (-1);
          }
        }
        else {
          LOG_E(PHY,"set_tdd_configuration_dedicated_nr: tdd downlink symbol configuration is not supported \n");
          return (-1);
        }
      }
      else if (p_current_TDD_UL_DL_SlotConfig->nrofUplinkSymbols != 0) {
        if (p_current_TDD_UL_DL_SlotConfig->nrofUplinkSymbols == NR_TDD_SET_ALL_SYMBOLS) {
          frame_parms->tdd_uplink_nr[slot_index] = NR_TDD_UPLINK_SLOT;
          NR_TST_PHY_PRINTF(" UL[%d] ", slot_index);
        }
        else {
          LOG_E(PHY,"set_tdd_configuration_dedicated_nr: tdd uplink symbol configuration is not supported \n");
          return (-1);
        }
      }
      else {
        LOG_E(PHY,"set_tdd_configuration_dedicated_nr: no tdd symbol configuration is specified \n");
        return (-1);
      }
    }
    else {
      LOG_E(PHY,"set_tdd_configuration_dedicated_nr: tdd slot index exceeds maximum value \n");
      return (-1);
    }

    p_current_TDD_UL_DL_SlotConfig = (TDD_UL_DL_SlotConfig_t *)(p_current_TDD_UL_DL_SlotConfig->p_next_TDD_UL_DL_SlotConfig);
  }
  NR_TST_PHY_PRINTF("\n");
  return (0);
}

/*******************************************************************
*
* NAME :         set_tdd_configuration
*
* PARAMETERS :   pointer to tdd common configuration
*                pointer to tdd common configuration2
*                pointer to tdd dedicated configuration
*
* OUTPUT:        table of uplink symbol for each slot for 2 frames
*
* RETURN :       0  if srs sequence has been successfully generated
*                -1 if sequence can not be properly generated
*
* DESCRIPTION :  generate bit map for uplink symbol for each slot for 2 frames
*                see TS 38.213 11.1 Slot configuration
*
*********************************************************************/

int slot_select_nr(NR_DL_FRAME_PARMS *frame_parms, int nr_frame, int nr_tti)
{
  /* for FFD all slot can be considered as an uplink */
  if (frame_parms->frame_type == FDD) {
    return (NR_UPLINK_SLOT | NR_DOWNLINK_SLOT );
  }

  if (nr_frame%2 == 0) {
    if (frame_parms->tdd_uplink_nr[nr_tti] == NR_TDD_UPLINK_SLOT) {
      return (NR_UPLINK_SLOT);
    }
    else {
      return (NR_DOWNLINK_SLOT);
    }
  }
  else if ((frame_parms->tdd_uplink_nr[(frame_parms->ttis_per_subframe * LTE_NUMBER_OF_SUBFRAMES_PER_FRAME) + nr_tti] == NR_TDD_UPLINK_SLOT)) {
    return (NR_UPLINK_SLOT);
  }
  else {
    return (NR_DOWNLINK_SLOT);
  }
}

/*******************************************************************
*
* NAME :         free_tdd_configuration_nr
*
* PARAMETERS :   pointer to frame configuration
*
* RETURN :       none
*
* DESCRIPTION :  free structure related to tdd configuration
*
*********************************************************************/

void free_tdd_configuration_nr(NR_DL_FRAME_PARMS *frame_parms)
{
  TDD_UL_DL_configCommon_t *p_tdd_UL_DL_Configuration = frame_parms->p_tdd_UL_DL_Configuration;

  free_tdd_configuration_dedicated_nr(frame_parms);

  if (p_tdd_UL_DL_Configuration != NULL) {
    frame_parms->p_tdd_UL_DL_Configuration = NULL;
    free(p_tdd_UL_DL_Configuration);
  }

  for (int number_of_slot = 0; number_of_slot < NR_MAX_SLOTS_PER_FRAME; number_of_slot++) {
    frame_parms->tdd_uplink_nr[number_of_slot] = NR_TDD_DOWNLINK_SLOT;
  }
}

/*******************************************************************
*
* NAME :         free_tdd_configuration_dedicated_nr
*
* PARAMETERS :   pointer to frame configuration
*
* RETURN :       none
*
* DESCRIPTION :  free structure related to tdd dedicated configuration
*
*********************************************************************/

void free_tdd_configuration_dedicated_nr(NR_DL_FRAME_PARMS *frame_parms)
{
  TDD_UL_DL_SlotConfig_t *p_current_TDD_UL_DL_ConfigDedicated = frame_parms->p_TDD_UL_DL_ConfigDedicated;
  TDD_UL_DL_SlotConfig_t *p_next_TDD_UL_DL_ConfigDedicated;
  int next = 0;
  if (p_current_TDD_UL_DL_ConfigDedicated != NULL) {
    do {
      if (p_current_TDD_UL_DL_ConfigDedicated->p_next_TDD_UL_DL_SlotConfig != NULL) {
        next = 1;
        p_next_TDD_UL_DL_ConfigDedicated =  (TDD_UL_DL_SlotConfig_t *)(p_current_TDD_UL_DL_ConfigDedicated->p_next_TDD_UL_DL_SlotConfig);
        p_current_TDD_UL_DL_ConfigDedicated->p_next_TDD_UL_DL_SlotConfig = NULL;
        //printf("free pt %p \n", p_current_TDD_UL_DL_ConfigDedicated);
        free(p_current_TDD_UL_DL_ConfigDedicated);
        p_current_TDD_UL_DL_ConfigDedicated = p_next_TDD_UL_DL_ConfigDedicated;
      }
      else {
        if (p_current_TDD_UL_DL_ConfigDedicated != NULL) {
          frame_parms->p_TDD_UL_DL_ConfigDedicated = NULL;
          //printf("free pt %p \n", p_current_TDD_UL_DL_ConfigDedicated);
          free(p_current_TDD_UL_DL_ConfigDedicated);
          next = 0;
        }
      }
    } while (next);
  }
}

