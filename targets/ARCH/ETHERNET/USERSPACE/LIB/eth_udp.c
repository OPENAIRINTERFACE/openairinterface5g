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

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <errno.h>
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "common_lib.h"
#include "ethernet_lib.h"

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

  if ((eth->sockfdd = socket(sock_dom, sock_type, sock_proto)) == -1) {
    perror("ETHERNET: Error opening socket (user)");
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
  if (setsockopt(eth->sockfdd, SOL_SOCKET, SO_NO_CHECK, &enable, sizeof(int))) {
    perror("ETHERNET: Cannot set SO_NO_CHECK option on socket (user)");
    exit(0);
  }
  
  /* want to receive -> so bind */   
  if (bind(eth->sockfdc,(struct sockaddr *)&eth->local_addrc,eth->addr_len)<0) {
    perror("ETHERNET: Cannot bind to socket (control)");
    exit(0);
  } else {
    printf("[%s] binding to %s:%d (control)\n",str[hostind],str_local,ntohs(eth->local_addrc.sin_port));
  }
  if (bind(eth->sockfdd,(struct sockaddr *)&eth->local_addrd,eth->addr_len)<0) {
    perror("ETHERNET: Cannot bind to socket (user)");
    exit(0);
  } else {
    printf("[%s] binding to %s:%d (user)\n",str[hostind],str_local,ntohs(eth->local_addrd.sin_port));
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
    bytes_received = recvfrom(eth->sockfdd,
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

  bytes_sent = sendto(eth->sockfdd,
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

int trx_eth_write_udp(openair0_device *device, openair0_timestamp timestamp, void *buff, int nsamps,int cc, int flags) {	
  
  int bytes_sent=0;
  eth_state_t *eth = (eth_state_t*)device->priv;
  int sendto_flag =0;

  //sendto_flag|=flags;
  eth->tx_nsamps=nsamps;

 


  int nsamps2;  // aligned to upper 32 or 16 byte boundary
  
#if defined(__x86_64) || defined(__i386__)
#ifdef __AVX2__
  nsamps2 = (nsamps+7)>>3;
  __m256i buff_tx[nsamps2+1];
  __m256i *buff_tx2=buff_tx+1;
#else
  nsamps2 = (nsamps+3)>>2;
  __m128i buff_tx[nsamps2+2];
  __m128i *buff_tx2=buff_tx+2;
#endif
#elif defined(__arm__) || defined(__aarch64__)
  nsamps2 = (nsamps+3)>>2;
  int16x8_t buff_tx[nsamps2+2];
  int16x8_t *buff_tx2=buff_tx+2;
#else
#error Unsupported CPU architecture, ethernet device cannot be built
#endif

    
    // bring TX data into 12 LSBs for softmodem RX
  for (int j=0; j<nsamps2; j++) {
#if defined(__x86_64__) || defined(__i386__)
#ifdef __AVX2__
    buff_tx2[j] = _mm256_slli_epi16(((__m256i *)buff)[j],4);
#else
    buff_tx2[j] = _mm_slli_epi16(((__m128i *)buff)[j],4);
#endif
#elif defined(__arm__)
    buff_tx2[j] = vshlq_n_s16(((int16x8_t *)buff)[j],4);
#endif
  }

        /* buff[i] points to the position in tx buffer where the payload to be sent is
       buff2 points to the position in tx buffer where the packet header will be placed */
    void *buff2 = ((void*)buff_tx2)- APP_HEADER_SIZE_BYTES; 
    
   
 
    bytes_sent = 0;
    
    /* constract application header */
    // ECPRI Protocol revision + reserved bits (1 byte)
    *(uint8_t *)buff2 = ECPRIREV;
    // ECPRI Message type (1 byte)
    *(uint8_t *)(buff2 + 1) = 64;
    // ECPRI Payload Size (2 bytes)
    AssertFatal(nsamps<16381,"nsamps > 16381\n");
    *(uint8_t *)(buff2 + 2) = (nsamps<<2)>>8;
    *(uint8_t *)(buff2 + 3) = (nsamps<<2)&0xff;
    // ECPRI PC_ID (2 bytes)
    *(uint16_t *)(buff2 + 4) = cc;
    // OAI modified SEQ_ID (4 bytes)
    *(uint64_t *)(buff2 + 6) = ((uint64_t )timestamp)*6;

    /*
    printf("ECPRI TX (REV %x, MessType %d, Payload size %d, PC_ID %d, TS %llu\n",
	   *(uint8_t *)buff2,
	   *(uint8_t *)(buff2+1),
	   *(uint16_t *)(buff2+2),
	   *(uint16_t *)(buff2+4),
	   *(uint64_t *)(buff2+6));
	   */	   	   
    
    int sent_byte;
    if (eth->compression == ALAW_COMPRESS) {
      sent_byte = UDP_PACKET_SIZE_BYTES_ALAW(nsamps);
    } else {
      sent_byte = UDP_PACKET_SIZE_BYTES(nsamps);
    }

    //while(bytes_sent < sent_byte) {
    //printf("eth->pck_seq_num: %d\n", eth->pck_seq_num);
#if DEBUG   
      printf("------- TX ------: buff2 current position=%d remaining_bytes=%d  bytes_sent=%d \n",
	     (void *)(buff2+bytes_sent), 
	     sent_byte - bytes_sent,
	     bytes_sent);
#endif
      /* Send packet */
      bytes_sent = sendto(eth->sockfdd,
			   buff2, 
                           sent_byte,
			   sendto_flag,
			   (struct sockaddr*)&eth->dest_addrd,
			   eth->addr_len);
      
      if ( bytes_sent == -1) {
	eth->num_tx_errors++;
	perror("ETHERNET WRITE: ");
	exit(-1);
      } else {
#if DEBUG
    printf("------- TX ------: nu=%d an_id=%d ts%d bytes_send=%d\n",
	   *(int16_t *)buff2,
	   *(int16_t *)(buff2 + sizeof(int16_t)),
	   *(openair0_timestamp *)(buff2 + sizeof(int32_t)),
	   bytes_sent);

    dump_packet((device->host_type == RAU_HOST)? "RAU":"RAU", buff2, UDP_PACKET_SIZE_BYTES(nsamps), TX_FLAG);
#endif
    eth->tx_actual_nsamps=bytes_sent>>2;
    eth->tx_count++;
    eth->pck_seq_num++;
    if ( eth->pck_seq_num > MAX_PACKET_SEQ_NUM(nsamps,device->openair0_cfg->samples_per_frame) )  eth->pck_seq_num = 1;
      }
    //}
                  
  return (bytes_sent-APP_HEADER_SIZE_BYTES)>>2;
}
      

#define NOSHIFT 1

int trx_eth_read_udp(openair0_device *device, openair0_timestamp *timestamp, void *buff, int nsamps, int *cc) {
  
  int bytes_received=0;
  eth_state_t *eth = (eth_state_t*)device->priv;
  //  openair0_timestamp prev_timestamp = -1;
  int rcvfrom_flag =0;
  int block_cnt=0;
  int again_cnt=0;
  static int packet_cnt=0;
  int payload_size = UDP_PACKET_SIZE_BYTES(nsamps);

#if defined(__x86_64__) || defined(__i386__)
#ifdef __AVX2__
    int nsamps2 = (payload_size>>5)+1;
  __m256i temp_rx[nsamps2];
  char *temp_rx0 = ((char *)&temp_rx[1])-APP_HEADER_SIZE_BYTES;
#else
    int nsamps2 = (payload_size>>4)+1;
  __m128i temp_rx[nsamps2];
  char *temp_rx0 = ((char *)&temp_rx[1])-APP_HEADER_SIZE_BYTES;  
#endif
#elif defined(__arm__) || defined(__aarch64__)
  int nsamps2 = (payload_size>>4)+1;
  int16x8_t temp_rx[nsamps2];
  char *temp_rx0 = ((char *)&temp_rx[1])-APP_HEADER_SIZE_BYTES;  
#else
#error Unsupported CPU architecture device cannot be built
  int nsamps2 = (payload_size>>2)+1;
  int32_t temp_rx[payload_size>>2];
  char* *temp_rx0 = ((char *)&temp_rx[1]) - APP_HEADER_SIZE_BYTES;  
#endif
  
  eth->rx_nsamps=nsamps;

  bytes_received=0;
  block_cnt=0;
  AssertFatal(eth->compression == NO_COMPRESS, "IF5 compression not supported for now\n");
  
  while(bytes_received < payload_size) {
  again:
    bytes_received +=recvfrom(eth->sockfdd,
			      temp_rx0,
			      payload_size,
			      rcvfrom_flag,
			      (struct sockaddr *)&eth->dest_addrd,
			      (socklen_t *)&eth->addr_len);
    packet_cnt++;
    if (bytes_received ==-1) {
      eth->num_rx_errors++;
      if (errno == EAGAIN) {
	again_cnt++;
	usleep(10);
	if (again_cnt == 1000) {
	  perror("ETHERNET READ: ");
	  exit(-1);
	} else {
	  bytes_received=0;
	  goto again;
	}	  
      } else if (errno == EWOULDBLOCK) {
	block_cnt++;
	usleep(10);	  
	if (block_cnt == 1000) {
	  perror("ETHERNET READ: ");
	  exit(-1);
	} else {
	  printf("BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK \n");
	  goto again;
	}
      }
    } else {
      /* store the timestamp value from packet's header */
      *timestamp =  *(openair0_timestamp *)(temp_rx0 + ECPRICOMMON_BYTES+ECPRIPCID_BYTES);
      // convert TS to samples, /3 for 30.72 Ms/s, /6 for 15.36 Ms/s, /12 for 7.68 Ms/s, etc.
      *timestamp = *timestamp/6;
      // handle 1.4,3,5,10,15 MHz cases
      *cc        = *(uint16_t*)(temp_rx0 + ECPRICOMMON_BYTES);
    }
    eth->rx_actual_nsamps=payload_size>>2;
    eth->rx_count++;
  }	 

#ifdef NOSHIFT
  memcpy(buff,(void*)(temp_rx+1),payload_size); 
#else
  // populate receive buffer in lower 12-bits from 16-bit representation
  for (int j=1; j<nsamps2; j++) {
#if defined(__x86_64__) || defined(__i386__)
#ifdef __AVX2__
       ((__m256i *)buff)[j-1] = _mm256_srai_epi16(temp_rx[j],2);
#else
       ((__m128i *)buff)[j-1] = _mm_srai_epi16(temp_rx[j],2);
#endif
#elif defined(__arm__)
       ((int16x8_t *)buff)[j] = vshrq_n_s16(temp_rx[i][j],2);
#endif
  }
#endif
  
  return (payload_size>>2);
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
