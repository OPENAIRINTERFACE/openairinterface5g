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
   OpenAirInterface Dev  : openair4g-devel@eurecom.fr
  
   Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/** usrp_lib.cpp
 *
 * Author: HongliangXU : hong-liang-xu@agilent.com
 */

#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/usrp/multi_usrp.hpp>
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

typedef struct
{

  // --------------------------------
  // variables for USRP configuration
  // --------------------------------
  uhd::usrp::multi_usrp::sptr usrp;
  //uhd::usrp::multi_usrp::sptr rx_usrp;
  
  //create a send streamer and a receive streamer
  uhd::tx_streamer::sptr tx_stream;
  uhd::rx_streamer::sptr rx_stream;

  uhd::tx_metadata_t tx_md;
  uhd::rx_metadata_t rx_md;

  uhd::time_spec_t tm_spec;
  //setup variables and allocate buffer
  uhd::async_metadata_t async_md;

  double sample_rate;
  // time offset between transmiter timestamp and receiver timestamp;
  double tdiff;
  // use usrp_time_offset to get this value
  int tx_forward_nsamps; //166 for 20Mhz


  // --------------------------------
  // Debug and output control
  // --------------------------------
  int num_underflows;
  int num_overflows;
  int num_seq_errors;

  int64_t tx_count;
  int64_t rx_count;
  openair0_timestamp rx_timestamp;

} usrp_state_t;


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

static void trx_usrp_end(openair0_device *device)
{
  usrp_state_t *s = (usrp_state_t*)device->priv;

  s->rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);

  //send a mini EOB packet
  s->tx_md.end_of_burst = true;
  s->tx_stream->send("", 0, s->tx_md);
  s->tx_md.end_of_burst = false;
  
}

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

openair0_timestamp get_usrp_time(openair0_device *device) 
{
 
  usrp_state_t *s = (usrp_state_t*)device->priv;
  
  return s->usrp->get_time_now().to_ticks(s->sample_rate);
} 

static bool is_equal(double a, double b)
{
  return std::fabs(a-b) < std::numeric_limits<double>::epsilon();
}

int trx_usrp_set_freq(openair0_device* device, openair0_config_t *openair0_cfg, int dummy) {

  usrp_state_t *s = (usrp_state_t*)device->priv;

  s->usrp->set_tx_freq(openair0_cfg[0].tx_freq[0]);
  s->usrp->set_rx_freq(openair0_cfg[0].rx_freq[0]);

  return(0);
  
}

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
  printf("Setting USRP RX gain to %f\n", openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0]);

  return(0);
}

int trx_usrp_stop(int card) {
  return(0);
}


rx_gain_calib_table_t calib_table[] = {
  {3500000000.0,46.0},
  {2660000000.0,53.0},
  {2300000000.0,54.0},
  {1880000000.0,55.0},
  {816000000.0,62.0},
  {-1,0}};

void set_rx_gain_offset(openair0_config_t *openair0_cfg, int chain_index) {

  int i=0;
  // loop through calibration table to find best adjustment factor for RX frequency
  double min_diff = 6e9,diff;
 
  while (calib_table[i].freq>0) {
    diff = fabs(openair0_cfg->rx_freq[chain_index] - calib_table[i].freq);
    printf("cal %d: freq %f, offset %f, diff %f\n",
	   i,calib_table[i].freq,calib_table[i].offset,diff);
    if (min_diff > diff) {
      min_diff = diff;
      openair0_cfg->rx_gain_offset[chain_index] = calib_table[i].offset;
    }
    i++;
  }
  
}


int trx_usrp_get_stats(openair0_device* device) {

  return(0);

}
int trx_usrp_reset_stats(openair0_device* device) {

  return(0);

}


int openair0_device_init(openair0_device* device, openair0_config_t *openair0_cfg)
{
  uhd::set_thread_priority_safe(1.0);
  usrp_state_t *s = (usrp_state_t*)malloc(sizeof(usrp_state_t));
  memset(s, 0, sizeof(usrp_state_t));

  // Initialize USRP device

  std::string args = "type=b200";


  uhd::device_addrs_t device_adds = uhd::device::find(args);
  size_t i;

  printf("Checking for USRPs\n");
  
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
    
    // this is not working yet, master clock has to be set via constructor
    // set master clock rate and sample rate for tx & rx for streaming
    //s->usrp->set_master_clock_rate(usrp_master_clock);
  } else {
    printf("Found USRP B200");
    s->usrp = uhd::usrp::multi_usrp::make(args);

    //  s->usrp->set_rx_subdev_spec(rx_subdev);
    //  s->usrp->set_tx_subdev_spec(tx_subdev);

// do not explicitly set the clock to "internal", because this will disable the gpsdo
//    // lock mboard clocks
//    s->usrp->set_clock_source("internal");
    // set master clock rate and sample rate for tx & rx for streaming
    s->usrp->set_master_clock_rate(30.72e6);
  }



  for(i=0;i<s->usrp->get_rx_num_channels();i++) {
    if (i<openair0_cfg[0].rx_num_channels) {
      s->usrp->set_rx_rate(openair0_cfg[0].sample_rate,i);
      s->usrp->set_rx_bandwidth(openair0_cfg[0].rx_bw,i);
      printf("Setting rx freq/gain on channel %lu/%lu\n",i,s->usrp->get_rx_num_channels());
      s->usrp->set_rx_freq(openair0_cfg[0].rx_freq[i],i);
      set_rx_gain_offset(&openair0_cfg[0],i);

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
      s->usrp->set_tx_bandwidth(openair0_cfg[0].tx_bw,i);
      printf("Setting tx freq/gain on channel %lu/%lu\n",i,s->usrp->get_tx_num_channels());
      s->usrp->set_tx_freq(openair0_cfg[0].tx_freq[i],i);
      s->usrp->set_tx_gain(openair0_cfg[0].tx_gain[i],i);
    }
  }


  // display USRP settings
  std::cout << boost::format("Actual master clock: %fMHz...") % (s->usrp->get_master_clock_rate()/1e6) << std::endl;

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
