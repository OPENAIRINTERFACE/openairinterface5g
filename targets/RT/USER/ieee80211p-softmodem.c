/******************************************************************************
 *
 * Copyright(c) EURECOM / Thales Communications & Security
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * Thales Communications & Security <philippe.agostini@thalesgroup.com>
 *
 *****************************************************************************/

/******************************************************************************
 *
 * Includes
 *
 *****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "ieee80211p-netlinkapi.h"
#include "phy/DOT11/defs.h"
#include "PHY/TOOLS/defs.h"
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include <rtai_lxrt.h>
#include <rtai_sem.h>
#include <rtai_msg.h>


/******************************************************************************
 *
 * Definitions
 *
 *****************************************************************************/

enum nl80211_band {
  NL80211_BAND_2GHZ,
  NL80211_BAND_5GHZ,
  NL80211_BAND_5_9GHZ,
  NL80211_BAND_0_8GHZ,
};

enum ieee80211_band {
  IEEE80211_BAND_2GHZ = NL80211_BAND_2GHZ,
  IEEE80211_BAND_5GHZ = NL80211_BAND_5GHZ,
  IEEE80211_BAND_5_9GHZ = NL80211_BAND_5_9GHZ,
  IEEE80211_BAND_0_8GHZ = NL80211_BAND_0_8GHZ,
};

struct ieee80211p_rx_status {
  short data_len; //frame data length in bytes
  char  rssi;     //received power in dBm
  char  rate;     //reveived data rate in units of 100 kbps
  enum ieee80211_band band;
  char  flags;    //RX flags
}; /* struct ieee80211p_rx_status */

extern uint32_t *txdata[2],*rxdata[2];
//CHANNEL_STATUS_t dot11_state = IDLE;
extern int oai_exit;

extern unsigned int *DAQ_MBOX;

extern int Ndbps[8];

extern int32_t rxDATA_F_comp_aggreg2[48*1024];
extern int32_t rxDATA_F_comp_aggreg3[48*1024];

extern uint32_t rxgain[4];
extern uint32_t rxg_max[4], rxg_med[4], rxg_byp[4];

#define FRAME_LENGTH_SAMPLES 76800
#define RX_THRES 60

#define SLOT_DURATION_5MHz 105
#define RX_THRES_dB 40

int tx_sdu_active = 0;
int tx_sdu_length = 0;
char rxsdu[2000];

void *tx_thread(void *arg)
{

  int fd=*((int*)arg);
  RT_TASK *task;
  int ret;
  int i;
  char dummy_data[10];


  if (fd > 0) {

    ret = netlink_send(fd,NLCMD_INIT,10,&dummy_data[0]);

    printf("tx_thread starting, fd %d\n",fd);

    task = rt_task_init_schmod(nam2num("TASK1"), 0, 0, 0, SCHED_FIFO, 0xF);
    mlockall(MCL_CURRENT | MCL_FUTURE);
    //  rt_make_hard_real_time();

    while (!oai_exit) {

      if (tx_sdu_active == 1)
        printf("tx_thread: waiting (MBOX %d)\n",((unsigned int*)DAQ_MBOX)[0]);

      while(((volatile int)tx_sdu_active) != 0) {
        rt_sleep(nano2count(66666));
      }

      printf("tx_thread: calling netlink\n");
      ret = netlink_recv(fd,rxsdu);
      tx_sdu_active = 1;
      tx_sdu_length = ret;

      /*
      if (ret > 0) {

      printf("received TX SDU: ");
      for (i=0;i<ret;i++) {
      printf("%02hhx ",rxsdu[i]);
      }

      printf("\n");

      }
      */

    }
  } else {
    printf("tx_thread: no netlink\n");
  }

  printf("tx_thread exiting\n");

  return(0);
}


/******************************************************************************
 *
 * Main
 *
 *****************************************************************************/

/*
int dot11_netlink_init() {

  int fd;
  int ret;
  int i;
  char txdata[10];

  fd = netlink_init();

  if (fd < 0) {
    return -1;
  }

  ret = netlink_send(fd,NLCMD_INIT,10,&txdata[0]);



  return(fd);
}

*/

