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
   OpenAirInterface Dev  : openair4g-devel@eurecom.fr

   Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
/*! \fileethernet_lib.c 
 * \brief API to stream I/Q samples over standard ethernet
 * \author Katerina Trilyraki, Navid Nikaein, Pedro Dinis, Lucio Ferreira, Raymond Knopp
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


int num_devices_eth = 0;
int dest_addr_len[MAX_INST];
char sendbuf[MAX_INST][BUF_SIZ]; /*TODO*/


/* Initialization of UDP Socket to communicate with one destination */
int ethernet_socket_init(openair0_device *device) {

  int i = 0;
  eth_state_t *eth = (eth_state_t*)device->priv;
  int Mod_id = device->Mod_id;
  
  // struct sockaddr_in *dest = &dest_addr[Mod_id];
  char str[INET_ADDRSTRLEN];
  const char *dest_ip;
  int dest_port;
  
  if (device->func_type == RRH_FUNC ){
    dest_ip   = device->openair0_cfg.my_ip;   
    dest_port = device->openair0_cfg.my_port;
    printf("[RRH] ip addr %s port %d\n",dest_ip, dest_port);
  } else {
    dest_ip   = device->openair0_cfg.remote_ip;
    dest_port = device->openair0_cfg.remote_port;
    printf("[BBU] ip addr %s port %d\n",dest_ip, dest_port);
  }
    
  /* Open RAW socket to send on */
  if ((eth->sockfd[Mod_id] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    perror("ETHERNET: Error opening socket");
    exit(0);
  }

  /* initialize destination address */
  for (i=0; i< MAX_INST; i++)  
    bzero((void *)&(eth->dest_addr[i]), sizeof(eth->dest_addr[i]));

 // bzero((void *)dest,sizeof(struct sockaddr_in));
  eth->dest_addr[Mod_id].sin_family = AF_INET;
  inet_pton(AF_INET,dest_ip,&(eth->dest_addr[Mod_id].sin_addr.s_addr));
  eth->dest_addr[Mod_id].sin_port=htons(dest_port);
  dest_addr_len[Mod_id] = sizeof(struct sockaddr_in);
  inet_ntop(AF_INET, &(eth->dest_addr[Mod_id].sin_addr), str, INET_ADDRSTRLEN);
  
  /* if RRH, then I am the server, so bind */
  if (device->func_type == RRH_FUNC ){    
    if (bind(eth->sockfd[Mod_id],(struct sockaddr *)&eth->dest_addr[Mod_id], dest_addr_len[Mod_id])<0) {
      perror("ETHERNET: Cannot bind to socket");
      exit(0);
    }else {
      printf("[RRH] binding mod_%d to %s:%d\n",Mod_id,str,ntohs(eth->dest_addr[Mod_id].sin_port));
    }
    
  }else {
    printf("[BBU] Connecting to %s:%d\n",str,ntohs(eth->dest_addr[Mod_id].sin_port));
  }
  
  return 0;
}

void ethernet_socket_opt (openair0_device *device){

  eth_state_t *eth = (eth_state_t*)device->priv;
  int Mod_id=device->Mod_id;
  
  int sndbuf_size=0, rcvbuf_size=0;      
  socklen_t optlen;    
  /* chang the MTU of the eth interface */ 
  struct ifreq ifr; 
  
  optlen = sizeof(int);  
  
  if (getsockopt(eth->sockfd[Mod_id],  
                 SOL_SOCKET,  
                 SO_SNDBUF,  
                 &sndbuf_size,&optlen))   
    printf("error:getsockopt()\n");  
  
  if (getsockopt(eth->sockfd[Mod_id],  
                 SOL_SOCKET,  
                 SO_RCVBUF,  
                 &rcvbuf_size,&optlen))
    printf("error:getsockopt()\n"); 

  printf( "sndbuf_size= %d bytes rcvbuf_size= %d bytes\n", sndbuf_size,
	  rcvbuf_size); 

  ifr.ifr_addr.sa_family = AF_INET;
  //iap->ifa_name is bond1:xx
  strncpy(ifr.ifr_name, DEFAULT_IF, sizeof(ifr.ifr_name));
  ifr.ifr_mtu = device->openair0_cfg.samples_per_packet*5; 
  if (ioctl(eth->sockfd[Mod_id], SIOCSIFMTU, (caddr_t)&ifr)  < 0 )
    perror ("Can't set the MTU");
  else 
    printf("[ETHERNET] %s MTU size has changed to %d\n",DEFAULT_IF,ifr.ifr_mtu);
  
}


int trx_eth_set_dev_conf(openair0_device *device) {

  int 	       Mod_id = device->Mod_id;
  eth_state_t *eth = (eth_state_t*)device->priv;
  void 	      *msg;
  ssize_t     msg_len;

  
  /* send to RRH (server) required configuration parameters:
     -number of downlink RBs (so that frame/packet size can be set accordingly)
     -rx_num_0channels
     -tx_num_channels
     -rx_freq
     -tx_freq
     -rxg_mode[4]	
     -rx_gain
     -tx_gain
     -rx_bw
     -tx_bw
     -autocal */ 
  
  msg=malloc(sizeof(openair0_config_t));
  msg_len=sizeof(openair0_config_t);
  memcpy(msg,(void*)&device->openair0_cfg,msg_len);	
  
  if (sendto(eth->sockfd[Mod_id],msg,msg_len,0,(struct sockaddr *)&eth->dest_addr[Mod_id],dest_addr_len[Mod_id])==-1){
    perror("ETHERNET: ");
    exit(0);
  }
     
  return 0;
}



int trx_eth_get_dev_conf(openair0_device *device) {

  eth_state_t   *eth = (eth_state_t*)device->priv;
  int 		Mod_id = device->Mod_id;
  char 		str[INET_ADDRSTRLEN];
  void 		*msg;
  ssize_t	msg_len;
  
  msg=malloc(sizeof(openair0_config_t));
  msg_len=sizeof(openair0_config_t);

  /* receive from client (lte-softmodem)  */
  if (recvfrom(eth->sockfd[Mod_id],
	       msg,
	       msg_len,
	       0,
	       (struct sockaddr *)&eth->dest_addr[Mod_id],
	       (socklen_t *)&dest_addr_len[Mod_id])==-1){
    perror("ETHERNET: ");
    exit(0);
  }
		
   memcpy((void*)&device->openair0_cfg,msg,msg_len);	
   inet_ntop(AF_INET, &(eth->dest_addr[Mod_id].sin_addr), str, INET_ADDRSTRLEN);
   device->openair0_cfg.remote_port =ntohs(eth->dest_addr[Mod_id].sin_port);
   device->openair0_cfg.remote_ip=str;
   //ethernet_socket_opt (device);
   // printf("[RRH] write mod_%d %d to %s:%d\n",Mod_id,eth->sockfd[Mod_id],str,ntohs(eth->dest_addr[Mod_id].sin_port));

   return 0;
}

int trx_eth_request(openair0_device *device, void *msg, ssize_t msg_len) {

  int 	       Mod_id = device->Mod_id;
  eth_state_t *eth = (eth_state_t*)device->priv;
 
 if (sendto(eth->sockfd[Mod_id],msg,msg_len,0,(struct sockaddr *)&eth->dest_addr[Mod_id],dest_addr_len[Mod_id])==-1){
    perror("ETHERNET: ");
    exit(0);
  }
     
  return 0;
}



int trx_eth_reply(openair0_device *device, void *msg, ssize_t msg_len) {

  eth_state_t   *eth = (eth_state_t*)device->priv;
  int 		Mod_id = device->Mod_id;

  /* receive from client (lte-softmodem)  */
  if (recvfrom(eth->sockfd[Mod_id],
	       msg,
	       msg_len,
	       0,
	       (struct sockaddr *)&eth->dest_addr[Mod_id],
	       (socklen_t *)&dest_addr_len[Mod_id])==-1){
    perror("ETHERNET: ");
    exit(0);
  }
		
 
   return 0;
}


int ethernet_write_data(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps,int antenna_id) {	
	
  int n_written;
  uint16_t header_size=sizeof(int32_t) + sizeof(openair0_timestamp);
  eth_state_t *eth = (eth_state_t*)device->priv;
  int Mod_id = device->Mod_id;
  int sendto_flag =0;
  sendto_flag|=MSG_DONTWAIT;
	
  /* buff[antenna_id] points to the position in tx buffer where the payload to be sent is
     buff2 points to the position in tx buffer where the packet header will be placed */
  void *buff2 = (void*)(buff[antenna_id]-header_size); // (void*)((unsigned char *)buff[antenna_id]-header_size);

  /* we don't want to ovewrite with the header info the previous tx buffer data so we store it*/
  int32_t temp0 = *(int32_t *)buff2;
  openair0_timestamp  temp1 = *(openair0_timestamp *)(buff2 + sizeof(int32_t));
  
  n_written = 0;
  
  *(int16_t *)(buff2 + sizeof(int16_t))=1+(antenna_id<<1);
  *(openair0_timestamp *)(buff2 + sizeof(int32_t)) = timestamp;
	
  /* printf("[RRH]write mod_%d %d , len %d, buff %p antenna %d\n",
     Mod_id,eth->sockfd[Mod_id],(nsamps<<2)+header_size, buff2, antenna_id);*/
	
while(n_written < nsamps) {
    /* Send packet */
  if ((n_written += sendto(eth->sockfd[Mod_id],
			   buff2, 
                           (nsamps<<2)+header_size,
                           0,
                           (struct sockaddr*)&eth->dest_addr[Mod_id],
                           dest_addr_len[Mod_id])) < 0) {
    perror("ETHERNET WRITE");
    exit(-1);
    }
  }

/* printf("Buffer head TX: nu=%d an_id=%d ts%d byte_send=%d \n",		*(int16_t *)buff2,
  *(int16_t *)(buff2 + sizeof(int16_t)),
  *(openair0_timestamp *)(buff2 + sizeof(int32_t)),
  n_written>>2);*/
  
  /* tx buffer values restored */  
  *(int32_t *)buff2 = temp0;
  *(openair0_timestamp *)(buff2 + sizeof(int32_t)) = temp1;
  return n_written;

}


int ethernet_read_data(openair0_device *device,openair0_timestamp *timestamp,void **buff, int nsamps,int antenna_id) {

  int bytes_received;
  int block_cnt;
  int ret;
  uint16_t  header_size=sizeof(int32_t) + sizeof(openair0_timestamp);

  
  /* buff[antenna_id] points to the position in rx buffer where the payload to be received will be placed
     buff2 points to the position in rx buffer where the packet header will be placed */
  void *buff2 = (void*)(buff[antenna_id]-header_size);
  
  /* we don't want to ovewrite with the header info the previous rx buffer data so we store it*/
  int32_t temp0 = *(int32_t *)buff2;
  openair0_timestamp temp1 = *(openair0_timestamp *)(buff2 + sizeof(int32_t));
  
  eth_state_t *eth = (eth_state_t*)device->priv;
  int Mod_id = device->Mod_id;
  
  bytes_received=0;
  block_cnt=0;

 /* printf("[RRH] read mod_%d %d,len %d, buff %p antenna %d\n",
     Mod_id,eth->sockfd[Mod_id],(nsamps<<2)+header_size, buff2, antenna_id);*/
	
  while(bytes_received < (int)((nsamps<<2))) {
    ret=recvfrom(eth->sockfd[Mod_id],
                 buff2+bytes_received,
                 (nsamps<<2)+header_size-bytes_received,
                 0,//MSG_DONTWAIT,
                 (struct sockaddr *)&eth->dest_addr[Mod_id],
                 (socklen_t *)&dest_addr_len[Mod_id]);

    if (ret==-1) {
      if (errno == EAGAIN) {
        perror("ETHERNET READ: ");
        return((nsamps<<2) + header_size);
      } else if (errno == EWOULDBLOCK) {
        block_cnt++;
        usleep(10);
	
        if (block_cnt == 100) return(-1);
      }
    } else {
      bytes_received+=ret;
    }
  }
  /*
    printf("Buffer head RX: nu=%d an_id=%d ts%d byte_recv=%d\n",      *(int16_t *)buff2,
    *(int16_t *)(buff2 + sizeof(int16_t)),
    *(openair0_timestamp *)(buff2 + sizeof(int32_t)),
    ret>>2);*/
  
  /* store the timestamp value from packet's header */
  *timestamp =  *(openair0_timestamp *)(buff2 + sizeof(int32_t));
  
  /* tx buffer values restored */  
  *(int32_t *)buff2 = temp0;
  *(openair0_timestamp *)(buff2 + sizeof(int32_t)) = temp1;
  // printf("Received %d samples, timestamp = %d\n",bytes_received>>2,*(int32_t*)timestamp);
  
  return nsamps;
  
}

int trx_eth_start(openair0_device *device){
  
  if (ethernet_socket_init(device)!=0) {
    return -1;
  }
  
  if (device->func_type == BBU_FUNC ) {
    return trx_eth_set_dev_conf(device);
  }
  else{
    return trx_eth_get_dev_conf(device);
  }
  return(0);
}

int trx_eth_write(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int antenna_id, int flags) {
  
  return ethernet_write_data(device,timestamp,buff,nsamps,antenna_id);
}

int trx_eth_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int antenna_id) {
  
  return(ethernet_read_data(device,ptimestamp,buff,nsamps,antenna_id));
  
}

int trx_eth_stop(int card) {
  return(0);
}

int trx_eth_set_freq(openair0_device* device, openair0_config_t *openair0_cfg,int exmimo_dump_config) {
  return(0);
}

int trx_eth_set_gains(openair0_device* device, openair0_config_t *openair0_cfg) {
  return(0);
}


int trx_eth_get_stats(openair0_device* device) {

  return(0);

}

int trx_eth_reset_stats(openair0_device* device) {

  return(0);

}

int openair0_set_gains(openair0_device* device, 
		       openair0_config_t *openair0_cfg) {

  return(0);
}

int openair0_set_frequencies(openair0_device* device, openair0_config_t *openair0_cfg, int dummy) {

  return(0);
}



void trx_eth_end(openair0_device *device) {

  eth_state_t *eth = (eth_state_t*)device->priv;
  int Mod_id = device->Mod_id;
  /*destroys socket only for the processes that call the eth_end fuction-- shutdown() for beaking the pipe */
  if ( close(eth->sockfd[Mod_id]) <0 ) {
    perror("ETHERNET: Failed to close socket");
    exit(0);
   }else {
    printf("[RRH] socket for mod_id %d has been successfully closed.",Mod_id);
   }
 
}


int openair0_dev_init_eth(openair0_device *device, openair0_config_t *openair0_cfg){

  eth_state_t *eth = (eth_state_t*)malloc(sizeof(eth_state_t));
  int card = 0;
  memset(eth, 0, sizeof(eth_state_t));
  eth->buffer_size =  (unsigned int)openair0_cfg[card].samples_per_packet*sizeof(int32_t); // buffer size = 4096 for sample_len of 1024
  eth->sample_rate = (unsigned int)openair0_cfg[card].sample_rate;
  device->priv = eth; 	

  printf("ETHERNET: Initializing openair0_device for %s ...\n", ((device->func_type == BBU_FUNC) ? "BBU": "RRH"));
  device->Mod_id           = num_devices_eth++;
  device->trx_start_func   = trx_eth_start;
  device->trx_request_func = trx_eth_request;
  device->trx_reply_func   = trx_eth_reply;
  device->trx_write_func   = trx_eth_write;
  device->trx_read_func    = trx_eth_read;  
  device->trx_get_stats_func   = trx_eth_get_stats;
  device->trx_reset_stats_func = trx_eth_reset_stats;
  device->trx_end_func = trx_eth_end;
  device->trx_stop_func = trx_eth_stop;
  device->trx_set_freq_func = trx_eth_set_freq;
  device->trx_set_gains_func = trx_eth_set_gains;
  
  memcpy((void*)&device->openair0_cfg,(void*)openair0_cfg,sizeof(openair0_config_t));
  return 0;
}
