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
#ifndef __PHY_PROCEDURES_EMOS_H__
#define __PHY_PROCEDURES_EMOS_H__

/*
#ifndef RTAI
typedef uint64_t RTIME;
#else
*/
#ifdef RTAI
#include <rtai.h>
#include <rtai_fifos.h>
#endif

//#include "PHY/TOOLS/defs.h"
#include "PHY/defs.h"
#define CHANSOUNDER_FIFO_SIZE 10485760 // 10 Mbytes FIFO
//#define CHANSOUNDER_FIFO_SIZE 20971520  // 20 Mbytes FIFO
#define CHANSOUNDER_FIFO_MINOR 4               // minor of the FIFO device - this is /dev/rtf3
#define CHANSOUNDER_FIFO_DEV "/dev/rtf4"

#define NUMBER_OF_OFDM_CARRIERS_EMOS 512 // the number of OFDM carriers used for channel sounding
#define NUMBER_OF_USEFUL_CARRIERS_EMOS 300    // the number of OFDM carriers that contain data

#define N_RB_UL_EMOS 25
#define N_PILOTS_DL_EMOS 2  // ofdm symbols with pilots per slot
#define N_PILOTS_UL_EMOS 2  // ofdm symbols with pilots per subframe
#define N_SLOTS_DL_EMOS 2     // we take slots 14,15
#define N_SUBFRAMES_UL_EMOS 1     // we take subframes 4
#define NB_ANTENNAS_TX_EMOS 2
#define NB_ANTENNAS_RX_EMOS 2

struct fifo_dump_emos_struct_UE {
  // RX
  uint64_t              timestamp;              //! Timestamp of the receiver
  unsigned int     frame_tx;               //! Framenumber of the TX (encoded in the BCH)
  unsigned int     frame_rx;               //! Framenumber of the RX
  UE_MODE_t        UE_mode;
  PHY_MEASUREMENTS PHY_measurements;       //! Structure holding all PHY measurements (one for every slot)
  unsigned int     pbch_errors;                        /// Total number of errors on PBCH
  unsigned int     pbch_errors_last;                   /// Total number of errors on PBCH 100 frames ago
  unsigned int     pbch_errors_conseq;                 /// Total number of consecutive errors on PBCH
  unsigned int     pbch_fer;                           /// PBCH FER (in percent)
  unsigned int     dlsch_errors;                       /// Total number of error on the DLSCH (data)
  unsigned int     dlsch_errors_last;
  unsigned int     dlsch_received;
  unsigned int     dlsch_received_last;
  unsigned int     dlsch_fer;
  unsigned int     dlsch_cntl_errors;                  /// Total number of error on the DLSCH (control)
  unsigned int     dlsch_ra_errors;                    /// Total number of error on the DLSCH (random access)
  unsigned int     total_TBS;
  unsigned int     total_TBS_last;
  unsigned int     bitrate;
  unsigned int     total_received_bits;
  int              timing_offset;                      /// Timing offset
  int              timing_advance;                     /// Timing advance
  int              freq_offset;                        /// Frequency offset
  int              use_ia_receiver;
  unsigned short   pmi_saved;
  unsigned short   mcs;
  unsigned int     rx_total_gain_dB;                   /// Total gain
  unsigned char    eNb_id;                             /// eNb_id UE is synched to
  unsigned char    mimo_mode;                          /// Transmission mode
#ifdef EMOS_CHANNEL
  int              channel[NB_ANTENNAS_RX_EMOS][NB_ANTENNAS_TX_EMOS][NUMBER_OF_OFDM_CARRIERS_EMOS*N_PILOTS_DL_EMOS*N_SLOTS_DL_EMOS];
#endif
};

typedef struct  fifo_dump_emos_struct_UE fifo_dump_emos_UE;


struct fifo_dump_emos_struct_eNB {
  // TX
  uint64_t              timestamp;              //! Timestamp of the receiver
  unsigned int     frame_tx;               //! Framenumber of the TX
  unsigned char    mimo_mode;              /// Transmission mode
  // RX
  PHY_MEASUREMENTS_eNB PHY_measurements_eNB;            /// UL measurements
  LTE_eNB_UE_stats eNB_UE_stats[NUMBER_OF_UE_MAX]; /// Contains received feedback
  unsigned int     rx_total_gain_dB;       /// Total gain
#ifdef EMOS_CHANNEL
  int              channel[NB_ANTENNAS_RX_EMOS][N_RB_UL_EMOS*12*N_PILOTS_UL_EMOS*N_SUBFRAMES_UL_EMOS]; ///UL channel estimate
#endif
};

typedef struct  fifo_dump_emos_struct_eNB fifo_dump_emos_eNB;

#endif
