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

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <errno.h>

#include <cmath>

/** @addtogroup _LMSSDR_PHY_RF_INTERFACE_
 * @{
 */
#include <lime/LimeSuite.h>
#include <lime/LMS7002M.h>
#include <lime/LMS7002M_RegistersMap.h>
#include "common_lib.h"

lms_device_t* lms_device;
lms_stream_t rx_stream;
lms_stream_t tx_stream;

/* We have a strange behavior when we just start reading
 * from the device (inconsistent values of the timestamp).
 * A quick solution is to discard the very first read packet
 * after a "start".
 * The following global variable "first_rx" serves that purpose.
 */
static int first_rx = 0;

#define RXDCLENGTH 4096
#define NUMBUFF 32

using namespace lime;

extern "C"
{
int write_output(const char *fname,const char *vname,void *data,int length,int dec,char format);
}

/*! \brief Called to send samples to the LMSSDR RF target
      \param device pointer to the device structure specific to the RF hardware target
      \param timestamp The timestamp at whicch the first sample MUST be sent
      \param buff Buffer which holds the samples
      \param nsamps number of samples to be sent
      \param antenna_id index of the antenna
      \param flags Ignored for the moment
      \returns 0 on success
*/
int trx_lms_write(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int antenna_id, int flags) {


    lms_stream_meta_t meta;
    meta.waitForTimestamp = true;
    meta.flushPartialPacket = false;
    meta.timestamp = timestamp;


    return LMS_SendStream(&tx_stream,(const void*)buff[0],nsamps,&meta,30);
}

/*! \brief Receive samples from hardware.
 * Read \ref nsamps samples from each channel to buffers. buff[0] is the array for
 * the first channel. *ptimestamp is the time at which the first sample
 * was received.
 * \param device the hardware to use
 * \param[out] ptimestamp the time at which the first sample was received.
 * \param[out] buff An array of pointers to buffers for received samples. The buffers must be large enough to hold the number of samples \ref nsamps.
 * \param nsamps Number of samples. One sample is 2 byte I + 2 byte Q => 4 byte.
 * \param antenna_id  Index of antenna port
 * \returns number of samples read
*/
int trx_lms_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int antenna_id) {

    lms_stream_meta_t meta;
    meta.waitForTimestamp = false;
    meta.flushPartialPacket = false;

    int ret;
    if (first_rx == 1) {
      first_rx = 0;
      ret = LMS_RecvStream(&rx_stream,buff[0],nsamps,&meta,50);
    }
    ret = LMS_RecvStream(&rx_stream,buff[0],nsamps,&meta,50);
    *ptimestamp = meta.timestamp;
    return ret;
}


/*! \brief set RX gain offset from calibration table
 * \param openair0_cfg RF frontend parameters set by application
 * \param chain_index RF chain ID
 */
void set_rx_gain_offset(openair0_config_t *openair0_cfg, int chain_index) {

  int i=0;
  // loop through calibration table to find best adjustment factor for RX frequency
  double min_diff = 6e9,diff;

  while (openair0_cfg->rx_gain_calib_table[i].freq>0) {
    diff = fabs(openair0_cfg->rx_freq[chain_index] - openair0_cfg->rx_gain_calib_table[i].freq);
    printf("cal %d: freq %f, offset %f, diff %f\n",
	   i,
	   openair0_cfg->rx_gain_calib_table[i].freq,
	   openair0_cfg->rx_gain_calib_table[i].offset,diff);
    if (min_diff > diff) {
      min_diff = diff;
      openair0_cfg->rx_gain_offset[chain_index] = openair0_cfg->rx_gain_calib_table[i].offset;
    }
    i++;
  }

}

/*! \brief Set Gains (TX/RX) on LMSSDR
 * \param device the hardware to use
 * \param openair0_cfg openair0 Config structure
 * \returns 0 in success, -1 on error
 */
int trx_lms_set_gains(openair0_device* device, openair0_config_t *openair0_cfg) {
  int ret = 0;

  if (openair0_cfg->rx_gain[0] > 70+openair0_cfg->rx_gain_offset[0]) {
    printf("[LMS] Reduce RX Gain 0 by %f dB\n",openair0_cfg->rx_gain[0]-openair0_cfg->rx_gain_offset[0]-70);
    ret = -1;
  }
  
  LMS_SetGaindB(lms_device, LMS_CH_TX, 0, openair0_cfg->tx_gain[0]);
  LMS_SetGaindB(lms_device, LMS_CH_RX, 0, openair0_cfg->rx_gain[0]-openair0_cfg->rx_gain_offset[0]); 

  return(ret);
}

/*! \brief Start LMSSDR
 * \param device the hardware to use
 * \returns 0 on success
 */
int trx_lms_start(openair0_device *device){

    lms_info_str_t list[16]={0};

    int n= LMS_GetDeviceList(list);

    if (n <= 0) {
        fprintf(stderr, "No LimeSDR board found: %s\n", n < 0?LMS_GetLastErrorMessage():"");
        return -1;
    }

    printf("Connecting to device: %s\n",list[0]);
    if (LMS_Open(&lms_device,list[0],NULL)<0) {
        fprintf(stderr, "Can't open device port: %s\n",LMS_GetLastErrorMessage());
        return -1;
    }

    LMS_Init(lms_device);
    LMS_EnableCalibCache(lms_device,false);

    if (LMS_LoadConfig(lms_device,device->openair0_cfg[0].configFilename) != 0)
    {
      printf("Failed to load configuration file %s\n%s",device->openair0_cfg[0].configFilename,LMS_GetLastErrorMessage());
      exit(-1);
    }

   /* LMS_EnableChannel(lms_device,LMS_CH_RX,0,true);
    LMS_EnableChannel(lms_device,LMS_CH_TX,0,true);
    if (device->openair0_cfg->rx_num_channels == 2)
    {
        LMS_EnableChannel(lms_device,LMS_CH_RX,1,true);
        LMS_EnableChannel(lms_device,LMS_CH_TX,1,true);
    } */
    LMS_VCTCXOWrite(lms_device,129);

    if (LMS_SetSampleRate(lms_device,device->openair0_cfg->sample_rate,2)!=0)
    {
        fprintf(stderr, "Failed to set sample rate %s\n",LMS_GetLastErrorMessage());
        return -1;
    }
    printf("Set sample rate %f MHz\n",device->openair0_cfg->sample_rate/1e6);

    if (LMS_SetLOFrequency(lms_device,LMS_CH_RX, 0, device->openair0_cfg[0].rx_freq[0])!=0)
    {
        fprintf(stderr, "Failed to Set Rx frequency: %s\n", LMS_GetLastErrorMessage());
        return -1;
    }
    if (LMS_SetLOFrequency(lms_device,LMS_CH_TX, 0,device->openair0_cfg[0].tx_freq[0])!=0)
    {
        fprintf(stderr, "Failed to Set Tx frequency: %s\n", LMS_GetLastErrorMessage());
        return -1;
    }
    printf("Set TX frequency %f MHz\n",device->openair0_cfg[0].tx_freq[0]/1e6);

    /*
    printf("Override antenna settings to: RX1_H, TXA_2");
    LMS_SetAntenna(lms_device, LMS_CH_RX, 0, 1);
    LMS_SetAntenna(lms_device, LMS_CH_TX, 0, 2);
    */

    
    for (int i = 0; i< device->openair0_cfg->rx_num_channels; i++)
    {
        if (LMS_SetLPFBW(lms_device,LMS_CH_RX,i,device->openair0_cfg->rx_bw)!=0)
            printf("RX ch:%d filter calibration failed, bw:%fMHz\n%s\n",i,device->openair0_cfg->rx_bw/1e6,LMS_GetLastErrorMessage());
        if (LMS_SetLPFBW(lms_device,LMS_CH_TX,i,device->openair0_cfg->tx_bw)!=0)
            printf("TX ch:%d filter calibration failed, bw:%fMHz\n%s\n",i,device->openair0_cfg->tx_bw/1e6,LMS_GetLastErrorMessage());

        if (LMS_Calibrate(lms_device,LMS_CH_RX,i,device->openair0_cfg->rx_bw,0)!=0)
            printf("RX ch:%d calibration failed, bw:%f MHz\n%s\n",i,device->openair0_cfg->rx_bw/1e6,LMS_GetLastErrorMessage());
        if (LMS_Calibrate(lms_device,LMS_CH_TX,i,device->openair0_cfg->tx_bw,0)!=0)
            printf("TX ch:%d calibration failed, bw:%fMHz\n%s\n",i,device->openair0_cfg->tx_bw/1e6,LMS_GetLastErrorMessage());
    }


    first_rx = 1;

    rx_stream.channel = 0;
    rx_stream.fifoSize = 256*1024;
    rx_stream.throughputVsLatency = 0.1;
    rx_stream.dataFmt = lms_stream_t::LMS_FMT_I12;
    rx_stream.isTx = false;
    if (LMS_SetupStream(lms_device, &rx_stream)!=0)
        printf("RX stream setup failed %s\n",LMS_GetLastErrorMessage());
    tx_stream.channel = 0;
    tx_stream.fifoSize = 256*1024;
    tx_stream.throughputVsLatency = 0.1;
    tx_stream.dataFmt = lms_stream_t::LMS_FMT_I12;
    tx_stream.isTx = true;

    trx_lms_set_gains(device, device->openair0_cfg);

    if (LMS_SetupStream(lms_device, &tx_stream)!=0)
        printf("TX stream setup failed %s\n",LMS_GetLastErrorMessage());

    printf("SR:   %.3f MHz\n", (float)device->openair0_cfg->sample_rate / 1e6);


    printf("SR:   %.3f MHz\n", (float)device->openair0_cfg->sample_rate / 1e6);

    if (LMS_StartStream(&rx_stream)!=0)
        printf("Failed to start TX stream %s\n",LMS_GetLastErrorMessage());
    if (LMS_StartStream(&tx_stream)!=0)
        printf("Failed to start Rx stream %s\n",LMS_GetLastErrorMessage());
    return 0;
}

/*! \brief Stop LMSSDR
 * \param card Index of the RF card to use
 * \returns 0 on success
 */
int trx_lms_stop(openair0_device *device) {
    LMS_StopStream(&rx_stream);
    LMS_StopStream(&tx_stream);
    LMS_DestroyStream(lms_device,&rx_stream);
    LMS_DestroyStream(lms_device,&tx_stream);
    LMS_Close(lms_device);
}

/*! \brief Set frequencies (TX/RX)
 * \param device the hardware to use
 * \param openair0_cfg openair0 Config structure (ignored. It is there to comply with RF common API)
 * \param exmimo_dump_config (ignored)
 * \returns 0 in success
 */
int trx_lms_set_freq(openair0_device* device, openair0_config_t *openair0_cfg,int exmimo_dump_config) {
  //Control port must be connected
  LMS_SetLOFrequency(lms_device,LMS_CH_TX,0,openair0_cfg->tx_freq[0]);
  LMS_SetLOFrequency(lms_device,LMS_CH_RX,0,openair0_cfg->rx_freq[0]);
  printf ("[LMS] rx frequency:%f;\n",openair0_cfg->rx_freq[0]/1e6);
  set_rx_gain_offset(openair0_cfg,0);
  return(0);

}

// 31 = 19 dB => 105 dB total gain @ 2.6 GHz
/*! \brief calibration table for LMSSDR */
// V1.2 board
rx_gain_calib_table_t calib_table_lmssdr_1v2[] = {
  {3500000000.0,44.0},  // on L PAD
  {2660000000.0,55.0},  // on L PAD
  {2300000000.0,54.0},  // on L PAD
  {1880000000.0,54.0},  // on L PAD
  {816000000.0,79.0},   // on W PAD
  {-1,0}};
// V1.4 board
rx_gain_calib_table_t calib_table_lmssdr[] = {
  {3500000000.0,44.0},  // on H PAD
  {2660000000.0,55.0},  // on H PAD
  {2300000000.0,54.0},  // on H PAD
  {1880000000.0,54.0},  // on H PAD
  {816000000.0,79.0},   // on L PAD
  {-1,0}};






/*! \brief Get LMSSDR Statistics
 * \param device the hardware to use
 * \returns 0 in success
 */

int trx_lms_get_stats(openair0_device* device) {

  return(0);

}

/*! \brief Reset LMSSDR Statistics
 * \param device the hardware to use
 * \returns 0 in success
 */
int trx_lms_reset_stats(openair0_device* device) {

  return(0);

}


/*! \brief Terminate operation of the LMSSDR transceiver -- free all associated resources
 * \param device the hardware to use
 */
void trx_lms_end(openair0_device *device) {


}

extern "C" {
/*! \brief Initialize Openair LMSSDR target. It returns 0 if OK
* \param device the hardware to use
* \param openair0_cfg RF frontend parameters set by application
*/
int device_init(openair0_device *device, openair0_config_t *openair0_cfg){

  device->type=LMSSDR_DEV;

  printf("LMSSDR: Initializing openair0_device for %s ...\n", ((device->host_type == RAU_HOST) ? "RAU": "RRU"));

  switch ((int)openair0_cfg[0].sample_rate) {
  case 30720000:
    // from usrp_time_offset
    openair0_cfg[0].samples_per_packet    = 2048;
    openair0_cfg[0].tx_sample_advance     = 40;
    openair0_cfg[0].tx_bw                 = 30.72e6;
    openair0_cfg[0].rx_bw                 = 30.72e6;
    break;
  case 15360000:
    openair0_cfg[0].samples_per_packet    = 2048;
    openair0_cfg[0].tx_sample_advance     = 50;   /* TODO: to be refined */
    openair0_cfg[0].tx_bw                 = 15.36e6;
    openair0_cfg[0].rx_bw                 = 15.36e6;
    break;
  case 7680000:
    openair0_cfg[0].samples_per_packet    = 1024;
    openair0_cfg[0].tx_sample_advance     = 50;
    openair0_cfg[0].tx_bw                 = 5.0e6;
    openair0_cfg[0].rx_bw                 = 5.0e6;
    break;
  case 1920000:
    openair0_cfg[0].samples_per_packet    = 256;
    openair0_cfg[0].tx_sample_advance     = 10;
    openair0_cfg[0].tx_bw                 = 1.25e6;
    openair0_cfg[0].rx_bw                 = 1.25e6;
    break;
  default:
    printf("Error: unknown sampling rate %f\n",openair0_cfg[0].sample_rate);
    exit(-1);
    break;
  }

  openair0_cfg[0].rx_gain_calib_table = calib_table_lmssdr;
  set_rx_gain_offset(openair0_cfg,0);

  device->Mod_id           = 1;
  device->trx_start_func   = trx_lms_start;
  device->trx_write_func   = trx_lms_write;
  device->trx_read_func    = trx_lms_read;
  device->trx_get_stats_func   = trx_lms_get_stats;
  device->trx_reset_stats_func = trx_lms_reset_stats;
  device->trx_end_func = trx_lms_end;
  device->trx_stop_func = trx_lms_stop;
  device->trx_set_freq_func = trx_lms_set_freq;
  device->trx_set_gains_func = trx_lms_set_gains;

  device->openair0_cfg = openair0_cfg;

  return 0;
}
}
/*@}*/
