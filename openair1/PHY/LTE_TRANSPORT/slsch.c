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

/*! \file PHY/LTE_TRANSPORT/slss.c
 * \brief Functions to Generate and Receive PSSCH
 * \author R. Knopp
 * \date 2017
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */
#ifndef __LTE_TRANSPORT_SLSS__C__
#define __LTE_TRANSPORT_SLSS__C__
#include "PHY/defs_UE.h"

extern int
multicast_link_write_sock(int groupP, char *dataP, uint32_t sizeP);


void generate_slsch(PHY_VARS_UE *ue,SLSCH_t *slsch,int frame_tx,int subframe_tx) {

  UE_tport_t pdu;
  size_t slsch_header_len = sizeof(UE_tport_header_t);

  if (slsch->rvidx==0) {
    pdu.header.packet_type = SLSCH;
    pdu.header.absSF = (frame_tx*10)+subframe_tx;
    
    memcpy((void*)&pdu.slsch,(void*)slsch,sizeof(SLSCH_t)-sizeof(uint8_t*));
    
    AssertFatal(slsch->payload_length <=1500-slsch_header_len - sizeof(SLSCH_t) + sizeof(uint8_t*),
		"SLSCH payload length > %zd\n",
		1500-slsch_header_len - sizeof(SLSCH_t) + sizeof(uint8_t*));
    memcpy((void*)&pdu.payload[0],
	   (void*)slsch->payload,
	   slsch->payload_length);
    
    LOG_I(PHY,"SLSCH configuration %zd bytes, TBS payload %d bytes => %zd bytes\n",
	  sizeof(SLSCH_t)-sizeof(uint8_t*),
	  slsch->payload_length,
	  slsch_header_len+sizeof(SLSCH_t)-sizeof(uint8_t*)+slsch->payload_length);
    
    multicast_link_write_sock(0, 
			      (char *)&pdu,
			      slsch_header_len+sizeof(SLSCH_t)-sizeof(uint8_t*)+slsch->payload_length);
    
  }
}

#endif
