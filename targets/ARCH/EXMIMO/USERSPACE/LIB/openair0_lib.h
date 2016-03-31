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
