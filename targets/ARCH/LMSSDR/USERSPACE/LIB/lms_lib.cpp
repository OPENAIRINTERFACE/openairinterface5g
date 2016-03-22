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

int trx_lms_write(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int antenna_id, int flags) {
  
 LMS_TRxWrite((int16_t*)buff[0], nsamps,0, timestamp);

 return nsamps;
}


int trx_lms_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int antenna_id) {
    
  uint64_t timestamp;
  int16_t *dst_ptr = (int16_t*) buff[0];
  int ret;
  ret = LMS_TRxRead(dst_ptr, nsamps,0,&timestamp, 10);
  *ptimestamp=timestamp;

  return ret;   
}
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
/*
void calibrate_rf(openair0_device *device) {

  openair0_timestamp ptimestamp;
  int16_t *calib_buffp,*calib_tx_buffp;
  int16_t calib_buff[2*RXDCLENGTH];
  int16_t calib_tx_buff[2*RXDCLENGTH];
  int i,j;
  int8_t offI,offQ,offIold,offQold,offInew,offQnew,offphase,offphaseold,offphasenew,offgain,offgainold,offgainnew;
  int32_t meanI,meanQ,meanIold,meanQold;
  int cnt=0,loop;
  liblms7_status opStatus;
  int16_t dcoffi;
  int16_t dcoffq;
  int16_t dccorri;
  int16_t dccorrq;
    const int16_t firCoefs[] =
    {
        -2531,
        -517,
        2708,
        188,
        -3059,
        216,
        3569,
        -770,
        -4199,
        1541,
        4886,
        -2577,
        -5552,
        3909,
        6108,
        -5537,
        -6457,
        7440,
        6507,
        -9566,
        -6174,
        11845,
        5391,
        -14179,
        -4110,
        16457,
        2310,
        -18561,
        0,
        20369,
        -2780,
        -21752,
        5963,
        22610,
        -9456,
        -22859,
        13127,
        22444,
        -16854,
        -21319,
        20489,
        19492,
        -23883,
        -17002,
        26881,
        13902,
        -29372,
        -10313,
        31226,
        6345,
        -32380,
        -2141,
        32767,
        -2141,
        -32380,
        6345,
        31226,
        -10313,
        -29372,
        13902,
        26881,
        -17002,
        -23883,
        19492,
        20489,
        -21319,
        -16854,
        22444,
        13127,
        -22859,
        -9456,
        22610,
        5963,
        -21752,
        -2780,
        20369,
        0,
        -18561,
        2310,
        16457,
        -4110,
        -14179,
        5391,
        11845,
        -6174,
        -9566,
        6507,
        7440,
        -6457,
        -5537,
        6108,
        3909,
        -5552,
        -2577,
        4886,
        1541,
        -4199,
        -770,
        3569,
        216,
        -3059,
        188,
        2708,
        -517,
        -2531
    };

  j=0;
  for (i=0;i<RXDCLENGTH;i++) {
    calib_tx_buff[j++] = cos_fsover8[i&7];
    calib_tx_buff[j++] = cos_fsover8[(i+6)&7];  // sin
  }
  calib_buffp = &calib_buff[0];
  calib_tx_buffp = &calib_tx_buff[0];

  lms7->BackupAllRegisters();
  uint8_t ch = (uint8_t)lms7->Get_SPI_Reg_bits(LMS7param(MAC));
  //Stage 1
  uint8_t sel_band1_trf = (uint8_t)lms7->Get_SPI_Reg_bits(LMS7param(SEL_BAND1_TRF));
  uint8_t sel_band2_trf = (uint8_t)lms7->Get_SPI_Reg_bits(LMS7param(SEL_BAND2_TRF));
  {
    uint16_t requiredRegs[] = { 0x0400, 0x040A, 0x010D, 0x040C };
    uint16_t requiredMask[] = { 0x6000, 0x3007, 0x0040, 0x00FF }; //CAPSEL, AGC_MODE, AGC_AVG, EN_DCOFF, Bypasses
    uint16_t requiredValue[] = { 0x0000, 0x1007, 0x0040, 0x00BD };
    
    lms7->Modify_SPI_Reg_mask(requiredRegs, requiredMask, requiredValue, 0, 3);
  }

  //  opStatus = lms7->SetFrequencySX(LMS7002M::Rx, device->openair0_cfg[0].tx_freq[0]/1e6,30.72);
  // put TX on fs/4
  opStatus = lms7->CalibrateRxSetup(device->openair0_cfg[0].sample_rate/1e6);
  if (opStatus != LIBLMS7_SUCCESS) {
    printf("Cannot calibrate for %f MHz\n",device->openair0_cfg[0].sample_rate/1e6);
    exit(-1);
  }
    // fill TX buffer with fs/8 complex sinusoid
  offIold=offQold=64;
  lms7->SetRxDCOFF(offIold,offQold);
  LMS_RxStart();  
  for (i=0;i<NUMBUFF;i++)
    trx_lms_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);

  for (meanIold=meanQold=i=j=0;i<RXDCLENGTH;i++) {
    meanIold+=calib_buff[j++];
    meanQold+=calib_buff[j++];
  }
  meanIold/=RXDCLENGTH;
  meanQold/=RXDCLENGTH;
  printf("[LMS] RX DC: (%d,%d) => (%d,%d)\n",offIold,offQold,meanIold,meanQold);

  offI=offQ=-64;
  lms7->SetRxDCOFF(offI,offQ);

  for (i=0;i<NUMBUFF;i++)
    trx_lms_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);

  for (meanI=meanQ=i=j=0;i<RXDCLENGTH;i++) {
    meanI+=calib_buff[j++];
    meanQ+=calib_buff[j++];
  }
  meanI/=RXDCLENGTH;
  meanQ/=RXDCLENGTH;
  printf("[LMS] RX DC: (%d,%d) => (%d,%d)\n",offI,offQ,meanI,meanQ);

  while (cnt++ < 7) {

    offInew=(offIold+offI)>>1;
    offQnew=(offQold+offQ)>>1;

    if (meanI*meanI < meanIold*meanIold) {
      meanIold = meanI;
      offIold = offI;
      printf("[LMS] *** RX DC: offI %d => %d\n",offIold,meanI);
    }
    if (meanQ*meanQ < meanQold*meanQold) {
      meanQold = meanQ;
      offQold = offQ;
      printf("[LMS] *** RX DC: offQ %d => %d\n",offQold,meanQ);
    }
    offI = offInew;
    offQ = offQnew;

    lms7->SetRxDCOFF(offI,offQ);

    for (i=0;i<NUMBUFF;i++)
      trx_lms_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);

    for (meanI=meanQ=i=j=0;i<RXDCLENGTH;i++) {
      meanI+=calib_buff[j++];
      meanQ+=calib_buff[j++];
    }
    meanI/=RXDCLENGTH;
    meanQ/=RXDCLENGTH;
    printf("[LMS] RX DC: (%d,%d) => (%d,%d)\n",offI,offQ,meanI,meanQ);
  }

  if (meanI*meanI < meanIold*meanIold) {
    meanIold = meanI;
    offIold = offI;
    printf("[LMS] *** RX DC: offI %d => %d\n",offIold,meanI);
  }
  if (meanQ*meanQ < meanQold*meanQold) {
    meanQold = meanQ;
    offQold = offQ;
    printf("[LMS] *** RX DC: offQ %d => %d\n",offQold,meanQ);
  }
  
  printf("[LMS] RX DC: (%d,%d) => (%d,%d)\n",offIold,offQold,meanIold,meanQold);
  
  lms7->SetRxDCOFF(offIold,offQold);

  dcoffi = offIold;
  dcoffq = offQold;

  lms7->Modify_SPI_Reg_bits(LMS7param(MAC), ch);
  lms7->Modify_SPI_Reg_bits(LMS7param(AGC_MODE_RXTSP), 1);
  lms7->Modify_SPI_Reg_bits(LMS7param(CAPSEL), 0);

  // TX LO leakage
  offQold=offIold=127;
  lms7->SPI_write(0x0204,(((int16_t)offIold)<<7)|offQold);

  {
    uint16_t requiredRegs[] = { 0x0400, 0x040A, 0x010D, 0x040C };
    uint16_t requiredMask[] = { 0x6000, 0x3007, 0x0040, 0x00FF }; //CAPSEL, AGC_MODE, AGC_AVG, EN_DCOFF, Bypasses
    uint16_t requiredValue[] = { 0x0000, 0x1007, 0x0040, 0x00BD };
    
    lms7->Modify_SPI_Reg_mask(requiredRegs, requiredMask, requiredValue, 0, 3);
  }
  sel_band1_trf = (uint8_t)lms7->Get_SPI_Reg_bits(LMS7param(SEL_BAND1_TRF));
  sel_band2_trf = (uint8_t)lms7->Get_SPI_Reg_bits(LMS7param(SEL_BAND2_TRF));
  //B
  lms7->Modify_SPI_Reg_bits(0x0100, 0, 0, 1); //EN_G_TRF 1
  if (sel_band1_trf == 1)
    {
      lms7->Modify_SPI_Reg_bits(LMS7param(PD_RLOOPB_1_RFE), 0); //PD_RLOOPB_1_RFE 0
      lms7->Modify_SPI_Reg_bits(LMS7param(EN_INSHSW_LB1_RFE), 0); //EN_INSHSW_LB1 0
    }
  if (sel_band2_trf == 1)
    {
      lms7->Modify_SPI_Reg_bits(LMS7param(PD_RLOOPB_2_RFE), 0); //PD_RLOOPB_2_RFE 0
      lms7->Modify_SPI_Reg_bits(LMS7param(EN_INSHSW_LB2_RFE), 0); // EN_INSHSW_LB2 0
    }
  //  FixRXSaturation();
  
  lms7->Modify_SPI_Reg_bits(LMS7param(GFIR3_BYP_RXTSP), 0); //GFIR3_BYP 0
  lms7->Modify_SPI_Reg_bits(LMS7param(HBD_OVR_RXTSP), 2);
  lms7->Modify_SPI_Reg_bits(LMS7param(GFIR3_L_RXTSP), 7);
  lms7->Modify_SPI_Reg_bits(LMS7param(GFIR3_N_RXTSP), 7);
  
  lms7->SetGFIRCoefficients(LMS7002M::Rx, 2, firCoefs, sizeof(firCoefs) / sizeof(int16_t));
    
  for (i=0;i<NUMBUFF;i++) {
    trx_lms_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
    trx_lms_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
  }

  write_output("calibrx.m","rxs",calib_buffp,RXDCLENGTH,1,1);
  exit(-1);
  for (meanIold=meanQold=i=j=0;i<RXDCLENGTH;i++) {
    switch (i&3) {
    case 0:
      meanIold+=calib_buff[j++];
      break;
    case 1:
      meanQold+=calib_buff[j++];
      break;
    case 2:
      meanIold-=calib_buff[j++];
      break;
    case 3:
      meanQold-=calib_buff[j++];
      break;
    }
  }
  //  meanIold/=RXDCLENGTH;
  //  meanQold/=RXDCLENGTH;
  printf("[LMS] TX DC (offI): %d => (%d,%d)\n",offIold,meanIold,meanQold);

  offI=-128;
  lms7->SPI_write(0x0204,(((int16_t)offI)<<7)|offQold);

  for (i=0;i<NUMBUFF;i++) {
    trx_lms_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
    trx_lms_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
  }

  for (meanI=meanQ=i=j=0;i<RXDCLENGTH;i++) {
    switch (i&3) {
    case 0:
      meanI+=calib_buff[j++];
      break;
    case 1:
      meanQ+=calib_buff[j++];
      break;
    case 2:
      meanI-=calib_buff[j++];
      break;
    case 3:
      meanQ-=calib_buff[j++];
      break;
    }
  }
  //  meanI/=RXDCLENGTH;
  //  meanQ/=RXDCLENGTH;
  printf("[LMS] TX DC (offI): %d => (%d,%d)\n",offI,meanI,meanQ);
  cnt = 0;
  while (cnt++ < 8) {

    offInew=(offIold+offI)>>1;
    if (meanI*meanI+meanQ*meanQ < meanIold*meanIold +meanQold*meanQold) {
      printf("[LMS] TX DC (offI): ([%d,%d]) => %d : %d\n",offIold,offI,offInew,meanI*meanI+meanQ*meanQ);
      meanIold = meanI;
      meanQold = meanQ;
      offIold = offI;
    }
    offI = offInew;
    lms7->SPI_write(0x0204,(((int16_t)offI)<<7)|offQold);

    for (i=0;i<NUMBUFF;i++) {
      trx_lms_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
      trx_lms_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
    }

    for (meanI=meanQ=i=j=0;i<RXDCLENGTH;i++) {
      switch (i&3) {
      case 0:
	meanI+=calib_buff[j++];
	break;
      case 1:
	meanQ+=calib_buff[j++];
	break;
      case 2:
	meanI-=calib_buff[j++];
	break;
      case 3:
	meanQ-=calib_buff[j++];
	break;
      }
    }
    //    meanI/=RXDCLENGTH;
    //   meanQ/=RXDCLENGTH;
    //    printf("[LMS] TX DC (offI): %d => (%d,%d)\n",offI,meanI,meanQ);
  }


  if (meanI*meanI+meanQ*meanQ < meanIold*meanIold +meanQold*meanQold) {
    printf("[LMS] TX DC (offI): ([%d,%d]) => %d : %d\n",offIold,offI,offInew,meanI*meanI+meanQ*meanQ);
    meanIold = meanI;
    meanQold = meanQ;
    offIold = offI;
  }
  offQ=-128;
  lms7->SPI_write(0x0204,(((int16_t)offIold)<<7)|offQ);

  for (i=0;i<NUMBUFF;i++) {
    trx_lms_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
    trx_lms_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
  }

  for (meanI=meanQ=i=j=0;i<RXDCLENGTH;i++) {
    switch (i&3) {
    case 0:
      meanI+=calib_buff[j++];
      break;
    case 1:
      meanQ+=calib_buff[j++];
      break;
    case 2:
      meanI-=calib_buff[j++];
      break;
    case 3:
      meanQ-=calib_buff[j++];
      break;
    }
  }
  //  meanI/=RXDCLENGTH;
  //  meanQ/=RXDCLENGTH;
  printf("[LMS] TX DC (offQ): %d => (%d,%d)\n",offQ,meanI,meanQ);

  cnt=0;
  while (cnt++ < 8) {

    offQnew=(offQold+offQ)>>1;
    if (meanI*meanI+meanQ*meanQ < meanIold*meanIold +meanQold*meanQold) {
      printf("[LMS] TX DC (offQ): ([%d,%d]) => %d : %d\n",offQold,offQ,offQnew,meanI*meanI+meanQ*meanQ);

      meanIold = meanI;
      meanQold = meanQ;
      offQold = offQ;
    }
    offQ = offQnew;
    lms7->SPI_write(0x0204,(((int16_t)offIold)<<7)|offQ);


    for (i=0;i<NUMBUFF;i++) {
      trx_lms_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
      trx_lms_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
    }

    for (meanI=meanQ=i=j=0;i<RXDCLENGTH;i++) {
      switch (i&3) {
      case 0:
	meanI+=calib_buff[j++];
	break;
      case 1:
	meanQ+=calib_buff[j++];
	break;
      case 2:
	meanI-=calib_buff[j++];
	break;
      case 3:
	meanQ-=calib_buff[j++];
	break;
      }
    }
    //    meanI/=RXDCLENGTH;
    //   meanQ/=RXDCLENGTH;
    //    printf("[LMS] TX DC (offQ): %d => (%d,%d)\n",offQ,meanI,meanQ);
  }

  LMS_RxStop();

  printf("[LMS] TX DC: (%d,%d) => (%d,%d)\n",offIold,offQold,meanIold,meanQold);

  dccorri = offIold;
  dccorrq = offQold;
  
  
  lms7->RestoreAllRegisters();
  lms7->Modify_SPI_Reg_bits(LMS7param(MAC), ch);

  lms7->Modify_SPI_Reg_bits(LMS7param(DCOFFI_RFE), dcoffi);
  lms7->Modify_SPI_Reg_bits(LMS7param(DCOFFQ_RFE), dcoffq);
  lms7->Modify_SPI_Reg_bits(LMS7param(DCCORRI_TXTSP), dccorri);
  lms7->Modify_SPI_Reg_bits(LMS7param(DCCORRQ_TXTSP), dccorrq);
  //  lms7->Modify_SPI_Reg_bits(LMS7param(GCORRI_TXTSP), gcorri);
  //  lms7->Modify_SPI_Reg_bits(LMS7param(GCORRQ_TXTSP), gcorrq);
  //  lms7->Modify_SPI_Reg_bits(LMS7param(IQCORR_TXTSP), iqcorr);
  
  //  lms7->Modify_SPI_Reg_bits(LMS7param(DC_BYP_TXTSP), 0); //DC_BYP
  lms7->Modify_SPI_Reg_bits(0x0208, 1, 0, 0); //GC_BYP PH_BYP
  
}
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


int trx_lms_stop(int card) {
  /*
  LMS_DeviceClose(usbport);
  LMS_DeviceClose(comport);
  delete lms7;
  return LMS_Destroy();
  */
}

int trx_lms_set_freq(openair0_device* device, openair0_config_t *openair0_cfg,int exmimo_dump_config) {
  //Control port must be connected 
   
  lms7->SetFrequencySX(LMS7002M::Tx,openair0_cfg->tx_freq[0]/1e6,30.72);
  lms7->SetFrequencySX(LMS7002M::Rx,openair0_cfg->rx_freq[0]/1e6,30.72);
  printf ("[LMS] rx frequency:%f;\n",openair0_cfg->rx_freq[0]/1e6);
  set_rx_gain_offset(openair0_cfg,0);
  return(0);
    
}

// 31 = 19 dB => 105 dB total gain @ 2.6 GHz
rx_gain_calib_table_t calib_table_sodera[] = {
  {3500000000.0,70.0},
  {2660000000.0,80.0},
  {2300000000.0,80.0},
  {1880000000.0,74.0},  // on W PAD
  {816000000.0,76.0},   // on W PAD
  {-1,0}};







int trx_lms_get_stats(openair0_device* device) {

  return(0);

}

int trx_lms_reset_stats(openair0_device* device) {

  return(0);

}

int openair0_set_gains(openair0_device* device, 
		       openair0_config_t *openair0_cfg) {

  return(0);
}

int openair0_set_frequencies(openair0_device* device, openair0_config_t *openair0_cfg, int dummy) {

  return(0);
}



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
