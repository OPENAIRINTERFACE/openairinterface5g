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

/** sodera_lib.c
 *
 * Author: Raymond Knopp
 */
 
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


#include <iostream>
#include <complex>
#include <fstream>
#include <cmath>

#include "common_lib.h"

#include "lmsComms.h"
#include "LMS7002M.h"
#include "Si5351C.h"
#ifdef __SSE4_1__
#  include <smmintrin.h>
#endif
 
#ifdef __AVX2__
#  include <immintrin.h>
#endif

using namespace std;

int num_devices=0;
/*These items configure the underlying asynch stream used by the the sync interface. 
 */

typedef struct
{

  // --------------------------------
  // variables for SoDeRa configuration
  // --------------------------------
  /*
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
  */

  LMScomms Port;
  Si5351C Si;
  double sample_rate;
  // time offset between transmiter timestamp and receiver timestamp;
  double tdiff;

  // --------------------------------
  // Debug and output control
  // --------------------------------
  int num_underflows;
  int num_overflows;
  int num_seq_errors;

  int64_t tx_count;
  int64_t rx_count;
  openair0_timestamp rx_timestamp;

} sodera_state_t;

sodera_state_t sodera_state;

static int trx_sodera_start(openair0_device *device)
{
  sodera_state_t *s = (sodera_state_t*)device->priv;

  // init recv and send streaming

  s->rx_count = 0;
  s->tx_count = 0;
  s->rx_timestamp = 0;

  return 0;
}

static void trx_sodera_end(openair0_device *device)
{
  sodera_state_t *s = (sodera_state_t*)device->priv;


  
}

static int trx_sodera_write(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags)
{
  sodera_state_t *s = (sodera_state_t*)device->priv;

  if (cc>1) {
    //    s->tx_stream->send(buff_ptrs, nsamps, s->tx_md);
  }
  else
    //    s->tx_stream->send(buff[0], nsamps, s->tx_md);

  return 0;
}

static int trx_sodera_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int cc)
{
   sodera_state_t *s = (sodera_state_t*)device->priv;
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
     
   } else {
     // receive a single channel (e.g. from connector RF A)
     
   }
   
  if (samples_received < nsamps) {
    printf("[recv] received %d samples out of %d\n",samples_received,nsamps);
    
  }

  //handle the error code

  s->rx_count += nsamps;
  //  s->rx_timestamp = s->rx_md.time_spec.to_ticks(s->sample_rate);
  *ptimestamp = s->rx_timestamp;

  return samples_received;
}



static bool is_equal(double a, double b)
{
  return fabs(a-b) < 1e-6;
}

int trx_sodera_set_freq(openair0_device* device, openair0_config_t *openair0_cfg, int dummy) {

  sodera_state_t *s = (sodera_state_t*)device->priv;

  //  s->usrp->set_tx_freq(openair0_cfg[0].tx_freq[0]);
  //  s->usrp->set_rx_freq(openair0_cfg[0].rx_freq[0]);

  return(0);
  
}

int openair0_set_rx_frequencies(openair0_device* device, openair0_config_t *openair0_cfg) {

  sodera_state_t *s = (sodera_state_t*)device->priv;
  static int first_call=1;
  static double rf_freq,diff;

  //  uhd::tune_request_t rx_tune_req(openair0_cfg[0].rx_freq[0]);

  //  rx_tune_req.rf_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
  //  rx_tune_req.rf_freq = openair0_cfg[0].rx_freq[0];
  //  rf_freq=openair0_cfg[0].rx_freq[0];
  //  s->usrp->set_rx_freq(rx_tune_req);

  return(0);
  
}

int trx_sodera_set_gains(openair0_device* device, 
		       openair0_config_t *openair0_cfg) {

  sodera_state_t *s = (sodera_state_t*)device->priv;

  //  s->usrp->set_tx_gain(openair0_cfg[0].tx_gain[0]);
  //  ::uhd::gain_range_t gain_range = s->usrp->get_rx_gain_range(0);
  // limit to maximum gain
  /* if (openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0] > gain_range.stop()) {
    
    printf("RX Gain 0 too high, reduce by %f dB\n",
	   openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0] - gain_range.stop());	   
    exit(-1);
  }
  s->usrp->set_rx_gain(openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0]);
  printf("Setting SODERA RX gain to %f (rx_gain %f,gain_range.stop() %f)\n", openair0_cfg[0].rx_gain[0]-openair0_cfg[0].rx_gain_offset[0],openair0_cfg[0].rx_gain[0],gain_range.stop());
  */
  return(0);
}

int trx_sodera_stop(int card) {
  return(0);
}


rx_gain_calib_table_t calib_table_sodera[] = {
  {3500000000.0,44.0},
  {2660000000.0,49.0},
  {2300000000.0,50.0},
  {1880000000.0,53.0},
  {816000000.0,58.0},
  {-1,0}};

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


int trx_sodera_get_stats(openair0_device* device) {

  return(0);

}
int trx_sodera_reset_stats(openair0_device* device) {

  return(0);

}


int openair0_dev_init_sodera(openair0_device* device, openair0_config_t *openair0_cfg)
{

  sodera_state_t *s=&sodera_state;

  size_t i;

  // Initialize SODERA device
  s->Port.RefreshDeviceList();
  vector<string> deviceNames=s->Port.GetDeviceList();

  if (deviceNames.size() == 1) {
    if (s->Port.Open(0) != IConnection::SUCCESS) {
      printf("Cannot open SoDeRa\n");
      exit(-1);
    }
    LMSinfo devInfo = s->Port.GetInfo();
    printf("Device %s, HW: %d, FW: %d, Protocol %d\n",
	   GetDeviceName(devInfo.device),
	   (int)devInfo.hardware,
	   (int)devInfo.firmware,
	   (int)devInfo.protocol);
    LMS7002M lmsControl(&s->Port);

    printf("Configuring Si5351C\n");
    s->Si.Initialize(&s->Port);
    s->Si.SetPLL(0, 25000000, 0);
    s->Si.SetPLL(1, 25000000, 0);
    s->Si.SetClock(0, 27000000, true, false);
    s->Si.SetClock(1, 27000000, true, false);
    for (int i = 2; i < 8; ++i)
      s->Si.SetClock(i, 27000000, false, false);
    Si5351C::Status status = s->Si.ConfigureClocks();
    if (status != Si5351C::SUCCESS)
      {
	printf("Failed to configure Si5351C");
	exit(-1);
      }
    status = s->Si.UploadConfiguration();
    if (status != Si5351C::SUCCESS)
      printf("Failed to upload Si5351C configuration");
    

    printf("Configuring LMS7002\n");

    int bw_gain_adjust=0;

   
    openair0_cfg[0].rx_gain_calib_table = calib_table_sodera;

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
    liblms7_status opStatus;
    lmsControl.ResetChip();
    opStatus = lmsControl.LoadConfig(openair0_cfg[0].configFilename);
    
    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Failed to load configuration file %s\n",openair0_cfg[0].configFilename);
      exit(-1);
    }
    /*
    for(i=0;i<openair0_cfg[0].rx_num_channels;i++) {
      s->usrp->set_rx_rate(openair0_cfg[0].sample_rate,i);
      s->usrp->set_rx_bandwidth(openair0_cfg[0].rx_bw,i);
      printf("Setting rx freq/gain on channel %lu/%lu : BW %f (readback %f)\n",i,s->usrp->get_rx_num_channels(),openair0_cfg[0].rx_bw/1e6,s->usrp->get_rx_bandwidth(i)/1e6);
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
      s->usrp->set_tx_bandwidth(openair0_cfg[0].tx_bw,i);
      printf("Setting tx freq/gain on channel %lu/%lu: BW %f (readback %f)\n",i,s->usrp->get_tx_num_channels(),openair0_cfg[0].tx_bw/1e6,s->usrp->get_tx_bandwidth(i)/1e6);
      s->usrp->set_tx_freq(openair0_cfg[0].tx_freq[i],i);
      s->usrp->set_tx_gain(openair0_cfg[0].tx_gain[i],i);
    }
  }
  */

  // create tx & rx streamer

  //stream_args_rx.args["spp"] = str(boost::format("%d") % 2048);//(openair0_cfg[0].rx_num_channels*openair0_cfg[0].samples_per_packet));
  
  /*
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
  */
  }
  else {
    printf("Please connect SoDeRa\n");
    exit(-1);
  }

  device->priv = s;
  device->trx_start_func = trx_sodera_start;
  device->trx_write_func = trx_sodera_write;
  device->trx_read_func  = trx_sodera_read;
  device->trx_get_stats_func = trx_sodera_get_stats;
  device->trx_reset_stats_func = trx_sodera_reset_stats;
  device->trx_end_func   = trx_sodera_end;
  device->trx_stop_func  = trx_sodera_stop;
  device->trx_set_freq_func = trx_sodera_set_freq;
  device->trx_set_gains_func   = trx_sodera_set_gains;
  
  s->sample_rate = openair0_cfg[0].sample_rate;
  // TODO:

  return 0;
}
