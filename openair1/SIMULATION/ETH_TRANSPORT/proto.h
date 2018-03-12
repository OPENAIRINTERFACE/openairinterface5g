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

/*! \file proto.h
* \brief
* \author Navid Nikaein
* \date 2011
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
*/

#include "SIMULATION/ETH_TRANSPORT/defs.h"

#ifndef EMU_PROTO_H_
#define EMU_PROTO_H_

void init_bypass (void);
void bypass_init ( unsigned int (*tx_handlerP) (unsigned char,char*, unsigned int*, unsigned int*),unsigned int (*rx_handlerP) (unsigned char,char*,unsigned int));
int bypass_rx_data(unsigned int frame, unsigned int last_slot,
                   unsigned int next_slot, uint8_t is_master);
void  bypass_signal_mac_phy(unsigned int frame, unsigned int last_slot,
                            unsigned int next_slot, uint8_t is_master);
void bypass_tx_data (emu_transport_info_t Type, unsigned int frame, unsigned int next_slot);

void emulation_tx_rx(void);

unsigned int emul_tx_handler(unsigned char Mode,char *Tx_buffer,unsigned int* Nbytes,unsigned int *Nb_flows);
unsigned int emul_rx_handler(unsigned char Mode,char *rx_buffer, unsigned int Nbytes);

unsigned int emul_rx_data(void);

void emu_transport_info(unsigned int last_slot, unsigned int next_slot);
void fill_phy_enb_vars(unsigned int enb_id, uint8_t CC_id,unsigned int next_slot);
void fill_phy_ue_vars(unsigned int ue_id, uint8_t CC_id,unsigned int last_slot);
void emu_transport_sync(void);
void emu_transport(unsigned int frame, unsigned int last_slot,unsigned int next_slot, lte_subframe_t direction, unsigned char frame_type, int ethernet_flag );
void emu_transport_DL(unsigned int frame, unsigned int last_slot,unsigned int next_slot);
void emu_transport_UL(unsigned int frame, unsigned int last_slot,unsigned int next_slot);
void emu_transport_release(void);

int multicast_link_read_data_from_sock(uint8_t eNB_flag);

void clear_eNB_transport_info(uint8_t);
void clear_UE_transport_info(uint8_t);
int netlink_init(void);

#endif /* EMU_PROTO_H_ */
