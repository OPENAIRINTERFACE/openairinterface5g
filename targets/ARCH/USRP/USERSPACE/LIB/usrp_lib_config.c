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

/** usrp_lib_config.c
 *
 * \author: HongliangXU : hong-liang-xu@agilent.com
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include "common/utils/LOG/log.h"
#include "assertions.h"
#include "common_lib.h"
#include "usrp_lib.h"


int read_usrpconfig(uint32_t *recplay_mode, recplay_state_t **recplay_state) {
unsigned int    u_sf_record = 0;                       // record mode
unsigned int    u_sf_replay = 0;                       // replay mode
uint32_t enable_recplay;

    paramdef_t usrp_params[] = USRP_DEVICE_PARAMS_DESC;
    config_get(usrp_params,sizeof(usrp_params)/sizeof(paramdef_t),USRP_SECTION);
    if (enable_recplay) {
      *recplay_state = calloc(sizeof(recplay_state_t),1);      
      paramdef_t usrp_recplay_params[]=USRP_RECPLAY_PARAMS_DESC ;
      struct sysinfo systeminfo;
 
    // Use mmap for IQ files for systems with less than 6GB total RAM
      sysinfo(&systeminfo);

      if (systeminfo.totalram < 6144000000) {
        (*recplay_state)->use_mmap = 0;
      } else {
        (*recplay_state)->use_mmap = 1;
      }

      memset((*recplay_state)->u_sf_filename, 0, 1024);
      config_get(usrp_recplay_params,sizeof(usrp_recplay_params)/sizeof(paramdef_t),USRP_RECPLAY_SECTION);

      if (strlen((*recplay_state)->u_sf_filename) == 0) {
        (void) strcpy((
        	*recplay_state)->u_sf_filename, DEF_SF_FILE);
      }
    } /* record player enabled */
    if (u_sf_replay == 1) *recplay_mode = RECPLAY_REPLAYMODE;

    if (u_sf_record == 1) *recplay_mode = RECPLAY_RECORDMODE;


    return 0;
  }