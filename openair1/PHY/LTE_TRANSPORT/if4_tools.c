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
void send_IF4(PHY_VARS_eNB *eNB, eNB_rxtx_proc_t *proc) {
	LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
	int32_t **txdataF = eNB->common_vars.txdataF[0];
  
	uint16_t symbol_id, element_id;
  uint16_t db_fulllength = 12*fp->N_RB_DL;
  uint16_t db_halflength = db_fulllength>>1;
  int slotoffsetF = (proc->subframe_tx)*(fp->ofdm_symbol_size)*((fp->Ncp==1) ? 12 : 14) + 1;
  int blockoffsetF = slotoffsetF + fp->ofdm_symbol_size - db_halflength; 
  
  int16_t *data_block = (int16_t*)malloc(db_fulllength*sizeof(int16_t));  

  // Caller: RCC - DL *** handle RRU case - UL and PRACH *** 
  if (eNB->node_function == NGFI_RCC_IF4) {
    IF4_dl_packet_t *dl_packet = (IF4_dl_packet_t*)malloc(sizeof_IF4_dl_packet_t);
    gen_IF4_dl_packet(dl_packet, proc);
		
    dl_packet->data_block = data_block;

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
      dl_packet->frame_status.sym_num = symbol_id; 
			
      // Write the packet(s) to the fronthaul
    //  if ((bytes_sent = eNB->ifdevice.trx_write_func(&eNB->ifdevice,
    //                                                (proc->timestamp_tx-eNB->ifdevice.openair0_cfg.tx_sample_advance),
    //  			                                         dl_packet,
    //  			                                         eNB->frame_parms.samples_per_tti,
    //  			                                         eNB->frame_parms.nb_antennas_tx,
    //                                                 0)) < 0) {
    //    perror("RCC : ETHERNET write");
    //}
      
      slotoffsetF  += fp->ofdm_symbol_size;
      blockoffsetF += fp->ofdm_symbol_size;    
    }
  }else {
    IF4_ul_packet_t *ul_packet = (IF4_ul_packet_t*)malloc(sizeof_IF4_ul_packet_t);
    gen_IF4_ul_packet(ul_packet, proc);
		
    ul_packet->data_block = data_block;

    for (symbol_id=0; symbol_id<fp->symbols_per_tti; symbol_id++) {			

      // Do compression of the two parts and generate data blocks	- rxdataF		
      for (element_id=0; element_id<db_halflength; element_id++) {
        data_block[element_id]  = lin2alaw[ (rxdataF[0][blockoffsetF+element_id] & 0xffff) + 32768 ];          
        data_block[element_id] |= lin2alaw[ (rxdataF[0][blockoffsetF+element_id]>>16) + 32768 ]<<8;  
        
        data_block[element_id+db_halflength]  = lin2alaw[ (txdataF[0][slotoffsetF+element_id] & 0xffff) + 32768 ];     
        data_block[element_id+db_halflength] |= lin2alaw[ (txdataF[0][slotoffsetF+element_id]>>16) + 32768 ]<<8;  
      }
       			
      // Update information in generated packet
      ul_packet->frame_status.sym_num = symbol_id; 
			
      // Write the packet(s) to the fronthaul 

      slotoffsetF  += fp->ofdm_symbol_size;
      blockoffsetF += fp->ofdm_symbol_size;    
    }		
	}
  		    
}

void recv_IF4(PHY_VARS_eNB *eNB, eNB_rxtx_proc_t *proc, uint16_t *packet_type, uint32_t *symbol_number) {

  // Caller: RRU - DL *** handle RCC case - UL and PRACH *** 
  if (eNB->node_function == NGFI_RRU_IF4) {
  
    printf("\n Recv IF4 for frame %d, subframe %d and symbol %d\n", proc->frame_tx, proc->subframe_tx, symbol_id);
  
//    for(i=0; i<fp->symbols_per_tti; i++) {  
      // Read packet(s) from the fronthaul    
//      if (dev->eth_dev.trx_read_func (&dev->eth_dev,
//                                      timestamp_rx,
//                                    rx_eNB,
//                                      spp_eth,
//                                      dev->eth_dev.openair0_cfg->rx_num_channels
//                                      ) < 0) {
//        perror("RRU : ETHERNET read");
//      }
      
      // Apply reverse processing - decompression
      // txAlawtolinear( Datablock )
      
      // Generate and return the OFDM symbols (txdataF)
      //txDataF 
    //}
  }else {
    
  }  
}

void gen_IF4_dl_packet(IF4_dl_packet_t *dl_packet, eNB_rxtx_proc_t *proc) {      
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
  dl_packet->fcs = 0;
}

void gen_IF4_ul_packet(IF4_ul_packet_t *ul_packet, eNB_rxtx_proc_t *proc) {  
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
  ul_packet->fcs = 0;
}

void gen_IF4_prach_packet(IF4_prach_packet_t *prach_packet, eNB_rxtx_proc_t *proc) {
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
  prach_packet->fcs = 0;
} 
