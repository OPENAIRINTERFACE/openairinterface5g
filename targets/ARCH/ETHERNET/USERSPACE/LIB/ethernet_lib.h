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
/*! \file ethernet_lib.h
 * \brief API to stream I/Q samples over standard ethernet
 * \author Katerina Trilyraki, Navid Nikaein
 * \date 2015
 * \version 0.2
 * \company Eurecom
 * \maintainer:  navid.nikaein@eurecom.fr
 * \note
 * \warning 
 */
#ifndef ETHERNET_LIB_H
#define ETHERNET_LIB_H

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>

#define MAX_INST      4
#define DEFAULT_IF   "lo"

#define ETH_RAW_MODE        1
#define ETH_UDP_MODE        0

#define TX_FLAG	        1
#define RX_FLAG 	0

#define MAX_PACKET_SEQ_NUM(spp,spf) (spf/spp)
#define MAC_HEADER_SIZE_BYTES (sizeof(struct ether_header))
#define APP_HEADER_SIZE_BYTES (sizeof(int32_t) + sizeof(openair0_timestamp))
#define PAYLOAD_SIZE_BYTES(nsamps) (nsamps<<2)
#define UDP_PACKET_SIZE_BYTES(nsamps) (APP_HEADER_SIZE_BYTES + PAYLOAD_SIZE_BYTES(nsamps))
#define RAW_PACKET_SIZE_BYTES(nsamps) (APP_HEADER_SIZE_BYTES + MAC_HEADER_SIZE_BYTES + PAYLOAD_SIZE_BYTES(nsamps))


/*!\brief opaque ethernet data structure */
typedef struct {
  
  /*!\brief socket file desc */ 
  int sockfd[MAX_INST];
  /*!\brief interface name */ 
  char *if_name[MAX_INST];
  /*!\brief buffer size */ 
  unsigned int buffer_size;
  /*!\brief timeout ms */ 
  unsigned int rx_timeout_ms;
  /*!\brief timeout ms */ 
  unsigned int tx_timeout_ms;
  /*!\brief runtime flags */ 
  uint32_t flags;   
  /*!\ time offset between transmiter timestamp and receiver timestamp */ 
  double tdiff;
  /*!\ calibration */
  int tx_forward_nsamps;
  
  // --------------------------------
  // Debug and output control
  // --------------------------------
  
  /*!\brief number of I/Q samples to be printed */ 
  int iqdumpcnt;

  /*!\brief number of underflows in interface */ 
  int num_underflows;
  /*!\brief number of overflows in interface */ 
  int num_overflows;
  /*!\brief number of concesutive errors in interface */ 
  int num_seq_errors;
  /*!\brief number of errors in interface's receiver */ 
  int num_rx_errors;
  /*!\brief umber of errors in interface's transmitter */ 
  int num_tx_errors;
  
  /*!\brief current TX timestamp */ 
  openair0_timestamp tx_current_ts;
  /*!\brief socket file desc */ 
  openair0_timestamp rx_current_ts;
  /*!\brief actual number of samples transmitted */ 
  uint64_t tx_actual_nsamps; 
  /*!\brief actual number of samples received */
  uint64_t rx_actual_nsamps;
  /*!\brief number of samples to be transmitted */
  uint64_t tx_nsamps; 
  /*!\brief number of samples to be received */
  uint64_t rx_nsamps;
  /*!\brief number of packets transmitted */
  uint64_t tx_count; 
  /*!\brief number of packets received */
  uint64_t rx_count;

} eth_state_t;



/*!\brief packet header */
typedef struct {
  /*!\brief packet sequence number max value=packets per frame*/
  uint16_t seq_num ;
  /*!\brief antenna port used to resynchronize */
  uint16_t antenna_id;
  /*!\brief packet's timestamp */ 
  openair0_timestamp timestamp;
} header_t;

/*!\brief different options for ethernet tuning in socket and driver level */
typedef enum {
  MIN_OPT = 0,  
  /*!\brief socket send buffer size in bytes */
  SND_BUF_SIZE,
  /*!\brief socket receive buffer size in bytes */
  RCV_BUF_SIZE,
  /*!\brief receiving timeout */
  RCV_TIMEOUT,
  /*!\brief sending timeout */
  SND_TIMEOUT,
  /*!\brief maximun transmission unit size in bytes */
  MTU_SIZE,
  /*!\brief TX queue length */
  TX_Q_LEN,
  /*!\brief RX/TX  ring parameters of ethernet device */
  RING_PAR,
  /*!\brief interruptions coalesence mechanism of ethernet device */
  COALESCE_PAR,
  /*!\brief pause parameters of ethernet device */
  PAUSE_PAR,
  MAX_OPT
} eth_opt_t;

/*
#define SND_BUF_SIZE	1
#define RCV_BUF_SIZE	1<<1
#define SND_TIMEOUT	1<<2
#define RCV_TIMEOUT	1<<3
#define MTU_SIZE        1<<4
#define TX_Q_LEN	1<<5
#define RING_PAR	1<<5
#define COALESCE_PAR	1<<6
#define PAUSE_PAR       1<<7
*/

/*!\brief I/Q samples */
typedef struct {
  /*!\brief phase  */
  short i;
  /*!\brief quadrature */
  short q;
} iqoai_t ;

void dump_packet(char *title, unsigned char* pkt, int bytes, unsigned int tx_rx_flag);
unsigned short calc_csum (unsigned short *buf, int nwords);
void dump_dev(openair0_device *device);
void inline dump_buff(openair0_device *device, char *buff,unsigned int tx_rx_flag,int nsamps);
void inline dump_rxcounters(openair0_device *device);
void inline dump_txcounters(openair0_device *device);
void dump_iqs(char * buff, int iq_cnt);



/*! \fn int ethernet_tune (openair0_device *device, unsigned int option, int value);
* \brief this function allows you to configure certain ethernet parameters in socket or device level
* \param[in] openair0 device which bears the socket
* \param[in] name of parameter to configure
* \return 0 on success, otherwise -1
* \note
* @ingroup  _oai
*/
int ethernet_tune(openair0_device *device, unsigned int option, int value);



/*! \fn int eth_socket_init_udp(openair0_device *device)
* \brief initialization of UDP Socket to communicate with one destination
* \param[in] *device openair device for which the socket will be created
* \param[out]
* \return 0 on success, otherwise -1
* \note
* @ingroup  _oai
*/
int eth_socket_init_udp(openair0_device *device);
int trx_eth_write_udp(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps,int cc, int flags);
int trx_eth_read_udp(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc);
int eth_get_dev_conf_udp(openair0_device *device);

/*! \fn static int eth_set_dev_conf_udp(openair0_device *device)
* \brief
* \param[in] *device openair device
* \param[out]
* \return 0 on success, otherwise -1
* \note
* @ingroup  _oai
*/
int eth_set_dev_conf_udp(openair0_device *device);
int eth_socket_init_raw(openair0_device *device);
int trx_eth_write_raw(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps,int cc, int flags);
int trx_eth_read_raw(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc);
int eth_get_dev_conf_raw(openair0_device *device);
int eth_set_dev_conf_raw(openair0_device *device);


#endif
