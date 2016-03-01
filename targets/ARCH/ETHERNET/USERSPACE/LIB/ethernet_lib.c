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


int num_devices_eth = 0;
struct sockaddr_in dest_addr[MAX_INST];
int dest_addr_len[MAX_INST];


int trx_eth_start(openair0_device *device) {

  eth_state_t *eth = (eth_state_t*)device->priv;
  
  /* initialize socket */
  if ((eth->flags & ETH_RAW_MODE) != 0 ) {     
    if (eth_socket_init_raw(device)!=0)   return -1;
    /* RRH gets openair0 device configuration - BBU sets openair0 device configuration*/
    if (device->host_type == BBU_HOST) {
      if(eth_set_dev_conf_raw(device)!=0)  return -1;
    } else {
      if(eth_get_dev_conf_raw(device)!=0)  return -1;
    }
    /* adjust MTU wrt number of samples per packet */
    if(ethernet_tune (device,MTU_SIZE,RAW_PACKET_SIZE_BYTES(device->openair0_cfg->samples_per_packet))!=0)  return -1;
  } else {
    if (eth_socket_init_udp(device)!=0)   return -1; 
    /* RRH gets openair0 device configuration - BBU sets openair0 device configuration*/
    if (device->host_type == BBU_HOST) {
      if(eth_set_dev_conf_udp(device)!=0)  return -1;
    } else {
      if(eth_get_dev_conf_udp(device)!=0)  return -1;
    }
    /* adjust MTU wrt number of samples per packet */
    //if(ethernet_tune (device,MTU_SIZE,UDP_PACKET_SIZE_BYTES(device->openair0_cfg->samples_per_packet))!=0)  return -1;
  }
  /* apply additional configuration */
  if(ethernet_tune (device, SND_BUF_SIZE,2000000000)!=0)  return -1;
  if(ethernet_tune (device, RCV_BUF_SIZE,2000000000)!=0)  return -1;
  
  return 0;
}


void trx_eth_end(openair0_device *device) {

  eth_state_t *eth = (eth_state_t*)device->priv;
  int Mod_id = device->Mod_id;
  /* destroys socket only for the processes that call the eth_end fuction-- shutdown() for beaking the pipe */
  if ( close(eth->sockfd[Mod_id]) <0 ) {
    perror("ETHERNET: Failed to close socket");
    exit(0);
   } else {
    printf("[%s] socket for mod_id %d has been successfully closed.\n",(device->host_type == BBU_HOST)? "BBU":"RRH",Mod_id);
   }
 
}


int trx_eth_request(openair0_device *device, void *msg, ssize_t msg_len) {

  int 	       Mod_id = device->Mod_id;
  eth_state_t *eth = (eth_state_t*)device->priv;
 
  /* BBU sends a message to RRH */
 if (sendto(eth->sockfd[Mod_id],msg,msg_len,0,(struct sockaddr *)&dest_addr[Mod_id],dest_addr_len[Mod_id])==-1) {
    perror("ETHERNET: ");
    exit(0);
  }
     
  return 0;
}


int trx_eth_reply(openair0_device *device, void *msg, ssize_t msg_len) {

  eth_state_t   *eth = (eth_state_t*)device->priv;
  int 		Mod_id = device->Mod_id;

  /* RRH receives from BBU a message */
  if (recvfrom(eth->sockfd[Mod_id],
	       msg,
	       msg_len,
	       0,
	       (struct sockaddr *)&dest_addr[Mod_id],
	       (socklen_t *)&dest_addr_len[Mod_id])==-1) {
    perror("ETHERNET: ");
    exit(0);
  }	
 
   return 0;
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


int ethernet_tune(openair0_device *device, unsigned int option, int value) {
  
  eth_state_t *eth = (eth_state_t*)device->priv;
  int Mod_id=device->Mod_id;
  struct timeval timeout;
  struct ifreq ifr;   
  char system_cmd[256]; 
  char* if_name=DEFAULT_IF;
  struct in_addr ia;
  struct if_nameindex *ids;
  int ret=0;
  int i=0;
  
  /****************** socket level options ************************/  
  switch(option) {
  case SND_BUF_SIZE:  /* transmit socket buffer size */   
    if (setsockopt(eth->sockfd[Mod_id],  
		   SOL_SOCKET,  
		   SO_SNDBUF,  
		   &value,sizeof(value))) {
      perror("[ETHERNET] setsockopt()");
    } else {
      printf("send buffer size= %d bytes\n",value); 
    }   
    break;
    
  case RCV_BUF_SIZE:   /* receive socket buffer size */   
    if (setsockopt(eth->sockfd[Mod_id],  
		   SOL_SOCKET,  
		   SO_RCVBUF,  
		   &value,sizeof(value))) {
      perror("[ETHERNET] setsockopt()");
    } else {     
      printf("receive bufffer size= %d bytes\n",value);    
    }
    break;
    
  case RCV_TIMEOUT:
    timeout.tv_sec = value/1000000000;
    timeout.tv_usec = value%1000000000;//less than rt_period?
    if (setsockopt(eth->sockfd[Mod_id],  
		   SOL_SOCKET,  
		   SO_RCVTIMEO,  
		   (char *)&timeout,sizeof(timeout))) {
      perror("[ETHERNET] setsockopt()");  
    } else {   
      printf( "receive timeout= %d,%d sec\n",timeout.tv_sec,timeout.tv_usec);  
    }  
    break;
    
  case SND_TIMEOUT:
    timeout.tv_sec = value/1000000000;
    timeout.tv_usec = value%1000000000;//less than rt_period?
    if (setsockopt(eth->sockfd[Mod_id],  
		   SOL_SOCKET,  
		   SO_SNDTIMEO,  
		   (char *)&timeout,sizeof(timeout))) {
      perror("[ETHERNET] setsockopt()");     
    } else {
      printf( "send timeout= %d,%d sec\n",timeout.tv_sec,timeout.tv_usec);    
    }
    break;
    
    
    /******************* interface level options  *************************/
  case MTU_SIZE: /* change  MTU of the eth interface */ 
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name,eth->if_name[Mod_id], sizeof(ifr.ifr_name));
    ifr.ifr_mtu =value;
    if (ioctl(eth->sockfd[Mod_id],SIOCSIFMTU,(caddr_t)&ifr) < 0 )
      perror ("[ETHERNET] Can't set the MTU");
    else 
      printf("[ETHERNET] %s MTU size has changed to %d\n",eth->if_name[Mod_id],ifr.ifr_mtu);
    break;
    
  case TX_Q_LEN:  /* change TX queue length of eth interface */ 
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name,eth->if_name[Mod_id], sizeof(ifr.ifr_name));
    ifr.ifr_qlen =value;
    if (ioctl(eth->sockfd[Mod_id],SIOCSIFTXQLEN,(caddr_t)&ifr) < 0 )
      perror ("[ETHERNET] Can't set the txqueuelen");
    else 
      printf("[ETHERNET] %s txqueuelen size has changed to %d\n",eth->if_name[Mod_id],ifr.ifr_qlen);
    break;
    
    /******************* device level options  *************************/
  case COALESCE_PAR:
    ret=snprintf(system_cmd,sizeof(system_cmd),"ethtool -C %s rx-usecs %d",eth->if_name[Mod_id],value);
    if (ret > 0) {
      ret=system(system_cmd);
      if (ret == -1) {
	fprintf (stderr,"[ETHERNET] Can't start shell to execute %s %s",system_cmd, strerror(errno));
      } else {
	printf ("[ETHERNET] status of %s is %i\n",WEXITSTATUS(ret));
      }
      printf("[ETHERNET] Coalesce parameters %s\n",system_cmd);
    } else {
      perror("[ETHERNET] Can't set coalesce parameters\n");
    }
    break;
    
  case PAUSE_PAR:
    if (value==1) ret=snprintf(system_cmd,sizeof(system_cmd),"ethtool -A %s autoneg off rx off tx off",eth->if_name[Mod_id]);
    else if (value==0) ret=snprintf(system_cmd,sizeof(system_cmd),"ethtool -A %s autoneg on rx on tx on",eth->if_name[Mod_id]);
    else break;
    if (ret > 0) {
      ret=system(system_cmd);
      if (ret == -1) {
	fprintf (stderr,"[ETHERNET] Can't start shell to execute %s %s",system_cmd, strerror(errno));
      } else {
	printf ("[ETHERNET] status of %s is %i\n",WEXITSTATUS(ret));
      }
      printf("[ETHERNET] Pause parameters %s\n",system_cmd);
    } else {
      perror("[ETHERNET] Can't set pause parameters\n");
    }
    break;
    
  case RING_PAR:
    ret=snprintf(system_cmd,sizeof(system_cmd),"ethtool -G %s rx %d tx %d",eth->if_name[Mod_id],value);
    if (ret > 0) {
      ret=system(system_cmd);
      if (ret == -1) {
	fprintf (stderr,"[ETHERNET] Can't start shell to execute %s %s",system_cmd, strerror(errno));
      } else {
	printf ("[ETHERNET] status of %s is %i\n",WEXITSTATUS(ret));
      }            
      printf("[ETHERNET] Ring parameters %s\n",system_cmd);
    } else {
      perror("[ETHERNET] Can't set ring parameters\n");
    }
    break;
    
  default:
    break;
  }
  
  return 0;
}



int transport_init(openair0_device *device, openair0_config_t *openair0_cfg, eth_params_t * eth_params ) {

  eth_state_t *eth = (eth_state_t*)malloc(sizeof(eth_state_t));
  memset(eth, 0, sizeof(eth_state_t));

  if (eth_params->transp_preference == 1) {
    eth->flags = ETH_RAW_MODE;
  } else {
    eth->flags = ETH_UDP_MODE;
  }
  
  printf("[ETHERNET]: Initializing openair0_device for %s ...\n", ((device->host_type == BBU_HOST) ? "BBU": "RRH"));
  device->Mod_id           = num_devices_eth++;
  device->transp_type      = ETHERNET_TP;
  device->trx_start_func   = trx_eth_start;
  device->trx_request_func = trx_eth_request;
  device->trx_reply_func   = trx_eth_reply;
  device->trx_get_stats_func   = trx_eth_get_stats;
  device->trx_reset_stats_func = trx_eth_reset_stats;
  device->trx_end_func         = trx_eth_end;
  device->trx_stop_func        = trx_eth_stop;
  device->trx_set_freq_func = trx_eth_set_freq;
  device->trx_set_gains_func = trx_eth_set_gains;

  if ((eth->flags & ETH_RAW_MODE) != 0 ) {
    device->trx_write_func   = trx_eth_write_raw;
    device->trx_read_func    = trx_eth_read_raw;     
  } else {
    device->trx_write_func   = trx_eth_write_udp;
    device->trx_read_func    = trx_eth_read_udp;     
  }

  eth->if_name[device->Mod_id] = eth_params->local_if_name;
  device->priv = eth;
 	
  /* device specific */
  openair0_cfg[0].txlaunch_wait = 0;//manage when TX processing is triggered
  openair0_cfg[0].txlaunch_wait_slotcount = 0; //manage when TX processing is triggered
  openair0_cfg[0].iq_rxrescale = 15;//rescale iqs
  openair0_cfg[0].iq_txshift = eth_params->iq_txshift;// shift
  openair0_cfg[0].tx_sample_advance = eth_params->tx_sample_advance;

  /* RRH does not have any information to make this configuration atm */
  if (device->host_type == BBU_HOST) {
    /*Note scheduling advance values valid only for case 7680000 */    
    switch ((int)openair0_cfg[0].sample_rate) {
    case 30720000:
      openair0_cfg[0].samples_per_packet    = 4096;     
      break;
    case 23040000:     
      openair0_cfg[0].samples_per_packet    = 2048;
      break;
    case 15360000:
      openair0_cfg[0].samples_per_packet    = 2048;      
      break;
    case 7680000:
      openair0_cfg[0].samples_per_packet    = 1024;     
      break;
    case 1920000:
      openair0_cfg[0].samples_per_packet    = 256;     
      break;
    default:
      printf("Error: unknown sampling rate %f\n",openair0_cfg[0].sample_rate);
      exit(-1);
      break;
    }
    openair0_cfg[0].tx_scheduling_advance = eth_params->tx_scheduling_advance*openair0_cfg[0].samples_per_packet;
  }
 
  device->openair0_cfg=&openair0_cfg[0];
  return 0;
}


/**************************************************************************************************************************
 *                                         DEBUGING-RELATED FUNCTIONS                                                     *
 **************************************************************************************************************************/
void dump_packet(char *title, unsigned char* pkt, int bytes, unsigned int tx_rx_flag) {
   
  static int numSend = 1;
  static int numRecv = 1;
  int num, k;
  char tmp[48];
  unsigned short int cksum;
  
  num = (tx_rx_flag)? numSend++:numRecv++;
  for (k = 0; k < 24; k++) sprintf(tmp+k, "%02X", pkt[k]);
  cksum = calc_csum((unsigned short *)pkt, bytes>>2);
  printf("%s-%s (%06d): %s 0x%04X\n", title,(tx_rx_flag)? "TX":"RX", num, tmp, cksum);
}

unsigned short calc_csum (unsigned short *buf, int nwords) {
 
 unsigned long sum;
  for (sum = 0; nwords > 0; nwords--)
    sum += *buf++;
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return ~sum;
}

void dump_dev(openair0_device *device) {

  eth_state_t *eth = (eth_state_t*)device->priv;
  
  printf("Ethernet device interface %i configuration:\n" ,device->openair0_cfg->Mod_id);
  printf("       Log level is %i :\n" ,device->openair0_cfg->log_level);	
  printf("       RB number: %i, sample rate: %lf \n" ,
        device->openair0_cfg->num_rb_dl, device->openair0_cfg->sample_rate);
  printf("       Scheduling_advance: %i, Sample_advance: %u \n" ,
        device->openair0_cfg->tx_scheduling_advance, device->openair0_cfg->tx_sample_advance);		
  printf("       BBU configured for %i tx/%i rx channels)\n",
	device->openair0_cfg->tx_num_channels,device->openair0_cfg->rx_num_channels);
   printf("       Running flags: %s %s %s\n",      
	((eth->flags & ETH_RAW_MODE)  ? "RAW socket mode - ":""),
	((eth->flags & ETH_UDP_MODE)  ? "UDP socket mode - ":""));	  	
  printf("       Number of iqs dumped when displaying packets: %i\n\n",eth->iqdumpcnt);   
  
}

void inline dump_txcounters(openair0_device *device) {
  eth_state_t *eth = (eth_state_t*)device->priv;  
  printf("   Ethernet device interface %i, tx counters:\n" ,device->openair0_cfg->Mod_id);
  printf("   Sent packets: %llu send errors: %i\n",   eth->tx_count, eth->num_tx_errors);	 
}

void inline dump_rxcounters(openair0_device *device) {

  eth_state_t *eth = (eth_state_t*)device->priv;
  printf("   Ethernet device interface %i rx counters:\n" ,device->openair0_cfg->Mod_id);
  printf("   Received packets: %llu missed packets errors: %i\n", eth->rx_count, eth->num_underflows);	 
}  

void inline dump_buff(openair0_device *device, char *buff,unsigned int tx_rx_flag, int nsamps) {
  
  char *strptr;
  eth_state_t *eth = (eth_state_t*)device->priv;  
  /*need to add ts number of iqs in printf need to fix dump iqs call */
  strptr = (( tx_rx_flag == TX_FLAG) ? "TX" : "RX");
  printf("\n %s, nsamps=%i \n" ,strptr,nsamps);     
  
  if (tx_rx_flag == 1) {
    dump_txcounters(device);
    printf("  First %i iqs of TX buffer\n",eth->iqdumpcnt);
    dump_iqs(buff,eth->iqdumpcnt);
  } else {
    dump_rxcounters(device);
    printf("  First %i iqs of RX buffer\n",eth->iqdumpcnt);
    dump_iqs(buff,eth->iqdumpcnt);      
  }
  
}

void dump_iqs(char * buff, int iq_cnt) {
  int i;
  for (i=0;i<iq_cnt;i++) {
    printf("s%02i: Q=%+ij I=%+i%s",i,
	   ((iqoai_t *)(buff))[i].q,
	   ((iqoai_t *)(buff))[i].i,
	   ((i+1)%3 == 0) ? "\n" : "  ");
  }   
}
