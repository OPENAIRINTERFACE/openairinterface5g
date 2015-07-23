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
/*! \file common_lib.h 
 * \brief common APIs for different RF frontend device 
 * \author HongliangXU, Navid Nikaein
 * \date 2015
 * \version 0.2
 * \company Eurecom
 * \maintainer:  navid.nikaein@eurecom.fr
 * \note
 * \warning
 */

#ifndef COMMON_LIB_H
#define COMMON_LIB_H
#include <stdint.h>
#include <sys/types.h>

typedef int64_t openair0_timestamp;
typedef volatile int64_t openair0_vtimestamp;


typedef struct openair0_device_t openair0_device;
/* structrue holds the parameters to configure USRP devices
 */

#ifndef EXMIMO
#define MAX_CARDS 1
#endif

//#define USRP_GAIN_OFFSET (56.0)  // 86 calibrated for USRP B210 @ 2.6 GHz to get equivalent RS EPRE in OAI to SMBV100 output

typedef enum {
  max_gain=0,med_gain,byp_gain
} rx_gain_t;

typedef struct {
  //! Frequency for which RX chain was calibrated
  double freq;
  //! Offset to be applied to RX gain
  double offset;
} rx_gain_calib_table_t;

typedef struct {
  //! Module ID for this configuration
  int Mod_id;
  // device log level
  int log_level;
  //! number of downlink resource blocks
  int num_rb_dl;
  //! number of samples per frame 
  unsigned int  samples_per_frame;
  //! the sample rate for both transmit and receive.
  double sample_rate;
  //! number of samples per RX/TX packet (USRP + Ethernet)
  int samples_per_packet;
  // delay in sending samples (write)  due to hardware access, softmodem processing and fronthaul delay if exist
  int tx_delay;
  //! adjust the position of the samples after delay when sending   
  unsigned int	tx_forward_nsamps;
  //! number of RX channels (=RX antennas)
  int rx_num_channels;
  //! number of TX channels (=TX antennas)
  int tx_num_channels;
  //! \brief Center frequency in Hz for RX.
  //! index: [0..rx_num_channels[
  double rx_freq[4];
  //! \brief Center frequency in Hz for TX.
  //! index: [0..rx_num_channels[ !!! see lte-ue.c:427 FIXME iterates over rx_num_channels
  double tx_freq[4];
  //! mode for rxgain (ExpressMIMO2) 
  rx_gain_t rxg_mode[4];
  //! \brief Gain for RX in dB.
  //! index: [0..rx_num_channels]
  double rx_gain[4];
  //! \brief Gain offset (for calibration) in dB
  //! index: [0..rx_num_channels]
  double rx_gain_offset[4];
  //! gain for TX in dB
  double tx_gain[4];
  //! RX bandwidth in Hz
  double rx_bw;
  //! TX bandwidth in Hz
  double tx_bw;
  //! Auto calibration flag
  int autocal[4];
  //! RRH IP addr for Ethernet interface
  char *remote_ip;
  //! RRH port number for Ethernet interface
  int remote_port;
  //! my IP addr for Ethernet interface (eNB/BBU, UE)
  char *my_ip;
  //! my port number for Ethernet interface (eNB/BBU, UE)
  int my_port;

} openair0_config_t;

typedef struct {
  /* card id */
  int card;
  /* rf chain id */
  int chain;
} openair0_rf_map;



/*!\brief device type */
typedef enum {
  MIN_DEV_TYPE = 0,
  /*!\brief device is ETH */
  ETH_IF,
  /*!\brief device is ExpressMIMO */
  EXMIMO_IF,
  /*!\brief device is USRP*/
  USRP_IF,
  /*!\brief device is BLADE RF*/
  BLADERF_IF,
  /*!\brief device is NONE*/
  NONE_IF,
  MAX_DEV_TYPE

} dev_type_t;


/*!\brief  type */
typedef enum {
  MIN_FUNC_TYPE = 0,
 /*!\brief device functions within a  BBU */
  BBU_FUNC,
 /*!\brief device functions within a RRH */
  RRH_FUNC,
  MAX_FUNC_TYPE

}func_type_t;

struct openair0_device_t {
  /* Module ID of this device */
  int Mod_id;
  
  /* Type of this device */
  dev_type_t type;

   /* Type of the device's host (BBU/RRH) */
  func_type_t func_type;

  /* RF frontend parameters set by application */
  openair0_config_t openair0_cfg;

  /* Can be used by driver to hold internal structure*/
  void *priv;

  /* Functions API, which are called by the application*/

  /* Called to start the transceiver. Return 0 if OK, < 0 if error */
  int (*trx_start_func)(openair0_device *device);

  /* Called to send a request message between BBU-RRH */
  int (*trx_request_func)(openair0_device *device, void *msg, ssize_t msg_len);

  /* Called to send a reply  message between BBU-RRH */
  int (*trx_reply_func)(openair0_device *openair0, void *msg, ssize_t msg_len);

  /* Write 'nsamps' samples on each channel from buffers. buff[0] is the array for
   * the first channel. timestamp if the time (in samples) at which the first sample
   * MUST be sent
   * use flags = 1 to send as timestamp specfied*/
  int (*trx_write_func)(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps,int antenna_id, int flags);

  /*! \brief Receive samples from hardware.
   * Read \ref nsamps samples from each channel to buffers. buff[0] is the array for
   * the first channel. *ptimestamp is the time at which the first sample
   * was received.
   * \param device the hardware to use
   * \param[out] ptimestamp the time at which the first sample was received.
   * \param[out] An array of pointers to buffers for received samples. The buffers must be large enough to hold the number of samples \ref nsamps.
   * \param nsamps Number of samples. One sample is 2 byte I + 2 byte Q => 4 byte.
   * \param cc Number of channels. If cc == 1, only buff[0] is filled with samples.
   * \returns the number of sample read
   */
  int (*trx_read_func)(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps,int antenna_id);

  /*! \brief print the device statistics  
   * \param device the hardware to use
   * \returns  0 on success
   */
  int (*trx_get_stats_func)(openair0_device *device);

  /*! \brief Reset device statistics  
   * \param device the hardware to use
   * \returns 0 in success 
   */
  int (*trx_reset_stats_func)(openair0_device *device);

  /* Terminate operation of the transceiver -- free all associated resources */
  void (*trx_end_func)(openair0_device *device);

  /* Terminate operation  */
  int (*trx_stop_func)(int card);

  /* Functions API related to UE*/

  /*! \brief Set RX feaquencies 
   * \param 
   * \returns 0 in success 
   */
  int (*trx_set_freq_func)(openair0_device* device, openair0_config_t *openair0_cfg,int exmimo_dump_config);
  /*! \brief Set gains
   * \param 
   * \returns 0 in success 
   */
  int (*trx_set_gains_func)(openair0_device* device, openair0_config_t *openair0_cfg);

};


#ifdef __cplusplus
extern "C"
{
#endif

/* return 0 if OK */
int openair0_device_init(openair0_device* device, openair0_config_t *openair0_cfg);
  //int openair0_stop(int card);

//ETHERNET
int openair0_dev_init_eth(openair0_device *device, openair0_config_t *openair0_cfg);
  //int openair0_stop_eth(int card);
  //int openair0_set_gains_eth(openair0_device* device, openair0_config_t *openair0_cfg);
  //int openair0_set_frequencies_eth(openair0_device* device, openair0_config_t *openair0_cfg,int exmimo_dump_config);

//USPRP
openair0_timestamp get_usrp_time(openair0_device *device);
int openair0_set_rx_frequencies(openair0_device* device, openair0_config_t *openair0_cfg);


#ifdef __cplusplus
}
#endif

#endif // COMMON_LIB_H

