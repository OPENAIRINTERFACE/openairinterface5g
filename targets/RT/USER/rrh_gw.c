
/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

   Contact Information
   OpenAirInterface Admin: openair_admin@eurecom.fr
   OpenAirInterface Tech : openair_tech@eurecom.fr
   OpenAirInterface Dev  : openair4g-devel@eurecom.fr

   Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

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
#include "rrh_gw.h" // change to rrh_new.h, put externs in rrh_extern.h
#include "rt_wrapper.h"
#include "rrh_gw_externs.h" // change to rrh_new.h, put externs in rrh_extern.h


#include "log_if.h"
#include "log_extern.h"
#include "vcd_signal_dumper.h"

//#undef LOWLATENCY

/******************************************************************************
 **                               FUNCTION PROTOTYPES                        **
 ******************************************************************************/
static void debug_init(void);
static void get_options(int argc, char *argv[]);
static void print_help(void);
static void get_RFinterfaces(void);
static rrh_module_t new_module(unsigned int id);
int get_ip_address(char* if_name);

char rrh_ip[20] = "192.168.12.242"; // there is code to detect the my ip address
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


/* flags definitions */
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
   - no RF device is asscociated with RRH */
uint8_t 	    num_eNB_mod=0;
uint8_t 	    num_UE_mod=0;
uint8_t 	    num_EXMIMO_mod=0;
uint8_t 	    num_USRP_mod=0;
uint8_t             hardware_target=NONE_IF;

rrh_module_t 	        *enb_array;
rrh_module_t            *ue_array;

pthread_mutex_t         timer_mutex;
openair0_vtimestamp 	hw_counter=0;
unsigned int		rt_period=0;
struct itimerspec       timerspec;

char* if_name="lo";

int main(int argc, char **argv) {

  unsigned int i;
  
  /* parse input arguments */
  get_options(argc, argv);
  /* initialize logger and signal analyzer */
  debug_init();
  /* */
  set_latency_target();
  /*make a graceful exit when ctrl-c is pressed */
  signal(SIGSEGV, signal_handler);
  signal(SIGINT, signal_handler);
  /*probe RF front end devices interfaced to RRH */
  // get_RFinterfaces();
  

#ifdef ETHERNET 
 int 			error_code_timer;
  pthread_t 		main_timer_proc_thread;

  LOG_I(RRH,"Creating timer thread with rt period  %d ns.\n",rt_period);
 
  /* setup the timer to generate an interrupt:
     -for the first time in (sample_per_packet/sample_rate) ns
     -and then every (sample_per_packet/sample_rate) ns */
  timerspec.it_value.tv_sec =     rt_period/1000000000;
  timerspec.it_value.tv_nsec =    rt_period%1000000000;
  timerspec.it_interval.tv_sec =  rt_period/1000000000;
  timerspec.it_interval.tv_nsec = rt_period%1000000000;
  

  //#ifndef LOWLATENCY
  pthread_attr_t 	attr_timer;
  struct sched_param 	sched_param_timer;
  
  pthread_attr_init(&attr_timer);
  sched_param_timer.sched_priority = sched_get_priority_max(SCHED_FIFO);
  pthread_attr_setschedparam(&attr_timer,&sched_param_timer);
  pthread_attr_setschedpolicy(&attr_timer,SCHED_FIFO);
   
  pthread_mutex_init(&timer_mutex,NULL);

  error_code_timer = pthread_create(&main_timer_proc_thread, &attr_timer, timer_proc, (void *)&timerspec);
  LOG_I(RRH,"[SCHED] FIFO scheduling applied to timer thread \n");		
  /*#else 
  error_code_timer = pthread_create(&main_timer_proc_thread, NULL, timer_proc, (void *)&timerspec);
  LOG_I(RRH,"[SCHED] deadline scheduling applied to timer thread \n");		
  #endif*/	
  
  if (error_code_timer) {
    LOG_E(RRH,"Error while creating timer proc thread\n");
    exit(-1);
  }
#endif

  /* create modules based on input arguments */
  if (eNB_flag==1){    
    enb_array=(rrh_module_t*)malloc(num_eNB_mod*sizeof(rrh_module_t));	
    for(i=0;i<num_eNB_mod;i++){  
      enb_array[i]=new_module(i);//enb_array[i]=new_module(i, get_RF_interfaces(&hardware_target));	
      create_eNB_trx_threads(&enb_array[i],RT_flag,NRT_flag);
      LOG_I(RRH,"[eNB %d] module(s) created (out of %u) \n",i,num_eNB_mod);		
    }
  }
  if (UE_flag==1){    
    ue_array=(rrh_module_t*)malloc(num_UE_mod*sizeof(rrh_module_t));	
    for(i=0;i<num_UE_mod;i++){
      ue_array[i]=new_module(i);
      create_UE_trx_threads(&ue_array[i],RT_flag,NRT_flag);			
      LOG_I(RRH,"[UE %d] module(s) created (out of %u)\n",i, num_UE_mod);
    }
  }
 
  printf("TYPE <CTRL-C> TO TERMINATE\n");
  
  while (rrh_exit==0)
    sleep(1);

  //close sockets 
  
  return EXIT_SUCCESS;
}




/*!\fn openair0_device new_module (unsigned int id, dev_type_t type)
* \brief this function
* \param[in]
* \param[out]
* \return
* \note
* @ingroup  _oai
*/
static rrh_module_t new_module (unsigned int id) {

  rrh_module_t 	  	rrh_mod;
  openair0_config_t 	openair0_cfg;

  rrh_mod.id=id;
  rrh_mod.loopback=loopback_flag;
  rrh_mod.measurements=measurements_flag;

  /* each module is associated with an ethernet device */
  rrh_mod.eth_dev.type=ETH_IF;
  rrh_mod.eth_dev.func_type=RRH_FUNC; 
  get_ip_address(if_name);
  openair0_cfg.my_ip=&rrh_ip[0];
  openair0_cfg.my_port=rrh_port;

  /* ethernet device initialization */
  if (openair0_dev_init_eth(&rrh_mod.eth_dev, &openair0_cfg)<0){
    LOG_E(RRH,"Exiting, cannot initialize ethernet interface.\n");
    exit(-1);
  }
 
  /* specify associated RF device */
  openair0_device *oai_dv = (openair0_device *)malloc(sizeof(openair0_device));
  memset(oai_dv,0, sizeof(openair0_device));

#ifdef EXMIMO
  rrh_mod.devs=oai_dv;   
  rrh_mod.devs->type=EXMIMO_IF;
  LOG_I(RRH,"Setting RF device to EXMIMO\n");	   
#elif OAI_USRP
  rrh_mod.devs=oai_dv;	
  rrh_mod.devs->type=USRP_IF;
  LOG_I(RRH,"Setting RF device to USRP\n");    	 
#elif OAI_BLADERF
  rrh_mod.devs=oai_dv;	
  rrh_mod.devs->type=BLADERF_IF;
  LOG_I(RRH,"Setting RF device to BLADERF\n");
#else
  rrh_mod.devs=oai_dv;
  rrh_mod.devs->type=NONE_IF;
  LOG_I(RRH,"Setting RF interface to NONE_IF... \n");
#endif  
  
  return rrh_mod;
}




/*! \fn void *timer_proc(void *arg)
* \brief this function
* \param[in]
* \param[out]
* \return
* \note
* @ingroup  _oai
*/
void *timer_proc(void *arg) {

  timer_t             timerid;    
  struct itimerspec   *timer= (struct itimerspec *)arg ; // the timer data structure
  struct itimerspec   *old_value;

  /* 
#ifdef LOWLATENCY
  struct sched_attr attr;
  unsigned int flags = 0;
  
  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;
  
  attr.sched_policy   = SCHED_DEADLINE;
  attr.sched_runtime  =  (0.1  *  100) * 10000; // 
  attr.sched_deadline =  rt_period-30000;//(0.1  *  100) * 10000; // 
  attr.sched_period   =  rt_period;//(0.1  *  100) * 10000; // each TX/RX thread has, as a function of RT PERIOD ?? 
  
  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] timer thread: sched_setattr failed\n");
    exit(-1);
  }
#endif  
  */
 if (timer_create (CLOCK_REALTIME, NULL, &timerid) == -1) {
    fprintf (stderr, "couldn't create a timer\n");
    perror (NULL);
    exit (EXIT_FAILURE);
  }
  
  signal(SIGALRM, timer_signal_handler);
  LOG_I(RRH,"Timer has started!\n");
  timer_settime (timerid, 0, timer, old_value);

  while (!rrh_exit) {
    sleep(1);
  }
  
  timer_delete(timerid);
  
  return (0);
}


/*! \fn void timer_signal_handler(int sig)
* \brief this function
* \param[in]
* \param[out]
* \return
* \note
* @ingroup  _oai
*/
void timer_signal_handler(int sig) {
  
  if (sig == SIGALRM) {
    pthread_mutex_lock(&timer_mutex);
    hw_counter ++;
    pthread_mutex_unlock(&timer_mutex);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_HW_FRAME, hw_counter);
  }
}


/*! \fn void signal_handler(int sig)
* \brief this function
* \param[in]
* \param[out]
* \return
* \note
* @ingroup  _oai
*/
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


/*! \fn void debug_init(void)
 * \brief this function
 * \param[in]
 * \param[out]
 * \return
 * \note
 * @ingroup  _oai
 */
static void debug_init(void) {
	
  // log initialization
  logInit();
  set_glog(glog_level,  glog_verbosity);
  
  set_comp_log(RRH,     rrh_log_level,   rrh_log_verbosity, 1);
  //set_comp_log(ENB_LOG, enb_log_level,   enb_log_verbosity, 1);
  //set_comp_log(UE_LOG,  ue_log_level,    ue_log_verbosity,  1);
  
  // vcd initialization
  if (ouput_vcd) {
    vcd_signal_dumper_init("/tmp/openair_dump_rrh.vcd");
    
}
}

/*!\fn void get_options(int argc, char *argv[])
* \brief this function
* \param[in]
* \param[out]
* \return
* \note
* @ingroup  _oai
*/
static void get_options(int argc, char *argv[]) {

  int 	opt;

  while ((opt = getopt(argc, argv, "xvhlte:n:u:g:r:w:i:")) != -1) {
    
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
    case 't':
      /*When measurements are enabled statistics related to TX/RX time are printed*/
      measurements_flag=1; 
      break;
    case 'w':
      /* force to use this target*/    
      hardware_target=1;
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

/*!\fn void print_help(void)
* \brief this function
* \param[in]
* \param[out]
* \return
* \note
* @ingroup  _oai
*/
int get_ip_address(char* if_name) {
  
  int fd;
  struct ifreq ifr;
  
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  
  /* I want to get an IPv4 IP address */
  ifr.ifr_addr.sa_family = AF_INET;

  /* I want IP address attached to "eth0" */
  strncpy(ifr.ifr_name, if_name, IFNAMSIZ-1);

 ioctl(fd, SIOCGIFADDR, &ifr);

 close(fd);

 /* display result */
 snprintf(&rrh_ip[0],20,"%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
 printf("Got IP address %s from interface %s\n", rrh_ip,if_name);
 return 0;
}

/*!\fn void print_help(void)
* \brief this function
* \param[in]
* \param[out]
* \return
* \note
* @ingroup  _oai
*/
static void print_help(void) {

  puts("Usage: \n");
  puts("     sudo -E chrt 99 ./rrh -n1 -g6 -v -t");
  puts("Options:\n");
  puts("\t -n create eNB module\n");
  puts("\t -u create UE module\n");
  puts("\t -g define global log level\n");
  puts("\t -i set the RRH interface (default eth0)\n");
  puts("\t -r define rrh log level\n");
  puts("\t -e define eNB log level\n");
  puts("\t -x enable real time bahaviour\n");
  puts("\t -v enable vcd dump\n");
  puts("\t -l enable loopback mode\n");
  puts("\t -t enable measurements\n");
  puts("\t -w force to use specified HW\n");
  puts("\t -h display info\n");

}

/*! \fn void exit_fun(const char* s)
* \brief this function
* \param[in]
* \param[out]
* \return
* \note
* @ingroup  _oai
*/
void exit_fun(const char* s)
{
  if (s != NULL) {
    printf("%s %s() Exiting RRH: %s\n",__FILE__, __FUNCTION__, s);
  }
  rrh_exit = 1;
  exit (-1);
}


/*! \fn static void get_RFinterfaces(void)
* \brief this function
* \param[in]
* \param[out]
* \return
* \note
* @ingroup  _oai
*/
static void get_RFinterfaces(void) {

  EXMIMO_flag=1;
  USRP_flag=1;
  num_EXMIMO_mod=1;
  num_USRP_mod=1;

}


/*!\fn void create_timer_thread(void)
* \brief this function
* \param[in]
* \param[out]
* \return
* \note
* @ingroup  _oai
*/
void create_timer_thread() {
  
  int 			error_code_timer;
  pthread_t 		main_timer_proc_thread;

  LOG_I(RRH,"Creating timer thread with rt period  %d ns.\n",rt_period);
 
  /* setup the timer to generate an interrupt:
     -for the first time in (sample_per_packet/sample_rate) ns
     -and then every (sample_per_packet/sample_rate) ns */
  timerspec.it_value.tv_sec =     rt_period/1000000000;
  timerspec.it_value.tv_nsec =    rt_period%1000000000;
  timerspec.it_interval.tv_sec =  rt_period/1000000000;
  timerspec.it_interval.tv_nsec = rt_period%1000000000;
  
  pthread_mutex_init(&timer_mutex,NULL);
  
#ifndef LOWLATENCY
  pthread_attr_t 	attr_timer;
  struct sched_param 	sched_param_timer;
  
  pthread_attr_init(&attr_timer);
  sched_param_timer.sched_priority = sched_get_priority_max(SCHED_FIFO-1);
  pthread_attr_setschedparam(&attr_timer,&sched_param_timer);
  pthread_attr_setschedpolicy(&attr_timer,SCHED_FIFO-1);
  error_code_timer = pthread_create(&main_timer_proc_thread, &attr_timer, timer_proc, (void *)&timerspec);
  LOG_I(RRH,"[SCHED] FIFO scheduling applied to timer thread \n");		
#else 
  error_code_timer = pthread_create(&main_timer_proc_thread, NULL, timer_proc, (void *)&timerspec);
  LOG_I(RRH,"[SCHED] deadline scheduling applied to timer thread \n");		
#endif	
  
  if (error_code_timer) {
    LOG_E(RRH,"Error while creating timer proc thread\n");
    exit(-1);
  }
}
