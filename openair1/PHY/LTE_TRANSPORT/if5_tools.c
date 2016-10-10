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

/*! \file PHY/LTE_TRANSPORT/if5_tools.c
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

#include "targets/ARCH/ETHERNET/USERSPACE/LIB/if_defs.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

void send_IF5(PHY_VARS_eNB *eNB, openair0_timestamp proc_timestamp, int subframe, uint8_t *seqno, uint16_t packet_type) {      
  
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int32_t *txp[fp->nb_antennas_tx], *rxp[fp->nb_antennas_rx]; 
  int32_t *tx_buffer=NULL;

  uint16_t packet_id=0, i=0;

  uint32_t spp_eth  = (uint32_t) eNB->ifdevice.openair0_cfg->samples_per_packet;
  uint32_t spsf     = (uint32_t) eNB->ifdevice.openair0_cfg->samples_per_frame/10;
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_SEND_IF5, 1 );  

  if (packet_type == IF5_RRH_GW_DL) {    

    for (i=0; i < fp->nb_antennas_tx; i++)
      txp[i] = (void*)&eNB->common_vars.txdata[0][i][subframe*fp->samples_per_tti];
    
    for (packet_id=0; packet_id < spsf / spp_eth; packet_id++) {

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE_IF, 1 );            
      eNB->ifdevice.trx_write_func(&eNB->ifdevice,
                                   (proc_timestamp + packet_id*spp_eth),
                                   (void**)txp,
                                   spp_eth,
                                   fp->nb_antennas_tx,
                                   0);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE_IF, 0 );  
      for (i=0; i < fp->nb_antennas_tx; i++)
        txp[i] += spp_eth;

    }
    
  } else if (packet_type == IF5_RRH_GW_UL) {
        
    for (i=0; i < fp->nb_antennas_rx; i++)
      rxp[i] = (void*)&eNB->common_vars.rxdata[0][i][subframe*fp->samples_per_tti];
    
    for (packet_id=0; packet_id < spsf / spp_eth; packet_id++) {

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE_IF, 1 );            
      eNB->ifdevice.trx_write_func(&eNB->ifdevice,
                                   (proc_timestamp + packet_id*spp_eth),
                                   (void**)rxp,
                                   spp_eth,
                                   fp->nb_antennas_rx,
                                   0);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE_IF, 0 );            
      for (i=0; i < fp->nb_antennas_rx; i++)
        rxp[i] += spp_eth;

    }    
    
  } else if (packet_type == IF5_MOBIPASS) {    
    uint16_t db_fulllength=640;
    
    __m128i *data_block=NULL, *data_block_head=NULL;

    __m128i *txp128;
    __m128i t0, t1;

    tx_buffer = memalign(16, MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t + db_fulllength*sizeof(int16_t));
    IF5_mobipass_header_t *header = (IF5_mobipass_header_t *)((uint8_t *)tx_buffer + MAC_HEADER_SIZE_BYTES);
    data_block_head = (__m128i *)((uint8_t *)tx_buffer + MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t + 4);
    
    header->flags = 0;
    header->fifo_status = 0;  
    header->seqno = *seqno;
    header->ack = 0;
    header->word0 = 0;  
    
    txp[0] = (void*)&eNB->common_vars.txdata[0][0][subframe*eNB->frame_parms.samples_per_tti];
    txp128 = (__m128i *) txp[0];
              
    for (packet_id=0; packet_id<fp->samples_per_tti/db_fulllength; packet_id++) {
      header->time_stamp = (uint32_t)(proc_timestamp + packet_id*db_fulllength);
      data_block = data_block_head; 
    
      for (i=0; i<db_fulllength>>2; i+=2) {
        t0 = _mm_srai_epi16(*txp128++, 4);
        t1 = _mm_srai_epi16(*txp128++, 4);   
        
        *data_block++ = _mm_packs_epi16(t0, t1);     
      }
      
      // Write the packet to the fronthaul
      if ((eNB->ifdevice.trx_write_func(&eNB->ifdevice,
                                        packet_id,
                                        (void**)&tx_buffer,
                                        db_fulllength,
                                        1,
                                        IF5_MOBIPASS)) < 0) {
        perror("ETHERNET write for IF5_MOBIPASS\n");
      }
    
      header->seqno += 1;    
    }  
    *seqno = header->seqno;
    
  } else {    
    AssertFatal(1==0, "send_IF5 - Unknown packet_type %x", packet_type);     
  }  
  
  free(tx_buffer);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_SEND_IF5, 0 );  

  return;  		    
}


void recv_IF5(PHY_VARS_eNB *eNB, openair0_timestamp *proc_timestamp, int subframe, uint16_t packet_type) {

  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int32_t *txp[fp->nb_antennas_tx], *rxp[fp->nb_antennas_rx]; 

  uint16_t packet_id=0, i=0;

  int32_t spp_eth  = (int32_t) eNB->ifdevice.openair0_cfg->samples_per_packet;
  int32_t spsf     = (int32_t) eNB->ifdevice.openair0_cfg->samples_per_frame/10;

  openair0_timestamp timestamp[spsf / spp_eth];

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_RECV_IF5, 1 );  
  
  if (packet_type == IF5_RRH_GW_DL) {
        
    for (i=0; i < fp->nb_antennas_tx; i++)
      txp[i] = (void*)&eNB->common_vars.txdata[0][i][subframe*fp->samples_per_tti];
    
    for (packet_id=0; packet_id < spsf / spp_eth; packet_id++) {

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ_IF, 1 );  
      eNB->ifdevice.trx_read_func(&eNB->ifdevice,
                                  &timestamp[packet_id],
                                  (void**)txp,
                                  spp_eth,
                                  fp->nb_antennas_tx);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ_IF, 0 );  
      for (i=0; i < fp->nb_antennas_tx; i++)
        txp[i] += spp_eth;

    }
    
    *proc_timestamp = timestamp[0];
    
  } else if (packet_type == IF5_RRH_GW_UL) { 
    
    for (i=0; i < fp->nb_antennas_rx; i++)
      rxp[i] = (void*)&eNB->common_vars.rxdata[0][i][subframe*fp->samples_per_tti];
    
    for (packet_id=0; packet_id < spsf / spp_eth; packet_id++) {
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ_IF, 1 );            
      eNB->ifdevice.trx_read_func(&eNB->ifdevice,
                                  &timestamp[packet_id],
                                  (void**)rxp,
                                  spp_eth,
                                  fp->nb_antennas_rx);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ_IF, 0 );            
      for (i=0; i < fp->nb_antennas_rx; i++)
        rxp[i] += spp_eth;

    }

    *proc_timestamp = timestamp[0];
      
  } else if (packet_type == IF5_MOBIPASS) {
    
    
  } else {
    AssertFatal(1==0, "recv_IF5 - Unknown packet_type %x", packet_type);     
  }  

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_RECV_IF5, 0 );  
  
  return;  
}
