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

#include "common_lib.h"
#include "LMS_SDR.h"
#include "LMS7002M.h"
#include "Si5351C.h"
#include "LMS_StreamBoard.h"
#include "LMS7002M_RegistersMap.h"

#include <cmath>

/** @addtogroup _LMSSDR_PHY_RF_INTERFACE_
 * @{
 */

///define for parameter enumeration if prefix might be needed
#define LMS7param(id) id

LMScomms* usbport;
LMScomms* comport;
LMS7002M* lms7;
Si5351C* Si;
LMS_StreamBoard *lmsStream;

#define RXDCLENGTH 4096
#define NUMBUFF 40
int16_t cos_fsover8[8]  = {2047,   1447,      0,  -1448,  -2047,  -1448,     0,   1447};
int16_t cos_3fsover8[8] = {2047,  -1448,      0,   1447,  -2047,   1447,     0,  -1448};

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
  
 LMS_TRxWrite((int16_t*)buff[0], nsamps,0, timestamp);

 return nsamps;
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
    
  uint64_t timestamp;
  int16_t *dst_ptr = (int16_t*) buff[0];
  int ret;
  ret = LMS_TRxRead(dst_ptr, nsamps,0,&timestamp, 10);
  *ptimestamp=timestamp;

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
 * \returns 0 in success 
 */

int trx_lms_set_gains(openair0_device* device, openair0_config_t *openair0_cfg) {

  double gv = openair0_cfg[0].rx_gain[0] - openair0_cfg[0].rx_gain_offset[0];

  if (gv > 31) {
    printf("RX Gain 0 too high, reduce by %f dB\n",gv-31);
    gv = 31;
  }
  if (gv < 0) {
    printf("RX Gain 0 too low, increase by %f dB\n",-gv);
    gv = 0;
  }
  printf("[LMS] Setting 7002M G_PGA_RBB to %d\n", (int16_t)gv);
  lms7->Modify_SPI_Reg_bits(LMS7param(G_PGA_RBB),(int16_t)gv);

  return(0);
}

/*! \brief Start LMSSDR
 * \param device the hardware to use 
 * \returns 0 on success
 */
int trx_lms_start(openair0_device *device){
 

 LMS_Init(0, 128*1024);   
 usbport = LMS_GetUSBPort();
 //connect data stream port
  LMS_UpdateDeviceList(usbport);
  const char *name = LMS_GetDeviceName(usbport, 0);
  printf("Connecting to device: %s\n",name);



  if (LMS_DeviceOpen(usbport, 0)==0)
  {
    Si = new Si5351C();
    lms7 = new LMS7002M(usbport);
    liblms7_status opStatus;

    printf("Configuring Si5351C\n");
    Si->Initialize(usbport);
    Si->SetPLL(0, 25000000, 0);
    Si->SetPLL(1, 25000000, 0);
    Si->SetClock(0, 27000000, true, false);
    Si->SetClock(1, 27000000, true, false);
    for (int i = 2; i < 8; ++i)
      Si->SetClock(i, 27000000, false, false);
    Si5351C::Status status = Si->ConfigureClocks();
    if (status != Si5351C::SUCCESS)
      {
	printf("Failed to configure Si5351C");
	exit(-1);
      }
    status = Si->UploadConfiguration();
    if (status != Si5351C::SUCCESS)
      printf("Failed to upload Si5351C configuration");
    


    lms7->ResetChip();

    opStatus = lms7->LoadConfig(device->openair0_cfg[0].configFilename);
    
    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Failed to load configuration file %s\n",device->openair0_cfg[0].configFilename);
      exit(-1);
    }
    opStatus = lms7->UploadAll();

    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Failed to upload configuration file\n");
      exit(-1);
    }

    // Set TX filter
    
    printf("Tuning TX filter\n");
    opStatus = lms7->TuneTxFilter(LMS7002M::TxFilter::TX_HIGHBAND,device->openair0_cfg[0].tx_bw/1e6);

    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Warning: Could not tune TX filter to %f MHz\n",device->openair0_cfg[0].tx_bw/1e6);
    }
    
    printf("Tuning RX filter\n");
    
    opStatus = lms7->TuneRxFilter(LMS7002M::RxFilter::RX_LPF_LOWBAND,device->openair0_cfg[0].rx_bw/1e6);

    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Warning: Could not tune RX filter to %f MHz\n",device->openair0_cfg[0].rx_bw/1e6);
    }

    /*    printf("Tuning TIA filter\n");
    opStatus = lms7->TuneRxFilter(LMS7002M::RxFilter::RX_TIA,7.0);

    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Warning: Could not tune RX TIA filter\n");
      }*/

    opStatus = lms7->SetInterfaceFrequency(lms7->GetFrequencyCGEN_MHz(), 
					   lms7->Get_SPI_Reg_bits(HBI_OVR_TXTSP), 
					   lms7->Get_SPI_Reg_bits(HBD_OVR_RXTSP));
    if (opStatus != LIBLMS7_SUCCESS) {
      printf("SetInterfaceFrequency failed: %f,%d,%d\n",
	     lms7->GetFrequencyCGEN_MHz(), 
	     lms7->Get_SPI_Reg_bits(HBI_OVR_TXTSP), 
	     lms7->Get_SPI_Reg_bits(HBD_OVR_RXTSP));
    }
    else {
      printf("SetInterfaceFrequency as %f,%d,%d\n",
	     lms7->GetFrequencyCGEN_MHz(), 
	     lms7->Get_SPI_Reg_bits(HBI_OVR_TXTSP), 
	     lms7->Get_SPI_Reg_bits(HBD_OVR_RXTSP));
    } 
    lmsStream = new LMS_StreamBoard(usbport);    
    LMS_StreamBoard::Status opStreamStatus; 
    // this will configure that sampling rate at output of FPGA
    opStreamStatus = lmsStream->ConfigurePLL(usbport,
					     device->openair0_cfg[0].sample_rate,
					     device->openair0_cfg[0].sample_rate,90);
    if (opStatus != LIBLMS7_SUCCESS){
      printf("Sample rate programming failed\n");
      exit(-1);
    }

    opStatus = lms7->SetFrequencySX(LMS7002M::Tx, device->openair0_cfg[0].tx_freq[0]/1e6,30.72);

    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Cannot set TX frequency %f MHz\n",device->openair0_cfg[0].tx_freq[0]/1e6);
      exit(-1);
    }
    else {
      printf("Set TX frequency %f MHz\n",device->openair0_cfg[0].tx_freq[0]/1e6);
    }
    opStatus = lms7->SetFrequencySX(LMS7002M::Rx, device->openair0_cfg[0].rx_freq[0]/1e6,30.72);

    if (opStatus != LIBLMS7_SUCCESS) {
      printf("Cannot set RX frequency %f MHz\n",device->openair0_cfg[0].rx_freq[0]/1e6);
      exit(-1);
    }
    else {
      printf("Set RX frequency %f MHz\n",device->openair0_cfg[0].rx_freq[0]/1e6);
    }

    trx_lms_set_gains(device, device->openair0_cfg);
    // Run calibration procedure
    //    calibrate_rf(device);
    //lms7->CalibrateTx(5.0);
    LMS_RxStart();
  }
  else
  {
   return(-1);
  }
  
 //connect control port 

 /* comport = LMS_GetCOMPort();
  LMS_UpdateDeviceList(comport);
  name = LMS_GetDeviceName(comport, 0);
  if (*name == 0)
      comport = usbport;  //attempt to use data port 
  else
  {
    printf("Connecting to device: %s\n",name);
    if (LMS_DeviceOpen(comport, 0)!=0)
        return (-1);
  }
   lms7 = new LMS7002M(comport);
   if( access( "./config.ini", F_OK ) != -1 ) //load config file
   lms7->LoadConfig("./config.ini");
   //calibration takes too long
   //lms7->CalibrateRx(5.0);  
   //lms7->CalibrateTx(5.0);
 */

  return 0;
}

/*! \brief Stop LMSSDR
 * \param card Index of the RF card to use 
 * \returns 0 on success
 */
int trx_lms_stop(int card) {
  /*
  LMS_DeviceClose(usbport);
  LMS_DeviceClose(comport);
  delete lms7;
  return LMS_Destroy();
  */
}

/*! \brief Set frequencies (TX/RX)
 * \param device the hardware to use
 * \param openair0_cfg openair0 Config structure (ignored. It is there to comply with RF common API)
 * \param exmimo_dump_config (ignored)
 * \returns 0 in success 
 */
int trx_lms_set_freq(openair0_device* device, openair0_config_t *openair0_cfg,int exmimo_dump_config) {
  //Control port must be connected 
   
  lms7->SetFrequencySX(LMS7002M::Tx,openair0_cfg->tx_freq[0]/1e6,30.72);
  lms7->SetFrequencySX(LMS7002M::Rx,openair0_cfg->rx_freq[0]/1e6,30.72);
  printf ("[LMS] rx frequency:%f;\n",openair0_cfg->rx_freq[0]/1e6);
  set_rx_gain_offset(openair0_cfg,0);
  return(0);
    
}

// 31 = 19 dB => 105 dB total gain @ 2.6 GHz
/*! \brief calibration table for LMSSDR */
rx_gain_calib_table_t calib_table_sodera[] = {
  {3500000000.0,70.0},
  {2660000000.0,80.0},
  {2300000000.0,80.0},
  {1880000000.0,74.0},  // on W PAD
  {816000000.0,76.0},   // on W PAD
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
  printf("LMSSDR: Initializing openair0_device for %s ...\n", ((device->host_type == BBU_HOST) ? "BBU": "RRH"));

  switch ((int)openair0_cfg[0].sample_rate) {
  case 30720000:
    // from usrp_time_offset
    openair0_cfg[0].samples_per_packet    = 2048;
    openair0_cfg[0].tx_sample_advance     = 15;
    openair0_cfg[0].tx_bw                 = 30.72e6;
    openair0_cfg[0].rx_bw                 = 30.72e6;
    openair0_cfg[0].tx_scheduling_advance = 8*openair0_cfg[0].samples_per_packet;
    break;
  case 15360000:
    openair0_cfg[0].samples_per_packet    = 2048;
    openair0_cfg[0].tx_sample_advance     = 45;
    openair0_cfg[0].tx_bw                 = 28e6;
    openair0_cfg[0].rx_bw                 = 10e6;
    openair0_cfg[0].tx_scheduling_advance = 8*openair0_cfg[0].samples_per_packet;
    break;
  case 7680000:
    openair0_cfg[0].samples_per_packet    = 1024;
    openair0_cfg[0].tx_sample_advance     = 70;
    openair0_cfg[0].tx_bw                 = 28e6;
    openair0_cfg[0].rx_bw                 = 5.0e6;
    openair0_cfg[0].tx_scheduling_advance = 8*openair0_cfg[0].samples_per_packet;
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

  openair0_cfg[0].rx_gain_calib_table = calib_table_sodera;
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
