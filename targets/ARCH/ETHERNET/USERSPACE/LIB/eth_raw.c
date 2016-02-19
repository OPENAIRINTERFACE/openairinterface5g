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

#include "common_lib.h"
#include "ethernet_lib.h"

#define DEBUG 0

struct sockaddr_ll dest_addr[MAX_INST];
struct sockaddr_ll local_addr[MAX_INST];
int addr_len[MAX_INST];
struct ifreq if_index[MAX_INST];
struct ether_header eh; 

int eth_socket_init_raw(openair0_device *device) {
 
  int i = 0;
  eth_state_t *eth = (eth_state_t*)device->priv;
  int Mod_id = device->Mod_id;
  const char *local_mac, *remote_mac;
  int local_port=0, remote_port=0;
  int sock_dom=0;
  int sock_type=0;
  int sock_proto=0;  
 
  if (device->host_type == RRH_HOST ) {  /* RRH doesn't know remote MAC(will be retrieved from first packet send from BBU) and remote port(don't care) */
    local_mac = device->openair0_cfg->my_addr; 
    local_port = device->openair0_cfg->my_port;    
    remote_mac = malloc(ETH_ALEN);
    memset(remote_mac,0,ETH_ALEN);
    remote_port = 0;    
    printf("[%s] local MAC addr %s remote MAC addr %s\n","RRH", local_mac,remote_mac);    
  } else {
    local_mac = device->openair0_cfg->my_addr;
    local_port = device->openair0_cfg->my_port;  
    remote_mac = device->openair0_cfg->remote_addr;
    remote_port = device->openair0_cfg->remote_port;  
    printf("[%s] local MAC addr %s remote MAC addr %s\n","BBU", local_mac,remote_mac);    
  }
   
  
  /* Open a RAW socket to send on */
  sock_dom=AF_PACKET;
  sock_type=SOCK_RAW;
  sock_proto=IPPROTO_RAW;
  if ((eth->sockfd[Mod_id] = socket(sock_dom, sock_type, sock_proto)) == -1) {
    perror("ETHERNET: Error opening RAW socket");
    exit(0);
  }
  
  /* initialize destination address */
  for (i=0; i< MAX_INST; i++) {
    bzero((void *)&(local_addr[i]), sizeof(struct sockaddr_ll));
    bzero((void *)&(if_index[i]), sizeof(struct ifreq)); 
  }
  /* Get the index of the interface to send on */
  strcpy(if_index[Mod_id].ifr_name,eth->if_name[Mod_id]);
  if (ioctl(eth->sockfd[Mod_id], SIOCGIFINDEX, &(if_index[Mod_id])) < 0)
    perror("SIOCGIFINDEX");
   
  local_addr[Mod_id].sll_family   = AF_PACKET;
  local_addr[Mod_id].sll_ifindex  = if_index[Mod_id].ifr_ifindex;
  /* hear traffic from specific protocol*/
  local_addr[Mod_id].sll_protocol = htons((short)device->openair0_cfg->my_port);
  local_addr[Mod_id].sll_halen    = ETH_ALEN;
  local_addr[Mod_id].sll_pkttype  = PACKET_OTHERHOST;
  addr_len[Mod_id] = sizeof(struct sockaddr_ll);
  
 if (bind(eth->sockfd[Mod_id],(struct sockaddr *)&local_addr[Mod_id],addr_len[Mod_id])<0) {
   perror("ETHERNET: Cannot bind to socket");
   exit(0);
 }
 
 /* Construct the Ethernet header */ 
 ether_aton_r(local_mac, (struct ether_addr *)(&(eh.ether_shost)));
 ether_aton_r(remote_mac, (struct ether_addr *)(&(eh.ether_dhost)));
 eh.ether_type = htons((short)device->openair0_cfg->my_port);

 printf("[%s] binding mod_%d to hardware address %x:%x:%x:%x:%x:%x\n",((device->host_type == BBU_HOST) ? "BBU": "RRH"),Mod_id,eh.ether_shost[0],eh.ether_shost[1],eh.ether_shost[2],eh.ether_shost[3],eh.ether_shost[4],eh.ether_shost[5]);
 
 return 0;
}


int trx_eth_write_raw(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps,int cc, int flags) {
  
  int bytes_sent=0;
  eth_state_t *eth = (eth_state_t*)device->priv;
  int Mod_id = device->Mod_id;
  int sendto_flag =0;
  int i=0;
  //sendto_flag|=flags;

  eth->tx_nsamps=nsamps;

  for (i=0;i<cc;i++) {	
    /* buff[i] points to the position in tx buffer where the payload to be sent is
       buff2 points to the position in tx buffer where the packet header will be placed */
    void *buff2 = (void*)(buff[i]-APP_HEADER_SIZE_BYTES-MAC_HEADER_SIZE_BYTES); 
    
    /* we don't want to ovewrite with the header info the previous tx buffer data so we store it*/
    struct ether_header temp =  *(struct ether_header *)buff2;
    int32_t temp0 = *(int32_t *)(buff2 + MAC_HEADER_SIZE_BYTES);
    openair0_timestamp  temp1 = *(openair0_timestamp *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int32_t));
    
    bytes_sent = 0;
    memcpy(buff2,(void*)&eh,MAC_HEADER_SIZE_BYTES);
    *(int16_t *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int16_t))=1+(i<<1);
    *(openair0_timestamp *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int32_t)) = timestamp;
    
    /*printf("[RRH]write mod_%d %d , len %d, buff %p \n",
      Mod_id,eth->sockfd[Mod_id],RAW_PACKET_SIZE_BYTES(nsamps), buff2);*/
    
    while(bytes_sent < RAW_PACKET_SIZE_BYTES(nsamps)) {
#if DEBUG   
      printf("------- TX ------: buff2 current position=%d remaining_bytes=%d  bytes_sent=%d \n",
	     (void *)(buff2+bytes_sent), 
	     RAW_PACKET_SIZE_BYTES(nsamps) - bytes_sent,
	     bytes_sent);
#endif
      /* Send packet */
      bytes_sent += send(eth->sockfd[Mod_id],
			   buff2, 
			   RAW_PACKET_SIZE_BYTES(nsamps),
			   sendto_flag);
      if ( bytes_sent == -1) {
	eth->num_tx_errors++;
	perror("ETHERNET WRITE: ");
	exit(-1);
      } else {
#if DEBUG
	printf("------- TX ------: nu=%x an_id=%d ts%d bytes_sent=%d\n",
	       *(uint8_t *)(buff2+ETH_ALEN),
	       *(int16_t *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int16_t)),
	       *(openair0_timestamp *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int32_t)),
	       bytes_sent);
    dump_packet((device->host_type == BBU_HOST)? "BBU":"RRH", buff2, RAW_PACKET_SIZE_BYTES(nsamps), TX_FLAG);
#endif
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


int trx_eth_read_raw(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc) {
      
  int bytes_received=0;
  int i=0;
  eth_state_t *eth = (eth_state_t*)device->priv;
  int Mod_id = device->Mod_id;
  int rcvfrom_flag =0;
  
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
      
      while(bytes_received < RAW_PACKET_SIZE_BYTES(nsamps)) {
	bytes_received +=recv(eth->sockfd[Mod_id],
			      buff2,
			      RAW_PACKET_SIZE_BYTES(nsamps),
			      rcvfrom_flag);
	
	if (bytes_received ==-1) {
	  eth->num_rx_errors++;
	  perror("ETHERNET READ: ");
	  exit(-1);	
	} else {
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
      dump_packet((device->host_type == BBU_HOST)? "BBU":"RRH", buff2, RAW_PACKET_SIZE_BYTES(nsamps),RX_FLAG);	  

#endif  

     /* tx buffer values restored */  
      *(struct ether_header *)buff2 = temp;
      *(int32_t *)(buff2 + MAC_HEADER_SIZE_BYTES) = temp0;
      *(openair0_timestamp *)(buff2 + MAC_HEADER_SIZE_BYTES + sizeof(int32_t)) = temp1;
    }
  return (bytes_received-APP_HEADER_SIZE_BYTES-MAC_HEADER_SIZE_BYTES)>>2;
}

 


int eth_set_dev_conf_raw(openair0_device *device) {

  int 	       Mod_id = device->Mod_id;
  eth_state_t *eth = (eth_state_t*)device->priv;
  void 	      *msg;
  ssize_t      msg_len;
  
  /* a BBU client sends to RRH a set of configuration parameters (openair0_config_t)
     so that RF front end is configured appropriately and
     frame/packet size etc. can be set */ 
  
  msg = malloc(MAC_HEADER_SIZE_BYTES + sizeof(openair0_config_t));
  msg_len = MAC_HEADER_SIZE_BYTES + sizeof(openair0_config_t);

  
  memcpy(msg,(void*)&eh,MAC_HEADER_SIZE_BYTES);	
  memcpy((msg+MAC_HEADER_SIZE_BYTES),(void*)device->openair0_cfg,sizeof(openair0_config_t));
 	  
  if (send(eth->sockfd[Mod_id],
	     msg,
	     msg_len,
	     0)==-1) {
    perror("ETHERNET: ");
    exit(0);
  }
  
  return 0;
}


int eth_get_dev_conf_raw(openair0_device *device) {

  eth_state_t   *eth = (eth_state_t*)device->priv;
  int 		Mod_id = device->Mod_id;
  char 		str[INET_ADDRSTRLEN];
  void 		*msg;
  ssize_t	msg_len;
  
  msg = malloc(MAC_HEADER_SIZE_BYTES + sizeof(openair0_config_t));
  msg_len = MAC_HEADER_SIZE_BYTES + sizeof(openair0_config_t);
  
  /* RRH receives from BBU openair0_config_t */
  if (recv(eth->sockfd[Mod_id],
	   msg,
	   msg_len,
	   0)==-1) {
    perror("ETHERNET: ");
    exit(0);
  }
  
  /* RRH stores the remote MAC address */
  memcpy(eh.ether_dhost,(msg+ETH_ALEN),ETH_ALEN);	
  //memcpy((void*)&device->openair0_cfg,(msg + MAC_HEADER_SIZE_BYTES), sizeof(openair0_config_t));
  device->openair0_cfg=(openair0_config_t *)(msg + MAC_HEADER_SIZE_BYTES);
  printf("[%s] binding mod_%d to hardware address %x:%x:%x:%x:%x:%x           hardware address %x:%x:%x:%x:%x:%x\n",((device->host_type == BBU_HOST) ? "BBU": "RRH"),Mod_id,eh.ether_shost[0],eh.ether_shost[1],eh.ether_shost[2],eh.ether_shost[3],eh.ether_shost[4],eh.ether_shost[5],eh.ether_dhost[0],eh.ether_dhost[1],eh.ether_dhost[2],eh.ether_dhost[3],eh.ether_dhost[4],eh.ether_dhost[5]);
 	  
  return 0;
}
