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

/** bladerf_lib.c
 *
 * Author: navid nikaein
 */


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "bladerf_lib.h"

int num_devices=0;
/*These items configure the underlying asynch stream used by the the sync interface. 
 */

int trx_brf_init(openair0_device *openair0) {
  
}

 
openair0_timestamp trx_get_timestamp(openair0_device *device) {
  int status;
  struct bladerf_metadata meta;
  brf_state_t *brf = (brf_state_t*)device->priv;
  
  if ((status=bladerf_get_timestamp(brf->dev, BLADERF_MODULE_TX, &meta.timestamp)) != 0) {
    fprintf(stderr,"Failed to get current RX timestamp: %s\n",bladerf_strerror(status));
  } else {
    printf("Current TX timestampe  0x%016"PRIx64"\n", meta.timestamp);
  }
  return meta.timestamp;
}


int trx_brf_start(openair0_device *openair0) {

  return 0;
}

static void trx_brf_stats(openair0_device *device){


}

static int trx_brf_write(openair0_device *device,openair0_timestamp ptimestamp, void **buff, int nsamps, int cc) {
  
  int status, i;
  brf_state_t *brf = (brf_state_t*)device->priv;
  /* BRF has only 1 rx/tx chaine : is it correct? */
  void *samples = (void*)buff[0];
  
  //brf->meta_tx.flags &= ~BLADERF_META_FLAG_TX_NOW;
  brf->meta_tx.flags = BLADERF_META_FLAG_TX_BURST_START | 
    BLADERF_META_FLAG_TX_NOW | 
                       BLADERF_META_FLAG_TX_BURST_END;

  brf->meta_tx.timestamp= (uint64_t) ptimestamp;

  status = bladerf_sync_tx(brf->dev, samples, (unsigned int) nsamps, &brf->meta_tx, 2*brf->tx_timeout_ms);
  
  if (status != 0) {
    fprintf(stderr,"Failed to TX sample: %s\n", bladerf_strerror(status));
    brf->num_tx_errors++;
    brf_error(status);
  } else if (brf->meta_tx.status & BLADERF_META_STATUS_UNDERRUN){
    /* libbladeRF does not report this status. It is here for future use. */ 
    fprintf(stderr, "TX Underrun detected. %u valid samples were read.\n",  brf->meta_tx.actual_count);
    brf->num_underflows++;
  } 
  //    printf("tx status %d \n",brf->meta_tx.status);
  brf->tx_current_ts=brf->meta_tx.timestamp;
  brf->tx_actual_nsamps+=brf->meta_tx.actual_count;
  brf->tx_nsamps+=nsamps;
  brf->tx_count++;
  

  return(0);
}

static int trx_brf_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int cc) {
  int status, ret;
  
  unsigned int i;
  brf_state_t *brf = (brf_state_t*)device->priv;
  
  // BRF has only one rx/tx chain
  void *samples = (void*)buff[0];
 
  brf->meta_rx.flags |= BLADERF_META_FLAG_RX_NOW;
  status = bladerf_sync_rx(brf->dev, samples, (unsigned int) nsamps, &brf->meta_rx, 2*brf->rx_timeout_ms);
  
  if (status != 0) {
    fprintf(stderr, "RX failed: %s\n", bladerf_strerror(status)); 
    brf->num_rx_errors++;
  } else if ( brf->meta_rx.status & BLADERF_META_STATUS_OVERRUN) {
    brf->num_overflows++;
    fprintf(stderr, "RX overrun (%d) is detected. t=0x%"PRIu64". Got %u samples. nsymps %d\n", 
	    brf->num_overflows,brf->meta_rx.timestamp,  brf->meta_rx.actual_count, nsamps);
    //brf->meta_rx.timestamp=(unsigned int)(nsamps-brf->meta_rx.actual_count);
  }
  //printf("[BRF] (buff %p) ts=0x%"PRIu64" %s\n",samples, brf->meta_rx.timestamp,bladerf_strerror(status));
  brf->rx_current_ts=brf->meta_rx.timestamp;
  brf->rx_actual_nsamps+=brf->meta_rx.actual_count;
  brf->rx_nsamps+=nsamps;
  brf->rx_count++;
  
  
  *ptimestamp = brf->meta_rx.timestamp;
  
  return brf->meta_rx.actual_count;

}

int trx_brf_end(openair0_device *device) {

  int status;
  brf_state_t *brf = (brf_state_t*)device->priv;
  // Disable RX module, shutting down our underlying RX stream
  if ((status=bladerf_enable_module(brf->dev, BLADERF_MODULE_RX, false))  != 0) {
    fprintf(stderr, "Failed to disable RX module: %s\n", bladerf_strerror(status));
  }
  if ((status=bladerf_enable_module(brf->dev, BLADERF_MODULE_TX, false))  != 0) {
    fprintf(stderr, "Failed to disable TX module: %s\n",  bladerf_strerror(status));
  }
  bladerf_close(brf->dev);
  return 0;
}


int trx_brf_get_stats(openair0_device* device) {

  return(0);

}

int trx_brf_reset_stats(openair0_device* device) {

  return(0);

}

int trx_brf_stop(openair0_device* device) {

  return(0);

}

int trx_brf_set_freq(openair0_device* device) {

  return(0);

}
int trx_brf_set_gains(openair0_device* device) {

  return(0);

}

int openair0_dev_init_bladerf(openair0_device *device, openair0_config_t *openair0_cfg) {

  int status;
  int card=0;
  
  brf_state_t *brf = (brf_state_t*)malloc(sizeof(brf_state_t));
  memset(brf, 0, sizeof(brf_state_t));
  // init required params for BRF
   brf->num_buffers   = 128;
   brf->buffer_size   = (unsigned int) openair0_cfg[card].samples_per_packet*sizeof(int32_t); // buffer size = 4096 for sample_len of 1024
   brf->num_transfers = 16;
   brf->rx_timeout_ms = 0;  
   brf->tx_timeout_ms = 0;
   brf->sample_rate=(unsigned int)openair0_cfg[card].sample_rate;

   
   printf("\n[BRF] sampling_rate %d, num_buffers %d,  buffer_size %d, num transfer %d, timeout_ms (rx %d, tx %d)\n", 
	  brf->sample_rate, brf->num_buffers, brf->buffer_size,brf->num_transfers, brf->rx_timeout_ms, brf->tx_timeout_ms);

  if ((status=bladerf_open(&brf->dev, "")) != 0 ) {
    fprintf(stderr,"Failed to open brf device: %s\n",bladerf_strerror(status));
    brf_error(status);
  }
  printf("[BRF] init dev %p\n", brf->dev);
  switch(bladerf_device_speed(brf->dev)){
  case BLADERF_DEVICE_SPEED_SUPER:
    printf("[BRF] Device operates at max speed\n");
    break;
  default:
    printf("[BRF] Device does not operates at max speed, change the USB port\n");
    brf_error(BLADERF_ERR_UNSUPPORTED);
  }
  // RX
  // Example of CLI output: RX Frequency: 2539999999Hz
  
  if ((status=bladerf_set_frequency(brf->dev, BLADERF_MODULE_RX, (unsigned int) openair0_cfg[card].rx_freq[0])) != 0){
    fprintf(stderr,"Failed to set RX frequency: %s\n",bladerf_strerror(status));
    brf_error(status);
  } else 
    printf("[BRF] set RX frequency to %f\n",openair0_cfg[card].rx_freq[0]);
  
 
  if ((status=bladerf_set_sample_rate(brf->dev, BLADERF_MODULE_RX, (unsigned int)openair0_cfg[card].sample_rate, NULL)) != 0){
    fprintf(stderr,"Failed to set RX sample rate: %s\n", bladerf_strerror(status));
    brf_error(status);
  }else 
    printf("[BRF] set RX sample rate to %f\n",openair0_cfg[card].sample_rate);
 
  if ((status=bladerf_set_bandwidth(brf->dev, BLADERF_MODULE_RX, (unsigned int) openair0_cfg[card].rx_bw, NULL)) != 0){
    fprintf(stderr,"Failed to set RX bandwidth: %s\n", bladerf_strerror(status));
    brf_error(status);
  }else 
    printf("[BRF] set RX bandwidth to %f\n",openair0_cfg[card].rx_bw);
 
  if ((status=bladerf_set_gain(brf->dev, BLADERF_MODULE_RX, (int) openair0_cfg[card].rx_gain[0])) != 0) {
    fprintf(stderr,"Failed to set RX gain: %s\n",bladerf_strerror(status));
    brf_error(status);
  } else 
    printf("[BRF] set RX gain to %f\n",openair0_cfg[card].rx_gain[0]);

  /* Configure the device's RX module for use with the sync interface.
   * SC16 Q11 samples *with* metadata are used. */
  if ((status=bladerf_sync_config(brf->dev, BLADERF_MODULE_RX, BLADERF_FORMAT_SC16_Q11_META,brf->num_buffers,brf->buffer_size,brf->num_transfers,brf->rx_timeout_ms)) != 0 ) {
    fprintf(stderr,"Failed to configure RX sync interface: %s\n", bladerf_strerror(status));
     brf_error(status);
  }else 
    printf("[BRF] configured Rx for sync interface \n");
  
   /* We must always enable the RX module after calling bladerf_sync_config(), and 
    * before  attempting to RX samples via  bladerf_sync_rx(). */
  if ((status=bladerf_enable_module(brf->dev, BLADERF_MODULE_RX, true)) != 0) {
    fprintf(stderr,"Failed to enable RX module: %s\n", bladerf_strerror(status));
    brf_error(status);
  }else 
    printf("[BRF] RX module enabled \n");
  
  // TX
  if ((status=bladerf_set_frequency(brf->dev, BLADERF_MODULE_TX, (unsigned int) openair0_cfg[card].tx_freq[0])) != 0){
    fprintf(stderr,"Failed to set TX frequency: %s\n",bladerf_strerror(status));
    brf_error(status);
  }else 
    printf("[BRF] set Tx Frequenct to %f \n", openair0_cfg[card].tx_freq[0]);

  if ((status=bladerf_set_sample_rate(brf->dev, BLADERF_MODULE_TX, (unsigned int) openair0_cfg[card].sample_rate, NULL)) != 0){
    fprintf(stderr,"Failed to set TX sample rate: %s\n", bladerf_strerror(status));
    brf_error(status);
  }else 
    printf("[BRF] set Tx sampling rate to %f \n", openair0_cfg[card].sample_rate);

  if ((status=bladerf_set_bandwidth(brf->dev, BLADERF_MODULE_TX,(unsigned int)openair0_cfg[card].tx_bw, NULL)) != 0){
    fprintf(stderr, "Failed to set RX bandwidth: %s\n", bladerf_strerror(status));
    brf_error(status);
  }else 
    printf("[BRF] set Tx sampling ratebandwidth to %f \n", openair0_cfg[card].tx_bw);

  if ((status=bladerf_set_gain(brf->dev, BLADERF_MODULE_TX, (int)openair0_cfg[card].tx_gain[0])) != 0) {
    fprintf(stderr,"Failed to set TX gain: %s\n",bladerf_strerror(status));
    brf_error(status);
  }else 
    printf("[BRF] set the Tx gain to %f \n", openair0_cfg[card].tx_gain[0]);


  /* Configure the device's TX module for use with the sync interface.
   * SC16 Q11 samples *with* metadata are used. */
  if ((status=bladerf_sync_config(brf->dev, BLADERF_MODULE_TX,BLADERF_FORMAT_SC16_Q11_META,brf->num_buffers,brf->buffer_size,brf->num_transfers,brf->tx_timeout_ms)) != 0 ) {
    fprintf(stderr,"Failed to configure TX sync interface: %s\n", bladerf_strerror(status));
     brf_error(status);
  }else 
    printf("[BRF] configured tx for sync interface \n");

   /* We must always enable the TX module after calling bladerf_sync_config(), and 
    * before  attempting to TX samples via  bladerf_sync_tx(). */
  if ((status=bladerf_enable_module(brf->dev, BLADERF_MODULE_TX, true)) != 0) {
    fprintf(stderr,"Failed to enable TX module: %s\n", bladerf_strerror(status));
    brf_error(status);
  } else 
    printf("[BRF] TX module enabled \n");
 
  bladerf_log_set_verbosity(get_brf_log_level(openair0_cfg[card].log_level));
  
  printf("BLADERF: Initializing openair0_device\n");
  device->priv           = brf; 
  device->Mod_id         = num_devices++;
  device->trx_start_func = trx_brf_start;
  device->trx_end_func   = trx_brf_end;
  device->trx_read_func  = trx_brf_read;
  device->trx_write_func = trx_brf_write;
  device->trx_get_stats_func   = trx_brf_get_stats;
  device->trx_reset_stats_func = trx_brf_reset_stats;
  device->trx_stop_func        = trx_brf_stop;
  device->trx_set_freq_func    = trx_brf_set_freq;
  device->trx_set_gains_func   = trx_brf_set_gains;
  memcpy((void*)&device->openair0_cfg,(void*)openair0_cfg,sizeof(openair0_config_t));

  return 0;
}

int brf_error(int status) {
  
  //exit(-1);
  //return 1; // or status error code
}



struct bladerf * open_bladerf_from_serial(const char *serial) {

  int status;
  struct bladerf *dev;
  struct bladerf_devinfo info;
  /* Initialize all fields to "don't care" wildcard values.
   *
   * Immediately passing this to bladerf_open_with_devinfo() would cause
   * libbladeRF to open any device on any available backend. */
  bladerf_init_devinfo(&info);
  /* Specify the desired device's serial number, while leaving all other
   * fields in the info structure wildcard values */
  strncpy(info.serial, serial, BLADERF_SERIAL_LENGTH - 1);
  info.serial[BLADERF_SERIAL_LENGTH - 1] = '\0';
  status = bladerf_open_with_devinfo(&dev, &info);
  
  if (status == BLADERF_ERR_NODEV) {
    printf("No devices available with serial=%s\n", serial);
    return NULL;
  } else if (status != 0) {
    fprintf(stderr, "Failed to open device with serial=%s (%s)\n", serial, bladerf_strerror(status));
    return NULL;
  } else {
    return dev;
  }
}
int get_brf_log_level(int log_level){

  int level=BLADERF_LOG_LEVEL_INFO;
  //return  BLADERF_LOG_LEVEL_DEBUG;
  switch(log_level) {
  case LOG_DEBUG:
    level=BLADERF_LOG_LEVEL_DEBUG;
    break;
  case LOG_INFO:
    level= BLADERF_LOG_LEVEL_INFO;
    break;
  case LOG_WARNING:
    level=BLADERF_LOG_LEVEL_WARNING;
    break;
  case LOG_ERR:
    level=BLADERF_LOG_LEVEL_ERROR;
    break;
  case LOG_CRIT:
    level=BLADERF_LOG_LEVEL_CRITICAL;
    break;
  case LOG_EMERG:
    level = BLADERF_LOG_LEVEL_SILENT;
    break;
  default:
    break;
  }
  return level;
}
