/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

int set_device(openair0_device *device) {

  switch (device->type) {
    
  case EXMIMO_DEV:
    printf("[%s] has loaded EXPRESS MIMO device.\n",((device->host_type == BBU_HOST) ? "BBU": "RRH"));
    break;
  case USRP_B200_DEV:
    printf("[%s] has loaded USRP B200 device.\n",((device->host_type == BBU_HOST) ? "BBU": "RRH")); 
    break;
case USRP_X300_DEV:
    printf("[%s] has loaded USRP X300 device.\n",((device->host_type == BBU_HOST) ? "BBU": "RRH")); 
    break;
  case BLADERF_DEV:
    printf("[%s] has loaded BLADERF device.\n",((device->host_type == BBU_HOST) ? "BBU": "RRH")); 
    break;
  case LMSSDR_DEV:
    printf("[%s] has loaded LMSSDR device.\n",((device->host_type == BBU_HOST) ? "BBU": "RRH")); 
    break;
  case NONE_DEV:
    printf("[%s] has not loaded a HW device.\n",((device->host_type == BBU_HOST) ? "BBU": "RRH"));
    break;    
  default:
    printf("[%s] invalid HW device.\n",((device->host_type == BBU_HOST) ? "BBU": "RRH")); 
    return -1;
  }
  return 0;
}

int set_transport(openair0_device *device) {

  switch (device->transp_type) {
    
  case ETHERNET_TP:
    printf("[%s] has loaded ETHERNET trasport protocol.\n",((device->host_type == BBU_HOST) ? "BBU": "RRH"));
    return 0;     
    break;
  case NONE_TP:
    printf("[%s] has not loaded a transport protocol.\n",((device->host_type == BBU_HOST) ? "BBU": "RRH"));
    return 0; 
    break;    
  default:
    printf("[%s] invalid transport protocol.\n",((device->host_type == BBU_HOST) ? "BBU": "RRH")); 
    return -1;
    break;
  }
  
}

/* look for the interface library and load it */
int load_lib(openair0_device *device, openair0_config_t *openair0_cfg, eth_params_t * cfg, uint8_t flag) {
  
  void *lib_handle;
  oai_device_initfunc_t dp ;
  oai_transport_initfunc_t tp ;
  int ret=0;

  if (flag == BBU_LOCAL_RADIO_HEAD) {
      lib_handle = dlopen(OAI_RF_LIBNAME, RTLD_LAZY);
      if (!lib_handle) {
	fprintf(stderr,"Unable to locate %s: HW device set to NONE_DEV.\n", OAI_RF_LIBNAME);
	fprintf(stderr,"%s\n",dlerror());
	return -1;
      } 
      
      dp = dlsym(lib_handle,"device_init");
      
      if (dp != NULL ) {
	ret = dp(device,openair0_cfg);
	if (ret<0) {
	  fprintf(stderr, "%s %d:oai device intialization failed %s\n", __FILE__, __LINE__, dlerror());
	}
      } else {
	fprintf(stderr, "%s %d:oai device intializing function not found %s\n", __FILE__, __LINE__, dlerror());
	return -1;
      }
    } else {
      lib_handle = dlopen(OAI_TP_LIBNAME, RTLD_LAZY);
      if (!lib_handle) {
	printf( "Unable to locate %s: transport protocol set to NONE_TP.\n", OAI_TP_LIBNAME);
	printf( "%s\n",dlerror());
	return -1;
      } 
      
      tp = dlsym(lib_handle,"transport_init");
      
      if (tp != NULL ) {
	tp(device,openair0_cfg,cfg);
      } else {
	fprintf(stderr, "%s %d:oai device intializing function not found %s\n", __FILE__, __LINE__, dlerror());
	return -1;
      }
    } 
    
  return ret; 	       
}



int openair0_device_load(openair0_device *device, openair0_config_t *openair0_cfg) {
  
  int rc=0;
  rc=load_lib(device, openair0_cfg, NULL,BBU_LOCAL_RADIO_HEAD );
  if ( rc >= 0) {       
    if ( set_device(device) < 0) {
      fprintf(stderr, "%s %d:Unsupported radio head\n",__FILE__, __LINE__);
      return -1;		   
    }   
  }
  return rc;
}

int openair0_transport_load(openair0_device *device, openair0_config_t *openair0_cfg, eth_params_t * eth_params) {
  int rc;
  rc=load_lib(device, openair0_cfg, eth_params, BBU_REMOTE_RADIO_HEAD);
  if ( rc >= 0) {       
    if ( set_transport(device) < 0) {
      fprintf(stderr, "%s %d:Unsupported transport protocol\n",__FILE__, __LINE__);
      return -1;		   
      }   
  }
  return rc;

}







