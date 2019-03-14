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

#include "common_lib.h"
#include "common/utils/load_module_shlib.h"
#include "targets/RT/USER/lte-softmodem.h"

int set_device(openair0_device *device) {
  switch (device->type) {
    case EXMIMO_DEV:
      printf("[%s] has loaded EXPRESS MIMO device.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      break;

    case USRP_B200_DEV:
      printf("[%s] has loaded USRP B200 device.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      break;

    case USRP_X300_DEV:
      printf("[%s] has loaded USRP X300 device.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      break;

    case BLADERF_DEV:
      printf("[%s] has loaded BLADERF device.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      break;

    case LMSSDR_DEV:
      printf("[%s] has loaded LMSSDR device.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      break;

    case IRIS_DEV:
      printf("[%s] has loaded Iris device.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      break;

    case NONE_DEV:
      printf("[%s] has not loaded a HW device.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      break;

    default:
      printf("[%s] invalid HW device.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      return -1;
  }

  return 0;
}

int set_transport(openair0_device *device) {
  switch (device->transp_type) {
    case ETHERNET_TP:
      printf("[%s] has loaded ETHERNET trasport protocol.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      return 0;
      break;

    case NONE_TP:
      printf("[%s] has not loaded a transport protocol.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      return 0;
      break;

    default:
      printf("[%s] invalid transport protocol.\n",((device->host_type == RAU_HOST) ? "RAU": "RRU"));
      return -1;
      break;
  }
}
typedef int(*devfunc_t)(openair0_device *, openair0_config_t *, eth_params_t *);
/* look for the interface library and load it */
int load_lib(openair0_device *device, openair0_config_t *openair0_cfg, eth_params_t *cfg, uint8_t flag) {
  loader_shlibfunc_t shlib_fdesc[1];
  int ret=0;
  char *libname;

  if ( IS_SOFTMODEM_BASICSIM ) {
    libname=OAI_BASICSIM_LIBNAME;
    shlib_fdesc[0].fname="device_init";
  } else if ( IS_SOFTMODEM_RFSIM ) {
    libname=OAI_RFSIM_LIBNAME;
    shlib_fdesc[0].fname="device_init";
  } else if (flag == RAU_LOCAL_RADIO_HEAD) {
    libname=OAI_RF_LIBNAME;
    shlib_fdesc[0].fname="device_init";
  } else {
    libname=OAI_TP_LIBNAME;
    shlib_fdesc[0].fname="transport_init";
  }

  ret=load_module_shlib(libname,shlib_fdesc,1,NULL);

  if (ret < 0) {
    fprintf(stderr,"Library %s couldn't be loaded\n",libname);
  } else {
    ret=((devfunc_t)shlib_fdesc[0].fptr)(device,openair0_cfg,cfg);
  }

  return ret;
}



int openair0_device_load(openair0_device *device, openair0_config_t *openair0_cfg) {
  int rc=0;
  rc=load_lib(device, openair0_cfg, NULL,RAU_LOCAL_RADIO_HEAD );

  if ( rc >= 0) {
    if ( set_device(device) < 0) {
      fprintf(stderr, "%s %d:Unsupported radio head\n",__FILE__, __LINE__);
      return -1;
    }
  }

  return rc;
}

int openair0_transport_load(openair0_device *device, openair0_config_t *openair0_cfg, eth_params_t *eth_params) {
  int rc;
  rc=load_lib(device, openair0_cfg, eth_params, RAU_REMOTE_RADIO_HEAD);

  if ( rc >= 0) {
    if ( set_transport(device) < 0) {
      fprintf(stderr, "%s %d:Unsupported transport protocol\n",__FILE__, __LINE__);
      return -1;
    }
  }

  return rc;
}







