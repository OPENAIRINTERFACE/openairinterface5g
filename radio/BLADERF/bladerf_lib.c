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

/** bladerf_lib.c
 *
 * Author: navid nikaein
 */


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "bladerf_lib.h"
#include "math.h"
#include "PHY/sse_intrin.h"

/** @addtogroup _BLADERF_PHY_RF_INTERFACE_
 * @{
 */

//! Number of BladeRF devices
int num_devices=0;

/*These items configure the underlying asynch stream used by the the sync interface.
 */

/*! \brief BladeRF Init function (not used at the moment)
 * \param device RF frontend parameters set by application
 * \returns 0 on success
 */
int trx_brf_init(openair0_device *device)
{
    return 0;
}


/*! \brief get current timestamp
 *\param device the hardware to use
 *\param module the bladeRf module
 *\returns timestamp of BladeRF
 */
openair0_timestamp trx_get_timestamp(openair0_device *device,
                                     bladerf_module module)
{
    int status;
    struct bladerf_metadata meta;
    brf_state_t *brf = (brf_state_t*)device->priv;
    memset(&meta, 0, sizeof(meta));

    if ((status=bladerf_get_timestamp(brf->dev, module, &meta.timestamp)) != 0) {
        fprintf(stderr,"Failed to get current %s timestamp: %s\n",(module == BLADERF_MODULE_RX ) ? "RX" : "TX", bladerf_strerror(status));
        return -1;
    } // else {printf("Current RX timestampe  0x%016"PRIx64"\n", meta.timestamp); }

    return meta.timestamp;
}


/*! \brief Start BladeRF
 * \param device the hardware to use
 * \returns 0 on success
 */
int trx_brf_start(openair0_device *device)
{
    brf_state_t *brf = (brf_state_t*)device->priv;
    int status;

    brf->meta_tx.flags = 0;

    if ((status = bladerf_sync_config(brf->dev,
                                      BLADERF_MODULE_TX,
                                      BLADERF_FORMAT_SC16_Q11_META,
                                      brf->num_buffers,
                                      brf->buffer_size,
                                      brf->num_transfers,
                                      100/*brf->tx_timeout_ms*/)) != 0 ) {
        fprintf(stderr,"Failed to configure TX sync interface: %s\n", bladerf_strerror(status));
        abort();
    }
    if ((status = bladerf_sync_config(brf->dev,
                                      BLADERF_MODULE_RX,
                                      BLADERF_FORMAT_SC16_Q11_META,
                                      brf->num_buffers,
                                      brf->buffer_size,
                                      brf->num_transfers,
                                      100/*brf->rx_timeout_ms*/)) != 0 ) {
        fprintf(stderr,"Failed to configure RX sync interface: %s\n", bladerf_strerror(status));
        abort();
    }
    if ((status=bladerf_enable_module(brf->dev, BLADERF_MODULE_TX, true)) != 0) {
        fprintf(stderr,"Failed to enable TX module: %s\n", bladerf_strerror(status));
        abort();
    }
    if ((status=bladerf_enable_module(brf->dev, BLADERF_MODULE_RX, true)) != 0) {
        fprintf(stderr,"Failed to enable RX module: %s\n", bladerf_strerror(status));
        abort();
    }

    return 0;
}


/*! \brief Called to send samples to the BladeRF RF target
      \param device pointer to the device structure specific to the RF hardware target
      \param timestamp The timestamp at whicch the first sample MUST be sent
      \param buff Buffer which holds the samples
      \param nsamps number of samples to be sent
      \param cc index of the component carrier
      \param flags Ignored for the moment
      \returns 0 on success
*/
static int trx_brf_write(openair0_device *device,
                         openair0_timestamp ptimestamp,
                         void **buff,
                         int nsamps,
                         int cc,
                         int flags)
{
    int status;
    brf_state_t *brf = (brf_state_t*)device->priv;
    /* BRF has only 1 rx/tx chaine : is it correct? */
    int16_t *samples = (int16_t*)buff[0];

    //memset(&brf->meta_tx, 0, sizeof(brf->meta_tx));
    // When  BLADERF_META_FLAG_TX_NOW is used the timestamp is not used, so one can't schedule a tx
    if (brf->meta_tx.flags == 0 )
        brf->meta_tx.flags = (BLADERF_META_FLAG_TX_BURST_START);// | BLADERF_META_FLAG_TX_BURST_END);// |  BLADERF_META_FLAG_TX_NOW);


    brf->meta_tx.timestamp= (uint64_t) (ptimestamp);
    status = bladerf_sync_tx(brf->dev, samples, (unsigned int) nsamps, &brf->meta_tx, 2*brf->tx_timeout_ms);

    if (brf->meta_tx.flags == BLADERF_META_FLAG_TX_BURST_START)
        brf->meta_tx.flags =  BLADERF_META_FLAG_TX_UPDATE_TIMESTAMP;


    if (status != 0) {
        //fprintf(stderr,"Failed to TX sample: %s\n", bladerf_strerror(status));
        brf->num_tx_errors++;
        brf_error(status);
    } else if (brf->meta_tx.status & BLADERF_META_STATUS_UNDERRUN) {
        /* libbladeRF does not report this status. It is here for future use. */
        fprintf(stderr, "TX Underrun detected. %u valid samples were read.\n",  brf->meta_tx.actual_count);
        brf->num_underflows++;
    }
    //printf("Provided TX timestampe  %u, meta timestame %u\n", ptimestamp,brf->meta_tx.timestamp);

    //    printf("tx status %d \n",brf->meta_tx.status);
    brf->tx_current_ts=brf->meta_tx.timestamp;
    brf->tx_actual_nsamps+=brf->meta_tx.actual_count;
    brf->tx_nsamps+=nsamps;
    brf->tx_count++;


    return nsamps; //brf->meta_tx.actual_count;
}


/*! \brief Receive samples from hardware.
 * Read \ref nsamps samples from each channel to buffers. buff[0] is the array for
 * the first channel. *ptimestamp is the time at which the first sample
 * was received.
 * \param device the hardware to use
 * \param[out] ptimestamp the time at which the first sample was received.
 * \param[out] buff An array of pointers to buffers for received samples. The buffers must be large enough to hold the number of samples \ref nsamps.
 * \param nsamps Number of samples. One sample is 2 byte I + 2 byte Q => 4 byte.
 * \param cc  Index of component carrier
 * \returns number of samples read
*/
static int trx_brf_read(openair0_device *device,
                        openair0_timestamp *ptimestamp,
                        void **buff,
                        int nsamps,
                        int cc)
{
    int status=0;
    brf_state_t *brf = (brf_state_t*)device->priv;

    // BRF has only one rx/tx chain
    int16_t *samples = (int16_t*)buff[0];

    brf->meta_rx.actual_count = 0;
    brf->meta_rx.flags = BLADERF_META_FLAG_RX_NOW;
    status = bladerf_sync_rx(brf->dev, samples, (unsigned int) nsamps, &brf->meta_rx, 2*brf->rx_timeout_ms);

    //  printf("Current RX timestampe  %u, nsamps %u, actual %u, cc %d\n",  brf->meta_rx.timestamp, nsamps, brf->meta_rx.actual_count, cc);

    if (status != 0) {
        fprintf(stderr, "RX failed: %s\n", bladerf_strerror(status));
        //    printf("RX failed: %s\n", bladerf_strerror(status));
        brf->num_rx_errors++;
    } else if ( brf->meta_rx.status & BLADERF_META_STATUS_OVERRUN) {
        brf->num_overflows++;
        printf("RX overrun (%d) is detected. t=" "%" PRIu64 "Got %u samples. nsymps %d\n",
               brf->num_overflows,brf->meta_rx.timestamp,  brf->meta_rx.actual_count, nsamps);
    }

    if (brf->meta_rx.actual_count != nsamps) {
        printf("RX bad samples count, wanted %d, got %d\n", nsamps, brf->meta_rx.actual_count);
    }

    //printf("Current RX timestampe  %u\n",  brf->meta_rx.timestamp);
    //printf("[BRF] (buff %p) ts=0x%"PRIu64" %s\n",samples, brf->meta_rx.timestamp,bladerf_strerror(status));
    brf->rx_current_ts=brf->meta_rx.timestamp;
    brf->rx_actual_nsamps+=brf->meta_rx.actual_count;
    brf->rx_nsamps+=nsamps;
    brf->rx_count++;


    *ptimestamp = brf->meta_rx.timestamp;

    return nsamps; //brf->meta_rx.actual_count;

}


/*! \brief Terminate operation of the BladeRF transceiver -- free all associated resources
 * \param device the hardware to use
 */
void trx_brf_end(openair0_device *device)
{
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
    exit(1);
}


/*! \brief print the BladeRF statistics
* \param device the hardware to use
* \returns  0 on success
*/
int trx_brf_get_stats(openair0_device* device)
{
    return(0);
}


/*! \brief Reset the BladeRF statistics
* \param device the hardware to use
* \returns  0 on success
*/
int trx_brf_reset_stats(openair0_device* device)
{
    return(0);
}


/*! \brief Stop BladeRF
 * \param card the hardware to use
 * \returns 0 in success
 */
int trx_brf_stop(openair0_device* device)
{
    return(0);
}


/*! \brief Set frequencies (TX/RX)
 * \param device the hardware to use
 * \param openair0_cfg1 openair0 Config structure (ignored. It is there to comply with RF common API)
 * \returns 0 in success
 */
int trx_brf_set_freq(openair0_device* device, openair0_config_t *openair0_cfg1)
{
    int status;
    brf_state_t *brf = (brf_state_t *)device->priv;
    openair0_config_t *openair0_cfg = (openair0_config_t *)device->openair0_cfg;


    if ((status=bladerf_set_frequency(brf->dev, BLADERF_MODULE_TX, (unsigned int) openair0_cfg->tx_freq[0])) != 0) {
        fprintf(stderr,"Failed to set TX frequency: %s\n",bladerf_strerror(status));
        brf_error(status);
    } else
        printf("[BRF] set TX Frequency to %u\n", (unsigned int) openair0_cfg->tx_freq[0]);

    if ((status=bladerf_set_frequency(brf->dev, BLADERF_MODULE_RX, (unsigned int) openair0_cfg->rx_freq[0])) != 0) {
        fprintf(stderr,"Failed to set RX frequency: %s\n",bladerf_strerror(status));
        brf_error(status);
    } else
        printf("[BRF] set RX frequency to %u\n",(unsigned int)openair0_cfg->rx_freq[0]);

    return(0);

}


/*! \brief Set Gains (TX/RX)
 * \param device the hardware to use
 * \param openair0_cfg openair0 Config structure
 * \returns 0 in success
 */
int trx_brf_set_gains(openair0_device* device,
                      openair0_config_t *openair0_cfg)
{
    return(0);
}
int trx_brf_write_init(openair0_device *device)
{
    return 0;
}

#define RXDCLENGTH 16384
int16_t cos_fsover8[8]  = {2047,   1447,      0,  -1448,  -2047,  -1448,     0,   1447};
int16_t cos_3fsover8[8] = {2047,  -1448,      0,   1447,  -2047,   1447,     0,  -1448};

/*! \brief calibration table for BladeRF */
rx_gain_calib_table_t calib_table_fx4[] = {
    {2300000000.0,53.5},
    {1880000000.0,57.0},
    {816000000.0,73.0},
    {-1,0}
};


/*! \brief set RX gain offset from calibration table
 * \param openair0_cfg RF frontend parameters set by application
 * \param chain_index RF chain ID
 */
void set_rx_gain_offset(openair0_config_t *openair0_cfg,
                        int chain_index)
{
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


/*! \brief Calibrate LMSSDR RF
 * \param device the hardware to use
 */
void calibrate_rf(openair0_device *device)
{
    /* TODO: this function does not seem to work. Disabled until fixed. */
    return;

    brf_state_t *brf = (brf_state_t *)device->priv;
    openair0_timestamp ptimestamp;
    int16_t *calib_buffp,*calib_tx_buffp;
    int16_t calib_buff[2*RXDCLENGTH];
    int16_t calib_tx_buff[2*RXDCLENGTH];
    int i,j,offI,offQ,offIold,offQold,offInew,offQnew,offphase,offphaseold,offphasenew,offgain,offgainold,offgainnew;
    int32_t meanI,meanQ,meanIold,meanQold;
    int cnt=0,loop;

    // put TX on a far-away frequency to avoid interference in RX band
    bladerf_set_frequency(brf->dev,BLADERF_MODULE_TX, (unsigned int) device->openair0_cfg->rx_freq[0] + 200e6);
    // Set gains to close to max
    bladerf_set_gain(brf->dev, BLADERF_MODULE_RX, 60);
    bladerf_set_gain(brf->dev, BLADERF_MODULE_TX, 60);

    // fill TX buffer with fs/8 complex sinusoid
    j=0;
    for (i=0; i<RXDCLENGTH; i++) {
        calib_tx_buff[j++] = cos_fsover8[i&7];
        calib_tx_buff[j++] = cos_fsover8[(i+6)&7];  // sin
    }
    calib_buffp = &calib_buff[0];
    calib_tx_buffp = &calib_tx_buff[0];
    // Calibrate RX DC offset

    offIold=offQold=2048;
    bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_LMS_DCOFF_I,offIold);
    bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_LMS_DCOFF_Q,offQold);
    for (i=0; i<10; i++)
        trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);

    for (meanIold=meanQold=i=j=0; i<RXDCLENGTH; i++) {
        meanIold+=calib_buff[j++];
        meanQold+=calib_buff[j++];
    }
    meanIold/=RXDCLENGTH;
    meanQold/=RXDCLENGTH;
    printf("[BRF] RX DC: (%d,%d) => (%d,%d)\n",offIold,offQold,meanIold,meanQold);

    offI=offQ=-2048;
    bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_LMS_DCOFF_I,offI);
    bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_LMS_DCOFF_Q,offQ);
    for (i=0; i<10; i++)
        trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);

    for (meanI=meanQ=i=j=0; i<RXDCLENGTH; i++) {
        meanI+=calib_buff[j++];
        meanQ+=calib_buff[j++];
    }
    meanI/=RXDCLENGTH;
    meanQ/=RXDCLENGTH;
    //  printf("[BRF] RX DC: (%d,%d) => (%d,%d)\n",offI,offQ,meanI,meanQ);

    while (cnt++ < 12) {

        offInew=(offIold+offI)>>1;
        offQnew=(offQold+offQ)>>1;

        if (meanI*meanI < meanIold*meanIold) {
            meanIold = meanI;
            offIold = offI;
            printf("[BRF] *** RX DC: offI %d => %d\n",offIold,meanI);
        }
        if (meanQ*meanQ < meanQold*meanQold) {
            meanQold = meanQ;
            offQold = offQ;
            printf("[BRF] *** RX DC: offQ %d => %d\n",offQold,meanQ);
        }
        offI = offInew;
        offQ = offQnew;
        bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_LMS_DCOFF_I,offI);
        bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_LMS_DCOFF_Q,offQ);

        for (i=0; i<10; i++)
            trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);

        for (meanI=meanQ=i=j=0; i<RXDCLENGTH; i++) {
            meanI+=calib_buff[j++];
            meanQ+=calib_buff[j++];
        }
        meanI/=RXDCLENGTH;
        meanQ/=RXDCLENGTH;
        printf("[BRF] RX DC: (%d,%d) => (%d,%d)\n",offI,offQ,meanI,meanQ);
    }

    printf("[BRF] RX DC: (%d,%d) => (%d,%d)\n",offIold,offQold,meanIold,meanQold);
    bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_LMS_DCOFF_I,offIold);
    bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_LMS_DCOFF_Q,offQold);

    // TX DC offset
    // PUT TX as f_RX + fs/4
    // loop back BLADERF_LB_RF_LNA1
    bladerf_set_frequency(brf->dev,BLADERF_MODULE_TX, (unsigned int) device->openair0_cfg->rx_freq[0] + (unsigned int) device->openair0_cfg->sample_rate/4);
    bladerf_set_loopback (brf->dev,BLADERF_LB_RF_LNA1);

    offIold=2048;
    bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_LMS_DCOFF_I,offIold);
    for (i=0; i<10; i++) {
        trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
        trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
    }
    for (meanIold=meanQold=i=j=0; i<RXDCLENGTH; i++) {
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
    printf("[BRF] TX DC (offI): %d => (%d,%d)\n",offIold,meanIold,meanQold);

    offI=-2048;
    bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_LMS_DCOFF_I,offI);
    for (i=0; i<10; i++) {
        trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
        trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
    }
    for (meanI=meanQ=i=j=0; i<RXDCLENGTH; i++) {
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
    printf("[BRF] TX DC (offI): %d => (%d,%d)\n",offI,meanI,meanQ);
    cnt = 0;
    while (cnt++ < 12) {

        offInew=(offIold+offI)>>1;
        if (meanI*meanI+meanQ*meanQ < meanIold*meanIold +meanQold*meanQold) {
            printf("[BRF] TX DC (offI): ([%d,%d]) => %d : %d\n",offIold,offI,offInew,meanI*meanI+meanQ*meanQ);
            meanIold = meanI;
            meanQold = meanQ;
            offIold = offI;
        }
        offI = offInew;
        bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_LMS_DCOFF_I,offI);

        for (i=0; i<10; i++) {
            trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
            trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
        }
        for (meanI=meanQ=i=j=0; i<RXDCLENGTH; i++) {
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
        //    printf("[BRF] TX DC (offI): %d => (%d,%d)\n",offI,meanI,meanQ);
    }

    bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_LMS_DCOFF_I,offIold);

    offQold=2048;
    bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_LMS_DCOFF_Q,offQold);
    for (i=0; i<10; i++) {
        trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
        trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
    }
    // project on fs/4
    for (meanIold=meanQold=i=j=0; i<RXDCLENGTH; i++) {
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
    printf("[BRF] TX DC (offQ): %d => (%d,%d)\n",offQold,meanIold,meanQold);

    offQ=-2048;
    bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_LMS_DCOFF_Q,offQ);
    for (i=0; i<10; i++) {
        trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
        trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
    }
    for (meanI=meanQ=i=j=0; i<RXDCLENGTH; i++) {
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
    printf("[BRF] TX DC (offQ): %d => (%d,%d)\n",offQ,meanI,meanQ);

    cnt=0;
    while (cnt++ < 12) {

        offQnew=(offQold+offQ)>>1;
        if (meanI*meanI+meanQ*meanQ < meanIold*meanIold +meanQold*meanQold) {
            printf("[BRF] TX DC (offQ): ([%d,%d]) => %d : %d\n",offQold,offQ,offQnew,meanI*meanI+meanQ*meanQ);

            meanIold = meanI;
            meanQold = meanQ;
            offQold = offQ;
        }
        offQ = offQnew;
        bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_LMS_DCOFF_Q,offQ);

        for (i=0; i<10; i++) {
            trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
            trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
        }
        for (meanI=meanQ=i=j=0; i<RXDCLENGTH; i++) {
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
        //    printf("[BRF] TX DC (offQ): %d => (%d,%d)\n",offQ,meanI,meanQ);
    }

    printf("[BRF] TX DC: (%d,%d) => (%d,%d)\n",offIold,offQold,meanIold,meanQold);

    bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_LMS_DCOFF_Q,offQold);

    // TX IQ imbalance
    for (loop=0; loop<2; loop++) {
        offphaseold=4096;
        bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_FPGA_PHASE,offphaseold);
        for (i=0; i<10; i++) {
            trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
            trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
        }
        // project on fs/8 (Image of TX signal in +ve frequencies)
        for (meanIold=meanQold=i=j=0; i<RXDCLENGTH; i++) {
            meanIold+= (calib_buff[j]*cos_fsover8[i&7] - calib_buff[j+1]*cos_fsover8[(i+2)&7])>>11;
            meanQold+= (calib_buff[j]*cos_fsover8[(i+2)&7] + calib_buff[j+1]*cos_fsover8[i&7])>>11;
            j+=2;
        }

        meanIold/=RXDCLENGTH;
        meanQold/=RXDCLENGTH;
        printf("[BRF] TX IQ (offphase): %d => (%d,%d)\n",offphaseold,meanIold,meanQold);

        offphase=-4096;
        bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_FPGA_PHASE,offphase);
        for (i=0; i<10; i++) {
            trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
            trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
        }
        // project on fs/8 (Image of TX signal in +ve frequencies)
        for (meanI=meanQ=i=j=0; i<RXDCLENGTH; i++) {
            meanI+= (calib_buff[j]*cos_fsover8[i&7] - calib_buff[j+1]*cos_fsover8[(i+2)&7])>>11;
            meanQ+= (calib_buff[j]*cos_fsover8[(i+2)&7] + calib_buff[j+1]*cos_fsover8[i&7])>>11;
            j+=2;
        }

        meanI/=RXDCLENGTH;
        meanQ/=RXDCLENGTH;
        printf("[BRF] TX IQ (offphase): %d => (%d,%d)\n",offphase,meanI,meanQ);

        cnt=0;
        while (cnt++ < 13) {

            offphasenew=(offphaseold+offphase)>>1;
            printf("[BRF] TX IQ (offphase): ([%d,%d]) => %d : %d\n",offphaseold,offphase,offphasenew,meanI*meanI+meanQ*meanQ);
            if (meanI*meanI+meanQ*meanQ < meanIold*meanIold +meanQold*meanQold) {


                meanIold = meanI;
                meanQold = meanQ;
                offphaseold = offphase;
            }
            offphase = offphasenew;
            bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_FPGA_PHASE,offphase);

            for (i=0; i<10; i++) {
                trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
                trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
            }
            // project on fs/8 (Image of TX signal in +ve frequencies)
            for (meanI=meanQ=i=j=0; i<RXDCLENGTH; i++) {
                meanI+= (calib_buff[j]*cos_fsover8[i&7] - calib_buff[j+1]*cos_fsover8[(i+2)&7])>>11;
                meanQ+= (calib_buff[j]*cos_fsover8[(i+2)&7] + calib_buff[j+1]*cos_fsover8[i&7])>>11;
                j+=2;
            }
            meanI/=RXDCLENGTH;
            meanQ/=RXDCLENGTH;

            //    printf("[BRF] TX DC (offQ): %d => (%d,%d)\n",offQ,meanI,meanQ);
        }

        printf("[BRF] TX IQ offphase: %d => (%d,%d)\n",offphaseold,meanIold,meanQold);

        bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_FPGA_PHASE,offphaseold);

        offgainold=4096;
        bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_FPGA_GAIN,offgainold);
        for (i=0; i<10; i++) {
            trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
            trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
        }
        // project on fs/8 (Image of TX signal in +ve frequencies)
        for (meanIold=meanQold=i=j=0; i<RXDCLENGTH; i++) {
            meanIold+= (calib_buff[j]*cos_fsover8[i&7] - calib_buff[j+1]*cos_fsover8[(i+2)&7])>>11;
            meanQold+= (calib_buff[j]*cos_fsover8[(i+2)&7] + calib_buff[j+1]*cos_fsover8[i&7])>>11;
            j+=2;
        }

        meanIold/=RXDCLENGTH;
        meanQold/=RXDCLENGTH;
        printf("[BRF] TX IQ (offgain): %d => (%d,%d)\n",offgainold,meanIold,meanQold);

        offgain=-4096;
        bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_FPGA_GAIN,offgain);
        for (i=0; i<10; i++) {
            trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
            trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
        }
        // project on fs/8 (Image of TX signal in +ve frequencies)
        for (meanI=meanQ=i=j=0; i<RXDCLENGTH; i++) {
            meanI+= (calib_buff[j]*cos_fsover8[i&7] - calib_buff[j+1]*cos_fsover8[(i+2)&7])>>11;
            meanQ+= (calib_buff[j]*cos_fsover8[(i+2)&7] + calib_buff[j+1]*cos_fsover8[i&7])>>11;
            j+=2;
        }

        meanI/=RXDCLENGTH;
        meanQ/=RXDCLENGTH;
        printf("[BRF] TX IQ (offgain): %d => (%d,%d)\n",offgain,meanI,meanQ);

        cnt=0;
        while (cnt++ < 13) {

            offgainnew=(offgainold+offgain)>>1;
            if (meanI*meanI+meanQ*meanQ < meanIold*meanIold +meanQold*meanQold) {
                printf("[BRF] TX IQ (offgain): ([%d,%d]) => %d : %d\n",offgainold,offgain,offgainnew,meanI*meanI+meanQ*meanQ);

                meanIold = meanI;
                meanQold = meanQ;
                offgainold = offgain;
            }
            offgain = offgainnew;
            bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_FPGA_GAIN,offgain);

            for (i=0; i<10; i++) {
                trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
                trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
            }
            // project on fs/8 (Image of TX signal in +ve frequencies)
            for (meanI=meanQ=i=j=0; i<RXDCLENGTH; i++) {
                meanI+= (calib_buff[j]*cos_fsover8[i&7] - calib_buff[j+1]*cos_fsover8[(i+2)&7])>>11;
                meanQ+= (calib_buff[j]*cos_fsover8[(i+2)&7] + calib_buff[j+1]*cos_fsover8[i&7])>>11;
                j+=2;
            }
            meanI/=RXDCLENGTH;
            meanQ/=RXDCLENGTH;

            //    printf("[BRF] TX DC (offQ): %d => (%d,%d)\n",offQ,meanI,meanQ);
        }

        printf("[BRF] TX IQ offgain: %d => (%d,%d)\n",offgainold,meanIold,meanQold);

        bladerf_set_correction(brf->dev,BLADERF_MODULE_TX,BLADERF_CORR_FPGA_GAIN,offgainold);
    }

    // RX IQ imbalance
    for (loop=0; loop<2; loop++) {
        offphaseold=4096;
        bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_FPGA_PHASE,offphaseold);
        for (i=0; i<10; i++) {
            trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
            trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
        }
        // project on -3fs/8 (Image of TX signal in -ve frequencies)
        for (meanIold=meanQold=i=j=0; i<RXDCLENGTH; i++) {
            meanIold+= (calib_buff[j]*cos_3fsover8[i&7] - calib_buff[j+1]*cos_3fsover8[(i+2)&7])>>11;
            meanQold+= (calib_buff[j]*cos_3fsover8[(i+2)&7] + calib_buff[j+1]*cos_3fsover8[i&7])>>11;
            j+=2;
        }

        meanIold/=RXDCLENGTH;
        meanQold/=RXDCLENGTH;
        printf("[BRF] RX IQ (offphase): %d => (%d,%d)\n",offphaseold,meanIold,meanQold);

        offphase=-4096;
        bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_FPGA_PHASE,offphase);
        for (i=0; i<10; i++) {
            trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
            trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
        }
        // project on -3fs/8 (Image of TX signal in -ve frequencies)
        for (meanI=meanQ=i=j=0; i<RXDCLENGTH; i++) {
            meanI+= (calib_buff[j]*cos_3fsover8[i&7] - calib_buff[j+1]*cos_3fsover8[(i+2)&7])>>11;
            meanQ+= (calib_buff[j]*cos_3fsover8[(i+2)&7] + calib_buff[j+1]*cos_3fsover8[i&7])>>11;
            j+=2;
        }

        meanI/=RXDCLENGTH;
        meanQ/=RXDCLENGTH;
        printf("[BRF] RX IQ (offphase): %d => (%d,%d)\n",offphase,meanI,meanQ);

        cnt=0;
        while (cnt++ < 13) {

            offphasenew=(offphaseold+offphase)>>1;
            printf("[BRF] RX IQ (offphase): ([%d,%d]) => %d : %d\n",offphaseold,offphase,offphasenew,meanI*meanI+meanQ*meanQ);
            if (meanI*meanI+meanQ*meanQ < meanIold*meanIold +meanQold*meanQold) {


                meanIold = meanI;
                meanQold = meanQ;
                offphaseold = offphase;
            }
            offphase = offphasenew;
            bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_FPGA_PHASE,offphase);

            for (i=0; i<10; i++) {
                trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
                trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
            }
            // project on -3fs/8 (Image of TX signal in -ve frequencies)
            for (meanI=meanQ=i=j=0; i<RXDCLENGTH; i++) {
                meanI+= (calib_buff[j]*cos_3fsover8[i&7] - calib_buff[j+1]*cos_3fsover8[(i+2)&7])>>11;
                meanQ+= (calib_buff[j]*cos_3fsover8[(i+2)&7] + calib_buff[j+1]*cos_3fsover8[i&7])>>11;
                j+=2;
            }
            meanI/=RXDCLENGTH;
            meanQ/=RXDCLENGTH;

            //    printf("[BRF] TX DC (offQ): %d => (%d,%d)\n",offQ,meanI,meanQ);
        }

        printf("[BRF] RX IQ offphase: %d => (%d,%d)\n",offphaseold,meanIold,meanQold);

        bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_FPGA_PHASE,offphaseold);

        offgainold=4096;
        bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_FPGA_GAIN,offgainold);
        for (i=0; i<10; i++) {
            trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
            trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0,0);
        }
        // project on -3fs/8 (Image of TX signal in +ve frequencies)
        for (meanIold=meanQold=i=j=0; i<RXDCLENGTH; i++) {
            meanIold+= (calib_buff[j]*cos_3fsover8[i&7] - calib_buff[j+1]*cos_3fsover8[(i+2)&7])>>11;
            meanQold+= (calib_buff[j]*cos_3fsover8[(i+2)&7] + calib_buff[j+1]*cos_3fsover8[i&7])>>11;
            j+=2;
        }

        meanIold/=RXDCLENGTH;
        meanQold/=RXDCLENGTH;
        printf("[BRF] RX IQ (offgain): %d => (%d,%d)\n",offgainold,meanIold,meanQold);

        offgain=-4096;
        bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_FPGA_GAIN,offgain);
        for (i=0; i<10; i++) {
            trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
            trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
        }
        // project on 3fs/8 (Image of TX signal in -ve frequencies)
        for (meanI=meanQ=i=j=0; i<RXDCLENGTH; i++) {
            meanI+= (calib_buff[j]*cos_3fsover8[i&7] - calib_buff[j+1]*cos_3fsover8[(i+2)&7])>>11;
            meanQ+= (calib_buff[j]*cos_3fsover8[(i+2)&7] + calib_buff[j+1]*cos_3fsover8[i&7])>>11;
            j+=2;
        }

        meanI/=RXDCLENGTH;
        meanQ/=RXDCLENGTH;
        printf("[BRF] RX IQ (offgain): %d => (%d,%d)\n",offgain,meanI,meanQ);

        cnt=0;
        while (cnt++ < 13) {

            offgainnew=(offgainold+offgain)>>1;
            if (meanI*meanI+meanQ*meanQ < meanIold*meanIold +meanQold*meanQold) {
                printf("[BRF] RX IQ (offgain): ([%d,%d]) => %d : %d\n",offgainold,offgain,offgainnew,meanI*meanI+meanQ*meanQ);

                meanIold = meanI;
                meanQold = meanQ;
                offgainold = offgain;
            }
            offgain = offgainnew;
            bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_FPGA_GAIN,offgain);

            for (i=0; i<10; i++) {
                trx_brf_read(device, &ptimestamp, (void **)&calib_buffp, RXDCLENGTH, 0);
                trx_brf_write(device,ptimestamp+5*RXDCLENGTH, (void **)&calib_tx_buffp, RXDCLENGTH, 0, 0);
            }
            // project on -3fs/8 (Image of TX signal in -ve frequencies)
            for (meanI=meanQ=i=j=0; i<RXDCLENGTH; i++) {
                meanI+= (calib_buff[j]*cos_3fsover8[i&7] - calib_buff[j+1]*cos_3fsover8[(i+2)&7])>>11;
                meanQ+= (calib_buff[j]*cos_3fsover8[(i+2)&7] + calib_buff[j+1]*cos_3fsover8[i&7])>>11;
                j+=2;
            }
            meanI/=RXDCLENGTH;
            meanQ/=RXDCLENGTH;

            //    printf("[BRF] TX DC (offQ): %d => (%d,%d)\n",offQ,meanI,meanQ);
        }

        printf("[BRF] RX IQ offgain: %d => (%d,%d)\n",offgainold,meanIold,meanQold);

        bladerf_set_correction(brf->dev,BLADERF_MODULE_RX,BLADERF_CORR_FPGA_GAIN,offgainold);
    }

    bladerf_set_frequency(brf->dev,BLADERF_MODULE_TX, (unsigned int) device->openair0_cfg->tx_freq[0]);
    bladerf_set_loopback(brf->dev,BLADERF_LB_NONE);
    bladerf_set_gain(brf->dev, BLADERF_MODULE_RX, (unsigned int) device->openair0_cfg->rx_gain[0]-device->openair0_cfg[0].rx_gain_offset[0]);
    bladerf_set_gain(brf->dev, BLADERF_MODULE_TX, (unsigned int) device->openair0_cfg->tx_gain[0]);
    //  LOG_M("blade_rf_test.m","rxs",calib_buff,RXDCLENGTH,1,1);
}


/*! \brief Initialize Openair BLADERF target. It returns 0 if OK
 * \param device the hardware to use
 * \param openair0_cfg RF frontend parameters set by application
 * \returns 0 on success
 */
int device_init(openair0_device *device,
                openair0_config_t *openair0_cfg)
{
    int status;
    brf_state_t *brf = (brf_state_t*)malloc(sizeof(brf_state_t));
    memset(brf, 0, sizeof(brf_state_t));
    /* device specific */
    //openair0_cfg->txlaunch_wait = 1;//manage when TX processing is triggered
    //openair0_cfg->txlaunch_wait_slotcount = 1; //manage when TX processing is triggered
    openair0_cfg->iq_txshift = 0;// shift
    openair0_cfg->iq_rxrescale = 15;//rescale iqs

    // init required params
    switch ((int)openair0_cfg->sample_rate) {
    case 30720000:
        openair0_cfg->samples_per_packet    = 2048;
        openair0_cfg->tx_sample_advance     = 0;
        break;
    case 15360000:
        openair0_cfg->samples_per_packet    = 2048;
        openair0_cfg->tx_sample_advance     = 0;
        break;
    case 7680000:
        openair0_cfg->samples_per_packet    = 1024;
        openair0_cfg->tx_sample_advance     = 0;
        break;
    case 1920000:
        openair0_cfg->samples_per_packet    = 256;
        openair0_cfg->tx_sample_advance     = 50;
        break;
    default:
        printf("Error: unknown sampling rate %f\n",openair0_cfg->sample_rate);
        exit(-1);
        break;
    }
    openair0_cfg->iq_txshift= 0;
    openair0_cfg->iq_rxrescale = 15; /*not sure*/
    openair0_cfg->rx_gain_calib_table = calib_table_fx4;

    //  The number of buffers to use in the underlying data stream
    brf->num_buffers   = 128;
    // the size of the underlying stream buffers, in samples
    brf->buffer_size   = (unsigned int) openair0_cfg->samples_per_packet;//*sizeof(int32_t); // buffer size = 4096 for sample_len of 1024
    brf->num_transfers = 16;
    brf->rx_timeout_ms = 0;
    brf->tx_timeout_ms = 0;
    brf->sample_rate=(unsigned int)openair0_cfg->sample_rate;

    memset(&brf->meta_rx, 0, sizeof(brf->meta_rx));
    memset(&brf->meta_tx, 0, sizeof(brf->meta_tx));

    printf("\n[BRF] sampling_rate %u, num_buffers %u,  buffer_size %u, num transfer %u, timeout_ms (rx %u, tx %u)\n",
           brf->sample_rate, brf->num_buffers, brf->buffer_size,brf->num_transfers, brf->rx_timeout_ms, brf->tx_timeout_ms);

    if ((status=bladerf_open(&brf->dev, "")) != 0 ) {
        fprintf(stderr,"Failed to open brf device: %s\n",bladerf_strerror(status));
        brf_error(status);
    }
    printf("[BRF] init dev %p\n", brf->dev);
    switch(bladerf_device_speed(brf->dev)) {
    case BLADERF_DEVICE_SPEED_SUPER:
        printf("[BRF] Device operates at max speed\n");
        break;
    default:
        printf("[BRF] Device does not operates at max speed, change the USB port\n");
        brf_error(BLADERF_ERR_UNSUPPORTED);
    }
    // RX
    // Example of CLI output: RX Frequency: 2539999999Hz


    if ((status=bladerf_set_gain_mode(brf->dev, BLADERF_MODULE_RX, BLADERF_GAIN_MGC))) {
        fprintf(stderr, "[BRF] Failed to disable AGC\n");
        brf_error(status);
    }

    if ((status=bladerf_set_frequency(brf->dev, BLADERF_MODULE_RX, (unsigned int) openair0_cfg->rx_freq[0])) != 0) {
        fprintf(stderr,"Failed to set RX frequency: %s\n",bladerf_strerror(status));
        brf_error(status);
    } else
        printf("[BRF] set RX frequency to %u\n",(unsigned int)openair0_cfg->rx_freq[0]);



    unsigned int actual_value=0;
    if ((status=bladerf_set_sample_rate(brf->dev, BLADERF_MODULE_RX, (unsigned int) openair0_cfg->sample_rate, &actual_value)) != 0) {
        fprintf(stderr,"Failed to set RX sample rate: %s\n", bladerf_strerror(status));
        brf_error(status);
    } else
        printf("[BRF] set RX sample rate to %u, %u\n", (unsigned int) openair0_cfg->sample_rate, actual_value);


    if ((status=bladerf_set_bandwidth(brf->dev, BLADERF_MODULE_RX, (unsigned int) openair0_cfg->rx_bw*2, &actual_value)) != 0) {
        fprintf(stderr,"Failed to set RX bandwidth: %s\n", bladerf_strerror(status));
        brf_error(status);
    } else
        printf("[BRF] set RX bandwidth to %u, %u\n",(unsigned int)openair0_cfg->rx_bw*2, actual_value);

    set_rx_gain_offset(&openair0_cfg[0],0);
    if ((status=bladerf_set_gain(brf->dev, BLADERF_MODULE_RX, (int) openair0_cfg->rx_gain[0]-openair0_cfg[0].rx_gain_offset[0])) != 0) {
        fprintf(stderr,"Failed to set RX gain: %s\n",bladerf_strerror(status));
        brf_error(status);
    } else
        printf("[BRF] set RX gain to %d (%d)\n",(int)(openair0_cfg->rx_gain[0]-openair0_cfg[0].rx_gain_offset[0]),(int)openair0_cfg[0].rx_gain_offset[0]);

    // TX

    if ((status=bladerf_set_frequency(brf->dev, BLADERF_MODULE_TX, (unsigned int) openair0_cfg->tx_freq[0])) != 0) {
        fprintf(stderr,"Failed to set TX frequency: %s\n",bladerf_strerror(status));
        brf_error(status);
    } else
        printf("[BRF] set TX Frequency to %u\n", (unsigned int) openair0_cfg->tx_freq[0]);

    if ((status=bladerf_set_sample_rate(brf->dev, BLADERF_MODULE_TX, (unsigned int) openair0_cfg->sample_rate, NULL)) != 0) {
        fprintf(stderr,"Failed to set TX sample rate: %s\n", bladerf_strerror(status));
        brf_error(status);
    } else
        printf("[BRF] set TX sampling rate to %u \n", (unsigned int) openair0_cfg->sample_rate);

    if ((status=bladerf_set_bandwidth(brf->dev, BLADERF_MODULE_TX,(unsigned int)openair0_cfg->tx_bw*2, NULL)) != 0) {
        fprintf(stderr, "Failed to set TX bandwidth: %s\n", bladerf_strerror(status));
        brf_error(status);
    } else
        printf("[BRF] set TX bandwidth to %u \n", (unsigned int) openair0_cfg->tx_bw*2);

    if ((status=bladerf_set_gain(brf->dev, BLADERF_MODULE_TX, (int) openair0_cfg->tx_gain[0])) != 0) {
        fprintf(stderr,"Failed to set TX gain: %s\n",bladerf_strerror(status));
        brf_error(status);
    } else
        printf("[BRF] set the TX gain to %d\n", (int)openair0_cfg->tx_gain[0]);


    /* Configure the device's TX module for use with the sync interface.
      * SC16 Q11 samples *with* metadata are used. */
    if ((status=bladerf_sync_config(brf->dev, BLADERF_MODULE_TX,BLADERF_FORMAT_SC16_Q11_META,brf->num_buffers,brf->buffer_size,brf->num_transfers,brf->tx_timeout_ms)) != 0 ) {
        fprintf(stderr,"Failed to configure TX sync interface: %s\n", bladerf_strerror(status));
        brf_error(status);
    } else
        printf("[BRF] configured TX  sync interface \n");

    /* Configure the device's RX module for use with the sync interface.
       * SC16 Q11 samples *with* metadata are used. */
    if ((status=bladerf_sync_config(brf->dev, BLADERF_MODULE_RX, BLADERF_FORMAT_SC16_Q11_META,brf->num_buffers,brf->buffer_size,brf->num_transfers,brf->rx_timeout_ms)) != 0 ) {
        fprintf(stderr,"Failed to configure RX sync interface: %s\n", bladerf_strerror(status));
        brf_error(status);
    } else
        printf("[BRF] configured Rx sync interface \n");


    /* We must always enable the TX module after calling bladerf_sync_config(), and
     * before  attempting to TX samples via  bladerf_sync_tx(). */
    if ((status=bladerf_enable_module(brf->dev, BLADERF_MODULE_TX, true)) != 0) {
        fprintf(stderr,"Failed to enable TX module: %s\n", bladerf_strerror(status));
        brf_error(status);
    } else
        printf("[BRF] TX module enabled \n");

    /* We must always enable the RX module after calling bladerf_sync_config(), and
       * before  attempting to RX samples via  bladerf_sync_rx(). */
    if ((status=bladerf_enable_module(brf->dev, BLADERF_MODULE_RX, true)) != 0) {
        fprintf(stderr,"Failed to enable RX module: %s\n", bladerf_strerror(status));
        brf_error(status);
    } else
        printf("[BRF] RX module enabled \n");

    // calibrate

#if 0
    /* those calls fail for "Nuand bladeRF 2.0 (bladerf2)", so commented,
     * but let's keep the code in case it's needed (for other boards,
     * for anything, unfortunately we don't have other boards to test
     * right now)
     */
    if ((status=bladerf_calibrate_dc(brf->dev, BLADERF_DC_CAL_LPF_TUNING)) != 0 ||
            (status=bladerf_calibrate_dc(brf->dev, BLADERF_DC_CAL_TX_LPF)) != 0 ||
            (status=bladerf_calibrate_dc(brf->dev, BLADERF_DC_CAL_RX_LPF)) != 0 ||
            (status=bladerf_calibrate_dc(brf->dev, BLADERF_DC_CAL_RXVGA2)) != 0) {
        fprintf(stderr, "[BRF] error calibrating\n");
        brf_error(status);
    } else
        printf("[BRF] calibration OK\n");
#endif

    /* set log to info, available log levels are:
     * - BLADERF_LOG_LEVEL_VERBOSE
     * - BLADERF_LOG_LEVEL_DEBUG
     * - BLADERF_LOG_LEVEL_INFO
     * - BLADERF_LOG_LEVEL_WARNING
     * - BLADERF_LOG_LEVEL_ERROR
     * - BLADERF_LOG_LEVEL_CRITICAL
     * - BLADERF_LOG_LEVEL_SILENT
     */
    bladerf_log_set_verbosity(BLADERF_LOG_LEVEL_INFO);

    printf("BLADERF: Initializing openair0_device\n");
    device->Mod_id         = num_devices++;
    device->type             = BLADERF_DEV;
    device->trx_start_func = trx_brf_start;
    device->trx_end_func   = trx_brf_end;
    device->trx_read_func  = trx_brf_read;
    device->trx_write_func = trx_brf_write;
    device->trx_get_stats_func   = trx_brf_get_stats;
    device->trx_reset_stats_func = trx_brf_reset_stats;
    device->trx_stop_func        = trx_brf_stop;
    device->trx_set_freq_func    = trx_brf_set_freq;
    device->trx_set_gains_func   = trx_brf_set_gains;
    device->trx_write_init       = trx_brf_write_init;
    device->openair0_cfg = openair0_cfg;
    device->priv = (void *)brf;

    calibrate_rf(device);

    //  memcpy((void*)&device->openair0_cfg,(void*)&openair0_cfg[0],sizeof(openair0_config_t));

    if ((status=bladerf_enable_module(brf->dev, BLADERF_MODULE_TX, false)) != 0) {
        fprintf(stderr,"Failed to enable TX module: %s\n", bladerf_strerror(status));
        abort();
    }
    if ((status=bladerf_enable_module(brf->dev, BLADERF_MODULE_RX, false)) != 0) {
        fprintf(stderr,"Failed to enable RX module: %s\n", bladerf_strerror(status));
        abort();
    }

    return 0;
}


/*! \brief bladeRF error report
 * \param status
 * \returns 0 on success
 */
int brf_error(int status)
{
    fprintf(stderr, "[BRF] brf_error: %s\n", bladerf_strerror(status));
    exit(-1);
    return status; // or status error code
}


/*! \brief Open BladeRF from serial port
 * \param serial name of serial port on which to open BladeRF device
 * \returns bladerf device structure
 */
struct bladerf * open_bladerf_from_serial(const char *serial)
{
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
/*@}*/
