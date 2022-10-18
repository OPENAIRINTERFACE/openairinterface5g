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
#include <linux/sysctl.h>

#include "common_lib.h"
#include "ethernet_lib.h"


int num_devices_eth = 0;
struct sockaddr_in dest_addr[MAX_INST];
int dest_addr_len[MAX_INST];

int load_lib(openair0_device *device,
             openair0_config_t *openair0_cfg,
             eth_params_t *cfg,
             uint8_t flag);

int trx_eth_start(openair0_device *device)
{
    eth_state_t *eth = (eth_state_t*)device->priv;

    if (eth->flags == ETH_UDP_IF5_ECPRI_MODE) {
       AssertFatal(device->thirdparty_init != NULL, "device->thirdparty_init is null\n");
       AssertFatal(device->thirdparty_init(device) == 0, "third-party init failed\n");
       device->openair0_cfg->samples_per_packet = 256;
       eth->num_fd = 1; //max(device->openair0_cfg->rx_num_channels,device->openair0_cfg->tx_num_channels); 
       udp_ctx_t *u[1+eth->num_fd];
       device->utx = (udp_ctx_t**)malloc(sizeof(device->utx));
       for (int i=0;i<eth->num_fd;i++) {
          u[i] = malloc(sizeof(udp_ctx_t));
          u[i]->thread_id=i;
          u[i]->device = device;
	  printf("UDP Read Thread %d on core %d\n",i,device->openair0_cfg->rxfh_cores[i]);
          threadCreate(&u[i]->pthread,udp_read_thread,u[i],"udp read thread",device->openair0_cfg->rxfh_cores[i],OAI_PRIORITY_RT_MAX);
          device->utx[i] = malloc(sizeof(udp_ctx_t));
          device->utx[i]->thread_id=i;
          device->utx[i]->device = device;
	  printf("UDP Write Thread %d on core %d\n",i,device->openair0_cfg->txfh_cores[i]);
          threadCreate(&device->utx[i]->pthread,udp_write_thread,device->utx[i],"udp write thread",device->openair0_cfg->txfh_cores[i],OAI_PRIORITY_RT_MAX);
       }
       device->sampling_rate_ratio_n=1;
       device->sampling_rate_ratio_d=1;
       if (device->openair0_cfg->nr_flag==1) { // This check if RRU knows about NR numerologies
          if (device->openair0_cfg->num_rb_dl <= 162 && device->openair0_cfg->num_rb_dl > 106) {
             device->sampling_rate_ratio_n = 2;
	     device->txrx_offset=5500;
	  }
          else if (device->openair0_cfg->num_rb_dl <= 106 && device->openair0_cfg->num_rb_dl > 51) {
             device->sampling_rate_ratio_d=3;
	     device->sampling_rate_ratio_n=4;
	     device->txrx_offset=3750;
	  }
          else if (device->openair0_cfg->num_rb_dl == 51) {
             device->sampling_rate_ratio_n=1;
	     device->sampling_rate_ratio_d=1;
	     device->txrx_offset=7500;
	  }
          else AssertFatal(1==0,"num_rb_dl %d not ok with ECPRI\n",device->openair0_cfg->num_rb_dl);
       }
       else {
          if (device->openair0_cfg->num_rb_dl == 100 || device->openair0_cfg->num_rb_dl == 51) {
             device->sampling_rate_ratio_d = 1;
	     device->txrx_offset=7500;
	  }
          else if (device->openair0_cfg->num_rb_dl == 75) {
             device->sampling_rate_ratio_d = 4; 
	     device->sampling_rate_ratio_n=3;
	     device->txrx_offset=7500;
	  }
          else if (device->openair0_cfg->num_rb_dl == 50) {
             device->sampling_rate_ratio_d = 2;
	     device->txrx_offset=7500;
	  }
          else if (device->openair0_cfg->num_rb_dl == 25) {
             device->sampling_rate_ratio_d = 4;
	     device->txrx_offset=7500;
	  }
          else AssertFatal(1==0,"num_rb_dl %d not ok with ECPRI\n",device->openair0_cfg->num_rb_dl);
       }
#ifdef USE_TX_TPOOL       
       int threadCnt = device->openair0_cfg->tx_num_channels;
       if (threadCnt < 2) LOG_E(PHY,"Number of threads for gNB should be more than 1. Allocated only %d\n",threadCnt);
       char pool[80];
       sprintf(pool,"-1");
       int s_offset = 0;
       for (int icpu=1; icpu<threadCnt; icpu++) {
          sprintf(pool+2+s_offset,",-1");
          s_offset += 3;
       }
       device->threadPool = (tpool_t*)malloc(sizeof(tpool_t));
       initTpool(pool, device->threadPool, cpumeas(CPUMEAS_GETSTATE));
       // ULSCH decoder result FIFO
       device->respudpTX = (notifiedFIFO_elt_t*) malloc(sizeof(notifiedFIFO_elt_t));
       initNotifiedFIFO(device->respudpTX);
#endif
   }
   /* initialize socket */
    if (eth->flags == ETH_RAW_MODE) {
        printf("Setting ETHERNET to ETH_RAW_IF5_MODE\n");
        if (eth_socket_init_raw(device)!=0)   return -1;
        /* RRU gets device configuration - RAU sets device configuration*/

        printf("Setting Timenout to 999999 usecs\n");
        if(ethernet_tune (device,RCV_TIMEOUT,999999)!=0)  return -1;

        /*
        if (device->host_type == RAU_HOST) {
          if(eth_set_dev_conf_raw(device)!=0)  return -1;
        } else {
          if(eth_get_dev_conf_raw(device)!=0)  return -1;
          }*/

        /* adjust MTU wrt number of samples per packet */
        if(eth->compression == ALAW_COMPRESS) {
            if(ethernet_tune (device,MTU_SIZE,RAW_PACKET_SIZE_BYTES_ALAW(device->openair0_cfg->samples_per_packet))!=0)  return -1;
        } else {
            if(ethernet_tune (device,MTU_SIZE,RAW_PACKET_SIZE_BYTES(device->openair0_cfg->samples_per_packet))!=0)  return -1;
        }
        if(ethernet_tune (device,RCV_TIMEOUT,999999)!=0)  return -1;
    } else if (eth->flags == ETH_RAW_IF4p5_MODE) {

        printf("Setting ETHERNET to ETH_RAW_IF4p5_MODE\n");
        if (eth_socket_init_raw(device)!=0)   return -1;

        printf("Setting Timenout to 999999 usecs\n");
        if(ethernet_tune (device,RCV_TIMEOUT,999999)!=0)  return -1;

        /*
        if (device->host_type == RAU_HOST) {
          if(eth_set_dev_conf_raw_IF4p5(device)!=0)  return -1;
        } else {
          if(eth_get_dev_conf_raw_IF4p5(device)!=0)  return -1;
        }
        */

        /* adjust MTU wrt number of samples per packet */
        if(ethernet_tune (device,MTU_SIZE,RAW_IF4p5_PRACH_SIZE_BYTES)!=0)  return -1;

        if(ethernet_tune (device,RCV_TIMEOUT,999999)!=0)  return -1;
    } else if (eth->flags == ETH_UDP_IF4p5_MODE) {
        printf("Setting ETHERNET to UDP_IF4p5_MODE\n");
        if (eth_socket_init_udp(device)!=0)   return -1;

        printf("Setting Timeout to 999999 usecs\n");
        if(ethernet_tune (device,RCV_TIMEOUT,999999)!=0)  return -1;

        /*
        if (device->host_type == RAU_HOST) {
          if(eth_set_dev_conf_udp(device)!=0)  return -1;
        } else {
          if(eth_get_dev_conf_udp(device)!=0)  return -1;
        }
        */

        /* adjust MTU wrt number of samples per packet */
	//        if(ethernet_tune (device,MTU_SIZE,UDP_IF4p5_PRACH_SIZE_BYTES)!=0)  return -1;




    } else {
        printf("Setting ETHERNET to UDP_IF5_MODE\n");
        if (eth_socket_init_udp(device)!=0)   return -1;

        /*
        if (device->host_type == RAU_HOST) {
          if(eth_set_dev_conf_udp(device)!=0)  return -1;
        } else {
          if(eth_get_dev_conf_udp(device)!=0)  return -1;
          }*/

        //if(ethernet_tune (device,MTU_SIZE,UDP_IF4p5_PRACH_SIZE_BYTES)!=0)  return -1;

        if(ethernet_tune (device,RCV_TIMEOUT,999999)!=0)  return -1;
    }
    /* apply additional configuration */
    if(ethernet_tune (device, SND_BUF_SIZE,67108864)!=0)  return -1;
    if(ethernet_tune (device, RCV_BUF_SIZE,67108864)!=0)  return -1;
    if(ethernet_tune (device, KERNEL_SND_BUF_MAX_SIZE, 67108864)!=0)  return -1;
    if(ethernet_tune (device, KERNEL_RCV_BUF_MAX_SIZE, 67108864)!=0)  return -1;
    if(ethernet_tune (device, TX_Q_LEN, 10000)!=0)  return -1;


    return 0;
}


void trx_eth_end(openair0_device *device)
{
    eth_state_t *eth = (eth_state_t*)device->priv;
    /* destroys socket only for the processes that call the eth_end fuction-- shutdown() for beaking the pipe */
    for (int i=0;i<eth->num_fd;i++)
      if ( close(eth->sockfdd[i]) <0 ) {
          perror("ETHERNET: Failed to close socket");
          exit(0);
      } else {
          printf("[%s] socket has been successfully closed.\n",(device->host_type == RAU_HOST)? "RAU":"RRU");
      }
}



int trx_eth_stop(openair0_device *device) {
  eth_state_t *eth = (eth_state_t*)device->priv;
  
  if (eth->flags == ETH_UDP_IF5_ECPRI_MODE) {
    AssertFatal(device->thirdparty_cleanup != NULL, "device->thirdparty_cleanup is null\n");
    AssertFatal(device->thirdparty_cleanup(device) == 0, "third-party cleanup failed\n");
  }
  return(0);
}


int trx_eth_set_freq(openair0_device* device, openair0_config_t *openair0_cfg)
{
    return(0);
}


int trx_eth_set_gains(openair0_device* device, openair0_config_t *openair0_cfg)
{
    return(0);
}


int trx_eth_get_stats(openair0_device* device)
{
    return(0);
}


int trx_eth_reset_stats(openair0_device* device)
{
    return(0);
}

int trx_eth_write_init(openair0_device *device)
{
    return 0;
}

int ethernet_tune(openair0_device *device,
                  unsigned int option,
                  int value)
{
    eth_state_t *eth = (eth_state_t*)device->priv;
    struct timeval timeout;
    struct ifreq ifr;
    char system_cmd[256];
    int ret=0;
    //  int i=0;

    /****************** socket level options ************************/
    switch(option) {
    case SND_BUF_SIZE:  /* transmit socket buffer size */
      for (int i=0;i<eth->num_fd;i++)
        if (setsockopt(eth->sockfdd[i],
                       SOL_SOCKET,
                       SO_SNDBUF,
                       &value,sizeof(value))) {
            perror("[ETHERNET] setsockopt()");
        } else {
            printf("send buffer size= %d bytes\n",value);
        }
      break;

    case RCV_BUF_SIZE:   /* receive socket buffer size */
      for (int i=0;i<eth->num_fd;i++) {
        if (setsockopt(eth->sockfdd[i],
                       SOL_SOCKET,
                       SO_RCVBUF,
                       &value,sizeof(value))) {
            perror("[ETHERNET] setsockopt()");
        } else {
            printf("receive bufffer size= %d bytes\n",value);
        }
      }
      break;

    case RCV_TIMEOUT:
        timeout.tv_sec = value/1000000;
        timeout.tv_usec = value%1000000;//less than rt_period?
        if (setsockopt(eth->sockfdc,
                         SOL_SOCKET,
                         SO_RCVTIMEO,
                         (char *)&timeout,sizeof(timeout))) {
              perror("[ETHERNET] setsockopt()");
        } else {
              printf( "receive timeout= %u usec\n",(unsigned int)timeout.tv_usec);
        }
        for (int i=0;i<eth->num_fd;i++) {
          if (setsockopt(eth->sockfdd[i],
                         SOL_SOCKET,
                         SO_RCVTIMEO,
                         (char *)&timeout,sizeof(timeout))) {
              perror("[ETHERNET] setsockopt()");
          } else {
              printf( "receive timeout= %u usec\n",(unsigned int)timeout.tv_usec);
          }
        }
        break;

    case SND_TIMEOUT:
        timeout.tv_sec = value/1000000000;
        timeout.tv_usec = value%1000000000;//less than rt_period?
        if (setsockopt(eth->sockfdc,
                       SOL_SOCKET,
                       SO_SNDTIMEO,
                       (char *)&timeout,sizeof(timeout))) {
            perror("[ETHERNET] setsockopt()");
        } else {
            printf( "send timeout= %d,%d sec\n",(int)timeout.tv_sec,(int)timeout.tv_usec);
        }
        for (int i=0;i<eth->num_fd;i++) {
          if (setsockopt(eth->sockfdd[i],
                         SOL_SOCKET,
                         SO_SNDTIMEO,
                         (char *)&timeout,sizeof(timeout))) {
              perror("[ETHERNET] setsockopt()");
          } else {
              printf( "send timeout= %d,%d sec\n",(int)timeout.tv_sec,(int)timeout.tv_usec);
          }
        }
        break;


    /******************* interface level options  *************************/
    case MTU_SIZE: /* change  MTU of the eth interface */
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name,eth->if_name, sizeof(ifr.ifr_name)-1);
        ifr.ifr_mtu =value;
        for (int i=0;i<eth->num_fd;i++) {
          if (ioctl(eth->sockfdd[i],SIOCSIFMTU,(caddr_t)&ifr) < 0 )
              perror ("[ETHERNET] Can't set the MTU");
          else
              printf("[ETHERNET] %s MTU size has changed to %d\n",eth->if_name,ifr.ifr_mtu);
        }
        break;

    case TX_Q_LEN:  /* change TX queue length of eth interface */
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name,eth->if_name, sizeof(ifr.ifr_name)-1);
        ifr.ifr_qlen =value;
        for (int i=0;i<eth->num_fd;i++) {
          if (ioctl(eth->sockfdd[i],SIOCSIFTXQLEN,(caddr_t)&ifr) < 0 )
              perror ("[ETHERNET] Can't set the txqueuelen");
          else
              printf("[ETHERNET] %s txqueuelen size has changed to %d\n",eth->if_name,ifr.ifr_qlen);
        }
        break;

    /******************* device level options  *************************/
    case COALESCE_PAR:
        ret=snprintf(system_cmd,sizeof(system_cmd),"ethtool -C %s rx-usecs %d",eth->if_name,value);
        if (ret > 0) {
            ret=system(system_cmd);
            if (ret == -1) {
                fprintf (stderr,"[ETHERNET] Can't start shell to execute %s %s",system_cmd, strerror(errno));
            } else {
                printf ("[ETHERNET] status of %s is %d\n", system_cmd, WEXITSTATUS(ret));
            }
            printf("[ETHERNET] Coalesce parameters %s\n",system_cmd);
        } else {
            perror("[ETHERNET] Can't set coalesce parameters\n");
        }
        break;

    case PAUSE_PAR:
        if (value==1) ret=snprintf(system_cmd,sizeof(system_cmd),"ethtool -A %s autoneg off rx off tx off",eth->if_name);
        else if (value==0) ret=snprintf(system_cmd,sizeof(system_cmd),"ethtool -A %s autoneg on rx on tx on",eth->if_name);
        else break;
        if (ret > 0) {
            ret=system(system_cmd);
            if (ret == -1) {
                fprintf (stderr,"[ETHERNET] Can't start shell to execute %s %s",system_cmd, strerror(errno));
            } else {
                printf ("[ETHERNET] status of %s is %d\n", system_cmd, WEXITSTATUS(ret));
            }
            printf("[ETHERNET] Pause parameters %s\n",system_cmd);
        } else {
            perror("[ETHERNET] Can't set pause parameters\n");
        }
        break;

    case RING_PAR:
        ret=snprintf(system_cmd,sizeof(system_cmd),"ethtool -G %s val %d",eth->if_name,value);
        if (ret > 0) {
            ret=system(system_cmd);
            if (ret == -1) {
                fprintf (stderr,"[ETHERNET] Can't start shell to execute %s %s",system_cmd, strerror(errno));
            } else {
                printf ("[ETHERNET] status of %s is %d\n", system_cmd, WEXITSTATUS(ret));
            }
            printf("[ETHERNET] Ring parameters %s\n",system_cmd);
        } else {
            perror("[ETHERNET] Can't set ring parameters\n");
        }
        break;
    case KERNEL_RCV_BUF_MAX_SIZE:
      ret=snprintf(system_cmd,sizeof(system_cmd),"sysctl -w net.core.rmem_max=%d;sysctl -w net.core.rmem_default=%d",value,value);
      if (ret > 0) {
	ret=system(system_cmd);
	if (ret == -1) {
	  fprintf (stderr,"[ETHERNET] Can't start shell to execute %s %s",system_cmd, strerror(errno));
	} else {
	  printf ("[ETHERNET] status of %s is %d\n", system_cmd, WEXITSTATUS(ret));
	}
	printf("[ETHERNET] net core rmem %s\n",system_cmd);
      } else {
	perror("[ETHERNET] Can't set net core rmem\n");
      }
      break;
    case KERNEL_SND_BUF_MAX_SIZE:
      ret=snprintf(system_cmd,sizeof(system_cmd),"sysctl -w net.core.wmem_max=%d;sysctl -w net.core.wmem_default=%d;sysctl -w net.core.netdev_max_backlog=5000;sysctl -w net.core.optmem_max=524288;",value,value);
      if (ret > 0) {
	ret=system(system_cmd);
	if (ret == -1) {
	  fprintf (stderr,"[ETHERNET] Can't start shell to execute %s %s",system_cmd, strerror(errno));
	} else {
	  printf ("[ETHERNET] status of %s is %d\n", system_cmd, WEXITSTATUS(ret));
	}
	printf("[ETHERNET] net core wmem %s\n",system_cmd);
      } else {
	perror("[ETHERNET] Can't set net core wmem\n");
      }
      break;
    default:
        break;
    }
    return 0;
}


int transport_init(openair0_device *device,
                   openair0_config_t *openair0_cfg,
                   eth_params_t * eth_params )
{
    eth_state_t *eth = (eth_state_t*)malloc(sizeof(eth_state_t));
    memset(eth, 0, sizeof(eth_state_t));

    eth->flags = eth_params->transp_preference;

    // load third-party driver
    if (eth->flags == ETH_UDP_IF5_ECPRI_MODE) load_lib(device,openair0_cfg,eth_params,RAU_REMOTE_THIRDPARTY_RADIO_HEAD);


    if (eth_params->if_compress == 0) {
        eth->compression = NO_COMPRESS;
    } else if (eth_params->if_compress == 1) {
        eth->compression = ALAW_COMPRESS;
    } else {
        printf("transport_init: Unknown compression scheme %d - default to ALAW", eth_params->if_compress);
        eth->compression = ALAW_COMPRESS;
    }

    eth->num_fd = 1;

    printf("[ETHERNET]: Initializing openair0_device for %s ...\n", ((device->host_type == RAU_HOST) ? "RAU": "RRU"));
    printf("[ETHERNET]: num_fd %d\n",eth->num_fd);
    device->Mod_id           = 0;//num_devices_eth++;
    device->transp_type      = ETHERNET_TP;
    device->trx_start_func   = trx_eth_start;
    device->trx_get_stats_func   = trx_eth_get_stats;
    device->trx_reset_stats_func = trx_eth_reset_stats;
    device->trx_end_func         = trx_eth_end;
    device->trx_stop_func        = trx_eth_stop;
    device->trx_set_freq_func    = trx_eth_set_freq;
    device->trx_set_gains_func   = trx_eth_set_gains;
    device->trx_write_init       = trx_eth_write_init;

    device->trx_read_func2 = NULL;
    device->trx_read_func = NULL;
    device->trx_write_func2 = NULL;
    device->trx_write_func = NULL;

    if  (eth->flags == ETH_RAW_MODE) {
        device->trx_write_func   = trx_eth_write_raw;
        device->trx_read_func    = trx_eth_read_raw;
    } else if (eth->flags == ETH_UDP_MODE || eth->flags == ETH_UDP_IF5_ECPRI_MODE) {
        device->trx_write_func2   = trx_eth_write_udp;
        device->trx_read_func2    = trx_eth_read_udp;
        device->trx_ctlsend_func = trx_eth_ctlsend_udp;
        device->trx_ctlrecv_func = trx_eth_ctlrecv_udp;
        memset((void*)&device->fhstate,0,sizeof(device->fhstate));
        printf("Setting %d RX channels to waiting\n",openair0_cfg->rx_num_channels);
	for (int i=openair0_cfg->rx_num_channels;i<8;i++) device->fhstate.r[i] = 1;
    } else if (eth->flags == ETH_RAW_IF4p5_MODE) {
        device->trx_write_func   = trx_eth_write_raw_IF4p5;
        device->trx_read_func    = trx_eth_read_raw_IF4p5;
    } else if (eth->flags == ETH_UDP_IF4p5_MODE) {
        device->trx_write_func   = trx_eth_write_udp_IF4p5;
        device->trx_read_func    = trx_eth_read_udp_IF4p5;
        device->trx_ctlsend_func = trx_eth_ctlsend_udp;
        device->trx_ctlrecv_func = trx_eth_ctlrecv_udp;
    } else {
        //device->trx_write_func   = trx_eth_write_udp_IF4p5;
        //device->trx_read_func    = trx_eth_read_udp_IF4p5;
    }

    eth->if_name = eth_params->local_if_name;
    device->priv = eth;

    
    /* device specific */
    //  openair0_cfg[0].iq_rxrescale = 15;//rescale iqs
    //  openair0_cfg[0].iq_txshift = eth_params->iq_txshift;// shift
    //  openair0_cfg[0].tx_sample_advance = eth_params->tx_sample_advance;

    /* RRU does not have any information to make this configuration atm */
    /*
    if (device->host_type == RAU_HOST) {

      switch ((int)openair0_cfg[0].sample_rate) {
      case 30720000:
        openair0_cfg[0].samples_per_packet    = 3840;
        break;
      case 23040000:
        openair0_cfg[0].samples_per_packet    = 2880;
        break;
      case 15360000:
        openair0_cfg[0].samples_per_packet    = 1920;
        break;
      case 7680000:
        openair0_cfg[0].samples_per_packet    = 960;
        break;
      case 1920000:
        openair0_cfg[0].samples_per_packet    = 240;
        break;
      default:
        printf("Error: unknown sampling rate %f\n",openair0_cfg[0].sample_rate);
        exit(-1);
        break;
      }
      }*/

    device->openair0_cfg=&openair0_cfg[0];
    return 0;
}


/**************************************************************************************************************************
 *                                         DEBUGING-RELATED FUNCTIONS                                                     *
 **************************************************************************************************************************/
void dump_packet(char *title,
                 unsigned char* pkt,
                 int bytes,
                 unsigned int tx_rx_flag)
{
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


unsigned short calc_csum (unsigned short *buf,
                          int nwords)
{
    unsigned long sum;
    for (sum = 0; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return ~sum;
}


void dump_dev(openair0_device *device)
{
    eth_state_t *eth = (eth_state_t*)device->priv;

    printf("Ethernet device interface %i configuration:\n" ,device->openair0_cfg->Mod_id);
    printf("       Log level is %i :\n" ,device->openair0_cfg->log_level);
    printf("       RB number: %i, sample rate: %lf \n" ,
           device->openair0_cfg->num_rb_dl, device->openair0_cfg->sample_rate);
    printf("       RAU configured for %i tx/%i rx channels)\n",
           device->openair0_cfg->tx_num_channels,device->openair0_cfg->rx_num_channels);
    printf("       Running flags: %s %s (\n",
           ((eth->flags & ETH_RAW_MODE)  ? "RAW socket mode - ":""),
           ((eth->flags & ETH_UDP_MODE)  ? "UDP socket mode - ":""));
    printf("       Number of iqs dumped when displaying packets: %i\n\n",eth->iqdumpcnt);

}


void inline dump_txcounters(openair0_device *device)
{
    eth_state_t *eth = (eth_state_t*)device->priv;
    printf("   Ethernet device interface %i, tx counters:\n" ,device->openair0_cfg->Mod_id);
    printf("   Sent packets: %llu send errors: %i\n",   (long long unsigned int)eth->tx_count, eth->num_tx_errors);
}


void inline dump_rxcounters(openair0_device *device)
{
    eth_state_t *eth = (eth_state_t*)device->priv;
    printf("   Ethernet device interface %i rx counters:\n" ,device->openair0_cfg->Mod_id);
    printf("   Received packets: %llu missed packets errors: %i\n", (long long unsigned int)eth->rx_count, eth->num_underflows);
}


void inline dump_buff(openair0_device *device,
                      char *buff,
                      unsigned int tx_rx_flag,
                      int nsamps)
{
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


void dump_iqs(char * buff,
              int iq_cnt)
{
    int i;
    for (i=0; i<iq_cnt; i++) {
        printf("s%02i: Q=%+ij I=%+i%s",i,
               ((iqoai_t *)(buff))[i].q,
               ((iqoai_t *)(buff))[i].i,
               ((i+1)%3 == 0) ? "\n" : "  ");
    }
}
