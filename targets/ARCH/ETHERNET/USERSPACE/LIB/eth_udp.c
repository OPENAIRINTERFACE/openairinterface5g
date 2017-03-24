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
#include "vcd_signal_dumper.h"

#include "common_lib.h"
#include "ethernet_lib.h"

#define DEBUG 0
//struct sockaddr_in dest_addr[MAX_INST];
//struct sockaddr_in local_addr[MAX_INST];
//int addr_len[MAX_INST];


uint16_t pck_seq_num = 1;
uint16_t pck_seq_num_cur=0;
uint16_t pck_seq_num_prev=0;

int eth_socket_init_udp(openair0_device *device) {

  eth_state_t *eth = (eth_state_t*)device->priv;
  char str_local[INET_ADDRSTRLEN];
  char str_remote[INET_ADDRSTRLEN];
  const char *local_ip, *remote_ip;
  int local_port=0, remote_port=0;
  int sock_dom=0;
  int sock_type=0;
  int sock_proto=0;
  int enable=1;

  if (device->host_type == RRH_HOST ) {
    local_ip   = device->openair0_cfg->my_addr;   
    local_port = device->openair0_cfg->my_port;
    remote_ip   = "0.0.0.0";   
    remote_port =  0;   
    printf("[%s] local ip addr %s port %d\n", "RRH", local_ip, local_port);    
  } else {
    local_ip   = device->openair0_cfg->my_addr;   
    local_port = device->openair0_cfg->my_port;
    remote_ip   = device->openair0_cfg->remote_addr;
    remote_port = device->openair0_cfg->remote_port;  
    printf("[%s] local ip addr %s port %d\n","BBU", local_ip, local_port);    
  }
  
  /* Open socket to send on */
  sock_dom=AF_INET;
  sock_type=SOCK_DGRAM;
  sock_proto=IPPROTO_UDP;
  
  if ((eth->sockfd = socket(sock_dom, sock_type, sock_proto)) == -1) {
    perror("ETHERNET: Error opening socket");
    exit(0);
  }
  
  /* initialize addresses */
  bzero((void *)&(eth->dest_addr), sizeof(eth->dest_addr));
  bzero((void *)&(eth->local_addr), sizeof(eth->local_addr));
  

  eth->addr_len = sizeof(struct sockaddr_in);

  eth->dest_addr.sin_family = AF_INET;
  inet_pton(AF_INET,remote_ip,&(eth->dest_addr.sin_addr.s_addr));
  eth->dest_addr.sin_port=htons(remote_port);
  inet_ntop(AF_INET, &(eth->dest_addr.sin_addr), str_remote, INET_ADDRSTRLEN);


  eth->local_addr.sin_family = AF_INET;
  inet_pton(AF_INET,local_ip,&(eth->local_addr.sin_addr.s_addr));
  eth->local_addr.sin_port=htons(local_port);
  inet_ntop(AF_INET, &(eth->local_addr.sin_addr), str_local, INET_ADDRSTRLEN);

  
  /* set reuse address flag */
  if (setsockopt(eth->sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
    perror("ETHERNET: Cannot set SO_REUSEADDR option on socket");
    exit(0);
  }
  
  /* want to receive -> so bind */   
    if (bind(eth->sockfd,(struct sockaddr *)&eth->local_addr,eth->addr_len)<0) {
      perror("ETHERNET: Cannot bind to socket");
      exit(0);
    } else {
      printf("[%s] binding to %s:%d\n","RRH",str_local,ntohs(eth->local_addr.sin_port));
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
  //int again_cnt=0;
  packet_size = max(UDP_IF4p5_PRACH_SIZE_BYTES, max(UDP_IF4p5_PULFFT_SIZE_BYTES(nblocks), UDP_IF4p5_PDLFFT_SIZE_BYTES(nblocks)));

  while(bytes_received == -1) {
  again:
    bytes_received = recvfrom(eth->sockfd,
                              buff[0],
                              packet_size,
                              0,
                              (struct sockaddr *)&eth->dest_addr,
                              (socklen_t *)&eth->addr_len);
    if (bytes_received ==-1) {
      eth->num_rx_errors++;
      if (errno == EAGAIN) {
	/*
        again_cnt++;
        usleep(10);
        if (again_cnt == 1000) {
          perror("ETHERNET IF4p5 READ (EAGAIN): ");
          exit(-1);
        } else {
          printf("AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN \n");
          goto again;
        }
	*/
	printf("Lost IF4p5 connection with %s\n", inet_ntoa(eth->dest_addr.sin_addr));
	exit(-1);
      } else if (errno == EWOULDBLOCK) {
        block_cnt++;
        usleep(10);
        if (block_cnt == 1000) {
          perror("ETHERNET IF4p5 READ (EWOULDBLOCK): ");
          exit(-1);
        } else {
          printf("BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK \n");
          goto again;
        }
      } else {
        perror("ETHERNET IF4p5 READ");
        printf("(%s):\n", strerror(errno));
        exit(-1);
      }
    } else {
      *timestamp = test_header->sub_type;
      eth->rx_actual_nsamps = bytes_received>>1;
      eth->rx_count++;
    }
  }
  //printf("size of third %d subtype %d frame %d subframe %d symbol %d \n", bytes_received, test_header->sub_type, ((test_header->frame_status)>>6)&0xffff, ((test_header->frame_status)>>22)&0x000f, ((test_header->frame_status)>>26)&0x000f) ;

  eth->rx_nsamps = nsamps;  
  return(bytes_received);
}

int trx_eth_write_udp_IF4p5(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags) {

  int nblocks = nsamps;  
  int bytes_sent = 0;

  
  eth_state_t *eth = (eth_state_t*)device->priv;
  
  ssize_t packet_size;
  
  if (flags == IF4p5_PDLFFT) {
    packet_size = UDP_IF4p5_PDLFFT_SIZE_BYTES(nblocks);    
  } else if (flags == IF4p5_PULFFT) {
    packet_size = UDP_IF4p5_PULFFT_SIZE_BYTES(nblocks); 
  } else if (flags == IF4p5_PULTICK) {
    packet_size = UDP_IF4p5_PULTICK_SIZE_BYTES; 
  } else if (flags == IF4p5_PRACH) {  
    packet_size = UDP_IF4p5_PRACH_SIZE_BYTES;   
  } else {
    printf("trx_eth_write_udp_IF4p5: unknown flags %d\n",flags);
    return(-1);
  }
   
  eth->tx_nsamps = nblocks;
  bytes_sent = sendto(eth->sockfd,
		      buff[0], 
		      packet_size,
		      0,
		      (struct sockaddr*)&eth->dest_addr,
		      eth->addr_len);
  
  if (bytes_sent == -1) {
    eth->num_tx_errors++;
    perror("ETHERNET WRITE: ");
    exit(-1);
  } else {
    eth->tx_actual_nsamps = bytes_sent>>1;
    eth->tx_count++;
  }
  
  return (bytes_sent);  	  
}

int trx_eth_write_udp(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps,int cc, int flags) {	
  
  int bytes_sent=0;
  eth_state_t *eth = (eth_state_t*)device->priv;
  int sendto_flag =0;
  int i=0;
  //sendto_flag|=flags;
  eth->tx_nsamps=nsamps;

 

  for (i=0;i<cc;i++) {	
    /* buff[i] points to the position in tx buffer where the payload to be sent is
       buff2 points to the position in tx buffer where the packet header will be placed */
    void *buff2 = (void*)(buff[i]- APP_HEADER_SIZE_BYTES); 
    
    /* we don't want to ovewrite with the header info the previous tx buffer data so we store it*/
    int32_t temp0 = *(int32_t *)buff2;
    openair0_timestamp  temp1 = *(openair0_timestamp *)(buff2 + sizeof(int32_t));
    
    bytes_sent = 0;
    
    /* constract application header */
    // eth->pck_header.seq_num = pck_seq_num;
    //eth->pck_header.antenna_id = 1+(i<<1);
    //eth->pck_header.timestamp = timestamp;
    *(uint16_t *)buff2 = eth->pck_seq_num;
    *(uint16_t *)(buff2 + sizeof(uint16_t)) = 1+(i<<1);
    *(openair0_timestamp *)(buff2 + sizeof(int32_t)) = timestamp;
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TX_SEQ_NUM, eth->pck_seq_num);
    
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
      bytes_sent += sendto(eth->sockfd,
			   buff2, 
                           sent_byte,
			   sendto_flag,
			   (struct sockaddr*)&eth->dest_addr,
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
    dump_packet((device->host_type == BBU_HOST)? "BBU":"RRH", buff2, sent_byte, TX_FLAG);
#endif
    eth->tx_actual_nsamps=bytes_sent>>2;
    eth->tx_count++;
    eth->pck_seq_num++;
    if ( eth->pck_seq_num > MAX_PACKET_SEQ_NUM(nsamps,device->openair0_cfg->samples_per_frame) )  eth->pck_seq_num = 1;
      }
    //}
                  
      /* tx buffer values restored */  
      *(int32_t *)buff2 = temp0;
      *(openair0_timestamp *)(buff2 + sizeof(int32_t)) = temp1;
  }
 
  return (bytes_sent-APP_HEADER_SIZE_BYTES)>>2;
}
      


int trx_eth_read_udp(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc) {
  
  int bytes_received=0;
  eth_state_t *eth = (eth_state_t*)device->priv;
  //  openair0_timestamp prev_timestamp = -1;
  int rcvfrom_flag =0;
  int block_cnt=0;
  int again_cnt=0;
  int i=0;

  eth->rx_nsamps=nsamps;

  for (i=0;i<cc;i++) {
    /* buff[i] points to the position in rx buffer where the payload to be received will be placed
       buff2 points to the position in rx buffer where the packet header will be placed */
    void *buff2 = (void*)(buff[i]- APP_HEADER_SIZE_BYTES);
    
    /* we don't want to ovewrite with the header info the previous rx buffer data so we store it*/
    int32_t temp0 = *(int32_t *)buff2;
    openair0_timestamp temp1 = *(openair0_timestamp *)(buff2 + sizeof(int32_t));
    
    bytes_received=0;
    block_cnt=0;
    int receive_bytes;
    if (eth->compression == ALAW_COMPRESS) {
      receive_bytes = UDP_PACKET_SIZE_BYTES_ALAW(nsamps);
    } else {
      receive_bytes = UDP_PACKET_SIZE_BYTES(nsamps);
    }
    
    while(bytes_received < receive_bytes) {
    again:
#if DEBUG   
	   printf("------- RX------: buff2 current position=%d remaining_bytes=%d  bytes_recv=%d \n",
		  (void *)(buff2+bytes_received),
		  receive_bytes - bytes_received,
		  bytes_received);
#endif
      bytes_received +=recvfrom(eth->sockfd,
				buff2,
	                        receive_bytes,
				rcvfrom_flag,
				(struct sockaddr *)&eth->dest_addr,
				(socklen_t *)&eth->addr_len);
      
      if (bytes_received ==-1) {
	eth->num_rx_errors++;
	if (errno == EAGAIN) {
	  again_cnt++;
	  usleep(10);
	  if (again_cnt == 1000) {
	  perror("ETHERNET READ: ");
	  exit(-1);
	  } else {
	    printf("AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN \n");
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
#if DEBUG   
	   printf("------- RX------: nu=%d an_id=%d ts%d bytes_recv=%d\n",
		  *(int16_t *)buff2,
		  *(int16_t *)(buff2 + sizeof(int16_t)),
		  *(openair0_timestamp *)(buff2 + sizeof(int32_t)),
		  bytes_received);
	   dump_packet((device->host_type == BBU_HOST)? "BBU":"RRH", buff2, receive_bytes, RX_FLAG);	  
#endif  
	   
	   /* store the timestamp value from packet's header */
	   *timestamp =  *(openair0_timestamp *)(buff2 + sizeof(int32_t));
	   /* store the sequence number of the previous packet received */    
	   if (eth->pck_seq_num_cur == 0) {
	     eth->pck_seq_num_prev = *(uint16_t *)buff2;
	   } else {
	     eth->pck_seq_num_prev = eth->pck_seq_num_cur;
	   }
	   /* get the packet sequence number from packet's header */
	   eth->pck_seq_num_cur = *(uint16_t *)buff2;
	   if ( ( eth->pck_seq_num_cur != (eth->pck_seq_num_prev + 1) ) && !((eth->pck_seq_num_prev==MAX_PACKET_SEQ_NUM(nsamps,device->openair0_cfg->samples_per_frame)) && (eth->pck_seq_num_cur==1 )) && !((eth->pck_seq_num_prev==1) && (eth->pck_seq_num_cur==1))) {	     
	     //#if DEBUG
	     printf("Out of order packet received: current_packet=%d previous_packet=%d timestamp=%"PRId64"\n",eth->pck_seq_num_cur,eth->pck_seq_num_prev,*timestamp);
	     //#endif
	   }
	   VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RX_SEQ_NUM,eth->pck_seq_num_cur);
	   VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RX_SEQ_NUM_PRV,eth->pck_seq_num_prev);
						    eth->rx_actual_nsamps=bytes_received>>2;
						    eth->rx_count++;
      }	 
	     
      }
      /* tx buffer values restored */  
      *(int32_t *)buff2 = temp0;
      *(openair0_timestamp *)(buff2 + sizeof(int32_t)) = temp1;
	  
    }      
  return (bytes_received-APP_HEADER_SIZE_BYTES)>>2;
}
      



int eth_set_dev_conf_udp(openair0_device *device) {

  eth_state_t *eth = (eth_state_t*)device->priv;
  void 	      *msg;
  ssize_t      msg_len;

  
  /* a BBU client sents to RRH a set of configuration parameters (openair0_config_t)
     so that RF front end is configured appropriately and
     frame/packet size etc. can be set */ 

  msg=malloc(sizeof(openair0_config_t));
  msg_len=sizeof(openair0_config_t);
  memcpy(msg,(void*)device->openair0_cfg,msg_len);	

  if (sendto(eth->sockfd,msg,msg_len,0,(struct sockaddr *)&eth->dest_addr,eth->addr_len)==-1) {
    perror("ETHERNET: sendto conf_udp");
    printf("addr_len : %d, msg_len %zd\n",eth->addr_len,msg_len);
    exit(0);
  }

  return 0;
}

int eth_get_dev_conf_udp(openair0_device *device) {

  eth_state_t   *eth = (eth_state_t*)device->priv;
  char 		str1[INET_ADDRSTRLEN],str[INET_ADDRSTRLEN];
  void 		*msg;
  ssize_t	msg_len;
  
  msg=malloc(sizeof(openair0_config_t));
  msg_len=sizeof(openair0_config_t);

  inet_ntop(AF_INET, &(eth->local_addr.sin_addr), str, INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &(eth->dest_addr.sin_addr), str1, INET_ADDRSTRLEN);

  /* RRH receives from BBU openair0_config_t */
  if (recvfrom(eth->sockfd,
	       msg,
	       msg_len,
	       0,
	       (struct sockaddr *)&eth->dest_addr,
	       (socklen_t *)&eth->addr_len)==-1) {
    perror("ETHERNET: recv_from conf_udp");
    exit(0);
  }
  device->openair0_cfg=(openair0_config_t *)msg;

   /* get remote ip address and port */
   /* inet_ntop(AF_INET, &(eth->dest_addr.sin_addr), str1, INET_ADDRSTRLEN); */
   /* device->openair0_cfg->remote_port =ntohs(eth->dest_addr.sin_port); */
   /* device->openair0_cfg->remote_addr =str1; */

   /* /\* restore local ip address and port *\/ */
   /* inet_ntop(AF_INET, &(eth->local_addr.sin_addr), str, INET_ADDRSTRLEN); */
   /* device->openair0_cfg->my_port =ntohs(eth->local_addr.sin_port); */
   /* device->openair0_cfg->my_addr =str; */

   /*  printf("[RRH] mod_%d socket %d connected to BBU %s:%d  %s:%d\n", Mod_id, eth->sockfd,str1, device->openair0_cfg->remote_port, str, device->openair0_cfg->my_port);  */
   return 0;
}
