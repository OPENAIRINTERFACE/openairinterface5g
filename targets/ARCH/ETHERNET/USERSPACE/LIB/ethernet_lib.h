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

#define TX_FLAG	        1
#define RX_FLAG 	0

#include "if_defs.h"
#define APP_HEADER_SIZE_BYTES (sizeof(int32_t) + sizeof(openair0_timestamp))

/*!\brief opaque ethernet data structure */
typedef struct {
  
  /*!\brief socket file desc (control)*/ 
  int sockfdc;
  /*!\brief socket file desc (user)*/ 
  int sockfdd;
  /*!\brief interface name */ 
  char *if_name;
  /*!\brief buffer size */ 
  unsigned int buffer_size;
  /*!\brief destination address (control) for UDP socket*/
  struct sockaddr_in dest_addrc;
  /*!\brief local address (control) for UDP socket*/
  struct sockaddr_in local_addrc;
  /*!\brief destination address (user) for UDP socket*/
  struct sockaddr_in dest_addrd;
  /*!\brief local address (user) for UDP socket*/
  struct sockaddr_in local_addrd;
  /*!\brief address length for both UDP and RAW socket*/
  int addr_len;
  /*!\brief destination address (control) for RAW socket*/
  struct sockaddr_ll dest_addrc_ll;
  /*!\brief local address (control) for RAW socket*/
  struct sockaddr_ll local_addrc_ll;
  /*!\brief destination address (user) for RAW socket*/
  struct sockaddr_ll dest_addrd_ll;
  /*!\brief local address (user) for RAW socket*/
  struct sockaddr_ll local_addrd_ll;
  /*!\brief inteface index for RAW socket*/
  struct ifreq if_index;
  /*!\brief timeout ms */ 
  unsigned int rx_timeout_ms;
  /*!\brief timeout ms */ 
  unsigned int tx_timeout_ms;
  /*!\brief runtime flags */ 
  uint32_t flags;   
  /*!\compression enalbe  */
  uint32_t compression;
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
  /*!\brief number of errors in interface's transmitter */ 
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
  /*!\brief TX sequence number*/   
  uint16_t pck_seq_num;   
  /*!\brief Current RX sequence number*/   
  uint16_t pck_seq_num_cur;   
  /*!\brief Previous RX sequence number */   
  uint16_t pck_seq_num_prev;
  /*!\brief precomputed ethernet header (control) */
  struct ether_header ehc; 
  /*!\brief precomputed ethernet header (data) */
  struct ether_header ehd; 
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
  /*!\brief kernel network receive buffer maximun size */
  KERNEL_RCV_BUF_MAX_SIZE,
  /*!\brief kernel network send buffer maximun size */
  KERNEL_SND_BUF_MAX_SIZE,
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
/*void inline dump_buff(openair0_device *device, char *buff,unsigned int tx_rx_flag,int nsamps);
void inline dump_rxcounters(openair0_device *device);
void inline dump_txcounters(openair0_device *device);
*/
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


int eth_socket_init_raw(openair0_device *device);
int trx_eth_write_raw(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps,int cc, int flags);
int trx_eth_read_raw(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc);
int trx_eth_write_raw_IF4p5(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps,int cc, int flags);
int trx_eth_read_raw_IF4p5(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc);
int trx_eth_read_raw_IF5_mobipass(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc);
int trx_eth_write_udp_IF4p5(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps,int cc, int flags);
int trx_eth_read_udp_IF4p5(openair0_device *device, openair0_timestamp *timestamp, void **buff, int nsamps, int cc);
int trx_eth_ctlsend_udp(openair0_device *device, void *msg, ssize_t msg_len);
int trx_eth_ctlrecv_udp(openair0_device *device, void *msg, ssize_t msg_len);

int eth_get_dev_conf_raw(openair0_device *device);
int eth_set_dev_conf_raw(openair0_device *device);
int eth_get_dev_conf_raw_IF4p5(openair0_device *device);
int eth_set_dev_conf_raw_IF4p5(openair0_device *device);

#endif
