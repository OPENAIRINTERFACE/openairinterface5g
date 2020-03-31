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

/** iqplayer_lib.cpp
 *
 * \author:FrancoisTaburet: francois.taburet@nokia-bell-labs.com
 */
#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64
#define NB_ANTENNAS_RX  2
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/resource.h>
#include <errno.h>
#include "common_lib.h"
#include "assertions.h"
#include "common/utils/LOG/log.h"






static void parse_iqfile_header(openair0_device *device, iqfile_header_t *iq_fh) {
  AssertFatal((memcmp(iq_fh->oaiid,OAIIQFILE_ID,sizeof(OAIIQFILE_ID)) == 0),
  	           "iqfile doesn't seem to be compatible with oai (invalid id %.4s in header)\n",
  	           iq_fh->oaiid);
  device->type = iq_fh->devtype;
  device->openair0_cfg[0].tx_sample_advance=iq_fh->tx_sample_advance;
  device->openair0_cfg[0].tx_bw =  device->openair0_cfg[0].rx_bw = iq_fh->bw;
  LOG_UI(HW,"Replay iqs from %s device, bandwidth %e\n",get_devname(iq_fh->devtype),iq_fh->bw);
}


/*! \brief Called to start the iqplayer device. Return 0 if OK, < 0 if error
    @param device pointer to the device structure specific to the RF hardware target
*/
static int iqplayer_loadfile(openair0_device *device, openair0_config_t *openair0_cfg) {
  recplay_state_t *s = device->recplay_state;
  recplay_conf_t  *c = openair0_cfg->recplay_conf;

  if (s->use_mmap) {
    // use mmap
    s->mmapfd = open(c->u_sf_filename, O_RDONLY );

    if (s->mmapfd != 0) {
      struct stat sb;
      fstat(s->mmapfd, &sb);
      s->mapsize=sb.st_size;
      LOG_I(HW,"Loading subframes using mmap() from %s size=%lu bytes ...\n",c->u_sf_filename, (uint64_t)sb.st_size );
      void *mptr = mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, s->mmapfd, 0) ;
      s->ms_sample = (iqrec_t *) ( mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, s->mmapfd, 0) + sizeof(iqfile_header_t));

      if (mptr != MAP_FAILED) {
        parse_iqfile_header(device, (iqfile_header_t *)mptr);
        s->ms_sample = (iqrec_t *)((char *)mptr + sizeof(iqfile_header_t));
        s->nb_samples = ((sb.st_size-sizeof(iqfile_header_t)) / sizeof(iqrec_t));
        int aligned = (((unsigned long)s->ms_sample & 31) == 0)? 1:0;
        LOG_I(HW,"Loaded %u subframes.\n",s->nb_samples );

        if (aligned == 0) {
          LOG_E(HW, "mmap address is not 32 bytes aligned, exiting.\n" );
          close(s->mmapfd);
          exit(-1);
        }
      } else {
        LOG_E(HW,"Cannot mmap file, exiting.\n");
        close(s->mmapfd);
        exit(-1);
      }
    } else {
      LOG_E( HW,"Cannot open %s exiting.\n", c->u_sf_filename );
      exit(-1);
    }
  } else {
    s->iqfd = open(c->u_sf_filename, O_RDONLY);

    if (s->iqfd != 0) {
      struct stat sb;
      iqfile_header_t fh;
      size_t hs = read(s->iqfd,&fh,sizeof(fh));

      if (hs == sizeof(fh)) {
        parse_iqfile_header(device, &fh);
        fstat(s->iqfd, &sb);
        s->mapsize=sb.st_size;
        s->nb_samples = ((sb.st_size-sizeof(iqfile_header_t))/ sizeof(iqrec_t));
        LOG_I(HW, "Loading %u subframes from %s,size=%lu bytes ...\n",s->nb_samples, c->u_sf_filename,(uint64_t)sb.st_size);
        // allocate buffer for 1 sample at a time
        s->ms_sample = (iqrec_t *) malloc(sizeof(iqrec_t));

        if (s->ms_sample == NULL) {
          LOG_E(HW,"Memory allocation failed for individual subframe replay mode.\n" );
          close(s->iqfd);
          exit(-1);
        }

        memset(s->ms_sample, 0, sizeof(iqrec_t));

        // point at beginning of iqs in file
        if (lseek(s->iqfd,sizeof(iqfile_header_t), SEEK_SET) == 0) {
          LOG_I(HW,"Initial seek at beginning of the file\n" );
        } else {
          LOG_I(HW,"Problem initial seek at beginning of the file\n");
        }
      } else {
        LOG_E(HW,"Cannot read header in %s exiting.\n",c->u_sf_filename );
        close(s->iqfd);
        exit(-1);
      }
    } else {
      LOG_E(HW,"Cannot open %s exiting.\n",c->u_sf_filename );
      exit(-1);
    }
  }

  return 0;
}

/*! \brief start the oai iq player
 * \param device, the hardware used
 */
static int trx_iqplayer_start(openair0_device *device){
	return 0;
}

/*! \brief Terminate operation of the oai iq player
 * \param device, the hardware used
 */
static void trx_iqplayer_end(openair0_device *device) {
  if (device == NULL)
    return;

  if (device->recplay_state == NULL)
    return;

  if (device->recplay_state->use_mmap) {
    if (device->recplay_state->ms_sample != MAP_FAILED) {
      munmap(device->recplay_state->ms_sample, device->recplay_state->mapsize);
      device->recplay_state->ms_sample = NULL;
    }

    if (device->recplay_state->mmapfd != 0) {
      close(device->recplay_state->mmapfd);
      device->recplay_state->mmapfd = 0;
    }
  } else {
    if (device->recplay_state->ms_sample != NULL) {
      free(device->recplay_state->ms_sample);
      device->recplay_state->ms_sample = NULL;
    }

    if (device->recplay_state->iqfd != 0) {
      close(device->recplay_state->iqfd);
      device->recplay_state->iqfd = 0;
    }
  }
}
/*! \brief Write iqs function when in replay mode, just introduce a delay, as configured at init time,
      @param device pointer to the device structure specific to the RF hardware target
      @param timestamp The timestamp at which the first sample MUST be sent
      @param buff Buffer which holds the samples
      @param nsamps number of samples to be sent
      @param antenna_id index of the antenna if the device has multiple antennas
      @param flags flags must be set to TRUE if timestamp parameter needs to be applied
*/
static int trx_iqplayer_write(openair0_device *device, openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags) {
  struct timespec req;
  req.tv_sec = 0;
  req.tv_nsec = device->openair0_cfg->recplay_conf->u_sf_write_delay * 1000;
  nanosleep(&req, NULL);
  return nsamps;
}

/*! \brief Receive samples from iq file.
 * Read \ref nsamps samples from each channel to buffers. buff[0] is the array for
 * the first channel. *ptimestamp is the time at which the first sample
 * was received.
 * \param device the hardware to use
 * \param[out] ptimestamp the time at which the first sample was received.
 * \param[out] buff An array of pointers to buffers for received samples. The buffers must be large enough to hold the number of samples \ref nsamps.
 * \param nsamps Number of samples. One sample is 2 byte I + 2 byte Q => 4 byte.
 * \param antenna_id Index of antenna for which to receive samples
 * \returns the number of sample read
*/
static int trx_iqplayer_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int cc) {
  int samples_received=0;
  static unsigned int    cur_samples;
  static int64_t         wrap_count;
  static int64_t  wrap_ts;
  recplay_state_t *s = device->recplay_state;

  if (cur_samples == s->nb_samples) {
    cur_samples = 0;
    wrap_count++;

    if (wrap_count == device->openair0_cfg->recplay_conf->u_sf_loops) {
      LOG_W(HW, "iqplayer device terminating subframes replay  after %u iteration\n",device->openair0_cfg->recplay_conf->u_sf_loops);
      exit_function(__FILE__, __FUNCTION__, __LINE__,"replay ended, triggering process termination\n");
    }

    wrap_ts = wrap_count * (s->nb_samples * (((int)(device->openair0_cfg[0].sample_rate)) / 1000));

    if (!device->recplay_state->use_mmap) {
      if (lseek(device->recplay_state->iqfd, 0, SEEK_SET) == 0) {
        LOG_I(HW,"Seeking at the beginning of IQ file");
      } else {
        LOG_I(HW, "Problem seeking at the beginning of IQ file");
      }
    }
  }

  if (s->use_mmap) {
    if (cur_samples < s->nb_samples) {
      *ptimestamp = (s->ms_sample[0].ts + (cur_samples * (((int)(device->openair0_cfg[0].sample_rate)) / 1000))) + wrap_ts;

      if (cur_samples == 0) {
        LOG_I(HW,"starting subframes file with wrap_count=%lu wrap_ts=%lu ts=%lu\n", wrap_count,wrap_ts,*ptimestamp);
      }

      memcpy(buff[0], &s->ms_sample[cur_samples].samples[0], nsamps*4);
      cur_samples++;
    }
  } else {
    // read sample from file
    if (read(s->iqfd, s->ms_sample, sizeof(iqrec_t)) != sizeof(iqrec_t)) {
      LOG_E(HW,"pb reading iqfile at index %lu\n",sizeof(iqrec_t)*cur_samples );
      close(s->iqfd);
      free(s->ms_sample);
      s->ms_sample = NULL;
      s->iqfd = 0;
      exit(-1);
    }

    if (cur_samples < s->nb_samples) {
      static int64_t ts0 = 0;

      if ((cur_samples == 0) && (wrap_count == 0)) {
        ts0 = s->ms_sample->ts;
      }

      *ptimestamp = ts0 + (cur_samples * (((int)(device->openair0_cfg[0].sample_rate)) / 1000)) + wrap_ts;

      if (cur_samples == 0) {
        LOG_I(HW, "starting subframes file with wrap_count=%lu wrap_ts=%lu ts=%lu ",wrap_count,wrap_ts, *ptimestamp);
      }

      memcpy(buff[0], &s->ms_sample->samples[0], nsamps*4);
      cur_samples++;
      // Prepare for next read
      off_t where = lseek(s->iqfd, cur_samples * sizeof(iqrec_t), SEEK_SET);

      if (where < 0) {
        LOG_E(HW,"Cannot lseek in iqfile: %s\n",strerror(errno));
        exit(-1);
      }
    }
  }

  struct timespec req;

  req.tv_sec = 0;

  req.tv_nsec = (device->openair0_cfg[0].recplay_conf->u_sf_read_delay) * 1000;

  nanosleep(&req, NULL);

  return nsamps;

  return samples_received;
}


int device_init(openair0_device *device, openair0_config_t *openair0_cfg) {
  device->openair0_cfg = openair0_cfg;
  device->trx_start_func = trx_iqplayer_start;
  device->trx_get_stats_func = NULL;
  device->trx_reset_stats_func = NULL;
  device->trx_end_func   = trx_iqplayer_end;
  device->trx_stop_func  = NULL;
  device->trx_set_freq_func = NULL;
  device->trx_set_gains_func   = NULL;
  // Replay subframes from from file
  //  openair0_cfg[0].rx_gain_calib_table = calib_table_b210_38;
  //  bw_gain_adjust=1;
  device->trx_write_func = trx_iqplayer_write;
  device->trx_read_func  = trx_iqplayer_read;
  iqplayer_loadfile(device, openair0_cfg);
  LOG_UI(HW,"iqplayer device initialized, replay %s  for %i iterations",openair0_cfg->recplay_conf->u_sf_filename,openair0_cfg->recplay_conf->u_sf_loops);
  return 0;
}

/*@}*/
