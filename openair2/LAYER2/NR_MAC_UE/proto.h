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

/* \file proto.h
 * \brief MAC functions prototypes for eNB and UE
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#ifndef __LAYER2_MAC_PROTO_H__
#define __LAYER2_MAC_PROTO_H__

#include "mac_defs.h"

/**\brief decode mib pdu in NR_UE, from if_module ul_ind with P7 tx_ind message
   \param module_id      module id
   \param CC_id          component carrier id
   \param gNB_index      gNB index
   \param extra_bits     extra bits for frame calculation
   \param l_ssb_equal_64 check if ssb number of candicate is equal 64, 1=equal; 0=non equal. Reference 38.212 7.1.1
   \param pduP           pointer to pdu
   \param pdu_length     length of pdu*/
int8_t nr_ue_decode_mib(module_id_t module_id, int CC_id, uint8_t gNB_index, uint8_t extra_bits, uint32_t l_ssb_equal_64, void *pduP, uint16_t pdu_len);


/**\brief primitive from RRC layer to MAC layer for configuration L1/L2, now supported 4 rrc messages: MIB, cell_group_config for MAC/PHY, spcell_config(serving cell config)
   \param module_id                 module id
   \param CC_id                     component carrier id
   \param gNB_index                 gNB index
   \param mibP                      pointer to RRC message MIB
   \param mac_cell_group_configP    pointer to RRC message MAC-related in cell group config 
   \param phy_cell_group_configP    pointer to RRC message PHY-related in cell group config
   \param spcell_configP            pointer to RRC message serving cell config*/
int nr_rrc_mac_config_req_ue( module_id_t module_id, int CC_id, uint8_t gNB_index, NR_MIB_t *mibP, NR_MAC_CellGroupConfig_t *mac_cell_group_configP, NR_PhysicalCellGroupConfig_t *phy_cell_group_configP, NR_SpCellConfig_t *spcell_configP );

int nr_l2_init_ue(void);

NR_UE_MAC_INST_t *get_mac_inst(module_id_t Mod_idP);

#endif
/** @}*/
