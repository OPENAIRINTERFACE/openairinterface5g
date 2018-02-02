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

#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <uhd/utils/thread_priority.hpp>
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
#include "UTIL/LOG/log_extern.h"
#include "common_lib.h"
#include "assertions.h"

#ifdef __SSE4_1__
#  include <smmintrin.h>
#endif

#ifdef __AVX2__
#  include <immintrin.h>
#endif

#ifdef __arm__
#  include <arm_neon.h>
#endif

/** @addtogroup _USRP_PHY_RF_INTERFACE_
 * @{
 */

/*! \brief USRP Configuration */
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

static int sync_to_gps(openair0_device *device)
{
    uhd::set_thread_priority_safe();

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
 
    usrp_state_t *s = (usrp_state_t*)device->priv;    

    try
    {
        size_t num_mboards = s->usrp->get_num_mboards();
        size_t num_gps_locked = 0;
        for (size_t mboard = 0; mboard < num_mboards; mboard++)
        {
            std::cout << "Synchronizing mboard " << mboard << ": " << s->usrp->get_mboard_name(mboard) << std::endl;

            //Set references to GPSDO
            s->usrp->set_clock_source("gpsdo", mboard);
            s->usrp->set_time_source("gpsdo", mboard);

            //std::cout << std::endl;
            //print_notes();
            //std::cout << std::endl;

            //Check for 10 MHz lock
            std::vector<std::string> sensor_names = s->usrp->get_mboard_sensor_names(mboard);
            if(std::find(sensor_names.begin(), sensor_names.end(), "ref_locked") != sensor_names.end())
            {
                std::cout << "Waiting for reference lock..." << std::flush;
                bool ref_locked = false;
                for (int i = 0; i < 30 and not ref_locked; i++)
                {
                    ref_locked = s->usrp->get_mboard_sensor("ref_locked", mboard).to_bool();
                    if (not ref_locked)
                    {
                        std::cout << "." << std::flush;
                        boost::this_thread::sleep(boost::posix_time::seconds(1));
                    }
                }
                if(ref_locked)
                {
                    std::cout << "LOCKED" << std::endl;
                } else {
                    std::cout << "FAILED" << std::endl;
                    std::cout << "Failed to lock to GPSDO 10 MHz Reference. Exiting." << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                std::cout << boost::format("ref_locked sensor not present on this board.\n");
            }

            //Wait for GPS lock
            bool gps_locked = s->usrp->get_mboard_sensor("gps_locked", mboard).to_bool();
            if(gps_locked)
            {
                num_gps_locked++;
                std::cout << boost::format("GPS Locked\n");
            }
            else
            {
                std::cerr << "WARNING:  GPS not locked - time will not be accurate until locked" << std::endl;
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

        if (num_gps_locked == num_mboards and num_mboards > 1)
        {
            //Check to see if all USRP times are aligned
            //First, wait for PPS.
            uhd::time_spec_t time_last_pps = s->usrp->get_time_last_pps();
            while (time_last_pps == s->usrp->get_time_last_pps())
            {
                boost::this_thread::sleep(boost::posix_time::milliseconds(1));
            }

            //Sleep a little to make sure all devices have seen a PPS edge
            boost::this_thread::sleep(boost::posix_time::milliseconds(200));

            //Compare times across all mboards
            bool all_matched = true;
            uhd::time_spec_t mboard0_time = s->usrp->get_time_last_pps(0);
            for (size_t mboard = 1; mboard < num_mboards; mboard++)
            {
                uhd::time_spec_t mboard_time = s->usrp->get_time_last_pps(mboard);
                if (mboard_time != mboard0_time)
                {
                    all_matched = false;
                    std::cerr << (boost::format("ERROR: Times are not aligned: USRP 0=%0.9f, USRP %d=%0.9f")
                                  % mboard0_time.get_real_secs()
                                  % mboard
                                  % mboard_time.get_real_secs()) << std::endl;
                }
            }
            if (all_matched)
            {
                std::cout << "SUCCESS: USRP times aligned" << std::endl << std::endl;
            } else {
                std::cout << "ERROR: USRP times are not aligned" << std::endl << std::endl;
            }
        }
    }
    catch (std::exception& e)
    {
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

#if defined(USRP_REC_PLAY)
#include "usrp_lib.h"
static FILE    *pFile = NULL;
int             mmapfd = 0;
struct stat     sb;
iqrec_t        *ms_sample = NULL;                      // memory for all subframes
unsigned int    nb_samples = 0;
unsigned int    cur_samples = 0;
int64_t         wrap_count = 0;
int64_t         wrap_ts = 0;
unsigned int    u_sf_mode = 0;                         // 1=record, 2=replay
unsigned int    u_sf_record = 0;                       // record mode
unsigned int    u_sf_replay = 0;                       // replay mode
char            u_sf_filename[1024] = "";              // subframes file path
unsigned int    u_sf_max = DEF_NB_SF;                  // max number of recorded subframes
unsigned int    u_sf_loops = DEF_SF_NB_LOOP;           // number of loops in replay mode
unsigned int    u_sf_read_delay = DEF_SF_DELAY_READ;   // read delay in replay mode
unsigned int    u_sf_write_delay = DEF_SF_DELAY_WRITE; // write delay in replay mode

char config_opt_sf_file[] = CONFIG_OPT_SF_FILE;
char config_def_sf_file[] = DEF_SF_FILE;
char config_hlp_sf_file[] = CONFIG_HLP_SF_FILE;
char config_opt_sf_rec[] = CONFIG_OPT_SF_REC;
char config_hlp_sf_rec[] = CONFIG_HLP_SF_REC;
char config_opt_sf_rep[] = CONFIG_OPT_SF_REP;
char config_hlp_sf_rep[] = CONFIG_HLP_SF_REP;
char config_opt_sf_max[] = CONFIG_OPT_SF_MAX;
char config_hlp_sf_max[] = CONFIG_HLP_SF_MAX;
char config_opt_sf_loops[] = CONFIG_OPT_SF_LOOPS;
char config_hlp_sf_loops[] = CONFIG_HLP_SF_LOOPS;
char config_opt_sf_rdelay[] = CONFIG_OPT_SF_RDELAY;
char config_hlp_sf_rdelay[] = CONFIG_HLP_SF_RDELAY;
char config_opt_sf_wdelay[] = CONFIG_OPT_SF_WDELAY;
char config_hlp_sf_wdelay[] = CONFIG_HLP_SF_WDELAY;

#endif

/*! \brief Called to start the USRP transceiver. Return 0 if OK, < 0 if error
    @param device pointer to the device structure specific to the RF hardware target
*/
static int trx_usrp_start(openair0_device *device) {

#if defined(USRP_REC_PLAY)
    if (u_sf_mode != 2) { // not replay mode
#endif      

    usrp_state_t *s = (usrp_state_t*)device->priv;


  // setup GPIO for TDD, GPIO(4) = ATR_RX
  //set data direction register (DDR) to output
    s->usrp->set_gpio_attr("FP0", "DDR", 0x1f, 0x1f);
  
  //set control register to ATR
    s->usrp->set_gpio_attr("FP0", "CTRL", 0x1f,0x1f);
  
  //set ATR register
    s->usrp->set_gpio_attr("FP0", "ATR_RX", 1<<4, 0x1f);

    // init recv and send streaming
    uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    LOG_I(PHY,"Time in secs now: %llu \n", s->usrp->get_time_now().to_ticks(s->sample_rate));
    LOG_I(PHY,"Time in secs last pps: %llu \n", s->usrp->get_time_last_pps().to_ticks(s->sample_rate));

    if (s->use_gps == 1) {
      s->wait_for_first_pps = 1;
      cmd.time_spec = s->usrp->get_time_last_pps() + uhd::time_spec_t(1.0);    
    }
    else {
      s->wait_for_first_pps = 0; 
      cmd.time_spec = s->usrp->get_time_now() + uhd::time_spec_t(0.05);
    }

    cmd.stream_now = false; // start at constant delay
    s->rx_stream->issue_stream_cmd(cmd);

    s->tx_md.time_spec = cmd.time_spec + uhd::time_spec_t(1-(double)s->tx_forward_nsamps/s->sample_rate);
    s->tx_md.has_time_spec = true;
    s->tx_md.start_of_burst = true;
    s->tx_md.end_of_burst = false;

    s->rx_count = 0;
    s->tx_count = 0;
    s->rx_timestamp = 0;
#if defined(USRP_REC_PLAY)
    }
#endif      
    return 0;
}
/*! \brief Terminate operation of the USRP transceiver -- free all associated resources
 * \param device the hardware to use
 */
static void trx_usrp_end(openair0_device *device) {
#if defined(USRP_REC_PLAY) // For some ugly reason, this can be called several times...
  static int done = 0;
  if (done == 1) return;
  done = 1;
  if (u_sf_mode != 2) { // not subframes replay
#endif  
    usrp_state_t *s = (usrp_state_t*)device->priv;

    s->rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    //send a mini EOB packet
    s->tx_md.end_of_burst = true;
    s->tx_stream->send("", 0, s->tx_md);
    s->tx_md.end_of_burst = false;
#if defined(USRP_REC_PLAY)
    }
#endif
#if defined(USRP_REC_PLAY)
    if (u_sf_mode == 1) { // subframes store
      pFile = fopen (u_sf_filename,"wb+");
      if (pFile == NULL) {
	std::cerr << "Cannot open " << u_sf_filename << std::endl;
      } else {
	unsigned int i = 0;
	unsigned int modu = 0;
	if ((modu = nb_samples % 10) != 0) {
	  nb_samples -= modu; // store entire number of frames
	}
	std::cerr << "Writing " << nb_samples << " subframes to " << u_sf_filename << " ..." << std::endl;
	for (i = 0; i < nb_samples; i++) {
	  fwrite(ms_sample+i, sizeof(unsigned char), sizeof(iqrec_t), pFile);
	}
	fclose (pFile);
	std::cerr << "File " << u_sf_filename << " closed." << std::endl;
      }
    }
    if (u_sf_mode == 1) { // record
      if (ms_sample != NULL) {
	free((void*)ms_sample);
	ms_sample = NULL;
      }
    }
    if (u_sf_mode == 2) { // replay
      if (ms_sample != MAP_FAILED) {
	munmap(ms_sample, sb.st_size);
	ms_sample = NULL;
      }
      if (mmapfd != 0) {
	close(mmapfd);
	mmapfd = 0;
      }
    }
#endif    
}

/*! \brief Called to send samples to the USRP RF target
      @param device pointer to the device structure specific to the RF hardware target
      @param timestamp The timestamp at which the first sample MUST be sent
      @param buff Buffer which holds the samples
      @param nsamps number of samples to be sent
      @param antenna_id index of the antenna if the device has multiple antennas
      @param flags flags must be set to TRUE if timestamp parameter needs to be applied
*/
static int trx_usrp_write(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags) {
  int ret=0;
#if defined(USRP_REC_PLAY)
  if (u_sf_mode != 2) { // not replay mode
#endif    
  usrp_state_t *s = (usrp_state_t*)device->priv;
  
  int nsamps2;  // aligned to upper 32 or 16 byte boundary
#if defined(__x86_64) || defined(__i386__)
#ifdef __AVX2__
  nsamps2 = (nsamps+7)>>3;
  __m256i buff_tx[2][nsamps2];
#else
  nsamps2 = (nsamps+3)>>2;
  __m128i buff_tx[2][nsamps2];
#endif
#elif defined(__arm__)
  nsamps2 = (nsamps+3)>>2;
  int16x8_t buff_tx[2][nsamps2];
#endif
  
  // bring RX data into 12 LSBs for softmodem RX
  for (int i=0; i<cc; i++) {
    for (int j=0; j<nsamps2; j++) {
#if defined(__x86_64__) || defined(__i386__)
#ifdef __AVX2__
      buff_tx[i][j] = _mm256_slli_epi16(((__m256i*)buff[i])[j],4);
#else
      buff_tx[i][j] = _mm_slli_epi16(((__m128i*)buff[i])[j],4);
#endif
#elif defined(__arm__)
      buff_tx[i][j] = vshlq_n_s16(((int16x8_t*)buff[i])[j],4);
#endif
    }
  }

  s->tx_md.time_spec = uhd::time_spec_t::from_ticks(timestamp, s->sample_rate);
  s->tx_md.has_time_spec = flags;
  
  
  if(flags>0)
    s->tx_md.has_time_spec = true;
  else
    s->tx_md.has_time_spec = false;
  
  if (flags == 2) { // start of burst
    s->tx_md.start_of_burst = true;
    s->tx_md.end_of_burst = false;
  } else if (flags == 3) { // end of burst
    s->tx_md.start_of_burst = false;
    s->tx_md.end_of_burst = true;
  } else if (flags == 4) { // start and end
    s->tx_md.start_of_burst = true;
    s->tx_md.end_of_burst = true;
  } else if (flags==1) { // middle of burst
    s->tx_md.start_of_burst = false;
    s->tx_md.end_of_burst = false;
  }
  
  if (cc>1) {
    std::vector<void *> buff_ptrs;
    for (int i=0; i<cc; i++)
      buff_ptrs.push_back(buff_tx[i]);
    ret = (int)s->tx_stream->send(buff_ptrs, nsamps, s->tx_md,1e-3);
  } else
    ret = (int)s->tx_stream->send(buff_tx[0], nsamps, s->tx_md,1e-3);
  
  
  
  if (ret != nsamps)
    LOG_E(PHY,"[xmit] tx samples %d != %d\n",ret,nsamps);
#if defined(USRP_REC_PLAY)
  } else {
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = u_sf_write_delay * 1000;
    nanosleep(&req, NULL);
    ret = nsamps;
  }
#endif    

  return ret;
}

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
  usrp_state_t *s = (usrp_state_t*)device->priv;
  int samples_received=0,i,j;
  int nsamps2;  // aligned to upper 32 or 16 byte boundary
#if defined(USRP_REC_PLAY)
  if (u_sf_mode != 2) { // not replay mode
#endif    
#if defined(__x86_64) || defined(__i386__)
#ifdef __AVX2__
    nsamps2 = (nsamps+7)>>3;
    __m256i buff_tmp[2][nsamps2];
#else
    nsamps2 = (nsamps+3)>>2;
    __m128i buff_tmp[2][nsamps2];
#endif
#elif defined(__arm__)
    nsamps2 = (nsamps+3)>>2;
    int16x8_t buff_tmp[2][nsamps2];
#endif

    if (device->type == USRP_B200_DEV) {
        if (cc>1) {
            // receive multiple channels (e.g. RF A and RF B)
            std::vector<void *> buff_ptrs;
            for (int i=0; i<cc; i++) buff_ptrs.push_back(buff_tmp[i]);
            samples_received = s->rx_stream->recv(buff_ptrs, nsamps, s->rx_md);
        } else {
            // receive a single channel (e.g. from connector RF A)
            samples_received=0;
            while (samples_received != nsamps) {
                samples_received += s->rx_stream->recv(buff_tmp[0]+samples_received,
                                                       nsamps-samples_received, s->rx_md);
                if  ((s->wait_for_first_pps == 0) && (s->rx_md.error_code!=uhd::rx_metadata_t::ERROR_CODE_NONE))
                    break;
	        if ((s->wait_for_first_pps == 1) && (samples_received != nsamps)) { printf("sleep...\n");} //usleep(100); 
            }
	    if (samples_received == nsamps) s->wait_for_first_pps=0;
        }
        // bring RX data into 12 LSBs for softmodem RX
        for (int i=0; i<cc; i++) {
            for (int j=0; j<nsamps2; j++) {
#if defined(__x86_64__) || defined(__i386__)
#ifdef __AVX2__
                ((__m256i *)buff[i])[j] = _mm256_srai_epi16(buff_tmp[i][j],4);
#else
                ((__m128i *)buff[i])[j] = _mm_srai_epi16(buff_tmp[i][j],4);
#endif
#elif defined(__arm__)
                ((int16x8_t*)buff[i])[j] = vshrq_n_s16(buff_tmp[i][j],4);
#endif
            }
        }
    } else if (device->type == USRP_X300_DEV) {
        if (cc>1) {
            // receive multiple channels (e.g. RF A and RF B)
            std::vector<void *> buff_ptrs;

            for (int i=0; i<cc; i++) buff_ptrs.push_back(buff[i]);
            samples_received = s->rx_stream->recv(buff_ptrs, nsamps, s->rx_md);
        } else {
            // receive a single channel (e.g. from connector RF A)
            samples_received = s->rx_stream->recv(buff[0], nsamps, s->rx_md);
        }
    }
    if (samples_received < nsamps)
        LOG_E(PHY,"[recv] received %d samples out of %d\n",samples_received,nsamps);

    if ( s->rx_md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE)
        LOG_E(PHY, "%s\n", s->rx_md.to_pp_string(true).c_str());

    s->rx_count += nsamps;
    s->rx_timestamp = s->rx_md.time_spec.to_ticks(s->sample_rate);
    *ptimestamp = s->rx_timestamp;
#if defined (USRP_REC_PLAY)
  }
#endif    
#if defined(USRP_REC_PLAY)
  if (u_sf_mode == 1) { // record mode
    // Copy subframes to memory (later dump on a file)
    if (nb_samples < u_sf_max) {
      (ms_sample+nb_samples)->header = BELL_LABS_IQ_HEADER; 
      (ms_sample+nb_samples)->ts = *ptimestamp;
      memcpy((ms_sample+nb_samples)->samples, buff[0], nsamps*4);
      nb_samples++;
    }
  } else if (u_sf_mode == 2) { // replay mode
    if (cur_samples == nb_samples) {
      cur_samples = 0;
      wrap_count++;
      if (wrap_count == u_sf_loops) {
	std::cerr << "USRP device terminating subframes replay mode after " << u_sf_loops << " loops." << std::endl;
	return 0; // should make calling process exit
      }
      wrap_ts = wrap_count * (nb_samples * (((int)(device->openair0_cfg[0].sample_rate)) / 1000));
    }
    if (cur_samples < nb_samples) {
      *ptimestamp = (ms_sample[0].ts + (cur_samples * (((int)(device->openair0_cfg[0].sample_rate)) / 1000))) + wrap_ts;
      if (cur_samples == 0) {
	std::cerr << "starting subframes file with wrap_count=" << wrap_count << " wrap_ts=" << wrap_ts
		  << " ts=" << *ptimestamp << std::endl;
      }
      memcpy(buff[0], &ms_sample[cur_samples].samples[0], nsamps*4);
      cur_samples++;
    }
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = u_sf_read_delay * 1000;
    nanosleep(&req, NULL);
    return nsamps;
  }
#endif
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
    usrp_state_t *s = (usrp_state_t*)device->priv;

    s->usrp->set_tx_freq(device->openair0_cfg[0].tx_freq[0]);
    s->usrp->set_rx_freq(device->openair0_cfg[0].rx_freq[0]);
}
/*! \brief Set frequencies (TX/RX). Spawns a thread to handle the frequency change to not block the calling thread
 * \param device the hardware to use
 * \param openair0_cfg RF frontend parameters set by application
 * \param dummy dummy variable not used
 * \returns 0 in success
 */
int trx_usrp_set_freq(openair0_device* device, openair0_config_t *openair0_cfg, int dont_block) {

    usrp_state_t *s = (usrp_state_t*)device->priv;
    pthread_t f_thread;

    printf("Setting USRP TX Freq %f, RX Freq %f\n",openair0_cfg[0].tx_freq[0],openair0_cfg[0].rx_freq[0]);

    // spawn a thread to handle the frequency change to not block the calling thread
    if (dont_block == 1)
        pthread_create(&f_thread,NULL,freq_thread,(void*)device);
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
int openair0_set_rx_frequencies(openair0_device* device, openair0_config_t *openair0_cfg) {

    usrp_state_t *s = (usrp_state_t*)device->priv;
    static int first_call=1;
    static double rf_freq,diff;

    uhd::tune_request_t rx_tune_req(openair0_cfg[0].rx_freq[0]);

    rx_tune_req.rf_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    rx_tune_req.rf_freq = openair0_cfg[0].rx_freq[0];
    rf_freq=openair0_cfg[0].rx_freq[0];
    s->usrp->set_rx_freq(rx_tune_req);

    return(0);

}

/*! \brief Set Gains (TX/RX)
 * \param device the hardware to use
 * \param openair0_cfg RF frontend parameters set by application
 * \returns 0 in success
 */
int trx_usrp_set_gains(openair0_device* device,
                       openair0_config_t *openair0_cfg) {

    usrp_state_t *s = (usrp_state_t*)device->priv;
    ::uhd::gain_range_t gain_range_tx = s->usrp->get_tx_gain_range(0);
    s->usrp->set_tx_gain(gain_range_tx.stop()-openair0_cfg[0].tx_gain[0]);
    ::uhd::gain_range_t gain_range = s->usrp->get_rx_gain_range(0);
    // limit to maximum gain
    if (openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0] > gain_range.stop()) {
        LOG_E(PHY,"RX Gain 0 too high, reduce by %f dB\n",
              openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0] - gain_range.stop());
        exit(-1);
    }
    s->usrp->set_rx_gain(openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0]);
    LOG_I(PHY,"Setting USRP RX gain to %f (rx_gain %f,gain_range.stop() %f)\n",
          openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0],
          openair0_cfg[0].rx_gain[0],gain_range.stop());

    return(0);
}

/*! \brief Stop USRP
 * \param card refers to the hardware index to use
 */
int trx_usrp_stop(openair0_device* device) {
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
            LOG_E(PHY,"unknown sampling rate %d\n",(int)openair0_cfg[0].sample_rate);
            exit(-1);
            break;
        }
    }
    while (openair0_cfg->rx_gain_calib_table[i].freq>0) {
        diff = fabs(openair0_cfg->rx_freq[chain_index] - openair0_cfg->rx_gain_calib_table[i].freq);
        LOG_I(PHY,"cal %d: freq %f, offset %f, diff %f\n",
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
int trx_usrp_get_stats(openair0_device* device) {
    return(0);
}

/*! \brief Reset the USRP statistics
 * \param device the hardware to use
 * \returns  0 on success
 */
int trx_usrp_reset_stats(openair0_device* device) {
    return(0);
}

#if defined(USRP_REC_PLAY)
extern "C" {
/*! \brief Initializer for USRP record/playback config
 * \param parameter array description
 * \returns  0 on success
 */
int trx_usrp_recplay_config_init(paramdef_t *usrp_recplay_params) {
    // --subframes-file
    memcpy(usrp_recplay_params[0].optname, config_opt_sf_file, strlen(config_opt_sf_file));
    usrp_recplay_params[0].helpstr = config_hlp_sf_file;
    usrp_recplay_params[0].paramflags=PARAMFLAG_NOFREE;
    usrp_recplay_params[0].strptr=(char **)&u_sf_filename;
    usrp_recplay_params[0].defstrval = NULL;
    usrp_recplay_params[0].type=TYPE_STRING;
    usrp_recplay_params[0].numelt=sizeof(u_sf_filename);
    // --subframes-record
    memcpy(usrp_recplay_params[1].optname, config_opt_sf_rec, strlen(config_opt_sf_rec));
    usrp_recplay_params[1].helpstr = config_hlp_sf_rec;
    usrp_recplay_params[1].paramflags=PARAMFLAG_BOOL;
    usrp_recplay_params[1].uptr=&u_sf_record;
    usrp_recplay_params[1].defuintval=0;
    usrp_recplay_params[1].type=TYPE_UINT;
    usrp_recplay_params[1].numelt=0;
    // --subframes-replay
    memcpy(usrp_recplay_params[2].optname, config_opt_sf_rep, strlen(config_opt_sf_rep));
    usrp_recplay_params[2].helpstr = config_hlp_sf_rep;
    usrp_recplay_params[2].paramflags=PARAMFLAG_BOOL;
    usrp_recplay_params[2].uptr=&u_sf_replay;
    usrp_recplay_params[2].defuintval=0;
    usrp_recplay_params[2].type=TYPE_UINT;
    usrp_recplay_params[2].numelt=0;
    // --subframes-max
    memcpy(usrp_recplay_params[3].optname, config_opt_sf_max, strlen(config_opt_sf_max));
    usrp_recplay_params[3].helpstr = config_hlp_sf_max;
    usrp_recplay_params[3].paramflags=0;
    usrp_recplay_params[3].uptr=&u_sf_max;
    usrp_recplay_params[3].defuintval=DEF_NB_SF;
    usrp_recplay_params[3].type=TYPE_UINT;
    usrp_recplay_params[3].numelt=0;
    // --subframes-loops
    memcpy(usrp_recplay_params[4].optname, config_opt_sf_loops, strlen(config_opt_sf_loops));
    usrp_recplay_params[4].helpstr = config_hlp_sf_loops;
    usrp_recplay_params[4].paramflags=0;
    usrp_recplay_params[4].uptr=&u_sf_loops;
    usrp_recplay_params[4].defuintval=DEF_SF_NB_LOOP;
    usrp_recplay_params[4].type=TYPE_UINT;
    usrp_recplay_params[4].numelt=0;
    // --subframes-read-delay
    memcpy(usrp_recplay_params[5].optname, config_opt_sf_rdelay, strlen(config_opt_sf_rdelay));
    usrp_recplay_params[5].helpstr = config_hlp_sf_rdelay;
    usrp_recplay_params[5].paramflags=0;
    usrp_recplay_params[5].uptr=&u_sf_read_delay;
    usrp_recplay_params[5].defuintval=DEF_SF_DELAY_READ;
    usrp_recplay_params[5].type=TYPE_UINT;
    usrp_recplay_params[5].numelt=0;
    // --subframes-write-delay
    memcpy(usrp_recplay_params[6].optname, config_opt_sf_wdelay, strlen(config_opt_sf_wdelay));
    usrp_recplay_params[6].helpstr = config_hlp_sf_wdelay;
    usrp_recplay_params[6].paramflags=0;
    usrp_recplay_params[6].uptr=&u_sf_write_delay;
    usrp_recplay_params[6].defuintval=DEF_SF_DELAY_WRITE;
    usrp_recplay_params[6].type=TYPE_UINT;
    usrp_recplay_params[6].numelt=0;

    return 0; // always ok
}
}
#endif

extern "C" {
    /*! \brief Initialize Openair USRP target. It returns 0 if OK
    * \param device the hardware to use
         * \param openair0_cfg RF frontend parameters set by application
         */
    int device_init(openair0_device* device, openair0_config_t *openair0_cfg) {
#if defined(USRP_REC_PLAY)
      paramdef_t usrp_recplay_params[7];
      // to check
      static int done = 0;
      if (done == 1) {
	return 0;
      } // prevent from multiple init
      done = 1;
      // end to check
      memset(usrp_recplay_params, 0, 7*sizeof(paramdef_t));
      memset(&u_sf_filename[0], 0, 1024);
      if (trx_usrp_recplay_config_init(usrp_recplay_params) != 0) {
	std::cerr << "USRP device record/replay mode configuration error exiting" << std::endl;
	return -1;
      }
      config_process_cmdline(usrp_recplay_params,sizeof(usrp_recplay_params)/sizeof(paramdef_t),NULL);

      if (strlen(u_sf_filename) == 0) {
	(void) strcpy(u_sf_filename, DEF_SF_FILE);
      }

      if (u_sf_replay == 1) u_sf_mode = 2;
      if (u_sf_record == 1) u_sf_mode = 1;
      
      if (u_sf_mode == 2) {
	// Replay subframes from from file
        int bw_gain_adjust=0;
        device->openair0_cfg = openair0_cfg;
	device->type = USRP_B200_DEV;
	openair0_cfg[0].rx_gain_calib_table = calib_table_b210_38;
	bw_gain_adjust=1;
	openair0_cfg[0].tx_sample_advance     = 80;
	openair0_cfg[0].tx_bw                 = 20e6;
	openair0_cfg[0].rx_bw                 = 20e6;
        openair0_cfg[0].iq_txshift = 4;//shift
        openair0_cfg[0].iq_rxrescale = 15;//rescale iqs
	set_rx_gain_offset(&openair0_cfg[0],0,bw_gain_adjust);
        device->priv = NULL;
        device->trx_start_func = trx_usrp_start;
        device->trx_write_func = trx_usrp_write;
        device->trx_read_func  = trx_usrp_read;
        device->trx_get_stats_func = trx_usrp_get_stats;
        device->trx_reset_stats_func = trx_usrp_reset_stats;
        device->trx_end_func   = trx_usrp_end;
        device->trx_stop_func  = trx_usrp_stop;
        device->trx_set_freq_func = trx_usrp_set_freq;
        device->trx_set_gains_func   = trx_usrp_set_gains;
        device->openair0_cfg = openair0_cfg;
	std::cerr << "USRP device initialized in subframes replay mode for " << u_sf_loops << " loops." << std::endl;
      } else {
#endif
        uhd::set_thread_priority_safe(1.0);
        usrp_state_t *s = (usrp_state_t*)calloc(sizeof(usrp_state_t),1);
        
	if (openair0_cfg[0].clock_source==gpsdo)        
	    s->use_gps =1;

        // Initialize USRP device
        device->openair0_cfg = openair0_cfg;

        std::string args = "type=b200";
        uhd::device_addrs_t device_adds = uhd::device::find(args);

        int vers=0,subvers=0,subsubvers=0;
        int bw_gain_adjust=0;

#if defined(USRP_REC_PLAY)
	if (u_sf_mode == 1) {
	  std::cerr << "USRP device initialized in subframes record mode" << std::endl;
	}
#endif	
        sscanf(uhd::get_version_string().c_str(),"%d.%d.%d",&vers,&subvers,&subsubvers);
        LOG_I(PHY,"Checking for USRPs : UHD %s (%d.%d.%d)\n",
              uhd::get_version_string().c_str(),vers,subvers,subsubvers);

        if(device_adds.size() == 0)  {
            double usrp_master_clock = 184.32e6;
            std::string args = "type=x300";

            // workaround for an api problem, master clock has to be set with the constructor not via set_master_clock_rate
            args += boost::str(boost::format(",master_clock_rate=%f") % usrp_master_clock);

//    args += ",num_send_frames=256,num_recv_frames=256, send_frame_size=4096, recv_frame_size=4096";

            //    args += ",num_send_frames=256,num_recv_frames=256, send_frame_size=4096, recv_frame_size=4096";
            uhd::device_addrs_t device_adds = uhd::device::find(args);

            if(device_adds.size() == 0) {
                std::cerr<<"No USRP Device Found. " << std::endl;
                free(s);
                return -1;
            }
            LOG_I(PHY,"Found USRP X300\n");
            s->usrp = uhd::usrp::multi_usrp::make(args);
            // lock mboard clocks
            if (openair0_cfg[0].clock_source == internal)
                s->usrp->set_clock_source("internal");
            else
                s->usrp->set_clock_source("external");

            //Setting device type to USRP X300/X310
            device->type=USRP_X300_DEV;

            // this is not working yet, master clock has to be set via constructor
            // set master clock rate and sample rate for tx & rx for streaming
            //s->usrp->set_master_clock_rate(usrp_master_clock);

            openair0_cfg[0].rx_gain_calib_table = calib_table_x310;

#if defined(USRP_REC_PLAY)
	    std::cerr << "-- Using calibration table: calib_table_x310" << std::endl; // Bell Labs info
#endif

            LOG_I(PHY,"%s() sample_rate:%u\n", __FUNCTION__, (int)openair0_cfg[0].sample_rate);

            switch ((int)openair0_cfg[0].sample_rate) {
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
                LOG_E(PHY,"Error: unknown sampling rate %f\n",openair0_cfg[0].sample_rate);
                exit(-1);
                break;
            }

        } else {
            LOG_I(PHY,"Found USRP B200\n");
            args += ",num_send_frames=256,num_recv_frames=256, send_frame_size=15360, recv_frame_size=15360" ;
            s->usrp = uhd::usrp::multi_usrp::make(args);

            //  s->usrp->set_rx_subdev_spec(rx_subdev);
            //  s->usrp->set_tx_subdev_spec(tx_subdev);

            // do not explicitly set the clock to "internal", because this will disable the gpsdo
            //    // lock mboard clocks
            //    s->usrp->set_clock_source("internal");
            // set master clock rate and sample rate for tx & rx for streaming

            // lock mboard clocks
            if (openair0_cfg[0].clock_source == internal){
	        s->usrp->set_clock_source("internal");
            }
            else{
                s->usrp->set_clock_source("external");
		s->usrp->set_time_source("external");
            }	

            device->type = USRP_B200_DEV;
            if ((vers == 3) && (subvers == 9) && (subsubvers>=2)) {
                openair0_cfg[0].rx_gain_calib_table = calib_table_b210;
                bw_gain_adjust=0;
#if defined(USRP_REC_PLAY)
		std::cerr << "-- Using calibration table: calib_table_b210" << std::endl; // Bell Labs info
#endif		
            } else {
                openair0_cfg[0].rx_gain_calib_table = calib_table_b210_38;
                bw_gain_adjust=1;
#if defined(USRP_REC_PLAY)
		std::cerr << "-- Using calibration table: calib_table_b210_38" << std::endl; // Bell Labs info
#endif		
            }

            switch ((int)openair0_cfg[0].sample_rate) {
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
                LOG_E(PHY,"Error: unknown sampling rate %f\n",openair0_cfg[0].sample_rate);
                exit(-1);
                break;
            }
        }

        /* device specific */
        //openair0_cfg[0].txlaunch_wait = 1;//manage when TX processing is triggered
        //openair0_cfg[0].txlaunch_wait_slotcount = 1; //manage when TX processing is triggered
        openair0_cfg[0].iq_txshift = 4;//shift
        openair0_cfg[0].iq_rxrescale = 15;//rescale iqs

        for(int i=0; i<s->usrp->get_rx_num_channels(); i++) {
            if (i<openair0_cfg[0].rx_num_channels) {
                s->usrp->set_rx_rate(openair0_cfg[0].sample_rate,i);
                s->usrp->set_rx_freq(openair0_cfg[0].rx_freq[i],i);
                set_rx_gain_offset(&openair0_cfg[0],i,bw_gain_adjust);

                ::uhd::gain_range_t gain_range = s->usrp->get_rx_gain_range(i);
                // limit to maximum gain
                AssertFatal( openair0_cfg[0].rx_gain[i]-openair0_cfg[0].rx_gain_offset[i] <= gain_range.stop(),
                             "RX Gain too high, lower by %f dB\n",
                             openair0_cfg[0].rx_gain[i]-openair0_cfg[0].rx_gain_offset[i] - gain_range.stop());
                s->usrp->set_rx_gain(openair0_cfg[0].rx_gain[i]-openair0_cfg[0].rx_gain_offset[i],i);
                LOG_I(PHY,"RX Gain %d %f (%f) => %f (max %f)\n",i,
                      openair0_cfg[0].rx_gain[i],openair0_cfg[0].rx_gain_offset[i],
                      openair0_cfg[0].rx_gain[i]-openair0_cfg[0].rx_gain_offset[i],gain_range.stop());
            }
        }

        for(int i=0; i<s->usrp->get_tx_num_channels(); i++) {
	  ::uhd::gain_range_t gain_range_tx = s->usrp->get_tx_gain_range(i);
            if (i<openair0_cfg[0].tx_num_channels) {
                s->usrp->set_tx_rate(openair0_cfg[0].sample_rate,i);
                s->usrp->set_tx_freq(openair0_cfg[0].tx_freq[i],i);
                s->usrp->set_tx_gain(gain_range_tx.stop()-openair0_cfg[0].tx_gain[i],i);

                LOG_I(PHY,"USRP TX_GAIN:%3.2lf gain_range:%3.2lf tx_gain:%3.2lf\n", gain_range_tx.stop()-openair0_cfg[0].tx_gain[i], gain_range_tx.stop(), openair0_cfg[0].tx_gain[i]);
            }
        }

        //s->usrp->set_clock_source("external");
        //s->usrp->set_time_source("external");

        // display USRP settings
        LOG_I(PHY,"Actual master clock: %fMHz...\n",s->usrp->get_master_clock_rate()/1e6);
        sleep(1);

        // create tx & rx streamer
        uhd::stream_args_t stream_args_rx("sc16", "sc16");
        int samples=openair0_cfg[0].sample_rate;
	int max=s->usrp->get_rx_stream(stream_args_rx)->get_max_num_samps();
        samples/=10000;
	LOG_I(PHY,"RF board max packet size %u, size for 100µs jitter %d \n", max, samples);
	if ( samples < max )
	  stream_args_rx.args["spp"] = str(boost::format("%d") % samples );
	LOG_I(PHY,"rx_max_num_samps %zu\n",
	      s->usrp->get_rx_stream(stream_args_rx)->get_max_num_samps());

        for (int i = 0; i<openair0_cfg[0].rx_num_channels; i++)
            stream_args_rx.channels.push_back(i);
        s->rx_stream = s->usrp->get_rx_stream(stream_args_rx);
	
        uhd::stream_args_t stream_args_tx("sc16", "sc16");
        for (int i = 0; i<openair0_cfg[0].tx_num_channels; i++)
            stream_args_tx.channels.push_back(i);
        s->tx_stream = s->usrp->get_tx_stream(stream_args_tx);

        /* Setting TX/RX BW after streamers are created due to USRP calibration issue */
        for(int i=0; i<s->usrp->get_tx_num_channels() && i<openair0_cfg[0].tx_num_channels; i++)
            s->usrp->set_tx_bandwidth(openair0_cfg[0].tx_bw,i);

        for(int i=0; i<s->usrp->get_rx_num_channels() && i<openair0_cfg[0].rx_num_channels; i++)
            s->usrp->set_rx_bandwidth(openair0_cfg[0].rx_bw,i);

        for (int i=0; i<openair0_cfg[0].rx_num_channels; i++) {
            LOG_I(PHY,"RX Channel %d\n",i);
            LOG_I(PHY,"  Actual RX sample rate: %fMSps...\n",s->usrp->get_rx_rate(i)/1e6);
            LOG_I(PHY,"  Actual RX frequency: %fGHz...\n", s->usrp->get_rx_freq(i)/1e9);
            LOG_I(PHY,"  Actual RX gain: %f...\n", s->usrp->get_rx_gain(i));
            LOG_I(PHY,"  Actual RX bandwidth: %fM...\n", s->usrp->get_rx_bandwidth(i)/1e6);
            LOG_I(PHY,"  Actual RX antenna: %s...\n", s->usrp->get_rx_antenna(i).c_str());
        }

        for (int i=0; i<openair0_cfg[0].tx_num_channels; i++) {
            LOG_I(PHY,"TX Channel %d\n",i);
            LOG_I(PHY,"  Actual TX sample rate: %fMSps...\n", s->usrp->get_tx_rate(i)/1e6);
            LOG_I(PHY,"  Actual TX frequency: %fGHz...\n", s->usrp->get_tx_freq(i)/1e9);
            LOG_I(PHY,"  Actual TX gain: %f...\n", s->usrp->get_tx_gain(i));
            LOG_I(PHY,"  Actual TX bandwidth: %fM...\n", s->usrp->get_tx_bandwidth(i)/1e6);
            LOG_I(PHY,"  Actual TX antenna: %s...\n", s->usrp->get_tx_antenna(i).c_str());
        }

        LOG_I(PHY,"Device timestamp: %f...\n", s->usrp->get_time_now().get_real_secs());

        device->priv = s;
        device->trx_start_func = trx_usrp_start;
        device->trx_write_func = trx_usrp_write;
        device->trx_read_func  = trx_usrp_read;
        device->trx_get_stats_func = trx_usrp_get_stats;
        device->trx_reset_stats_func = trx_usrp_reset_stats;
        device->trx_end_func   = trx_usrp_end;
        device->trx_stop_func  = trx_usrp_stop;
        device->trx_set_freq_func = trx_usrp_set_freq;
        device->trx_set_gains_func   = trx_usrp_set_gains;
        device->openair0_cfg = openair0_cfg;

        s->sample_rate = openair0_cfg[0].sample_rate;
        // TODO:
        // init tx_forward_nsamps based usrp_time_offset ex
        if(is_equal(s->sample_rate, (double)30.72e6))
            s->tx_forward_nsamps  = 176;
        if(is_equal(s->sample_rate, (double)15.36e6))
            s->tx_forward_nsamps = 90;
        if(is_equal(s->sample_rate, (double)7.68e6))
            s->tx_forward_nsamps = 50;

        if (s->use_gps == 1) {
	   if (sync_to_gps(device)) {
		 LOG_I(PHY,"USRP fails to sync with GPS...\n");
            exit(0);   
           } 
        }
 
#if defined(USRP_REC_PLAY)
      }
#endif
#if defined(USRP_REC_PLAY)
      if (u_sf_mode == 1) { // record mode	
	ms_sample = (iqrec_t*) malloc(u_sf_max * sizeof(iqrec_t));
	if (ms_sample == NULL) {
	  std::cerr<< "Memory allocation failed for subframe record or replay mode." << std::endl;
	  exit(-1);
	}
	memset(ms_sample, 0, u_sf_max * BELL_LABS_IQ_BYTES_PER_SF);
      }
      if (u_sf_mode == 2) {
	// use mmap
	mmapfd = open(u_sf_filename, O_RDONLY | O_LARGEFILE);
	if (mmapfd != 0) {
	  fstat(mmapfd, &sb);
	  std::cerr << "Loading subframes using mmap() from " << u_sf_filename << " size=" << (uint64_t)sb.st_size << " bytes ..." << std::endl;
	  ms_sample = (iqrec_t*) mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, mmapfd, 0);
	  if (ms_sample != MAP_FAILED) {
	    nb_samples = (sb.st_size / sizeof(iqrec_t));
	    int aligned = (((unsigned long)ms_sample & 31) == 0)? 1:0;
	    std::cerr<< "Loaded "<< nb_samples << " subframes." << std::endl;
	    if (aligned == 0) {
	      std::cerr<< "mmap address is not 32 bytes aligned, exiting." << std::endl;
	      close(mmapfd);
	      exit(-1);
	    }
	  } else {
	    std::cerr << "Cannot mmap file, exiting." << std::endl;
	    close(mmapfd);
	    exit(-1);
	  }
	} else {
	    std::cerr << "Cannot open " << u_sf_filename << " , exiting." << std::endl;
	    exit(-1);
	}
      }
#endif	
        return 0;
    }
}
/*@}*/
