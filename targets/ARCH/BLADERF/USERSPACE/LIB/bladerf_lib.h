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

/** bladerf_lib.h
 *
 * Author: navid nikaein
 */

#include <libbladeRF.h>

#include "common_lib.h"
#include "log.h"

/** @addtogroup _BLADERF_PHY_RF_INTERFACE_
 * @{
 */

/*! \brief BladeRF specific data structure */ 
typedef struct {

  //! opaque BladeRF device struct. An empty ("") or NULL device identifier will result in the first encountered device being opened (using the first discovered backend)
  struct bladerf *dev;
  
  //! Number of buffers
  unsigned int num_buffers;
  //! Buffer size 
  unsigned int buffer_size;
  //! Number of transfers
  unsigned int num_transfers;
  //! RX timeout
  unsigned int rx_timeout_ms;
  //! TX timeout
  unsigned int tx_timeout_ms;
  //! Metadata for RX
  struct bladerf_metadata meta_rx;
  //!Metadata for TX
  struct bladerf_metadata meta_tx;
  //! Sample rate
  unsigned int sample_rate;
  //! time offset between transmiter timestamp and receiver timestamp;
  double tdiff;
  //! TX number of forward samples use brf_time_offset to get this value
  int tx_forward_nsamps; //166 for 20Mhz


  // --------------------------------
  // Debug and output control
  // --------------------------------
  //! Number of underflows
  int num_underflows;
  //! Number of overflows
  int num_overflows;
  //! number of sequential errors
  int num_seq_errors;
  //! number of RX errors
  int num_rx_errors;
  //! Number of TX errors
  int num_tx_errors;

  //! timestamp of current TX
  uint64_t tx_current_ts;
  //! timestamp of current RX
  uint64_t rx_current_ts;
  //! number of actual samples transmitted
  uint64_t tx_actual_nsamps;
  //! number of actual samples received
  uint64_t rx_actual_nsamps;
  //! number of TX samples
  uint64_t tx_nsamps;
  //! number of RX samples
  uint64_t rx_nsamps;
  //! number of TX count
  uint64_t tx_count;
  //! number of RX count
  uint64_t rx_count;
  //! timestamp of RX packet
  openair0_timestamp rx_timestamp;

} brf_state_t;
/*
 * func prototypes 
 */
/*! \brief BladeRF Init function (not used at the moment)
 * \param device RF frontend parameters set by application
 */
int trx_brf_init(openair0_device *device);

/*! \brief get current timestamp
 *\param device the hardware to use 
 *\param module the bladeRf module
 */
openair0_timestamp trx_get_timestamp(openair0_device *device, bladerf_module module);

/*! \brief Get BladeRF log level
 * \param log_level log level
 * \returns log level of BLADERF device
 */
int get_brf_log_level(int log_level);

/*! \brief bladeRF error report 
 * \param status 
 * \returns 0 on success
 */
int brf_error(int status);

/*@}*/
