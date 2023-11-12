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

/** record_player.c
 *
 * \author: Nokia bellLabs B. Mongazon F. Taburet
 *
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include "common/utils/LOG/log.h"
#include "assertions.h"
#include "common_lib.h"
#include "record_player.h"

/*! \brief read the oai recorder or player configuration, called from common device code
 * \param recplay_conf:  store parameters
 * \param recplay_state: store recorder or player data while the device runs
 */
int read_recplayconfig(recplay_conf_t **recplay_conf, recplay_state_t **recplay_state) {
  *recplay_conf = calloc(sizeof(recplay_conf_t),1);
  paramdef_t device_recplay_params[]=DEVICE_RECPLAY_PARAMS_DESC ;
  config_get(config_get_if(), device_recplay_params, sizeofArray(device_recplay_params), DEVICE_RECPLAY_SECTION);

  if ((*recplay_conf)->u_sf_record || (*recplay_conf)->u_sf_replay ) {
    struct sysinfo systeminfo;
    *recplay_state = calloc(sizeof(recplay_state_t),1);

    if ( *recplay_state == NULL) {
      LOG_E(HW,"calloc error in %s\n", __FILE__);
      return -1;
    }

    // Use mmap for IQ files for systems with less than 6GB total RAM
    sysinfo(&systeminfo);

    if (systeminfo.totalram < 6144000000 && ((*recplay_conf)->use_mmap == 1)) {
	  LOG_W(HW,"System with %f GB of mem (<6GB), mmap usage disabled\n",systeminfo.totalram/10E9);
      (*recplay_conf)->use_mmap = 0;
    }
  } else { /* player-recorder disable */
    free(*recplay_conf);
    *recplay_conf=NULL;
    return RECPLAY_DISABLED;
  }

  if ((*recplay_conf)->u_sf_replay == 1)
    return RECPLAY_REPLAYMODE;
  else if ((*recplay_conf)->u_sf_record == 1)
    return RECPLAY_RECORDMODE;

  return RECPLAY_DISABLED;
}

/*! \brief Terminate operation of the oai iq recorder. to be called by any device
 * used in record mode
 * \param device the hardware used
 */
void iqrecorder_end(openair0_device *device) {
  if (device->recplay_state != NULL) { // subframes store
    iqfile_header_t fh = {device->type,device->openair0_cfg->tx_sample_advance, device->openair0_cfg->rx_bw,0,OAIIQFILE_ID};
    recplay_state_t *rs = device->recplay_state;
    recplay_conf_t *rc = device->openair0_cfg[0].recplay_conf;
    rs->pFile = fopen (rc->u_sf_filename,"wb+");

    if (rs->pFile == NULL) {
      LOG_E(HW,"Cannot open %s\n", rc->u_sf_filename);
    } else {
      unsigned int i = 0;
      fh.nbSamplesBlocks=rs->nbSamplesBlocks;
      LOG_I(HW,"Writing file header to %s \n", rc->u_sf_filename );
      fwrite(&fh, sizeof(fh), 1, rs->pFile);
      LOG_UI(HW,"Writing %u subframes to %s \n",rs->nbSamplesBlocks, rc->u_sf_filename );
      uint8_t *ptr=(uint8_t *)rs->ms_sample;

      for (i = 0; i < rs->nbSamplesBlocks; i++) {
        int blockBytes=sizeof(iqrec_t)+BELL_LABS_IQ_BYTES_PER_SF;
        fwrite(ptr, sizeof(unsigned char), blockBytes, rs->pFile);
        ptr+=blockBytes;
      }

      fclose (rs->pFile);
      LOG_UI(HW,"File %s closed\n",rc->u_sf_filename );
    }

    if (rs->ms_sample != NULL) {
      free((void *)rs->ms_sample);
      rs->ms_sample = NULL;
    }
  }
}
