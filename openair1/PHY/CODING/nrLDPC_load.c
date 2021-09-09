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

/*! \file openair1/PHY/CODING/coding_nr_load.c
 * \brief: load library implementing coding/decoding algorithms
 * \author Francois TABURET
 * \date 2020
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#define _GNU_SOURCE 
#include <sys/types.h>
#include <stdlib.h>
#include <malloc.h>
#include "assertions.h"
#include "common/utils/LOG/log.h"
#define LDPC_LOADER
#include "PHY/CODING/nrLDPC_extern.h"
#include "common/config/config_userapi.h" 
#include "common/utils/load_module_shlib.h" 


/* function description array, to be used when loading the encoding/decoding shared lib */
static loader_shlibfunc_t shlib_fdesc[3];

/* arguments used when called from phy simulators exec's which do not use the config module */
/* arg is used to initialize the config module so that the loader works as expected */
char *arg[64]={"ldpctest","-O","cmdlineonly::dbgl0",NULL,NULL};

int load_nrLDPClib(int run_cuda) {
	 char *ptr = (char*)config_get_if();
	 char libname[64]="ldpc";
	 int argc=3;
	 if (run_cuda) {
         arg[3]="--loader.ldpc.shlibversion";
         argc++;
         arg[4]="_cuda";
         argc++;
     }
     if ( ptr==NULL )  {// phy simulators, config module possibly not loaded
     	 load_configmodule(argc,(char **)arg,CONFIG_ENABLECMDLINEONLY) ;
     	 logInit();
     }	 
     shlib_fdesc[0].fname = "nrLDPC_decod";
     shlib_fdesc[1].fname = "nrLDPC_encod";
     shlib_fdesc[2].fname = "nrLDPC_initcall";
     int ret=load_module_shlib(libname,shlib_fdesc,sizeof(shlib_fdesc)/sizeof(loader_shlibfunc_t),NULL);
     AssertFatal( (ret >= 0),"Error loading ldpc decoder");
     nrLDPC_decoder = (nrLDPC_decoderfunc_t)shlib_fdesc[0].fptr;
     nrLDPC_encoder = (nrLDPC_encoderfunc_t)shlib_fdesc[1].fptr;
     nrLDPC_initcall = (nrLDPC_initcallfunc_t)shlib_fdesc[2].fptr;
return 0;
}

int load_nrLDPClib_ref(char *libversion, nrLDPC_encoderfunc_t * nrLDPC_encoder_ptr) {
	loader_shlibfunc_t shlib_encoder_fdesc;

     shlib_encoder_fdesc.fname = "nrLDPC_encod";
     char libpath[64];
     sprintf(libpath,"ldpc%s",libversion);
     int ret=load_module_shlib(libpath,&shlib_encoder_fdesc,1,NULL);
     AssertFatal( (ret >= 0),"Error loading ldpc encoder %s\n",libpath);
     *nrLDPC_encoder_ptr = (nrLDPC_encoderfunc_t)shlib_encoder_fdesc.fptr;
return 0;
}


