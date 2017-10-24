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


/*! \file rrh_gw.h
 * \brief top-level for the remote radio head gateway (RRH_gw) module reusing the ethernet library 
 * \author Navid Nikaein,  Katerina Trilyraki, Raymond Knopp
 * \date 2015
 * \version 0.1
 * \company Eurecom
 * \maintainer:  navid.nikaein@eurecom.fr
 * \note
 * \warning very experimental 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <time.h>

#include "common_lib.h"
#include "rrh_gw.h"
#include "rt_wrapper.h"
#include "rrh_gw_externs.h"


#include "log_if.h"
#include "log_extern.h"
#include "vcd_signal_dumper.h"

/*****************************************************************************************
 *                                                                        ----------     *
 *     -------    RRH_BBU_IF    -------    RRH_RF_IF     -------USRP      - COTS_UE-     *
 *     - BBU - ---------------  - RRH -  -------------   -------BLADERF   ----------     *
 *     -------                  -------                  -------EXMIMO                   *
 *                                                                        ---------      *
 *                                                       -------ETH_IF    - EMU_UE-      *
 *                                                                        ---------      *                              
 *****************************************************************************************/


/* local IP/MAC address is detected*/
char rrh_ip[20] = "0.0.0.0"; 
unsigned char rrh_mac[20] = "0:0:0:0:0:0";
int  rrh_port = 50000; // has to be an option

/* log */
int16_t           glog_level          = LOG_DEBUG;
int16_t           glog_verbosity      = LOG_MED;
int16_t           rrh_log_level       = LOG_INFO;
int16_t           rrh_log_verbosity   = LOG_MED;
int16_t           enb_log_level       = LOG_INFO;
int16_t           enb_log_verbosity   = LOG_MED;
int16_t           ue_log_level        = LOG_INFO;
int16_t           ue_log_verbosity    = LOG_MED;


/* flag definitions */
uint8_t 	eNB_flag=0;
uint8_t 	UE_flag=0;
uint8_t 	EXMIMO_flag=0;
uint8_t 	USRP_flag=0;
uint8_t 	RT_flag=0, NRT_flag=1;
uint8_t 	rrh_exit=0;
uint8_t         loopback_flag=0;
uint8_t         measurements_flag=0;

/* Default operation as RRH:
   - there are neither eNB nor UE modules
   - no RF hardware is specified (NONE_IF)
   - default ethernet interface is local */
uint8_t 	    num_eNB_mod=0;
uint8_t 	    num_UE_mod=0;
char*           if_name="lo";
uint8_t         eth_mode=ETH_UDP_MODE;

rrh_module_t 	        *enb_array;
rrh_module_t            *ue_array;

openair0_vtimestamp 	hw_counter=0;

char   rf_config_file[1024];

static void debug_init(void);
static void get_options(int argc, char *argv[]);
static void print_help(void);

/*!\fn static rrh_module_t new_module(unsigned int id);
* \brief creation of a eNB/UE module
* \param[in] module id
* \return module
* \note
* @ingroup  _oai
*/
static rrh_module_t new_module(unsigned int id);

/*!\fn static int get_address(char* if_name, uint8_t flag);
 * \brief retrieves IP address from the specified network interface
 * \param[in] name of network interface
 * \return 0 
 * \note
 * @ingroup  _oai
 */
static int get_address(char* if_name, uint8_t flag);





int main(int argc, char **argv) {
  
  unsigned int i;
  rf_config_file[0]='\0';
  /* parse input arguments */
  get_options(argc, argv);
  /* initialize logger and signal analyzer */
  debug_init();
  /* */
  set_latency_target();
  /* make a graceful exit when ctrl-c is pressed */
  signal(SIGSEGV, signal_handler);
  signal(SIGINT, signal_handler);
  
  /* create modules based on input arguments */
  if (eNB_flag==1){    
    enb_array=(rrh_module_t*)malloc(num_eNB_mod*sizeof(rrh_module_t));	
    for(i=0;i<num_eNB_mod;i++){  
      enb_array[i]=new_module(i);//enb_array[i]=new_module(i, get_RF_interfaces(&hardware_target));	
      config_BBU_mod(&enb_array[i],RT_flag,NRT_flag);
      LOG_I(RRH,"[eNB %d] module(s) created (out of %u) \n",i,num_eNB_mod);		
    }
  }
  if (UE_flag==1){    
    ue_array=(rrh_module_t*)malloc(num_UE_mod*sizeof(rrh_module_t));	
    for(i=0;i<num_UE_mod;i++){
      ue_array[i]=new_module(i);
      config_UE_mod(&ue_array[i],RT_flag,NRT_flag);			
      LOG_I(RRH,"[UE %d] module(s) created (out of %u)\n",i, num_UE_mod);
    }
  }
 
  printf("TYPE <CTRL-C> TO TERMINATE\n");
  
  while (rrh_exit==0)
    sleep(1);

  return EXIT_SUCCESS;
}


static rrh_module_t new_module (unsigned int id) {

  rrh_module_t 	  	rrh_mod;
  openair0_config_t 	openair0_cfg;

  rrh_mod.id=id;
  rrh_mod.loopback=loopback_flag;
  rrh_mod.measurements=measurements_flag;

  /* each module is associated with an ethernet device */
  rrh_mod.eth_dev.type=NONE_DEV;
  rrh_mod.eth_dev.transp_type=NONE_TP;
  /* ethernet device is functioning within RRH */
  rrh_mod.eth_dev.host_type=RRH_HOST;
  /* */
  rrh_mod.eth_dev.openair0_cfg = (openair0_config_t*)malloc(sizeof(openair0_config_t));
  memset(rrh_mod.eth_dev.openair0_cfg,0,sizeof(openair0_config_t));
  /* get IP and MAC address */
  get_address(if_name,eth_mode);
  
  if(eth_mode==ETH_UDP_MODE) {
    openair0_cfg.my_addr = &rrh_ip[0];
    openair0_cfg.my_port = rrh_port;
    LOG_I(RRH,"UDP mode selected for ethernet.\n");
  } else if (eth_mode==ETH_RAW_MODE) {
    openair0_cfg.my_addr = (char*)&rrh_mac[0];
    openair0_cfg.my_port = rrh_port;
    LOG_I(RRH,"RAW mode selected for ethernet.\n");
  } 

  /* */
  eth_params_t *eth_params = (eth_params_t*)malloc(sizeof(eth_params_t));
  memset(eth_params, 0, sizeof(eth_params_t));
  eth_params->local_if_name     = if_name;
  eth_params->transp_preference = eth_mode;

  /* ethernet device initialization */
  if (openair0_transport_load(&rrh_mod.eth_dev, &openair0_cfg,eth_params)<0) {
    LOG_E(RRH,"Exiting, cannot initialize ethernet interface.\n");
    exit(-1);
  }

  /* allocate space and specify associated RF device */
  openair0_device *oai_dv = (openair0_device *)malloc(sizeof(openair0_device));
  memset(oai_dv,0,sizeof(openair0_device));

  rrh_mod.devs=oai_dv;   
  rrh_mod.devs->type=NONE_DEV;
  rrh_mod.devs->transp_type=NONE_TP;
  rrh_mod.devs->host_type=RRH_HOST; 

  return rrh_mod;
}

static void debug_init(void) {
  
  /* log initialization */
  logInit();
  set_glog(glog_level,  glog_verbosity);
  
  set_comp_log(RRH,     rrh_log_level,   rrh_log_verbosity, 1);
  //set_comp_log(ENB_LOG, enb_log_level,   enb_log_verbosity, 1);
  //set_comp_log(UE_LOG,  ue_log_level,    ue_log_verbosity,  1);
  
  /* vcd initialization */
  if (ouput_vcd) {
    vcd_signal_dumper_init("/tmp/openair_dump_rrh.vcd");
    
  }
}


static void get_options(int argc, char *argv[]) {

  int 	opt;

  while ((opt = getopt(argc, argv, "xvhlte:n:u:g:r:m:i:f:")) != -1) {
    
    switch (opt) {
    case 'n':
      eNB_flag=1;
      num_eNB_mod=atoi(optarg);
      break;
    case 'u':
      UE_flag=1;
      num_UE_mod=atoi(optarg);
      break;
    case 'g':
      glog_level=atoi(optarg);
      break;
    case 'i':  
      if (optarg) {
	if_name=strdup(optarg); 
	printf("RRH interface name is set to %s\n", if_name);	
      }
      break;
    case 'm':  
      eth_mode=atoi(optarg);      
      break;
    case 'r':
      //rrh_log_level=atoi(optarg);
      break;
    case 'e':
      //enb_log_level=atoi(optarg);
      break;
    case 'x':
      rt_period = DEFAULT_PERIOD_NS;
      RT_flag=1;
      NRT_flag=0;
      break;
    case 'v':
      /* extern from vcd */
      ouput_vcd=1;  
      break;
    case 'l':
      /*In loopback mode rrh sends back to bbu what it receives*/
      loopback_flag=1; 
      break;
    case 'f':
      if (optarg){
	if ((strcmp("null", optarg) == 0) || (strcmp("NULL", optarg) == 0)) {
	  printf("no configuration filename is provided\n");
	}
	else if (strlen(optarg)<=1024){
	  // rf_config_file = strdup(optarg);
	  strcpy(rf_config_file,optarg);
	}else {
	  printf("Configuration filename is too long\n");
	  exit(-1);   
	}
      }
      break;
    case 't':
      /* When measurements are enabled statistics related to TX/RX time are printed */
      measurements_flag=1; 
      break;      
    case 'h':
      print_help();
      exit(-1);
    default: /* '?' */
      //fprintf(stderr, "Usage: \n", argv[0]);
      exit(-1);
  }
}

}

static int get_address(char* if_name, uint8_t flag) {
  
  int fd;
  struct ifreq ifr;
    
  fd = socket(AF_INET, SOCK_DGRAM, 0);  
  /* I want to get an IPv4 IP address */
  ifr.ifr_addr.sa_family = AF_INET;  
  /* I want IP address attached to "if_name" */
  strncpy(ifr.ifr_name, if_name, IFNAMSIZ-1);

  if (flag==ETH_UDP_MODE) {
    if ( ioctl(fd, SIOCGIFADDR, &ifr)<0 ) {
      perror("IOCTL:");
      exit(-1);
    } 
    snprintf(&rrh_ip[0],20,"%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    LOG_I(RRH,"%s: IP address: %s\n",if_name,rrh_ip);
  } else if (flag==ETH_RAW_MODE) {
    if ( ioctl(fd, SIOCGIFHWADDR, &ifr)<0 ) {
      perror("IOCTL:");
      exit(-1);
    } 
    ether_ntoa_r ((struct ether_addr *)ifr.ifr_hwaddr.sa_data, (char*)rrh_mac);
    LOG_I(RRH,"%s: MAC address: %s\n",if_name,rrh_mac);    
  }
  
  close(fd);
  return 0;
}


static void print_help(void) {

  puts("Usage: \n");
  puts("     sudo -E chrt 99 ./rrh -n1 -g6 -v -t -i lo -m1");
  puts("Options:\n");
  puts("\t -n create eNB module\n");
  puts("\t -u create UE module\n");
  puts("\t -g define global log level\n");
  puts("\t -i set the RRH interface (default lo)\n");
  puts("\t -m set ethernet mode to be used by RRH, valid options: (1:raw, 0:udp) \n");
  puts("\t -r define rrh log level\n");
  puts("\t -e define eNB log level\n");
  puts("\t -x enable real time bahaviour\n");
  puts("\t -v enable vcd dump\n");
  puts("\t -l enable loopback mode\n");
  puts("\t -t enable measurements\n");
  puts("\t -h display info\n");

}


void *timer_proc(void *arg) {

  timer_t             timerid;    
  struct itimerspec   *timer= (struct itimerspec *)arg ; // the timer data structure
  struct itimerspec   old_value;

  
#ifdef DEADLINE_SCHEDULER
  struct sched_attr attr;
  unsigned int flags = 0;
  
  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;
  
  attr.sched_policy   = SCHED_DEADLINE;
  attr.sched_runtime  =  (0.1  *  100) * 10000; // 
  attr.sched_deadline =  rt_period-30000;//(0.1  *  100) * 10000; 
  attr.sched_period   =  rt_period;//(0.1  *  100) * 10000; // each TX/RX thread has, as a function of RT PERIOD ?? 
  
  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] timer thread: sched_setattr failed\n");
    exit(-1);
  }
#endif  
  
 if (timer_create (CLOCK_REALTIME, NULL, &timerid) == -1) {
    fprintf (stderr, "couldn't create a timer\n");
    perror (NULL);
    exit (EXIT_FAILURE);
  }
  
  signal(SIGALRM, timer_signal_handler);
  LOG_I(RRH,"Timer has started!\n");
  timer_settime (timerid, 0, timer, &old_value);

  while (!rrh_exit) {
    sleep(1);
  }
  
  timer_delete(timerid);
  
  return (0);
}


void timer_signal_handler(int sig) {
  
  if (sig == SIGALRM) {
    pthread_mutex_lock(&timer_mutex);
    hw_counter ++;
    pthread_mutex_unlock(&timer_mutex);
     VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_CNT, hw_counter);//USED ELSEWHERE
  }
}


void signal_handler(int sig) {
  
  void *array[10];
  size_t size;
  
  if (sig==SIGSEGV) {
    // get void*'s for all entries on the stack
    size = backtrace(array, 10);
    
    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, 2);
    exit(-1);
  } else {
    printf("trying to exit gracefully...\n");
    rrh_exit = 1;
  }
}

void exit_fun(const char* s) {
  if (s != NULL) {
    printf("%s %s() Exiting RRH: %s\n",__FILE__, __FUNCTION__, s);
  }
  rrh_exit = 1;
  exit (-1);
}


