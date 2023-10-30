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

/*! \file otg_defs.h main used structures
 * \brief otg structure
 * \author N. Nikaein and A. Hafsaoui
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr
 * \note
 * \warning
 */

#ifndef __OTG_DEFS_H__
# define __OTG_DEFS_H__

/* \brief To define the NUMBER_OF_eNB_MAX and MAX_MOBILES_PER_ENB */
#include "common/openairinterface5g_limits.h"
#include <common/platform_constants.h>
#include "otg_config.h"

/**
 * \enum Application
 * \brief OTG applications type
 *
 *
 * Application describes the class of traffic supported and can be pre-configured bu otg_tx
 */
typedef enum {
  NO_PREDEFINED_MULTICAST_TRAFFIC =0,
  MSCBR,
  MMCBR,
  MBCBR,
  MSVBR,
  MMVBR,
  MBVBR,
  MVIDEO_VBR_4MBPS
} Multicast_Application;

/**
 * \enum Application
 * \brief OTG applications type
 *
 *
 * Application describes the class of traffic supported and can be pre-configured bu otg_tx
 */
typedef enum {
  NO_PREDEFINED_TRAFFIC =0,
  M2M, // 1
  SCBR, //2
  MCBR, //3
  BCBR, //4
  AUTO_PILOT, //5
  BICYCLE_RACE, // 6
  OPENARENA, // 7
  TEAM_FORTRESS, // 8
  FULL_BUFFER, // 9
  M2M_TRAFFIC, // 10
  AUTO_PILOT_L,  //11     /*AUTO PILOT LOW SPEEDS*/
  AUTO_PILOT_M, // 12   /*AUTO PILOT MEDIEUM SPEEDS*/
  AUTO_PILOT_H, // 13     /*AUTO PILOT HIGH SPEEDS*/
  AUTO_PILOT_E, // 14       /*AUTO PILOT EMERGENCY*/
  VIRTUAL_GAME_L,// 15    /*VIRTUAL GAME LOW SPEEDS*/
  VIRTUAL_GAME_M, // 16     /*VIRTUAL GAME MEDIEUM SPEEDS*/
  VIRTUAL_GAME_H, // 17   /*VIRTUAL GAME HIGH SPEEDS*/
  VIRTUAL_GAME_F, // 18   /*VIRTUAL GAME FINISH*/
  ALARM_HUMIDITY, // 19   /* SENSOR BASED ALARM : HUMIDITY */
  ALARM_SMOKE,   // 20    /* SENSOR BASED ALARM : SMOKE */
  ALARM_TEMPERATURE, // 21 /* SENSOR BASED ALARM : TEMPERATURE */
  OPENARENA_DL, // 22
  OPENARENA_UL, // 23
  VOIP_G711, // 24
  VOIP_G729, // 25
  IQSIM_MANGO, // 26
  IQSIM_NEWSTEO, // 27
  OPENARENA_DL_TARMA, // 28
  VIDEO_VBR_10MBPS, // 29
  VIDEO_VBR_4MBPS, // 30
  VIDEO_VBR_2MBPS, // 31
  VIDEO_VBR_768KBPS, // 32
  VIDEO_VBR_384KBPS, // 33
  VIDEO_VBR_192KBPS, // 34
  BACKGROUND_USERS, // 35
  DUMMY
} Application;


/**
 * \enum dist_type
 * \brief OTG implemented distribution for packet and inter departure time computation
 *
 *
 * dist_type presents the used distribition to generate inter departure time and packet size
 */

typedef enum {
  NO_TRAFFIC=0,
  UNIFORM, //1
  GAUSSIAN, //2
  EXPONENTIAL, //3
  POISSON, // 4
  FIXED, // 5
  WEIBULL, // 6
  PARETO, // 7
  GAMMA, // 8
  CAUCHY, // 9
  LOG_NORMAL, // 10
  TARMA,
  VIDEO,
  BACKGROUND_DIST,
} dist_type;


/**
 * \enum trans_proto
 *
 * \brief trans_proto enmerates used transport protocol
 *
 *
 */

typedef enum {
  NO_PROTO=0,
  UDP=1,
  TCP,
} trans_proto;


/**
 * \enum ip_v
 *
 *\brief ip_v presents the used IP version to generate the packet
 *
 */
typedef enum {
  NO_IP=0,
  IPV4=1,
  IPV6,
} ip_v;

/**
 * \enum ip_v
 *
 *\brief ip_v presents the used IP version to generate the packet
 *
 */
/*
typedef enum {
  OFF_STATE=0,
  ON_STATE,
  ACTIVE_STATE,
  MAX_NUM_STATE,
}TRAFFIC_STATE;
 */

typedef enum {
  OFF_STATE=0,
  PU_STATE,
  ED_STATE,
  PE_STATE,
} TRAFFIC_STATE;

typedef enum {
  SILENCE=0,
  SIMPLE_TALK=3,
} VOIP_STATE;

/**
 * \enum ALPHABET_GEN
 *
 *\brief ALPHABET_GEN Alphabet type to generate random string
 *
 */
typedef enum {
  REPEAT_STRING=0, // repeat the same set of characters
  SUBSTRACT_STRING, // substract the string from the know character string
  RANDOM_POSITION, // take the char from the known character string randomly
  RANDOM_STRING, // rake the charcters randomly
} ALPHABET_GEN;
typedef enum {
  HEADER_ALPHABET=0,
  PAYLOAD_ALPHABET,
} ALPHABET_TYPE;



/**
 * \enum HEADER_TYPE
 *
 * \brief HEADER_TYPE alows to identify the transport protocol and IP version
 *
 */

typedef enum {
  NO_HEADER=0,
  UDP_IPV4,
  TCP_IPV4,
  UDP_IPV6,
  TCP_IPV6,
} HEADER_TYPE;

/*
 otg_models: definitions and structures,
 see otg_models.c for a detailed description
 */

/*tarma*/
#define TARMA_NUM_MA_MAX    5  /* this value defines the maximum MA (FIR) filter length - min: 3*/
#define TARMA_NUM_AR_MAX    5  /* this value defines the maximum AR (IIR) filter length - min: 3*/
#define TARMA_NUM_POLY_MAX  10  /* this value defines the maximum order of the polynomial transformation - min: 13*/
#define TARMA_NUM_PROCESSES  2  /* this value defines the number of different gaussian iid input processes: 1*/
#define TARMA_NUM_GOP_MAX  32  /* this value defines the maximum length of a group of pictures (GOP)*/
#define TARMA_NUM_FRAME_TYPES 3  /*this number defines the maximum amount of different frame types (e.g., I,P,B)*/

/*background*/
#define BACKGROUND_NUM_ACTIVE_MAX 10 /*maximum number of simultaneously active users*/
#define BACKGROUND_RATE_MAX 1e6/8      /*maximum mean rate for bg-traffic (B/s))*/

/*tarma*/
typedef struct {
  /* this struct inherits all the necessary information for cunstructing
          the next random sample of one random process -
          - the xml parser must instanciate such a struct for each <Process> environment
          within the <RandomProcesses> environment*/
  double inputWeight [TARMA_NUM_PROCESSES];  /* weights for summation of different gaussian input processes
          - default: 0 */
  double maWeight [TARMA_NUM_MA_MAX];  /* weigths of the MA (FIR) filter -
            -  <MaCoeff><Lag>0</Lag><Weight>  0.3</Weight></MaCoeff> goes here -
            - the xml parser must place the value of <Weight> at the index of <Lag> -
            - default: 0*/
  double maHist [TARMA_NUM_MA_MAX];  /* the MA history, updated by the CalculateSample() function,
            - default: 0 */
  double arWeight [TARMA_NUM_AR_MAX];  /* weigths of the AR (IIR) filter -
            -  <ArCoeff><Lag>0</Lag><Weight>  0.3</Weight></ArCoeff> goes here -
            - the xml parser must place the value of <Weight> at the index of <Lag> -
            - default: 0*/
  double arHist [TARMA_NUM_AR_MAX];  /* the AR history, updated by the CalculateSample() function,
            - default: 0 */
  double polyWeight [TARMA_NUM_POLY_MAX];  /* weights of the polynomial transformation -
          - <PolyCoeff><Power>0</Power><Weight>   123</Weight></PolyCoeff> goes here -
          - the xml parser must place the value of <Weigth> at the index of <Power> -
          - default: 0*/
} tarmaProcess_t;

typedef struct { /*this struct inherits processes and input samples for one flow*/
  // processes for packet size and packet inter arrival time
  tarmaProcess_t tarma_idt;
  tarmaProcess_t tarma_size;
  // input samples used for both, packet size and packet inter-arrival time
  double tarma_input_samples [TARMA_NUM_PROCESSES];
} tarmaStream_t;

typedef struct { /*this struct inherits processes and input samples for a video sequence*/
  tarmaProcess_t tarma_size;
  int tarmaVideoFrameNumber;
  int tarmaVideoGopStructure[TARMA_NUM_GOP_MAX];
  double polyWeightFrame [TARMA_NUM_FRAME_TYPES][TARMA_NUM_POLY_MAX];
} tarmaVideo_t;

/*background*/
typedef struct { /*this struct constitutes a single bg-session*/
  double meanSessionRate;
  int endTime;
} backgroundSession_t;

typedef struct { /*this structure constitutes a whole bg-stream with multiple bg-sessions*/
  double meanNumSessions; /*equivalent to lambda_n*/
  int lastUpdateTime;
  backgroundSession_t activeSessions[BACKGROUND_NUM_ACTIVE_MAX];
} backgroundStream_t;



typedef struct {

  int application_type[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];  /*!\brief It identify the application of the simulated traffic, could be cbr, m2m, gaming,etc*/
  int trans_proto[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  int ip_v[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];

  int flow_start[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION]; /*!\brief Duration of traffic generation or use the emuulation time instead */
  int flow_duration[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION]; /*!\brief Duration of traffic generation or use the emuulation time instead */

  int idt_dist[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION]; /*!\brief Inter Departure Time distribution */
  int idt_min[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];   /*!\brief Min Inter Departure Time, for uniform distrib  */
  int idt_max[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];   /*!\brief idt, Max Inter Departure Time, for uniform distrib  */

  int size_dist[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION]; /*!\brief Paylolad size distribution */
  int size_min[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];  /*!\brief Min Payload size, for uniform distrib  */
  int size_max[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];  /*!\brief payload, Max Inter Departure Time, for uniform distrib  */

} otg_multicast_t;


/**
 * \struct otg_t
 *
 *\brief otg_t  define the traffic generator global parameters, it include a matrix of nodes (source, destination and state) parameters
 *
 */
typedef struct {
  int max_nb_frames; /*!< \brief  Max Number of frames*/
  int application_type[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB]
                      [MAX_NUM_APPLICATION]; /*!\brief It identify the application of the simulated traffic, could be cbr, m2m,
                                                gaming,etc*/
  int trans_proto[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];   /*!\brief Transport Protocol*/
  int ip_v[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];  /*!\brief Ip version */
  //int header_compression;         /*!\brief Specify if header compression is used or not */
  int num_nodes;            /*!\brief Number of used nodes in the simulation */
  int packet_gen_type;             /*!\brief define how the payload is generated: fixed, predefined, random position, random see  ALPHABET_GEN */
  unsigned int background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION]; /*!\brief enable or disable background traffic  */
  unsigned int aggregation_level[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION]; /* define packet aggregation level for the case of gateway*/
  // src id , dst id, and state             // think to the case of several streams per node !!!!!
  int idt_dist[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION][MAX_NUM_TRAFFIC_STATE]; /*!\brief Inter Departure Time distribution */
  int idt_min[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION][MAX_NUM_TRAFFIC_STATE];  /*!\brief Min Inter Departure Time, for uniform distrib  */
  int idt_max[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION][MAX_NUM_TRAFFIC_STATE];  /*!\brief idt, Max Inter Departure Time, for uniform distrib  */

  double idt_std_dev[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION][MAX_NUM_TRAFFIC_STATE];   /*!\brief idt, Standard Deviation, for guassian distrib */
  double idt_lambda[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION][MAX_NUM_TRAFFIC_STATE];     /*!\brief idt, lambda, for exponential/poisson  distrib */
  double idt_scale[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION][MAX_NUM_TRAFFIC_STATE];   /*!\brief scale :parameter for Pareto, Gamma, Weibull and Cauchy distribution*/
  double idt_shape[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION][MAX_NUM_TRAFFIC_STATE];   /*!\brief shape :parameter for Pareto, Gamma, Weibull and Cauchy distribution*/

  int size_dist[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION][MAX_NUM_TRAFFIC_STATE];  /*!\brief Paylolad size distribution */
  int size_min[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION][MAX_NUM_TRAFFIC_STATE]; /*!\brief Min Payload size, for uniform distrib  */
  int size_max[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION][MAX_NUM_TRAFFIC_STATE];   /*!\brief payload, Max Inter Departure Time, for uniform distrib  */
  double size_std_dev[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION][MAX_NUM_TRAFFIC_STATE];  /*!\brief payload, Standard Deviation, for guassian distrib */
  double size_lambda[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION][MAX_NUM_TRAFFIC_STATE];     /*!\brief payload, lambda, for exponential/poisson  distrib */

  double size_scale[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION][MAX_NUM_TRAFFIC_STATE];    /*!\brief scale :parameter for Pareto, Gamma, Weibull and Cauchy distribution */
  double size_shape[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION][MAX_NUM_TRAFFIC_STATE];  /*!\brief shape :parameter for Pareto, Gamma, Weibull and Cauchy distribution*/

  // info for state-based traffic gen
  int num_state [NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB]; /*!\brief Number of states for source node*/
  //  int state_dist[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_TRAFFIC_STATE]; /*!\brief States distribution */
  double state_prob[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_TRAFFIC_STATE]; /*!\brief State probablity: prob to move from one state to the other one */

  // num stream for each src
  // int stream [NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB]; // this requires multi thread for parallel stream for a givcen src
  // emu info
  int flow_start_flag[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION]; /*!\brief flow start time flag for traffic generation or use the emuulation time instead */
  int flow_start[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION]; /*!\brief flow start time of traffic generation or use the emuulation time instead */
  int flow_duration[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION]; /*!\brief flow duration of traffic generation or use the emuulation time instead */
  int seed; /*!\brief The seed used to generate the random positions*/


  int  dst_port[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB]; /*!\brief Destination port number, for the socket mode*/
  char *dst_ip[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB]; /*!\brief Destination IP address, for the socket mode*/

  int trans_proto_background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB]; /*!\brief define the transport protocol and IP version for background traffic*/



  double prob_off_pu[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  double prob_off_ed[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  double prob_off_pe[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  double prob_pu_ed[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  double prob_pu_pe[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  double prob_ed_pe[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  double prob_ed_pu[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int holding_time_off_ed[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int holding_time_off_pu[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int holding_time_off_pe[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int holding_time_pe_off[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int pu_size_pkts[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int ed_size_pkts[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int m2m[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];


  unsigned int throughput_metric;
  unsigned int latency_metric;
  unsigned int loss_metric;
  unsigned int curve;
  unsigned int owd_radio_access;
  unsigned int background_stats;
  unsigned int application_idx[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];


} otg_t;


typedef struct {

  unsigned char flow_id;      /*!< \brief It identify the flow ID (we can have source and destination with several flows)  */
  unsigned int time;          /*!< \brief simulation time at the tx, this is ctime */
  unsigned int seq_num;       /*!< \brief Sequence Number, counter of data packets between tx and rx */
  unsigned char state;        /*!< \brief state of node : on, off, or active */
  unsigned char hdr_type;     /*!< \brief Header type: tcp/udp vs ipv4/ipv6 */
  unsigned char src_instance; /*!< \brief traffic source instance */
  unsigned char dst_instance; /*!< \brief traffic destination instance */
  unsigned short pkts_size;   /*!< \brief the size of payload + header */
  unsigned short aggregation_level;/*!< \brief packet aggregation level */
  unsigned int traffic_type;       /*!< \brief to specify if it corresponds to an m2m traffic */
  //int payload_size; /*!< \brief the size of the payload to transmit */
  //int header_size;  /*!< \brief Header type: tcp/udp vs ipv4/ipv6 */
} __attribute__((__packed__)) otg_hdr_t;

typedef struct {
  unsigned int flag;
  unsigned int size;
} __attribute__((__packed__)) otg_hdr_info_t;


/**
 * \struct packet_t
 *
 * \brief packet_t corresponds to the global structure of the generated packet
 *
 *
 */


typedef struct {
  char* header ;    /*!< \brief  Header */
  char* payload;    /*!< \brief  Payload*/
} __attribute__((__packed__)) packet_t;

typedef struct {
  int ctime;
  int ptime[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];

  unsigned int header_size_otg[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];
  unsigned int header_size_app[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];

  int idt[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];


  int tx_sn[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];
  int rx_sn[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];

  int tx_num_pkt[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];
  int tx_num_bytes[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];
  int rx_num_pkt[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];
  int rx_num_bytes[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];

  float rx_owd_min[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_EMU_TRAFFIC];      /*!< \brief  One way delay min*/
  float rx_owd_max[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_EMU_TRAFFIC];    /*!< \brief  One way delay max*/

  int rx_pkt_owd[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX];
  int loss_pkts_dl[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];
  int loss_rate[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][MAX_NUM_APPLICATION];

  float radio_access_delay[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX];

  double tx_throughput[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX];
  double rx_goodput[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX];  /*!< \brief  Rx goodput: (size of received data)/ctime*/
  float rx_loss_rate[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX];   /*!< \brief  Rx Loss Rate: ratio, unit: bytes*/

  unsigned int rx_total_bytes_dl;

  /*TARMA parameteres*/
  tarmaStream_t *mtarma_stream[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  tarmaVideo_t *mtarma_video[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];

} otg_multicast_info_t;




/**
 * \struct packet_t
 *
 * \brief The OTG information and KPI of the simulation structure
 *
 */

typedef struct {
  /*!< \brief  info part: */
  int ctime;                        /*!< \brief Simulation time in ms*/
  int ptime_background_ul;                    /*!< \brief time of last sent background UL data (time in ms)*/
  int ptime_background_dl;                    /*!< \brief time of last sent background DL data (time in ms)*/
  int ptime[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];       /*!< \brief time of last sent data (time in ms)*/
  int seq_num[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];   /*!< \brief the sequence number of the sender  */
  int seq_num_background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];     /*!< \brief the sequence number for background traffic of the sender  */
  int seq_num_rx[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];    /*!< \brief the sequence number of the receiver */
  int seq_num_rx_background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];  /*!< \brief the sequence number for background traffic of the receiver */


  int idt_background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];         /*!< \brief  Inter Departure Time for background traffic in ms*/
  int size_background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];        /*!< \brief  payload size for background traffic*/
  int idt[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];     /*!< \brief  Inter Departure Time in ms*/
  int header_type[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];    /*!< \brief  Define the type of header: Transport layer + IP version*/

  /*!< \brief Statics part: vars updated at each iteration of otg_tx */
  int tx_num_pkt[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];    /*!< \brief  Number of data packet in the tx*/
  int tx_num_bytes[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];    /*!< \brief  Number of bytes in the tx*/ // get the size and calculate the avg throughput
  // vars updated at each iteration of otg_rx
  int rx_num_pkt[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];    /*!< \brief  Number of data packet in the rx */
  int rx_num_bytes[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];    /*!< \brief  Number of bytes in the rx */
  float rx_pkt_owd[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];     /*!< \brief  One way delay: rx_ctime - tx_ctime, */
  float rx_owd_min[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];      /*!< \brief  One way delay min*/
  float rx_owd_max[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];    /*!< \brief  One way delay max*/
  float rx_pkt_owd_e2e[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];     /*!< \brief  One way delay: rx_ctime - tx_ctime, */
  float rx_owd_min_e2e[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];      /*!< \brief  One way delay min*/
  float rx_owd_max_e2e[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];    /*!< \brief  One way delay max*/
  float rx_pkt_owd_history[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][2];    /*!< \brief  One way delay histoy used for jitter calculation: rx_ctime - tx_ctime, [2] to keep the owd for the current and previous pkt */
  float rx_pkt_owd_history_e2e[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][2];    /*!< \brief  One way delay histoy used for jitter calculation: rx_ctime - tx_ctime, [2] to keep the owd for the current and previous pkt */
  float rx_pkt_jitter[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];    /*!< \brief  One way delay: rx_ctime - tx_ctime */
  float rx_jitter_min[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];     /*!< \brief  One way delay min*/
  float rx_jitter_max[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];     /*!< \brief  One way delay max*/
  float rx_jitter_avg[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];     /*!< \brief  One way delay max*/
  float rx_pkt_jitter_e2e[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];    /*!< \brief  One way delay: rx_ctime - tx_ctime */
  float rx_jitter_min_e2e[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];     /*!< \brief  One way delay min*/
  float rx_jitter_max_e2e[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];     /*!< \brief  One way delay max*/
  float rx_jitter_avg_e2e[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];     /*!< \brief  One way delay max*/
  int rx_jitter_sample[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];    /*!< \brief  One way delay max*/
  int nb_loss_pkts_ul[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];   /*!< \brief  Number of data packets losses for UL*/
  int nb_loss_pkts_dl[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];   /*!< \brief  Number of data packets losses for DL*/
  float owd_const[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];   /*!< \brief  One way delay constant*/
  /*!< \brief KPI part: calculate the KPIs, total */
  double tx_throughput[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];  /*!< \brief  Tx throughput: (size of transmitted data)/ctime*/
  double rx_goodput[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];     /*!< \brief  Rx goodput: (size of received data)/ctime*/
  float rx_loss_rate[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_EMU_TRAFFIC];  /*!< \brief  Rx Loss Rate: ratio, unit: bytes*/
  //int rx_latency[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];     /*!< \brief  Rx Latency */

  /*!< \brief Background traffic part: SATS + KPIs */
  int tx_num_pkt_background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];  /*!< \brief  Number of background data packet in the rx */
  int tx_num_bytes_background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];  /*!< \brief  Number of background bytes in the rx */
  int rx_num_pkt_background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];  /*!< \brief  Number of background data packet in the tx */
  int rx_num_bytes_background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];  /*!< \brief  Number of background bytes in the tx */
  int nb_loss_pkts_background_ul[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB]; /*!< \brief  Number of background packets losses for UL*/
  int nb_loss_pkts_background_dl[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB]; /*!< \brief  Number of background packets losses for DL*/
  double tx_throughput_background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];  /*!< \brief  Tx throughput: (size of transmitted data)/ctime*/
  double rx_goodput_background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];   /*!< \brief  Rx goodput: (size of received data)/ctime*/
  float rx_loss_rate_background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];  /*!< \brief  Rx Loss Rate: ratio, unit: bytes*/

  float radio_access_delay[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];
  int nb_loss_pkts_otg[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];
  unsigned int aggregation_level[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];

  unsigned int state[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];                      /*!< \brief  current state of src node */
  float state_transition_prob[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int start_holding_time_off[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int c_holding_time_off[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int c_holding_time_pe_off[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int start_holding_time_pe_off[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int tx_total_bytes_dl;
  unsigned int tx_total_bytes_ul;
  unsigned int rx_total_bytes_dl;
  unsigned int rx_total_bytes_ul;
  unsigned int total_loss_dl;
  unsigned int total_loss_ul;
  float average_jitter_dl;
  float average_jitter_ul;
  float average_jitter_dl_e2e;
  float average_jitter_ul_e2e;

  /* VOIP tarffic parameters*/
  float voip_transition_prob[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int voip_state[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int start_voip_silence[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int c_holding_time_silence[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int start_voip_talk[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int c_holding_time_talk[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int silence_time[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int simple_talk_time[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];

  /*TARMA parameteres*/
  tarmaStream_t *tarma_stream[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  tarmaVideo_t *tarma_video[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  /*BACKGROUND_USERS parameters*/
  backgroundStream_t *background_stream[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];

  unsigned int header_size_app[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION];
  unsigned int header_size[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];
  unsigned int m2m_aggregation[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];
  unsigned int flow_id[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];
  unsigned int traffic_type[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];
  unsigned int traffic_type_background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];
  //  unsigned int traffic_type_multicast[NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX_MAX][NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX];
  unsigned int hdr_size[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];
  unsigned int header_type_app[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_NUM_APPLICATION]; ;
  unsigned int gen_pkts;
  unsigned int header_size_background[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];
} otg_info_t;


//char buffer_tx[MAX_BUFF_TX]; /*!< \brief define the buffer for the data to transmit */


typedef struct {
  //  char *src_ip;     /*!< \brief   */
  int src:16;     /*!< \brief  */
  int dst:16;     /*!< \brief  */
  int  trans_proto:16;  /*!< \brief  */
  int  ip_v:16;     /*!< \brief  */
  //  int payload_size:16;
} __attribute__((__packed__)) control_hdr_t;




typedef struct {
  control_hdr_t* control_hdr;   /*!< \brief   */
  char* payload_rest;       /*!< \brief  */
} __attribute__((__packed__)) payload_t;




#define MAX_SAMPLES 30

typedef struct {
  int init_forms;
  int is_data_plot_ul;
  int is_data_plot_dl;
  float data_owd_ul[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_SAMPLES];
  float data_throughput_ul[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_SAMPLES];
  float data_ctime_ul[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_SAMPLES];
  int idx_ul[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];
  float data_owd_dl[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_SAMPLES];
  float data_throughput_dl[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_SAMPLES];
  float data_ctime_dl[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][MAX_SAMPLES];
  int idx_dl[NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB][NUMBER_OF_eNB_MAX + MAX_MOBILES_PER_ENB];
} otg_forms_info_t;


#endif
