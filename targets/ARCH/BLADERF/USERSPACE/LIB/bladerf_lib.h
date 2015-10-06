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

/** bladerf_lib.h
 *
 * Author: navid nikaein
 */

#include <libbladeRF.h>

#include "common_lib.h"
#include "log.h"

typedef struct {

  // opaque BRF data struct
  struct bladerf *dev;
  // An empty ("") or NULL device identifier will result in the first encountered device being opened (using the first discovered backend)

  unsigned int num_buffers;
  unsigned int buffer_size;
  unsigned int num_transfers;
  unsigned int rx_timeout_ms;
  unsigned int tx_timeout_ms;

  struct bladerf_metadata meta_rx;
  struct bladerf_metadata meta_tx;

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

  uint64_t tx_current_ts;
  uint64_t rx_current_ts;
  uint64_t tx_actual_nsamps;
  uint64_t rx_actual_nsamps;
  uint64_t tx_nsamps;
  uint64_t rx_nsamps;
  uint64_t tx_count;
  uint64_t rx_count;
  openair0_timestamp rx_timestamp;

} brf_state_t;
/*
 * func prototypes 
 */

int brf_error(int status);
