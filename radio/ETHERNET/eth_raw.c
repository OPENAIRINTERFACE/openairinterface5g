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
 * \author  add alcatel Katerina Trilyraki, Navid Nikaein, Pedro Dinis, Lucio Ferreira, Raymond Knopp, Tien-Thinh Nguyen
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

#include "common_lib.h"
#include "ethernet_lib.h"

//#define DEBUG 0

//struct sockaddr_ll dest_addr[MAX_INST];
//struct sockaddr_ll local_addr[MAX_INST];
//int addr_len[MAX_INST];
//struct ifreq if_index[MAX_INST];

int eth_socket_init_raw(openair0_device *device) {
 
  eth_state_t *eth = (eth_state_t*)device->priv;
  const char *local_mac, *remote_mac;
  int sock_dom=0;
  int sock_type=0;
  int sock_proto=0;  
 
  if (device->host_type == RRU_HOST ) {  /* RRU doesn't know remote MAC(will be retrieved from first packet send from RAU) and remote port(don't care) */
    local_mac = device->eth_params->my_addr; 
    remote_mac = malloc(ETH_ALEN);
    memset((void*)remote_mac,0,ETH_ALEN);
    printf("[%s] local MAC addr (user) %s remote MAC addr (user) %s\n","RRU", local_mac,remote_mac);    
  } else {
    local_mac = device->eth_params->my_addr;
    remote_mac = device->eth_params->remote_addr;
    printf("[%s] local MAC addr (user) %s remote MAC addr (user) %s\n","RAU", local_mac,remote_mac);    
  }
   
  
  /* Open a RAW socket to send on */
  sock_dom=AF_PACKET;
  sock_type=SOCK_RAW;
  sock_proto=IPPROTO_RAW;
  if ((eth->sockfdc = socket(sock_dom, sock_type, sock_proto)) == -1) {
    perror("ETHERNET: Error opening RAW socket (control)");
    exit(0);
  }
  for (int i=0;i<eth->num_fd;i++)
    if ((eth->sockfdd[i] = socket(sock_dom, sock_type, sock_proto)) == -1) {
       perror("ETHERNET: Error opening RAW socket (user)");
       exit(0);
    }
  
  /* initialize destination address */
  bzero((void *)&(eth->local_addrd_ll), sizeof(struct sockaddr_ll));
  bzero((void *)&(eth->if_index), sizeof(struct ifreq)); 
  
  /* Get the index of the interface to send on */
  strcpy(eth->if_index.ifr_name,eth->if_name);
  if (ioctl(eth->sockfdc, SIOCGIFINDEX, &(eth->if_index)) < 0)
    perror("SIOCGIFINDEX");
  for (int i=0;i<eth->num_fd;i++)
    if (ioctl(eth->sockfdd[i], SIOCGIFINDEX, &(eth->if_index)) < 0)
      perror("SIOCGIFINDEX");

  eth->local_addrd_ll.sll_family   = AF_PACKET;
  eth->local_addrd_ll.sll_ifindex  = eth->if_index.ifr_ifindex;
  /* hear traffic from specific protocol*/
  eth->local_addrd_ll.sll_protocol = htons((short)device->eth_params->my_portd);

  eth->local_addrd_ll.sll_halen    = ETH_ALEN;
  eth->local_addrd_ll.sll_pkttype  = PACKET_OTHERHOST;
  eth->addr_len = sizeof(struct sockaddr_ll);
  
  for (int i=0;i<eth->num_fd;i++)
    if (bind(eth->sockfdd[i],(struct sockaddr *)&eth->local_addrd_ll,eth->addr_len)<0) {
      perror("ETHERNET: Cannot bind to socket (user)");
      exit(0);
    }
 
 /* Construct the Ethernet header */ 
 ether_aton_r(local_mac, (struct ether_addr *)(&(eth->ehd.ether_shost)));
 ether_aton_r(remote_mac, (struct ether_addr *)(&(eth->ehd.ether_dhost)));
 eth->ehc.ether_type = htons((short)device->eth_params->my_portc);
 eth->ehd.ether_type = htons((short)device->eth_params->my_portd);
  
 printf("[%s] binding to hardware address %x:%x:%x:%x:%x:%x\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"),eth->ehd.ether_shost[0],eth->ehd.ether_shost[1],eth->ehd.ether_shost[2],eth->ehd.ether_shost[3],eth->ehd.ether_shost[4],eth->ehd.ether_shost[5]);
 
 return 0;
}

/* 09/03/2019: fix obvious inconsistencies, but this code hasn't be tested for sure */
int trx_eth_write_raw(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps,int cc, int flags) {
  
  int bytes_sent=0;
  eth_state_t *eth = (eth_state_t*)device->priv;
  int sendto_flag =0;
  int i=0;
  //sendto_flag|=flags;

  eth->tx_nsamps=nsamps;
  int pktsize;
  if (eth->compression == ALAW_COMPRESS) {
    pktsize = RAW_PACKET_SIZE_BYTES_ALAW(nsamps);
  } else {
    pktsize = RAW_PACKET_SIZE_BYTES(nsamps);
  }   
  for (i=0;i<cc;i++) {	
    /* buff[i] points to the position in tx buffer where the payload to be sent is
       buff2 points to the position in tx buffer where the packet header will be placed */
    void *buff2 = (void*)(buff[i]-APP_HEADER_SIZE_BYTES-MAC_HEADER_SIZE_BYTES); 
    
    /* we don't want to ovewrite with the header info the previous tx buffer data so we store it*/
    struct ether_header temp =  *(struct ether_header *)buff2;
    int32_t temp0 = *(int32_t *)(buff2 + MAC_HEADER_SIZE_BYTES);
    openair0_timestamp  temp1 = *(openair0_timestamp *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int32_t));
    
    bytes_sent = 0;
    memcpy(buff2,(void*)&eth->ehd,MAC_HEADER_SIZE_BYTES);
    *(int16_t *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int16_t))=1+(i<<1);
    *(openair0_timestamp *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int32_t)) = timestamp;  

    /*printf("[RRU]write mod_%d %d , len %d, buff %p \n",
      Mod_id,eth->sockfd[Mod_id],RAW_PACKET_SIZE_BYTES(nsamps), buff2);*/
    
    while(bytes_sent < pktsize) {

      /* Send packet */

      bytes_sent += send(eth->sockfdd[cc % eth->num_fd],
			 buff2, 
			 pktsize,
			 sendto_flag);
      if ( bytes_sent == -1) {
	eth->num_tx_errors++;
	perror("ETHERNET WRITE: ");
	exit(-1);
      } else {

      eth->tx_actual_nsamps=bytes_sent>>2;
      eth->tx_count++;
      }
    }    			    
    
  /* tx buffer values restored */  
    *(struct ether_header *)buff2 = temp;
    *(int32_t *)(buff2 + MAC_HEADER_SIZE_BYTES) = temp0;
    *(openair0_timestamp *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int32_t)) = temp1;
  }
  return (bytes_sent-APP_HEADER_SIZE_BYTES-MAC_HEADER_SIZE_BYTES)>>2;
}



int trx_eth_write_raw_IF4p5(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags) {

  int nblocks = nsamps;  
  int bytes_sent = 0;
  
  eth_state_t *eth = (eth_state_t*)device->priv;
  
  ssize_t packet_size;
  
  if (flags == IF4p5_PDLFFT) {
    packet_size = RAW_IF4p5_PDLFFT_SIZE_BYTES(nblocks);    
  } else if (flags == IF4p5_PULFFT) {
    packet_size = RAW_IF4p5_PULFFT_SIZE_BYTES(nblocks);    
  } else if (flags == IF4p5_PULTICK) {
    packet_size = RAW_IF4p5_PULTICK_SIZE_BYTES;    
  } else {
    packet_size = RAW_IF4p5_PRACH_SIZE_BYTES;
  }
  
  eth->tx_nsamps = nblocks;
  
  memcpy(buff[0], (void*)&eth->ehd, MAC_HEADER_SIZE_BYTES);	


  bytes_sent = send(eth->sockfdd[cc % eth->num_fd],
                    buff[0], 
                    packet_size,
                    0);
  
  if (bytes_sent == -1) {
    eth->num_tx_errors++;
    perror("ETHERNET WRITE: ");
    exit(-1);
  } else {
    eth->tx_actual_nsamps = bytes_sent>>1;
    eth->tx_count++;
  }
  
  return (bytes_sent-MAC_HEADER_SIZE_BYTES);  	  
}



int trx_eth_read_raw(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc) {

  int ret;
  int bytes_received=0;
  int i=0;
  eth_state_t *eth = (eth_state_t*)device->priv;
  int rcvfrom_flag =0;

  int block_cnt=0;
  int again_cnt=0;
  
  eth->rx_nsamps=nsamps;

  for (i=0;i<cc;i++) {
      /* buff[i] points to the position in rx buffer where the payload to be received will be placed
	 buff2 points to the position in rx buffer where the packet header will be placed */
      void *buff2 = (void*)(buff[i]-APP_HEADER_SIZE_BYTES-MAC_HEADER_SIZE_BYTES); 

      /* we don't want to ovewrite with the header info the previous rx buffer data so we store it*/
      struct ether_header temp =  *(struct ether_header *)buff2;
      int32_t temp0 = *(int32_t *)(buff2 + MAC_HEADER_SIZE_BYTES);
      openair0_timestamp  temp1 = *(openair0_timestamp *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int32_t));

      bytes_received=0;
      int receive_bytes;
      if (eth->compression == ALAW_COMPRESS) {
        receive_bytes = RAW_PACKET_SIZE_BYTES_ALAW(nsamps);
      } else {
        receive_bytes = RAW_PACKET_SIZE_BYTES(nsamps);
      }
      
      while(bytes_received < receive_bytes) {
      again:
	ret = recv(eth->sockfdd[cc % eth->num_fd],
			      buff2,
			      receive_bytes,
			      rcvfrom_flag);

	if (ret ==-1) {
	  eth->num_rx_errors++;
          if (errno == EAGAIN) {
            again_cnt++;
            usleep(10);
            if (again_cnt == 1000) {
              perror("ETHERNET IF5 READ (EAGAIN): ");
              exit(-1);
            } else {
              printf("AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN \n");
              goto again;
            }
          } else if (errno == EWOULDBLOCK) {
            block_cnt++;
            usleep(10);
            if (block_cnt == 1000) {
              perror("ETHERNET IF5 READ (EWOULDBLOCK): ");
              exit(-1);
            } else {
              printf("BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK BLOCK \n");
              goto again;
            }
          } else {
            perror("ETHERNET IF5 READ");
            printf("(%s):\n", strerror(errno));
            exit(-1);
          }

	} else {
	  bytes_received+=ret;
	  /* store the timestamp value from packet's header */
	  *timestamp =  *(openair0_timestamp *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int32_t));  
	  eth->rx_actual_nsamps=bytes_received>>2;   
	  eth->rx_count++;
	}
      }
      
#if DEBUG   
      printf("------- RX------: nu=%x an_id=%d ts%d bytes_recv=%d \n",
	     *(uint8_t *)(buff2+ETH_ALEN),
	     *(int16_t *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int16_t)),
	     *(openair0_timestamp *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int32_t)),
	     bytes_received);

      dump_packet((device->host_type == RAU_HOST)? "RAU":"RRU", buff2, RAW_PACKET_SIZE_BYTES(nsamps),RX_FLAG);	  

#endif  

     /* tx buffer values restored */  
      *(struct ether_header *)buff2 = temp;
      *(int32_t *)(buff2 + MAC_HEADER_SIZE_BYTES) = temp0;
      *(openair0_timestamp *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int32_t)) = temp1;
    }
  return (bytes_received-APP_HEADER_SIZE_BYTES-MAC_HEADER_SIZE_BYTES)>>2;
}



int trx_eth_read_raw_IF4p5(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc) {

  int ret;
  // Read nblocks info from packet itself
  int nblocks = nsamps;  
  int bytes_received=0;
  eth_state_t *eth = (eth_state_t*)device->priv;
  
  ssize_t packet_size = MAC_HEADER_SIZE_BYTES + sizeof_IF4p5_header_t;      
  IF4p5_header_t *test_header = (IF4p5_header_t*)(buff[0] + MAC_HEADER_SIZE_BYTES);

  int block_cnt=0;
  int again_cnt=0;

  while (bytes_received < packet_size) {
  again:
    ret = recv(eth->sockfdd[cc % eth->num_fd],
	       buff[0],
	       packet_size,
	       MSG_PEEK);                        
    if (ret ==-1) {
      eth->num_rx_errors++;
      if (errno == EAGAIN) {
        again_cnt++;
        usleep(10);
        if (again_cnt == 1000) {
          perror("ETHERNET IF4p5 READ (EAGAIN): ");
          exit(-1);
        } else {
          printf("AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN AGAIN \n");
          goto again;
        }
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
    }
    else bytes_received+=ret;
  }

#ifdef DEBUG
  for (int i=0;i<packet_size;i++)
    printf("%2x.",((uint8_t*)buff[0])[i]);
  printf("\n");
#endif
  
  *timestamp = test_header->sub_type; 
  
  if (test_header->sub_type == IF4p5_PDLFFT) {
    packet_size = RAW_IF4p5_PDLFFT_SIZE_BYTES(nblocks);             
  } else if (test_header->sub_type == IF4p5_PULFFT) {
    packet_size = RAW_IF4p5_PULFFT_SIZE_BYTES(nblocks);             
  } else {
    packet_size = RAW_IF4p5_PRACH_SIZE_BYTES;
  }  
  
  while(bytes_received < packet_size) {
    ret = recv(eth->sockfdd[cc % eth->num_fd],
	       buff[0],
	       packet_size,
	       0);
    if (ret ==-1) {
      eth->num_rx_errors++;
      perror("ETHERNET IF4p5 READ (payload): ");
      return(-1);
    } else {
      bytes_received+=ret;
      eth->rx_actual_nsamps = bytes_received>>1;   
      eth->rx_count++;
    }
  }

  eth->rx_nsamps = nsamps;  
  return(bytes_received);
}


int eth_set_dev_conf_raw(openair0_device *device) {

  eth_state_t *eth = (eth_state_t*)device->priv;
  void 	      *msg;
  ssize_t      msg_len;
  
  /* a RAU client sends to RRU a set of configuration parameters (openair0_config_t)
     so that RF front end is configured appropriately and
     frame/packet size etc. can be set */ 
  
  msg = malloc(MAC_HEADER_SIZE_BYTES + sizeof(openair0_config_t));
  msg_len = MAC_HEADER_SIZE_BYTES + sizeof(openair0_config_t);

  
  memcpy(msg,(void*)&eth->ehc,MAC_HEADER_SIZE_BYTES);	
  memcpy((msg+MAC_HEADER_SIZE_BYTES),(void*)device->openair0_cfg,sizeof(openair0_config_t));
 	  
  if (send(eth->sockfdc,
	   msg,
	   msg_len,
	   0)==-1) {
    perror("ETHERNET: ");
    exit(0);
  }
  free(msg);
  
  return 0;
}



int eth_set_dev_conf_raw_IF4p5(openair0_device *device) {  
  // use for cc_id info

  eth_state_t *eth = (eth_state_t*)device->priv;
  void 	      *msg;
  ssize_t      msg_len;
  
  /* a RAU client sends to RRU a set of configuration parameters (openair0_config_t)
     so that RF front end is configured appropriately and
     frame/packet size etc. can be set */ 
  
  msg = malloc(MAC_HEADER_SIZE_BYTES + sizeof(openair0_config_t));
  msg_len = MAC_HEADER_SIZE_BYTES + sizeof(openair0_config_t);

  
  memcpy(msg,(void*)&eth->ehc,MAC_HEADER_SIZE_BYTES);	
  memcpy((msg+MAC_HEADER_SIZE_BYTES),(void*)device->openair0_cfg,sizeof(openair0_config_t));
 	  
  if (send(eth->sockfdc,
	   msg,
	   msg_len,
	   0)==-1) {
    perror("ETHERNET: ");
    exit(0);
  }

  free(msg);
  return 0;
}


int eth_get_dev_conf_raw(openair0_device *device) {

  eth_state_t   *eth = (eth_state_t*)device->priv;
  void 		*msg;
  ssize_t	msg_len;
  
  msg = malloc(MAC_HEADER_SIZE_BYTES + sizeof(openair0_config_t));
  msg_len = MAC_HEADER_SIZE_BYTES + sizeof(openair0_config_t);
  
  /* RRU receives from RAU openair0_config_t */
  if (recv(eth->sockfdc,
	   msg,
	   msg_len,
	   0)==-1) {
    perror("ETHERNET: ");
    exit(0);
  }
  
  /* RRU stores the remote MAC address */
  memcpy(eth->ehd.ether_dhost,(msg+ETH_ALEN),ETH_ALEN);	
  //memcpy((void*)&device->openair0_cfg,(msg + MAC_HEADER_SIZE_BYTES), sizeof(openair0_config_t));
  device->openair0_cfg=(openair0_config_t *)(msg + MAC_HEADER_SIZE_BYTES);
  printf("[%s] binding mod to hardware address %x:%x:%x:%x:%x:%x           hardware address %x:%x:%x:%x:%x:%x\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"),eth->ehd.ether_shost[0],eth->ehd.ether_shost[1],eth->ehd.ether_shost[2],eth->ehd.ether_shost[3],eth->ehd.ether_shost[4],eth->ehd.ether_shost[5],eth->ehd.ether_dhost[0],eth->ehd.ether_dhost[1],eth->ehd.ether_dhost[2],eth->ehd.ether_dhost[3],eth->ehd.ether_dhost[4],eth->ehd.ether_dhost[5]);
 	  
  return 0;
}


int eth_get_dev_conf_raw_IF4p5(openair0_device *device) {
  // use for cc_id info

  eth_state_t   *eth = (eth_state_t*)device->priv;
  void 		*msg;
  ssize_t	msg_len;
  
  msg = malloc(MAC_HEADER_SIZE_BYTES + sizeof(openair0_config_t));
  msg_len = MAC_HEADER_SIZE_BYTES + sizeof(openair0_config_t);
  
  /* RRU receives from RAU openair0_config_t */
  if (recv(eth->sockfdc,
	   msg,
	   msg_len,
	   0)==-1) {
    perror("ETHERNET: ");
    exit(0);
  }
  
  /* RRU stores the remote MAC address */
  memcpy(eth->ehd.ether_dhost,(msg+ETH_ALEN),ETH_ALEN);	
  //memcpy((void*)&device->openair0_cfg,(msg + MAC_HEADER_SIZE_BYTES), sizeof(openair0_config_t));
  //device->openair0_cfg=(openair0_config_t *)(msg + MAC_HEADER_SIZE_BYTES);
  printf("[%s] binding mod to hardware address %x:%x:%x:%x:%x:%x           hardware address %x:%x:%x:%x:%x:%x\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"),eth->ehd.ether_shost[0],eth->ehd.ether_shost[1],eth->ehd.ether_shost[2],eth->ehd.ether_shost[3],eth->ehd.ether_shost[4],eth->ehd.ether_shost[5],eth->ehd.ether_dhost[0],eth->ehd.ether_dhost[1],eth->ehd.ether_dhost[2],eth->ehd.ether_dhost[3],eth->ehd.ether_dhost[4],eth->ehd.ether_dhost[5]);
 	  
  free(msg);
  return 0;
}
