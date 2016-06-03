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
* \author Mauricio Gunther, Raymond Knopp, S. Sandeep Kumar
* \date 2016
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr, ee13b1025@iith.ac.in
* \note
* \warning
*/

#include "PHY/LTE_TRANSPORT/if4_tools.h"

/*
typedef struct data_block_type {
    
} data_block;
*/

/*
void allocate_data_block(long *data_block ,int length){
	data_block = malloc(length*sizeof(long));
}
*/

void send_IF4(PHY_VARS_eNB *eNB, int subframe){
	eNB_proc_t *proc = &eNB->proc;
	//int frame=proc->frame_tx;
	//int subframe=proc->subframe_tx;

	LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
	int i,j;

	float *data_block = malloc(length*sizeof(long));

	
	// find number of consecutive non zero in values in symbol
	// NrOfNonZeroValues
	
	// how many values does the atan function output?
	
	for(i = 0; i <= fp->symbols_per_tti; i++){
		for(j = 0; j < NrOfNonZeroValues; j=j+2){  
			
			symbol = eNB->common_vars.txdataF[0][0 /*antenna number*/][subframe*fp->ofdm_symbol_size*(fp->symbols_per_tti)]
			
			data_block[j] = Atan(symbol[fp->ofmd_symbol_size - NrOfNonZeroValues + j -1])<<16 + Atan(symbol[fp->ofmd_symbol_size - NrOfNonZeroValues + j]);
			data_block[j+NrOfNonZeroValues] = Atan(subframe[i][j+1])<<16 + Atan(subframe[i][j+2]);
			// use memset?
		}
		
	}

/*
memset(&eNB->common_vars.txdataF[0][aa][subframe*fp->ofdm_symbol_size*(fp->symbols_per_tti)],
             0,fp->ofdm_symbol_size*(fp->symbols_per_tti)*sizeof(int32_t));
    }
  */  
    
}

void recv_IF4( ) {
  
}

IF4_dl_packet gen_IF4_dl_packet(  ) {
  
  IF4_dl_packet dl_packet;
  
  return dl_packet;
}

IF4_ul_packet gen_IF4_ul_packet(  ) {
  
  IF4_ul_packet ul_packet;
  
  return ul_packet;
}

IF4_prach_packet gen_IF4_prach_packet(  ) {
  
  IF4_prach_packet prach_packet;
  
  return prach_packet;
} 
