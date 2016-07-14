
#include <stdint.h>

#include "PHY/defs.h"
#include "PHY/LTE_TRANSPORT/if5_mobipass_tools.h"

#include "targets/ARCH/ETHERNET/USERSPACE/LIB/if_defs.h"


uint8_t send_IF5(PHY_VARS_eNB *eNB, eNB_rxtx_proc_t *proc, uint8_t init_seq) {      
  
  uint8_t seqno=init_seq;
  void *txp[2]; 
  void *tx_buffer=NULL;
  __m128i *data_block=NULL,*main_data_block=NULL;

  __m128i *txp128;
  __m128i t0, t1;

  uint16_t packet_id=0, i;
  uint16_t db_fulllength = 640;

  tx_buffer = memalign(16, MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t + db_fulllength*sizeof(int16_t));
  IF5_mobipass_header_t *header = (IF5_mobipass_header_t *)(tx_buffer + MAC_HEADER_SIZE_BYTES);
  data_block = (__m128i *)(tx_buffer + MAC_HEADER_SIZE_BYTES + sizeof_IF5_mobipass_header_t + 4);
  main_data_block = data_block;
  
  header->flags = 0;
  header->fifo_status = 0;  
  header->ack = 0;
  header->seqno = seqno;
  header->rsvd = 0;  

  txp[0] = (void*)&eNB->common_vars.txdata[0][0][proc->subframe_tx*eNB->frame_parms.samples_per_tti];
  txp128 = (__m128i *) txp[0];
    		    
  for (packet_id=0; packet_id<(7680*2)/640; packet_id++) {
    header->time_stamp = proc->timestamp_tx + packet_id*640; 
    data_block = main_data_block; 

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
  
  seqno = header->seqno;
  free(tx_buffer);
  return(seqno);  		    
}

