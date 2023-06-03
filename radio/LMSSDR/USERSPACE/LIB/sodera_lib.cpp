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
#include "LMS_StreamBoard.h"

#include "openair1/PHY/sse_intrin.h"

using namespace std;

int num_devices=0;
/*These items configure the underlying asynch stream used by the the sync interface. 
 */

#define BUFFERSIZE 4096
#define BUFFERSCOUNT 32
typedef struct
{

  // --------------------------------
  // variables for SoDeRa configuration
  // --------------------------------

  LMScomms Port;
  Si5351C Si;
  LMS7002M lmsControl;
  LMS_StreamBoard *lmsStream;

  char buffers_rx[BUFFERSIZE*BUFFERSCOUNT];
  int handles[BUFFERSCOUNT];
  int current_handle;
  int samples_left_buffer;

  double sample_rate;
  // time offset between transmiter timestamp and receiver timestamp;
  double tdiff;

  int channelscount;
  // --------------------------------
  // Debug and output control
  // --------------------------------
  int num_underflows;
  int num_overflows;
  int num_seq_errors;

  int64_t tx_count;
  int64_t rx_count;
  openair0_timestamp rx_timestamp;

} sodera_t;

typedef struct {
  uint8_t reserved[8];
  uint64_t counter;
  char data[4080];
} StreamPacket_t;

sodera_t sodera_state;

enum STATUS {
  SUCCESS,
  FAILURE
};

STATUS SPI_write(LMScomms* dataPort, uint16_t address, uint16_t data)
{
  assert(dataPort != nullptr);
  LMScomms::GenericPacket ctrPkt;
  ctrPkt.cmd = CMD_BRDSPI_WR;
  ctrPkt.outBuffer.push_back((address >> 8) & 0xFF);
  ctrPkt.outBuffer.push_back(address & 0xFF);
  ctrPkt.outBuffer.push_back((data >> 8) & 0xFF);
  ctrPkt.outBuffer.push_back(data & 0xFF);
  dataPort->TransferPacket(ctrPkt);
  return ctrPkt.status == 1 ? SUCCESS : FAILURE;
}

uint16_t SPI_read(LMScomms* dataPort, uint16_t address)
{
  assert(dataPort != nullptr);
  LMScomms::GenericPacket ctrPkt;
  ctrPkt.cmd = CMD_BRDSPI_RD;
  ctrPkt.outBuffer.push_back((address >> 8) & 0xFF);
  ctrPkt.outBuffer.push_back(address & 0xFF);
  dataPort->TransferPacket(ctrPkt);
  if (ctrPkt.inBuffer.size() > 4)
    return ctrPkt.inBuffer[2] * 256 + ctrPkt.inBuffer[3];
  else
    return 0;
}

static int trx_sodera_start(openair0_device *device)
{
  sodera_t *s = (sodera_t*)device->priv;


  // init recv and send streaming

  printf("Starting LMS Streaming\n");
  s->rx_count = 0;
  s->tx_count = 0;
  s->rx_timestamp = 0;
  s->current_handle = 0;

  // switch off RX
  uint16_t regVal = SPI_read(&s->Port,0x0005);
  SPI_write(&s->Port,0x0005,regVal & ~0x6);






  // USB FIFO reset
  LMScomms::GenericPacket ctrPkt; 
  ctrPkt.cmd = CMD_USB_FIFO_RST;
  ctrPkt.outBuffer.push_back(0x01);
  s->Port.TransferPacket(ctrPkt);
  ctrPkt.outBuffer[0]=0x00;
  s->Port.TransferPacket(ctrPkt);
 
  regVal = SPI_read(&s->Port,0x0005);
  // provide timestamp, set streamTXEN, set TX/RX enable 
  SPI_write(&s->Port,0x0005,(regVal & ~0x20) | 0x6);


  if (s->channelscount==2) {
    SPI_write(&s->Port,0x0001,0x0003);
    SPI_write(&s->Port,0x0007,0x000A);
  }
  else {
    SPI_write(&s->Port,0x0001,0x0001);
    SPI_write(&s->Port,0x0007,0x0008);
  }


  for (int i=0; i< BUFFERSCOUNT ; i++) 
    s->handles[i] = s->Port.BeginDataReading(&s->buffers_rx[i*BUFFERSIZE],BUFFERSIZE);
  printf("Armed %d transfers\n",BUFFERSCOUNT);
  return 0;
}

static void trx_sodera_end(openair0_device *device)
{
  sodera_t *s = (sodera_t*)device->priv;


  // stop TX/RX if they were active
  uint16_t regVal = SPI_read(&s->Port,0x0005);
  SPI_write(&s->Port,0x0005,regVal & ~0x6);

}

static int trx_sodera_write(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags)
{
  sodera_t *s = (sodera_t*)device->priv;


  return 0;
}

#define DEBUG_READ 1

static int trx_sodera_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int cc)
{
   sodera_t *s = (sodera_t*)device->priv;
   int samples_received=0,i,j;
   int nsamps2;  // aligned to upper 32 or 16 byte boundary
   StreamPacket_t *p;
   int16_t sampleI,sampleQ;
   char *pktStart;
   int offset = 0;
   int num_p;
   int ind=0;
   int buffsize;
   int spp;
   int bufindex;

   // this assumes that each request is of size 4096 bytes (spp = 4080/4/channelscount)
   spp = sizeof(p->data)>>2; // spp = size of payload in samples  
   spp /= s->channelscount;

#ifdef DEBUG_READ
   printf("\nIn trx_read\n");
   printf("s->current_handle %d\n", s->current_handle);
   printf("s->samples_left_buffer %d\n",s->samples_left_buffer);
#endif
   // first get rid of remaining samples
   if (s->samples_left_buffer > 0) {
     buffsize = min(s->samples_left_buffer,nsamps);
     pktStart = ((StreamPacket_t*)&s->buffers_rx[s->current_handle*BUFFERSIZE])->data;
     pktStart += (spp-s->samples_left_buffer);
     const int stepSize = s->channelscount * 3;

     for (int b=0;b<buffsize<<2;b+=stepSize) {
       for (int ch=0;ch<s->channelscount;ch++) {
	 // I sample
	 sampleI = (pktStart[b + 1 + 3*ch]&0x0F)<<8;
	 sampleI |= (pktStart[b + 3*ch]&0xFF);
	 sampleI = (sampleI<<4)>>4;
	 // Q sample
	 sampleQ = (pktStart[b + 2 + 3*ch]&0x0F)<<8;
	 sampleQ |= (pktStart[b + 1 + 3*ch]&0xFF);
	 sampleQ = (sampleQ<<4)>>4;
	 ((uint32_t*)buff[ch])[ind] = ((uint32_t)sampleI) | (((uint32_t)sampleQ)<<16);
       }
       ind++;
     }
   }
   if (ind == nsamps) {
     s->samples_left_buffer -= nsamps;
     s->rx_count += nsamps;
     *ptimestamp = s->rx_timestamp;
     s->rx_timestamp+=nsamps;
      
     return(nsamps);
   }
   else {
     s->samples_left_buffer = 0;
     nsamps -= ind;
     samples_received = ind;
   }

   // This is for the left-over part => READ from USB



   num_p = nsamps / spp;
   if ((nsamps%spp) > 0)
     num_p++;
   s->samples_left_buffer = (num_p*spp)-nsamps;
  

#ifdef DEBUG_READ
   printf("num_p %d\n",num_p);
#endif
   const int stepSize = s->channelscount * 3;

   for (i=0;i<num_p;i++) {

     bufindex = (s->current_handle+i)&(BUFFERSCOUNT-1);
     if (s->Port.WaitForReading(s->handles[bufindex],1000) == false) {
       printf("[recv] Error: request %d samples (%d/%d) WaitForReading timed out\n",nsamps,bufindex,num_p);
       *ptimestamp = s->rx_timestamp;
       s->rx_timestamp+=samples_received;
       return(samples_received);
     }
     long bytesToRead=BUFFERSIZE;
     if (s->Port.FinishDataReading(&s->buffers_rx[bufindex*BUFFERSIZE],bytesToRead,s->handles[bufindex]) != BUFFERSIZE) {  
       printf("[recv] Error: request %d samples (%d/%d) WaitForReading timed out\n",nsamps,bufindex,num_p);
       *ptimestamp = s->rx_timestamp;
       s->rx_timestamp+=samples_received;
       return(samples_received);
     }
    
     p = (StreamPacket_t*)&s->buffers_rx[bufindex*BUFFERSIZE];
     // handle timestamp
     if ((i==0) & (ind==0)) { // grab the timestamp from HW
       *ptimestamp = p->counter;
       s->rx_timestamp = p->counter+nsamps; // for next time
#ifdef DEBUG_READ
       printf("RX timestamp %d\n",s->rx_timestamp);
#endif
     }
     else { // check the timestamp
       if (i==0) {
	 if ((s->rx_timestamp + ind) != p->counter) {
	   printf("Error, RX timestamp error, got %lu, should be %llu\n",p->counter,s->rx_timestamp+ind);
	   return(ind);
	 }
       }
       *ptimestamp = s->rx_timestamp;
       s->rx_timestamp+=nsamps;
     }
     pktStart = p->data;
     for (uint16_t b=0;b<sizeof(p->data);b+=stepSize) {
       for (int ch=0;ch < s->channelscount;ch++) {
	 // I sample
	 sampleI = (pktStart[b + 1 + 3*ch]&0x0F)<<8;
	 sampleI |= (pktStart[b + 3*ch]&0xFF);
	 sampleI = (sampleI<<4)>>4;
	 // Q sample
	 sampleQ = (pktStart[b + 2 + 3*ch]&0x0F)<<8;
	 sampleQ |= (pktStart[b + 1 + 3*ch]&0xFF);
	 sampleQ = (sampleQ<<4)>>4;
	 ((uint32_t*)buff[ch])[ind] = ((uint32_t)sampleI) | (((uint32_t)sampleQ)<<16);
       }
       ind++;	        
     }
     samples_received+=spp;
     // schedule a new transmission for this index
     s->handles[bufindex] = s->Port.BeginDataReading(&s->buffers_rx[bufindex*BUFFERSIZE],BUFFERSIZE);
     s->current_handle=(s->current_handle+1)&(BUFFERSCOUNT-1);
   }   

  //handle the error code

  s->rx_count += samples_received;
  //  s->rx_timestamp = s->rx_md.time_spec.to_ticks(s->sample_rate);

  return samples_received;
}



static bool is_equal(double a, double b)
{
  return fabs(a-b) < 1e-6;
}

int trx_sodera_set_freq(openair0_device* device, openair0_config_t *openair0_cfg, int dummy) {

  sodera_t *s = (sodera_t*)device->priv;

  //  s->usrp->set_tx_freq(openair0_cfg[0].tx_freq[0]);
  //  s->usrp->set_rx_freq(openair0_cfg[0].rx_freq[0]);

  return(0);
  
}

int openair0_set_rx_frequencies(openair0_device* device, openair0_config_t *openair0_cfg) {

  sodera_t *s = (sodera_t*)device->priv;
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

  sodera_t *s = (sodera_t*)device->priv;

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

  sodera_t *s=&sodera_state;

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

    s->lmsControl = LMS7002M(&s->Port);

    liblms7_status opStatus;
    s->lmsControl.ResetChip();
    opStatus = s->lmsControl.LoadConfig(openair0_cfg[0].configFilename);
    
    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Failed to load configuration file %s\n",openair0_cfg[0].configFilename);
      exit(-1);
    }
    opStatus = s->lmsControl.UploadAll();

    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Failed to upload configuration file\n");
      exit(-1);
    }
    
    opStatus = s->lmsControl.SetFrequencySX(LMS7002M::Tx, openair0_cfg[0].tx_freq[0]/1e6,30.72);

    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Cannot set TX frequency %f MHz\n",openair0_cfg[0].tx_freq[0]/1e6);
      exit(-1);
    }

    opStatus = s->lmsControl.SetFrequencySX(LMS7002M::Rx, openair0_cfg[0].rx_freq[0]/1e6,30.72);

    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Cannot set RX frequency %f MHz\n",openair0_cfg[0].rx_freq[0]/1e6);
      exit(-1);
    }


    
    // this makes RX/TX sampling rates equal
    opStatus = s->lmsControl.Modify_SPI_Reg_bits(EN_ADCCLKH_CLKGN,0);
    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Cannot modify SPI (EN_ADCCLKH_CLKGN)\n");
      exit(-1);
    }
    opStatus = s->lmsControl.Modify_SPI_Reg_bits(CLKH_OV_CLKL_CGEN,2);
    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Cannot modify SPI (CLKH_OV_CLKL_CGEN)\n");
      exit(-1);
    }

    const float cgen_freq_MHz = 245.76;
    const int interpolation   = 0; // real interpolation = 2
    const int decimation      = 0; // real decimation = 2
    opStatus = s->lmsControl.SetInterfaceFrequency(cgen_freq_MHz,interpolation,decimation);
    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Cannot SetInterfaceFrequency (%f,%d,%d)\n",cgen_freq_MHz,interpolation,decimation);
      exit(-1);
    }
    /*
    // Run calibration procedure
    float txrx_calibrationBandwidth_MHz = 5;
    opStatus = s->lmsControl.CalibrateTx(txrx_calibrationBandwidth_MHz);
    if (opStatus != LIBLMS7_SUCCESS){
      printf("TX Calibration failed\n");
      exit(-1);
    }
    opStatus = s->lmsControl.CalibrateRx(txrx_calibrationBandwidth_MHz);
    if (opStatus != LIBLMS7_SUCCESS){
      printf("RX Calibration failed\n");
      exit(-1);
    }
    */
        
    s->lmsStream = new LMS_StreamBoard(&s->Port);    
    LMS_StreamBoard::Status opStreamStatus; 
    // this will configure that sampling rate at output of FPGA
    opStreamStatus = s->lmsStream->ConfigurePLL(&s->Port,openair0_cfg[0].sample_rate,openair0_cfg[0].sample_rate,90);
    if (opStatus != LIBLMS7_SUCCESS){
      printf("Sample rate programming failed\n");
      exit(-1);
    }
    
    /*
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
  
  s->sample_rate   = openair0_cfg[0].sample_rate;
  s->channelscount = openair0_cfg[0].rx_num_channels;

  // TODO:
  return 0;
}
