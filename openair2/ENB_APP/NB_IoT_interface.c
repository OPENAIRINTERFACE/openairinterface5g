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

/*! \file openair2/ENB_APP/NB_IoT_interface.c
 * \brief: load library implementing coding/decoding algorithms
 * \date 2018
 * \version 0.1
 * \note
 * \warning
 */
#define _GNU_SOURCE 
#include <sys/types.h>


#include "openair1/PHY/phy_extern.h"
#include "common/utils/load_module_shlib.h" 
#define NBIOT_INTERFACE_SOURCE
#include "NB_IoT_interface.h"




int load_NB_IoT(void) {
 int ret;
 RCConfig_NbIoT_f_t RCConfig;
 loader_shlibfunc_t shlib_fdesc[]=NBIOT_INTERFACE_FLIST; 

     ret=load_module_shlib(NBIOT_MODULENAME,shlib_fdesc,sizeof(shlib_fdesc)/sizeof(loader_shlibfunc_t),NULL);
     if (ret) {
        return ret;
     }
     RCConfig = get_shlibmodule_fptr(NBIOT_MODULENAME,NBIOT_RCCONFIG_FNAME );
     if (RCConfig == NULL) {
        return -1;
     } 
 
     RCConfig(&RC);
return 0;
}

