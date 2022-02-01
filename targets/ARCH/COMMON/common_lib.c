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

/*! \file common_lib.c
 * \brief common APIs for different RF frontend device
 * \author HongliangXU, Navid Nikaein
 * \date 2015
 * \version 0.2
 * \company Eurecom
 * \maintainer:  navid.nikaein@eurecom.fr
 * \note
 * \warning
 */
#include <stdio.h>
#include <strings.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "common_lib.h"
#include "assertions.h"
#include "common/utils/load_module_shlib.h"
#include "common/utils/LOG/log.h"
//#include "targets/RT/USER/lte-softmodem.h"
#include "executables/softmodem-common.h"

char *get_devname(int devtype) {
char *devnames[MAX_RF_DEV_TYPE]=DEVTYPE_NAMES;
  if (devtype < MAX_RF_DEV_TYPE && devtype !=MIN_RF_DEV_TYPE )
  	  return devnames[devtype];
  return "none";
}

int set_device(openair0_device *device)
{
  char *devname = get_devname(device->type);
    if (strcmp(devname,"none") != 0) {
      LOG_I(HW,"[%s] has loaded %s device.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"),devname);
    } else {
      LOG_E(HW,"[%s] invalid HW device.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      return -1;
    }
  return 0;
}

int set_transport(openair0_device *device)
{
  switch (device->transp_type) {
    case ETHERNET_TP:
      LOG_I(HW,"[%s] has loaded ETHERNET trasport protocol.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      return 0;
      break;

    case NONE_TP:
      LOG_I(HW,"[%s] has not loaded a transport protocol.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      return 0;
      break;

    default:
      LOG_E(HW,"[%s] invalid transport protocol.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      return -1;
      break;
  }
}

typedef int(*devfunc_t)(openair0_device *, openair0_config_t *, eth_params_t *);


/* look for the interface library and load it */
int load_lib(openair0_device *device,
             openair0_config_t *openair0_cfg,
             eth_params_t *cfg,
             uint8_t flag)
{
  loader_shlibfunc_t shlib_fdesc[1];
  int ret=0;
  char *deflibname=OAI_RF_LIBNAME;
  
  openair0_cfg->recplay_mode = read_recplayconfig(&(openair0_cfg->recplay_conf),&(device->recplay_state));

  if ( openair0_cfg->recplay_mode == RECPLAY_REPLAYMODE ) {
  	  deflibname=OAI_IQPLAYER_LIBNAME;
  	  shlib_fdesc[0].fname="device_init";
  	  set_softmodem_optmask(SOFTMODEM_RECPLAY_BIT);  // softmodem has to know we use the iqplayer to workaround randomized algorithms
  } else  if ( IS_SOFTMODEM_BASICSIM ) {
	  deflibname=OAI_BASICSIM_LIBNAME;
	  shlib_fdesc[0].fname="device_init";
  } else if (IS_SOFTMODEM_RFSIM && flag == RAU_LOCAL_RADIO_HEAD) {
	  deflibname=OAI_RFSIM_LIBNAME;
	  shlib_fdesc[0].fname="device_init";
  } else if (flag == RAU_LOCAL_RADIO_HEAD) {
	  if (IS_SOFTMODEM_RFSIM)
		  deflibname="rfsimulator";
	  else
          deflibname=OAI_RF_LIBNAME;
      shlib_fdesc[0].fname="device_init";
  } else if (flag == RAU_REMOTE_THIRDPARTY_RADIO_HEAD) {
    deflibname=OAI_THIRDPARTY_TP_LIBNAME;
    shlib_fdesc[0].fname="transport_init";
  } else {
	  deflibname=OAI_TP_LIBNAME;
	  shlib_fdesc[0].fname="transport_init";
  }
  
  char *devname=NULL;
  paramdef_t device_params[]=DEVICE_PARAMS_DESC ;
  int numparams = sizeof(device_params)/sizeof(paramdef_t);
  int devname_pidx = config_paramidx_fromname(device_params,numparams, CONFIG_DEVICEOPT_NAME);
  device_params[devname_pidx].defstrval=deflibname;
  
  config_get(device_params,numparams,DEVICE_SECTION);
  
  ret=load_module_shlib(devname,shlib_fdesc,1,NULL);
  AssertFatal( (ret >= 0),
  	           "Library %s couldn't be loaded\n",devname);

  return ((devfunc_t)shlib_fdesc[0].fptr)(device,openair0_cfg,cfg);
}


int openair0_device_load(openair0_device *device,
                         openair0_config_t *openair0_cfg)
{
  int rc=0;
  rc=load_lib(device, openair0_cfg, NULL,RAU_LOCAL_RADIO_HEAD );

  if ( rc >= 0) {
    if ( set_device(device) < 0) {
      LOG_E(HW, "%s %d:Unsupported radio head\n", __FILE__, __LINE__);
      return -1;
	}
  } else
    AssertFatal(false, "can't open the radio device: %s\n", get_devname(device->type));

  return rc;
}


int openair0_transport_load(openair0_device *device,
                            openair0_config_t *openair0_cfg,
                            eth_params_t *eth_params)
{
  int rc;
  rc=load_lib(device, openair0_cfg, eth_params, RAU_REMOTE_RADIO_HEAD);

  if ( rc >= 0) {
    if ( set_transport(device) < 0) {
      LOG_E(HW, "%s %d:Unsupported transport protocol\n", __FILE__, __LINE__);
      return -1;
    }
  }

  return rc;
}
