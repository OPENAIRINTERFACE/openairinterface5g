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

/*! \file asn1_msg.h
* \brief primitives to build the asn1 messages
* \author Raymond Knopp and Navid Nikaein, WIE-TAI CHEN
* \date 2011, 2018
* \version 1.0
* \company Eurecom, NTUST
* \email: raymond.knopp@eurecom.fr and  navid.nikaein@eurecom.fr, kroempa@gmail.com
*/

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h> /* for atoi(3) */
#include <unistd.h> /* for getopt(3) */
#include <string.h> /* for strerror(3) */
#include <sysexits.h> /* for EX_* exit codes */
#include <errno.h>  /* for errno */

#include <asn_application.h>
#include <asn_internal.h> /* for _ASN_DEFAULT_STACK_MAX */

#include "RRC/NR/nr_rrc_defs.h"
#include "RRC/NR/nr_rrc_config.h"


/*
 * The variant of the above function which dumps the BASIC-XER (XER_F_BASIC)
 * output into the chosen string buffer.
 * RETURN VALUES:
 *       0: The structure is printed.
 *      -1: Problem printing the structure.
 * WARNING: No sensible errno value is returned.
 */
int xer_sprint_NR(char *string, size_t string_size, struct asn_TYPE_descriptor_s *td, void *sptr);

uint8_t do_MIB_NR(gNB_RRC_INST *rrc,
                  uint32_t frame);

/**
\brief Generate configuration for SIB1 (gNB).
@param carrier pointer to Carrier information
@param configuration Pointer Configuration Request structure  
@return size of encoded bit stream in bytes*/
uint8_t do_SIB1_NR(rrc_gNB_carrier_data_t *carrier,
  gNB_RrcConfigurationReq *configuration
                  );

void do_RLC_BEARER(uint8_t Mod_id,
                    int CC_id,
                    struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_BearerToAddModList,
                    rlc_bearer_config_t  *rlc_config);
void do_MAC_CELLGROUP(uint8_t Mod_id,
                      int CC_id,
                      NR_MAC_CellGroupConfig_t *mac_CellGroupConfig,
                      mac_cellgroup_t *mac_cellgroup_config);

void do_PHYSICALCELLGROUP(uint8_t Mod_id,
                          int CC_id,
                          NR_PhysicalCellGroupConfig_t *physicalCellGroupConfig,
                          physicalcellgroup_t *physicalcellgroup_config);


void do_SpCellConfig(gNB_RRC_INST *rrc,
                      struct NR_SpCellConfig  *spconfig);
