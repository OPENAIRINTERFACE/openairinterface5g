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

/** usrp_lib.cpp
 *
 * \author: HongliangXU : hong-liang-xu@agilent.com
 */
#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <uhd/version.hpp>
#if UHD_VERSION < 3110000
  #include <uhd/utils/thread_priority.hpp>
#else
  #include <uhd/utils/thread.hpp>
#endif
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/version.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <complex>
#include <fstream>
#include <cmath>
#include <time.h>
#include "common/utils/LOG/log.h"
#include "common_lib.h"
#include "assertions.h"

#include "common/utils/LOG/vcd_signal_dumper.h"

#include <sys/resource.h>

#ifdef __SSE4_1__
  #include <smmintrin.h>
#endif

#ifdef __AVX2__
  #include <immintrin.h>
#endif

#ifdef __arm__
  #include <arm_neon.h>
#endif

/** @addtogroup _USRP_PHY_RF_INTERFACE_
 * @{
 */
int gpio789=0;
extern int usrp_tx_thread;


typedef struct {

  // --------------------------------
  // variables for USRP configuration
  // --------------------------------
  //! USRP device pointer
  uhd::usrp::multi_usrp::sptr usrp;

  //create a send streamer and a receive streamer
  //! USRP TX Stream
  uhd::tx_streamer::sptr tx_stream;
  //! USRP RX Stream
  uhd::rx_streamer::sptr rx_stream;

  //! USRP TX Metadata
  uhd::tx_metadata_t tx_md;
  //! USRP RX Metadata
  uhd::rx_metadata_t rx_md;

  //! Sampling rate
  double sample_rate;

  //! TX forward samples. We use usrp_time_offset to get this value
  int tx_forward_nsamps; //166 for 20Mhz

  // --------------------------------
  // Debug and output control
  // --------------------------------
  int num_underflows;
  int num_overflows;
  int num_seq_errors;
  int64_t tx_count;
  int64_t rx_count;
  int wait_for_first_pps;
  int use_gps;
  //int first_tx;
  //int first_rx;
  //! timestamp of RX packet
  openair0_timestamp rx_timestamp;
} usrp_state_t;

//void print_notes(void)
//{
// Helpful notes
//  std::cout << boost::format("**************************************Helpful Notes on Clock/PPS Selection**************************************\n");
//  std::cout << boost::format("As you can see, the default 10 MHz Reference and 1 PPS signals are now from the GPSDO.\n");
//  std::cout << boost::format("If you would like to use the internal reference(TCXO) in other applications, you must configure that explicitly.\n");
//  std::cout << boost::format("You can no longer select the external SMAs for 10 MHz or 1 PPS signaling.\n");
//  std::cout << boost::format("****************************************************************************************************************\n");
//}

int check_ref_locked(usrp_state_t *s,size_t mboard) {
  std::vector<std::string> sensor_names = s->usrp->get_mboard_sensor_names(mboard);
  bool ref_locked = false;

  if(std::find(sensor_names.begin(), sensor_names.end(), "ref_locked") != sensor_names.end()) {
    std::cout << "Waiting for reference lock..." << std::flush;

    for (int i = 0; i < 30 and not ref_locked; i++) {
      ref_locked = s->usrp->get_mboard_sensor("ref_locked", mboard).to_bool();

      if (not ref_locked) {
        std::cout << "." << std::flush;
        boost::this_thread::sleep(boost::posix_time::seconds(1));
      }
    }

    if(ref_locked) {
      std::cout << "LOCKED" << std::endl;
    } else {
      std::cout << "FAILED" << std::endl;
    }
  } else {
    std::cout << boost::format("ref_locked sensor not present on this board.\n");
  }

  return ref_locked;
}

static int sync_to_gps(openair0_device *device) {
  //uhd::set_thread_priority_safe();
  //std::string args;
  //Set up program options
  //po::options_description desc("Allowed options");
  //desc.add_options()
  //("help", "help message")
  //("args", po::value<std::string>(&args)->default_value(""), "USRP device arguments")
  //;
  //po::variables_map vm;
  //po::store(po::parse_command_line(argc, argv, desc), vm);
  //po::notify(vm);
  //Print the help message
  //if (vm.count("help"))
  //{
  //  std::cout << boost::format("Synchronize USRP to GPS %s") % desc << std::endl;
  // return EXIT_FAILURE;
  //}
  //Create a USRP device
  //std::cout << boost::format("\nCreating the USRP device with: %s...\n") % args;
  //uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
  //std::cout << boost::format("Using Device: %s\n") % usrp->get_pp_string();
  usrp_state_t *s = (usrp_state_t *)device->priv;

  try {
    size_t num_mboards = s->usrp->get_num_mboards();
    size_t num_gps_locked = 0;

    for (size_t mboard = 0; mboard < num_mboards; mboard++) {
      std::cout << "Synchronizing mboard " << mboard << ": " << s->usrp->get_mboard_name(mboard) << std::endl;
      bool ref_locked = check_ref_locked(s,mboard);

      if (ref_locked) {
        std::cout << boost::format("Ref Locked\n");
      } else {
        std::cout << "Failed to lock to GPSDO 10 MHz Reference. Exiting." << std::endl;
        exit(EXIT_FAILURE);
      }

      //Wait for GPS lock
      bool gps_locked = s->usrp->get_mboard_sensor("gps_locked", mboard).to_bool();

      if(gps_locked) {
        num_gps_locked++;
        std::cout << boost::format("GPS Locked\n");
      } else {
        LOG_W(HW,"WARNING:  GPS not locked - time will not be accurate until locked\n");
      }

      //Set to GPS time
      uhd::time_spec_t gps_time = uhd::time_spec_t(time_t(s->usrp->get_mboard_sensor("gps_time", mboard).to_int()));
      //s->usrp->set_time_next_pps(gps_time+1.0, mboard);
      s->usrp->set_time_next_pps(uhd::time_spec_t(0.0));
      //Wait for it to apply
      //The wait is 2 seconds because N-Series has a known issue where
      //the time at the last PPS does not properly update at the PPS edge
      //when the time is actually set.
      boost::this_thread::sleep(boost::posix_time::seconds(2));
      //Check times
      gps_time = uhd::time_spec_t(time_t(s->usrp->get_mboard_sensor("gps_time", mboard).to_int()));
      uhd::time_spec_t time_last_pps = s->usrp->get_time_last_pps(mboard);
      std::cout << "USRP time: " << (boost::format("%0.9f") % time_last_pps.get_real_secs()) << std::endl;
      std::cout << "GPSDO time: " << (boost::format("%0.9f") % gps_time.get_real_secs()) << std::endl;
      //if (gps_time.get_real_secs() == time_last_pps.get_real_secs())
      //    std::cout << std::endl << "SUCCESS: USRP time synchronized to GPS time" << std::endl << std::endl;
      //else
      //    std::cerr << std::endl << "ERROR: Failed to synchronize USRP time to GPS time" << std::endl << std::endl;
    }

    if (num_gps_locked == num_mboards and num_mboards > 1) {
      //Check to see if all USRP times are aligned
      //First, wait for PPS.
      uhd::time_spec_t time_last_pps = s->usrp->get_time_last_pps();

      while (time_last_pps == s->usrp->get_time_last_pps()) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1));
      }

      //Sleep a little to make sure all devices have seen a PPS edge
      boost::this_thread::sleep(boost::posix_time::milliseconds(200));
      //Compare times across all mboards
      bool all_matched = true;
      uhd::time_spec_t mboard0_time = s->usrp->get_time_last_pps(0);

      for (size_t mboard = 1; mboard < num_mboards; mboard++) {
        uhd::time_spec_t mboard_time = s->usrp->get_time_last_pps(mboard);

        if (mboard_time != mboard0_time) {
          all_matched = false;
          std::cerr << (boost::format("ERROR: Times are not aligned: USRP 0=%0.9f, USRP %d=%0.9f")
                        % mboard0_time.get_real_secs()
                        % mboard
                        % mboard_time.get_real_secs()) << std::endl;
        }
      }

      if (all_matched) {
        std::cout << "SUCCESS: USRP times aligned" << std::endl << std::endl;
      } else {
        std::cout << "ERROR: USRP times are not aligned" << std::endl << std::endl;
      }
    }
  } catch (std::exception &e) {
    std::cout << boost::format("\nError: %s") % e.what();
    std::cout << boost::format("This could mean that you have not installed the GPSDO correctly.\n\n");
    std::cout << boost::format("Visit one of these pages if the problem persists:\n");
    std::cout << boost::format(" * N2X0/E1X0: http://files.ettus.com/manual/page_gpsdo.html");
    std::cout << boost::format(" * X3X0: http://files.ettus.com/manual/page_gpsdo_x3x0.html\n\n");
    std::cout << boost::format(" * E3X0: http://files.ettus.com/manual/page_usrp_e3x0.html#e3x0_hw_gps\n\n");
    exit(EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}

/*! \brief Called to start the USRP transceiver. Return 0 if OK, < 0 if error
    @param device pointer to the device structure specific to the RF hardware target
*/
static int trx_usrp_start(openair0_device *device) {
  usrp_state_t *s = (usrp_state_t *)device->priv;

  // setup GPIO for TDD, GPIO(4) = ATR_RX
  //set data direction register (DDR) to output
  s->usrp->set_gpio_attr("FP0", "DDR", 0xfff, 0xfff);
  //set lower 7 bits to be controlled automatically by ATR (the rest 5 bits are controlled manually)
  s->usrp->set_gpio_attr("FP0", "CTRL", 0x7f,0xfff);
  //set pins 4 (RX_TX_Switch) and 6 (Shutdown PA) to 1 when the radio is only receiving (ATR_RX)
  s->usrp->set_gpio_attr("FP0", "ATR_RX", (1<<4)|(1<<6), 0x7f);
  // set pin 5 (Shutdown LNA) to 1 when the radio is transmitting and receiveing (ATR_XX)
  // (we use full duplex here, because our RX is on all the time - this might need to change later)
  s->usrp->set_gpio_attr("FP0", "ATR_XX", (1<<5), 0x7f);
  // set the output pins to 1
  s->usrp->set_gpio_attr("FP0", "OUT", 7<<7, 0xf80);

  s->wait_for_first_pps = 1;
  s->rx_count = 0;
  s->tx_count = 0;
  //s->first_tx = 1;
  //s->first_rx = 1;
  s->rx_timestamp = 0;

  s->usrp->set_time_next_pps(uhd::time_spec_t(0.0));
  // wait for the pps to change
  uhd::time_spec_t time_last_pps = s->usrp->get_time_last_pps();
  while (time_last_pps == s->usrp->get_time_last_pps()) {
    boost::this_thread::sleep(boost::posix_time::milliseconds(1));
  }

  uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
  cmd.time_spec = uhd::time_spec_t(1.0);
  cmd.stream_now = false; // start at constant delay
  s->rx_stream->issue_stream_cmd(cmd);

  return 0;
}
/*! \brief Terminate operation of the USRP transceiver -- free all associated resources
 * \param device the hardware to use
 */
static void trx_usrp_end(openair0_device *device) {
  if (device == NULL)
    return;

  usrp_state_t *s = (usrp_state_t *)device->priv;

  if (s == NULL)
    return;
  iqrecorder_end(device);


}

/*! \brief Called to send samples to the USRP RF target
      @param device pointer to the device structure specific to the RF hardware target
      @param timestamp The timestamp at which the first sample MUST be sent
      @param buff Buffer which holds the samples
      @param nsamps number of samples to be sent
      @param antenna_id index of the antenna if the device has multiple antennas
      @param flags flags must be set to TRUE if timestamp parameter needs to be applied
*/
static int trx_usrp_write(openair0_device *device,
			  openair0_timestamp timestamp,
			  void **buff,
			  int nsamps,
			  int cc,
			  int flags) {
  int ret=0;
  usrp_state_t *s = (usrp_state_t *)device->priv;
  int nsamps2;  // aligned to upper 32 or 16 byte boundary

  int flags_lsb = flags&0xff;
  int flags_msb = (flags>>8)&0xff;

  int end;
  openair0_thread_t *write_thread = &device->write_thread;
  openair0_write_package_t *write_package = write_thread->write_package;

  AssertFatal( MAX_WRITE_THREAD_BUFFER_SIZE >= cc,"Do not support more than %d cc number\n", MAX_WRITE_THREAD_BUFFER_SIZE);

    boolean_t first_packet_state=false,last_packet_state=false;

    if (flags_lsb == 2) { // start of burst
      //      s->tx_md.start_of_burst = true;
      //      s->tx_md.end_of_burst = false;
      first_packet_state = true;
      last_packet_state  = false;
    } else if (flags_lsb == 3) { // end of burst
      //s->tx_md.start_of_burst = false;
      //s->tx_md.end_of_burst = true;
      first_packet_state = false;
      last_packet_state  = true;
    } else if (flags_lsb == 4) { // start and end
    //  s->tx_md.start_of_burst = true;
    //  s->tx_md.end_of_burst = true;
      first_packet_state = true;
      last_packet_state  = true;
    } else if (flags_lsb==1) { // middle of burst
    //  s->tx_md.start_of_burst = false;
    //  s->tx_md.end_of_burst = false;
      first_packet_state = false;
      last_packet_state  = false;
    }
    else if (flags_lsb==10) { // fail safe mode
     // s->tx_md.has_time_spec = false;
     // s->tx_md.start_of_burst = false;
     // s->tx_md.end_of_burst = true;
     first_packet_state = false;
     last_packet_state  = true;
    }

  if(usrp_tx_thread == 0){
#if defined(__x86_64) || defined(__i386__)
  #ifdef __AVX2__
      nsamps2 = (nsamps+7)>>3;
      __m256i buff_tx[cc<2?2:cc][nsamps2];
  #else
    nsamps2 = (nsamps+3)>>2;
    __m128i buff_tx[cc<2?2:cc][nsamps2];
  #endif
#elif defined(__arm__)
    nsamps2 = (nsamps+3)>>2;
    int16x8_t buff_tx[cc<2?2:cc][nsamps2];
#else
#error Unsupported CPU architecture, USRP device cannot be built
#endif

    // bring RX data into 12 LSBs for softmodem RX
    for (int i=0; i<cc; i++) {
      for (int j=0; j<nsamps2; j++) {
#if defined(__x86_64__) || defined(__i386__)
#ifdef __AVX2__
        buff_tx[i][j] = _mm256_slli_epi16(((__m256i *)buff[i])[j],4);
#else
        buff_tx[i][j] = _mm_slli_epi16(((__m128i *)buff[i])[j],4);
#endif
#elif defined(__arm__)
        buff_tx[i][j] = vshlq_n_s16(((int16x8_t *)buff[i])[j],4);
#endif
      }
    }

    s->tx_md.has_time_spec  = true;
    s->tx_md.start_of_burst = (s->tx_count==0) ? true : first_packet_state;
    s->tx_md.end_of_burst   = last_packet_state;
    s->tx_md.time_spec      = uhd::time_spec_t::from_ticks(timestamp, s->sample_rate);
    s->tx_count++;

VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_BEAM_SWITCHING_GPIO,1);
    // bit 3 enables gpio (for backward compatibility)
    if (flags_msb&8) {
      // push GPIO bits 7-9 from flags_msb
      int gpio789=(flags_msb&7)<<7;
      s->usrp->set_command_time(s->tx_md.time_spec);
      s->usrp->set_gpio_attr("FP0", "OUT", gpio789, 0x380);
      s->usrp->clear_command_time();
    }
VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_BEAM_SWITCHING_GPIO,0);

    if (cc>1) {
      std::vector<void *> buff_ptrs;

      for (int i=0; i<cc; i++)
        buff_ptrs.push_back(&(((int16_t *)buff_tx[i])[0]));

      ret = (int)s->tx_stream->send(buff_ptrs, nsamps, s->tx_md);
    }
    else {
      ret = (int)s->tx_stream->send(&(((int16_t *)buff_tx[0])[0]), nsamps, s->tx_md);
    }

    if (ret != nsamps) LOG_E(HW,"[xmit] tx samples %d != %d\n",ret,nsamps);
    return ret;
  }
  else{
    pthread_mutex_lock(&write_thread->mutex_write);

    if(write_thread->count_write >= MAX_WRITE_THREAD_PACKAGE){
      LOG_W(HW,"Buffer overflow, count_write = %d, start = %d end = %d, resetting write package\n", write_thread->count_write, write_thread->start, write_thread->end);
      write_thread->end = write_thread->start;
      write_thread->count_write = 0;
    }

    end = write_thread->end;
    write_package[end].timestamp    = timestamp;
    write_package[end].nsamps       = nsamps;
    write_package[end].cc           = cc;
    write_package[end].first_packet = first_packet_state;
    write_package[end].last_packet  = last_packet_state;
    write_package[end].flags_msb    = flags_msb;
    for (int i = 0; i < cc; i++)
      write_package[end].buff[i]    = buff[i];
    write_thread->count_write++;
    write_thread->end = (write_thread->end + 1)% MAX_WRITE_THREAD_PACKAGE;
    LOG_D(HW,"Signaling TX TS %llu\n",(unsigned long long)timestamp);
    pthread_cond_signal(&write_thread->cond_write);
    pthread_mutex_unlock(&write_thread->mutex_write);
    return 0;
  }

}

//-----------------------start--------------------------
/*! \brief Called to send samples to the USRP RF target
      @param device pointer to the device structure specific to the RF hardware target
      @param timestamp The timestamp at which the first sample MUST be sent
      @param buff Buffer which holds the samples
      @param nsamps number of samples to be sent
      @param antenna_id index of the antenna if the device has multiple antennas
      @param flags flags must be set to TRUE if timestamp parameter needs to be applied
*/
void *trx_usrp_write_thread(void * arg){
  int ret=0;
  openair0_device *device=(openair0_device *)arg;
  openair0_thread_t *write_thread = &device->write_thread;
  openair0_write_package_t *write_package = write_thread->write_package;

  usrp_state_t *s;
  int nsamps2;  // aligned to upper 32 or 16 byte boundary
  int start;
  openair0_timestamp timestamp;
  void               **buff;
  int                nsamps;
  int                cc;
  signed char        first_packet;
  signed char        last_packet;
  int                flags_msb;

  while(1){
    pthread_mutex_lock(&write_thread->mutex_write);
    while (write_thread->count_write == 0) {
      pthread_cond_wait(&write_thread->cond_write,&write_thread->mutex_write); // this unlocks mutex_rxtx while waiting and then locks it again
    }
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE_THREAD, 1 );
    s = (usrp_state_t *)device->priv;
    start = write_thread->start;
    timestamp    = write_package[start].timestamp;
    buff         = write_package[start].buff;
    nsamps       = write_package[start].nsamps;
    cc           = write_package[start].cc;
    first_packet = write_package[start].first_packet;
    last_packet  = write_package[start].last_packet;
    flags_msb    = write_package[start].flags_msb;
    write_thread->start = (write_thread->start + 1)% MAX_WRITE_THREAD_PACKAGE;
    write_thread->count_write--;
    pthread_mutex_unlock(&write_thread->mutex_write);
    /*if(write_thread->count_write != 0){
      LOG_W(HW,"count write = %d, start = %d, end = %d\n", write_thread->count_write, write_thread->start, write_thread->end);
    }*/

    #if defined(__x86_64) || defined(__i386__)
      #ifdef __AVX2__
        nsamps2 = (nsamps+7)>>3;
        __m256i buff_tx[cc<2?2:cc][nsamps2];
      #else
        nsamps2 = (nsamps+3)>>2;
        __m128i buff_tx[cc<2?2:cc][nsamps2];
      #endif
    #elif defined(__arm__)
      nsamps2 = (nsamps+3)>>2;
      int16x8_t buff_tx[cc<2?2:cc][nsamps2];
    #else
    #error Unsupported CPU architecture, USRP device cannot be built
    #endif

    // bring RX data into 12 LSBs for softmodem RX
    for (int i=0; i<cc; i++) {
      for (int j=0; j<nsamps2; j++) {
        #if defined(__x86_64__) || defined(__i386__)
          #ifdef __AVX2__
            buff_tx[i][j] = _mm256_slli_epi16(((__m256i *)buff[i])[j],4);
          #else
            buff_tx[i][j] = _mm_slli_epi16(((__m128i *)buff[i])[j],4);
          #endif
        #elif defined(__arm__)
          buff_tx[i][j] = vshlq_n_s16(((int16x8_t *)buff[i])[j],4);
        #endif
      }
    }


    s->tx_md.has_time_spec  = true;
    s->tx_md.start_of_burst = (s->tx_count==0) ? true : first_packet;
    s->tx_md.end_of_burst   = last_packet;
    s->tx_md.time_spec      = uhd::time_spec_t::from_ticks(timestamp, s->sample_rate);
    s->tx_count++;

    // bit 3 enables gpio (for backward compatibility)
    if (flags_msb&8) {
      // push GPIO bits 7-9 from flags_msb
      int gpio789=(flags_msb&7)<<7;
      s->usrp->set_command_time(s->tx_md.time_spec);
      s->usrp->set_gpio_attr("FP0", "OUT", gpio789, 0x380);
      s->usrp->clear_command_time();
    }

    if (cc>1) {
      std::vector<void *> buff_ptrs;

      for (int i=0; i<cc; i++)
        buff_ptrs.push_back(&(((int16_t *)buff_tx[i])[0]));

      ret = (int)s->tx_stream->send(buff_ptrs, nsamps, s->tx_md);
    }
    else {
      ret = (int)s->tx_stream->send(&(((int16_t *)buff_tx[0])[0]), nsamps, s->tx_md);
    }

    if (ret != nsamps) LOG_E(HW,"[xmit] tx samples %d != %d\n",ret,nsamps);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_USRP_SEND_RETURN, ret );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE_THREAD, 0 );

  }

  return NULL;
}

int trx_usrp_write_init(openair0_device *device){

  //uhd::set_thread_priority_safe(1.0);
  openair0_thread_t *write_thread = &device->write_thread;
  printf("initializing tx write thread\n");

  write_thread->start              = 0;
  write_thread->end                = 0;
  write_thread->count_write        = 0;
  printf("end of tx write thread\n");
  pthread_mutex_init(&write_thread->mutex_write, NULL);
  pthread_cond_init(&write_thread->cond_write, NULL);
  pthread_create(&write_thread->pthread_write,NULL,trx_usrp_write_thread,(void *)device);

  return(0);
}

//---------------------end-------------------------

/*! \brief Receive samples from hardware.
 * Read \ref nsamps samples from each channel to buffers. buff[0] is the array for
 * the first channel. *ptimestamp is the time at which the first sample
 * was received.
 * \param device the hardware to use
 * \param[out] ptimestamp the time at which the first sample was received.
 * \param[out] buff An array of pointers to buffers for received samples. The buffers must be large enough to hold the number of samples \ref nsamps.
 * \param nsamps Number of samples. One sample is 2 byte I + 2 byte Q => 4 byte.
 * \param antenna_id Index of antenna for which to receive samples
 * \returns the number of sample read
*/
static int trx_usrp_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int cc) {
  usrp_state_t *s = (usrp_state_t *)device->priv;
  int samples_received=0;
  int nsamps2;  // aligned to upper 32 or 16 byte boundary
#if defined(__x86_64) || defined(__i386__)
#ifdef __AVX2__
  nsamps2 = (nsamps+7)>>3;
  __m256i buff_tmp[cc<2 ? 2 : cc][nsamps2];
#else
  nsamps2 = (nsamps+3)>>2;
  __m128i buff_tmp[cc<2 ? 2 : cc][nsamps2];
#endif
#elif defined(__arm__)
  nsamps2 = (nsamps+3)>>2;
  int16x8_t buff_tmp[cc<2 ? 2 : cc][nsamps2];
#endif

  int rxshift;
  switch (device->type) {
     case USRP_B200_DEV:
        rxshift=4;
        break;
     case USRP_X300_DEV:
     case USRP_N300_DEV:
        rxshift=2;
        break;
     default:
       AssertFatal(1==0,"Shouldn't be here\n");
  }

    samples_received=0;
    while (samples_received != nsamps) {

      if (cc>1) {
      // receive multiple channels (e.g. RF A and RF B)
        std::vector<void *> buff_ptrs;

        for (int i=0; i<cc; i++) buff_ptrs.push_back(buff_tmp[i]+samples_received);

        samples_received += s->rx_stream->recv(buff_ptrs, nsamps, s->rx_md);
      } else {
      // receive a single channel (e.g. from connector RF A)

        samples_received += s->rx_stream->recv((void*)((int32_t*)buff_tmp[0]+samples_received),
                                               nsamps-samples_received, s->rx_md);
      }
      if  ((s->wait_for_first_pps == 0) && (s->rx_md.error_code!=uhd::rx_metadata_t::ERROR_CODE_NONE))
        break;

      if ((s->wait_for_first_pps == 1) && (samples_received != nsamps)) {
        printf("sleep...\n"); //usleep(100);
      }
    }
    if (samples_received == nsamps) s->wait_for_first_pps=0;

    // bring RX data into 12 LSBs for softmodem RX
    for (int i=0; i<cc; i++) {
      for (int j=0; j<nsamps2; j++) {
#if defined(__x86_64__) || defined(__i386__)
#ifdef __AVX2__
        // FK: in some cases the buffer might not be 32 byte aligned, so we cannot use avx2

        if ((((uintptr_t) buff[i])&0x1F)==0) {
          ((__m256i *)buff[i])[j] = _mm256_srai_epi16(buff_tmp[i][j],rxshift);
        } else {
          ((__m128i *)buff[i])[2*j] = _mm_srai_epi16(((__m128i *)buff_tmp[i])[2*j],rxshift);
          ((__m128i *)buff[i])[2*j+1] = _mm_srai_epi16(((__m128i *)buff_tmp[i])[2*j+1],rxshift);
        }

#else
        ((__m128i *)buff[i])[j] = _mm_srai_epi16(buff_tmp[i][j],rxshift);
#endif
#elif defined(__arm__)
        ((int16x8_t *)buff[i])[j] = vshrq_n_s16(buff_tmp[i][j],rxshift);
#endif
      }
    }

    if (samples_received < nsamps) {
      LOG_E(HW,"[recv] received %d samples out of %d\n",samples_received,nsamps);
    }

  if ( s->rx_md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE)
    LOG_E(HW, "%s\n", s->rx_md.to_pp_string(true).c_str());

  s->rx_count += nsamps;
  s->rx_timestamp = s->rx_md.time_spec.to_ticks(s->sample_rate);
  *ptimestamp = s->rx_timestamp;

  // push GPIO bits 7-9 from flags_msb
   /*s->usrp->set_command_time(uhd::time_spec_t::from_ticks((s->rx_timestamp+(2*nsamps)),s->sample_rate));
   s->usrp->set_gpio_attr("FP0", "OUT", gpio789<<7, 0x380);
   s->usrp->clear_command_time();
   gpio789 = (gpio789+1)&7;*/
  recplay_state_t *recPlay=device->recplay_state;

  if ( recPlay != NULL) { // record mode
    // Copy subframes to memory (later dump on a file)
    if (recPlay->nbSamplesBlocks < device->openair0_cfg->recplay_conf->u_sf_max &&
        recPlay->maxSizeBytes > (recPlay->currentPtr-(uint8_t *)recPlay->ms_sample) +
        sizeof(iqrec_t) + nsamps*4 ) {
      iqrec_t *hdr=(iqrec_t *)recPlay->currentPtr;
      hdr->header = BELL_LABS_IQ_HEADER;
      hdr->ts = *ptimestamp;
      hdr->nbBytes=nsamps*4;
      memcpy(hdr+1, buff[0], nsamps*4);
      recPlay->currentPtr+=sizeof(iqrec_t)+nsamps*4;
      recPlay->nbSamplesBlocks++;
      LOG_D(HW,"recorded %d samples, for TS %lu, shift in buffer %ld\n", nsamps, hdr->ts, recPlay->currentPtr-(uint8_t *)recPlay->ms_sample);
    } else
      exit_function(__FILE__, __FUNCTION__, __LINE__,"Recording reaches max iq limit\n");
  }

  return samples_received;
}

/*! \brief Compares two variables within precision
 * \param a first variable
 * \param b second variable
*/
static bool is_equal(double a, double b) {
  return std::fabs(a-b) < std::numeric_limits<double>::epsilon();
}

void *freq_thread(void *arg) {
  openair0_device *device=(openair0_device *)arg;
  usrp_state_t *s = (usrp_state_t *)device->priv;
  s->usrp->set_tx_freq(device->openair0_cfg[0].tx_freq[0]);
  s->usrp->set_rx_freq(device->openair0_cfg[0].rx_freq[0]);
  return NULL;
}
/*! \brief Set frequencies (TX/RX). Spawns a thread to handle the frequency change to not block the calling thread
 * \param device the hardware to use
 * \param openair0_cfg RF frontend parameters set by application
 * \param dummy dummy variable not used
 * \returns 0 in success
 */
int trx_usrp_set_freq(openair0_device *device, openair0_config_t *openair0_cfg, int dont_block) {
  usrp_state_t *s = (usrp_state_t *)device->priv;
  pthread_t f_thread;
  printf("Setting USRP TX Freq %f, RX Freq %f\n",openair0_cfg[0].tx_freq[0],openair0_cfg[0].rx_freq[0]);

  // spawn a thread to handle the frequency change to not block the calling thread
  if (dont_block == 1)
    pthread_create(&f_thread,NULL,freq_thread,(void *)device);
  else {
    s->usrp->set_tx_freq(device->openair0_cfg[0].tx_freq[0]);
    s->usrp->set_rx_freq(device->openair0_cfg[0].rx_freq[0]);
  }

  return(0);
}

/*! \brief Set RX frequencies
 * \param device the hardware to use
 * \param openair0_cfg RF frontend parameters set by application
 * \returns 0 in success
 */
int openair0_set_rx_frequencies(openair0_device *device, openair0_config_t *openair0_cfg) {
  usrp_state_t *s = (usrp_state_t *)device->priv;
  uhd::tune_request_t rx_tune_req(openair0_cfg[0].rx_freq[0]);
  rx_tune_req.rf_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
  rx_tune_req.rf_freq = openair0_cfg[0].rx_freq[0];
  s->usrp->set_rx_freq(rx_tune_req);
  return(0);
}

/*! \brief Set Gains (TX/RX)
 * \param device the hardware to use
 * \param openair0_cfg RF frontend parameters set by application
 * \returns 0 in success
 */
int trx_usrp_set_gains(openair0_device *device,
                       openair0_config_t *openair0_cfg) {
  usrp_state_t *s = (usrp_state_t *)device->priv;
  ::uhd::gain_range_t gain_range_tx = s->usrp->get_tx_gain_range(0);
  s->usrp->set_tx_gain(gain_range_tx.stop()-openair0_cfg[0].tx_gain[0]);
  ::uhd::gain_range_t gain_range = s->usrp->get_rx_gain_range(0);

  // limit to maximum gain
  if (openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0] > gain_range.stop()) {
    LOG_E(HW,"RX Gain 0 too high, reduce by %f dB\n",
          openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0] - gain_range.stop());
    exit(-1);
  }

  s->usrp->set_rx_gain(openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0]);
  LOG_I(HW,"Setting USRP RX gain to %f (rx_gain %f,gain_range.stop() %f)\n",
        openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0],
        openair0_cfg[0].rx_gain[0],gain_range.stop());
  return(0);
}

/*! \brief Stop USRP
 * \param card refers to the hardware index to use
 */
int trx_usrp_stop(openair0_device *device) {
  return(0);
}

/*! \brief USRPB210 RX calibration table */
rx_gain_calib_table_t calib_table_b210[] = {
  {3500000000.0,44.0},
  {2660000000.0,49.0},
  {2300000000.0,50.0},
  {1880000000.0,53.0},
  {816000000.0,58.0},
  {-1,0}
};

/*! \brief USRPB210 RX calibration table */
rx_gain_calib_table_t calib_table_b210_38[] = {
  {3500000000.0,44.0},
  {2660000000.0,49.8},
  {2300000000.0,51.0},
  {1880000000.0,53.0},
  {816000000.0,57.0},
  {-1,0}
};

/*! \brief USRPx310 RX calibration table */
rx_gain_calib_table_t calib_table_x310[] = {
  {3500000000.0,77.0},
  {2660000000.0,81.0},
  {2300000000.0,81.0},
  {1880000000.0,82.0},
  {816000000.0,85.0},
  {-1,0}
};

/*! \brief USRPB210 RX calibration table */
rx_gain_calib_table_t calib_table_n310[] = {
  {3500000000.0,0.0},
  {2660000000.0,0.0},
  {2300000000.0,0.0},
  {1880000000.0,0.0},
  {816000000.0, 0.0},
  {-1,0}
};


/*! \brief Set RX gain offset
 * \param openair0_cfg RF frontend parameters set by application
 * \param chain_index RF chain to apply settings to
 * \returns 0 in success
 */
void set_rx_gain_offset(openair0_config_t *openair0_cfg, int chain_index,int bw_gain_adjust) {
  int i=0;
  // loop through calibration table to find best adjustment factor for RX frequency
  double min_diff = 6e9,diff,gain_adj=0.0;

  if (bw_gain_adjust==1) {
    switch ((int)openair0_cfg[0].sample_rate) {
      case 46080000:
        break;

      case 30720000:
        break;

      case 23040000:
        gain_adj=1.25;
        break;

      case 15360000:
        gain_adj=3.0;
        break;

      case 7680000:
        gain_adj=6.0;
        break;

      case 3840000:
        gain_adj=9.0;
        break;

      case 1920000:
        gain_adj=12.0;
        break;

      default:
        LOG_E(HW,"unknown sampling rate %d\n",(int)openair0_cfg[0].sample_rate);
        //exit(-1);
        break;
    }
  }

  while (openair0_cfg->rx_gain_calib_table[i].freq>0) {
    diff = fabs(openair0_cfg->rx_freq[chain_index] - openair0_cfg->rx_gain_calib_table[i].freq);
    LOG_I(HW,"cal %d: freq %f, offset %f, diff %f\n",
          i,
          openair0_cfg->rx_gain_calib_table[i].freq,
          openair0_cfg->rx_gain_calib_table[i].offset,diff);

    if (min_diff > diff) {
      min_diff = diff;
      openair0_cfg->rx_gain_offset[chain_index] = openair0_cfg->rx_gain_calib_table[i].offset+gain_adj;
    }

    i++;
  }
}

/*! \brief print the USRP statistics
* \param device the hardware to use
* \returns  0 on success
*/
int trx_usrp_get_stats(openair0_device *device) {
  return(0);
}

/*! \brief Reset the USRP statistics
 * \param device the hardware to use
 * \returns  0 on success
 */
int trx_usrp_reset_stats(openair0_device *device) {
  return(0);
}

extern "C" {
  int device_init(openair0_device *device, openair0_config_t *openair0_cfg) {
    LOG_I(HW, "openair0_cfg[0].sdr_addrs == '%s'\n", openair0_cfg[0].sdr_addrs);
    LOG_I(HW, "openair0_cfg[0].clock_source == '%d' (internal = %d, external = %d)\n", openair0_cfg[0].clock_source,internal,external);
    usrp_state_t *s ;
    int choffset = 1;

    if ( device->priv == NULL) {
      s=(usrp_state_t *)calloc(sizeof(usrp_state_t),1);
      device->priv=s;
      AssertFatal( s!=NULL,"USRP device: memory allocation failure\n");
    } else {
      LOG_E(HW, "multiple device init detected\n");
      return 0;
    }

    device->openair0_cfg = openair0_cfg;
    device->trx_start_func = trx_usrp_start;
    device->trx_get_stats_func = trx_usrp_get_stats;
    device->trx_reset_stats_func = trx_usrp_reset_stats;
    device->trx_end_func   = trx_usrp_end;
    device->trx_stop_func  = trx_usrp_stop;
    device->trx_set_freq_func = trx_usrp_set_freq;
    device->trx_set_gains_func   = trx_usrp_set_gains;
    device->trx_write_init = trx_usrp_write_init;


    // hotfix! to be checked later
    //uhd::set_thread_priority_safe(1.0);
    // Initialize USRP device
    int vers=0,subvers=0,subsubvers=0;
    int bw_gain_adjust=0;

    if (device->openair0_cfg->recplay_mode == RECPLAY_RECORDMODE) {
      std::cerr << "USRP device initialized in subframes record mode" << std::endl;
    }

    sscanf(uhd::get_version_string().c_str(),"%d.%d.%d",&vers,&subvers,&subsubvers);
    LOG_I(HW,"UHD version %s (%d.%d.%d)\n",
          uhd::get_version_string().c_str(),vers,subvers,subsubvers);
    std::string args;

    if (openair0_cfg[0].sdr_addrs == NULL) {
      args = "type=b200";
    } else {
      args = openair0_cfg[0].sdr_addrs;
      LOG_I(HW,"Checking for USRP with args %s\n",openair0_cfg[0].sdr_addrs);
    }

    uhd::device_addrs_t device_adds = uhd::device::find(args);

    if (device_adds.size() == 0) {
      LOG_E(HW,"No USRP Device Found.\n ");
      free(s);
      return -1;
    } else if (device_adds.size() > 1) {
      LOG_E(HW,"More than one USRP Device Found. Please specify device more precisely in config file.\n");
      free(s);
      return -1;
    }

    LOG_I(HW,"Found USRP %s\n", device_adds[0].get("type").c_str());
    double usrp_master_clock;

    if (device_adds[0].get("type") == "b200") {
      device->type = USRP_B200_DEV;
      usrp_master_clock = 30.72e6;
      args += boost::str(boost::format(",master_clock_rate=%f") % usrp_master_clock);
      args += ",num_send_frames=256,num_recv_frames=256, send_frame_size=7680, recv_frame_size=7680" ;
    }

    if (device_adds[0].get("type") == "n3xx") {
      printf("Found USRP n300\n");
      device->type=USRP_N300_DEV;
      usrp_master_clock = 122.88e6;
      args += boost::str(boost::format(",master_clock_rate=%f") % usrp_master_clock);
      //args += ", send_buff_size=33554432";
    }

    if (device_adds[0].get("type") == "x300") {
      printf("Found USRP x300\n");
      device->type=USRP_X300_DEV;
      usrp_master_clock = 184.32e6;
      args += boost::str(boost::format(",master_clock_rate=%f") % usrp_master_clock);

      // USRP recommended: https://files.ettus.com/manual/page_usrp_x3x0_config.html
      if ( 0 != system("sysctl -w net.core.rmem_max=33554432 net.core.wmem_max=33554432") )
        LOG_W(HW,"Can't set kernel parameters for X3xx\n");
    }

    s->usrp = uhd::usrp::multi_usrp::make(args);

    if (args.find("clock_source")==std::string::npos) {
	if (openair0_cfg[0].clock_source == internal) {
	  s->usrp->set_clock_source("internal");
	  LOG_I(HW,"Setting clock source to internal\n");
	}
	else if (openair0_cfg[0].clock_source == external ) {
	  s->usrp->set_clock_source("external");
	  LOG_I(HW,"Setting clock source to external\n");
	}
	else if (openair0_cfg[0].clock_source==gpsdo) {
	  s->usrp->set_clock_source("gpsdo");
	  LOG_I(HW,"Setting clock source to gpsdo\n");
	}
	else {
	  LOG_W(HW,"Clock source set neither in usrp_args nor on command line, using default!\n");
	}
    }
    else {
	if (openair0_cfg[0].clock_source != unset) {
	  LOG_W(HW,"Clock source set in both usrp_args and in clock_source, ingnoring the latter!\n");
	}
  }

    if (args.find("time_source")==std::string::npos) {
	if (openair0_cfg[0].time_source == internal) {
	  s->usrp->set_time_source("internal");
	  LOG_I(HW,"Setting time source to internal\n");
	}
	else if (openair0_cfg[0].time_source == external ) {
	  s->usrp->set_time_source("external");
	  LOG_I(HW,"Setting time source to external\n");
	}
	else if (openair0_cfg[0].time_source==gpsdo) {
	  s->usrp->set_time_source("gpsdo");
	  LOG_I(HW,"Setting time source to gpsdo\n");
	}
	else {
	  LOG_W(HW,"Time source set neither in usrp_args nor on command line, using default!\n");
	}
    }
    else {
	if (openair0_cfg[0].clock_source != unset) {
	  LOG_W(HW,"Time source set in both usrp_args and in time_source, ingnoring the latter!\n");
	}
  }


  if (s->usrp->get_clock_source(0) == "gpsdo") {
    s->use_gps = 1;

    if (sync_to_gps(device)==EXIT_SUCCESS) {
      LOG_I(HW,"USRP synced with GPS!\n");
    } else {
      LOG_I(HW,"USRP fails to sync with GPS. Exiting.\n");
      exit(EXIT_FAILURE);
    }
  } else if (s->usrp->get_clock_source(0) == "external") {
    if (check_ref_locked(s,0)) {
      LOG_I(HW,"USRP locked to external reference!\n");
    } else {
      LOG_I(HW,"Failed to lock to external reference. Exiting.\n");
      exit(EXIT_FAILURE);
    }
  }

  if (device->type==USRP_X300_DEV) {
    openair0_cfg[0].rx_gain_calib_table = calib_table_x310;
    std::cerr << "-- Using calibration table: calib_table_x310" << std::endl;
  }

  if (device->type==USRP_N300_DEV) {
    openair0_cfg[0].rx_gain_calib_table = calib_table_n310;
    std::cerr << "-- Using calibration table: calib_table_n310" << std::endl;
  }


  if (device->type==USRP_N300_DEV || device->type==USRP_X300_DEV) {
    LOG_I(HW,"%s() sample_rate:%u\n", __FUNCTION__, (int)openair0_cfg[0].sample_rate);

    switch ((int)openair0_cfg[0].sample_rate) {
      case 122880000:
        // from usrp_time_offset
        //openair0_cfg[0].samples_per_packet    = 2048;
        openair0_cfg[0].tx_sample_advance     = 15; //to be checked
        openair0_cfg[0].tx_bw                 = 80e6;
        openair0_cfg[0].rx_bw                 = 80e6;
        break;

      case 92160000:
        // from usrp_time_offset
        //openair0_cfg[0].samples_per_packet    = 2048;
        openair0_cfg[0].tx_sample_advance     = 15; //to be checked
        openair0_cfg[0].tx_bw                 = 80e6;
        openair0_cfg[0].rx_bw                 = 80e6;
        break;

      case 61440000:
        // from usrp_time_offset
        //openair0_cfg[0].samples_per_packet    = 2048;
        openair0_cfg[0].tx_sample_advance     = 15;
        openair0_cfg[0].tx_bw                 = 40e6;
        openair0_cfg[0].rx_bw                 = 40e6;
        break;

      case 46080000:
        //openair0_cfg[0].samples_per_packet    = 2048;
        openair0_cfg[0].tx_sample_advance     = 15;
        openair0_cfg[0].tx_bw                 = 40e6;
        openair0_cfg[0].rx_bw                 = 40e6;
        break;

      case 30720000:
        // from usrp_time_offset
        //openair0_cfg[0].samples_per_packet    = 2048;
        openair0_cfg[0].tx_sample_advance     = 15;
        openair0_cfg[0].tx_bw                 = 20e6;
        openair0_cfg[0].rx_bw                 = 20e6;
        break;

      case 15360000:
        //openair0_cfg[0].samples_per_packet    = 2048;
        openair0_cfg[0].tx_sample_advance     = 45;
        openair0_cfg[0].tx_bw                 = 10e6;
        openair0_cfg[0].rx_bw                 = 10e6;
        break;

      case 7680000:
        //openair0_cfg[0].samples_per_packet    = 2048;
        openair0_cfg[0].tx_sample_advance     = 50;
        openair0_cfg[0].tx_bw                 = 5e6;
        openair0_cfg[0].rx_bw                 = 5e6;
        break;

      case 1920000:
        //openair0_cfg[0].samples_per_packet    = 2048;
        openair0_cfg[0].tx_sample_advance     = 50;
        openair0_cfg[0].tx_bw                 = 1.25e6;
        openair0_cfg[0].rx_bw                 = 1.25e6;
        break;

      default:
        LOG_E(HW,"Error: unknown sampling rate %f\n",openair0_cfg[0].sample_rate);
        exit(-1);
        break;
    }
  }

  if (device->type == USRP_B200_DEV) {
    if ((vers == 3) && (subvers == 9) && (subsubvers>=2)) {
      openair0_cfg[0].rx_gain_calib_table = calib_table_b210;
      bw_gain_adjust=0;
      std::cerr << "-- Using calibration table: calib_table_b210" << std::endl; // Bell Labs info
    } else {
      openair0_cfg[0].rx_gain_calib_table = calib_table_b210_38;
      bw_gain_adjust=1;
      std::cerr << "-- Using calibration table: calib_table_b210_38" << std::endl; // Bell Labs info
    }

    switch ((int)openair0_cfg[0].sample_rate) {
      case 46080000:
        s->usrp->set_master_clock_rate(46.08e6);
        //openair0_cfg[0].samples_per_packet    = 1024;
        openair0_cfg[0].tx_sample_advance     = 115;
        openair0_cfg[0].tx_bw                 = 40e6;
        openair0_cfg[0].rx_bw                 = 40e6;
        break;

      case 30720000:
        s->usrp->set_master_clock_rate(30.72e6);
        //openair0_cfg[0].samples_per_packet    = 1024;
        openair0_cfg[0].tx_sample_advance     = 115;
        openair0_cfg[0].tx_bw                 = 20e6;
        openair0_cfg[0].rx_bw                 = 20e6;
        break;

      case 23040000:
        s->usrp->set_master_clock_rate(23.04e6); //to be checked
        //openair0_cfg[0].samples_per_packet    = 1024;
        openair0_cfg[0].tx_sample_advance     = 113;
        openair0_cfg[0].tx_bw                 = 20e6;
        openair0_cfg[0].rx_bw                 = 20e6;
        break;

      case 15360000:
        s->usrp->set_master_clock_rate(30.72e06);
        //openair0_cfg[0].samples_per_packet    = 1024;
        openair0_cfg[0].tx_sample_advance     = 103;
        openair0_cfg[0].tx_bw                 = 20e6;
        openair0_cfg[0].rx_bw                 = 20e6;
        break;

      case 7680000:
        s->usrp->set_master_clock_rate(30.72e6);
        //openair0_cfg[0].samples_per_packet    = 1024;
        openair0_cfg[0].tx_sample_advance     = 80;
        openair0_cfg[0].tx_bw                 = 20e6;
        openair0_cfg[0].rx_bw                 = 20e6;
        break;

      case 1920000:
        s->usrp->set_master_clock_rate(30.72e6);
        //openair0_cfg[0].samples_per_packet    = 1024;
        openair0_cfg[0].tx_sample_advance     = 40;
        openair0_cfg[0].tx_bw                 = 20e6;
        openair0_cfg[0].rx_bw                 = 20e6;
        break;

      default:
        LOG_E(HW,"Error: unknown sampling rate %f\n",openair0_cfg[0].sample_rate);
        exit(-1);
        break;
    }
  }

  /* device specific */
  //openair0_cfg[0].txlaunch_wait = 1;//manage when TX processing is triggered
  //openair0_cfg[0].txlaunch_wait_slotcount = 1; //manage when TX processing is triggered
  openair0_cfg[0].iq_txshift = 4;//shift
  openair0_cfg[0].iq_rxrescale = 15;//rescale iqs

  for(int i=0; i<((int) s->usrp->get_rx_num_channels()); i++) {
    if (i<openair0_cfg[0].rx_num_channels) {
      s->usrp->set_rx_rate(openair0_cfg[0].sample_rate,i+choffset);
      s->usrp->set_rx_freq(openair0_cfg[0].rx_freq[i],i+choffset);
      set_rx_gain_offset(&openair0_cfg[0],i,bw_gain_adjust);
      ::uhd::gain_range_t gain_range = s->usrp->get_rx_gain_range(i+choffset);
      // limit to maximum gain
      double gain=openair0_cfg[0].rx_gain[i]-openair0_cfg[0].rx_gain_offset[i];
      if ( gain > gain_range.stop())  {
                   LOG_E(HW,"RX Gain too high, lower by %f dB\n",
                   gain - gain_range.stop());
               gain=gain_range.stop();
      }

      s->usrp->set_rx_gain(gain,i+choffset);
      LOG_I(HW,"RX Gain %d %f (%f) => %f (max %f)\n",i,
            openair0_cfg[0].rx_gain[i],openair0_cfg[0].rx_gain_offset[i],
            openair0_cfg[0].rx_gain[i]-openair0_cfg[0].rx_gain_offset[i],gain_range.stop());
    }
  }

  LOG_D(HW, "usrp->get_tx_num_channels() == %zd\n", s->usrp->get_tx_num_channels());
  LOG_D(HW, "openair0_cfg[0].tx_num_channels == %d\n", openair0_cfg[0].tx_num_channels);

  for(int i=0; i<((int) s->usrp->get_tx_num_channels()); i++) {
    ::uhd::gain_range_t gain_range_tx = s->usrp->get_tx_gain_range(i);

    if (i<openair0_cfg[0].tx_num_channels) {
      s->usrp->set_tx_rate(openair0_cfg[0].sample_rate,i+choffset);
      s->usrp->set_tx_freq(openair0_cfg[0].tx_freq[i],i+choffset);
      s->usrp->set_tx_gain(gain_range_tx.stop()-openair0_cfg[0].tx_gain[i],i+choffset);
      LOG_I(HW,"USRP TX_GAIN:%3.2lf gain_range:%3.2lf tx_gain:%3.2lf\n", gain_range_tx.stop()-openair0_cfg[0].tx_gain[i], gain_range_tx.stop(), openair0_cfg[0].tx_gain[i]);
    }
  }

  //s->usrp->set_clock_source("external");
  //s->usrp->set_time_source("external");
  // display USRP settings
  LOG_I(HW,"Actual master clock: %fMHz...\n",s->usrp->get_master_clock_rate()/1e6);
  LOG_I(HW,"Actual clock source %s...\n",s->usrp->get_clock_source(0).c_str());
  LOG_I(HW,"Actual time source %s...\n",s->usrp->get_time_source(0).c_str());
   sleep(1);
  // create tx & rx streamer
  uhd::stream_args_t stream_args_rx("sc16", "sc16");
  int samples=openair0_cfg[0].sample_rate;
  int max=s->usrp->get_rx_stream(stream_args_rx)->get_max_num_samps();
  samples/=10000;
  LOG_I(HW,"RF board max packet size %u, size for 100µs jitter %d \n", max, samples);

  if ( samples < max ) {
    stream_args_rx.args["spp"] = str(boost::format("%d") % samples );
  }

  LOG_I(HW,"rx_max_num_samps %zu\n",
        s->usrp->get_rx_stream(stream_args_rx)->get_max_num_samps());

  for (int i = 0; i<openair0_cfg[0].rx_num_channels; i++) {
    LOG_I(HW,"setting rx channel %d\n",i+choffset);
    stream_args_rx.channels.push_back(i+choffset);
  }

  s->rx_stream = s->usrp->get_rx_stream(stream_args_rx);
  uhd::stream_args_t stream_args_tx("sc16", "sc16");

  for (int i = 0; i<openair0_cfg[0].tx_num_channels; i++)
    stream_args_tx.channels.push_back(i+choffset);

  s->tx_stream = s->usrp->get_tx_stream(stream_args_tx);

  /* Setting TX/RX BW after streamers are created due to USRP calibration issue */
  for(int i=0; i<((int) s->usrp->get_tx_num_channels()) && i<openair0_cfg[0].tx_num_channels; i++)
    s->usrp->set_tx_bandwidth(openair0_cfg[0].tx_bw,i+choffset);

  for(int i=0; i<((int) s->usrp->get_rx_num_channels()) && i<openair0_cfg[0].rx_num_channels; i++)
    s->usrp->set_rx_bandwidth(openair0_cfg[0].rx_bw,i+choffset);

  for (int i=0; i<openair0_cfg[0].rx_num_channels; i++) {
    LOG_I(HW,"RX Channel %d\n",i);
    LOG_I(HW,"  Actual RX sample rate: %fMSps...\n",s->usrp->get_rx_rate(i+choffset)/1e6);
    LOG_I(HW,"  Actual RX frequency: %fGHz...\n", s->usrp->get_rx_freq(i+choffset)/1e9);
    LOG_I(HW,"  Actual RX gain: %f...\n", s->usrp->get_rx_gain(i+choffset));
    LOG_I(HW,"  Actual RX bandwidth: %fM...\n", s->usrp->get_rx_bandwidth(i+choffset)/1e6);
    LOG_I(HW,"  Actual RX antenna: %s...\n", s->usrp->get_rx_antenna(i+choffset).c_str());
  }

  for (int i=0; i<openair0_cfg[0].tx_num_channels; i++) {
    LOG_I(HW,"TX Channel %d\n",i);
    LOG_I(HW,"  Actual TX sample rate: %fMSps...\n", s->usrp->get_tx_rate(i+choffset)/1e6);
    LOG_I(HW,"  Actual TX frequency: %fGHz...\n", s->usrp->get_tx_freq(i+choffset)/1e9);
    LOG_I(HW,"  Actual TX gain: %f...\n", s->usrp->get_tx_gain(i+choffset));
    LOG_I(HW,"  Actual TX bandwidth: %fM...\n", s->usrp->get_tx_bandwidth(i+choffset)/1e6);
    LOG_I(HW,"  Actual TX antenna: %s...\n", s->usrp->get_tx_antenna(i+choffset).c_str());
    LOG_I(HW,"  Actual TX packet size: %lu\n",s->tx_stream->get_max_num_samps());
  }

  LOG_I(HW,"Device timestamp: %f...\n", s->usrp->get_time_now().get_real_secs());
  device->trx_write_func = trx_usrp_write;
  device->trx_read_func  = trx_usrp_read;
  s->sample_rate = openair0_cfg[0].sample_rate;

  // TODO:
  // init tx_forward_nsamps based usrp_time_offset ex
  if(is_equal(s->sample_rate, (double)30.72e6))
    s->tx_forward_nsamps  = 176;

  if(is_equal(s->sample_rate, (double)15.36e6))
    s->tx_forward_nsamps = 90;

  if(is_equal(s->sample_rate, (double)7.68e6))
    s->tx_forward_nsamps = 50;

  recplay_state_t *recPlay=device->recplay_state;

  if (recPlay != NULL) { // record mode
    recPlay->maxSizeBytes=openair0_cfg[0].recplay_conf->u_sf_max *
                            (sizeof(iqrec_t)+BELL_LABS_IQ_BYTES_PER_SF);
    recPlay->ms_sample = (iqrec_t *) malloc(recPlay->maxSizeBytes);
    recPlay->currentPtr= (uint8_t *)recPlay->ms_sample;

    if (recPlay->ms_sample == NULL) {
      std::cerr<< "Memory allocation failed for subframe record or replay mode." << std::endl;
      exit(-1);
    }
  }
  return 0;
}
/*@}*/
}/* extern c */
