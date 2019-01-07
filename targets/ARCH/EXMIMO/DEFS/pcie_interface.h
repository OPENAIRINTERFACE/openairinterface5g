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

/** pcie_interface.h

   Interface definitions to handle communication between PC (kernel driver & SDR application) and Leon3

   - commands from PC->Leon and Leon->PC passed through CONTROL1 register (or new: FIFOs)
   - structure for passing initial (shared PC memory) buffer pointers
   - #defines for buffer sizes

   This is now the CENTRAL header file for all structs and defines that are
   requires on both LEON and Kernel and PC Application

   Changelog:
   01.12.2012: Fix: RXEN and TXEN were swapped
               New rf_mode flag: use DMAMODE_RX and DMAMODE_TX to selectively enable DMA transfers (only used on ExpressMIMO2)
                                 use RXOUTSW to close Lime baseband RXout switch between RFFE and ADC
   09.01.2013: Renamed exmimo_rf_t.mbox --> .p_mbox to indicate this is a pointer to the mbox and not the mbox itself
   15.01.2013: added command (EXMIMO_GET_SYSTEMID) and structure (exmimo_system_id_t)
               to get system ID and bitstream and software versions on PC
   22.01.2013: Moved structures into pcie/pcie_interface.h to have all these spread-out definitions in a single place

   Author: Matthias Ihmig <matthias.ihmig@mytum.de>, 2012
           Raymond Knopp <raymond.knopp@eurecom.fr>
           Riadh Ghaddab <riadh.ghaddab@eurecom.fr>
*/

#ifndef PCIE_INTERFACE_H
#define PCIE_INTERFACE_H

#ifndef __KERNEL__
#include <stdint.h>
#endif

// ------------------------------
// Buffer sizes
// ------------------------------
#define MAX_ANTENNAS                       4
#define MAX_FIRMWARE_BLOCK_SIZE_B     262144
#define MAX_PRINTK_BUFFER_B             1024

// #define FRAME_LENGTH_COMPLEX_SAMPLES   76800
// Buffer size per channel: FRAME_LENGTH_COMPLEX_SAMPLES+2048 smp/frame: LTE frame+tail, *4 (7.68*4 MsmpPS), *4 Bytes/smp
#define ADAC_BUFFERSZ_PERCHAN_B  ((76800+2048)*4*4)

#define BIGSHM_SIZE_PAGES ((( MAX_FIRMWARE_BLOCK_SIZE_B + \
                              MAX_PRINTK_BUFFER_B + \
                              sizeof(exmimo_pci_interface_bot_t) +   \
                              sizeof(exmimo_config_t) + \
                              sizeof(exmimo_id_t) + \
                              2*MAX_ANTENNAS*sizeof(uint32_t)*4  + \
                              16*256 /* alignment overhead */  ) >> PAGE_SHIFT ) +1 )

/*
 * Commands exchanged between PC <--> LEON
 *
 * Idle/Ready: EXMIMO_NOP
 */
#define EXMIMO_NOP                  0x9999

// Commands from PC --> LEON, passed through ahbpcie_control1 register (must be smaller than 0x1000 !

#define EXMIMO_PCIE_INIT            0x0000
#define EXMIMO_FW_INIT              0x0001
#define EXMIMO_FW_CLEAR_BSS         0x0002
#define EXMIMO_FW_START_EXEC        0x0003
#define EXMIMO_REBOOT               0x0004
#define EXMIMO_CONFIG               0x0005
#define EXMIMO_GET_FRAME            0x0006
#define EXMIMO_START_RT_ACQUISITION 0x0007
#define EXMIMO_STOP                 0x0008
#define EXMIMO_STOP_WITHOUT_RESET   0x0009

#define IS_EXMIMOCMD_FROM_PC(x)  ( (x)<0x1000 ? 1 : 0)

// Commands from LEON --> PC, passed through ahbpcie_control2 register

#define PCI_PRINTK        0x2222
#define GET_FRAME_DONE    0x3333

#define EXMIMO_CONTROL2_COOKIE      0x43545232

// Default MODEM configuration in TV whitespace(400.850MHz), using RX/TX switch
#define TVWS_TDD          0xF7E04002

// 850MHz .. 865MHz
#define  DD_FDD      0xFAE40041
#define  DD_TDD      0xF7E04000
#define  B19G_TDD      0xCBC20084
#define  B24G_TDD      0xCBA20084
#define  B26G_TDD      0xCB820084
#define  B26G_FDD      0xDEE60108
#define  B35G_TDD      0xE9F01810
#define  B50G_TDD      0xC079A020


// used for DPD and dirty RF validation, TX fed back to RX through attenuators
#define  TVWS_TDD_DPD  0xF5E04000
#define  DD_TDD_DPD    0xF5E04000
#define  B19G_TDD_DPD    0xCDC20080
#define  B24G_TDD_DPD    0xCDA20080
#define  B26G_TDD_DPD    0xCD820080
#define  B35G_TDD_DPD    0xEDF01880


// -----------------------------------------------------------
// structures for communication between ExMIMO and kernel
// -----------------------------------------------------------

/*
 * struct exmimo_system_id_t:
 *   Contains information on the bitstream, filled by Leon,
 *   based on data stored in an APB block on FPGA (TODO)
 *
 * struct exmimo_id_t:
 *   Compound structure, also includes information
 *   from PCI configuration space (board_)
 */
typedef struct {
  uint32_t bitstream_id;
  uint32_t bitstream_build_date;
  uint32_t software_id;
  uint32_t software_build_date;
  uint32_t dsp_bitstream_id;
  uint32_t dsp_bitstream_build_date;
} exmimo_system_id_t;

typedef struct {
  exmimo_system_id_t system_id;
  uint32_t board_vendor;
  uint32_t board_exmimoversion;
  uint32_t board_hwrev;
  uint32_t board_swrev;
} exmimo_id_t;


/****************************************************
 * Structure and flags for configuring RF and LIME
 * (struct exmimo_rf_t)
 */
//
// register values and masks for rf_mode
//
#define RXEN 1
#define TXEN 2

#define TXLPFENMASK 4
#define TXLPFEN 4

#define TXLPFMASK     (15<<3)
#define TXLPF14        0
#define TXLPF10       (1<<3)
#define TXLPF7        (2<<3)
#define TXLPF6        (3<<3)
#define TXLPF5        (4<<3)
#define TXLPF4375     (5<<3)
#define TXLPF35       (6<<3)
#define TXLPF3        (7<<3)
#define TXLPF275      (8<<3)
#define TXLPF25       (9<<3)
#define TXLPF192      (10<<3)
#define TXLPF15       (11<<3)
#define TXLPF1375     (12<<3)
#define TXLPF125      (13<<3)
#define TXLPF0875     (14<<3)
#define TXLPF075      (15<<3)


#define RXLPFENMASK (1<<7)
#define RXLPFEN      128

#define RXLPFMASK   (15<<8)
#define RXLPF14      0
#define RXLPF10     (1<<8)
#define RXLPF7      (2<<8)
#define RXLPF6      (3<<8)
#define RXLPF5      (4<<8)
#define RXLPF4375   (5<<8)
#define RXLPF35     (6<<8)
#define RXLPF3      (7<<8)
#define RXLPF275    (8<<8)
#define RXLPF25     (9<<8)
#define RXLPF192    (10<<8)
#define RXLPF15     (11<<8)
#define RXLPF1375   (12<<8)
#define RXLPF125    (13<<8)
#define RXLPF0875   (14<<8)
#define RXLPF075    (15<<8)

#define LNAMASK     (3<<12)
#define LNADIS       0
#define LNA1ON      (1<<12)
#define LNA2ON      (2<<12)
#define LNA3ON      (3<<12)

#define LNAGAINMASK (3<<14)
#define LNAByp      (1<<14)
#define LNAMed      (2<<14)
#define LNAMax      (3<<14)

// RFBBMASK: enable different loopback modes for calibration and testing. Use RFBBNORM for normal operation
#define RFBBMASK   (7<<16)
#define RFBBNORM    0
#define RFBBRXLPF  (1<<16)
#define RFBBRXVGA  (2<<16)
#define RFBBOUTPUT (3<<16)
#define RFBBLNA1   (4<<16)
#define RFBBLNA2   (5<<16)
#define RFBBLNA3   (6<<16)

#define RXLPFMODEMASK   (3<<19)
#define RXLPFNORM        0
#define RXLPFBYP        (1<<19)
#define RXLPFBPY2       (3<<19)

#define TXLPFMODEMASK   (1<<21)
#define TXLPFNORM        0
#define TXLPFBYP        (1<<21)

#define RXOUTSW         (1<<22)

#define DMAMODE_TRXMASK (3<<23)
#define DMAMODE_RX      (1<<23)
#define DMAMODE_TX      (2<<23)


// register values and masks for rf_local
#define TXLOIMASK   63
#define TXLOQMASK  (63<<6)
#define RXLOIMASK  (63<<12)
#define RXLOQMASK  (63<<18)

typedef struct {
  uint32_t do_autocal[MAX_ANTENNAS];
  uint32_t rf_freq_rx[MAX_ANTENNAS];
  uint32_t rf_freq_tx[MAX_ANTENNAS];

  // TX Gain [Chain0..3][0=LIME Gain, 1=currently ignored]
  uint32_t tx_gain[MAX_ANTENNAS][2];

  // RX Gain [Chain0..3][0=LIME Gain, 1=currently ignored]
  uint32_t rx_gain[MAX_ANTENNAS][2];

  //LIME RF modes
  // 21    | 20:19 | 18:16 |15:14  | 13:12|11:8 |  7    |6:3  |2      |1   |0   |
  // TXBYP | RXBYP | RF/BB |LNAMode| LNA  |RXLPF|RXLPFen|TXLPF|TXLPFen|TXen|RXen|
  uint32_t rf_mode[MAX_ANTENNAS];

  // LIME LO Calibration Constants
  // | RXLOQ | RXLOI | TXLOQ | TXLOI |
  // | 23:18 | 17:12 | 11:6  | 5:0   |
  uint32_t rf_local[MAX_ANTENNAS];

  // LIME RX DC OFFSET
  // | RXDCQ | RXDCI |
  // | 15:8  | 7:0   |
  uint32_t rf_rxdc[MAX_ANTENNAS];

  // LIME VCO Calibration Constants
  // | RXVCOCAP | TXVCOCAP |
  // | 11:6     | 5:0      |
  uint32_t rf_vcocal[MAX_ANTENNAS];

  // External RF Frontend, as used on ExpressMIMO-2
  uint32_t rffe_gain_txlow[MAX_ANTENNAS];
  uint32_t rffe_gain_txhigh[MAX_ANTENNAS];
  uint32_t rffe_gain_rxfinal[MAX_ANTENNAS];
  uint32_t rffe_gain_rxlow[MAX_ANTENNAS];
  uint32_t rffe_band_mode[MAX_ANTENNAS];

} exmimo_rf_t;


/*********************************************************
 * Structure and flags for configuring Framing and Sync
 * (struct exmimo_framing_t)
 */
//
// ** multicard synchronization mode (framing.multicard_syncmode)
//
//    slave: take sync_newframe & sync_getframe from IO_SYNC header pin
//    master: enable IO_SYNC header OUTPUT (do this only on a single card!) for sync_newframe & sync_getframe
#define SYNCMODE_FREE       0
#define SYNCMODE_MASTER     1
#define SYNCMODE_SLAVE      2

//
// ** register values and masks for tdd_config
//
//    In TDD mode, there are two ways to control the RX/TX switch:
//    1. using the LSB from the TX data (TXRXSWITCH_LSB)
//       only the LSB from RF chain set in the ACTIVE_RF register controls the switch on all the chains
//    2. using FPGA logic, based on switch_offset[0..3]
//       this mode is not tested well and should be used with care
#define DUPLEXMODE_MASK   (1<<0)
#define DUPLEXMODE_FDD     0
#define DUPLEXMODE_TDD    (1<<0)
#define TXRXSWITCH_MASK   (3<<1)
#define TXRXSWITCH_LSB     0
#define TXRXSWITCH_FPGA   (1<<1)
#define TXRXSWITCH_TESTRX (2<<1)
#define TXRXSWITCH_TESTTX (3<<1)
#define SWITCHSTATE_MASK  (1<<3)
#define SWITCHSTATE_0      0
#define SWITCHSTATE_1     (1<<3)
#define TEST_ADACLOOP_MASK  (1<<4)
#define TEST_ADACLOOP_EN    (1<<4)
#define RF_ACTIVE_MASK      (15<<5)
#define RF_ACTIVE_1         (1<<5)
#define RF_ACTIVE_2         (2<<5)
#define RF_ACTIVE_3         (4<<5)
#define RF_ACTIVE_4         (8<<5)

typedef enum {
  BW5,
  BW10,
  BW20
} exmimo_bw_t;

typedef struct {
  uint32_t multicard_syncmode;

  uint32_t eNB_flag;

  uint32_t tdd_config;

  uint32_t frame_length;          // e.g. 76800
  uint32_t frame_start;           // e.g. 18
  uint32_t adac_buffer_period;    // e.g. 2048
  uint32_t adac_intr_period;      // e.g. 1024

  uint32_t switch_offset[4];      // sample offsets (relative to start of frame) used to control the RX/TX switch in TDD mode
  uint32_t resampling_factor[4];     // 0=>1, 1=>2, 2=>4, applied equally to each chain and TX/RX
} exmimo_framing_t;


typedef struct {
  exmimo_rf_t rf;
  exmimo_framing_t framing;
} exmimo_config_t;



/*****************************************
 * Structures to pass pointers
 * between Leon, Kernel and Userspace
 */

/*
 * struct exmimo_pci_interface_bot_t:
 *   Main structure to exchange physical(DMA) pointers between kernel and Leon
 */
typedef struct {
  uint32_t firmware_block_ptr;
  uint32_t printk_buffer_ptr;

  uint32_t exmimo_config_ptr;
  uint32_t exmimo_id_ptr;

  // rxcnt / txcnt: stores the current ring buffer position indicators (formerly mbox)
  // - increment by one for every 512 samples (512 DW)
  // - currently wraps at 150 (0..149) to cover 1 LTE frame in the buffer
  // - in current sdr LEON code: only rxcnt_ptr[0] is used
  uint32_t rxcnt_ptr[MAX_ANTENNAS];    // points to uint32_t RX ring buffer position for each antenna
  uint32_t txcnt_ptr[MAX_ANTENNAS];    // points to uint32_t TX ring buffer position for each antenna
  uint32_t adc_head[MAX_ANTENNAS];     // address of start of ADC (RX) buffer in PC memory
  uint32_t dac_head[MAX_ANTENNAS];     // address of start of DAC (TX) buffer in PC memory

#ifdef NOINIT
  uint32_t dummy[13];      // used only in simulation for Modelsim memcopy tests
#endif
} exmimo_pci_interface_bot_t;
/*
 * Equivalent structure to exmimo_pci_interface_bot, which keeps type-specific virtual pointers
 */
typedef struct {
  char *firmware_block_ptr;
  char *printk_buffer_ptr;
  exmimo_config_t *exmimo_config_ptr;
  exmimo_id_t *exmimo_id_ptr;
  uint32_t *rxcnt_ptr[MAX_ANTENNAS];    // points to uint32_t RX ring buffer position for each antenna
  uint32_t *txcnt_ptr[MAX_ANTENNAS];    // points to uint32_t TX ring buffer position for each antenna
  uint32_t *adc_head[MAX_ANTENNAS];     // address of start of ADC (RX) buffer in PC memory
  uint32_t *dac_head[MAX_ANTENNAS];     // address of start of DAC (TX) buffer in PC memory
} exmimo_pci_interface_bot_virtual_t;

#endif
