/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file PHY/LTE_TRANSPORT/if4_tools.c
* \brief 
* \author S. Sandeep Kumar, Raymond Knopp
* \date 2016
* \version 0.1
* \company Eurecom
* \email: ee13b1025@iith.ac.in, knopp@eurecom.fr 
* \note
* \warning
*/

#include "PHY/defs.h"
#include "PHY/TOOLS/alaw_lut.h"
#include "PHY/extern.h"
#include "SCHED/defs.h"

//#include "targets/ARCH/ETHERNET/USERSPACE/LIB/if_defs.h"
#include "targets/ARCH/ETHERNET/USERSPACE/LIB/ethernet_lib.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

void send_IF4p5(PHY_VARS_eNB *eNB, int frame, int subframe, uint16_t packet_type, int k) {
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  int32_t **txdataF      = (eNB->CC_id==0) ? eNB->common_vars.txdataF[0] : PHY_vars_eNB_g[0][0]->common_vars.txdataF[0];
  int32_t **rxdataF      = eNB->common_vars.rxdataF[0];
  int16_t **rxsigF       = eNB->prach_vars.rxsigF;  
  void *tx_buffer        = eNB->ifbuffer.tx[subframe&1];
  void *tx_buffer_prach  = eNB->ifbuffer.tx_prach;
      
  uint16_t symbol_id=0, element_id=0;
  uint16_t db_fulllength, db_halflength; 
  int slotoffsetF=0, blockoffsetF=0; 

  uint16_t *data_block=NULL, *i=NULL;

  IF4p5_header_t *packet_header=NULL;
  eth_state_t *eth = (eth_state_t*) (eNB->ifdevice.priv);
  int nsym = fp->symbols_per_tti;
  
  if (eNB->CC_id==0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_SEND_IF4, 1 );   

  if (packet_type == IF4p5_PDLFFT) {
    if (subframe_select(fp,subframe)==SF_S)
      nsym=fp->dl_symbols_in_S_subframe;

    db_fulllength = 12*fp->N_RB_DL;
    db_halflength = (db_fulllength)>>1;
    slotoffsetF = (subframe)*(fp->ofdm_symbol_size)*((fp->Ncp==1) ? 12 : 14) + 1;
    blockoffsetF = slotoffsetF + fp->ofdm_symbol_size - db_halflength - 1; 


    if (eth->flags == ETH_RAW_IF4p5_MODE) {
      packet_header = (IF4p5_header_t *)(tx_buffer + MAC_HEADER_SIZE_BYTES);
      data_block = (uint16_t*)(tx_buffer + MAC_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t);
    } else {
      packet_header = (IF4p5_header_t *)(tx_buffer);
      data_block = (uint16_t*)(tx_buffer + sizeof_IF4p5_header_t);
    }    
    gen_IF4p5_dl_header(packet_header, frame, subframe);
		    
    for (symbol_id=0; symbol_id<nsym; symbol_id++) {
      if (eNB->CC_id==1) LOG_I(PHY,"DL_IF4p5: CC_id %d : frame %d, subframe %d, symbol %d\n",eNB->CC_id,frame,subframe,symbol_id);
      
      for (element_id=0; element_id<db_halflength; element_id++) {
        i = (uint16_t*) &txdataF[eNB->CC_id][blockoffsetF+element_id];
        data_block[element_id] = ((uint16_t) lin2alaw[*i]) | (lin2alaw[*(i+1)]<<8);

        i = (uint16_t*) &txdataF[eNB->CC_id][slotoffsetF+element_id];
        data_block[element_id+db_halflength] = ((uint16_t) lin2alaw[*i]) | (lin2alaw[*(i+1)]<<8);        
      }
				 		
      packet_header->frame_status &= ~(0x000f<<26);
      packet_header->frame_status |= (symbol_id&0x000f)<<26; 
			
      if ((eNB->ifdevice.trx_write_func(&eNB->ifdevice,
                                        symbol_id,
                                        &tx_buffer,
                                        db_fulllength,
      			                            1,
                                        IF4p5_PDLFFT)) < 0) {
        perror("ETHERNET write for IF4p5_PDLFFT\n");
      }
      
      slotoffsetF  += fp->ofdm_symbol_size;
      blockoffsetF += fp->ofdm_symbol_size;    
    }
  } else if ((packet_type == IF4p5_PULFFT)||
	     (packet_type == IF4p5_PULTICK)){
    db_fulllength = 12*fp->N_RB_UL;
    db_halflength = (db_fulllength)>>1;
    slotoffsetF = 1;
    blockoffsetF = slotoffsetF + fp->ofdm_symbol_size - db_halflength - 1; 

    if (subframe_select(fp,subframe)==SF_S) {
      nsym=fp->ul_symbols_in_S_subframe;
      slotoffsetF  += (fp->ofdm_symbol_size*(fp->symbols_per_tti-nsym));
      blockoffsetF += (fp->ofdm_symbol_size*(fp->symbols_per_tti-nsym));
    }

    if (eth->flags == ETH_RAW_IF4p5_MODE) {
      packet_header = (IF4p5_header_t *)(tx_buffer + MAC_HEADER_SIZE_BYTES);
      data_block = (uint16_t*)(tx_buffer + MAC_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t);
    } else {
      packet_header = (IF4p5_header_t *)(tx_buffer);
      data_block = (uint16_t*)(tx_buffer + sizeof_IF4p5_header_t);
    }
    gen_IF4p5_ul_header(packet_header, packet_type, frame, subframe);

    if (packet_type == IF4p5_PULFFT) {
      for (symbol_id=fp->symbols_per_tti-nsym; symbol_id<fp->symbols_per_tti; symbol_id++) {			
	LOG_D(PHY,"IF4p5_PULFFT: frame %d, subframe %d, symbol %d\n",frame,subframe,symbol_id);
	for (element_id=0; element_id<db_halflength; element_id++) {
	  i = (uint16_t*) &rxdataF[0][blockoffsetF+element_id];
	  data_block[element_id] = ((uint16_t) lin2alaw[*i]) | (lin2alaw[*(i+1)]<<8);
	  
	  i = (uint16_t*) &rxdataF[0][slotoffsetF+element_id];
	  data_block[element_id+db_halflength] = ((uint16_t) lin2alaw[*i]) | (lin2alaw[*(i+1)]<<8);        
	}
       	
	packet_header->frame_status &= ~(0x000f<<26);
	packet_header->frame_status |= (symbol_id&0x000f)<<26; 
	
	if ((eNB->ifdevice.trx_write_func(&eNB->ifdevice,
					  symbol_id,
					  &tx_buffer,
					  db_fulllength,
					  1,
					  IF4p5_PULFFT)) < 0) {
	  perror("ETHERNET write for IF4p5_PULFFT\n");
	}
	
	slotoffsetF  += fp->ofdm_symbol_size;
	blockoffsetF += fp->ofdm_symbol_size;
      }    
    }
    else {
      if ((eNB->ifdevice.trx_write_func(&eNB->ifdevice,
					0,
					&tx_buffer,
					0,
					1,
					IF4p5_PULTICK)) < 0) {
	perror("ETHERNET write for IF4p5_PULFFT\n");
      }
    }
  } else if (packet_type == IF4p5_PRACH) {
    // FIX: hard coded prach samples length
    LOG_D(PHY,"IF4p5_PRACH: frame %d, subframe %d\n",frame,subframe);
    db_fulllength = PRACH_HARD_CODED_NUM_SAMPLES;
    
    if (eth->flags == ETH_RAW_IF4p5_MODE) {
      packet_header = (IF4p5_header_t *)(tx_buffer_prach + MAC_HEADER_SIZE_BYTES);
      data_block = (uint16_t*)(tx_buffer + MAC_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t);
    } else {
      packet_header = (IF4p5_header_t *)(tx_buffer_prach);
      data_block = (uint16_t*)(tx_buffer_prach + sizeof_IF4p5_header_t);
    }  
    gen_IF4p5_prach_header(packet_header, frame, subframe);

    if (eth->flags == ETH_RAW_IF4p5_MODE) {
      memcpy((int16_t*)(tx_buffer_prach + MAC_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t),
             (&rxsigF[0][k]), 
             PRACH_BLOCK_SIZE_BYTES);
    } else {
      memcpy((int16_t*)(tx_buffer_prach + sizeof_IF4p5_header_t),
             (&rxsigF[0][k]),
             PRACH_BLOCK_SIZE_BYTES);
    }

    if ((eNB->ifdevice.trx_write_func(&eNB->ifdevice,
                                      symbol_id,
                                      &tx_buffer_prach,
                                      db_fulllength,
                                      1,
                                      IF4p5_PRACH)) < 0) {
      perror("ETHERNET write for IF4p5_PRACH\n");
    }      
  } else {    
    AssertFatal(1==0, "send_IF4p5 - Unknown packet_type %x", packet_type);     
  }

  if (eNB->CC_id==0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_SEND_IF4, 0 );  
  return;  		    
}


void recv_IF4p5(PHY_VARS_eNB *eNB, int *frame, int *subframe, uint16_t *packet_type, uint32_t *symbol_number) {
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  int32_t **txdataF = eNB->common_vars.txdataF[0];
  int32_t **rxdataF = eNB->common_vars.rxdataF[0];
  int16_t **rxsigF = eNB->prach_vars.rxsigF;  
  void *rx_buffer = eNB->ifbuffer.rx;

  uint16_t element_id;
  uint16_t db_fulllength, db_halflength; 
  int slotoffsetF=0, blockoffsetF=0; 
  eth_state_t *eth = (eth_state_t*) (eNB->ifdevice.priv);

  if (eNB->CC_id==0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_RECV_IF4, 1 );   
  
  if (eNB->node_function == NGFI_RRU_IF4p5) {
    db_fulllength = (12*fp->N_RB_DL); 
  } else {
    db_fulllength = (12*fp->N_RB_UL);     
  }  
  db_halflength = db_fulllength>>1;

  IF4p5_header_t *packet_header=NULL;
  uint16_t *data_block=NULL, *i=NULL;
     
  if (eNB->ifdevice.trx_read_func(&eNB->ifdevice,
                                  (int64_t*) packet_type,
                                  &rx_buffer,
                                  db_fulllength,
                                  0) < 0) {
    perror("ETHERNET read");
  }

  if (eth->flags == ETH_RAW_IF4p5_MODE) {
    packet_header = (IF4p5_header_t*) (rx_buffer+MAC_HEADER_SIZE_BYTES);
    data_block = (uint16_t*) (rx_buffer+MAC_HEADER_SIZE_BYTES+sizeof_IF4p5_header_t);
  } else {
    packet_header = (IF4p5_header_t*) (rx_buffer);
    data_block = (uint16_t*) (rx_buffer+sizeof_IF4p5_header_t);
  }


  
  *frame = ((packet_header->frame_status)>>6)&0xffff;
  *subframe = ((packet_header->frame_status)>>22)&0x000f;

  *packet_type = packet_header->sub_type; 


  if (*packet_type == IF4p5_PDLFFT) {          
    *symbol_number = ((packet_header->frame_status)>>26)&0x000f;         

    LOG_D(PHY,"DL_IF4p5: CC_id %d : frame %d, subframe %d, symbol %d\n",eNB->CC_id,*frame,*subframe,*symbol_number);

    slotoffsetF = (*symbol_number)*(fp->ofdm_symbol_size) + (*subframe)*(fp->ofdm_symbol_size)*((fp->Ncp==1) ? 12 : 14) + 1;
    blockoffsetF = slotoffsetF + fp->ofdm_symbol_size - db_halflength - 1; 
        
    for (element_id=0; element_id<db_halflength; element_id++) {
      i = (uint16_t*) &txdataF[0][blockoffsetF+element_id];
      *i = alaw2lin[ (data_block[element_id] & 0xff) ]; 
      *(i+1) = alaw2lin[ (data_block[element_id]>>8) ];

      i = (uint16_t*) &txdataF[0][slotoffsetF+element_id];
      *i = alaw2lin[ (data_block[element_id+db_halflength] & 0xff) ]; 
      *(i+1) = alaw2lin[ (data_block[element_id+db_halflength]>>8) ];
    }
		        
  } else if (*packet_type == IF4p5_PULFFT) {         
    *symbol_number = ((packet_header->frame_status)>>26)&0x000f;         

    if (eNB->CC_id==1) LOG_I(PHY,"UL_IF4p5: CC_id %d : frame %d, subframe %d, symbol %d\n",eNB->CC_id,*frame,*subframe,*symbol_number);

    slotoffsetF = (*symbol_number)*(fp->ofdm_symbol_size) + 1;
    blockoffsetF = slotoffsetF + fp->ofdm_symbol_size - db_halflength - 1; 
    
    for (element_id=0; element_id<db_halflength; element_id++) {
      i = (uint16_t*) &rxdataF[0][blockoffsetF+element_id];
      *i = alaw2lin[ (data_block[element_id] & 0xff) ]; 
      *(i+1) = alaw2lin[ (data_block[element_id]>>8) ];

      i = (uint16_t*) &rxdataF[0][slotoffsetF+element_id];
      *i = alaw2lin[ (data_block[element_id+db_halflength] & 0xff) ]; 
      *(i+1) = alaw2lin[ (data_block[element_id+db_halflength]>>8) ];
    }
		
  } else if (*packet_type == IF4p5_PRACH) {    
    if (eNB->CC_id==1) LOG_I(PHY,"PRACH_IF4p5: CC_id %d : frame %d, subframe %d, symbol %d\n",eNB->CC_id,*frame,*subframe);

    // FIX: hard coded prach samples length
    db_fulllength = PRACH_HARD_CODED_NUM_SAMPLES;

    if (eth->flags == ETH_RAW_IF4p5_MODE) {		
      memcpy((&rxsigF[0][0]), 
             (int16_t*) (rx_buffer+MAC_HEADER_SIZE_BYTES+sizeof_IF4p5_header_t), 
             PRACH_BLOCK_SIZE_BYTES);
    } else {
      memcpy((&rxsigF[0][0]),
             (int16_t*) (rx_buffer+sizeof_IF4p5_header_t),
             PRACH_BLOCK_SIZE_BYTES);
    }
  } else if (*packet_type == IF4p5_PULTICK) {

  } else {
    AssertFatal(1==0, "recv_IF4p5 - Unknown packet_type %x", *packet_type);            
  }

  if (eNB->CC_id==0) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_RECV_IF4, 0 );     
  return;   
}


void gen_IF4p5_dl_header(IF4p5_header_t *dl_packet, int frame, int subframe) {      
  dl_packet->type = IF4p5_PACKET_TYPE; 
  dl_packet->sub_type = IF4p5_PDLFFT;

  dl_packet->rsvd = 0;
  
  // Set frame status
  dl_packet->frame_status = 0;
  dl_packet->frame_status |= (frame&0xffff)<<6;
  dl_packet->frame_status |= (subframe&0x000f)<<22;
}


void gen_IF4p5_ul_header(IF4p5_header_t *ul_packet, uint16_t packet_subtype, int frame, int subframe) {  

  ul_packet->type = IF4p5_PACKET_TYPE; 
  ul_packet->sub_type = packet_subtype;

  ul_packet->rsvd = 0;
  
  // Set frame status
  ul_packet->frame_status = 0;
  ul_packet->frame_status |= (frame&0xffff)<<6;
  ul_packet->frame_status |= (subframe&0x000f)<<22;
}


void gen_IF4p5_prach_header(IF4p5_header_t *prach_packet, int frame, int subframe) {
  prach_packet->type = IF4p5_PACKET_TYPE; 
  prach_packet->sub_type = IF4p5_PRACH;

  prach_packet->rsvd = 0;
  
  // Set LTE Prach configuration
  prach_packet->frame_status = 0;
  prach_packet->frame_status |= (frame&0xffff)<<6;
  prach_packet->frame_status |= (subframe&0x000f)<<22;
} 


void malloc_IF4p5_buffer(PHY_VARS_eNB *eNB) {
  // Keep the size large enough 
  eth_state_t *eth = (eth_state_t*) (eNB->ifdevice.priv);
  int i;

  if (eth->flags == ETH_RAW_IF4p5_MODE) {
    for (i=0;i<10;i++)
      eNB->ifbuffer.tx[i]       = malloc(RAW_IF4p5_PRACH_SIZE_BYTES);
    eNB->ifbuffer.tx_prach = malloc(RAW_IF4p5_PRACH_SIZE_BYTES);
    eNB->ifbuffer.rx       = malloc(RAW_IF4p5_PRACH_SIZE_BYTES); 
  } else {
    for (i=0;i<10;i++)
      eNB->ifbuffer.tx[i]       = malloc(UDP_IF4p5_PRACH_SIZE_BYTES);
    eNB->ifbuffer.tx_prach = malloc(UDP_IF4p5_PRACH_SIZE_BYTES);
    eNB->ifbuffer.rx       = malloc(UDP_IF4p5_PRACH_SIZE_BYTES);
  }
}
