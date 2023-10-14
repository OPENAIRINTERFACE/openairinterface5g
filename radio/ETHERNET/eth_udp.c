/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
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

/*! \file ethernet_lib.c 
 * \brief API to stream I/Q samples over standard ethernet
 * \author  add alcatel Katerina Trilyraki, Navid Nikaein, Pedro Dinis, Lucio Ferreira, Raymond Knopp
 * \date 2015
 * \version 0.2
 * \company Eurecom
 * \maintainer:  navid.nikaein@eurecom.fr
 * \note
 * \warning 
 */

// _GNU_SOURCE needed to have sched_getcpu() from sched.h
#define _GNU_SOURCE

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/filter.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

#include "common_lib.h"
#include "ethernet_lib.h"
#include "openair1/PHY/sse_intrin.h"
#include "common/utils/threadPool/thread-pool.h"

//#define DEBUG 1

// These are for IF5 and must be put into the device structure if multiple RUs in the same RAU !!!!!!!!!!!!!!!!!
uint16_t pck_seq_num = 1;
uint16_t pck_seq_num_cur=0;
uint16_t pck_seq_num_prev=0;

int eth_socket_init_udp(openair0_device *device) {

  eth_state_t *eth = (eth_state_t*)device->priv;
  eth_params_t *eth_params = device->eth_params;
 
  char str_local[INET_ADDRSTRLEN];
  char str_remote[INET_ADDRSTRLEN];

  const char *local_ip, *remote_ipc,*remote_ipd;
  int local_portc=0,local_portd=0, remote_portc=0,remote_portd=0;
  int sock_dom=0;
  int sock_type=0;
  int sock_proto=0;
  int enable=1;
  const char str[2][4] = {"RRU\0","RAU\0"};
  int hostind = 0;

  local_ip     = eth_params->my_addr;   
  local_portc  = eth_params->my_portc;
  local_portd  = eth_params->my_portd;
  

  if (device->host_type == RRU_HOST ) {

    remote_ipc   = "0.0.0.0";   
    remote_ipd   = eth_params->remote_addr;   
    remote_portc =  0;   
    remote_portd =  eth_params->remote_portd;;   
    printf("[%s] local ip addr %s portc %d portd %d\n", "RRU", local_ip, local_portc, local_portd);    
  } else { 
    remote_ipc   = eth_params->remote_addr;   
    remote_ipd   = "0.0.0.0";   
    remote_portc = eth_params->remote_portc;   
    remote_portd = 0;
    hostind      = 1;
    printf("[%s] local ip addr %s portc %d portd %d\n","RAU", local_ip, local_portc, local_portd);    
  }
  
  /* Open socket to send on */
  sock_dom=AF_INET;
  sock_type=SOCK_DGRAM;
  sock_proto=IPPROTO_UDP;
  
  if ((eth->sockfdc = socket(sock_dom, sock_type, sock_proto)) == -1) {
    perror("ETHERNET: Error opening socket (control)");
    exit(0);
  }

  for (int i = 0;i<eth->num_fd;i++)
    if ((eth->sockfdd[i] = socket(sock_dom, sock_type, sock_proto)) == -1) {
      printf("ETHERNET: Error opening socket (user %d)",i);
      exit(0);
    }
  
  /* initialize addresses */
  bzero((void *)&(eth->dest_addrc), sizeof(eth->dest_addrc));
  bzero((void *)&(eth->local_addrc), sizeof(eth->local_addrc));
  bzero((void *)&(eth->dest_addrd), sizeof(eth->dest_addrd));
  bzero((void *)&(eth->local_addrd), sizeof(eth->local_addrd));
  

  eth->addr_len = sizeof(struct sockaddr_in);

  eth->dest_addrc.sin_family = AF_INET;
  inet_pton(AF_INET,remote_ipc,&(eth->dest_addrc.sin_addr.s_addr));
  eth->dest_addrc.sin_port=htons(remote_portc);
  inet_ntop(AF_INET, &(eth->dest_addrc.sin_addr), str_remote, INET_ADDRSTRLEN);

  eth->dest_addrd.sin_family = AF_INET;
  inet_pton(AF_INET,remote_ipd,&(eth->dest_addrd.sin_addr.s_addr));
  eth->dest_addrd.sin_port=htons(remote_portd);

  eth->local_addrc.sin_family = AF_INET;
  inet_pton(AF_INET,local_ip,&(eth->local_addrc.sin_addr.s_addr));
  eth->local_addrc.sin_port=htons(local_portc);
  inet_ntop(AF_INET, &(eth->local_addrc.sin_addr), str_local, INET_ADDRSTRLEN);

  eth->local_addrd.sin_family = AF_INET;
  inet_pton(AF_INET,local_ip,&(eth->local_addrd.sin_addr.s_addr));
  eth->local_addrd.sin_port=htons(local_portd);



  
  /* set reuse address flag */
  if (setsockopt(eth->sockfdc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
    perror("ETHERNET: Cannot set SO_REUSEADDR option on socket (control)");
    exit(0);
  }
  for (int i=0;i<eth->num_fd;i++) {
    if (setsockopt(eth->sockfdd[i], SOL_SOCKET, SO_NO_CHECK, &enable, sizeof(int))) {
      printf("ETHERNET: Cannot set SO_NO_CHECK option on socket (user %d)",i);
      exit(0);
    }
#if 0 /*def SO_ATTACH_REUSEPORT_EBPF*/    
    if (setsockopt(eth->sockfdd[i], SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int))) {
      printf("ETHERNET: Cannot set SO_REUSEPORT option on socket (user %d)",i);
      exit(0);
    }
    struct sock_filter code[]={
              { BPF_LD  | BPF_W | BPF_ABS, 0, 0, SKF_AD_OFF + SKF_AD_CPU }, // A = #cpu
              { BPF_RET | BPF_A, 0, 0, 0 },                                 // return A
    };
    struct sock_fprog bpf = {
      .len = sizeof(code)/sizeof(struct sock_filter),
      .filter = code,
    };
    if (i==0 && setsockopt(eth->sockfdd[i], SOL_SOCKET, SO_ATTACH_REUSEPORT_CBPF, &bpf, sizeof(bpf))) {
      printf("ETHERNET: Cannot set SO_ATTACH_REUSEPORT_EBPF option on socket (user %d)",i);
      exit(0);
    }
    else printf("ETHERNET: set SO_ATTACH_REUSEPORT_EBPF option on socket (user %d)\n",i);
#endif

  }
  
  /* want to receive -> so bind */   
  if (bind(eth->sockfdc,(struct sockaddr *)&eth->local_addrc,eth->addr_len)<0) {
    perror("ETHERNET: Cannot bind to socket (control)");
    exit(0);
  } else {
    printf("[%s] binding to %s:%d (control)\n",str[hostind],str_local,ntohs(eth->local_addrc.sin_port));
  }
  for (int i=0;i<eth->num_fd;i++)
    if (bind(eth->sockfdd[i],(struct sockaddr *)&eth->local_addrd,eth->addr_len)<0) {
      printf("ETHERNET: Cannot bind to socket (user %d)",i);
      exit(0);
    } else {
      printf("[%s] binding to %s:%d (user %d)\n",str[hostind],str_local,ntohs(eth->local_addrd.sin_port),i);
    }
 
  return 0;
}

int trx_eth_read_udp_IF4p5(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc) {

  // Read nblocks info from packet itself
  int nblocks = nsamps;  
  int bytes_received=-1;
  eth_state_t *eth = (eth_state_t*)device->priv;

  ssize_t packet_size = sizeof_IF4p5_header_t;      
  IF4p5_header_t *test_header = (IF4p5_header_t*)(buff[0]);
  
  int block_cnt=0; 

  // *2 because of 2 antennas PUL/DLFFT are controlled by nsamps, PRACH is not
  packet_size = max(UDP_IF4p5_PRACH_SIZE_BYTES*2, max(UDP_IF4p5_PULFFT_SIZE_BYTES(nblocks), UDP_IF4p5_PDLFFT_SIZE_BYTES(nblocks)));

  while(bytes_received == -1) {
  again:
    bytes_received = recvfrom(eth->sockfdd[0/*cc%eth->num_fd*/],
                              buff[0],
                              packet_size,
                              0,
                              (struct sockaddr *)&eth->dest_addrd,
                              (socklen_t *)&eth->addr_len);
    if (bytes_received ==-1) {
      eth->num_rx_errors++;
      if (errno == EAGAIN) {
	printf("Lost IF4p5 connection with %s\n", inet_ntoa(eth->dest_addrd.sin_addr));
	return -1;
      } else if (errno == EWOULDBLOCK) {
        block_cnt++;
        usleep(10);
        if (block_cnt == 1000) {
          perror("ETHERNET IF4p5 READ (EWOULDBLOCK): ");
        } else {
          printf("BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK \n");
          goto again;
        }
      } else {
	return(-1);
        //perror("ETHERNET IF4p5 READ");
        //printf("(%s):\n", strerror(errno));
      }
    } else {
      *timestamp = test_header->sub_type;
      eth->rx_actual_nsamps = bytes_received>>1;
      eth->rx_count++;
    }
  }
  eth->rx_nsamps = nsamps;  
  return(bytes_received);
}

int trx_eth_write_udp_IF4p5(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags) {

  int nblocks = nsamps;  
  int bytes_sent = 0;

  
  eth_state_t *eth = (eth_state_t*)device->priv;
  
  ssize_t packet_size;

  char str[INET_ADDRSTRLEN];

  inet_ntop(AF_INET, &(eth->dest_addrd.sin_addr), str, INET_ADDRSTRLEN);
  
  if (flags == IF4p5_PDLFFT) {
    packet_size = UDP_IF4p5_PDLFFT_SIZE_BYTES(nblocks);    
  } else if (flags == IF4p5_PULFFT) {
    packet_size = UDP_IF4p5_PULFFT_SIZE_BYTES(nblocks); 
  } else if (flags == IF4p5_PULTICK) {
    packet_size = UDP_IF4p5_PULTICK_SIZE_BYTES; 
  } else if ((flags >= IF4p5_PRACH)&&
             (flags <= (IF4p5_PRACH+4))) {  
    packet_size = UDP_HEADER_SIZE_BYTES + IPV4_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t + (nsamps<<1);   
  } else {
    printf("trx_eth_write_udp_IF4p5: unknown flags %d\n",flags);
    return(-1);
  }
   
  eth->tx_nsamps = nblocks;

  bytes_sent = sendto(eth->sockfdd[0/*cc%eth->num_fd*/],
		      buff[0], 
		      packet_size,
		      0,
		      (struct sockaddr*)&eth->dest_addrd,
		      eth->addr_len);
  
  if (bytes_sent == -1) {
    eth->num_tx_errors++;
    perror("error writing to remote unit (user) : ");
    exit(-1);
  } else {
    eth->tx_actual_nsamps = bytes_sent>>1;
    eth->tx_count++;
  }
  return (bytes_sent);  	  
}



void *trx_eth_write_udp_cmd(udpTXelem_t *udpTXelem) {

  openair0_device *device=udpTXelem->device;
  openair0_timestamp timestamp = udpTXelem->timestamp;
  void **buff = udpTXelem->buff;
  int nsamps = udpTXelem->nsamps;
  int nant = udpTXelem->nant; 
  int bytes_sent=0;
  eth_state_t *eth = (eth_state_t*)device->priv;
  int sendto_flag =0;
  fhstate_t *fhstate = &device->fhstate;

  //sendto_flag|=flags;
  eth->tx_nsamps=nsamps;

  uint64_t last_rxTS = fhstate->TS[0]-fhstate->TS0;
  uint64_t TS_advance=0;
  if (timestamp > last_rxTS) TS_advance = timestamp - last_rxTS;
  else {
    LOG_W(PHY,"TS_advance is < 0  TS %llu absslot %llu(%llu) last_rxTS %llu TS_advance %llu samples\n", (unsigned long long)timestamp,(unsigned long long)timestamp/nsamps,((unsigned long long)timestamp/nsamps)%20,(unsigned long long)last_rxTS,(unsigned long long)TS_advance);
    free(buff);
    return(NULL);
  }
  if (TS_advance < (nsamps/2)) {
     LOG_W(PHY,"Starting TX FH for TS %llu absslot %llu(%llu) last_rxTS %llu TS_advance %llu samples\n",(unsigned long long)timestamp,(unsigned long long)timestamp/nsamps,((unsigned long long)timestamp/nsamps)%20,(unsigned long long)last_rxTS,(unsigned long long)TS_advance);
  }
  void *buff2;
  int nsamps2 = 256>>3;
  simde__m256i buff_tx[nsamps2 + 1];
  buff2 = (void *)&buff_tx[1] - APP_HEADER_SIZE_BYTES;

  /* construct application header */
  // ECPRI Protocol revision + reserved bits (1 byte)
  *(uint8_t *)buff2 = ECPRIREV;
   // ECPRI Message type (1 byte)
  *(uint8_t *)(buff2 + 1) = 64;

  openair0_timestamp TS = timestamp + fhstate->TS0;
  TS = (6*device->sampling_rate_ratio_d*TS)/device->sampling_rate_ratio_n;
  TS -= device->txrx_offset; 
  int TSinc = (6*256*device->sampling_rate_ratio_d)/device->sampling_rate_ratio_n;
  int len=256;
  LOG_D(NR_PHY,"in eth send: TS %llu (%llu),txrx_offset %d,d %d, n %d, buff[0] %p buff[1] %p\n",
        (unsigned long long)TS,(unsigned long long)timestamp,device->txrx_offset,device->sampling_rate_ratio_d,device->sampling_rate_ratio_n,
	buff[0],buff[1]);
  for (int offset=0;offset<nsamps;offset+=256,TS+=TSinc) {
    // OAI modified SEQ_ID (4 bytes)
    *(uint64_t *)(buff2 + 6) = TS;
    if ((offset + 256) <= nsamps) len=1024;
    else len = (nsamps-offset)<<2; 
    // ECPRI Payload Size (2 bytes)
    *(uint8_t *)(buff2 + 2) = len>>8;
    *(uint8_t *)(buff2 + 3) = len&0xff;
    for (int aid = 0; aid<nant; aid++) {
      LOG_D(NR_PHY,"TS %llu (TS0 %llu) aa %d : offset %d, len %d\n",(unsigned long long)TS,(unsigned long long)fhstate->TS0,aid,offset,len);
      // ECPRI PC_ID (2 bytes)
      *(uint16_t *)(buff2 + 4) = aid;
      // bring TX data into 12 MSBs
      simde__m256i *buff256 = (simde__m256i *)&(((int32_t *)buff[aid])[offset]);
      for (int j=0; j<32; j+=8) {
        buff_tx[1+j] = simde_mm256_slli_epi16(buff256[j],4);
        buff_tx[2+j] = simde_mm256_slli_epi16(buff256[j+1],4);
        buff_tx[3+j] = simde_mm256_slli_epi16(buff256[j+2],4);
        buff_tx[4+j] = simde_mm256_slli_epi16(buff256[j+3],4);
        buff_tx[5+j] = simde_mm256_slli_epi16(buff256[j+4],4);
        buff_tx[6+j] = simde_mm256_slli_epi16(buff256[j+5],4);
        buff_tx[7+j] = simde_mm256_slli_epi16(buff256[j+6],4);
        buff_tx[8+j] = simde_mm256_slli_epi16(buff256[j+7],4);
      }

       /* Send packet */
      bytes_sent = sendto(eth->sockfdd[0],
                          buff2, 
                          UDP_PACKET_SIZE_BYTES(len>>2),
                          sendto_flag,
                          (struct sockaddr*)&eth->dest_addrd,
                          eth->addr_len);
      if ( bytes_sent == -1) {
  	eth->num_tx_errors++;
	perror("ETHERNET WRITE: ");
	exit(-1);
      } else {
        eth->tx_actual_nsamps=bytes_sent>>2;
        eth->tx_count++;
      }
    } // aid
  } // offset
  free(buff);  
  LOG_D(NR_PHY,"Returning from eth send\n");
  return(NULL);
}

int trx_eth_write_udp(openair0_device *device, openair0_timestamp timestamp, void **buff, int fd_ind, int nsamps, int flags, int nant) {	

    union udpTXReqUnion id = {.s={(uint64_t)timestamp,nsamps,0}};
    notifiedFIFO_elt_t *req=newNotifiedFIFO_elt(sizeof(udpTXelem_t), id.p, device->utx[fd_ind]->resp,NULL);
    udpTXelem_t * udptxelem=(udpTXelem_t *) NotifiedFifoData(req);
    udptxelem->device = device;
    udptxelem->timestamp = timestamp;
    udptxelem->buff = calloc(nant,sizeof(void*));
    memcpy(udptxelem->buff,buff,nant*sizeof(void*));    
    udptxelem->fd_ind = fd_ind;
    udptxelem->nsamps = nsamps;
    udptxelem->flags = flags;
    udptxelem->nant = nant;
    pushNotifiedFIFO(device->utx[fd_ind]->resp, req);
    LOG_D(PHY,"Pushed to TX FH FIFO, TS %llu, nsamps %d, nant %d buffs[0] %p buffs[1] %p\n",
          (unsigned long long)timestamp,nsamps,nant,udptxelem->buff[0],udptxelem->buff[1]);
    return(0);
}
extern int oai_exit;

void *udp_write_thread(void *arg) {

   udp_ctx_t *utx = (udp_ctx_t *)arg;
   utx->resp = malloc(sizeof(*utx->resp));
   initNotifiedFIFO(utx->resp);
   LOG_D(PHY,"UDP write thread started on core %d\n",sched_getcpu()); 
   reset_meas(&utx->device->tx_fhaul);
   while (oai_exit == 0) {
      notifiedFIFO_elt_t *res = pullNotifiedFIFO(utx->resp);
      udpTXelem_t *udptxelem = (udpTXelem_t *)NotifiedFifoData(res);
      LOG_D(PHY,"Pulled from TX FH FIFO, TS %llu, nsamps %d, nant %d\n",(unsigned long long)udptxelem->timestamp,udptxelem->nsamps,udptxelem->nant);
      start_meas(&utx->device->tx_fhaul);
      trx_eth_write_udp_cmd(udptxelem);
      stop_meas(&utx->device->tx_fhaul);
      // send data to RU
      delNotifiedFIFO_elt(res);
   }
   free(utx->resp);
   return(NULL);
}

void *udp_read_thread(void *arg) {
  openair0_timestamp TS;

  int aid;
  udp_ctx_t *u = (udp_ctx_t *)arg;
  openair0_device *device=u->device;
  fhstate_t *fhstate = &device->fhstate;
  char buffer[UDP_PACKET_SIZE_BYTES(256)];
  int first_read=0;
  while (oai_exit == 0) {
    LOG_I(PHY,"UDP read thread %d on core %d, waiting for start sampling_rate_d %d, sampling_rate_n %d\n",u->thread_id,sched_getcpu(),device->sampling_rate_ratio_n,device->sampling_rate_ratio_d);
    while (fhstate->active > 0) {
      ssize_t count = recvfrom(((eth_state_t*)device->priv)->sockfdd[0],
                               buffer,sizeof(buffer),0,
                               (struct sockaddr *)&((eth_state_t*)device->priv)->dest_addrd,
                               (socklen_t *)&((eth_state_t*)device->priv)->addr_len);
      /* log and skip processing in case of error from recvfrom */
      /* (to be refined if needed) */
      if (count == 0) {
        LOG_E(PHY, "recvfrom returned 0\n");
        continue;
      }
      if (count < 0) {
        LOG_E(PHY, "recvfrom failed (%s)\n", strerror(errno));
        continue;
      }
      /* if oai_exit is 1 here, don't access the array rxbase,
       * it may have been freed(), so let's break at this point
       */
      if (oai_exit)
        break;
      aid = *(uint16_t*)(&buffer[ECPRICOMMON_BYTES]);
      TS  = *(openair0_timestamp *)(&buffer[ECPRICOMMON_BYTES+ECPRIPCID_BYTES]);   
      // convert TS to samples, /6 for AW2S @ 30.72 Ms/s, this is converted for other sample rates in OAI application
      TS = (device->sampling_rate_ratio_n*TS)/(device->sampling_rate_ratio_d*6);
      AssertFatal(aid < 8,"Cannot handle more than 8 antennas, got aid %d\n",aid);
      fhstate->r[aid]=1;
      if (aid==0 && first_read == 0) fhstate->TS0 = TS;
      first_read = 1;
      /* store the timestamp value from packet's header */
      fhstate->TS[aid] =  TS;
      int64_t offset =  TS - fhstate->TS0;
      if (offset > 0) offset = offset % device->openair0_cfg->rxsize;
      else offset = TS % device->openair0_cfg->rxsize + ((((uint64_t)1)<<63)-(fhstate->TS0-1)) % device->openair0_cfg->rxsize;   
      // need to do memcpy since there is no guarantee that aid is the same each time, otherwise we could have used
      // zero-copy and corrected the header component.   	 

      memcpy((void*)(device->openair0_cfg->rxbase[aid]+offset),
             (void*)&buffer[APP_HEADER_SIZE_BYTES],
             count-APP_HEADER_SIZE_BYTES);
      LOG_D(PHY,"UDP read thread_id %d (%d), aid %d, TS %llu, TS0 %llu, offset %ld\n",(int)u->thread_id,(int)sched_getcpu(),aid,(unsigned long long)TS,(unsigned long long)fhstate->TS0,offset);
    }
    sleep(1);
  }

  /* let's unblock reader (maybe not the best way to do it) */
  fhstate->first_read = 0;
  fhstate->r[0] = 1;

  return(0);  
}

int trx_eth_read_udp(openair0_device *device, openair0_timestamp *timestamp, uint32_t **buff, int nsamps) {
  
  fhstate_t *fhstate = &device->fhstate;
  openair0_timestamp prev_read_TS= fhstate->TS_read;
  volatile openair0_timestamp min_TS;
  // block until FH is ready
  while (fhstate->r[0] == 0 || fhstate->r[1] == 0 || fhstate->r[2] == 0 || fhstate->r[3] == 0 ||
         fhstate->r[4] == 0 || fhstate->r[5] == 0 || fhstate->r[6] == 0 || fhstate->r[7] == 0) usleep(100);

  // get minimum TS over all antennas
  min_TS = (volatile openair0_timestamp)fhstate->TS[0];
  for (int i=1;i<device->openair0_cfg->rx_num_channels;i++) min_TS = min(min_TS,fhstate->TS[i]);
  // poll/sleep until we accumulated enough samples on each antenna port
  int count=0;
  while (fhstate->first_read == 1 && min_TS < (fhstate->TS0+prev_read_TS + nsamps)) {
    usleep(10);
    min_TS = (volatile openair0_timestamp)fhstate->TS[0];
    for (int i=1;i<device->openair0_cfg->rx_num_channels;i++) min_TS = min(min_TS,(volatile openair0_timestamp)fhstate->TS[i]);
    count++;
  }
  if (fhstate->first_read == 0) {
     *timestamp = 0;
     fhstate->TS_read = *timestamp+nsamps;
     LOG_D(PHY,"first read : TS_read %llu, TS %llu state (%d,%d,%d,%d,%d,%d,%d,%d)\n",(unsigned long long)fhstate->TS_read,(unsigned long long)*timestamp,
           fhstate->r[0],fhstate->r[1],fhstate->r[2],fhstate->r[3],fhstate->r[4],fhstate->r[5],fhstate->r[6],fhstate->r[7]);
  }
  else {  
     *timestamp = fhstate->TS_read;
     fhstate->TS_read = prev_read_TS + nsamps;
     LOG_D(PHY,"TS_read %llu (%llu, %llu), min_TS %llu, prev_read_TS %llu, nsamps %d, fhstate->TS0+prev_TS+nsamps %llu, wait count %d x 100us\n",(unsigned long long)fhstate->TS_read,(unsigned long long)*timestamp/nsamps,((unsigned long long)*timestamp/nsamps)%20,(unsigned long long)min_TS,(unsigned long long)prev_read_TS,nsamps,(unsigned long long)(fhstate->TS0+prev_read_TS+nsamps),count);
  }
  fhstate->first_read = 1;
  return (nsamps);
}



int trx_eth_ctlsend_udp(openair0_device *device, void *msg, ssize_t msg_len) {

  return(sendto(((eth_state_t*)device->priv)->sockfdc,
		msg,
		msg_len,
		0,
		(struct sockaddr *)&((eth_state_t*)device->priv)->dest_addrc,
		((eth_state_t*)device->priv)->addr_len));
}


int trx_eth_ctlrecv_udp(openair0_device *device, void *msg, ssize_t msg_len) {
  
  return (recvfrom(((eth_state_t*)device->priv)->sockfdc,
		   msg,
		   msg_len,
		   0,
		   (struct sockaddr *)&((eth_state_t*)device->priv)->dest_addrc,
		   (socklen_t *)&((eth_state_t*)device->priv)->addr_len));
}
