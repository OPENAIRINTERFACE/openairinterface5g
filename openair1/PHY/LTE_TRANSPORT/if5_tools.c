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


void send_IF5(PHY_VARS_eNB *eNB, eNB_rxtx_proc_t *proc, uint8_t *seqno, uint16_t packet_type) {      
  
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  void *txp[fp->nb_antennas_tx]; 
  void *tx_buffer=NULL;

  uint16_t packet_id=0, i=0;
  
  if (packet_type == IF5_RRH_GW) {

    unsigned int spp_eth  = eNB->ifdevice.openair0_cfg->samples_per_packet;
    unsigned int spsf     = eNB->ifdevice.openair0_cfg->samples_per_frame/10;
    
    for (i=0; i < fp->nb_antennas_tx; i++)
      txp[i] = (void*)&eNB->common_vars.txdata[0][i][proc->subframe_tx*fp->samples_per_tti];
    
    for (packet_id=0; packet_id < spsf / spp_eth; packet_id++) {
      eNB->ifdevice.trx_write_func(&eNB->ifdevice,
                                   (proc->timestamp_tx-eNB->ifdevice.openair0_cfg->tx_sample_advance + packet_id*spp_eth),
                                   txp,
                                   spp_eth,
                                   fp->nb_antennas_tx,
                                   0);
    }
    
    printf("SPP = %d and packet_id %d\n", spp_eth, packet_id);

  } else if (packet_type == IF5_MOBIPASS) {
    
    uint16_t db_fulllength=640;
    
    __m128i *data_block=NULL, *data_block_head=NULL;

    __m128i *txp128;
    __m128i t0, t1;

    tx_buffer = memalign(16, MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t + db_fulllength*sizeof(int16_t));
    IF5_mobipass_header_t *header = (IF5_mobipass_header_t *)(tx_buffer + MAC_HEADER_SIZE_BYTES);
    data_block_head = (__m128i *)(tx_buffer + MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t + 4);
    
    header->flags = 0;
    header->fifo_status = 0;  
    header->seqno = *seqno;
    header->ack = 0;
    header->word0 = 0;  
    
    txp[0] = (void*)&eNB->common_vars.txdata[0][0][proc->subframe_tx*eNB->frame_parms.samples_per_tti];
    txp128 = (__m128i *) txp[0];
              
    for (packet_id=0; packet_id<(fp->samples_per_tti*2)/db_fulllength; packet_id++) {
      header->time_stamp = proc->timestamp_tx + packet_id * db_fulllength; 
      data_block = data_block_head; 
    
      for (i=0; i<db_fulllength>>3; i+=2) {
        t0 = _mm_srli_epi16(*txp128++, 4);
        t1 = _mm_srli_epi16(*txp128++, 4);   
        
        *data_block++ = _mm_packs_epi16(t0, t1);     
      }
      
      // Write the packet to the fronthaul
      if ((eNB->ifdevice.trx_write_func(&eNB->ifdevice,
                                        packet_id,
                                        &tx_buffer,
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
  return;  		    
}


void recv_IF5(PHY_VARS_eNB *eNB, eNB_proc_t *proc, uint16_t packet_type) {

  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  void *rxp[fp->nb_antennas_rx]; 
  void *rx_buffer=NULL;

  uint16_t packet_id=0, i=0;
  
  if (packet_type == IF5_RRH_GW) {

    unsigned int spp_eth  = eNB->ifdevice.openair0_cfg->samples_per_packet;
    unsigned int spsf     = eNB->ifdevice.openair0_cfg->samples_per_frame/10;
    
    openair0_timestamp timestamp[spsf / spp_eth];
    
    for (i=0; i < fp->nb_antennas_rx; i++)
      rxp[i] = (void*)&eNB->common_vars.rxdata[0][i][proc->subframe_rx*fp->samples_per_tti];
    
    for (packet_id=0; packet_id < spsf / spp_eth; packet_id++) {
      eNB->ifdevice.trx_read_func(&eNB->ifdevice,
                                  &timestamp[packet_id],
                                  rxp,
                                  spp_eth,
                                  fp->nb_antennas_rx);
    }
    
//    proc->timestamp_rx = timestamp[0];
    
  } else if (packet_type == IF5_MOBIPASS) {
    
    
  } else {
    AssertFatal(1==0, "recv_IF5 - Unknown packet_type %x", packet_type);     
  }  
  
  free(rx_buffer);  
  return;  
}
