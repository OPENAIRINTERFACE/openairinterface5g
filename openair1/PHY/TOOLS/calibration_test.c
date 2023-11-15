#include <stdint.h>
#include <openair1/PHY/impl_defs_top.h>
#include <radio/COMMON/common_lib.h>
#include <executables/softmodem-common.h>
#include <openair1/PHY/TOOLS/calibration_scope.h>
#include "nfapi/oai_integration/vendor_ext.h"


int oai_exit=false;
unsigned int mmapped_dma=0;
int      single_thread_flag;
uint32_t timing_advance;
int8_t threequarter_fs;
uint64_t downlink_frequency[MAX_NUM_CCs][4];
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];
int opp_enabled;
static double snr_dB=20;
THREAD_STRUCT thread_struct;
uint32_t target_ul_mcs = 9;
uint32_t target_dl_mcs = 9;
uint64_t dlsch_slot_bitmap = (1<<1);
uint64_t ulsch_slot_bitmap = (1<<8);
uint32_t target_ul_bw = 50;
uint32_t target_dl_bw = 50;
uint32_t target_dl_Nl;
uint32_t target_ul_Nl;
char *uecap_file;
#include <executables/nr-softmodem.h>

int read_recplayconfig(recplay_conf_t **recplay_conf, recplay_state_t **recplay_state) {return 0;}
void nfapi_setmode(nfapi_mode_t nfapi_mode) {}
void set_taus_seed(unsigned int seed_init){};
configmodule_interface_t *uniqCfg = NULL;
int main(int argc, char **argv) {
  ///static configuration for NR at the moment
  if ((uniqCfg = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY)) == NULL) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }
  set_softmodem_sighandler();
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  logInit();
   paramdef_t cmdline_params[] = CMDLINE_PARAMS_DESC_GNB ;

  CONFIG_SETRTFLAG(CONFIG_NOEXITONHELP);
  get_common_options(uniqCfg, SOFTMODEM_GNB_BIT);
  config_process_cmdline(uniqCfg, cmdline_params, sizeofArray(cmdline_params), NULL);
  CONFIG_CLEARRTFLAG(CONFIG_NOEXITONHELP);
  set_latency_target();

    
  int N_RB=50;
  int sampling_rate=30.72e6;
  int DFT=2048;
  int TxAdvanceInDFTSize=12;
  int antennas=1;
  uint64_t freq=3619.200e6;
  int rxGain=90;
  int txGain=90;
  int filterBand=40e6;
  char * usrp_addrs="type=b200";

  openair0_config_t openair0_cfg= {
    //! Module ID for this configuration
    .Mod_id=0,
    //! device log level
    .log_level=0,
    //! duplexing mode
    .duplex_mode=0,
    //! number of downlink resource blocks
    .num_rb_dl=N_RB,
    //! number of samples per frame
    .samples_per_frame=0,
    //! the sample rate for both transmit and receive.
    .sample_rate=sampling_rate,
    //device is doing mmapped DMA transfers
    .mmapped_dma=0,
    //! offset in samples between TX and RX paths
    .tx_sample_advance=0,
    //! samples per packet on the fronthaul interface
    .samples_per_packet=1024,
    //! number of RX channels (=RX antennas)
    .rx_num_channels=antennas,
    //! number of TX channels (=TX antennas)
    .tx_num_channels=antennas,
    //! \brief Center frequency in Hz for RX.
    //! index: [0..rx_num_channels[
    .rx_freq={freq,freq,freq,freq},
    //! \brief Center frequency in Hz for TX.
    //! index: [0..rx_num_channels[ !!! see lte-ue.c:427 FIXME iterates over rx_num_channels
    .tx_freq={freq,freq,freq,freq},
    //! \brief memory
    //! \brief Pointer to Calibration table for RX gains
    .rx_gain_calib_table=NULL,
    //! mode for rxgain (ExpressMIMO2)
    .rxg_mode={0},
    //! \brief Gain for RX in dB.
    //! index: [0..rx_num_channels]
    .rx_gain={rxGain,rxGain,rxGain,rxGain},
    //! \brief Gain offset (for calibration) in dB
    //! index: [0..rx_num_channels]
    .rx_gain_offset={0},
    //! gain for TX in dB
    .tx_gain={txGain,txGain,txGain,txGain},
    //! RX bandwidth in Hz
    .rx_bw=filterBand,
    //! TX bandwidth in Hz
    .tx_bw=filterBand,
    //! clock source
    .clock_source=external,//internal gpsdo external
    //! timing_source
    .time_source=internal, //internal gpsdo external
    //! Manual SDR IP address
    .sdr_addrs=usrp_addrs,
    //! Auto calibration flag
    .autocal={0},
    //! rf devices work with x bits iqs when oai have its own iq format
    //! the two following parameters are used to convert iqs
    .iq_txshift=0,
    .iq_rxrescale=0,
    //! Configuration file for LMS7002M
    .configFilename="",
    //! remote IP/MAC addr for Ethernet interface
    .remote_addr="",
    //! remote port number for Ethernet interface
    .remote_port=0,
    //! local IP/MAC addr for Ethernet interface (eNB/BBU, UE)
    .my_addr=0,
    //! local port number for Ethernet interface (eNB/BBU, UE)
    .my_port=0,
    //! record player configuration, definition in record_player.h
    .recplay_mode=0,
    .recplay_conf=NULL,
    //! number of samples per tti
    .samples_per_tti=0,
    //! check for threequarter sampling rate
    .threequarter_fs=0,
  };
  //-----------------------
  openair0_device rfdevice= {
    /*!tx write thread*/
    //.write_thread={0},
    /*!brief Module ID of this device */
    .Mod_id=0,
    /*!brief Component Carrier ID of this device */
    .CC_id=0,
    /*!brief Type of this device */
    .type=NONE_DEV,
    /*!brief Transport protocol type that the device supports (in case I/Q samples need to be transported) */
    .transp_type=NONE_TP,
    /*!brief Type of the device's host (RAU/RRU) */
    .host_type=MIN_HOST_TYPE,
    /* !brief RF frontend parameters set by application */
    .openair0_cfg=NULL, //set by device_init
    /* !brief ETH params set by application */
    .eth_params=NULL,
    //! record player data, definition in record_player.h
    .recplay_state=NULL,
    /* !brief Indicates if device already initialized */
    .is_init=0,
    /*!brief Can be used by driver to hold internal structure*/
    .priv=NULL,
    /* Functions API, which are called by the application*/
    /*! \brief Called to start the transceiver. Return 0 if OK, < 0 if error
        @param device pointer to the device structure specific to the RF hardware target
    */
    .trx_start_func=NULL,

    /*! \brief Called to configure the device
         @param device pointer to the device structure specific to the RF hardware target
     */
    .trx_config_func=NULL,

    /*! \brief Called to send a request message between RAU-RRU on control port
        @param device pointer to the device structure specific to the RF hardware target
        @param msg pointer to the message structure passed between RAU-RRU
        @param msg_len length of the message
    */
    .trx_ctlsend_func=NULL,

    /*! \brief Called to receive a reply  message between RAU-RRU on control port
        @param device pointer to the device structure specific to the RF hardware target
        @param msg pointer to the message structure passed between RAU-RRU
        @param msg_len length of the message
    */
    .trx_ctlrecv_func=NULL,

    /*! \brief Called to send samples to the RF target
          @param device pointer to the device structure specific to the RF hardware target
        @param timestamp The timestamp at whicch the first sample MUST be sent
        @param buff Buffer which holds the samples (2 dimensional)
        @param nsamps number of samples to be sent
        @param number of antennas
        @param flags flags must be set to TRUE if timestamp parameter needs to be applied
    */
    .trx_write_func=NULL,

    /*! \brief Called to send samples to the RF target
        @param device pointer to the device structure specific to the RF hardware target
        @param timestamp The timestamp at whicch the first sample MUST be sent
        @param buff Buffer which holds the samples (1 dimensional)
        @param nsamps number of samples to be sent
        @param antenna_id index of the antenna if the device has multiple anteannas
        @param flags flags must be set to TRUE if timestamp parameter needs to be applied
    */
    .trx_write_func2=NULL,

    /*! \brief Receive samples from hardware.
     * Read \ref nsamps samples from each channel to buffers. buff[0] is the array for
     * the first channel. *ptimestamp is the time at which the first sample
     * was received.
     * \param device the hardware to use
     * \param[out] ptimestamp the time at which the first sample was received.
     * \param[out] buff An array of pointers to buffers for received samples. The buffers must be large enough to hold the number of samples \ref nsamps.
     * \param nsamps Number of samples. One sample is 2 byte I + 2 byte Q => 4 byte.
     * \param num_antennas number of antennas from which to receive samples
     * \returns the number of sample read
     */

    .trx_read_func=NULL,

    /*! \brief Receive samples from hardware, this version provides a single antenna at a time and returns.
     * Read \ref nsamps samples from each channel to buffers. buff[0] is the array for
     * the first channel. *ptimestamp is the time at which the first sample
     * was received.
     * \param device the hardware to use
     * \param[out] ptimestamp the time at which the first sample was received.
     * \param[out] buff A pointers to a buffer for received samples. The buffer must be large enough to hold the number of samples \ref nsamps.
     * \param nsamps Number of samples. One sample is 2 byte I + 2 byte Q => 4 byte.
     * \param antenna_id Index of antenna from which samples were received
     * \returns the number of sample read
     */
    .trx_read_func2=NULL,

    /*! \brief print the device statistics
     * \param device the hardware to use
     * \returns  0 on success
     */
    /*! \brief print the device statistics
       * \param device the hardware to use
       * \returns  0 on success
       */
    .trx_get_stats_func=NULL,

    /*! \brief Reset device statistics
     * \param device the hardware to use
     * \returns 0 in success
     */
    .trx_reset_stats_func=NULL,

    /*! \brief Terminate operation of the transceiver -- free all associated resources
     * \param device the hardware to use
     */
    .trx_end_func=NULL,

    /*! \brief Stop operation of the transceiver
     */
    .trx_stop_func=NULL,

    /* Functions API related to UE*/

    /*! \brief Set RX feaquencies
     * \param device the hardware to use
     * \param openair0_cfg RF frontend parameters set by application
     * \returns 0 in success
     */
    .trx_set_freq_func=NULL,

    /*! \brief Set gains
     * \param device the hardware to use
     * \param openair0_cfg RF frontend parameters set by application
     * \returns 0 in success
     */
    .trx_set_gains_func=NULL,

    /*! \brief RRU Configuration callback
     * \param idx RU index
     * \param arg pointer to capabilities or configuration
     */
    .configure_rru=NULL,
    /*! \brief Pointer to generic RRU private information
       */
    .thirdparty_priv=NULL,
    .thirdparty_init=NULL,
    /*! \brief Callback for Third-party RRU Cleanup routine
       \param device the hardware configuration to use
     */
    .thirdparty_cleanup=NULL,

    /*! \brief Callback for Third-party start streaming routine
       \param device the hardware configuration to use
     */
    .thirdparty_startstreaming=NULL,

    /*! \brief RRU Configuration callback
     * \param idx RU index
     * \param arg pointer to capabilities or configuration
     */
    .trx_write_init=NULL,
    /* \brief Get internal parameter
     * \param id parameter to get
     * \return a pointer to the parameter
     */
    .get_internal_parameter=NULL,
  };
  
  openair0_device_load(&rfdevice,&openair0_cfg);

  void ** samplesRx = (void **)malloc16(antennas* sizeof(c16_t *) );
  void ** samplesTx = (void **)malloc16(antennas* sizeof(c16_t *) );

  int fd=open(getenv("rftestInputFile"),O_RDONLY);
  AssertFatal(fd>=0,"%s",strerror(errno));
  
  for (int i=0; i<antennas; i++) {
    samplesRx[i] = (int32_t *)malloc16_clear( DFT*sizeof(c16_t) );
    samplesTx[i] = (int32_t *)malloc16_clear( DFT*sizeof(c16_t) );
  }

  CalibrationInitScope(samplesRx, &rfdevice);
  openair0_timestamp timestamp=0;
  rfdevice.trx_start_func(&rfdevice);
  
  while(!oai_exit) {
    for (int i=0; i<antennas; i++)
      read(fd, samplesTx[i], DFT*sizeof(c16_t));
    rfdevice.trx_read_func(&rfdevice, &timestamp, samplesRx, DFT, antennas);
    rfdevice.trx_write_func(&rfdevice, timestamp + TxAdvanceInDFTSize * DFT, samplesTx, DFT, antennas, 0);
  }

  return 0;
}
