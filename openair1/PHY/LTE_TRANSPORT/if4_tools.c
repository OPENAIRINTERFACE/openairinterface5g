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

// Get device information
void send_IF4(PHY_VARS_eNB *eNB, eNB_rxtx_proc_t *proc, uint16_t packet_type) {
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  int32_t **txdataF = eNB->common_vars.txdataF[0];
  int32_t **rxdataF = eNB->common_vars.rxdataF[0];
  
  IF4_dl_header_t *dl_header=NULL;
  IF4_ul_header_t *ul_header=NULL;
  IF4_prach_header_t *prach_header=NULL;   
  
  uint16_t symbol_id, element_id;
  uint16_t db_halflength; 
  int slotoffsetF, blockoffsetF; 

  int16_t *data_block=NULL;
//  int16_t *txbuffer = (int16_t*)malloc(db_halflength*sizeof(int16_t));

  if (packet_type == IF4_PDLFFT) {
    dl_header = (IF4_dl_header_t*)malloc(sizeof_IF4_dl_header_t);
    gen_IF4_dl_header(dl_header, proc);
		
    db_halflength = (12*fp->N_RB_DL)>>1;
    slotoffsetF = (proc->subframe_tx)*(fp->ofdm_symbol_size)*((fp->Ncp==1) ? 12 : 14) + 1;
    blockoffsetF = slotoffsetF + fp->ofdm_symbol_size - db_halflength; 
    
    printf("Problem here - db_half %d\n", db_halflength);
    data_block = (int16_t*)malloc(db_halflength*sizeof(int16_t));

    for (symbol_id=0; symbol_id<fp->symbols_per_tti; symbol_id++) {
      
      printf("\n Send IF4 for frame %d, subframe %d and symbol %d\n", proc->frame_tx, proc->subframe_tx, symbol_id);

      // Do compression of the two parts and generate data blocks			
      for (element_id=0; element_id<db_halflength; element_id++) {
        data_block[element_id]  = lin2alaw[ (txdataF[0][blockoffsetF+element_id] & 0xffff) + 32768 ];          
        data_block[element_id] |= lin2alaw[ (txdataF[0][blockoffsetF+element_id]>>16) + 32768 ]<<8;  
        
        data_block[element_id+db_halflength]  = lin2alaw[ (txdataF[0][slotoffsetF+element_id] & 0xffff) + 32768 ];     
        data_block[element_id+db_halflength] |= lin2alaw[ (txdataF[0][slotoffsetF+element_id]>>16) + 32768 ]<<8;  
      }
				 		
      // Update information in generated packet
      dl_header->frame_status.sym_num = symbol_id; 
			
      printf("\n Not even here !!\n");
      // Write the packet(s) to the fronthaul
      if ((eNB->ifdevice.trx_write_func(&eNB->ifdevice,
                                                     symbol_id,
      			                                         &data_block,
      			                                         1,
      			                                         1,
                                                     0)) < 0) {
        perror("RCC : ETHERNET write");
      }
      
      slotoffsetF  += fp->ofdm_symbol_size;
      blockoffsetF += fp->ofdm_symbol_size;    
    }
  } else if (packet_type == IF4_PULFFT) {
    ul_header = (IF4_ul_header_t*)malloc(sizeof_IF4_ul_header_t);
    gen_IF4_ul_header(ul_header, proc);

    db_halflength = (12*fp->N_RB_UL)>>1;
    slotoffsetF = (proc->subframe_rx)*(fp->ofdm_symbol_size)*((fp->Ncp==1) ? 12 : 14) + 1;
    blockoffsetF = slotoffsetF + fp->ofdm_symbol_size - db_halflength; 
		
    data_block = (int16_t*)malloc(db_halflength*sizeof(int16_t));

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

      slotoffsetF  += fp->ofdm_symbol_size;
      blockoffsetF += fp->ofdm_symbol_size;    
    }		
  } else if (packet_type == IF4_PRACH) {
       
       
  } else {    
    AssertFatal(1==0, "send_IF4 - Unknown packet_type %x", packet_type);     
  }
  
  return;  		    
}

void recv_IF4(PHY_VARS_eNB *eNB, eNB_rxtx_proc_t *proc, uint16_t *packet_type, uint32_t *symbol_number) {

  *packet_type = 0;
  //int16_t *data_block=NULL;
   
  // Read packet(s) from the fronthaul    
//    for(i=0; i<fp->symbols_per_tti; i++) {  
//      if (dev->eth_dev.trx_read_func (&dev->eth_dev,
//                                      timestamp_rx,
//                                    rx_eNB,
//                                      spp_eth,
//                                      dev->eth_dev.openair0_cfg->rx_num_channels
//                                      ) < 0) {
//        perror("RRU : ETHERNET read");
//      }
//  printf("\n Recv IF4 for frame %d, subframe %d and symbol %d\n", proc->frame_tx, proc->subframe_tx, symbol_id);

  //*packet_type = ;  
  
  if (*packet_type == IF4_PDLFFT) {
      
      // Apply reverse processing - decompression
      // txAlawtolinear( Datablock )
      
      // Generate and return the OFDM symbols (txdataF)
      // txDataF 
    
  } else if (*packet_type == IF4_PULFFT) {
    
  } else if (*packet_type == IF4_PRACH) {
       
  } else {
    AssertFatal(1==0, "recv_IF4 - Unknown packet_type %x", *packet_type);            
  }

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

void gen_IF4_prach_header(IF4_prach_header_t *prach_packet, eNB_rxtx_proc_t *proc) {
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
