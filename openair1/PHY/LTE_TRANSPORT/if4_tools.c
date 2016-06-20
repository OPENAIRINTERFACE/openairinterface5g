/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/*! \file PHY/LTE_TRANSPORT/if4_tools.c
* \brief 
* \author Fredrik Skretteberg, Tobias Schuster, Mauricio Gunther, S. Sandeep Kumar, Raymond Knopp
* \date 2016
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr 
* \note
* \warning
*/

#include <stdint.h>

#include "PHY/defs.h"
#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/TOOLS/ALAW/alaw_lut.h"

// --- Careful to handle buffer memory --- RAW/UDP modes --- PRACH variables and data
void send_IF4(PHY_VARS_eNB *eNB, eNB_rxtx_proc_t *proc, uint16_t packet_type) {
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  int32_t **txdataF = eNB->common_vars.txdataF[0];
  int32_t **rxdataF = eNB->common_vars.rxdataF[0];
  int16_t *prachF = eNB->prach_vars.prachF;  
      
  uint16_t symbol_id, element_id;
  uint16_t db_fulllength, db_halflength; 
  int slotoffsetF, blockoffsetF; 

  void *tx_buffer=NULL;
  int16_t *data_block=NULL;

  if (packet_type == IF4_PDLFFT) {
    db_fulllength = 12*fp->N_RB_DL;
    db_halflength = (db_fulllength)>>1;
    slotoffsetF = (proc->subframe_tx)*(fp->ofdm_symbol_size)*((fp->Ncp==1) ? 12 : 14) + 1;
    blockoffsetF = slotoffsetF + fp->ofdm_symbol_size - db_halflength; 

    tx_buffer = malloc(/*SIZE_OF_MAC_BYTES +*/ sizeof_IF4_dl_header_t + db_fulllength*sizeof(int16_t));
    IF4_dl_header_t *dl_header = (IF4_dl_header_t *)(tx_buffer /*+ MAC_HEADER_SIZE_BYTES*/);
    data_block = (int16_t*)(tx_buffer /*+ MAC_HEADER_SIZE_BYTES*/ + sizeof_IF4_dl_header_t);

    gen_IF4_dl_header(dl_header, proc);
		    
    for (symbol_id=0; symbol_id<fp->symbols_per_tti; symbol_id++) {
      // Do compression of the two parts and generate data blocks			
      for (element_id=0; element_id<db_halflength; element_id++) {
        data_block[element_id]  = lin2alaw[ (txdataF[0][blockoffsetF+element_id] & 0xffff) + 32768 ];          
        data_block[element_id] |= lin2alaw[ (txdataF[0][blockoffsetF+element_id]>>16) + 32768 ]<<8;  
        
        data_block[element_id+db_halflength]  = lin2alaw[ (txdataF[0][slotoffsetF+element_id] & 0xffff) + 32768 ];     
        data_block[element_id+db_halflength] |= lin2alaw[ (txdataF[0][slotoffsetF+element_id]>>16) + 32768 ]<<8;  
      }
				 		
      // Update information in generated packet
      dl_header->frame_status.sym_num = symbol_id; 
			
      // Write the packet to the fronthaul
      if ((eNB->ifdevice.trx_write_func(&eNB->ifdevice,
                                        symbol_id,
                                        tx_buffer,
                                        db_fulllength,
      			                            1,
                                        IF4_PDLFFT)) < 0) {
        perror("ETHERNET write for IF4_PDLFFT\n");
      }
      
      slotoffsetF  += fp->ofdm_symbol_size;
      blockoffsetF += fp->ofdm_symbol_size;    
    }
  } else if (packet_type == IF4_PULFFT) {
    db_fulllength = 12*fp->N_RB_UL;
    db_halflength = (db_fulllength)>>1;
    slotoffsetF = (proc->subframe_rx)*(fp->ofdm_symbol_size)*((fp->Ncp==1) ? 12 : 14) + 1;
    blockoffsetF = slotoffsetF + fp->ofdm_symbol_size - db_halflength; 

    tx_buffer = malloc(/*SIZE_OF_MAC_BYTES +*/ sizeof_IF4_dl_header_t + db_fulllength*sizeof(int16_t));
    IF4_ul_header_t *ul_header = (IF4_ul_header_t *)(tx_buffer /*+ MAC_HEADER_SIZE_BYTES*/);
    data_block = (int16_t*)(tx_buffer /*+ MAC_HEADER_SIZE_BYTES*/ + sizeof_IF4_ul_header_t);

    gen_IF4_ul_header(ul_header, proc);

    for (symbol_id=0; symbol_id<fp->symbols_per_tti; symbol_id++) {			
      // Do compression of the two parts and generate data blocks - rxdataF		
      for (element_id=0; element_id<db_halflength; element_id++) {
        data_block[element_id]  = lin2alaw[ (rxdataF[0][blockoffsetF+element_id] & 0xffff) + 32768 ];          
        data_block[element_id] |= lin2alaw[ (rxdataF[0][blockoffsetF+element_id]>>16) + 32768 ]<<8;  
        
        data_block[element_id+db_halflength]  = lin2alaw[ (rxdataF[0][slotoffsetF+element_id] & 0xffff) + 32768 ];     
        data_block[element_id+db_halflength] |= lin2alaw[ (rxdataF[0][slotoffsetF+element_id]>>16) + 32768 ]<<8;  
      }
       			
      // Update information in generated packet
      ul_header->frame_status.sym_num = symbol_id; 
			
      // Write the packet(s) to the fronthaul 
      if ((eNB->ifdevice.trx_write_func(&eNB->ifdevice,
                                        symbol_id,
                                        tx_buffer,
                                        db_fulllength,
      			                            1,
                                        IF4_PULFFT)) < 0) {
        perror("ETHERNET write for IF4_PULFFT\n");
      }

      slotoffsetF  += fp->ofdm_symbol_size;
      blockoffsetF += fp->ofdm_symbol_size;    
    }		
  } else if (packet_type == IF4_PRACH) {
    // FIX: hard coded prach samples length
    db_fulllength = 839;
    slotoffsetF = (proc->subframe_tx)*(fp->ofdm_symbol_size)*((fp->Ncp==1) ? 12 : 14) + 1;

    tx_buffer = malloc(/*SIZE_OF_MAC_BYTES +*/ sizeof_IF4_prach_header_t + db_fulllength*sizeof(int16_t));
    IF4_prach_header_t *prach_header = (IF4_prach_header_t *)(tx_buffer /*+ MAC_HEADER_SIZE_BYTES*/);
    data_block = (int16_t*)(tx_buffer /*+ MAC_HEADER_SIZE_BYTES*/ + sizeof_IF4_prach_header_t);

    gen_IF4_prach_header(prach_header, proc);
		    
    // Do compression and generate data blocks			
    for (element_id=0; element_id<db_fulllength; element_id++) {
      data_block[element_id]  = lin2alaw[ (prachF[blockoffsetF+element_id] & 0xffff) + 32768 ];          
      data_block[element_id] |= lin2alaw[ (prachF[blockoffsetF+element_id]>>16) + 32768 ]<<8;  
    }
              
    // Write the packet to the fronthaul
    if ((eNB->ifdevice.trx_write_func(&eNB->ifdevice,
                                      symbol_id,
                                      tx_buffer,
                                      db_fulllength,
                                      1,
                                      IF4_PRACH)) < 0) {
      perror("ETHERNET write for IF4_PRACH\n");
    }      
  } else {    
    AssertFatal(1==0, "send_IF4 - Unknown packet_type %x", packet_type);     
  }
  
  free(tx_buffer);
  return;  		    
}

void recv_IF4(PHY_VARS_eNB *eNB, eNB_rxtx_proc_t *proc, uint16_t *packet_type, uint32_t *symbol_number) {
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  int32_t **txdataF = eNB->common_vars.txdataF[0];
  int32_t **rxdataF = eNB->common_vars.rxdataF[0];

  uint16_t element_id;
  uint16_t db_halflength; 
  int slotoffsetF, blockoffsetF; 

  *packet_type = 0;
  void *rx_buffer=NULL;
  int16_t *data_block=NULL;
   
  // Read packet(s) from the fronthaul    
  if (eNB->ifdevice.trx_read_func(&eNB->ifdevice,
                                  symbol_number,
                                  rx_buffer,
                                  0,
                                  0) < 0) {
    perror("ETHERNET read");
  }

  packet_type = (uint16_t*) (rx_buffer+2);  
  
  if (*packet_type == IF4_PDLFFT) {
    data_block = (int16_t*) (rx_buffer+sizeof_IF4_dl_header_t);
      
    db_halflength = (12*fp->N_RB_DL)>>1;
    
    // Calculate from received packet
    slotoffsetF = (proc->subframe_tx)*(fp->ofdm_symbol_size)*((fp->Ncp==1) ? 12 : 14) + 1;
    blockoffsetF = slotoffsetF + fp->ofdm_symbol_size - db_halflength; 
    
    data_block = (int16_t*)malloc(db_halflength*sizeof(int16_t));

    // Do decompression of the two parts and generate data blocks			
    for (element_id=0; element_id<db_halflength; element_id++) {
      txdataF[0][blockoffsetF+element_id]  = alaw2lin[ (data_block[element_id] & 0xff) ];
      txdataF[0][blockoffsetF+element_id] |= alaw2lin[ (data_block[element_id]>>8) ]<<16;

      txdataF[0][slotoffsetF+element_id]  = alaw2lin[ (data_block[element_id+db_halflength] & 0xff) ];
      txdataF[0][slotoffsetF+element_id] |= alaw2lin[ (data_block[element_id+db_halflength]>>8) ]<<16;
    }
		
    // Find and return symbol_number		 		
    *symbol_number = 0;      
        
  } else if (*packet_type == IF4_PULFFT) {
    data_block = (int16_t*) (rx_buffer+sizeof_IF4_ul_header_t);
      
    db_halflength = (12*fp->N_RB_UL)>>1;
    
    // Calculate from received packet
    slotoffsetF = (proc->subframe_rx)*(fp->ofdm_symbol_size)*((fp->Ncp==1) ? 12 : 14) + 1;
    blockoffsetF = slotoffsetF + fp->ofdm_symbol_size - db_halflength; 
    
    data_block = (int16_t*)malloc(db_halflength*sizeof(int16_t));

    // Do decompression of the two parts and generate data blocks			
    for (element_id=0; element_id<db_halflength; element_id++) {
      rxdataF[0][blockoffsetF+element_id]  = alaw2lin[ (data_block[element_id] & 0xff) ];
      rxdataF[0][blockoffsetF+element_id] |= alaw2lin[ (data_block[element_id]>>8) ]<<16;

      rxdataF[0][slotoffsetF+element_id]  = alaw2lin[ (data_block[element_id+db_halflength] & 0xff) ];
      rxdataF[0][slotoffsetF+element_id] |= alaw2lin[ (data_block[element_id+db_halflength]>>8) ]<<16;
    }
		
    // Find and return symbol_number		 		
    *symbol_number = 0;      
    
  } else if (*packet_type == IF4_PRACH) {
    data_block = (int16_t*) (rx_buffer+sizeof_IF4_prach_header_t);
       
  } else {
    AssertFatal(1==0, "recv_IF4 - Unknown packet_type %x", *packet_type);            
  }
  
  free(rx_buffer);
  return;   
}

void gen_IF4_dl_header(IF4_dl_header_t *dl_packet, eNB_rxtx_proc_t *proc) {      
  // Set Type and Sub-Type
  dl_packet->type = IF4_PACKET_TYPE; 
  dl_packet->sub_type = IF4_PDLFFT;

  // Leave reserved as it is 
  dl_packet->rsvd = 0;
  
  // Set frame status
  dl_packet->frame_status.ant_num = 0;
  dl_packet->frame_status.ant_start = 0;
  dl_packet->frame_status.rf_num = proc->frame_tx;
  dl_packet->frame_status.sf_num = proc->subframe_tx;
  dl_packet->frame_status.sym_num = 0;
  dl_packet->frame_status.rsvd = 0;

  // Set frame check sequence

}

void gen_IF4_ul_header(IF4_ul_header_t *ul_packet, eNB_rxtx_proc_t *proc) {  
  // Set Type and Sub-Type
  ul_packet->type = IF4_PACKET_TYPE; 
  ul_packet->sub_type = IF4_PULFFT;

  // Leave reserved as it is 
  ul_packet->rsvd = 0;
  
  // Set frame status
  ul_packet->frame_status.ant_num = 0;
  ul_packet->frame_status.ant_start = 0;
  ul_packet->frame_status.rf_num = proc->frame_rx;
  ul_packet->frame_status.sf_num = proc->subframe_rx;
  ul_packet->frame_status.sym_num = 0;
  ul_packet->frame_status.rsvd = 0;
    
  // Set antenna specific gain *** set other antenna gain ***
  ul_packet->gain0.exponent = 0;
  ul_packet->gain0.rsvd = 0;
    
  // Set frame check sequence

}

void gen_IF4_prach_header(IF4_prach_header_t *prach_packet, eNB_proc_t *proc) {
  // Set Type and Sub-Type
  prach_packet->type = IF4_PACKET_TYPE; 
  prach_packet->sub_type = IF4_PRACH;

  // Leave reserved as it is 
  prach_packet->rsvd = 0;
  
  // Set LTE Prach configuration
  prach_packet->prach_conf.rsvd = 0;
  prach_packet->prach_conf.ant = 0;
  prach_packet->prach_conf.rf_num = proc->frame_rx;
  prach_packet->prach_conf.sf_num = proc->subframe_rx;
  prach_packet->prach_conf.exponent = 0;  
        
  // Set frame check sequence

} 
