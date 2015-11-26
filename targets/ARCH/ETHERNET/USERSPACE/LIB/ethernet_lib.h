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

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>

#define MAX_INST        4
#define DEFAULT_IF  "lo"
#define BUF_SIZ      8960 /*Jumbo frame size*/

typedef struct {

  // opaque eth data struct
  //struct eth_if *dev;
  // An empty ("") or NULL device identifier will result in the first encountered device being opened (using the first discovered backend)

  int sockfd[MAX_INST];
  struct sockaddr_in dest_addr[MAX_INST];

  unsigned int buffer_size;
  unsigned int timeout_ns;

  //struct eth_metadata meta_rx;
  //struct eth_metadata meta_tx;

  unsigned int sample_rate;
  // time offset between transmiter timestamp and receiver timestamp;
  double tdiff;
  // use brf_time_offset to get this value
  int tx_forward_nsamps; //166 for 20Mhz


  // --------------------------------
  // Debug and output control
  // --------------------------------
  int num_underflows;
  int num_overflows;
  int num_seq_errors;
  int num_rx_errors;
  int num_tx_errors;

  uint64_t tx_actual_nsamps; // actual number of samples transmitted
  uint64_t rx_actual_nsamps;
  uint64_t tx_nsamps; // number of planned samples
  uint64_t rx_nsamps;
  uint64_t tx_count; // number pf packets
  uint64_t rx_count;
  //openair0_timestamp rx_timestamp;

} eth_state_t;

#define 	ETH_META_STATUS_OVERRUN   (1 << 0)
#define 	ETH_META_STATUS_UNDERRUN  (1 << 1)

struct eth_meta_data{
	uint64_t 	timestamp;
	uint32_t 	flags;	 
	uint32_t 	status;
 	unsigned int 	actual_count;
};



/*!\brief packet header */
typedef struct {
  /*!\brief packet's timestamp */ 
  openair0_timestamp timestamp;
  /*!\brief variable declared for alignment purposes (sample size=32 bit)  */
  int16_t not_used;
  /*!\brief antenna port used to resynchronize */
  int16_t antenna_id;
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



/*! \fn int ethernet_tune (openair0_device *device, eth_opt_t option)
* \brief this function allows you to configure certain ethernet parameters in socket or device level
* \param[in] openair0 device which bears the socket
* \param[in] name of parameter to configure
* \return 0 on success, otherwise -1
* \note
* @ingroup  _oai
*/
int ethernet_tune (openair0_device *device, eth_opt_t option);
int ethernet_write_data(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps,int cc) ;
int ethernet_read_data(openair0_device *device,openair0_timestamp *timestamp,void **buff, int nsamps,int cc);
