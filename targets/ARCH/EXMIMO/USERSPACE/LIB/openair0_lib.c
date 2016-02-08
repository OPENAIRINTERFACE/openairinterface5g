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

/** openair0_lib : API to interface with ExpressMIMO-1&2 kernel driver
*
*  Authors: Matthias Ihmig <matthias.ihmig@mytum.de>, 2013
*           Raymond Knopp <raymond.knopp@eurecom.fr>
*
*  Changelog:
*  28.01.2013: Initial version
*/

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "openair0_lib.h"
#include "openair_device.h"
#include "common_lib.h"
#define max(a,b) ((a)>(b) ? (a) : (b))
exmimo_pci_interface_bot_virtual_t openair0_exmimo_pci[MAX_CARDS]; // contains userspace pointers for each card

char *bigshm_top[MAX_CARDS] = INIT_ZEROS;

int openair0_fd;
int openair0_num_antennas[MAX_CARDS];
int openair0_num_detected_cards = 0;

unsigned int PAGE_SHIFT;

static uint32_t                      rf_local[4] =       {8255000,8255000,8255000,8255000}; // UE zepto
//{8254617, 8254617, 8254617, 8254617}; //eNB khalifa
//{8255067,8254810,8257340,8257340}; // eNB PETRONAS

static uint32_t                      rf_vcocal[4] =      {910,910,910,910};
static uint32_t                      rf_vcocal_850[4] =  {2015, 2015, 2015, 2015};
static uint32_t                      rf_rxdc[4] =        {32896,32896,32896,32896};

unsigned int log2_int( unsigned int x )
{
  unsigned int ans = 0 ;

  while( x>>=1 ) ans++;

  return ans ;
}

int openair0_open(void)
{
  exmimo_pci_interface_bot_virtual_t exmimo_pci_kvirt[MAX_CARDS];
  void *bigshm_top_kvirtptr[MAX_CARDS];

  int card;
  int ant;

  PAGE_SHIFT = log2_int( sysconf( _SC_PAGESIZE ) );


  if ((openair0_fd = open("/dev/openair0", O_RDWR,0)) <0) {
    return -1;
  }

  ioctl(openair0_fd, openair_GET_NUM_DETECTED_CARDS, &openair0_num_detected_cards);



  if ( openair0_num_detected_cards == 0 ) {
    fprintf(stderr, "No cards detected!\n");
    return -4;
  }

  ioctl(openair0_fd, openair_GET_BIGSHMTOPS_KVIRT, &bigshm_top_kvirtptr[0]);
  ioctl(openair0_fd, openair_GET_PCI_INTERFACE_BOTS_KVIRT, &exmimo_pci_kvirt[0]);

  //printf("bigshm_top_kvirtptr (MAX_CARDS %d): %p  %p  %p  %p\n", MAX_CARDS,bigshm_top_kvirtptr[0], bigshm_top_kvirtptr[1], bigshm_top_kvirtptr[2], bigshm_top_kvirtptr[3]);



  for( card=0; card < openair0_num_detected_cards; card++) {
    bigshm_top[card] = (char *)mmap( NULL,
                                     BIGSHM_SIZE_PAGES<<PAGE_SHIFT,
                                     PROT_READ|PROT_WRITE,
                                     MAP_SHARED, //|MAP_FIXED,//MAP_SHARED,
                                     openair0_fd,
                                     ( openair_mmap_BIGSHM | openair_mmap_Card(card) )<<PAGE_SHIFT);

    if (bigshm_top[card] == MAP_FAILED) {
      openair0_close();
      return -2;
    }

    // calculate userspace addresses
#if __x86_64
    openair0_exmimo_pci[card].firmware_block_ptr = (bigshm_top[card] +  (int64_t)exmimo_pci_kvirt[0].firmware_block_ptr - (int64_t)bigshm_top_kvirtptr[0]);
    openair0_exmimo_pci[card].printk_buffer_ptr  = (bigshm_top[card] +  (int64_t)exmimo_pci_kvirt[0].printk_buffer_ptr  - (int64_t)bigshm_top_kvirtptr[0]);
    openair0_exmimo_pci[card].exmimo_config_ptr  = (exmimo_config_t*) (bigshm_top[card] +  (int64_t)exmimo_pci_kvirt[0].exmimo_config_ptr  - (int64_t)bigshm_top_kvirtptr[0]);
    openair0_exmimo_pci[card].exmimo_id_ptr      = (exmimo_id_t*)     (bigshm_top[card] +  (int64_t)exmimo_pci_kvirt[0].exmimo_id_ptr      - (int64_t)bigshm_top_kvirtptr[0]);
#else
    openair0_exmimo_pci[card].firmware_block_ptr = (bigshm_top[card] +  (int32_t)exmimo_pci_kvirt[0].firmware_block_ptr - (int32_t)bigshm_top_kvirtptr[0]);
    openair0_exmimo_pci[card].printk_buffer_ptr  = (bigshm_top[card] +  (int32_t)exmimo_pci_kvirt[0].printk_buffer_ptr  - (int32_t)bigshm_top_kvirtptr[0]);
    openair0_exmimo_pci[card].exmimo_config_ptr  = (exmimo_config_t*) (bigshm_top[card] +  (int32_t)exmimo_pci_kvirt[0].exmimo_config_ptr  - (int32_t)bigshm_top_kvirtptr[0]);
    openair0_exmimo_pci[card].exmimo_id_ptr      = (exmimo_id_t*)     (bigshm_top[card] +  (int32_t)exmimo_pci_kvirt[0].exmimo_id_ptr      - (int32_t)bigshm_top_kvirtptr[0]);
#endif

    /*
          printf("openair0_exmimo_pci.firmware_block_ptr (%p) =  bigshm_top(%p) + exmimo_pci_kvirt.firmware_block_ptr(%p) - bigshm_top_kvirtptr(%p)\n",
              openair0_exmimo_pci[card].firmware_block_ptr, bigshm_top, exmimo_pci_kvirt[card].firmware_block_ptr, bigshm_top_kvirtptr[card]);
          printf("card%d, openair0_exmimo_pci.exmimo_id_ptr      (%p) =  bigshm_top(%p) + exmimo_pci_kvirt.exmimo_id_ptr     (%p) - bigshm_top_kvirtptr(%p)\n",
              card, openair0_exmimo_pci[card].exmimo_id_ptr, bigshm_top[card], exmimo_pci_kvirt[card].exmimo_id_ptr, bigshm_top_kvirtptr[card]);
    */

    /*
    if (openair0_exmimo_pci[card].exmimo_id_ptr->board_swrev != BOARD_SWREV_CNTL2)
      {
        error("Software revision %d and firmware revision %d do not match, Please update either Software or Firmware",BOARD_SWREV_CNTL2,openair0_exmimo_pci[card].exmimo_id_ptr->board_swrev);
        return -5;
      }
    */

    if ( openair0_exmimo_pci[card].exmimo_id_ptr->board_exmimoversion == 1)
      openair0_num_antennas[card] = 2;

    if ( openair0_exmimo_pci[card].exmimo_id_ptr->board_exmimoversion == 2)
      openair0_num_antennas[card] = 4;


    for (ant=0; ant<openair0_num_antennas[card]; ant++) {
#if __x86_64__
      openair0_exmimo_pci[card].rxcnt_ptr[ant] = (unsigned int *) (bigshm_top[card] +  (int64_t)exmimo_pci_kvirt[card].rxcnt_ptr[ant] - (int64_t)bigshm_top_kvirtptr[card]);
      openair0_exmimo_pci[card].txcnt_ptr[ant] = (unsigned int *) (bigshm_top[card] +  (int64_t)exmimo_pci_kvirt[card].txcnt_ptr[ant] - (int64_t)bigshm_top_kvirtptr[card]);
#else
      openair0_exmimo_pci[card].rxcnt_ptr[ant] = (unsigned int *) (bigshm_top[card] +  (int32_t)exmimo_pci_kvirt[card].rxcnt_ptr[ant] - (int32_t)bigshm_top_kvirtptr[card]);
      openair0_exmimo_pci[card].txcnt_ptr[ant] = (unsigned int *) (bigshm_top[card] +  (int32_t)exmimo_pci_kvirt[card].txcnt_ptr[ant] - (int32_t)bigshm_top_kvirtptr[card]);
#endif
    }

    for (ant=0; ant<openair0_num_antennas[card]; ant++) {
      openair0_exmimo_pci[card].adc_head[ant] = mmap( NULL,
          ADAC_BUFFERSZ_PERCHAN_B,
          PROT_READ|PROT_WRITE,
          MAP_SHARED, //|MAP_FIXED,//MAP_SHARED,
          openair0_fd,
          ( openair_mmap_RX(ant) | openair_mmap_Card(card) )<<PAGE_SHIFT );

      openair0_exmimo_pci[card].dac_head[ant] = mmap( NULL,
          ADAC_BUFFERSZ_PERCHAN_B,
          PROT_READ|PROT_WRITE,
          MAP_SHARED, //|MAP_FIXED,//MAP_SHARED,
          openair0_fd,
          ( openair_mmap_TX(ant) | openair_mmap_Card(card) )<<PAGE_SHIFT );

      if (openair0_exmimo_pci[card].adc_head[ant] == MAP_FAILED || openair0_exmimo_pci[card].dac_head[ant] == MAP_FAILED) {
        openair0_close();
        return -3;
      }
    }

    //printf("p_exmimo_config = %p, p_exmimo_id = %p\n", openair0_exmimo_pci.exmimo_config_ptr, openair0_exmimo_pci.exmimo_id_ptr);

    printf("card %d: ExpressMIMO %d, HW Rev %d, SW Rev 0x%d, %d antennas\n", card, openair0_exmimo_pci[card].exmimo_id_ptr->board_exmimoversion,
           openair0_exmimo_pci[card].exmimo_id_ptr->board_hwrev, openair0_exmimo_pci[card].exmimo_id_ptr->board_swrev, openair0_num_antennas[card]);

  } // end for(card)

  return 0;
}


int openair0_close(void)
{
  int ant;
  int card;

  close(openair0_fd);

  for (card=0; card<openair0_num_detected_cards; card++) {
    if (bigshm_top[card] != NULL && bigshm_top[card] != MAP_FAILED)
      munmap(bigshm_top[card], BIGSHM_SIZE_PAGES<<PAGE_SHIFT);

    for (ant=0; ant<openair0_num_antennas[card]; ant++) {
      if (openair0_exmimo_pci[card].adc_head[ant] != NULL && openair0_exmimo_pci[card].adc_head[ant] != MAP_FAILED)
        munmap(openair0_exmimo_pci[card].adc_head[ant], ADAC_BUFFERSZ_PERCHAN_B);

      if (openair0_exmimo_pci[card].dac_head[ant] != NULL && openair0_exmimo_pci[card].dac_head[ant] != MAP_FAILED)
        munmap(openair0_exmimo_pci[card].dac_head[ant], ADAC_BUFFERSZ_PERCHAN_B);
    }
  }

  return 0;
}

int openair0_dump_config(int card)
{
  return ioctl(openair0_fd, openair_DUMP_CONFIG, card);
}

int openair0_get_frame(int card)
{
  return ioctl(openair0_fd, openair_GET_FRAME, card);
}

int openair0_start_rt_acquisition(int card)
{
  return ioctl(openair0_fd, openair_START_RT_ACQUISITION, card);
}

int openair0_stop(int card)
{
  return ioctl(openair0_fd, openair_STOP, card);
}

int openair0_stop_without_reset(int card)
{
  return ioctl(openair0_fd, openair_STOP_WITHOUT_RESET, card);
}

#define MY_RF_MODE      (RXEN + TXEN + TXLPFNORM + TXLPFEN + TXLPF25 + RXLPFNORM + RXLPFEN + RXLPF25 + LNA1ON +LNAMax + RFBBNORM + DMAMODE_RX + DMAMODE_TX)
#define RF_MODE_BASE    (LNA1ON + RFBBNORM)

int device_init(openair0_device *device, openair0_config_t *openair0_cfg) {

  // Initialize card
  //  exmimo_config_t         *p_exmimo_config;
  exmimo_id_t             *p_exmimo_id;
  int ret;

  ret = openair0_open();


  if ( ret != 0 ) {
    if (ret == -1)
      printf("Error opening /dev/openair0");

    if (ret == -2)
      printf("Error mapping bigshm");

    if (ret == -3)
      printf("Error mapping RX or TX buffer");

    return(ret);
  }

  if (openair0_num_detected_cards>MAX_CARDS) {
    printf ("Detected %d number of cards, but MAX_CARDS=%d\n", openair0_num_detected_cards, MAX_CARDS);
  } else {
    printf ("Detected %d number of cards, %d number of antennas.\n", openair0_num_detected_cards, openair0_num_antennas[0]);
  }

  //  p_exmimo_config = openair0_exmimo_pci[0].exmimo_config_ptr;
  p_exmimo_id     = openair0_exmimo_pci[0].exmimo_id_ptr;

  printf("Card %d: ExpressMIMO %d, HW Rev %d, SW Rev 0x%d\n", 0, p_exmimo_id->board_exmimoversion, p_exmimo_id->board_hwrev, p_exmimo_id->board_swrev);

  // check if the software matches firmware
  if (p_exmimo_id->board_swrev!=BOARD_SWREV_CNTL2) {
    printf("Software revision %d and firmware revision %d do not match. Please update either the firmware or the software!\n",BOARD_SWREV_CNTL2,p_exmimo_id->board_swrev);
    return(-1);
  }

  device->type             = EXMIMO_DEV; 

  return(0);
}

int openair0_config(openair0_config_t *openair0_cfg, int UE_flag)
{
  int ret;
  int ant, card;
  int resampling_factor=2;
  int rx_filter=RXLPF25, tx_filter=TXLPF25;
  int ACTIVE_RF=0;

  exmimo_config_t         *p_exmimo_config;
  exmimo_id_t             *p_exmimo_id;

  if (!openair0_cfg) {
    printf("Error, openair0_cfg is null!!\n");
    return(-1);
  }

  for (card=0; card<openair0_num_detected_cards; card++) {

    p_exmimo_config = openair0_exmimo_pci[card].exmimo_config_ptr;
    p_exmimo_id     = openair0_exmimo_pci[card].exmimo_id_ptr;

    if (p_exmimo_id->board_swrev>=9)
      p_exmimo_config->framing.eNB_flag   = 0;
    else
      p_exmimo_config->framing.eNB_flag   = !UE_flag;

    if (openair0_num_detected_cards==1)
      p_exmimo_config->framing.multicard_syncmode=SYNCMODE_FREE;
    else if (card==0)
      p_exmimo_config->framing.multicard_syncmode=SYNCMODE_MASTER;
    else
      p_exmimo_config->framing.multicard_syncmode=SYNCMODE_SLAVE;

    /* device specific */
    openair0_cfg[card].txlaunch_wait = 1;//manage when TX processing is triggered
    openair0_cfg[card].txlaunch_wait_slotcount = 1; //manage when TX processing is triggered
    openair0_cfg[card].iq_txshift = 4;//shift
    openair0_cfg[card].iq_rxrescale = 15;//rescale iqs

    if (openair0_cfg[card].sample_rate==30.72e6) {
      resampling_factor = 0;
      rx_filter = RXLPF10;
      tx_filter = TXLPF10;
    } else if (openair0_cfg[card].sample_rate==15.36e6) {
      resampling_factor = 1;
      rx_filter = RXLPF5;
      tx_filter = TXLPF5;
    } else if (openair0_cfg[card].sample_rate==7.68e6) {
      resampling_factor = 2;
      rx_filter = RXLPF25;
      tx_filter = TXLPF25;
    } else {
      printf("Sampling rate not supported, using default 7.68MHz");
      resampling_factor = 2;
      rx_filter = RXLPF25;
      tx_filter = TXLPF25;

    }

#if (BOARD_SWREV_CNTL2>=0x0A)

    for (ant=0; ant<4; ant++)
      p_exmimo_config->framing.resampling_factor[ant] = resampling_factor;

#else
    p_exmimo_config->framing.resampling_factor = resampling_factor;
#endif

    for (ant=0; ant<4; ant++) {
      if (openair0_cfg[card].rx_freq[ant] || openair0_cfg[card].tx_freq[ant]) {
	ACTIVE_RF += (1<<ant)<<5;
        p_exmimo_config->rf.rf_mode[ant] = RF_MODE_BASE;
        p_exmimo_config->rf.do_autocal[ant] = 1;//openair0_cfg[card].autocal[ant];
	printf("card %d, antenna %d, autocal %d\n",card,ant,openair0_cfg[card].autocal[ant]);
      }

      if (openair0_cfg[card].tx_freq[ant]) {
        p_exmimo_config->rf.rf_mode[ant] += (TXEN + DMAMODE_TX + TXLPFNORM + TXLPFEN + tx_filter);
        p_exmimo_config->rf.rf_freq_tx[ant] = (unsigned int)openair0_cfg[card].tx_freq[ant];
        p_exmimo_config->rf.tx_gain[ant][0] = (unsigned int)openair0_cfg[card].tx_gain[ant];

      }

      if (openair0_cfg[card].rx_freq[ant]) {
        p_exmimo_config->rf.rf_mode[ant] += (RXEN + DMAMODE_RX + RXLPFNORM + RXLPFEN + rx_filter);

        p_exmimo_config->rf.rf_freq_rx[ant] = (unsigned int)openair0_cfg[card].rx_freq[ant];
        p_exmimo_config->rf.rx_gain[ant][0] = (unsigned int)openair0_cfg[card].rx_gain[ant];
        printf("openair0 : programming card %d RX antenna %d (freq %u, gain %d)\n",card,ant,p_exmimo_config->rf.rf_freq_rx[ant],p_exmimo_config->rf.rx_gain[ant][0]);

        switch (openair0_cfg[card].rxg_mode[ant]) {
        default:
        case max_gain:
          p_exmimo_config->rf.rf_mode[ant] += LNAMax;
          break;

        case med_gain:
          p_exmimo_config->rf.rf_mode[ant] += LNAMed;
          break;

        case byp_gain:
          p_exmimo_config->rf.rf_mode[ant] += LNAByp;
          break;
        }
      } else {
        p_exmimo_config->rf.rf_mode[ant] = 0;
        p_exmimo_config->rf.do_autocal[ant] = 0;
      }

      p_exmimo_config->rf.rf_local[ant]   = rf_local[ant];
      p_exmimo_config->rf.rf_rxdc[ant]    = rf_rxdc[ant];

      if (( p_exmimo_config->rf.rf_freq_tx[ant] >= 850000000) && ( p_exmimo_config->rf.rf_freq_tx[ant] <= 865000000)) {
        p_exmimo_config->rf.rf_vcocal[ant]  = rf_vcocal_850[ant];
        p_exmimo_config->rf.rffe_band_mode[ant] = DD_TDD;
      } else if (( p_exmimo_config->rf.rf_freq_tx[ant] >= 1900000000) && ( p_exmimo_config->rf.rf_freq_tx[ant] <= 2000000000)) {
        p_exmimo_config->rf.rf_vcocal[ant]  = rf_vcocal[ant];
        p_exmimo_config->rf.rffe_band_mode[ant] = B19G_TDD;
      } else {
        p_exmimo_config->rf.rf_vcocal[ant]  = rf_vcocal[ant];
        p_exmimo_config->rf.rffe_band_mode[ant] = 0;
      }
    }

    if (openair0_cfg[card].duplex_mode==duplex_mode_FDD) {
      p_exmimo_config->framing.tdd_config = DUPLEXMODE_FDD;
      printf("!!!!!setting FDD (tdd_config=%d)\n",p_exmimo_config->framing.tdd_config);
    } 
    else {
      p_exmimo_config->framing.tdd_config = DUPLEXMODE_TDD + TXRXSWITCH_LSB + ACTIVE_RF;
      printf("!!!!!setting TDD (tdd_config=%d)\n",p_exmimo_config->framing.tdd_config);
    }

    ret = ioctl(openair0_fd, openair_DUMP_CONFIG, card);

    if (ret!=0)
      return(-1);

  }

  return(0);
}

int openair0_reconfig(openair0_config_t *openair0_cfg)
{

  int ant, card;

  exmimo_config_t         *p_exmimo_config;
  //  exmimo_id_t             *p_exmimo_id;

  if (!openair0_cfg) {
    printf("Error, openair0_cfg is null!!\n");
    return(-1);
  }

  for (card=0; card<openair0_num_detected_cards; card++) {

    p_exmimo_config = openair0_exmimo_pci[card].exmimo_config_ptr;
    //    p_exmimo_id     = openair0_exmimo_pci[card].exmimo_id_ptr;

    for (ant=0; ant<4; ant++) {
      if (openair0_cfg[card].tx_freq[ant]) {
        p_exmimo_config->rf.rf_freq_tx[ant] = (unsigned int)openair0_cfg[card].tx_freq[ant];
        p_exmimo_config->rf.tx_gain[ant][0] = (unsigned int)openair0_cfg[card].tx_gain[ant];
        //printf("openair0 : programming TX antenna %d (freq %u, gain %d)\n",ant,p_exmimo_config->rf.rf_freq_tx[ant],p_exmimo_config->rf.tx_gain[ant][0]);
      }

      if (openair0_cfg[card].rx_freq[ant]) {
        p_exmimo_config->rf.rf_freq_rx[ant] = (unsigned int)openair0_cfg[card].rx_freq[ant];
        p_exmimo_config->rf.rx_gain[ant][0] = (unsigned int)openair0_cfg[card].rx_gain[ant];
        //printf("openair0 : programming RX antenna %d (freq %u, gain %d)\n",ant,p_exmimo_config->rf.rf_freq_rx[ant],p_exmimo_config->rf.rx_gain[ant][0]);

        switch (openair0_cfg[card].rxg_mode[ant]) {
        default:
        case max_gain:
          p_exmimo_config->rf.rf_mode[ant] = (p_exmimo_config->rf.rf_mode[ant]&(~LNAGAINMASK))|LNAMax;
          break;

        case med_gain:
          p_exmimo_config->rf.rf_mode[ant] = (p_exmimo_config->rf.rf_mode[ant]&(~LNAGAINMASK))|LNAMed;
          break;

        case byp_gain:
          p_exmimo_config->rf.rf_mode[ant] = (p_exmimo_config->rf.rf_mode[ant]&(~LNAGAINMASK))|LNAByp;
          break;
        }
      }
    }
  }

  return(0);
}

int openair0_set_frequencies(openair0_device* device, openair0_config_t *openair0_cfg,int exmimo_dump_config) {

  if (exmimo_dump_config > 0) {
    // do a full configuration
    openair0_config(openair0_cfg,0);
  }
  else {  // just change the frequencies in pci descriptor
    openair0_reconfig(openair0_cfg);
  }
  return(0);
  
}

int openair0_set_gains(openair0_device* device, openair0_config_t *openair0_cfg){
  return(0);
}

unsigned int *openair0_daq_cnt(void) {

  return((unsigned int *)openair0_exmimo_pci[0].rxcnt_ptr[0]);

}
