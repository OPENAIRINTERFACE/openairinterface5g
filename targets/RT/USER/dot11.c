/*******************************************************************************

  Eurecom OpenAirInterface
  Copyright(c) 1999 - 2011 Eurecom

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information
  Openair Admin: openair_admin@eurecom.fr
  Openair Tech : openair_tech@eurecom.fr
  Forums       : http://forums.eurecom.fsr/openairinterface
  Address      : Eurecom, 2229, route des crêtes, 06560 Valbonne Sophia Antipolis, France

*******************************************************************************/

<<<<<<< .mine
/*! \file dot11.c
* \brief main program to control HW and scheduling for openairITS dot11 MODEM
* \author R. Knopp, F. Kaltenberger
* \date 2012
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
* \note
* \warning
*/
=======
  /*! \file dot11.c
   * \brief main program to control HW and scheduling for openairITS dot11 MODEM
   * \author R. Knopp, F. Kaltenberger
   * \date 2012
   * \version 0.1
   * \company Eurecom
   * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
   * \note
   * \warning
   */
  >>>>>>> .r3153
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>

#include <rtai_lxrt.h>
#include <rtai_sem.h>
#include <rtai_msg.h>

#include "PHY/types.h"
#include "PHY/defs.h"

#include "ARCH/COMMON/defs.h"
#include "ARCH/CBMIMO1/DEVICE_DRIVER/cbmimo1_device.h"
#include "ARCH/CBMIMO1/DEVICE_DRIVER/cbmimo1_pci.h"
#include "SIMULATION/LTE_PHY/openair_hw.h"

#include "ARCH/CBMIMO1/DEVICE_DRIVER/vars.h"
#include "SCHED/defs.h"
#include "SCHED/vars.h"


  <<<<<<< .mine
#include "phy/DOT11/defs.h"
#include "phy/DOT11/commonvars.h"
#include <malloc.h>
  =======
#include "phy/DOT11/defs.h"
#include "phy/DOT11/commonvars.h"
#include "PHY/TOOLS/defs.h"
    >>>>>>> .r3153

    <<<<<<< .mine
    =======
#include <malloc.h>
      >>>>>>> .r3153

      <<<<<<< .mine
#include "UTIL/LOG/log.h"
      =======
        >>>>>>> .r3153

        <<<<<<< .mine
#define FRAME_LENGTH_SAMPLES_MAX 100000

        uint16_t rev64[64];

int generate_test_tx=0;




=======
#include "UTIL/LOG/log.h"
#include "ieee80211p-netlinkapi.h"

#define FRAME_LENGTH_SAMPLES_MAX 100000

  uint16_t rev64[64];

int generate_test_tx=0;




>>>>>>> .r3153
#define FRAME_PERIOD 100000000ULL
#define DAQ_PERIOD 66666ULL


#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
enum nl80211_band {
  NL80211_BAND_2GHZ,
  NL80211_BAND_5GHZ,
  NL80211_BAND_5_9GHZ,
  NL80211_BAND_0_8GHZ,
};

<<<<<<< .mine

=======
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


>>>>>>> .r3153
//static CND *cond;

static int thread1;
static int thread2;

static int sync_thread;


static int instance_cnt=-1; //0 means worker is busy, -1 means its free
int instance_cnt_ptr_kern,*instance_cnt_ptr_user;
int pci_interface_ptr_kern;

extern unsigned int bigphys_top;
extern unsigned int mem_base;

int openair_fd = 0;

int oai_exit = 0;

//PCI_interface_t *pci_interface[3];

unsigned int *DAQ_MBOX;

unsigned int time_offset[4] = {0,0,0,0};

int fs4_test=0;
char UE_flag=0;

struct timing_info_t {
  unsigned int frame, hw_slot, last_slot, next_slot;
  RTIME time0, time1, time2;
  unsigned int mbox0, mbox1, mbox2, mbox_target;
} timing_info[20];

extern s16* sync_corr_ue0;
extern s16 prach_ifft[4][1024*2];

typedef enum {normal_txrx=0,rx_calib_ue=1,rx_calib_ue_med=2,rx_calib_ue_byp=3} runmode_t;

runmode_t mode;
int rx_input_level_dBm;


int otg_enabled = 0;

TX_RX_VARS dummy_tx_rx_vars;
unsigned int bigphys_top;
unsigned int mem_base;

<<<<<<< .mine
uint32_t *txdata[2],*rxdata[2];

uint8_t *data_ind = NULL;

extern int dot11_netlink_init();
extern void *rx_thread(void *);
extern void *tx_thread(void *);


void dot11_init()
{



  set_taus_seed(0);

  // Basic initializations
  init_fft(64,6,rev64);
  init_interleavers();
  ccodedot11_init();
  ccodedot11_init_inv();
  phy_generate_viterbi_tables();

  init_crc32();

}
=======
  uint32_t *txdata[2],*rxdata[2];
>>>>>>> .r3153

<<<<<<< .mine
void generate_test_tx_signal()
{
  =======
    uint8_t *data_ind = NULL;
  >>>>>>> .r3153

  <<<<<<< .mine
  TX_VECTOR_t tx_vector;
  int i;

  if (data_ind == NULL) {
    data_ind = (uint8_t*)malloc(4095+2+1);
    data_ind[0] = 0;
    data_ind[1] = 0;
  }


  tx_vector.rate=1;
  tx_vector.sdu_length=512;
  tx_vector.service=0;

  for (i=0; i<tx_vector.sdu_length; i++)
    data_ind[i+2] = taus();  // randomize packet

  data_ind[tx_vector.sdu_length+2+4]=0;  // Tail byte


  printf("Generating signal at %p\n",txdata[0]);
  phy_tx_start(&tx_vector,txdata[0],0,data_ind);

}

void signal_handler(int sig)
{
  void *array[10];
  size_t size;
  =======
    CHANNEL_STATUS_t dot11_state = IDLE;
  extern int Ndbps[8];
  >>>>>>> .r3153

  extern int32_t rxDATA_F_comp_aggreg3[48*1024];
  extern int32_t rxDATA_F_comp_aggreg2[48*1024];

#define FRAME_LENGTH_SAMPLES 76800
#define RX_THRES 60

#define SLOT_DURATION_5MHz 105
#define RX_THRES_dB 300

  u32 rxgain[4]= {30,30,30,30};
  unsigned int rxg_max[4]= {133,133,133,133}, rxg_med[4]= {127,127,127,127}, rxg_byp[4]= {120,120,120,120};

  extern int tx_sdu_active;
  extern int tx_sdu_length;
  extern char rxsdu[2000];
  int n;

  static void *rx_thread(void *arg) {

    int fd = *((int*)arg);
    int rx_offset;
    RX_VECTOR_t *rxv;
    uint8_t *data_ind_rx;
    int i;
    struct ieee80211p_rx_status *rs;
    int ret,frame;
    RT_TASK *task;
    int16_t rx_energy;
    int initial_sample_offset = 0,off=0;
    int dlen,dlen_symb;
    int mbox_off = 0,old_mbox,mbox_diff;
    int rt_skip_cond;
    int pos_crc=0,neg_crc=0;
    int sdu_received;
    int sample_threshold;
    int log2_maxh;
    struct sched_param mysched;
    int skip=0;
    int txlen;

    /*  mysched.sched_priority = 99;

        sched_setscheduler( 0, SCHED_FIFO, &mysched);
    */
    char dummy_data[16];


    if (fd>0) {
      printf("rx_thread starting, fd %d\n",fd);

      data_ind_rx = (uint8_t*)malloc(4095+2+1+12);

      task = rt_task_init_schmod(nam2num("TASK0"), 0, 0, 0, SCHED_FIFO, 0xF);
      mlockall(MCL_CURRENT | MCL_FUTURE);

      //  rt_make_hard_real_time();


      //  printf("Started rx_thread ... MBOX %d\n",((unsigned int *)DAQ_MBOX)[0]);
      // wait until MBOX gets around to zero
      i=0;

      while (((volatile unsigned int *)DAQ_MBOX)[0] != 0) {
        rt_sleep(nano2count(10000));

        if (i>1000) {
          printf("HW not counting,exiting rx_thread\n");
          return(0);
        }
      }

      //  printf("Got first MBOX = 0\n");
      // wait for first 120us
      while (((unsigned int *)DAQ_MBOX)[0] < 2)
        rt_sleep(nano2count(2*66666));

      old_mbox = ((unsigned int *)DAQ_MBOX)[0];

      //  printf("MBOX = %d\n",((unsigned int *)DAQ_MBOX)[0]);
      i = 0;
      frame = 0;
      //  oai_exit=1;
      rt_skip_cond=0;

      while (!oai_exit) {

        //    printf("While in ... mbox %d\n",((unsigned int *)DAQ_MBOX)[0]);

        rx_energy = dB_fixed_times10(signal_energy((int32_t*)(rxdata[0]+(initial_sample_offset&(~0x1))),
                                     SLOT_DURATION_5MHz - (SLOT_DURATION_5MHz&1)));

        sdu_received = 0;

        if (rx_energy>RX_THRES_dB) {
          if (initial_sample_offset < SLOT_DURATION_5MHz)
            off = initial_sample_offset + FRAME_LENGTH_SAMPLES - SLOT_DURATION_5MHz;
          else
            off = initial_sample_offset - SLOT_DURATION_5MHz;

          if (((dot11_state = initial_sync(&rxv,
                                           &rx_offset,
                                           &log2_maxh,
                                           rxdata[0],
                                           FRAME_LENGTH_SAMPLES,
                                           off,
                                           1)) == BUSY)) {


            //if ((frame % 100) == 0)
            //  printf("Channel is busy, rxv %p, offset %d\n",(void*)rxv,rx_offset);

            if (rxv) {
              rx_energy = dB_fixed_times10(signal_energy((int32_t*)(rxdata[0]+rx_offset),
                                           80));
              //    if ((frame%100) == 0)
              printf("Frame %d: Rate %d, SDU_LENGTH %d,rx_offset %d,log2_maxh %d, rxp %f dBm (dig %f,rxgain %d)\n",
                     frame,rxv->rate,rxv->sdu_length,rx_offset,log2_maxh,(rx_energy/10.0)-rxg_max[0]+30-rxgain[0],
                     rx_energy/10.0,rxg_max[0]-30+rxgain[0]);

              if ((rxv->sdu_length > 1500) || (rxv->rate > 3) )
                printf("ERROR: Frame %d: Rate %d, SDU_LENGTH %d,rx_offset %d,log2_maxh %d, rxp %f dBm (dig %f,rxgain %d)\n",
                       frame,rxv->rate,rxv->sdu_length,rx_offset,log2_maxh,(rx_energy/10.0)-rxg_max[0]+30-rxgain[0],
                       rx_energy/10.0,rxg_max[0]-30+rxgain[0]);
              else {
                memset((void*)&data_ind_rx[10],0,rxv->sdu_length+4+2+1+16);

                <<<<<<< .mine
                =======

                  if (data_detection(rxv,&data_ind_rx[10],
                                     (uint32_t*)rxdata[0],
                76800,rx_offset,log2_maxh,NULL)) {
                  pos_crc++;
                  printf("Received SDU with positive CRC\n");

                  if (fd) {
                    rs = (struct ieee80211p_rx_status *)&data_ind_rx[0];
                    rs->data_len = rxv->sdu_length;
                    rs->rssi = (char)((rx_energy/10.0)-rxg_max[0]+30-rxgain[0]);
                    rs->rate = 60;
                    rs->band = IEEE80211_BAND_0_8GHZ;
                    rs->flags = 0;
                    ret = netlink_send(fd,NLCMD_DATA,128,&data_ind_rx[0]);
                  }

                } else {
                  neg_crc++;
                  printf("Received SDU with negative CRC\n");
                  oai_exit=1;
                  write_output("rxDATA_F_comp_aggreg3.m","rxDAT_F_comp_aggreg3", rxDATA_F_comp_aggreg3,48*200,1,1);
                  write_output("rxsig_sdu.m","rxsig_sdu",&rxdata[0][rx_offset],80*40,1,1);

                  //    write_output("rxDATA_F_comp_aggreg2.m","rxDAT_F_comp_aggreg2", rxDATA_F_comp_aggreg2,48*200,1,1);
                }

                sdu_received = 1;

                //  oai_exit = 1;
                dlen      = 32+16+6+(rxv->sdu_length<<3); // data length is 32-bits CRC + sdu + 16 service + 6 tail
                dlen_symb = dlen/Ndbps[rxv->rate];

                if ((dlen%Ndbps[rxv->rate])>0)
                  dlen_symb++;

                //      printf("after dd: initial_sample_offset %d =>",initial_sample_offset);
                initial_sample_offset = rx_offset + (80*dlen_symb);
                //    printf("%d\n",initial_sample_offset);
              }
            } else {
              printf("BUSY, no synch (off %d) Frame %d (%llu us): rxp %f dBm (dig %f,rxgain %d)\n",
                     off,frame,rt_get_time_ns()/1000,(rx_energy/10.0)-rxg_max[0]+30-rxgain[0],
                     rx_energy/10.0,rxg_max[0]-30+rxgain[0]);
            }
          } else {
            /*  printf("Frame %d (%llu us): rxp %d dBm (dig %d,rxgain %d)\n",
            frame,rt_get_time_ns()/1000,rx_energy-rxg_max[0]+30-rxgain[0],
            rx_energy,rxg_max[0]-30+rxgain[0]);
            */
          }

        } else {

          if (((frame%100) == 0) && (initial_sample_offset < 2*SLOT_DURATION_5MHz)) {
            printf("Frame %d (%llu us): rxp %f dBm (dig %f,rxgain %d)\n",
                   frame,rt_get_time_ns()/1000,(rx_energy/10.0)-rxg_max[0]+30-rxgain[0],
                   rx_energy/10.0,rxg_max[0]-30+rxgain[0]);

          }

          if ((frame > 100) &&
              (tx_sdu_active == 1) &&
              (initial_sample_offset < 60000)) {

            printf("Frame %d: Generating SDU of length %d (%p), initial_sample_offset %d, MBOX <<9 %d\n",frame,tx_sdu_length,rxsdu,initial_sample_offset,DAQ_MBOX[0]<<9); /*
                                            for (n=0;n<tx_sdu_length;n++)
                                            printf("%2hhx.",rxsdu[n]);
                                            printf("\n");
                                          */
            initial_sample_offset += (8*512);

            if (initial_sample_offset > FRAME_LENGTH_SAMPLES)
              initial_sample_offset -= FRAME_LENGTH_SAMPLES;



            txlen= generate_tx_signal(initial_sample_offset);
            // wait until TX is finished

            printf("TX: txlen %d, initial_sample_offset %d\n",txlen,initial_sample_offset);
            //oai_exit=1;

            rt_sleep(nano2count((66666*8)+((txlen*66666)>>9)));
            skip = initial_sample_offset+txlen-FRAME_LENGTH_SAMPLES;

            if (skip < 0)
              skip = 0;

            printf("TX: erasing signal, MBOX %d (%d)\n",DAQ_MBOX[0],DAQ_MBOX[0]<<9);

            // erase TX signal
            for (i=0; i<(txlen-skip); i++)
              txdata[0][initial_sample_offset+i] = 0x00010001;

            for (i=0; i<skip; i++)
              txdata[0][i] = 0x00010001;


            initial_sample_offset += txlen;

            if (initial_sample_offset > FRAME_LENGTH_SAMPLES) {
              initial_sample_offset -= FRAME_LENGTH_SAMPLES;
              frame++;
              mbox_off = 0;
            }

            tx_sdu_active = 0;
            old_mbox = DAQ_MBOX[0];

          }


          //rt_sleep(nano2count(10000));
          //      printf("back from sleep 10000 ... mbox %d\n",((unsigned int *)DAQ_MBOX)[0]);

        }


        initial_sample_offset+=SLOT_DURATION_5MHz;

        if (initial_sample_offset>FRAME_LENGTH_SAMPLES) {
          initial_sample_offset-=FRAME_LENGTH_SAMPLES;
          mbox_off = 0;
          frame++;
          //  if ((frame%100) == 0)
          //printf("**** New frame %d\n",frame);

          if (frame == 100000)
            oai_exit = 1;
        }

        // sleep until HW has filled enough samples


        mbox_diff = ((unsigned int*)DAQ_MBOX)[0]-old_mbox;
        //   if ((frame%100) == 0)
        //     printf("frame %d, old_mbox %d, mbox %d (initial_sample_offset %d : mbox<<9 %d)\n",frame,old_mbox,((unsigned int*)DAQ_MBOX)[0],initial_sample_offset,((unsigned int*)DAQ_MBOX)[0]<<9);


        if ((mbox_diff>10) && (sdu_received == 0)) {
          mbox_off = 0;
          initial_sample_offset = ((unsigned int*)DAQ_MBOX)[0]<<9;
          //      printf("initial_sample_offset adjusted %d\n",initial_sample_offset);
          rt_skip_cond++;
          //      printf("old_mbox %d, mbox %d (initial_sample_offset %d : mbox<<9 %d)\n",
          //       old_mbox,((unsigned int*)DAQ_MBOX)[0],initial_sample_offset,((unsigned int*)DAQ_MBOX)[0]<<9);
          old_mbox = ((unsigned int *)DAQ_MBOX)[0];

        } else {
          if (old_mbox > ((unsigned int *)DAQ_MBOX)[0])
            mbox_off = 150;

          old_mbox = ((unsigned int *)DAQ_MBOX)[0];
        }

        /*
        printf("off: %d (%d,%d), mbox_off %d => rx_energy %d\n",initial_sample_offset,
        ((unsigned int *)DAQ_MBOX)[0],
        (initial_sample_offset>>9),mbox_off,
        rx_energy);
        */

        sample_threshold = initial_sample_offset+1024;

        if (sample_threshold > FRAME_LENGTH_SAMPLES)
          sample_threshold -= FRAME_LENGTH_SAMPLES;

        while (old_mbox+mbox_off <= (sample_threshold>>9)) {
          //     if ((frame % 100) == 0)
          //  printf("sleeping (mbox %d, mbox_off %d, initial_sample_offset>>9 %d\n",
          //         old_mbox,mbox_off,(initial_sample_offset>>9));
          rt_sleep(nano2count(66666));

          if (old_mbox > ((unsigned int *)DAQ_MBOX)[0])
            mbox_off = 150;

          old_mbox = ((unsigned int *)DAQ_MBOX)[0];
        }

        //    printf("While out ... mbox %d\n",((unsigned int *)DAQ_MBOX)[0]);

      }

      printf("rt_skip_cond %d, frames %d, pos_crc %d, neg_crc %d\n",
             rt_skip_cond,frame,pos_crc,neg_crc);

      printf("Dumping IS stats\n");
      print_is_stats();
      print_dd_stats();

      write_output("rxsig0.m","rxs", rxdata[0],76800,1,1);
      write_output("txsig0.m","txs", txdata[0],76800,1,1);
      write_output("rxDATA_F_comp_aggreg3.m","rxDAT_F_comp_aggreg3", rxDATA_F_comp_aggreg3,48*200,1,1);
      write_output("rxDATA_F_comp_aggreg2.m","rxDAT_F_comp_aggreg2", rxDATA_F_comp_aggreg2,48*200,1,1);

      printf("[DOT11][PHY] Leaving rx_thread\n");
      free(data_ind_rx);
    } else {
      printf("[DOT11][PHY] No netlink, exiting\n");
    }

    return(0);


  }

  //extern int dot11_netlink_init();
  //extern int dot11_rx_thread_init();

  //extern void *rx_thread(void *);
  extern void *tx_thread(void *);


  void dot11_init() {



    set_taus_seed(0);

    // Basic initializations
    init_fft(64,6,rev64);
    init_interleavers();
    ccodedot11_init();
    ccodedot11_init_inv();
    phy_generate_viterbi_tables();

    init_crc32();

  }

  int generate_tx_signal(int tx_offset) {

    TX_VECTOR_t tx_vector;
    int i;

    printf("Generating Signal @ %d (MBOX << 9 = %d)\n",
           tx_offset,DAQ_MBOX[0]<<9);

    if (data_ind == NULL) {
      data_ind = (uint8_t*)malloc(4095+2+1);
      data_ind[0] = 0;
      data_ind[1] = 0;
    }


    tx_vector.rate=1;
    tx_vector.sdu_length=tx_sdu_length;
    tx_vector.service=0;

    for (i=0; i<tx_vector.sdu_length; i++)
      data_ind[i+2] = rxsdu[i];

    data_ind[tx_vector.sdu_length+2+4]=0;  // Tail byte

    >>>>>>> .r3153

    //  printf("Generating signal at %p\n",txdata[0]);
    return(phy_tx_start(&tx_vector,txdata[0],tx_offset,FRAME_LENGTH_SAMPLES,data_ind));

  }

  void signal_handler(int sig) {
    void *array[10];
    size_t size;

    oai_exit=1;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, 2);
    exit(-1);
  }

  void exit_fun(const char* s) {
    void *array[10];
    size_t size;
    int fd;

    printf("Exiting: %s\n",s);

    oai_exit=1;
    rt_sleep(nano2count(FRAME_PERIOD));

    // cleanup
    stop_rt_timer();

    fd = 0;
    ioctl(openair_fd,openair_STOP,&fd);
    munmap((void*)mem_base, BIGPHYS_NUMPAGES*4096);

    exit (-1);
  }

  int dummy_tx_buffer[3840*4] __attribute__((aligned(16)));

  /* This is the main dot11 thread. */
  static void *dot11_thread(void *arg) {
    RT_TASK *task;
    int slot=0,hw_slot,last_slot, next_slot,frame=0;
    unsigned int msg1;
    unsigned int aa,slot_offset, slot_offset_F;
    int diff;
    int delay_cnt;
    RTIME time_in;
    int mbox_target=0,mbox_current=0;
    int i;

    task = rt_task_init_schmod(nam2num("TASK0"), 0, 0, 0, SCHED_FIFO, 0xF);
    mlockall(MCL_CURRENT | MCL_FUTURE);

#ifdef HARD_RT
    <<<<<<< .mine
    rt_printk("Started dot11 thread (id %p)\n",task);


    rt_make_hard_real_time();
#else

    printf("Started dot11 thread (id %p)\n",task);
#endif
    =======
      rt_printk("Started dot11 thread (id %p)\n",task);
    >>>>>>> .r3153

    <<<<<<< .mine

    while (!oai_exit) {
      //      rt_printk("eNB: slot %d\n",slot);
      =======
        >>>>>>> .r3153

        <<<<<<< .mine
        =======
          rt_make_hard_real_time();
#else
      >>>>>>> .r3153

      printf("Started dot11 thread (id %p)\n",task);


      <<<<<<< .mine

      if (frame>5) {
        if ((frame%100)==0)
#ifdef HARD_RT
          rt_printk("slot %d, hw_slot %d, next_slot %d (before): DAQ_MBOX %d\n", slot, hw_slot,next_slot,DAQ_MBOX[0]);

#else
          printf("frame %d slot %d, hw_slot %d, next_slot %d (before): DAQ_MBOX %d\n", frame,slot, hw_slot,next_slot,DAQ_MBOX[0]);
#endif

        if (fs4_test==0) {
          if ((next_slot == 0) && (generate_test_tx==1) && ((frame%100)==0)) {
            printf("Generating tx_signal in frame %d ...",frame);
            generate_test_tx_signal();
            printf("done\n");

          } else { // Check for normal TX packet
            /*for (i=0;i<3840;i++) {
            ((uint32_t *)txdata[0] + (3840*next_slot))[i] = 0x00010001;
            }*/
          }
        }


      }



      =======

      while (!oai_exit) {
        //      rt_printk("eNB: slot %d\n",slot);

        hw_slot = (((((unsigned int *)DAQ_MBOX)[0]+1)%150)<<1)/15;
        //this is the mbox counter where we should be
        mbox_target = ((((slot+1)%20)*15+1)>>1)%150;
        //this is the mbox counter where we are
        mbox_current = ((unsigned int *)DAQ_MBOX)[0];

        //this is the time we need to sleep in order to synchronize with the hw (in multiples of DAQ_PERIOD)
        if ((mbox_current>=135) && (mbox_target<15)) //handle the frame wrap-arround
          diff = 150-mbox_current+mbox_target;
        else if ((mbox_current<15) && (mbox_target>=135))
          diff = -150+mbox_target-mbox_current;
        else
          diff = mbox_target - mbox_current;

        if (diff < (-5)) {
          printf("[dot11_thread] Frame %d: missed slot, proceeding with next one (slot %d, hw_slot %d, diff %d)\n",frame, slot, hw_slot, diff);
          >>>>>>> .r3153
          slot++;

          if (slot==20)
            <<<<<<< .mine
            slot=0;

          //slot++;
          if ((slot%20)==0)
            frame++;

          =======
            slot=0;
          continue;
          >>>>>>> .r3153
        }

        if (diff>8)
          printf("[dot11_thread] eNB Frame %d: skipped slot, waiting for hw to catch up (slot %d, hw_slot %d, mbox_current %d, mbox_target %d, diff %d)\n",frame, slot, hw_slot, mbox_current, mbox_target, diff);

        delay_cnt = 0;

        while ((diff>0) && (!oai_exit)) {
          time_in = rt_get_time_ns();
          //rt_printk("eNB Frame %d delaycnt %d : hw_slot %d (%d), slot %d, (slot+1)*15=%d, diff %d, time %llu\n",frame,delay_cnt,hw_slot,((unsigned int *)DAQ_MBOX)[0],slot,(((slot+1)*15)>>1),diff,time_in);
          //rt_printk("Frame %d: slot %d, sleeping for %llu\n", frame, slot, diff*DAQ_PERIOD);
          rt_sleep(nano2count(diff*DAQ_PERIOD));
          hw_slot = (((((unsigned int *)DAQ_MBOX)[0]+1)%150)<<1)/15;
          //rt_printk("eNB Frame %d : hw_slot %d, time %llu\n",frame,hw_slot,rt_get_time_ns());
          delay_cnt++;

          if (delay_cnt == 10) {
            oai_exit = 1;
            printf("[dot11_thread]eNB Frame %d: HW stopped ... \n",frame);
          }

          mbox_current = ((unsigned int *)DAQ_MBOX)[0];

          if ((mbox_current>=135) && (mbox_target<15)) //handle the frame wrap-arround
            diff = 150-mbox_current+mbox_target;
          else
            diff = mbox_target - mbox_current;
        }


        last_slot = (slot)%LTE_SLOTS_PER_FRAME;

        if (last_slot <0)
          last_slot+=20;

        next_slot = (slot+3)%LTE_SLOTS_PER_FRAME;

        <<<<<<< .mine


        =======

        if (frame>5) {
          if ((frame%100)==0)
#ifdef HARD_RT
            rt_printk("slot %d, hw_slot %d, next_slot %d (before): DAQ_MBOX %d\n", slot, hw_slot,next_slot,DAQ_MBOX[0]);

#else
            printf("frame %d slot %d, hw_slot %d, next_slot %d (before): DAQ_MBOX %d\n", frame,slot, hw_slot,next_slot,DAQ_MBOX[0]);
#endif

          if (fs4_test==0) {
            if ((next_slot == 0) && (generate_test_tx==1) && ((frame%100)==0)) {
              printf("Generating tx_signal in frame %d ...",frame);
              //generate_test_tx_signal();
              printf("done\n");

            } else { // Check for normal TX packet
              /*for (i=0;i<3840;i++) {
                ((uint32_t *)txdata[0] + (3840*next_slot))[i] = 0x00010001;
                }*/
            }
          }


        }



        slot++;

        if (slot==20)
          slot=0;

        //slot++;
        if ((slot%20)==0)
          frame++;
      }

      rt_printk("fun0: finished, ran %d times.\n",slot);

#ifdef HARD_RT
      rt_make_soft_real_time();
#endif

      // clean task
      rt_task_delete(task);
      rt_printk("Task deleted. returning\n");
      return 0;
    }



    >>>>>>> .r3153
    int main(int argc, char **argv) {

      RT_TASK *task;
      int i,j,aa;

      LTE_DL_FRAME_PARMS *frame_parms;

      u32 carrier_freq[4]= {1907600000,1907600000,1907600000,1907600000};
      u32 rf_mode_max[4]     = {55231,55231,55231,55231};
      u32 rf_mode_med[4]     = {39375,39375,39375,39375};
      u32 rf_mode_byp[4]     = {22991,22991,22991,22991};

      u32 rf_local[4]    = {8255000,8255000,8255000,8255000}; // UE zepto
      //{8254617, 8254617, 8254617, 8254617}; //eNB khalifa
      //{8255067,8254810,8257340,8257340}; // eNB PETRONAS

      u32 rf_vcocal[4]   = {2340,2340,2340,2340};
      u32 rf_rxdc[4]     = {32896,32896,32896,32896};
      <<<<<<< .mine
      u32 rxgain[4]= {20,20,20,20};
      =======
        >>>>>>> .r3153

        <<<<<<< .mine

        =======


          >>>>>>> .r3153
          u8  eNB_id=0,UE_id=0;
      u16 Nid_cell = 0;
      u8  cooperation_flag=0, transmission_mode=1, abstraction_flag=0;
      u8 beta_ACK=0,beta_RI=0,beta_CQI=2;

      int c;
      char do_forms=0;
      unsigned int fd,dot11_netlink_fd;
      unsigned int tcxo = 114;

      int amp;

      char rxg_fname[100];
      char rflo_fname[100];
      FILE *rxg_fd=NULL;
      FILE *rflo_fd=NULL;

      <<<<<<< .mine
      =======

        >>>>>>> .r3153
        const struct option long_options[] = {
        {"calib-rx", required_argument, NULL, 256},
        {"calib-rx-med", required_argument, NULL, 257},
        {"calib-rx-byp", required_argument, NULL, 258},
        {NULL, 0, NULL, 0}
      };

      mode = normal_txrx;


      while ((c = getopt_long (argc, argv, "C:ST:dF:t",long_options,NULL)) != -1) {
        switch (c) {
        case 'd':
          do_forms=1;
          break;

        case 't':
          generate_test_tx = 1;
          break;

        case 'C':
          carrier_freq[0] = atoi(optarg);
          carrier_freq[1] = atoi(optarg);
          carrier_freq[2] = atoi(optarg);
          carrier_freq[3] = atoi(optarg);
          break;

        case 'S':
          fs4_test=1;
          break;

        case 'T':
          tcxo=atoi(optarg);
          break;

        case 'F':
          sprintf(rxg_fname,"%srxg.lime",optarg);
          rxg_fd = fopen(rxg_fname,"r");

          if (rxg_fd) {
            printf("Loading RX Gain parameters from %s\n",rxg_fname);
            fscanf(rxg_fd,"%d %d %d %d",&rxg_max[0],&rxg_max[1],&rxg_max[2],&rxg_max[3]);
            fscanf(rxg_fd,"%d %d %d %d",&rxg_med[0],&rxg_med[1],&rxg_med[2],&rxg_med[3]);
            fscanf(rxg_fd,"%d %d %d %d",&rxg_byp[0],&rxg_byp[1],&rxg_byp[2],&rxg_byp[3]);
          } else
            printf("%s not found, running with defaults\n",rxg_fname);

          sprintf(rflo_fname,"%srflo.lime",optarg);
          rflo_fd = fopen(rflo_fname,"r");

          if (rflo_fd) {
            printf("Loading RF LO parameters from %s\n",rflo_fname);
            fscanf(rflo_fd,"%d %d %d %d",&rf_local[0],&rf_local[1],&rf_local[2],&rf_local[3]);
          } else
            printf("%s not found, running with defaults\n",rflo_fname);

          break;

        case 256:
          mode = rx_calib_ue;
          rx_input_level_dBm = atoi(optarg);
          printf("Running with UE calibration on (LNA max), input level %d dBm\n",rx_input_level_dBm);
          break;

        case 257:
          mode = rx_calib_ue_med;
          rx_input_level_dBm = atoi(optarg);
          printf("Running with UE calibration on (LNA med), input level %d dBm\n",rx_input_level_dBm);
          break;

        case 258:
          mode = rx_calib_ue_byp;
          rx_input_level_dBm = atoi(optarg);
          printf("Running with UE calibration on (LNA byp), input level %d dBm\n",rx_input_level_dBm);
          break;

        default:
          break;
        }
      }


      // to make a graceful exit when ctrl-c is pressed
      signal(SIGSEGV, signal_handler);

      // init the parameters
      frame_parms = (LTE_DL_FRAME_PARMS*) malloc(sizeof(LTE_DL_FRAME_PARMS));
      frame_parms->N_RB_DL            = 25;
      frame_parms->N_RB_UL            = 25;
      frame_parms->Ncp                = 0;
      frame_parms->Ncp_UL             = 0;
      frame_parms->Nid_cell           = Nid_cell;
      frame_parms->nushift            = 0;
      frame_parms->nb_antennas_tx     = 1;
      frame_parms->nb_antennas_rx     = 1;
      frame_parms->mode1_flag         = 1; //default == SISO
      frame_parms->frame_type         = 1;

      if (fs4_test==1)
        frame_parms->tdd_config         = 255;
      else
        frame_parms->tdd_config         = 3;

      frame_parms->tdd_config_S       = 0;
      frame_parms->phich_config_common.phich_resource = oneSixth;
      frame_parms->phich_config_common.phich_duration = normal;
      frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift = 0;//n_DMRS1 set to 0

      frame_parms->node_id = NODE;

      // for Express MIMO
      for (i=0; i<4; i++) {
        frame_parms->carrier_freq[i] = carrier_freq[i];
        frame_parms->carrier_freqtx[i] = carrier_freq[i];

        <<<<<<< .mine
        frame_parms->rxgain[i]       = rxgain[i];
        =======
          frame_parms->rxgain[i]       = rxgain[i];

        frame_parms->rflocal[i]      = rf_local[i];
        frame_parms->rfvcolocal[i]   = rf_vcocal[i];
        frame_parms->rxdc[i]         = rf_rxdc[i];
        frame_parms->rfmode[i] = rf_mode_max[i];

        >>>>>>> .r3153
      }

      printf("Freq %d,%d,%d,%d, Gain %d,%d,%d,%d, RFmode %d, RXDC %d, RF_local %d, rf_vcocal %d\n",
             frame_parms->carrier_freq[0],frame_parms->carrier_freq[1],frame_parms->carrier_freq[2],frame_parms->carrier_freq[3],
             frame_parms->rxgain[0],frame_parms->rxgain[1],frame_parms->rxgain[2],frame_parms->rxgain[3],
             frame_parms->rfmode[0],frame_parms->rflocal[0],
             frame_parms->rxdc[0],frame_parms->rfvcolocal[0]);


      frame_parms->nb_prefix_samples0 = 40;
      frame_parms->nb_prefix_samples = 36;
      frame_parms->symbols_per_tti = 14;
      frame_parms->ofdm_symbol_size = 512;

      frame_parms->log2_symbol_size = 9;
      frame_parms->samples_per_tti = 7680;
      frame_parms->first_carrier_offset = frame_parms->ofdm_symbol_size - 150;

      openair_fd = setup_oai_hw(frame_parms);
      printf("Setting up buffers for Antenna port 0\n");
      setup_dot11_buffers(&(rxdata[0]),&(txdata[0]),0);
      printf("Setting up buffers for Antenna port 1\n");
      setup_dot11_buffers(&(rxdata[1]),&(txdata[1]),1);

      <<<<<<< .mine
      printf("Initializing dot11 DSP functions\n");
      dot11_init();
      dot11_netlink_fd = dot11_netlink_init();
      =======
        >>>>>>> .r3153

        <<<<<<< .mine

        for (j=0; j<76800; j+=4)
        for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
          amp = 0x8000;
          //  ((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j+1] = 0;
          //((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j+3] = amp-1;
          //((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j+5] = 0;
          //((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j+7] = amp;
          //((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j] = amp-1;
          //((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j+2] = 0;
          //((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j+4] = amp;
          //((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j+6] = 0;
        }

      sleep(1);
      printf("Calling openair_GET_PCI_INTERFACE %x\n",openair_GET_PCI_INTERFACE);
      ioctl(openair_fd,openair_GET_PCI_INTERFACE,&pci_interface_ptr_kern);

      if (pci_interface_ptr_kern == 0) {
        printf("null pci_interface_ptr, exiting\n");
        exit(-1);
      }

      =======

        for (j=0; j<76800; j+=4)
        for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
          amp = 0x8000;
          //  ((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j+1] = 0;
          //((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j+3] = amp-1;
          //((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j+5] = 0;
          //((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j+7] = amp;
          //((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j] = amp-1;
          //((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j+2] = 0;
          //((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j+4] = amp;
          //((short*)PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][aa])[2*j+6] = 0;
        }

      sleep(1);
      printf("Calling openair_GET_PCI_INTERFACE %x\n",openair_GET_PCI_INTERFACE);
      ioctl(openair_fd,openair_GET_PCI_INTERFACE,&pci_interface_ptr_kern);

      if (pci_interface_ptr_kern == 0) {
        printf("null pci_interface_ptr, exiting\n");
        exit(-1);
      }

      exmimo_pci_interface = (exmimo_pci_interface_t*) (pci_interface_ptr_kern-bigphys_top+mem_base);
      printf("pci_interface_ptr_kern = %p, exmimo_pci_interface = %p\n", (void*) pci_interface_ptr_kern, exmimo_pci_interface);
      DAQ_MBOX = (unsigned int *)(0xc0000000+exmimo_pci_interface->rf.mbox-bigphys_top+mem_base);

      printf("Initializing dot11 DSP functions\n");
      dot11_init();
      dot11_netlink_fd = netlink_init();
      >>>>>>> .r3153
      exmimo_pci_interface = (exmimo_pci_interface_t*) (pci_interface_ptr_kern-bigphys_top+mem_base);
      printf("pci_interface_ptr_kern = %p, exmimo_pci_interface = %p\n", (void*) pci_interface_ptr_kern, exmimo_pci_interface);
      DAQ_MBOX = (unsigned int *)(0xc0000000+exmimo_pci_interface->rf.mbox-bigphys_top+mem_base);

      <<<<<<< .mine
      =======
        printf("dot11_netlink_fd %d\n",dot11_netlink_fd);



      >>>>>>> .r3153
      // make main thread LXRT soft realtime
      printf("Starting LXRT ...");
      task = rt_task_init_schmod(nam2num("MYTASK"), 9, 0, 0, SCHED_FIFO, 0xF);
      mlockall(MCL_CURRENT | MCL_FUTURE);

      // start realtime timer and scheduler
      //rt_set_oneshot_mode();
      rt_set_periodic_mode();
      start_rt_timer(0);
      <<<<<<< .mine
      printf(" done\n");
      =======
        printf(" done\n");

      >>>>>>> .r3153
      <<<<<<< .mine
      //now = rt_get_time() + 10*PERIOD;
      //rt_task_make_periodic(task, now, PERIOD);

      // initialize the instance cnt before starting the thread
      //  instance_cnt_ptr_user = &instance_cnt;



      // signal the driver to set up for user-space operation
      // this will initialize the semaphore and the task pointers in the kernel
      // further we receive back the pointer to the shared instance counter which is used to signal if the thread is busy or not. This pointer needs to be mapped to user space.
      /*
      ioctl(openair_fd,openair_START_LXRT,&instance_cnt_ptr_kern);
      instance_cnt_ptr_user = (int*) (instance_cnt_ptr_kern -bigphys_top+mem_base);
      *instance_cnt_ptr_user = -1;
      printf("instance_cnt_ptr_kern %p, instance_cnt_ptr_user %p, *instance_cnt_ptr_user %d\n", (void*) instance_cnt_ptr_kern, (void*) instance_cnt_ptr_user,*instance_cnt_ptr_user);
      */

      =======
        >>>>>>> .r3153

        <<<<<<< .mine


        rt_sleep(nano2count(FRAME_PERIOD));
      =======
        >>>>>>> .r3153
        // this starts the DMA transfers
        <<<<<<< .mine
        =======
          ioctl(openair_fd,openair_START_TX_SIG,NULL);
      //ioctl(openair_fd,openair_GET_BUFFER,NULL);
      >>>>>>> .r3153

      <<<<<<< .mine
      ioctl(openair_fd,openair_START_TX_SIG,NULL);


      =======
        >>>>>>> .r3153
        rt_sleep(nano2count(10*FRAME_PERIOD));


      <<<<<<< .mine
      thread1 = rt_thread_create(dot11_thread, NULL, 100000000);
      =======
        //thread1 = rt_thread_create(dot11_thread, NULL, 100000000);
        >>>>>>> .r3153


        <<<<<<< .mine
        printf("thread created\n");
      =======
        thread1 = rt_thread_create(rx_thread, &dot11_netlink_fd, 10000000);
      >>>>>>> .r3153

      thread2 = rt_thread_create(tx_thread, &dot11_netlink_fd, 10000000);

      // wait for end of program
      printf("TYPE <ENTER> TO TERMINATE main thread\n");
      getchar();

      // stop threads
      rt_sleep(nano2count(FRAME_PERIOD));

      stop_rt_timer();

      fd = 0;
      ioctl(openair_fd,openair_STOP,&fd);
      munmap((void*)mem_base, BIGPHYS_NUMPAGES*4096);

      return 0;
    }
