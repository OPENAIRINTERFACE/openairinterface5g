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
* \author Mauricio Gunther, S. Sandeep Kumar, Raymond Knopp
* \date 2016
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr 
* \note
* \warning
*/

#ifndef USER_MODE
#include "if4_tools.h"
#include <stdint.h>
#else
#include "PHY/LTE_TRANSPORT/if4_tools.h"
#endif

void send_IF4(PHY_VARS_eNB *eNB, eNB_rxtx_proc_t *proc) {
	int frame = proc->frame_tx;
	int subframe = proc->subframe_tx;
	LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
	
	uint16_t i;

  float_t data_block_length = 1200*(fp->ofdm_symbol_size/2048);
  uint16_t *data_block = (uint16_t*)malloc(data_block_length*sizeof(uint16_t));

  // Caller: RCC - DL *** handle RRU case - UL and PRACH *** 
  if (eNB->node_function == NGFI_RCC_IF4) {
    IF4_dl_packet_t *dl_packet = (IF4_dl_packet_t*)malloc(sizeof_IF4_dl_packet_t);
    gen_IF4_dl_packet(dl_packet, proc);
		
    dl_packet->data_block = data_block;

    for(i=0; i<fp->symbols_per_tti; i++) {
			
      //Do compression of the two parts and generate data blocks

      //symbol = eNB->common_vars.txdataF[0][0 /*antenna number*/][subframe*fp->ofdm_symbol_size*(fp->symbols_per_tti)]
      //data_block[j] = Atan(symbol[fp->ofmd_symbol_size - NrOfNonZeroValues + j -1])<<16 + Atan(symbol[fp->ofmd_symbol_size - NrOfNonZeroValues + j]);
      //data_block[j+NrOfNonZeroValues] = Atan(subframe[i][j+1])<<16 + Atan(subframe[i][j+2]);
	 		
      // Update information in generated packet
      dl_packet->frame_status.sym_num = i; 
			
      // Write the packet(s) to the fronthaul 

    }
  }else {
    IF4_ul_packet_t *ul_packet = (IF4_ul_packet_t*)malloc(sizeof_IF4_ul_packet_t);
    gen_IF4_ul_packet(ul_packet, proc);
		
    ul_packet->data_block = data_block;

    for(i=0; i<fp->symbols_per_tti; i++) {
			
      //Do compression of the two parts and generate data blocks

      //symbol = eNB->common_vars.txdataF[0][0 /*antenna number*/][subframe*fp->ofdm_symbol_size*(fp->symbols_per_tti)]
      //data_block[j] = Atan(symbol[fp->ofmd_symbol_size - NrOfNonZeroValues + j -1])<<16 + Atan(symbol[fp->ofmd_symbol_size - NrOfNonZeroValues + j]);
      //data_block[j+NrOfNonZeroValues] = Atan(subframe[i][j+1])<<16 + Atan(subframe[i][j+2]);
			
      // Update information in generated packet
      ul_packet->frame_status.sym_num = i; 
			
      // Write the packet(s) to the fronthaul 

    }		
	}
  		    
}

void recv_IF4(PHY_VARS_eNB *eNB, eNB_rxtx_proc_t *proc) {

  // Read packet(s) from the fronthaul
  
  // Apply reverse processing - decompression
  
  // Generate and return the OFDM symbols (txdataF)

  // Caller: RRU - DL *** handle RCC case - UL and PRACH *** 
	  
}

void gen_IF4_dl_packet(IF4_dl_packet_t *dl_packet, eNB_rxtx_proc_t *proc) {      
  // Set Type and Sub-Type
  dl_packet->type = 0x080A; 
  dl_packet->sub_type = 0x0020;

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
  ul_packet->type = 0x080A; 
  ul_packet->sub_type = 0x0019;

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
  prach_packet->type = 0x080A; 
  prach_packet->sub_type = 0x0021;

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
