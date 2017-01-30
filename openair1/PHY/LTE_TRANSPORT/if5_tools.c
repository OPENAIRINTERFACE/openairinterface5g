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
* \author S. Sandeep Kumar, Raymond Knopp, Tien-Thinh Nguyen
* \date 2016
* \version 0.1
* \company Eurecom
* \email: ee13b1025@iith.ac.in, knopp@eurecom.fr, tien-thinh.nguyen@eurecom.fr 
* \note
* \warning
*/

#include "PHY/defs.h"

#include "targets/ARCH/ETHERNET/USERSPACE/LIB/if_defs.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
//#define DEBUG_DL_MOBIPASS
//#define DEBUG_UL_MOBIPASS
#define SUBFRAME_SKIP_NUM_MOBIPASS 8

int dummy_cnt = 0;
int subframe_skip_extra = 0;
int start_flag = 1;
int offset_cnt = 1;
void send_IF5(PHY_VARS_eNB *eNB, openair0_timestamp proc_timestamp, int subframe, uint8_t *seqno, uint16_t packet_type) {      
  
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int32_t *txp[fp->nb_antennas_tx], *rxp[fp->nb_antennas_rx]; 
  int32_t *tx_buffer=NULL;

  int8_t dummy_buffer[fp->samples_per_tti*2];
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
    uint16_t db_fulllength = PAYLOAD_MOBIPASS_NUM_SAMPLES;
    
    __m128i *data_block=NULL, *data_block_head=NULL;

    __m128i *txp128;
    __m128i t0, t1;

    // tx_buffer = memalign(16, MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t + db_fulllength*sizeof(int16_t));
    tx_buffer = malloc(MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t + db_fulllength*sizeof(int16_t));
    IF5_mobipass_header_t *header = (IF5_mobipass_header_t *)((uint8_t *)tx_buffer + MAC_HEADER_SIZE_BYTES);
    data_block_head = (__m128i *)((uint8_t *)tx_buffer + MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t);
  
    header->flags = 0;
    header->fifo_status = 0;  
    header->seqno = *seqno;
    header->ack = 0;
    header->word0 = 0;  
    
    txp[0] = (void*)&eNB->common_vars.txdata[0][0][subframe*eNB->frame_parms.samples_per_tti];
    txp128 = (__m128i *) txp[0];
              
    for (packet_id=0; packet_id<fp->samples_per_tti/db_fulllength; packet_id++) {
      header->time_stamp = htonl((uint32_t)(proc_timestamp + packet_id*db_fulllength));
      data_block = data_block_head; 
    
      for (i=0; i<db_fulllength>>2; i+=2) {
        t0 = _mm_srai_epi16(*txp128++, 4);
        t1 = _mm_srai_epi16(*txp128++, 4);   
//        *data_block++ = _mm_packs_epi16(t0, t1);     
       _mm_storeu_si128(data_block++, _mm_packs_epi16(t0, t1));     
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
#ifdef DEBUG_DL_MOBIPASS
     if ((subframe==0)&&(dummy_cnt == 100)) {
        memcpy((void*)&dummy_buffer[packet_id*db_fulllength*2],(void*)data_block_head,db_fulllength*2);
      }
#endif
      header->seqno += 1;    
    }  
    *seqno = header->seqno;

#ifdef DEBUG_DL_MOBIPASS
    uint8_t txe;
    txe = dB_fixed(signal_energy(txp[0],fp->samples_per_tti));
    if (txe > 0){
      LOG_D(PHY,"[Mobipass] frame:%d, subframe:%d, energy %d\n", (proc_timestamp/(10*fp->samples_per_tti))&1023,subframe, txe);
    }
#endif  
  } else {    
    AssertFatal(1==0, "send_IF5 - Unknown packet_type %x", packet_type);     
  }  
  
  free(tx_buffer);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_SEND_IF5, 0 );  
#ifdef DEBUG_DL_MOBIPASS 
  if(subframe==0) {
    if (dummy_cnt==100) {
      write_output("txsigmb.m","txs",(void*)dummy_buffer, fp->samples_per_tti,1, 5); 
      exit(-1);
    } else {
    dummy_cnt++;
    }
  }
#endif
  return;  		    
}


void recv_IF5(PHY_VARS_eNB *eNB, openair0_timestamp *proc_timestamp, int subframe, uint16_t packet_type) {

  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int32_t *txp[fp->nb_antennas_tx], *rxp[fp->nb_antennas_rx]; 

  uint16_t packet_id=0, i=0;
  int8_t dummy_buffer_rx[fp->samples_per_tti*2];
  uint8_t rxe;

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
    
    uint16_t db_fulllength = PAYLOAD_MOBIPASS_NUM_SAMPLES;
    openair0_timestamp timestamp_mobipass[fp->samples_per_tti/db_fulllength];
    int lower_offset = 0;
    int  upper_offset = 70000;
    int subframe_skip = 0;
    int reset_flag = 0;
    int32_t *rx_buffer=NULL;
    __m128i *data_block=NULL, *data_block_head=NULL;
    __m128i *rxp128;
    __m128i r0, r1;

    //rx_buffer = memalign(16, MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t + db_fulllength*sizeof(int16_t));
    rx_buffer = malloc(MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t + db_fulllength*sizeof(int16_t));
    IF5_mobipass_header_t *header = (IF5_mobipass_header_t *)((uint8_t *)rx_buffer + MAC_HEADER_SIZE_BYTES);
    data_block_head = (__m128i *)((uint8_t *)rx_buffer + MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t);
 
    rxp[0] = (void*)&eNB->common_vars.rxdata[0][0][subframe*eNB->frame_parms.samples_per_tti];
    rxp128 = (__m128i *) (rxp[0]);
 
    eNB_proc_t *proc = &eNB->proc;
/*
 //   while(packet_id<fp->samples_per_tti/db_fulllength) {
      data_block = data_block_head;

      eNB->ifdevice.trx_read_func(&eNB->ifdevice,
                                       &ts0,
                                       (void**)&rx_buffer,
                                       db_fulllength,
                                        1
                                        );

      if ((header->seqno == 1)&&(first_packet==1))  { 
         first_packet = 0;  //ignore the packets before synchnorization
         packet_id = 0;
        ts_offset = ntohl(ts0);
      } 
      if (first_packet==0) { 
        packet_cnt++;
        ts = ntohl(ts0);
        packet_id = (ts-ts_offset)/db_fulllength;
        packet_id = packet_id % (fp->samples_per_tti/db_fulllength);

        printf("[IF5_tools]packet_id:%d\n", packet_id);
        // if (ts_stored == 0) {
        //   ts_stored = 1;
        *proc_timestamp = ntohl(ts - (packet_id*db_fulllength));
        // }
        rxp[0] = (void*)&eNB->common_vars.rxdata[0][0][(subframe*eNB->frame_parms.samples_per_tti)+packet_id*db_fulllength];
        rxp128 = (__m128i *) (rxp[0]);

        for (i=0; i<db_fulllength>>2; i+=2) {
          r0 = _mm_loadu_si128(data_block++);
          *rxp128++ =_mm_slli_epi16(_mm_srai_epi16(_mm_unpacklo_epi8(r0,r0),8),4);
          *rxp128++ =_mm_slli_epi16(_mm_srai_epi16(_mm_unpackhi_epi8(r0,r0),8),4);
        }
      }
  //  }//end while
*/
 

    packet_id=0; 
    while(packet_id<fp->samples_per_tti/db_fulllength) {
      data_block = data_block_head;

      eNB->ifdevice.trx_read_func(&eNB->ifdevice,
                                       &timestamp_mobipass[packet_id],
                                       (void**)&rx_buffer,
                                       db_fulllength,
                                        1
                                        );
#ifdef DEBUG_UL_MOBIPASS
      if (((proc->timestamp_tx + lower_offset) > ntohl(timestamp_mobipass[packet_id])) || ((proc->timestamp_tx + upper_offset) < ntohl(timestamp_mobipass[packet_id]))) {
        //ignore the packet
        subframe_skip_extra = (subframe_skip_extra + 1)%67;         
       LOG_D("[Mobipass] ignored packet, id:[%d,%d], proc->timestamp_tx:%llu, proc->timestamp_rx:%llu, seqno:%d\n", packet_id,subframe_skip_extra, proc->timestamp_tx, ntohl(timestamp_mobipass[packet_id]), header->seqno);
      }             
#endif
      //skip SUBFRAME_SKIP_NUM_MOBIPASS additional UL packets
      if ((start_flag == 1) && (subframe_skip < SUBFRAME_SKIP_NUM_MOBIPASS)){
        subframe_skip++;
        offset_cnt = header->seqno;
      } else {
        if ((offset_cnt != header->seqno) && (start_flag == 0) && (proc->first_rx > 3)){
#ifdef DEBUG_UL_MOBIPASS
           LOG_D(PHY,"[Mobipass] Reset sequence number, offset_cnt:%d, header->seqno:%d, packet_id:%d\n", offset_cnt, header->seqno, packet_id);
#endif
           reset_flag=1;
        }
        if ((reset_flag == 1) && (proc->first_rx > 3 ) && (start_flag == 0) && (packet_id == 0)) {
           packet_id = 1;  
           reset_flag = 0;
        }
        start_flag = 0;

        //store rxdata and increase packet_id
        rxp[0] = (void*)&eNB->common_vars.rxdata[0][0][(subframe*eNB->frame_parms.samples_per_tti)+packet_id*db_fulllength];
        rxp128 = (__m128i *) (rxp[0]);
        for (i=0; i<db_fulllength>>2; i+=2) {
          r0 = _mm_loadu_si128(data_block++);
          *rxp128++ =_mm_slli_epi16(_mm_srai_epi16(_mm_unpacklo_epi8(r0,r0),8),4);
          *rxp128++ =_mm_slli_epi16(_mm_srai_epi16(_mm_unpackhi_epi8(r0,r0),8),4);
        }   
        packet_id++; 
        offset_cnt = (header->seqno+1)&255;
      }
    }//end while
  
      *proc_timestamp = ntohl(timestamp_mobipass[0]); 
#ifdef DEBUG_UL_MOBIPASS
   LOG_I(PHY,"[Mobipass][Recv_MOBIPASS] timestamp: %llu\n ",  *proc_timestamp);
if (eNB->CC_id>0) {
    rxe = dB_fixed(signal_energy(rxp[0],fp->samples_per_tti)); 
    if (rxe > 0){
      LOG_I(PHY,"[Mobipass] frame:%d, subframe:%d, energy %d\n", (*proc_timestamp/(10*fp->samples_per_tti))&1023,subframe, rxe);
//    write_output("rxsigmb.m","rxs",(void*)dummy_buffer_rx, fp->samples_per_tti,1, 5); 
//    exit(-1);
    }
}
#endif


   
  } else {
    AssertFatal(1==0, "recv_IF5 - Unknown packet_type %x", packet_type);     
  }  

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_RECV_IF5, 0 );  
  
  return;  
}
