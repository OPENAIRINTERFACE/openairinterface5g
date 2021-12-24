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

/*! \file lte-softmodem-common.c
 * \brief common code for 5G and LTE softmodem main xNB and UEs source (nr-softmodem.c, lte-softmodem.c...)
 * \author Nokia BellLabs France, francois Taburet
 * \date 2020
 * \version 0.1
 * \company Nokia BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#include <time.h>
#include <dlfcn.h>
#include <sys/resource.h>
#include "UTIL/OPT/opt.h"
#include "common/config/config_userapi.h"
#include "common/utils/load_module_shlib.h"
#include "common/utils/telnetsrv/telnetsrv.h"
#include "executables/thread-common.h"
#include "common/utils/LOG/log.h"
#include "softmodem-common.h"
#include "nfapi/oai_integration/vendor_ext.h"


static softmodem_params_t softmodem_params;
char *parallel_config=NULL;
char *worker_config=NULL;
msc_interface_t msc_interface;
int usrp_tx_thread = 0;

uint8_t nfapi_mode=0;

static mapping softmodem_funcs[] = MAPPING_SOFTMODEM_FUNCTIONS;
static struct timespec start;

uint64_t get_softmodem_optmask(void) {
  return softmodem_params.optmask;
}

uint64_t set_softmodem_optmask(uint64_t bitmask) {
  softmodem_params.optmask = softmodem_params.optmask | bitmask;
  return softmodem_params.optmask;
}

softmodem_params_t *get_softmodem_params(void) {
  return &softmodem_params;
}

int32_t check_execmask(uint64_t execmask) {
  char *softmodemfunc=map_int_to_str(softmodem_funcs, execmask);
  if (softmodemfunc != NULL) {
  	  set_softmodem_optmask(execmask);
  	  return 0;
  } 
  return -1;
}
char *get_softmodem_function(uint64_t *sofmodemfunc_mask_ptr) {
  uint64_t fmask=(get_softmodem_optmask()&SOFTMODEM_FUNC_BITS);
  char *softmodemfunc=map_int_to_str(softmodem_funcs, fmask);
  if (sofmodemfunc_mask_ptr != NULL)
  	  *sofmodemfunc_mask_ptr=fmask;
  if (softmodemfunc != NULL) {
  	  return softmodemfunc;
  }
  return "???";
}

void get_common_options(uint32_t execmask) {
  uint32_t online_log_messages=0;
  uint32_t glog_level=0 ;
  uint32_t start_telnetsrv = 0, start_telnetclt = 0;
  uint32_t noS1 = 0, nokrnmod = 1, nonbiot = 0;
  uint32_t rfsim = 0, basicsim = 0, do_forms = 0;
  char *logmem_filename = NULL;
  paramdef_t cmdline_params[] =CMDLINE_PARAMS_DESC ;
  paramdef_t cmdline_logparams[] =CMDLINE_LOGPARAMS_DESC ;
  checkedparam_t cmdline_log_CheckParams[] = CMDLINE_LOGPARAMS_CHECK_DESC;
  check_execmask(execmask);
  config_get( cmdline_params,sizeof(cmdline_params)/sizeof(paramdef_t),NULL);
  
  int numparams=sizeof(cmdline_logparams)/sizeof(paramdef_t);
  config_set_checkfunctions(cmdline_logparams, cmdline_log_CheckParams,numparams);
  config_get( cmdline_logparams,numparams,NULL);
  
  if(config_isparamset(cmdline_logparams,config_paramidx_fromname(cmdline_logparams,numparams, CONFIG_FLOG_OPT))) {
    set_glog_onlinelog(online_log_messages);
  }

  if(config_isparamset(cmdline_logparams,config_paramidx_fromname(cmdline_logparams,numparams, CONFIG_LOGL_OPT))) {
    set_glog(glog_level);
  }

  if (start_telnetsrv) {
    load_module_shlib("telnetsrv",NULL,0,NULL);
  }
  
  if (start_telnetclt) {
    set_softmodem_optmask(SOFTMODEM_TELNETCLT_BIT);
  }

  if (logmem_filename != NULL && strlen(logmem_filename) > 0) {
    log_mem_filename = &logmem_filename[0];
    log_mem_flag = 1;
    printf("Enabling OPT for log save at memory %s\n",log_mem_filename);
    logInit_log_mem();
  }

  if (noS1) {
    set_softmodem_optmask(SOFTMODEM_NOS1_BIT);
  }

  if (nokrnmod) {
    printf("nokrnmod bit enabled \n");
    set_softmodem_optmask(SOFTMODEM_NOKRNMOD_BIT);
  }

  if (nonbiot) {
    set_softmodem_optmask(SOFTMODEM_NONBIOT_BIT);
  }

  if (rfsim) {
    set_softmodem_optmask(SOFTMODEM_RFSIM_BIT);
  }

  if (basicsim) {
    set_softmodem_optmask(SOFTMODEM_BASICSIM_BIT);
  }

  if (do_forms) {
    set_softmodem_optmask(SOFTMODEM_DOSCOPE_BIT);
  }

  if(parallel_config != NULL) set_parallel_conf(parallel_config);

  if(worker_config != NULL)   set_worker_conf(worker_config);
  nfapi_setmode(nfapi_mode);
}
void softmodem_printresources(int sig, telnet_printfunc_t pf) {
  struct rusage usage;
  struct timespec stop;

  clock_gettime(CLOCK_BOOTTIME, &stop);

  uint64_t elapse = (stop.tv_sec - start.tv_sec) ;   // in seconds


  int st = getrusage(RUSAGE_SELF,&usage);
  if (!st) {
    pf("\nRun time: %lluh %llus\n",(unsigned long long)elapse/3600,(unsigned long long)(elapse - (elapse/3600)));
    pf("\tTime executing user inst.: %lds %ldus\n",(long)usage.ru_utime.tv_sec,(long)usage.ru_utime.tv_usec);
    pf("\tTime executing system inst.: %lds %ldus\n",(long)usage.ru_stime.tv_sec,(long)usage.ru_stime.tv_usec);
    pf("\tMax. Phy. memory usage: %ldkB\n",(long)usage.ru_maxrss);
    pf("\tPage fault number (no io): %ld\n",(long)usage.ru_minflt);
    pf("\tPage fault number (requiring io): %ld\n",(long)usage.ru_majflt);
    pf("\tNumber of file system read: %ld\n",(long)usage.ru_inblock);
    pf("\tNumber of filesystem write: %ld\n",(long)usage.ru_oublock);
    pf("\tNumber of context switch (process origin, io...): %ld\n",(long)usage.ru_nvcsw);
    pf("\tNumber of context switch (os origin, priority...): %ld\n",(long)usage.ru_nivcsw);
  }
}

void signal_handler(int sig) {
  //void *array[10];
  //size_t size;

  if (sig==SIGSEGV) {
    // get void*'s for all entries on the stack
    /* backtrace uses malloc, that is not good in signal handlers
     * I let the code, because it would be nice to make it better
    size = backtrace(array, 10);
    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, 2);
    */
    exit(-1);
  } else {
    if(sig==SIGINT ||sig==SOFTMODEM_RTSIGNAL)
      softmodem_printresources(sig,(telnet_printfunc_t)printf);
    if (sig != SOFTMODEM_RTSIGNAL) {
      printf("Linux signal %s...\n",strsignal(sig));
      exit_function(__FILE__, __FUNCTION__, __LINE__,"softmodem starting exit procedure\n");
    }
  }
}



void set_softmodem_sighandler(void) {
  struct sigaction  act,oldact;
  clock_gettime(CLOCK_BOOTTIME, &start);
  memset(&act,0,sizeof(act));
  act.sa_handler=signal_handler;
  sigaction(SOFTMODEM_RTSIGNAL,&act,&oldact);
  printf("Send signal %d to display resource usage...\n",SIGRTMIN+1);
  signal(SIGSEGV, signal_handler);
  signal(SIGINT,  signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGABRT, signal_handler);
}

