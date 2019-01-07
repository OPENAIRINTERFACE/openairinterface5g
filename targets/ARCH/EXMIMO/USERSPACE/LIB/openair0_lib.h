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

/** openair0_lib : API to interface with ExpressMIMO kernel driver
*
*  Authors: Matthias Ihmig <matthias.ihmig@mytum.de>, 2013
*           Raymond Knopp <raymond.knopp@eurecom.fr>
*
*  Changelog:
*  28.01.2013: Initial version
*/

#ifndef __OPENAIR0_LIB_H__
#define __OPENAIR0_LIB_H__

#include "pcie_interface.h"
#include "openair_device.h"
#include "common_lib.h"
#include <pthread.h>
#include <sched.h>
#include <linux/sched.h>

typedef enum {
  idle=0,
  waiting_for_synch,
  running
} exmimo_daq_state_t;

typedef struct {
  pthread_t watchdog;
  pthread_attr_t watchdog_attr;
  struct sched_param watchdog_sched_param;
  pthread_mutex_t watchdog_mutex;
  int watchdog_exit;
  int wait_first_read;
  exmimo_daq_state_t daq_state;
  openair0_timestamp ts;
  openair0_timestamp last_ts_rx;
  int samples_per_tick;
  int samples_per_frame;
  int last_mbox;
} exmimo_state_t;

// Use this to access shared memory (configuration structures, adc/dac data buffers, ...)
// contains userspace pointers
extern exmimo_pci_interface_bot_virtual_t openair0_exmimo_pci[MAX_CARDS];

extern int openair0_fd;

extern int openair0_num_antennas[MAX_CARDS];

extern int openair0_num_detected_cards;

// opens device and mmaps kernel memory and calculates interface and system_id userspace pointers
// return 0 on success
int openair0_open(void);

// close device and unmaps kernel memory
// return 0 on success
int openair0_close(void);

// trigger config update on card
// return 0 on success
int openair0_dump_config(int card);

// wrapper function for openair0_open (defined in common_lib.h)
// int openair0_device_init(openair0_device *device, openair0_config_t *openair0_cfg);

// copies data from openair0_cfg into exmimo_config and calls openair0_dump_config (for all cards)
int openair0_config(openair0_config_t *openair0_cfg, int UE_flag);

// copies data from openair0_cfg into exmimo_config (frequencies and gains only); does not call openair0_dump_configu (no IOCTL)
int openair0_reconfig(openair0_config_t *openair0_cfg);

// triggers recording of exactly 1 frame
// in case of synchronized multiple cards, send this only to the master card
// return 0 on success
int openair0_get_frame(int card);

// starts continuous acquisition/transmission
// in case of synchronized multiple cards, send this only to the master card
// return 0 on success
int openair0_start_rt_acquisition(int card);

// stops continuous acquitision/transmission and reset the RF chips
// return 0 on success
int openair0_stop(int card);

// stops continuous acquitision/transmission without resetting the RF chips
// return 0 on success
int openair0_stop_without_reset(int card);

// return the DAQ block counter
unsigned int *openair0_daq_cnt(void);

// set the TX and RX frequencies (card 0 only for now, to retain USRP compatibility)
int openair0_set_frequencies(openair0_device* device, openair0_config_t *openair0_cfg,int exmimo_dump_config);

int openair0_set_gains(openair0_device* device, openair0_config_t *openair0_cfg);

#endif
