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

/*! \file oaisim_config.c
* \brief Configuration of oaisim
* \author Navid Nikaein 
* \date 2013-2015
* \version 1.0
* \company Eurecom
* \email: openair_tech@eurecom.fr
* \note
* \warning
*/

#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "oaisim_config.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "UTIL/OMG/omg.h"
#include "UTIL/OMG/grid.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg.h"
#include "UTIL/OTG/otg_externs.h"
#include "oml.h"
#include "msc.h"
#if ENABLE_RAL
#include "lteRALenb.h"
#include "lteRALue.h"
#endif

#if defined(ENABLE_ITTI)
# include "intertask_interface_init.h"
#endif


mapping omg_model_names[] = {
  {"STATIC", STATIC},
  {"RWP", RWP},
  {"RWALK", RWALK},
  {"TRACE", TRACE},
  {"SUMO", SUMO},
  {"STEADY_RWP", STEADY_RWP},
  {"MAX_NUM_MOB_TYPES", MAX_NUM_MOB_TYPES},
  {NULL, -1}
};
// subtypes for RWP in grid map
mapping omg_rwp_names[] = {
  {"MIN_RWP_TYPES", MIN_RWP_TYPES},
  {"RESTRICTED_RWP",RESTIRICTED_RWP},
  {"CONNECTED_DOMAIN", CONNECTED_DOMAIN},
  {"MAX_RWP_TYPES", MAX_RWP_TYPES},
  {NULL, -1}
};

void  ocg_config_proto(void);

void init_oai_emulation(void)
{

  int i;
  int CC_id;

  oai_emulation.environment_system_config.fading.large_scale.selected_option = "free_space";
  oai_emulation.environment_system_config.fading.free_space_model_parameters.pathloss_exponent = 3.00;
  oai_emulation.environment_system_config.fading.free_space_model_parameters.pathloss_0_dB = -100;
  oai_emulation.environment_system_config.fading.small_scale.selected_option = "AWGN";
  oai_emulation.environment_system_config.fading.ricean_8tap.rice_factor_dB = 0;
  oai_emulation.environment_system_config.fading.shadowing.decorrelation_distance_m = 0;
  oai_emulation.environment_system_config.fading.shadowing.variance_dB = 0;
  oai_emulation.environment_system_config.fading.shadowing.inter_site_correlation = 0;
  oai_emulation.environment_system_config.antenna.eNB_antenna.number_of_sectors = 1;
  oai_emulation.environment_system_config.antenna.eNB_antenna.beam_width_dB = 1.13;
  oai_emulation.environment_system_config.antenna.eNB_antenna.alpha_rad[0] = 0;
  oai_emulation.environment_system_config.antenna.eNB_antenna.alpha_rad[1] = 0;
  oai_emulation.environment_system_config.antenna.eNB_antenna.alpha_rad[2] = 0;
  oai_emulation.environment_system_config.antenna.eNB_antenna.antenna_gain_dBi = 0;
  oai_emulation.environment_system_config.antenna.eNB_antenna.tx_power_dBm = 15;
  oai_emulation.environment_system_config.antenna.eNB_antenna.rx_noise_level_dB = 0;
  oai_emulation.environment_system_config.antenna.eNB_antenna.antenna_orientation_degree[0] = 0;
  oai_emulation.environment_system_config.antenna.eNB_antenna.antenna_orientation_degree[1] = 0;
  oai_emulation.environment_system_config.antenna.eNB_antenna.antenna_orientation_degree[2] = 0;
  oai_emulation.environment_system_config.antenna.UE_antenna.antenna_gain_dBi = 0;
  oai_emulation.environment_system_config.antenna.UE_antenna.tx_power_dBm = 20;
  oai_emulation.environment_system_config.antenna.UE_antenna.rx_noise_level_dB = 0; // noise figure
  oai_emulation.environment_system_config.wall_penetration_loss_dB = 5;
  oai_emulation.environment_system_config.system_bandwidth_MB = 7.68;
  oai_emulation.environment_system_config.system_frequency_GHz = 1.9;

  oai_emulation.topology_config.area.x_m = 1000;
  oai_emulation.topology_config.area.y_m = 1000;
  oai_emulation.topology_config.network_type.selected_option = "homogeneous";
  oai_emulation.topology_config.cell_type.selected_option = "macrocell";
  oai_emulation.topology_config.relay.number_of_relays = 0;
  oai_emulation.topology_config.mobility.UE_mobility.UE_mobility_type.selected_option = "STATIC";
  oai_emulation.topology_config.mobility.UE_mobility.grid_walk.grid_map.selected_option="MAX_RWP_TYPES";
  //oai_emulation.topology_config.mobility.UE_mobility.grid_walk.grid_map.horizontal_grid = 1;
  //oai_emulation.topology_config.mobility.UE_mobility.grid_walk.grid_map.vertical_grid = 1;
  oai_emulation.topology_config.mobility.UE_mobility.grid_walk.grid_trip_type.selected_option = "random_destination";
  oai_emulation.topology_config.mobility.UE_mobility.UE_initial_distribution.selected_option = "random";
  oai_emulation.topology_config.mobility.UE_mobility.random_UE_distribution.number_of_nodes = 1;
  oai_emulation.topology_config.mobility.UE_mobility.concentrated_UE_distribution.number_of_nodes = 1;
  oai_emulation.topology_config.mobility.UE_mobility.grid_UE_distribution.random_grid.number_of_nodes = 1;
  oai_emulation.topology_config.mobility.UE_mobility.grid_UE_distribution.border_grid.number_of_nodes = 1;
  oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.min_speed_mps = 0.1;
  oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.max_speed_mps = 20.0;
  oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.min_sleep_ms = 0.1;
  oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.max_sleep_ms = 5.0;
  oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.min_journey_time_ms = 0.1;
  oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.max_journey_time_ms = 10.0;
  oai_emulation.topology_config.mobility.eNB_mobility.eNB_mobility_type.selected_option = "STATIC";
  oai_emulation.topology_config.mobility.eNB_mobility.eNB_initial_distribution.selected_option = "random";
  oai_emulation.topology_config.mobility.eNB_mobility.fixed_eNB_distribution.pos_x = 1;
  oai_emulation.topology_config.mobility.eNB_mobility.fixed_eNB_distribution.pos_y = 1;
  oai_emulation.topology_config.mobility.eNB_mobility.random_eNB_distribution.number_of_cells = 1;
  oai_emulation.topology_config.mobility.eNB_mobility.hexagonal_eNB_distribution.number_of_cells = 1;
  oai_emulation.topology_config.mobility.eNB_mobility.hexagonal_eNB_distribution.inter_eNB_distance_km = 1;
  oai_emulation.topology_config.mobility.eNB_mobility.grid_eNB_distribution.number_of_grid_x = 1;
  oai_emulation.topology_config.mobility.eNB_mobility.grid_eNB_distribution.number_of_grid_y = 1;
  oai_emulation.topology_config.mobility.eNB_mobility.trace_config.trace_mobility_file = (char*) malloc(256);
  sprintf(oai_emulation.topology_config.mobility.eNB_mobility.trace_config.trace_mobility_file,"static_1enb.tr");
  oai_emulation.topology_config.mobility.UE_mobility.trace_config.trace_mobility_file = (char*) malloc(256);
  sprintf(oai_emulation.topology_config.mobility.UE_mobility.trace_config.trace_mobility_file,"static_2ues.tr");
  oai_emulation.topology_config.mobility.UE_mobility.sumo_config.command = (char*) malloc(20);
  sprintf(oai_emulation.topology_config.mobility.UE_mobility.sumo_config.command,"sumo");
  oai_emulation.topology_config.mobility.UE_mobility.sumo_config.file = (char*) malloc(256);
  snprintf( oai_emulation.topology_config.mobility.UE_mobility.sumo_config.file, 256, "%s/UTIL/OMG/SUMO/SCENARIOS/scen.sumo.cfg", getenv("OPENAIR2_DIR") );
  oai_emulation.topology_config.mobility.UE_mobility.sumo_config.start=0;
  oai_emulation.topology_config.mobility.UE_mobility.sumo_config.end=0;
  oai_emulation.topology_config.mobility.UE_mobility.sumo_config.step=1; //  1000ms
  oai_emulation.topology_config.mobility.UE_mobility.sumo_config.hip = (char*) malloc(40);
  sprintf(oai_emulation.topology_config.mobility.UE_mobility.sumo_config.hip,"127.0.1.1");
  oai_emulation.topology_config.mobility.UE_mobility.sumo_config.hport = 8883;

  oai_emulation.application_config.packet_gen_type = "substract_string";

  for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
    oai_emulation.application_config.predefined_traffic.source_id[i] = "1:10";
    oai_emulation.application_config.predefined_traffic.application_type[i] = "no_predefined_traffic";
    oai_emulation.application_config.predefined_traffic.background[i] = "disable";
    oai_emulation.application_config.predefined_traffic.aggregation_level[i] = 1;
    oai_emulation.application_config.predefined_traffic.flow_start[i] = 0;
    oai_emulation.application_config.predefined_traffic.flow_duration[i] = 1000;//uniform_dist((i+1)*100,0xffff);

    oai_emulation.application_config.predefined_traffic.destination_id[i] = 0;

    oai_emulation.application_config.customized_traffic.source_id[i] = "1";
    oai_emulation.application_config.customized_traffic.destination_id[i] = "2";
    oai_emulation.application_config.customized_traffic.transport_protocol[i] = "udp";
    oai_emulation.application_config.customized_traffic.background[i] = "disable";
    oai_emulation.application_config.customized_traffic.m2m[i] = "disable";
    oai_emulation.application_config.customized_traffic.ip_version[i] = "ipv4";
    oai_emulation.application_config.customized_traffic.aggregation_level[i] = 1;
    oai_emulation.application_config.customized_traffic.flow_start[i] = 0;// (i+1)*30;
    oai_emulation.application_config.customized_traffic.flow_duration[i] = 1000;// uniform_dist((i+1)*100,0xffff);
    oai_emulation.application_config.customized_traffic.idt_dist[i] = "uniform";
    oai_emulation.application_config.customized_traffic.idt_min_ms[i] = 100;
    oai_emulation.application_config.customized_traffic.idt_max_ms[i] = 1000;
    oai_emulation.application_config.customized_traffic.idt_standard_deviation[i] = 1;
    oai_emulation.application_config.customized_traffic.idt_lambda[i] = 1;
    oai_emulation.application_config.customized_traffic.size_dist[i] = "uniform";
    oai_emulation.application_config.customized_traffic.size_min_byte[i] = 200;
    oai_emulation.application_config.customized_traffic.size_max_byte[i] = 500;
    oai_emulation.application_config.customized_traffic.size_standard_deviation[i] = 1;
    oai_emulation.application_config.customized_traffic.size_lambda[i] = 1;
    oai_emulation.application_config.customized_traffic.stream[i] = 1;
    oai_emulation.application_config.customized_traffic.destination_port[i] = 8080;
    oai_emulation.application_config.customized_traffic.prob_off_pu[i]= 0;
    oai_emulation.application_config.customized_traffic.prob_off_ed[i]= 0;
    oai_emulation.application_config.customized_traffic.prob_off_pe[i]= 0;
    oai_emulation.application_config.customized_traffic.prob_pu_ed[i]= 0;
    oai_emulation.application_config.customized_traffic.prob_pu_pe[i]= 0;
    oai_emulation.application_config.customized_traffic.prob_ed_pe[i]= 0;
    oai_emulation.application_config.customized_traffic.prob_ed_pu[i]= 0;
    oai_emulation.application_config.customized_traffic.holding_time_off_ed[i]= 0;
    oai_emulation.application_config.customized_traffic.holding_time_off_pu[i]= 0;
    oai_emulation.application_config.customized_traffic.holding_time_off_pe[i]= 0;
    oai_emulation.application_config.customized_traffic.holding_time_pe_off[i]= 0;
    oai_emulation.application_config.customized_traffic.pu_size_pkts[i]= 0;
    oai_emulation.application_config.customized_traffic.ed_size_pkts[i]= 0;
  }

  /* protocol config */
  oai_emulation.protocol_config.eNB_mac_config.num_groups=1;


  oai_emulation.emulation_config.emulation_time_ms = 0;
  oai_emulation.emulation_config.curve = "disable";
  oai_emulation.emulation_config.background_stats = "disable";
  oai_emulation.emulation_config.performance_metrics.throughput = "disable";
  oai_emulation.emulation_config.performance_metrics.latency ="disable";
  oai_emulation.emulation_config.performance_metrics.loss_rate ="disable";
  oai_emulation.emulation_config.performance_metrics.owd_radio_access = "disable";
  oai_emulation.emulation_config.layer.phy = 0;
  oai_emulation.emulation_config.layer.mac = 0;
  oai_emulation.emulation_config.layer.rlc = 0;
  oai_emulation.emulation_config.layer.pdcp = 0;
  oai_emulation.emulation_config.layer.rrc = 0;
  oai_emulation.emulation_config.layer.omg = 0;
  oai_emulation.emulation_config.layer.otg = 0;
  oai_emulation.emulation_config.layer.emu = 1;

  oai_emulation.emulation_config.log_emu.level = "debug";
  oai_emulation.emulation_config.log_emu.verbosity = "low";
  oai_emulation.emulation_config.log_emu.interval = 1;
  oai_emulation.emulation_config.packet_trace.enabled = 0;
  oai_emulation.emulation_config.seed.value = 0; // 0 means randomly generated by OAI

  oai_emulation.info.ocg_ok = 0;


  // phy related params :
  oai_emulation.info.n_frames=MAX_FRAME_NUMBER; // number of frames simulated by default
  oai_emulation.info.n_frames_flag=0; // if set, then let the emulation goes to infinity

  //status
  oai_emulation.info.is_primary_master=0;
  oai_emulation.info.master_list=0;
  oai_emulation.info.nb_ue_remote=0;
  oai_emulation.info.nb_enb_remote=0;
  oai_emulation.info.nb_rn_remote=0;
  oai_emulation.info.first_ue_local=0;
  oai_emulation.info.first_enb_local=0;
  oai_emulation.info.first_rn_local=0;
  oai_emulation.info.master_id=0;
  oai_emulation.info.nb_ue_local= 1;//default 1 UE
  oai_emulation.info.nb_enb_local= 0;//default 0 eNB (to be loaded from config)
  oai_emulation.info.nb_ru_local= 0;//default 0 ru (to be loaded from config)
  oai_emulation.info.nb_rn_local= 0;//default 0 RN : currently only applicable for eMBMS
  oai_emulation.info.ethernet_flag=0;
  oai_emulation.info.ocm_enabled=1;// flag ?
  oai_emulation.info.ocg_enabled=0;// flag c
  oai_emulation.info.otg_enabled=0;// flag T
  oai_emulation.info.opt_enabled=0; // P flag
  oai_emulation.info.opt_mode=-1; // arg for P flag
  oai_emulation.info.cli_enabled=0;// I flag
  oai_emulation.info.omv_enabled =0; // v flag
  oai_emulation.info.vcd_enabled=0;
  oai_emulation.info.opp_enabled=0;
  oai_emulation.info.oeh_enabled=0;

  oai_emulation.info.cba_group_active=0;
  oai_emulation.info.cba_backoff=30;
  oai_emulation.info.eMBMS_active_state=0;
  oai_emulation.info.handover_active=0;
  oai_emulation.info.omg_model_enb=STATIC; //default to static mobility model
  oai_emulation.info.omg_model_rn=STATIC; //default to static mobility model
  oai_emulation.info.omg_model_ue=STATIC; //default to static mobility model
  oai_emulation.info.omg_rwp_type = RESTIRICTED_RWP;
  oai_emulation.info.omg_model_ue_current=STATIC; //default to static mobility model
  oai_emulation.info.otg_traffic="no_predefined_traffic";
  oai_emulation.info.otg_bg_traffic_enabled = 0; // G flag
  oai_emulation.info.max_predefined_traffic_config_index = 0;
  oai_emulation.info.max_customized_traffic_config_index = 0;
  oai_emulation.info.frame = 0; // frame counter of emulation
  oai_emulation.info.time_s = 0; // time of emulation
  oai_emulation.info.time_ms = 0; // time of emulation
  oai_emulation.info.seed = time(NULL); // time-based random seed , , included in ocg report

  oai_emulation.info.cli_num_enb= NUMBER_OF_eNB_MAX;
  oai_emulation.info.cli_num_ue= NUMBER_OF_UE_MAX;

  oai_emulation.info.slot_isr = 0;
  oai_emulation.info.slot_sfd = -1;

  //for (i=0; i < oai_emulation.info.cli_num_enb; i++)
  for (i=0; i < NUMBER_OF_eNB_MAX; i++)
    oai_emulation.info.cli_start_enb[i]=1;

  // for (i=0; i < oai_emulation.info.cli_num_ue; i++)
  for (i=0; i < NUMBER_OF_UE_MAX; i++)
    oai_emulation.info.cli_start_ue[i]=1;

  for (i=0; i < NUMBER_OF_eNB_MAX+NUMBER_OF_UE_MAX; i++)
    oai_emulation.info.oai_ifup[i]=0;

  oai_emulation.info.nb_master       =0;
  oai_emulation.info.ethernet_id     =0;
  oai_emulation.info.multicast_group =0;
  oai_emulation.info.multicast_ifname=NULL;
  oai_emulation.info.g_log_level     = LOG_INFO;
  oai_emulation.info.g_log_verbosity = 0x15;
  oai_emulation.info.g_log_verbosity_option = "medium";

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    oai_emulation.info.frame_type[CC_id]=TDD;
    oai_emulation.info.frame_type_name[CC_id]="TDD";
    oai_emulation.info.tdd_config[CC_id]=3;
    oai_emulation.info.tdd_config_S[CC_id]=0;
    oai_emulation.info.extended_prefix_flag[CC_id]=0;
    oai_emulation.info.N_RB_DL[CC_id]=25;
    oai_emulation.info.transmission_mode[CC_id]=2;
  }

  oai_emulation.profile = "EURECOM";
}


void oaisim_config(void)
{

  // init log gen first
  //initialize the log generator


  logInit();

#if defined(ENABLE_ITTI)
  itti_init(TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info, messages_definition_xml, oai_emulation.info.itti_dump_file);
  MSC_INIT(MSC_E_UTRAN, THREAD_MAX+TASK_MAX);
#endif

  // init ocg if enabled, otherwise take the params form the init_oai_emulation()
  //  and command line options given by the user
  if (oai_emulation.info.ocg_enabled == 1) { // activate OCG: xml-based scenario parser
    OCG_main(oai_emulation.info.local_server);// eurecom or portable

    if (oai_emulation.info.ocg_ok != 1) {
      LOG_E(OCG, "Error found by OCG; emulation not launched.\n");
      LOG_E(OCG, "Please find more information in the OCG_report.xml located at %s.\n", oai_emulation.info.output_path);
      exit (-1);
    }
  }


#if ENABLE_RAL
  mRAL_init_default_values();
  eRAL_init_default_values();
#endif
  olg_config();
  ocg_config_emu();
  ocg_config_env();// mobility gen
  ocg_config_topo(); // packet tracer using wireshark
  ocg_config_proto();

  // if T is set or ocg enabled
  if (oai_emulation.info.otg_enabled ) {
    set_component_filelog(OTG);
    set_component_filelog(OTG_LATENCY);
    set_component_filelog(OTG_GP);
    set_component_filelog(OTG_LATENCY_BG);
    set_component_filelog(OTG_GP_BG);
    set_component_filelog(OTG_JITTER);
    /* g_log->log_component[OTG].filelog=1;
    g_log->log_component[OTG_LATENCY].filelog=1;
    g_log->log_component[OTG_OWD].filelog = 1;*/
    ocg_config_app(); // packet generator
  }

  // add a var to control this, and pass this var to OMG
  set_component_filelog(OMG);
  LOG_I(OMG,"setting OMG file log \n");

}

int olg_config(void)
{
  int comp;
  int ocg_log_level     = map_str_to_int(log_level_names,     oai_emulation.emulation_config.log_emu.level);
  int ocg_log_verbosity = map_str_to_int(log_verbosity_names, oai_emulation.emulation_config.log_emu.verbosity);
  // fix me:
  oai_emulation.info.g_log_level     = ((oai_emulation.info.ocg_enabled == 1)  && (ocg_log_level != -1))     ? ocg_log_level : oai_emulation.info.g_log_level;
  oai_emulation.info.g_log_verbosity = (((oai_emulation.info.ocg_enabled == 1) && (ocg_log_verbosity != -1)) ? ocg_log_verbosity :
                                        map_str_to_int(log_verbosity_names, oai_emulation.info.g_log_verbosity_option));
  set_glog(oai_emulation.info.g_log_level, oai_emulation.info.g_log_verbosity ); //g_glog

  // component, log level, log interval
  for (comp = PHY; comp < MAX_LOG_COMPONENTS ; comp++)
    set_comp_log(comp,
                 oai_emulation.info.g_log_level,
                 oai_emulation.info.g_log_verbosity,
                 oai_emulation.emulation_config.log_emu.interval);

  LOG_I(EMU, "OCG log level %d, oai log level%d \n ",ocg_log_level, oai_emulation.info.g_log_level);
  LOG_N(EMU,"global log level is set to (%s,%d) with vebosity (%s, 0x%x) and frequency %d\n",
        map_int_to_str (log_level_names, oai_emulation.info.g_log_level),
        oai_emulation.info.g_log_level,
        map_int_to_str (log_verbosity_names,oai_emulation.info.g_log_verbosity),
        oai_emulation.info.g_log_verbosity,
        oai_emulation.emulation_config.log_emu.interval );


  /*
  // if perf eval then reset the otg log level
  set_comp_log(PHY,  LOG_EMERG, 0x15,1);
  set_comp_log(EMU,  LOG_EMERG, 0x15,1);
  set_comp_log(OCG,  LOG_EMERG, 0x15,1);
  set_comp_log(OCM,  LOG_EMERG, 0x15,1);
  set_comp_log(OTG,  LOG_EMERG, 0x15,1);
  set_comp_log(MAC,  LOG_EMERG, 0x15,1);
  set_comp_log(OMG,  LOG_EMERG, 0x15,1);
  set_comp_log(OPT,  LOG_EMERG, 0x15,1);
  set_comp_log(PDCP, LOG_DEBUG, LOG_MED,1);
  set_comp_log(RLC,  LOG_DEBUG, LOG_MED,1);
  set_comp_log(RRC,  LOG_DEBUG, LOG_MED,1);
  */
#if defined(ENABLE_USE_MME)
  set_comp_log(UDP_,  LOG_DEBUG, LOG_MED,1);
  set_comp_log(GTPU,  LOG_DEBUG, LOG_MED,1);
  set_comp_log(SPGW,  LOG_DEBUG, LOG_MED,1);
  set_comp_log(S1AP,  LOG_DEBUG, LOG_MED,1);
  set_comp_log(SCTP,  LOG_DEBUG, LOG_MED,1);
#endif

#if ENABLE_RAL
  set_comp_log(RAL_ENB, LOG_TRACE, LOG_MED,1);
  set_comp_log(RAL_UE,  LOG_TRACE, LOG_MED,1);
  set_log(RAL_ENB,  LOG_DEBUG, 1);
  set_log(RAL_UE,  LOG_DEBUG, 1);
#endif


   //set_log(OCG,  LOG_DEBUG, 1);
   //set_log(EMU,  LOG_INFO,  20);
   set_log(MAC,  LOG_DEBUG, 1);
   set_log(RLC,  LOG_TRACE, 1);
   //set_log(PHY,  LOG_DEBUG, 1);
   set_log(PDCP, LOG_TRACE, 1);
   set_log(RRC,  LOG_DEBUG, 1);
   //set_log(OCM,  LOG_INFO, 20);
   //set_log(OTG,  LOG_INFO, 1);
   set_comp_log(OCG,  LOG_ERR, 0x15,1);
   set_comp_log(EMU,  LOG_DEBUG,  0x15,20);
   set_comp_log(MAC,  LOG_TRACE, 0x15,1);
   set_comp_log(RLC,  LOG_TRACE, 0x15,1);
   set_comp_log(PHY,  LOG_TRACE, 0x15, 1);
   set_comp_log(PDCP, LOG_DEBUG, 0x15,1);
   set_comp_log(RRC,  LOG_DEBUG, 0x15,1);
   set_comp_log(OCM,  LOG_DEBUG, 0x15,20);
   set_comp_log(OTG,  LOG_DEBUG, 0x15,1);
   set_comp_log(OMG,  LOG_NOTICE, 0x15,1);
   set_comp_log(OPT,  LOG_ERR, 0x15,1);


  // set_comp_log(MAC, LOG_TRACE, LOG_FULL,1);
  return 1;
}

int ocg_config_env(void)
{
  // int func related to channel desc from oaisim.c could be moved here
  int CC_id;

  if (oai_emulation.info.ocg_enabled) {
    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      oai_emulation.info.frame_type[CC_id] = map_str_to_int(frame_type_names, oai_emulation.info.frame_type_name[CC_id]);

      if (oai_emulation.info.frame_type[CC_id] == -1) {
        LOG_E(EMU,"frame type incorrect %s, set it to TDD \n",oai_emulation.info.frame_type_name[CC_id]);
        oai_emulation.info.frame_type[CC_id]=TDD;
      } else
        LOG_I(EMU,"Frame type is %s \n",oai_emulation.info.frame_type_name[CC_id]);

      if (oai_emulation.info.frame_type[CC_id] == TDD ) {
        if ((oai_emulation.info.tdd_config[CC_id] > 6) || (oai_emulation.info.tdd_config[CC_id] < 0)) {
          LOG_E(EMU,"TDD config %d out of range, set it to 3\n",oai_emulation.info.tdd_config[CC_id]);
          oai_emulation.info.tdd_config[CC_id]=3;
        } else
          LOG_I(EMU,"TDD config is set to %d\n",oai_emulation.info.tdd_config[CC_id]);
      }
    }
  }

  return 1;
}
int ocg_config_topo(void)
{

  int i;
  // omg
  init_omg_global_params();

  // setup params for openair mobility generator
  //common params
  for(i=0; i < MAX_NUM_NODE_TYPES; i++) {

    omg_param_list[i].min_x = 0;
    omg_param_list[i].max_x = oai_emulation.topology_config.area.x_m;
    omg_param_list[i].min_y = 0;
    omg_param_list[i].max_y= oai_emulation.topology_config.area.y_m;
    // init values
    omg_param_list[i].min_speed = 0.1;
    omg_param_list[i].max_speed = 20.0;
    omg_param_list[i].min_journey_time = 0.1;
    omg_param_list[i].max_journey_time = 10.0;
    omg_param_list[i].min_azimuth = 0; // ???
    omg_param_list[i].max_azimuth = 360; // ???
    omg_param_list[i].min_sleep = 0.1;
    omg_param_list[i].max_sleep = 8.0;
    omg_param_list[i].nodes_type = i;
    omg_param_list[i].nodes=0;
    omg_param_list[i].mobility_type=STATIC;

    // init OMG for eNBs

    if(i==eNB) {
      if ((oai_emulation.info.omg_model_enb = map_str_to_int(omg_model_names, oai_emulation.topology_config.mobility.eNB_mobility.eNB_mobility_type.selected_option))== -1)
        oai_emulation.info.omg_model_enb = STATIC;

      LOG_I(OMG,"eNB mobility model is (%s, %d)\n",
            oai_emulation.topology_config.mobility.eNB_mobility.eNB_mobility_type.selected_option,
            oai_emulation.info.omg_model_enb);

      omg_param_list[i].mobility_type = oai_emulation.info.omg_model_enb;
      omg_param_list[i].nodes = oai_emulation.info.nb_enb_local + oai_emulation.info.nb_rn_local;
      omg_param_list[i].seed = oai_emulation.info.seed; // specific seed for enb and ue to avoid node overlapping


      if (oai_emulation.info.omg_model_enb == TRACE) {
        omg_param_list[i].mobility_file = (char*) malloc(256);// user-specific trace file "%s/UTIL/OMG/mobility.txt",getenv("OPENAIR2_DIR")
        //memset(oai_emulation.topology_config.mobility.UE_mobility.trace_config.trace_mobility_file,0,256);
        //sprintf(omg_param_list.mobility_file,"%s",oai_emulation.topology_config.mobility.UE_mobility.trace_config.trace_mobility_file);
        sprintf(omg_param_list[i].mobility_file,"%s/UTIL/OMG/TRACE/%s",
                getenv("OPENAIR2_DIR"),
                oai_emulation.topology_config.mobility.eNB_mobility.trace_config.trace_mobility_file);
        LOG_I(OMG,"TRACE file at %s\n", omg_param_list[i].mobility_file);
        // notify the user if the file is not found
      }



      if (strcmp(oai_emulation.topology_config.mobility.eNB_mobility.eNB_initial_distribution.selected_option, "fixed") == 0) {
        omg_param_list[i].user_fixed = true;
        omg_param_list[i].fixed_x = (double)oai_emulation.topology_config.mobility.eNB_mobility.fixed_eNB_distribution.pos_x;
        omg_param_list[i].fixed_y = (double)oai_emulation.topology_config.mobility.eNB_mobility.fixed_eNB_distribution.pos_y;
      }

      /*// at this moment, we use the above moving dynamics for mobile eNB
      if (omg_param_list.nodes >0 )
      init_mobility_generator(omg_param_list);*/

    } else if (i== UE) {      // init OMG for UE


      // input of OMG: STATIC: 0, RWP: 1, RWALK 2, or TRACE 3, or SUMO or STEADY_RWP
      if ((oai_emulation.info.omg_model_ue = map_str_to_int(omg_model_names, oai_emulation.topology_config.mobility.UE_mobility.UE_mobility_type.selected_option))== -1)
        oai_emulation.info.omg_model_ue = STATIC;

      LOG_I(OMG,"UE mobility model is (%s, %d)\n",
            oai_emulation.topology_config.mobility.UE_mobility.UE_mobility_type.selected_option,
            oai_emulation.info.omg_model_ue);

      if (oai_emulation.info.omg_model_ue == RWP) {
        oai_emulation.info.omg_rwp_type = map_str_to_int(omg_rwp_names,oai_emulation.topology_config.mobility.UE_mobility.grid_walk.grid_map.selected_option);

        if (oai_emulation.info.omg_rwp_type == -1) {
          LOG_I(OMG,"Apply standard RWP model \n");
        } else {
          LOG_I(OMG,"Apply %s mobility model (%d) \n", oai_emulation.topology_config.mobility.UE_mobility.grid_walk.grid_map.selected_option,
                oai_emulation.info.omg_rwp_type);
          omg_param_list[i].rwp_type=  oai_emulation.info.omg_rwp_type;
          omg_param_list[i].max_vertices =
            max_vertices_ongrid (omg_param_list[i]);

          omg_param_list[i].max_block_num =
            max_connecteddomains_ongrid (omg_param_list[i]);
        }
      }

      omg_param_list[i].mobility_type    = oai_emulation.info.omg_model_ue;
      omg_param_list[i].nodes = oai_emulation.info.nb_ue_local+ oai_emulation.info.nb_rn_local;
      omg_param_list[i].seed = oai_emulation.info.seed + oai_emulation.info.nb_ue_local; //fixme: specific seed for enb and ue to avoid node overlapping

      omg_param_list[i].min_speed = (oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.min_speed_mps == 0) ? 0.1 :
                                    oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.min_speed_mps;

      omg_param_list[i].max_speed = (oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.max_speed_mps == 0) ? 0.1 :
                                    oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.max_speed_mps;

      omg_param_list[i].min_journey_time = (oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.min_journey_time_ms == 0) ? 0.1 :
                                           oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.min_journey_time_ms;

      omg_param_list[i].max_journey_time = (oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.max_journey_time_ms == 0) ? 0.1 :
                                           oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.max_journey_time_ms;

      omg_param_list[i].min_azimuth = 0.1; // wait for advanced OSD
      omg_param_list[i].max_azimuth = 360;

      omg_param_list[i].min_sleep = (oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.min_sleep_ms == 0) ? 0.1 :
                                    oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.min_sleep_ms;

      omg_param_list[i].max_sleep = (oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.max_sleep_ms == 0) ? 0.1 :
                                    oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.max_sleep_ms;


      if (oai_emulation.info.omg_model_ue == TRACE) {
        omg_param_list[i].mobility_file = (char*) malloc(256);// user-specific trace file "%s/UTIL/OMG/mobility.txt",getenv("OPENAIR2_DIR")
        //memset(oai_emulation.topology_config.mobility.UE_mobility.trace_config.trace_mobility_file,0,256);
        //sprintf(omg_param_list.mobility_file,"%s",oai_emulation.topology_config.mobility.UE_mobility.trace_config.trace_mobility_file);
        snprintf( omg_param_list[i].mobility_file, 256, "%s/UTIL/OMG/TRACE/%s",
                  getenv("OPENAIR2_DIR"),
                  oai_emulation.topology_config.mobility.UE_mobility.trace_config.trace_mobility_file);
        LOG_I(OMG,"TRACE file at %s\n", omg_param_list[i].mobility_file);

      } else if (oai_emulation.info.omg_model_ue == SUMO) {
        omg_param_list[i].sumo_command = (char*) malloc(20);
        sprintf(omg_param_list[i].sumo_command, "%s", oai_emulation.topology_config.mobility.UE_mobility.sumo_config.command);
        omg_param_list[i].sumo_config = (char*) malloc(256);
        sprintf(omg_param_list[i].sumo_config, "%s", oai_emulation.topology_config.mobility.UE_mobility.sumo_config.file);
        omg_param_list[i].sumo_start = oai_emulation.topology_config.mobility.UE_mobility.sumo_config.start;

        if (oai_emulation.topology_config.mobility.UE_mobility.sumo_config.end > 0 )
          omg_param_list[i].sumo_end = oai_emulation.topology_config.mobility.UE_mobility.sumo_config.end;
        else
          omg_param_list[i].sumo_end = (oai_emulation.info.n_frames_flag == 1 ) ?  oai_emulation.info.n_frames : MAX_FRAME_NUMBER ; // fixme: the else case is infinity

        omg_param_list[i].sumo_step = oai_emulation.topology_config.mobility.UE_mobility.sumo_config.step=1; //  1000ms
        omg_param_list[i].sumo_host = (char*) malloc(40);
        sprintf(omg_param_list[i].sumo_host,"%s",oai_emulation.topology_config.mobility.UE_mobility.sumo_config.hip);
        omg_param_list[i].sumo_port = oai_emulation.topology_config.mobility.UE_mobility.sumo_config.hport ;
        LOG_D(OMG, "opt (%s,%d) cmd (%s,%s) config_file (%s,%s) hip (%s,%s) \n",
              oai_emulation.topology_config.mobility.UE_mobility.UE_mobility_type.selected_option,oai_emulation.info.omg_model_ue,
              omg_param_list[i].sumo_command, oai_emulation.topology_config.mobility.UE_mobility.sumo_config.command,
              omg_param_list[i].sumo_config, oai_emulation.topology_config.mobility.UE_mobility.sumo_config.file,
              omg_param_list[i].sumo_host, oai_emulation.topology_config.mobility.UE_mobility.sumo_config.hip);
      }

    }

    if (oai_emulation.topology_config.omv == 1 )
      oai_emulation.info.omv_enabled =  1;

  }//for


  init_mobility_generator(omg_param_list);

  return 1;
}



int ocg_config_app(void)
{

  char colon[] = ":";
  char comma[] = ",";
  char tmp_source_id[128];
  char tmp_destination_id[128];
  char *check_format1;
  char *check_format2;
  char *check_format1_dst;
  char *source_id_start = NULL;
  char *source_id_end = NULL;
  char *destination_id_start;
  char *destination_id_end;
  int sid_start;
  int sid_end;
  int did_start;
  int did_end;
  char *per_source_id;
  int source_id_index;
  int destination_id_index;
  int i,j,k,l;

  int predefined_traffic_config_index;
  int customized_traffic_config_index;
  unsigned int state;

  init_all_otg(oai_emulation.info.n_frames);
  g_otg->seed= oai_emulation.info.seed;
  g_otg->num_nodes = oai_emulation.info.nb_enb_local + oai_emulation.info.nb_ue_local;
  g_otg->throughput_metric =map_str_to_int(switch_names,oai_emulation.emulation_config.performance_metrics.throughput);
  g_otg->latency_metric =map_str_to_int(switch_names,oai_emulation.emulation_config.performance_metrics.latency);
  g_otg->loss_metric =map_str_to_int(switch_names,oai_emulation.emulation_config.performance_metrics.loss_rate);
  g_otg->owd_radio_access =map_str_to_int(switch_names,oai_emulation.emulation_config.performance_metrics.owd_radio_access);
  g_otg->curve=map_str_to_int(switch_names,oai_emulation.emulation_config.curve);
  g_otg->background_stats=map_str_to_int(switch_names,oai_emulation.emulation_config.background_stats);

  g_otg->packet_gen_type=map_str_to_int(packet_gen_names,oai_emulation.application_config.packet_gen_type);

  LOG_I(OTG,"Metrics: Throuput %s(%d), One-way latency %s(%d) for %s, loss rate %s(%d), online curves %s(%d)\n",
        oai_emulation.emulation_config.performance_metrics.throughput,g_otg->throughput_metric,
        oai_emulation.emulation_config.performance_metrics.latency,g_otg->latency_metric,
        (g_otg->owd_radio_access==1)? "Radio Access Network" : "End to End",
        oai_emulation.emulation_config.performance_metrics.loss_rate,g_otg->loss_metric,
        oai_emulation.emulation_config.curve,g_otg->curve );

  for (i=0; i<g_otg->num_nodes; i++) {

    for (j=0; j<g_otg->num_nodes; j++) {
      // g_otg->duration[i][j]=oai_emulation.emulation_config.emulation_time_ms;
      g_otg->dst_port[i][j]=oai_emulation.application_config.customized_traffic.destination_port[i];
      g_otg->dst_ip[i][j]=oai_emulation.application_config.customized_traffic.destination_id[i];

      for (k=0; k<MAX_NUM_APPLICATION; k++) {
        g_otg->ip_v[i][j][k]=map_str_to_int(otg_ip_version_names, oai_emulation.application_config.customized_traffic.ip_version[i]);
        g_otg->trans_proto[i][j][k]=map_str_to_int(otg_transport_protocol_names, oai_emulation.application_config.customized_traffic.transport_protocol[i]);
        g_otg->application_type[i][j][k]=map_str_to_int(otg_app_type_names, "no_predefined_traffic");
        g_otg->aggregation_level[i][j][k]=oai_emulation.application_config.customized_traffic.aggregation_level[i];

        g_otg->flow_start[i][j][k]=flow_start_time(i,j,oai_emulation.info.n_frames,
                                   oai_emulation.application_config.customized_traffic.flow_start[i+j],
                                   oai_emulation.application_config.customized_traffic.flow_duration[i+j]);
        g_otg->flow_duration[i][j][k]=oai_emulation.application_config.customized_traffic.flow_duration[i+j];

        for (l=0; l<MAX_NUM_TRAFFIC_STATE; l++) {
          g_otg->idt_dist[i][j][k][l]=map_str_to_int(otg_distribution_names, "no_customized_traffic");
          g_otg->idt_min[i][j][k][l]=oai_emulation.application_config.customized_traffic.idt_min_ms[i];
          g_otg->idt_max[i][j][k][l]=oai_emulation.application_config.customized_traffic.idt_max_ms[i];
          g_otg->idt_std_dev[i][j][k][l]=oai_emulation.application_config.customized_traffic.idt_standard_deviation[i];
          g_otg->idt_lambda[i][j][k][l]=oai_emulation.application_config.customized_traffic.idt_lambda[i];
          g_otg->idt_scale[i][j][k][l]=oai_emulation.application_config.customized_traffic.idt_scale[i];
          g_otg->idt_shape[i][j][k][l]=oai_emulation.application_config.customized_traffic.idt_shape[i];
          g_otg->size_dist[i][j][k][l]=map_str_to_int(otg_distribution_names, "no_customized_traffic");
          g_otg->size_min[i][j][k][l]=oai_emulation.application_config.customized_traffic.size_min_byte[i];
          g_otg->size_max[i][j][k][l]=oai_emulation.application_config.customized_traffic.size_max_byte[i];
          g_otg->size_std_dev[i][j][k][l]=oai_emulation.application_config.customized_traffic.size_standard_deviation[i];
          g_otg->size_lambda[i][j][k][l]=oai_emulation.application_config.customized_traffic.size_lambda[i];
          g_otg->size_scale[i][j][k][l]=oai_emulation.application_config.customized_traffic.size_scale[i];
          g_otg->size_shape[i][j][k][l]=oai_emulation.application_config.customized_traffic.size_shape[i];
        }
      }
    }
  }

  LOG_D(OTG,"initializing the RNG with %s %d\n",(g_otg->seed==0)?"Random Seed" : "Fixed Seed", g_otg->seed);
  init_seeds(g_otg->seed); // initialize all the nodes, then configure the nodes the user specifically did in the XML in the following

  LOG_I(OTG,"oai_emulation.info.max_predefined_traffic_config_index = %d\n", oai_emulation.info.max_predefined_traffic_config_index);
  LOG_I(OTG,"oai_emulation.info.max_customized_traffic_config_index = %d\n", oai_emulation.info.max_customized_traffic_config_index);


  if (oai_emulation.info.ocg_ok) {
    ///// for the predefined traffic
    for (predefined_traffic_config_index = 1;
         predefined_traffic_config_index <= oai_emulation.info.max_predefined_traffic_config_index;
         predefined_traffic_config_index++) {

      LOG_I(OTG,"OCG_config_OTG: predefined no. %d\n", predefined_traffic_config_index);

      //strcpy(tmp_source_id, oai_emulation.application_config.predefined_traffic.source_id[predefined_traffic_config_index]);
      //strcpy(tmp_destination_id, oai_emulation.application_config.predefined_traffic.destination_id[predefined_traffic_config_index]);
      strncpy(tmp_source_id, oai_emulation.application_config.predefined_traffic.source_id[predefined_traffic_config_index], sizeof(tmp_source_id));
      tmp_source_id[sizeof(tmp_source_id) - 1] = 0; // terminate string
      strncpy(tmp_destination_id, oai_emulation.application_config.predefined_traffic.destination_id[predefined_traffic_config_index], sizeof(tmp_destination_id));
      tmp_destination_id[sizeof(tmp_destination_id) - 1] = 0; // terminate string
      check_format1 = strstr(tmp_source_id, colon);
      check_format2 = strstr(tmp_source_id, comma);
      check_format1_dst = strstr(tmp_destination_id, colon);


      if (check_format1 != NULL) { // format 1:10
        source_id_start = strtok(tmp_source_id, colon);
        source_id_end = strtok(NULL, colon);

        sid_start = atoi(source_id_start);
        sid_end = atoi(source_id_end);

        //destination_id_index = atoi(oai_emulation.application_config.predefined_traffic.destination_id[predefined_traffic_config_index]);
        if (check_format1_dst != NULL) { // format 1:10
          destination_id_start = strtok(tmp_destination_id, colon);
          destination_id_end = strtok(NULL, colon);

          did_start = atoi(destination_id_start);
          did_end = atoi(destination_id_end);
        } else {
          did_start = atoi(oai_emulation.application_config.predefined_traffic.destination_id[predefined_traffic_config_index]);
          did_end = atoi(oai_emulation.application_config.predefined_traffic.destination_id[predefined_traffic_config_index]);
        }

        for (destination_id_index = did_start; destination_id_index <= did_end; destination_id_index++) {
          for (source_id_index = sid_start; source_id_index <= sid_end; source_id_index++) {
            if (g_otg->application_type[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] == 0) { // no app is defined for this pair src.dst
              g_otg->application_type[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] = map_str_to_int(otg_app_type_names,
                  oai_emulation.application_config.predefined_traffic.application_type[predefined_traffic_config_index]);

              g_otg->background[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] =map_str_to_int(switch_names,
                  oai_emulation.application_config.predefined_traffic.background[predefined_traffic_config_index]);
              g_otg->aggregation_level[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.predefined_traffic.aggregation_level[predefined_traffic_config_index];
              g_otg->flow_start[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=  flow_start_time(source_id_index,destination_id_index,
                  oai_emulation.info.n_frames,oai_emulation.application_config.predefined_traffic.flow_start[predefined_traffic_config_index],
                  oai_emulation.application_config.predefined_traffic.flow_duration[predefined_traffic_config_index]);
              g_otg->flow_duration[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.predefined_traffic.flow_duration[predefined_traffic_config_index];

              LOG_I(OTG,"predef (1):: OCG_config_OTG [MAX UE=%d] [MAX eNB=%d]: FORMAT (%d:%d) source = %d, dest = %d, Application ID = %d background %d, Aggregation=%d , m2m= %d, flow start/duration %d/%d \n",
                    oai_emulation.info.nb_ue_local, oai_emulation.info.nb_enb_local,  sid_start, sid_end, source_id_index, destination_id_index,
                    g_otg->application_idx[source_id_index][destination_id_index],
                    g_otg->background[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                    g_otg->aggregation_level[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                    g_otg->m2m[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                    g_otg->flow_start[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                    g_otg->flow_duration[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]);
              g_otg->application_idx[source_id_index][destination_id_index]+=1;
            }
          }
        }
      } else if (check_format2 != NULL) { // format 1,2,3,5
        per_source_id = strtok(tmp_source_id, comma);
        destination_id_index = atoi(oai_emulation.application_config.predefined_traffic.destination_id[predefined_traffic_config_index]);

        while (per_source_id != NULL) {
          source_id_index = atoi(per_source_id);

          if (g_otg->application_type[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] == 0) { // no app is defined for this pair src.dst
            g_otg->application_type[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] = map_str_to_int(otg_app_type_names,
                oai_emulation.application_config.predefined_traffic.application_type[predefined_traffic_config_index]);
            g_otg->background[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] =map_str_to_int(switch_names,
                oai_emulation.application_config.predefined_traffic.background[predefined_traffic_config_index]);
            g_otg->aggregation_level[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.predefined_traffic.aggregation_level[predefined_traffic_config_index];
            g_otg->flow_start[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=flow_start_time(source_id_index, destination_id_index,
                oai_emulation.info.n_frames,oai_emulation.application_config.predefined_traffic.flow_start[predefined_traffic_config_index],
                oai_emulation.application_config.predefined_traffic.flow_duration[predefined_traffic_config_index]);
            g_otg->flow_duration[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.predefined_traffic.flow_duration[predefined_traffic_config_index];
            g_otg->application_idx[source_id_index][destination_id_index]+=1;
          }

          per_source_id = strtok(NULL, comma);

        }
      } else { // single node configuration
        source_id_index = atoi(oai_emulation.application_config.predefined_traffic.source_id[predefined_traffic_config_index]);
        //destination_id_index = atoi(oai_emulation.application_config.predefined_traffic.destination_id[predefined_traffic_config_index]);
        sid_start = atoi(oai_emulation.application_config.predefined_traffic.source_id[predefined_traffic_config_index]);
        sid_end = atoi(oai_emulation.application_config.predefined_traffic.source_id[predefined_traffic_config_index]);

        if (check_format1_dst != NULL) { // format 1:10
          destination_id_start = strtok(tmp_destination_id, colon);
          destination_id_end = strtok(NULL, colon);

          did_start = atoi(destination_id_start);
          did_end = atoi(destination_id_end);
        } else {
          did_start = atoi(oai_emulation.application_config.predefined_traffic.destination_id[predefined_traffic_config_index]);
          did_end = atoi(oai_emulation.application_config.predefined_traffic.destination_id[predefined_traffic_config_index]);
        }

        for (destination_id_index = did_start; destination_id_index <= did_end; destination_id_index++) {
          for (source_id_index = sid_start; source_id_index <= sid_end; source_id_index++) {
            if (g_otg->application_type[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] == 0) { // no app is defined for this pair src.dst
              g_otg->application_type[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] = map_str_to_int(otg_app_type_names,
                  oai_emulation.application_config.predefined_traffic.application_type[predefined_traffic_config_index]);
              g_otg->background[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] =map_str_to_int(switch_names,
                  oai_emulation.application_config.predefined_traffic.background[predefined_traffic_config_index]);
              g_otg->aggregation_level[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.predefined_traffic.aggregation_level[predefined_traffic_config_index];
              g_otg->flow_start[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=flow_start_time(source_id_index,destination_id_index,
                  oai_emulation.info.n_frames,oai_emulation.application_config.predefined_traffic.flow_start[predefined_traffic_config_index],
                  oai_emulation.application_config.predefined_traffic.flow_duration[predefined_traffic_config_index]);
              g_otg->flow_duration[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.predefined_traffic.flow_duration[predefined_traffic_config_index];

              LOG_I(OTG,"predef (2):: OCG_config_OTG: [MAX UE=%d] [MAX eNB=%d]: FORMAT (%d:%d) source = %d, dest = %d, Application ID %d, type %d, Background=%d, Aggregation=%d, m2m=%d, start/duration %d/%d\n",
                    oai_emulation.info.nb_ue_local, oai_emulation.info.nb_enb_local, sid_start, sid_end, source_id_index, destination_id_index,
                    g_otg->application_idx[source_id_index][destination_id_index],
                    g_otg->application_type[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                    g_otg->background[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                    g_otg->aggregation_level[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                    g_otg->m2m[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                    g_otg->flow_start[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                    g_otg->flow_duration[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]);
              g_otg->application_idx[source_id_index][destination_id_index]+=1;
            }
          }
        }
      }

      init_predef_traffic(oai_emulation.info.nb_ue_local, oai_emulation.info.nb_enb_local);
    }

    ///////// for the customized traffic
    for (customized_traffic_config_index = 1;
         customized_traffic_config_index <= oai_emulation.info.max_customized_traffic_config_index;
         customized_traffic_config_index++) {

      LOG_I(OTG,"OCG_config_OTG: customized no. %d, max_customized %d \n", customized_traffic_config_index, oai_emulation.info.max_customized_traffic_config_index);
      strcpy(tmp_source_id, oai_emulation.application_config.customized_traffic.source_id[customized_traffic_config_index]);

      check_format1 = strstr(tmp_source_id, colon);
      check_format2 = strstr(tmp_source_id, comma);

      strcpy(tmp_destination_id, oai_emulation.application_config.customized_traffic.destination_id[customized_traffic_config_index]);
      check_format1_dst = strstr(tmp_destination_id, colon);
      state=PE_STATE;

      if (check_format1 != NULL) { // format 1:10
        source_id_start = strtok(tmp_source_id, colon);
        source_id_end = strtok(NULL, colon) ;

        sid_start = atoi(source_id_start);
        sid_end = atoi(source_id_end);

        if (check_format1_dst != NULL) { // format 1:10
          destination_id_start = strtok(tmp_destination_id, colon);
          destination_id_end = strtok(NULL, colon);

          did_start = atoi(destination_id_start);
          did_end = atoi(destination_id_end);
        } else {
          did_start = atoi(oai_emulation.application_config.customized_traffic.destination_id[customized_traffic_config_index]);
          did_end = atoi(oai_emulation.application_config.customized_traffic.destination_id[customized_traffic_config_index]);
        }

        for (destination_id_index = did_start; destination_id_index <= did_end; destination_id_index++) {
          for (source_id_index = sid_start; source_id_index <= sid_end; source_id_index++) {
            if (g_otg->application_type[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] == 0) { // no app is defined for this pair src.dst
              g_otg->background[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] = map_str_to_int(switch_names,
                  oai_emulation.application_config.customized_traffic.background[customized_traffic_config_index]);
              g_otg->m2m[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] =map_str_to_int(switch_names,
                  oai_emulation.application_config.customized_traffic.m2m[customized_traffic_config_index]);
              g_otg->trans_proto[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] = map_str_to_int(otg_transport_protocol_names,
                  oai_emulation.application_config.customized_traffic.transport_protocol[customized_traffic_config_index]);
              g_otg->ip_v[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] = map_str_to_int(otg_ip_version_names,
                  oai_emulation.application_config.customized_traffic.ip_version[customized_traffic_config_index]);
              g_otg->aggregation_level[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.aggregation_level[customized_traffic_config_index];
              g_otg->flow_start[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]= flow_start_time(source_id_index,destination_id_index,
                  oai_emulation.info.n_frames,oai_emulation.application_config.customized_traffic.flow_start[customized_traffic_config_index],
                  oai_emulation.application_config.customized_traffic.flow_duration[customized_traffic_config_index]);
              g_otg->flow_duration[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.flow_duration[customized_traffic_config_index];
              g_otg->idt_dist[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] = map_str_to_int(otg_distribution_names,
                  oai_emulation.application_config.customized_traffic.idt_dist[customized_traffic_config_index]);
              //  printf("[OTG] config index %d dist %s\n", customized_traffic_config_index,oai_emulation.application_config.customized_traffic.idt_dist[customized_traffic_config_index]);
              g_otg->idt_min[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.idt_min_ms[customized_traffic_config_index];
              g_otg->idt_max[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.idt_max_ms[customized_traffic_config_index];
              g_otg->idt_std_dev[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.idt_standard_deviation[customized_traffic_config_index];
              g_otg->idt_lambda[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.idt_lambda[customized_traffic_config_index];
              g_otg->idt_scale[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.idt_scale[customized_traffic_config_index];
              g_otg->idt_shape[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.idt_shape[customized_traffic_config_index];
              g_otg->size_dist[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] = map_str_to_int(otg_distribution_names,
                  oai_emulation.application_config.customized_traffic.size_dist[customized_traffic_config_index]);
              g_otg->size_min[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.size_min_byte[customized_traffic_config_index];
              g_otg->size_max[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.size_max_byte[customized_traffic_config_index];
              g_otg->size_std_dev[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.size_standard_deviation[customized_traffic_config_index];
              g_otg->size_lambda[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.size_lambda[customized_traffic_config_index];
              g_otg->size_scale[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.size_scale[customized_traffic_config_index];
              g_otg->size_shape[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.size_shape[customized_traffic_config_index];
              g_otg->dst_port[source_id_index][destination_id_index] = oai_emulation.application_config.customized_traffic.destination_port[customized_traffic_config_index];
              g_otg->pu_size_pkts[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.pu_size_pkts[customized_traffic_config_index];
              g_otg->ed_size_pkts[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.ed_size_pkts[customized_traffic_config_index];
              g_otg->prob_off_pu[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.prob_off_pu[customized_traffic_config_index];
              g_otg->prob_off_ed[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.prob_off_ed[customized_traffic_config_index];
              g_otg->prob_off_pe[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.prob_off_pe[customized_traffic_config_index];
              g_otg->prob_pu_ed[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.prob_pu_ed[customized_traffic_config_index];
              g_otg->prob_pu_pe[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.prob_pu_pe[customized_traffic_config_index];
              g_otg->prob_ed_pe[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.prob_ed_pe[customized_traffic_config_index];
              g_otg->prob_ed_pu[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.prob_ed_pu[customized_traffic_config_index];
              g_otg->holding_time_off_ed[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.holding_time_off_ed[customized_traffic_config_index];
              g_otg->holding_time_off_pu[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.holding_time_off_pu[customized_traffic_config_index];
              g_otg->holding_time_off_pe[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.holding_time_off_pe[customized_traffic_config_index];
              g_otg->holding_time_pe_off[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.holding_time_pe_off[customized_traffic_config_index];

              LOG_I(OTG,"customized:: OCG_config_OTG: (1) FORMAT (%d:%d) source = %d, dest = %d, Application = %d, state %d, background %d IDT DIST %d start/duration %d/%d\n", sid_start, sid_end, source_id_index,
                    destination_id_index,
                    g_otg->application_type[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                    state,
                    g_otg->background[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                    g_otg->idt_dist[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state],
                    g_otg->flow_start[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                    g_otg->flow_duration[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]);

              g_otg->application_idx[source_id_index][destination_id_index]+=1;
            }
          }
        }

      } else if (check_format2 != NULL) { // format 1,2,3,5
        per_source_id = strtok(tmp_source_id, comma);
        destination_id_index = atoi(oai_emulation.application_config.customized_traffic.destination_id[customized_traffic_config_index]);

        while (per_source_id != NULL) {
          source_id_index = atoi(per_source_id);

          if (g_otg->application_type[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] == 0) { // no app is defined for this pair src.dst

            g_otg->background[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] =map_str_to_int(switch_names,
                oai_emulation.application_config.customized_traffic.background[customized_traffic_config_index]);
            g_otg->m2m[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] =map_str_to_int(switch_names,
                oai_emulation.application_config.customized_traffic.m2m[customized_traffic_config_index]);
            g_otg->trans_proto[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] = map_str_to_int(otg_transport_protocol_names,
                oai_emulation.application_config.customized_traffic.transport_protocol[customized_traffic_config_index]);
            g_otg->ip_v[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] = map_str_to_int(otg_ip_version_names,
                oai_emulation.application_config.customized_traffic.ip_version[customized_traffic_config_index]);
            g_otg->aggregation_level[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.aggregation_level[customized_traffic_config_index];
            g_otg->flow_start[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=flow_start_time(source_id_index,destination_id_index,
                oai_emulation.info.n_frames,oai_emulation.application_config.customized_traffic.flow_start[customized_traffic_config_index],
                oai_emulation.application_config.customized_traffic.flow_duration[customized_traffic_config_index]);
            g_otg->flow_duration[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.flow_duration[customized_traffic_config_index];
            g_otg->idt_dist[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] = map_str_to_int(otg_distribution_names,
                oai_emulation.application_config.customized_traffic.idt_dist[customized_traffic_config_index]);
            g_otg->idt_min[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
              oai_emulation.application_config.customized_traffic.idt_min_ms[customized_traffic_config_index];
            g_otg->idt_max[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
              oai_emulation.application_config.customized_traffic.idt_max_ms[customized_traffic_config_index];
            g_otg->idt_std_dev[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
              oai_emulation.application_config.customized_traffic.idt_standard_deviation[customized_traffic_config_index];
            g_otg->idt_lambda[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
              oai_emulation.application_config.customized_traffic.idt_lambda[customized_traffic_config_index];
            g_otg->idt_scale[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
              oai_emulation.application_config.customized_traffic.idt_scale[customized_traffic_config_index];
            g_otg->idt_shape[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
              oai_emulation.application_config.customized_traffic.idt_shape[customized_traffic_config_index];
            g_otg->size_dist[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] = map_str_to_int(otg_distribution_names,
                oai_emulation.application_config.customized_traffic.size_dist[customized_traffic_config_index]);
            g_otg->size_min[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
              oai_emulation.application_config.customized_traffic.size_min_byte[customized_traffic_config_index];
            g_otg->size_max[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
              oai_emulation.application_config.customized_traffic.size_max_byte[customized_traffic_config_index];
            g_otg->size_std_dev[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
              oai_emulation.application_config.customized_traffic.size_standard_deviation[customized_traffic_config_index];
            g_otg->size_lambda[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
              oai_emulation.application_config.customized_traffic.size_lambda[customized_traffic_config_index];
            g_otg->size_scale[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
              oai_emulation.application_config.customized_traffic.size_scale[customized_traffic_config_index];
            g_otg->size_shape[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
              oai_emulation.application_config.customized_traffic.size_shape[customized_traffic_config_index];
            g_otg->dst_port[source_id_index][destination_id_index] = oai_emulation.application_config.customized_traffic.destination_port[customized_traffic_config_index];
            g_otg->pu_size_pkts[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.pu_size_pkts[customized_traffic_config_index];
            g_otg->ed_size_pkts[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.ed_size_pkts[customized_traffic_config_index];
            g_otg->prob_off_pu[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.prob_off_pu[customized_traffic_config_index];
            g_otg->prob_off_ed[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.prob_off_ed[customized_traffic_config_index];
            g_otg->prob_off_pe[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.prob_off_pe[customized_traffic_config_index];
            g_otg->prob_pu_ed[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.prob_pu_ed[customized_traffic_config_index];
            g_otg->prob_pu_pe[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.prob_pu_pe[customized_traffic_config_index];
            g_otg->prob_ed_pe[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.prob_ed_pe[customized_traffic_config_index];
            g_otg->prob_ed_pu[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.prob_ed_pu[customized_traffic_config_index];
            g_otg->holding_time_off_ed[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.holding_time_off_ed[customized_traffic_config_index];
            g_otg->holding_time_off_pu[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.holding_time_off_pu[customized_traffic_config_index];
            g_otg->holding_time_off_pe[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.holding_time_off_pe[customized_traffic_config_index];
            g_otg->holding_time_pe_off[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
              oai_emulation.application_config.customized_traffic.holding_time_pe_off[customized_traffic_config_index];


            LOG_I(OTG,"customized:: OCG_config_OTG: (2) FORMAT (%s:%s) source = %d, dest = %d, dist type for size = %d [TODO: check code, printed value may not be correct, checks the indexes of the arry access] start/duration %d/%d\n", source_id_start, source_id_end, source_id_index,
                  destination_id_index, g_otg->size_dist[source_id_index][destination_id_index][0][0],
                  g_otg->flow_start[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                  g_otg->flow_duration[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]);
            per_source_id = strtok(NULL, comma);
            g_otg->application_idx[source_id_index][destination_id_index]+=1;
          }
        }
      } else { // single node configuration
        source_id_index = atoi(oai_emulation.application_config.customized_traffic.source_id[customized_traffic_config_index]);
        //destination_id_index = atoi(oai_emulation.application_config.customized_traffic.destination_id[customized_traffic_config_index]);

        sid_start = atoi(oai_emulation.application_config.customized_traffic.source_id[customized_traffic_config_index]);
        sid_end = atoi(oai_emulation.application_config.customized_traffic.source_id[customized_traffic_config_index]);

        if (check_format1_dst != NULL) { // format 1:10
          destination_id_start = strtok(tmp_destination_id, colon);
          destination_id_end = strtok(NULL, colon);

          did_start = atoi(destination_id_start);
          did_end = atoi(destination_id_end);
        } else {
          did_start = atoi(oai_emulation.application_config.customized_traffic.destination_id[customized_traffic_config_index]);
          did_end = atoi(oai_emulation.application_config.customized_traffic.destination_id[customized_traffic_config_index]);
        }

        for (destination_id_index = did_start; destination_id_index <= did_end; destination_id_index++) {
          for (source_id_index = sid_start; source_id_index <= sid_end; source_id_index++) {

            if (g_otg->application_type[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] == 0) { // no app is defined for this pair src.dst

              g_otg->background[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] =map_str_to_int(switch_names,
                  oai_emulation.application_config.customized_traffic.background[customized_traffic_config_index]);
              g_otg->m2m[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] =map_str_to_int(switch_names,
                  oai_emulation.application_config.customized_traffic.m2m[customized_traffic_config_index]);

              /* LOG_I(OTG, "[src %d][dst %d][app %d]test_m2m %d \n",source_id_index, destination_id_index , g_otg->application_idx[source_id_index][destination_id_index] , g_otg->m2m[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]  );
               */

              g_otg->trans_proto[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] = map_str_to_int(otg_transport_protocol_names,
                  oai_emulation.application_config.customized_traffic.transport_protocol[customized_traffic_config_index]);
              g_otg->ip_v[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]] = map_str_to_int(otg_ip_version_names,
                  oai_emulation.application_config.customized_traffic.ip_version[customized_traffic_config_index]);
              g_otg->aggregation_level[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.aggregation_level[customized_traffic_config_index];

              g_otg->flow_start[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=flow_start_time(source_id_index,destination_id_index,
                  oai_emulation.info.n_frames,oai_emulation.application_config.customized_traffic.flow_start[customized_traffic_config_index],
                  oai_emulation.application_config.customized_traffic.flow_duration[customized_traffic_config_index]);
              g_otg->flow_duration[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.flow_duration[customized_traffic_config_index];

              g_otg->idt_dist[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] = map_str_to_int(otg_distribution_names,
                  oai_emulation.application_config.customized_traffic.idt_dist[customized_traffic_config_index]);
              g_otg->idt_min[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.idt_min_ms[customized_traffic_config_index];
              g_otg->idt_max[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.idt_max_ms[customized_traffic_config_index];
              g_otg->idt_std_dev[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.idt_standard_deviation[customized_traffic_config_index];
              g_otg->idt_lambda[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.idt_lambda[customized_traffic_config_index];
              g_otg->idt_scale[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.idt_scale[customized_traffic_config_index];
              g_otg->idt_shape[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.idt_shape[customized_traffic_config_index];
              g_otg->size_dist[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] = map_str_to_int(otg_distribution_names,
                  oai_emulation.application_config.customized_traffic.size_dist[customized_traffic_config_index]);
              g_otg->size_min[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.size_min_byte[customized_traffic_config_index];
              g_otg->size_max[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.size_max_byte[customized_traffic_config_index];
              g_otg->size_std_dev[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.size_standard_deviation[customized_traffic_config_index];
              g_otg->size_lambda[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.size_lambda[customized_traffic_config_index];
              g_otg->size_scale[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.size_scale[customized_traffic_config_index];
              g_otg->size_shape[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state] =
                oai_emulation.application_config.customized_traffic.size_shape[customized_traffic_config_index];
              g_otg->dst_port[source_id_index][destination_id_index] = oai_emulation.application_config.customized_traffic.destination_port[customized_traffic_config_index];
              g_otg->pu_size_pkts[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.pu_size_pkts[customized_traffic_config_index];
              g_otg->ed_size_pkts[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.ed_size_pkts[customized_traffic_config_index];
              g_otg->prob_off_pu[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.prob_off_pu[customized_traffic_config_index];
              g_otg->prob_off_ed[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.prob_off_ed[customized_traffic_config_index];
              g_otg->prob_off_pe[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.prob_off_pe[customized_traffic_config_index];
              g_otg->prob_pu_ed[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.prob_pu_ed[customized_traffic_config_index];
              g_otg->prob_pu_pe[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.prob_pu_pe[customized_traffic_config_index];
              g_otg->prob_ed_pe[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.prob_ed_pe[customized_traffic_config_index];
              g_otg->prob_ed_pu[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.prob_ed_pu[customized_traffic_config_index];
              g_otg->holding_time_off_ed[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.holding_time_off_ed[customized_traffic_config_index];
              g_otg->holding_time_off_pu[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.holding_time_off_pu[customized_traffic_config_index];
              g_otg->holding_time_off_pe[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.holding_time_off_pe[customized_traffic_config_index];
              g_otg->holding_time_pe_off[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]=
                oai_emulation.application_config.customized_traffic.holding_time_pe_off[customized_traffic_config_index];

              LOG_I(OTG,"OCG_config_OTG (single node config): source = %d, dest = %d\n", source_id_index, destination_id_index);
              LOG_I(OTG,"customized:: OCG_config_OTG:(3) FORMAT (%d:%d) source = %d, dest = %d, Application = %d, IDT dist %d flow start %d duration %d \n",
                    did_start, did_end, source_id_index, destination_id_index,
                    g_otg->application_type[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                    g_otg->idt_dist[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]][state],
                    g_otg->flow_start[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]],
                    g_otg->flow_duration[source_id_index][destination_id_index][g_otg->application_idx[source_id_index][destination_id_index]]);
              g_otg->application_idx[source_id_index][destination_id_index]+=1;

            }
          }
        }
      }
    }
  }


  if ((oai_emulation.info.max_predefined_traffic_config_index == 0)  &&
      (oai_emulation.info.max_customized_traffic_config_index == 0) &&
      (oai_emulation.info.otg_enabled==1)) { // OCG not used to configure OTG, but -T option is used, so config here
    LOG_I(OTG,"configure OTG through options %s\n", oai_emulation.info.otg_traffic);

    for (i=0; i<g_otg->num_nodes; i++) {
      for (j=0; j<g_otg->num_nodes; j++) {
        g_otg->application_idx[i][j]=1;

        for (k=0; k<g_otg->application_idx[i][j]; k++) {

          g_otg->application_type[i][j][k] = map_str_to_int(otg_app_type_names, oai_emulation.info.otg_traffic);
          g_otg->background[i][j][k]=oai_emulation.info.otg_bg_traffic_enabled;
          g_otg->packet_gen_type=SUBSTRACT_STRING;
          g_otg->flow_start[i][j][k]=flow_start_time(i,j,oai_emulation.info.n_frames,
                                     oai_emulation.application_config.customized_traffic.flow_start[i+j],
                                     oai_emulation.application_config.customized_traffic.flow_duration[i+j]);
          g_otg->flow_duration[i][j][k]=oai_emulation.application_config.customized_traffic.flow_duration[i+j];
          LOG_I(OTG,"predefined traffic:: source = %d, dest = %d, Application = %d, start %d duration %d \n",
                i,j, g_otg->application_type[i][j][k],g_otg->flow_start[i][j][k],g_otg->flow_duration[i][j][k]);
        }
      }
    }

    init_predef_traffic(oai_emulation.info.nb_ue_local, oai_emulation.info.nb_enb_local);

    for (i=0; i<NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX; i++) { //maxServiceCount
      for (j=0; j<NUMBER_OF_eNB_MAX + NUMBER_OF_SERVICE_MAX; j++) { // maxSessionPerPMCH
        for (k=0; k<MAX_NUM_APPLICATION; k++) {
          g_otg_multicast->application_type[i][j][k] = map_str_to_int( otg_multicast_app_type_names, oai_emulation.info.otg_traffic);
        }
      }
    }

    init_predef_multicast_traffic();
    LOG_I(OTG,"initilizae multicast traffic %s\n",oai_emulation.info.otg_traffic);

  }

  return 1;
}

int flow_start_time(int sid, int did, uint32_t n_frames, uint32_t start, uint32_t duration)
{

  int offset0=uniform_dist(1,10);
  int offset1=uniform_dist(10,100);
  int offset2=uniform_dist(100,1000);
  int interval = 300;
  int flow_start_margin_1=0;
  int flow_start_margin_2=0;

  // return user-defined flow start time
  if (start > 0 )
    return start;

  //determine the flow start time
  start = (sid+did+1)*interval;
  flow_start_margin_1 = 10*n_frames - (start + offset1 + duration);
  flow_start_margin_2 = 10*n_frames - (start + offset2 + duration);

  //LOG_I(OTG,"nframes %d start %d margin_1 %d margin_2 %d\n",n_frames, start,flow_start_margin_1, flow_start_margin_2);
  if (flow_start_margin_2 > 0 )
    return uniform_dist(start, start+offset2);
  else if (flow_start_margin_1 > 0 )
    return uniform_dist(start, start+offset1);
  else
    return uniform_dist(start, start+offset0);

}


int ocg_config_emu(void)
{

  if (oai_emulation.emulation_config.emulation_time_ms != 0) {
    oai_emulation.info.n_frames  =  (int) oai_emulation.emulation_config.emulation_time_ms / 10; // configure the number of frame
    oai_emulation.info.n_frames_flag = 1;
  }

  if (oai_emulation.info.n_frames_flag) {
    LOG_I(OCG, "number of frames in emulation is set to %d\n", oai_emulation.info.n_frames);
  } else {
    LOG_I(OCG, "number of frames in emulation is set to infinity\n");
  }

  oai_emulation.info.seed = (oai_emulation.emulation_config.seed.value == 0) ? oai_emulation.info.seed : oai_emulation.emulation_config.seed.value;
  LOG_I (OCG,"The seed value is set to %d \n", oai_emulation.info.seed );

  if (oai_emulation.info.cli_enabled == 1) {
    if (cli_server_init(cli_server_recv) < 0) {
      LOG_E(EMU,"cli server init failed \n");
      exit(-1);
    }

    LOG_I(EMU, "OAI CLI is enabled\n");
    LOG_I(EMU,"eNB start state is %d, UE start state %d\n",
          oai_emulation.info.cli_start_enb[0],
          oai_emulation.info.cli_start_ue[0]);
  }

  if (oai_emulation.info.opp_enabled)
    opp_enabled = 1;

  //bin/LOG_I(OCG, "OPT output file directory = %s\n", oai_emulation.info.output_path);
  oai_emulation.info.opt_enabled = ( oai_emulation.emulation_config.packet_trace.enabled == 0) ? oai_emulation.info.opt_enabled :  oai_emulation.emulation_config.packet_trace.enabled;

  if (oai_emulation.info.opt_enabled == 1) {
    radio_type_t radio_type;
    opt_enabled = 1;

    if (oai_emulation.info.frame_type[0] == FDD) {
      radio_type = RADIO_TYPE_FDD;
    } else {
      radio_type = RADIO_TYPE_TDD;
    }

    opt_type = oai_emulation.info.opt_mode;

    if (init_opt(NULL, NULL, NULL, radio_type) == -1) {
      LOG_E(OPT,"failed to run OPT \n");
    }
  }

  return 1;
}

void  ocg_config_proto(void)
{

  LOG_I(MAC,"num groups is set to %d\n",oai_emulation.protocol_config.eNB_mac_config.num_groups);

}

