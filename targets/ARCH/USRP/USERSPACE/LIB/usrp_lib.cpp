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
#include <iostream>
#include <complex>
#include <fstream>
#include <cmath>

#include "common_lib.h"
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
typedef struct
{

  // --------------------------------
  // variables for USRP configuration
  // --------------------------------
  //! USRP device pointer
  uhd::usrp::multi_usrp::sptr usrp;
  //uhd::usrp::multi_usrp::sptr rx_usrp;
  
  //create a send streamer and a receive streamer
  //! USRP TX Stream
  uhd::tx_streamer::sptr tx_stream;
  //! USRP RX Stream
  uhd::rx_streamer::sptr rx_stream;

  //! USRP TX Metadata
  uhd::tx_metadata_t tx_md;
  //! USRP RX Metadata
  uhd::rx_metadata_t rx_md;

  //! USRP Timestamp Information
  uhd::time_spec_t tm_spec;

  //setup variables and allocate buffer
  //! USRP Metadata
  uhd::async_metadata_t async_md;

  //! Sampling rate
  double sample_rate;

  //! time offset between transmiter timestamp and receiver timestamp;
  double tdiff;

  //! TX forward samples. We use usrp_time_offset to get this value
  int tx_forward_nsamps; //166 for 20Mhz


  // --------------------------------
  // Debug and output control
  // --------------------------------
  //! Number of underflows
  int num_underflows;
  //! Number of overflows
  int num_overflows;
  
  //! Number of sequential errors
  int num_seq_errors;
  //! tx count
  int64_t tx_count;
  //! rx count
  int64_t rx_count;
  //! timestamp of RX packet
  openair0_timestamp rx_timestamp;

} usrp_state_t;

/*! \brief Called to start the USRP transceiver. Return 0 if OK, < 0 if error
    @param device pointer to the device structure specific to the RF hardware target
*/
static int trx_usrp_start(openair0_device *device)
{
  usrp_state_t *s = (usrp_state_t*)device->priv;

  // init recv and send streaming
  uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
  cmd.time_spec = s->usrp->get_time_now() + uhd::time_spec_t(0.05);
  cmd.stream_now = false; // start at constant delay
  s->rx_stream->issue_stream_cmd(cmd);

  s->tx_md.time_spec = cmd.time_spec + uhd::time_spec_t(1-(double)s->tx_forward_nsamps/s->sample_rate);
  s->tx_md.has_time_spec = true;
  s->tx_md.start_of_burst = true;
  s->tx_md.end_of_burst = false;


  s->rx_count = 0;
  s->tx_count = 0;
  s->rx_timestamp = 0;

  return 0;
}
/*! \brief Terminate operation of the USRP transceiver -- free all associated resources 
 * \param device the hardware to use
 */
static void trx_usrp_end(openair0_device *device)
{
  usrp_state_t *s = (usrp_state_t*)device->priv;

  s->rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);

  //send a mini EOB packet
  s->tx_md.end_of_burst = true;
  s->tx_stream->send("", 0, s->tx_md);
  s->tx_md.end_of_burst = false;
  
}

/*! \brief Called to send samples to the USRP RF target
      @param device pointer to the device structure specific to the RF hardware target
      @param timestamp The timestamp at whicch the first sample MUST be sent 
      @param buff Buffer which holds the samples
      @param nsamps number of samples to be sent
      @param antenna_id index of the antenna if the device has multiple anteannas
      @param flags flags must be set to TRUE if timestamp parameter needs to be applied
*/ 
static int trx_usrp_write(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags)
{
  usrp_state_t *s = (usrp_state_t*)device->priv;
  s->tx_md.time_spec = uhd::time_spec_t::from_ticks(timestamp, s->sample_rate);
  if(flags)
    s->tx_md.has_time_spec = true;
  else
    s->tx_md.has_time_spec = false;

  if (cc>1) {
    std::vector<void *> buff_ptrs;
    for (int i=0;i<cc;i++) buff_ptrs.push_back(buff[i]);
    s->tx_stream->send(buff_ptrs, nsamps, s->tx_md);
  }
  else
    s->tx_stream->send(buff[0], nsamps, s->tx_md);
  s->tx_md.start_of_burst = false;

  return 0;
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
static int trx_usrp_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int cc)
{
   usrp_state_t *s = (usrp_state_t*)device->priv;
   int samples_received=0,i,j;
   int nsamps2;  // aligned to upper 32 or 16 byte boundary
#if defined(__x86_64) || defined(__i386__)
#ifdef __AVX2__
   __m256i buff_tmp[2][nsamps>>3];
   nsamps2 = (nsamps+7)>>3;
#else
   __m128i buff_tmp[2][nsamps>>2];
   nsamps2 = (nsamps+3)>>2;
#endif
#elif defined(__arm__)
   int16x8_t buff_tmp[2][nsamps>>2];
   nsamps2 = (nsamps+3)>>2;
#endif


  if (device->type == USRP_B200_DEV) {  
    if (cc>1) {
    // receive multiple channels (e.g. RF A and RF B)
      std::vector<void *> buff_ptrs;
 
      for (int i=0;i<cc;i++) buff_ptrs.push_back(buff_tmp[i]);
      samples_received = s->rx_stream->recv(buff_ptrs, nsamps, s->rx_md);
    } else {
    // receive a single channel (e.g. from connector RF A)
      samples_received = s->rx_stream->recv(buff_tmp[0], nsamps, s->rx_md);
    }
   
  // bring RX data into 12 LSBs for softmodem RX
    for (int i=0;i<cc;i++) {
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
 
      for (int i=0;i<cc;i++) buff_ptrs.push_back(buff[i]);
      samples_received = s->rx_stream->recv(buff_ptrs, nsamps, s->rx_md);
    } else {
    // receive a single channel (e.g. from connector RF A)
      samples_received = s->rx_stream->recv(buff[0], nsamps, s->rx_md);
    }
  }

  if (samples_received < nsamps) {
    printf("[recv] received %d samples out of %d\n",samples_received,nsamps);
    
  }

  //handle the error code
  switch(s->rx_md.error_code){
  case uhd::rx_metadata_t::ERROR_CODE_NONE:
    break;
  case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
    printf("[recv] USRP RX OVERFLOW!\n");
    s->num_overflows++;
    break;
  case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
    printf("[recv] USRP RX TIMEOUT!\n");
    break;
  default:
    printf("[recv] Unexpected error on RX, Error code: 0x%x\n",s->rx_md.error_code);
    break;
  }
  s->rx_count += nsamps;
  s->rx_timestamp = s->rx_md.time_spec.to_ticks(s->sample_rate);
  *ptimestamp = s->rx_timestamp;

  return samples_received;
}

/*! \brief Get current timestamp of USRP
 * \param device the hardware to use
*/
openair0_timestamp get_usrp_time(openair0_device *device) 
{
 
  usrp_state_t *s = (usrp_state_t*)device->priv;
  
  return s->usrp->get_time_now().to_ticks(s->sample_rate);
} 

/*! \brief Compares two variables within precision
 * \param a first variable
 * \param b second variable
*/
static bool is_equal(double a, double b)
{
  return std::fabs(a-b) < std::numeric_limits<double>::epsilon();
}

/*! \brief Set frequencies (TX/RX)
 * \param device the hardware to use
 * \param openair0_cfg RF frontend parameters set by application
 * \param dummy dummy variable not used
 * \returns 0 in success 
 */
int trx_usrp_set_freq(openair0_device* device, openair0_config_t *openair0_cfg, int dummy) {

  usrp_state_t *s = (usrp_state_t*)device->priv;

  printf("Setting USRP TX Freq %f, RX Freq %f\n",openair0_cfg[0].tx_freq[0],openair0_cfg[0].rx_freq[0]);
  s->usrp->set_tx_freq(openair0_cfg[0].tx_freq[0]);
  s->usrp->set_rx_freq(openair0_cfg[0].rx_freq[0]);

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

  s->usrp->set_tx_gain(openair0_cfg[0].tx_gain[0]);
  ::uhd::gain_range_t gain_range = s->usrp->get_rx_gain_range(0);
  // limit to maximum gain
  if (openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0] > gain_range.stop()) {
    
    printf("RX Gain 0 too high, reduce by %f dB\n",
	   openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0] - gain_range.stop());	   
    exit(-1);
  }
  s->usrp->set_rx_gain(openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0]);
  printf("Setting USRP RX gain to %f (rx_gain %f,gain_range.stop() %f)\n", openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0],openair0_cfg[0].rx_gain[0],gain_range.stop());

  return(0);
}

/*! \brief Stop USRP
 * \param card refers to the hardware index to use
 */
int trx_usrp_stop(int card) {
  return(0);
}

/*! \brief USRPB210 RX calibration table */
rx_gain_calib_table_t calib_table_b210[] = {
  {3500000000.0,44.0},
  {2660000000.0,49.0},
  {2300000000.0,50.0},
  {1880000000.0,53.0},
  {816000000.0,58.0},
  {-1,0}};

/*! \brief USRPB210 RX calibration table */
rx_gain_calib_table_t calib_table_b210_38[] = {
  {3500000000.0,44.0},
  {2660000000.0,49.8},
  {2300000000.0,51.0},
  {1880000000.0,53.0},
  {816000000.0,57.0},
  {-1,0}};

/*! \brief USRPx310 RX calibration table */
rx_gain_calib_table_t calib_table_x310[] = {
  {3500000000.0,77.0},
  {2660000000.0,81.0},
  {2300000000.0,81.0},
  {1880000000.0,82.0},
  {816000000.0,85.0},
  {-1,0}};

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
      printf("unknown sampling rate %d\n",(int)openair0_cfg[0].sample_rate);
      exit(-1);
      break;
    }
  }
  while (openair0_cfg->rx_gain_calib_table[i].freq>0) {
    diff = fabs(openair0_cfg->rx_freq[chain_index] - openair0_cfg->rx_gain_calib_table[i].freq);
    printf("cal %d: freq %f, offset %f, diff %f\n",
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



extern "C" {
/*! \brief Initialize Openair USRP target. It returns 0 if OK
* \param device the hardware to use
* \param openair0_cfg RF frontend parameters set by application
*/
  int device_init(openair0_device* device, openair0_config_t *openair0_cfg) {
    
    uhd::set_thread_priority_safe(1.0);
    usrp_state_t *s = (usrp_state_t*)malloc(sizeof(usrp_state_t));
    memset(s, 0, sizeof(usrp_state_t));
    
    // Initialize USRP device


  std::string args = "type=b200";


  uhd::device_addrs_t device_adds = uhd::device::find(args);
  size_t i;
  
  int vers=0,subvers=0,subsubvers=0;
  int bw_gain_adjust=0;

  sscanf(uhd::get_version_string().c_str(),"%d.%d.%d",&vers,&subvers,&subsubvers);

  printf("Checking for USRPs : UHD %s (%d.%d.%d)\n",uhd::get_version_string().c_str(),vers,subvers,subsubvers);
  
  if(device_adds.size() == 0)
  {
    double usrp_master_clock = 184.32e6;

    std::string args = "type=x300";
    
    // workaround for an api problem, master clock has to be set with the constructor not via set_master_clock_rate
    args += boost::str(boost::format(",master_clock_rate=%f") % usrp_master_clock);
    
    uhd::device_addrs_t device_adds = uhd::device::find(args);

    if(device_adds.size() == 0)
    {
      std::cerr<<"No USRP Device Found. " << std::endl;
      free(s);
      return -1;

    }


    printf("Found USRP X300\n");
    s->usrp = uhd::usrp::multi_usrp::make(args);
    //  s->usrp->set_rx_subdev_spec(rx_subdev);
    //  s->usrp->set_tx_subdev_spec(tx_subdev);

    // lock mboard clocks
    s->usrp->set_clock_source("internal");
    
    //Setting device type to USRP X300/X310 
    device->type=USRP_X300_DEV;

    // this is not working yet, master clock has to be set via constructor
    // set master clock rate and sample rate for tx & rx for streaming
    //s->usrp->set_master_clock_rate(usrp_master_clock);

    openair0_cfg[0].rx_gain_calib_table = calib_table_x310;
    
    switch ((int)openair0_cfg[0].sample_rate) {
    case 30720000:
            // from usrp_time_offset
      openair0_cfg[0].samples_per_packet    = 2048;
      openair0_cfg[0].tx_sample_advance     = 15;
      openair0_cfg[0].tx_bw                 = 20e6;
      openair0_cfg[0].rx_bw                 = 20e6;
      openair0_cfg[0].tx_scheduling_advance = 8*openair0_cfg[0].samples_per_packet;
      break;
    case 15360000:
      openair0_cfg[0].samples_per_packet    = 2048;
      openair0_cfg[0].tx_sample_advance     = 45;
      openair0_cfg[0].tx_bw                 = 10e6;
      openair0_cfg[0].rx_bw                 = 10e6;
      openair0_cfg[0].tx_scheduling_advance = 5*openair0_cfg[0].samples_per_packet;
      break;
    case 7680000:
      openair0_cfg[0].samples_per_packet    = 1024;
      openair0_cfg[0].tx_sample_advance     = 50;
      openair0_cfg[0].tx_bw                 = 5e6;
      openair0_cfg[0].rx_bw                 = 5e6;
      openair0_cfg[0].tx_scheduling_advance = 5*openair0_cfg[0].samples_per_packet;
      break;
    case 1920000:
      openair0_cfg[0].samples_per_packet    = 256;
      openair0_cfg[0].tx_sample_advance     = 50;
      openair0_cfg[0].tx_bw                 = 1.25e6;
      openair0_cfg[0].rx_bw                 = 1.25e6;
      openair0_cfg[0].tx_scheduling_advance = 8*openair0_cfg[0].samples_per_packet;
      break;
    default:
      printf("Error: unknown sampling rate %f\n",openair0_cfg[0].sample_rate);
      exit(-1);
      break;
    }

  } else {
    printf("Found USRP B200");
    args += ",num_recv_frames=256" ; 
    s->usrp = uhd::usrp::multi_usrp::make(args);

    //  s->usrp->set_rx_subdev_spec(rx_subdev);
    //  s->usrp->set_tx_subdev_spec(tx_subdev);
    
    // do not explicitly set the clock to "internal", because this will disable the gpsdo
    //    // lock mboard clocks
    //    s->usrp->set_clock_source("internal");
    // set master clock rate and sample rate for tx & rx for streaming

    device->type = USRP_B200_DEV;


    if ((vers == 3) && (subvers == 9) && (subsubvers>=2)) {
      openair0_cfg[0].rx_gain_calib_table = calib_table_b210;
      bw_gain_adjust=0;
    }
    else {
      openair0_cfg[0].rx_gain_calib_table = calib_table_b210_38;
      bw_gain_adjust=1;
    }

    switch ((int)openair0_cfg[0].sample_rate) {
    case 30720000:
      s->usrp->set_master_clock_rate(30.72e6);
      openair0_cfg[0].samples_per_packet    = 4096;
      openair0_cfg[0].tx_sample_advance     = 115;
      openair0_cfg[0].tx_bw                 = 20e6;
      openair0_cfg[0].rx_bw                 = 20e6;
      openair0_cfg[0].tx_scheduling_advance = 11*openair0_cfg[0].samples_per_packet;
      break;
    case 23040000:
      s->usrp->set_master_clock_rate(23.04e6); //to be checked
      openair0_cfg[0].samples_per_packet    = 2048;
      openair0_cfg[0].tx_sample_advance     = 113;
      openair0_cfg[0].tx_bw                 = 20e6;
      openair0_cfg[0].rx_bw                 = 20e6;
      openair0_cfg[0].tx_scheduling_advance = 8*openair0_cfg[0].samples_per_packet;
      break;
    case 15360000:
      s->usrp->set_master_clock_rate(30.72e06);
      openair0_cfg[0].samples_per_packet    = 2048;
      openair0_cfg[0].tx_sample_advance     = 103; 
      openair0_cfg[0].tx_bw                 = 20e6;
      openair0_cfg[0].rx_bw                 = 20e6;
      openair0_cfg[0].tx_scheduling_advance = 10240;
      break;
    case 7680000:
      s->usrp->set_master_clock_rate(30.72e6);
      openair0_cfg[0].samples_per_packet    = 1024;
      openair0_cfg[0].tx_sample_advance     = 80;
      openair0_cfg[0].tx_bw                 = 20e6;
      openair0_cfg[0].rx_bw                 = 20e6;
      openair0_cfg[0].tx_scheduling_advance = 5*openair0_cfg[0].samples_per_packet;
      break;
    case 1920000:
      s->usrp->set_master_clock_rate(7.68e6);
      openair0_cfg[0].samples_per_packet    = 256;
      openair0_cfg[0].tx_sample_advance     = 40;
      openair0_cfg[0].tx_bw                 = 20e6;
      openair0_cfg[0].rx_bw                 = 20e6;
      openair0_cfg[0].tx_scheduling_advance = 8*openair0_cfg[0].samples_per_packet;
      break;
    default:
      printf("Error: unknown sampling rate %f\n",openair0_cfg[0].sample_rate);
      exit(-1);
      break;
    }
  }

  /* device specific */
  openair0_cfg[0].txlaunch_wait = 1;//manage when TX processing is triggered
  openair0_cfg[0].txlaunch_wait_slotcount = 1; //manage when TX processing is triggered
  openair0_cfg[0].iq_txshift = 4;//shift
  openair0_cfg[0].iq_rxrescale = 15;//rescale iqs
  
  for(i=0;i<s->usrp->get_rx_num_channels();i++) {
    if (i<openair0_cfg[0].rx_num_channels) {
      s->usrp->set_rx_rate(openair0_cfg[0].sample_rate,i);
      //s->usrp->set_rx_bandwidth(openair0_cfg[0].rx_bw,i);
      //printf("Setting rx freq/gain on channel %lu/%lu : BW %f (readback %f)\n",i,s->usrp->get_rx_num_channels(),openair0_cfg[0].rx_bw/1e6,s->usrp->get_rx_bandwidth(i)/1e6);
      s->usrp->set_rx_freq(openair0_cfg[0].rx_freq[i],i);
      set_rx_gain_offset(&openair0_cfg[0],i,bw_gain_adjust);

      ::uhd::gain_range_t gain_range = s->usrp->get_rx_gain_range(i);
      // limit to maximum gain
      if (openair0_cfg[0].rx_gain[i]-openair0_cfg[0].rx_gain_offset[i] > gain_range.stop()) {
	
        printf("RX Gain %lu too high, lower by %f dB\n",i,openair0_cfg[0].rx_gain[i]-openair0_cfg[0].rx_gain_offset[i] - gain_range.stop());
	exit(-1);
      }
      s->usrp->set_rx_gain(openair0_cfg[0].rx_gain[i]-openair0_cfg[0].rx_gain_offset[i],i);
      printf("RX Gain %lu %f (%f) => %f (max %f)\n",i,
	     openair0_cfg[0].rx_gain[i],openair0_cfg[0].rx_gain_offset[i],
	     openair0_cfg[0].rx_gain[i]-openair0_cfg[0].rx_gain_offset[i],gain_range.stop());
    }
  }
  for(i=0;i<s->usrp->get_tx_num_channels();i++) {
    if (i<openair0_cfg[0].tx_num_channels) {
      s->usrp->set_tx_rate(openair0_cfg[0].sample_rate,i);
      //s->usrp->set_tx_bandwidth(openair0_cfg[0].tx_bw,i);
      //printf("Setting tx freq/gain on channel %lu/%lu: BW %f (readback %f)\n",i,s->usrp->get_tx_num_channels(),openair0_cfg[0].tx_bw/1e6,s->usrp->get_tx_bandwidth(i)/1e6);
      s->usrp->set_tx_freq(openair0_cfg[0].tx_freq[i],i);
      s->usrp->set_tx_gain(openair0_cfg[0].tx_gain[i],i);
    }
  }


  // display USRP settings
  std::cout << boost::format("Actual master clock: %fMHz...") % (s->usrp->get_master_clock_rate()/1e6) << std::endl;
  
  sleep(1);

  // create tx & rx streamer
  uhd::stream_args_t stream_args_rx("sc16", "sc16");
  //stream_args_rx.args["spp"] = str(boost::format("%d") % 2048);//(openair0_cfg[0].rx_num_channels*openair0_cfg[0].samples_per_packet));
  for (i = 0; i<openair0_cfg[0].rx_num_channels; i++)
    stream_args_rx.channels.push_back(i);
  s->rx_stream = s->usrp->get_rx_stream(stream_args_rx);
  std::cout << boost::format("rx_max_num_samps %u") % (s->rx_stream->get_max_num_samps()) << std::endl;
  //openair0_cfg[0].samples_per_packet = s->rx_stream->get_max_num_samps();

  uhd::stream_args_t stream_args_tx("sc16", "sc16");
  //stream_args_tx.args["spp"] = str(boost::format("%d") % 2048);//(openair0_cfg[0].tx_num_channels*openair0_cfg[0].samples_per_packet));
  for (i = 0; i<openair0_cfg[0].tx_num_channels; i++)
      stream_args_tx.channels.push_back(i);
  s->tx_stream = s->usrp->get_tx_stream(stream_args_tx);
  std::cout << boost::format("tx_max_num_samps %u") % (s->tx_stream->get_max_num_samps()) << std::endl;


 /* Setting TX/RX BW after streamers are created due to USRP calibration issue */
  for(i=0;i<s->usrp->get_tx_num_channels();i++) {
    if (i<openair0_cfg[0].tx_num_channels) {
      s->usrp->set_tx_bandwidth(openair0_cfg[0].tx_bw,i);
      printf("Setting tx freq/gain on channel %lu/%lu: BW %f (readback %f)\n",i,s->usrp->get_tx_num_channels(),openair0_cfg[0].tx_bw/1e6,s->usrp->get_tx_bandwidth(i)/1e6);
    }
  }
  for(i=0;i<s->usrp->get_rx_num_channels();i++) {
    if (i<openair0_cfg[0].rx_num_channels) {
      s->usrp->set_rx_bandwidth(openair0_cfg[0].rx_bw,i);
      printf("Setting rx freq/gain on channel %lu/%lu : BW %f (readback %f)\n",i,s->usrp->get_rx_num_channels(),openair0_cfg[0].rx_bw/1e6,s->usrp->get_rx_bandwidth(i)/1e6);
    }
  }

  s->usrp->set_time_now(uhd::time_spec_t(0.0));
 

  for (i=0;i<openair0_cfg[0].rx_num_channels;i++) {
    if (i<openair0_cfg[0].rx_num_channels) {
      printf("RX Channel %lu\n",i);
      std::cout << boost::format("Actual RX sample rate: %fMSps...") % (s->usrp->get_rx_rate(i)/1e6) << std::endl;
      std::cout << boost::format("Actual RX frequency: %fGHz...") % (s->usrp->get_rx_freq(i)/1e9) << std::endl;
      std::cout << boost::format("Actual RX gain: %f...") % (s->usrp->get_rx_gain(i)) << std::endl;
      std::cout << boost::format("Actual RX bandwidth: %fM...") % (s->usrp->get_rx_bandwidth(i)/1e6) << std::endl;
      std::cout << boost::format("Actual RX antenna: %s...") % (s->usrp->get_rx_antenna(i)) << std::endl;
    }
  }

  for (i=0;i<openair0_cfg[0].tx_num_channels;i++) {

    if (i<openair0_cfg[0].tx_num_channels) { 
      printf("TX Channel %lu\n",i);
      std::cout << std::endl<<boost::format("Actual TX sample rate: %fMSps...") % (s->usrp->get_tx_rate(i)/1e6) << std::endl;
      std::cout << boost::format("Actual TX frequency: %fGHz...") % (s->usrp->get_tx_freq(i)/1e9) << std::endl;
      std::cout << boost::format("Actual TX gain: %f...") % (s->usrp->get_tx_gain(i)) << std::endl;
      std::cout << boost::format("Actual TX bandwidth: %fM...") % (s->usrp->get_tx_bandwidth(i)/1e6) << std::endl;
      std::cout << boost::format("Actual TX antenna: %s...") % (s->usrp->get_tx_antenna(i)) << std::endl;
    }
  }

  std::cout << boost::format("Device timestamp: %f...") % (s->usrp->get_time_now().get_real_secs()) << std::endl;

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
  
  s->sample_rate = openair0_cfg[0].sample_rate;
  // TODO:
  // init tx_forward_nsamps based usrp_time_offset ex
  if(is_equal(s->sample_rate, (double)30.72e6))
    s->tx_forward_nsamps  = 176;
  if(is_equal(s->sample_rate, (double)15.36e6))
    s->tx_forward_nsamps = 90;
  if(is_equal(s->sample_rate, (double)7.68e6))
    s->tx_forward_nsamps = 50;
  return 0;
  }
}
/*@}*/
