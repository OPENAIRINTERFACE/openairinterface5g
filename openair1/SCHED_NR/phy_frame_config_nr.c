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

#include "PHY/defs_nr_common.h"
#include "PHY/defs_gNB.h"
#include "PHY/defs_nr_UE.h"
#include "SCHED_NR/phy_frame_config_nr.h"

/*******************************************************************
*
* NAME :         set_tdd_configuration
*
* PARAMETERS :   pointer to frame configuration
*
* OUTPUT:        table of uplink symbol for each slot for 2 frames
*
* RETURN :       nb_periods_per_frame if tdd has been properly configurated
*                -1 tdd configuration can not be done
*
* DESCRIPTION :  generate bit map for uplink symbol for each slot for several frames
*                see TS 38.213 11.1 Slot configuration
*
*********************************************************************/

int set_tdd_config_nr( nfapi_nr_config_request_scf_t *cfg,
                       int mu,
                       int nrofDownlinkSlots, int nrofDownlinkSymbols,
                       int nrofUplinkSlots,   int nrofUplinkSymbols) {
  int slot_number = 0;
  int nb_periods_per_frame;
  int nb_slots_to_set = TDD_CONFIG_NB_FRAMES*(1<<mu)*NR_NUMBER_OF_SUBFRAMES_PER_FRAME;

  switch(cfg->tdd_table.tdd_period.value) {
    case 0:
      nb_periods_per_frame = 20; // 10ms/0p5ms
      break;

    case 1:
      nb_periods_per_frame = 16; // 10ms/0p625ms
      break;

    case 2:
      nb_periods_per_frame = 10; // 10ms/1ms
      break;

    case 3:
      nb_periods_per_frame = 8; // 10ms/1p25ms
      break;

    case 4:
      nb_periods_per_frame = 5; // 10ms/2ms
      break;

    case 5:
      nb_periods_per_frame = 4; // 10ms/2p5ms
      break;

    case 6:
      nb_periods_per_frame = 2; // 10ms/5ms
      break;

    case 7:
      nb_periods_per_frame = 1; // 10ms/10ms
      break;

    default:
      AssertFatal(1==0,"Undefined tdd period %d\n", cfg->tdd_table.tdd_period.value);
  }

  int nb_slots_per_period = ((1<<mu) * NR_NUMBER_OF_SUBFRAMES_PER_FRAME)/nb_periods_per_frame;

  if ( (nrofDownlinkSymbols + nrofUplinkSymbols) == 0 )
    AssertFatal(nb_slots_per_period == (nrofDownlinkSlots + nrofUplinkSlots),
                "set_tdd_configuration_nr: given period is inconsistent with current tdd configuration, nrofDownlinkSlots %d, nrofUplinkSlots %d, nb_slots_per_period %d \n",
                nrofDownlinkSlots,nrofUplinkSlots,nb_slots_per_period);
  else {
    AssertFatal(nrofDownlinkSymbols + nrofUplinkSymbols < 14,"illegal symbol configuration DL %d, UL %d\n",nrofDownlinkSymbols,nrofUplinkSymbols);
    AssertFatal(nb_slots_per_period == (nrofDownlinkSlots + nrofUplinkSlots + 1),
                "set_tdd_configuration_nr: given period is inconsistent with current tdd configuration, nrofDownlinkSlots %d, nrofUplinkSlots %d, nrofMixed slots 1, nb_slots_per_period %d \n",
                nrofDownlinkSlots,nrofUplinkSlots,nb_slots_per_period);
  }

  cfg->tdd_table.max_tdd_periodicity_list = (nfapi_nr_max_tdd_periodicity_t *) malloc(nb_slots_to_set*sizeof(nfapi_nr_max_tdd_periodicity_t));

  for(int memory_alloc =0 ; memory_alloc<nb_slots_to_set; memory_alloc++)
    cfg->tdd_table.max_tdd_periodicity_list[memory_alloc].max_num_of_symbol_per_slot_list = (nfapi_nr_max_num_of_symbol_per_slot_t *) malloc(NR_NUMBER_OF_SYMBOLS_PER_SLOT*sizeof(
          nfapi_nr_max_num_of_symbol_per_slot_t));

  while(slot_number != nb_slots_to_set) {
    if(nrofDownlinkSlots != 0) {
      for (int number_of_symbol = 0; number_of_symbol < nrofDownlinkSlots*NR_NUMBER_OF_SYMBOLS_PER_SLOT; number_of_symbol++) {
        cfg->tdd_table.max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol%NR_NUMBER_OF_SYMBOLS_PER_SLOT].slot_config.value= 0;

        if((number_of_symbol+1)%NR_NUMBER_OF_SYMBOLS_PER_SLOT == 0)
          slot_number++;
      }
    }

    if (nrofDownlinkSymbols != 0 || nrofUplinkSymbols != 0) {
      for(int number_of_symbol =0; number_of_symbol < nrofDownlinkSymbols; number_of_symbol++) {
        cfg->tdd_table.max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol].slot_config.value= 0;
      }

      for(int number_of_symbol = nrofDownlinkSymbols; number_of_symbol < NR_NUMBER_OF_SYMBOLS_PER_SLOT-nrofUplinkSymbols; number_of_symbol++) {
        cfg->tdd_table.max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol].slot_config.value= 2;
      }

      for(int number_of_symbol = NR_NUMBER_OF_SYMBOLS_PER_SLOT-nrofUplinkSymbols; number_of_symbol < NR_NUMBER_OF_SYMBOLS_PER_SLOT; number_of_symbol++) {
        cfg->tdd_table.max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol].slot_config.value= 1;
      }

      slot_number++;
    }

    if(nrofUplinkSlots != 0) {
      for (int number_of_symbol = 0; number_of_symbol < nrofUplinkSlots*NR_NUMBER_OF_SYMBOLS_PER_SLOT; number_of_symbol++) {
        cfg->tdd_table.max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol%NR_NUMBER_OF_SYMBOLS_PER_SLOT].slot_config.value= 1;

        if((number_of_symbol+1)%NR_NUMBER_OF_SYMBOLS_PER_SLOT == 0)
          slot_number++;
      }
    }
  }

  /*
  while(slot_number != nb_slots_to_set) {
    for (int number_of_slot = 0; number_of_slot < nrofDownlinkSlots; number_of_slot++) {
      frame_parms->tdd_uplink_nr[slot_number] = NR_TDD_DOWNLINK_SLOT;
      printf("slot %d set as downlink\n",slot_number);
      slot_number++;
    }

    if (nrofDownlinkSymbols != 0 || nrofUplinkSymbols != 0) {
       frame_parms->tdd_uplink_nr[slot_number] = (1<<nrofUplinkSymbols) - 1;
       printf("slot %d set as SL\n",slot_number);
       slot_number++;
    }

    for (int number_of_slot = 0; number_of_slot < nrofUplinkSlots; number_of_slot++) {
      frame_parms->tdd_uplink_nr[slot_number] = NR_TDD_UPLINK_SLOT;
      printf("slot %d set as uplink\n",slot_number);
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
  }*/
  return (nb_periods_per_frame);
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

void add_tdd_dedicated_configuration_nr(NR_DL_FRAME_PARMS *frame_parms, int slotIndex, int nrofDownlinkSymbols, int nrofUplinkSymbols) {
  TDD_UL_DL_SlotConfig_t *p_TDD_UL_DL_ConfigDedicated = frame_parms->p_TDD_UL_DL_ConfigDedicated;
  TDD_UL_DL_SlotConfig_t *p_previous_TDD_UL_DL_ConfigDedicated=NULL;
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
  } else {
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

int set_tdd_configuration_dedicated_nr(NR_DL_FRAME_PARMS *frame_parms) {
  TDD_UL_DL_SlotConfig_t *p_current_TDD_UL_DL_SlotConfig;
  p_current_TDD_UL_DL_SlotConfig = frame_parms->p_TDD_UL_DL_ConfigDedicated;
  NR_TST_PHY_PRINTF("\nSet tdd dedicated configuration\n ");

  while(p_current_TDD_UL_DL_SlotConfig != NULL) {
    int slot_index = p_current_TDD_UL_DL_SlotConfig->slotIndex;

    if (slot_index < TDD_CONFIG_NB_FRAMES * frame_parms->slots_per_frame) {
      if (p_current_TDD_UL_DL_SlotConfig->nrofDownlinkSymbols != 0) {
        if (p_current_TDD_UL_DL_SlotConfig->nrofDownlinkSymbols == NR_TDD_SET_ALL_SYMBOLS) {
          if (p_current_TDD_UL_DL_SlotConfig->nrofUplinkSymbols == 0) {
            frame_parms->tdd_uplink_nr[slot_index] = NR_TDD_DOWNLINK_SLOT;
            NR_TST_PHY_PRINTF(" DL[%d] ", slot_index);
          } else {
            LOG_E(PHY,"set_tdd_configuration_dedicated_nr: tdd downlink & uplink symbol configuration is not supported \n");
            return (-1);
          }
        } else {
          LOG_E(PHY,"set_tdd_configuration_dedicated_nr: tdd downlink symbol configuration is not supported \n");
          return (-1);
        }
      } else if (p_current_TDD_UL_DL_SlotConfig->nrofUplinkSymbols != 0) {
        if (p_current_TDD_UL_DL_SlotConfig->nrofUplinkSymbols == NR_TDD_SET_ALL_SYMBOLS) {
          frame_parms->tdd_uplink_nr[slot_index] = NR_TDD_UPLINK_SLOT;
          NR_TST_PHY_PRINTF(" UL[%d] ", slot_index);
        } else {
          LOG_E(PHY,"set_tdd_configuration_dedicated_nr: tdd uplink symbol configuration is not supported \n");
          return (-1);
        }
      } else {
        LOG_E(PHY,"set_tdd_configuration_dedicated_nr: no tdd symbol configuration is specified \n");
        return (-1);
      }
    } else {
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

int get_next_downlink_slot(PHY_VARS_gNB *gNB, nfapi_nr_config_request_scf_t *cfg, int nr_frame, int nr_slot) {

  int slot = nr_slot;
  int frame = nr_frame;
  int slots_per_frame = gNB->frame_parms.slots_per_frame;
  while (true) {
    slot++;
    if (slot/slots_per_frame) frame++;
    slot %= slots_per_frame;
    int slot_type = nr_slot_select(cfg, frame, slot);
    if (slot_type == NR_DOWNLINK_SLOT || slot_type == NR_MIXED_SLOT) return slot;
    AssertFatal(frame < (nr_frame+2), "Something went worng. This shouldn't happen\n");
  }
}

int nr_slot_select(nfapi_nr_config_request_scf_t *cfg, int nr_frame, int nr_slot) {
  /* for FFD all slot can be considered as an uplink */
  int mu = cfg->ssb_config.scs_common.value,check_slot=0;

  if (cfg->cell_config.frame_duplex_type.value == FDD) {
    return (NR_UPLINK_SLOT | NR_DOWNLINK_SLOT );
  }

  if (nr_frame%2 == 0) {
    for(int symbol_count=0; symbol_count<NR_NUMBER_OF_SYMBOLS_PER_SLOT; symbol_count++) {
      if (cfg->tdd_table.max_tdd_periodicity_list[nr_slot].max_num_of_symbol_per_slot_list[symbol_count].slot_config.value==1) {
        check_slot++;
      }
    }

    if(check_slot == NR_NUMBER_OF_SYMBOLS_PER_SLOT) {
      return (NR_UPLINK_SLOT);
    }

    check_slot = 0;

    for(int symbol_count=0; symbol_count<NR_NUMBER_OF_SYMBOLS_PER_SLOT; symbol_count++) {
      if (cfg->tdd_table.max_tdd_periodicity_list[nr_slot].max_num_of_symbol_per_slot_list[symbol_count].slot_config.value==0) {
        check_slot++;
      }
    }

    if(check_slot == NR_NUMBER_OF_SYMBOLS_PER_SLOT) {
      return (NR_DOWNLINK_SLOT);
    } else {
      return (NR_MIXED_SLOT);
    }
  } else {
    for(int symbol_count=0; symbol_count<NR_NUMBER_OF_SYMBOLS_PER_SLOT; symbol_count++) {
      if (cfg->tdd_table.max_tdd_periodicity_list[((1<<mu) * NR_NUMBER_OF_SUBFRAMES_PER_FRAME) + nr_slot].max_num_of_symbol_per_slot_list[symbol_count].slot_config.value==1) {
        check_slot++;
      }
    }

    if(check_slot == NR_NUMBER_OF_SYMBOLS_PER_SLOT) {
      return (NR_UPLINK_SLOT);
    }

    check_slot = 0;

    for(int symbol_count=0; symbol_count<NR_NUMBER_OF_SYMBOLS_PER_SLOT; symbol_count++) {
      if (cfg->tdd_table.max_tdd_periodicity_list[((1<<mu) * NR_NUMBER_OF_SUBFRAMES_PER_FRAME) + nr_slot].max_num_of_symbol_per_slot_list[symbol_count].slot_config.value==0) {
        check_slot++;
      }
    }

    if(check_slot == NR_NUMBER_OF_SYMBOLS_PER_SLOT) {
      return (NR_DOWNLINK_SLOT);
    } else {
      return (NR_MIXED_SLOT);
    }
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

void free_tdd_configuration_nr(NR_DL_FRAME_PARMS *frame_parms) {
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

void free_tdd_configuration_dedicated_nr(NR_DL_FRAME_PARMS *frame_parms) {
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
      } else {
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

