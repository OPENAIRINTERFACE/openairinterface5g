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

/*! \file OCG_parse_XML.c
* \brief Parse the content of the XML configuration file
* \author Lusheng Wang  & Navid Nikaein & Andre Gomes(One Source)
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

/*--- INCLUDES ---------------------------------------------------------------*/
#include <libxml/tree.h>
#include <libxml/parser.h>
//#include <stdio.h>
#include <string.h>
//#include <stdlib.h>
#include "OCG_vars.h"
#include "OCG_parse_XML.h"
#include "UTIL/LOG/log.h"
#include "UTIL/OTG/otg_defs.h"
#include "UTIL/OPT/opt.h"
/*----------------------------------------------------------------------------*/

static int oai_emulation_;  /*!< \brief indicating that the parsing position is now within OAI_Emulation_*/

static int environment_system_config_;    /*!< \brief indicating that the parsing position is now within Envi_Config_*/
static int fading_;
static int large_scale_;
static int free_space_model_parameters_;
static int pathloss_exponent_;
static int pathloss_0_dB_;
static int small_scale_;
static int ricean_8tap_;
static int rice_factor_dB_;
static int shadowing_;
static int decorrelation_distance_m_;
static int variance_dB_;
static int inter_site_correlation_;
static int wall_penetration_loss_dB_;
static int system_bandwidth_MB_;
static int system_frequency_GHz_;
static int number_of_rbs_dl_;
static int transmission_mode_;
static int frame_config_;
static int frame_type_;
static int tdd_config_;
static int antenna_;
static int eNB_antenna_;
static int UE_antenna_;
static int number_of_sectors_;
static int beam_width_dB_;
static int alpha_rad_1_;
static int alpha_rad_2_;
static int alpha_rad_3_;
static int antenna_gain_dBi_;
static int tx_power_dBm_;
static int rx_noise_level_dB_;
static int antenna_orientation_degree_1_;
static int antenna_orientation_degree_2_;
static int antenna_orientation_degree_3_;

static int topology_config_;
static int area_;       /*!< \brief indicating that the parsing position is now within Area_*/
static int x_m_;
static int y_m_;
static int network_type_;
static int cell_type_;
static int relay_;
static int number_of_relays_;
static int mobility_;
static int UE_mobility_;
static int UE_mobility_type_;
static int grid_walk_;
static int grid_map_;
static int horizontal_grid_;
static int vertical_grid_;
static int grid_trip_type_;
static int UE_initial_distribution_;
static int random_UE_distribution_;
static int number_of_nodes_;
static int concentrated_UE_distribution_;
static int grid_UE_distribution_;
static int random_grid_;
static int border_grid_;
static int UE_moving_dynamics_;
static int min_speed_mps_;
static int max_speed_mps_;
static int min_sleep_ms_;
static int max_sleep_ms_;
static int min_journey_time_ms_;
static int max_journey_time_ms_;
static int eNB_mobility_;
static int eNB_mobility_type_;
static int eNB_initial_distribution_;
static int eNB_initial_coordinates_;
static double eNB_pos_x_;
static double eNB_pos_y_;
static int random_eNB_distribution_;
static int number_of_cells_;
static int hexagonal_eNB_distribution_;
static int inter_eNB_distance_km_;
static int grid_eNB_distribution_;
static int number_of_grid_x_;
static int number_of_grid_y_;
static int trace_mobility_file_;
static int sumo_config_;
static int sumo_cmd_;
static int sumo_config_file_;
static int sumo_start_;
static int sumo_end_;
static int sumo_step_;
static int sumo_hip_;
static int sumo_hport_;
static int omv_;

static int application_config_;   /*!< \brief indicating that the parsing position is now within App_Config_*/
static int predefined_traffic_;
static int background_traffic_;
static int customized_traffic_;
static int application_type_;     /*!< \brief indicating that the parsing position is now within App_Type_*/
static int source_id_;
static int destination_id_;
static int traffic_;    /*!< \brief indicating that the parsing position is now within Traffic_*/
static int transport_protocol_; /*!< \brief indicating that the parsing position is now within Transport_Protocol_*/
static int ip_version_;
static unsigned int aggregation_level_;
static int flow_start_;
static int flow_duration_;
static int idt_dist_;
static int idt_min_ms_;
static int idt_max_ms_;
static double idt_standard_deviation_;
static double idt_lambda_;
static double idt_scale_;
static double idt_shape_;
static int size_dist_;
static int size_min_byte_;
static int size_max_byte_;
static double size_standard_deviation_;
static double size_lambda_;
static double size_scale_;
static double size_shape_;
static int stream_;
static int destination_port_;
static int packet_gen_type_;

static int emulation_config_;   /*!< \brief indicating that the parsing position is now within Emu_Config_*/
static int emulation_time_ms_;
static int curve_;
static int profiling_;
static int trace_file_;
static int background_stats_;
static int performance_metrics_;    /*!< \brief indicating that the parsing position is now within Performance_*/
static int throughput_;
static int latency_;
static int loss_rate_;
static int owd_radio_access_;
static int layer_;        /*!< \brief indicating that the parsing position is now within Layer_*/
static int phy_;
static int mac_;
static int rlc_;
static int pdcp_;
static int rrc_;
static int emu_;
static int omg_;
static int otg_;

/* MAC*/
static int protocol_;
static int mac_;
static int num_groups_;

static int log_;
static int level_;
static int verbosity_;
static int interval_;
static int packet_trace_;
static int seed_value_;

static int cli_enabled_;
static int cli_start_enb_;
static int cli_start_ue_;

static unsigned int m2m_traffic_;
static double prob_off_pu_;
double prob_off_ed_;
static double prob_off_pe_;
static double prob_pu_ed_;
static double prob_pu_pe_;
static double prob_ed_pe_;
static double prob_ed_pu_;
static unsigned int holding_time_off_ed_;
static unsigned int holding_time_off_pu_;
static unsigned int holding_time_off_pe_;
static unsigned int holding_time_pe_off_;
static unsigned int pu_size_pkts_;
static unsigned int ed_size_pkts_;

static int profile_;


void start_document(void *user_data)
{
  //printf("Start parsing ............ \n");
}

void end_document(void *user_data)
{
  //printf("End parsing .\n");
}

void start_element(void *user_data, const xmlChar *name, const xmlChar **attrs)   // called once at the beginning of each element
{
  if (!xmlStrcmp(name,(unsigned char*) "OAI_EMULATION")) {
    oai_emulation_ = 1;

  } else if (!xmlStrcmp(name,(unsigned char*) "ENVIRONMENT_SYSTEM_CONFIG")) {
    environment_system_config_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "FADING")) {
    fading_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "LARGE_SCALE")) {
    large_scale_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "FREE_SPACE_MODEL_PARAMETERS")) {
    free_space_model_parameters_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "PATHLOSS_EXPONENT")) {
    pathloss_exponent_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "PATHLOSS_0_dB")) {
    pathloss_0_dB_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "SMALL_SCALE")) {
    small_scale_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "RICEAN_8TAP")) {
    ricean_8tap_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "RICE_FACTOR_dB")) {
    rice_factor_dB_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "SHADOWING")) {
    shadowing_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "DECORRELATION_DISTANCE_m")) {
    decorrelation_distance_m_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "VARIANCE_dB")) {
    variance_dB_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "INTER_SITE_CORRELATION")) {
    inter_site_correlation_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "WALL_PENETRATION_LOSS_dB")) {
    wall_penetration_loss_dB_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "SYSTEM_BANDWIDTH_MB")) {
    system_bandwidth_MB_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "SYSTEM_FREQUENCY_GHz")) {
    system_frequency_GHz_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUMBER_OF_RBS_DL")) {
    number_of_rbs_dl_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "TRANSMISSION_MODE")) {
    transmission_mode_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "FRAME_CONFIG")) {
    frame_config_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "FRAME_TYPE")) {
    frame_type_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "TDD_CONFIG")) {
    tdd_config_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "ANTENNA")) {
    antenna_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "eNB_ANTENNA")) {
    eNB_antenna_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "UE_ANTENNA")) {
    UE_antenna_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUMBER_OF_SECTORS")) {
    number_of_sectors_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "BEAM_WIDTH_dB")) {
    beam_width_dB_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "ALPHA_RAD_1")) {
    alpha_rad_1_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "ALPHA_RAD_2")) {
    alpha_rad_2_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "ALPHA_RAD_3")) {
    alpha_rad_3_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "ANTENNA_GAIN_dBi")) {
    antenna_gain_dBi_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "TX_POWER_dBm")) {
    tx_power_dBm_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "RX_NOISE_LEVEL_dB")) {
    rx_noise_level_dB_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "ANTENNA_ORIENTATION_degree1")) {
    antenna_orientation_degree_1_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "ANTENNA_ORIENTATION_degree2")) {
    antenna_orientation_degree_2_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "ANTENNA_ORIENTATION_degree3")) {
    antenna_orientation_degree_3_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "TOPOLOGY_CONFIG")) {
    topology_config_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "AREA")) {
    area_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "X_m")) {
    x_m_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "Y_m")) {
    y_m_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "NETWORK_TYPE")) {
    network_type_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "CELL_TYPE")) {
    cell_type_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "RELAY")) {
    relay_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUMBER_OF_RELAYS")) {
    number_of_relays_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "MOBILITY")) {
    mobility_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "UE_MOBILITY")) {
    UE_mobility_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "UE_MOBILITY_TYPE")) {
    UE_mobility_type_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "GRID_WALK")) {
    grid_walk_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "GRID_MAP")) {
    grid_map_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "HORIZONTAL_GRID")) {
    horizontal_grid_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "VERTICAL_GRID")) {
    vertical_grid_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "GRID_TRIP_TYPE")) {
    grid_trip_type_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "UE_INITIAL_DISTRIBUTION")) {
    UE_initial_distribution_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "RANDOM_UE_DISTRIBUTION")) {
    random_UE_distribution_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUMBER_OF_NODES")) {
    number_of_nodes_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "CONCENTRATED_UE_DISTRIBUTION")) {
    concentrated_UE_distribution_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "GRID_UE_DISTRIBUTION")) {
    grid_UE_distribution_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "RANDOM_GRID")) {
    random_grid_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "BORDER_GRID")) {
    border_grid_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "UE_MOVING_DYNAMICS")) {
    UE_moving_dynamics_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "MIN_SPEED_mps")) {
    min_speed_mps_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "MAX_SPEED_mps")) {
    max_speed_mps_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "MIN_SLEEP_ms")) {
    min_sleep_ms_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "MAX_SLEEP_ms")) {
    max_sleep_ms_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "MIN_JOURNEY_TIME_ms")) {
    min_journey_time_ms_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "MAX_JOURNEY_TIME_ms")) {
    max_journey_time_ms_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "eNB_MOBILITY")) {
    eNB_mobility_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "eNB_MOBILITY_TYPE")) {
    eNB_mobility_type_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "eNB_INITIAL_DISTRIBUTION")) {
    eNB_initial_distribution_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "eNB_INITIAL_COORDINATES")) {
    eNB_initial_coordinates_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "POS_X")) {
    eNB_pos_x_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "POS_Y")) {
    eNB_pos_y_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "RANDOM_eNB_DISTRIBUTION")) {
    random_eNB_distribution_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUMBER_OF_CELLS")) {
    number_of_cells_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "HEXAGONAL_eNB_DISTRIBUTION")) {
    hexagonal_eNB_distribution_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "INTER_eNB_DISTANCE_km")) {
    inter_eNB_distance_km_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "GRID_eNB_DISTRIBUTION")) {
    grid_eNB_distribution_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUMBER_OF_GRID_X")) {
    number_of_grid_x_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUMBER_OF_GRID_Y")) {
    number_of_grid_y_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "TRACE_MOBILITY_FILE")) {
    trace_mobility_file_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_CONFIG")) {
    sumo_config_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_CMD")) {
    sumo_cmd_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_CONFIG_FILE")) {
    sumo_config_file_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_START")) {
    sumo_start_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_END")) {
    sumo_end_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_STEP")) {
    sumo_step_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_HOST_IP")) {
    sumo_hip_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_HOST_PORT")) {
    sumo_hport_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "OMV")) {
    omv_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "APPLICATION_CONFIG")) {
    application_config_ = 1;
    oai_emulation.info.max_predefined_traffic_config_index = 0;
    oai_emulation.info.max_customized_traffic_config_index = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "PREDEFINED_TRAFFIC")) {
    predefined_traffic_ = 1;
    oai_emulation.info.otg_enabled =1;
    oai_emulation.info.max_predefined_traffic_config_index ++;
  } else if (!xmlStrcmp(name, (unsigned char*) "CUSTOMIZED_TRAFFIC")) {
    customized_traffic_ = 1;
    oai_emulation.info.otg_enabled =1;
    oai_emulation.info.max_customized_traffic_config_index ++;
  } else if (!xmlStrcmp(name, (unsigned char*) "CUSTOMIZED_TRAFFIC")) {
    customized_traffic_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "M2M")) {
    m2m_traffic_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "PROB_OFF_PU")) {
    prob_off_pu_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "PROB_OFF_ED")) {
    prob_off_ed_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "PROB_OFF_PE")) {
    prob_off_pe_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "PROB_PU_ED")) {
    prob_pu_ed_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "PROB_PU_PE")) {
    prob_pu_pe_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "PROB_ED_PE")) {
    prob_ed_pe_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "PROB_ED_PU")) {
    prob_ed_pu_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "HOLDING_TIME_OFF_ED")) {
    holding_time_off_ed_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "HOLDING_TIME_OFF_PU")) {
    holding_time_off_pu_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "HOLDING_TIME_OFF_PE")) {
    holding_time_off_pe_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "HOLDING_TIME_PE_OFF")) {
    holding_time_pe_off_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "PU_SIZE_PKTS")) {
    pu_size_pkts_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "ED_SIZE_PKTS")) {
    ed_size_pkts_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "SOURCE_ID")) {
    source_id_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "DESTINATION_ID")) {
    destination_id_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "APPLICATION_TYPE")) {
    application_type_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "PACKET_GEN_TYPE")) {
    packet_gen_type_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "BACKGROUND_TRAFFIC")) {
    background_traffic_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "TRAFFIC")) {
    traffic_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "TRANSPORT_PROTOCOL")) {
    transport_protocol_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "IP_VERSION")) {
    ip_version_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "AGGREGATION_LEVEL")) {
    aggregation_level_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "FLOW_START_ms")) {
    flow_start_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "FLOW_DURATION_ms")) {
    flow_duration_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "IDT_DIST")) {
    idt_dist_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "IDT_MIN_ms")) {
    idt_min_ms_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "IDT_MAX_ms")) {
    idt_max_ms_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "IDT_STANDARD_DEVIATION")) {
    idt_standard_deviation_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "IDT_LAMBDA")) {
    idt_lambda_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "IDT_SCALE")) {
    idt_scale_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "IDT_SHAPE")) {
    idt_shape_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "SIZE_DIST")) {
    size_dist_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "SIZE_MIN_byte")) {
    size_min_byte_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "SIZE_MAX_byte")) {
    size_max_byte_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "SIZE_STANDARD_DEVIATION")) {
    size_standard_deviation_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "SIZE_LAMBDA")) {
    size_lambda_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "SIZE_SCALE")) {
    size_scale_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "SIZE_SHAPE")) {
    size_shape_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "STREAM")) {
    stream_ = 1;
  } else if (!xmlStrcmp(name, (unsigned char*) "DESTINATION_PORT")) {
    destination_port_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "EMULATION_CONFIG")) {
    emulation_config_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "EMULATION_TIME_ms")) {
    emulation_time_ms_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "CURVE")) {
    curve_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "PROFILING")) {
    profiling_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "TRACE_FILE")) {
    trace_file_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "BACKGROUND_STATS")) {
    background_stats_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "PERFORMANCE_METRICS")) {
    performance_metrics_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "THROUGHPUT")) {
    throughput_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "LATENCY")) {
    latency_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "LOSS_RATE")) {
    loss_rate_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "OWD_RADIO_ACCESS")) {
    owd_radio_access_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "LAYER")) {
    layer_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "PHY")) {
    phy_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "MAC")) {
    mac_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "RLC")) {
    rlc_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "PDCP")) {
    pdcp_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "RRC")) {
    rrc_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "OMG")) {
    omg_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "OTG")) {
    otg_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "EMU")) {
    emu_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "LOG")) {
    log_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "LEVEL")) {
    level_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "VERBOSITY")) {
    verbosity_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "INTERVAL")) {
    interval_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "PACKET_TRACE")) {
    packet_trace_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "SEED_VALUE")) {
    seed_value_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "CLI")) {
    cli_enabled_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "START_ENB")) {
    cli_start_enb_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "START_UE")) {
    cli_start_ue_ = 1;

  } else if (!xmlStrcmp(name,(unsigned char*) "PROTOCOL")) {
    protocol_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "MAC")) {
    mac_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUM_GROUPS")) {
    num_groups_ = 1;
  } else if (!xmlStrcmp(name,(unsigned char*) "PROFILE")) {
    profile_ = 1;
  } else {
    LOG_W(OCG, "One element could not be parsed : unknown element name '%s'\n", name);
  }
}

void end_element(void *user_data, const xmlChar *name)   // called once at the end of each element
{
  if (!xmlStrcmp(name,(unsigned char*) "OAI_EMULATION")) {
    oai_emulation_ = 0;

  } else if (!xmlStrcmp(name,(unsigned char*) "ENVIRONMENT_SYSTEM_CONFIG")) {
    environment_system_config_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "FADING")) {
    fading_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "LARGE_SCALE")) {
    large_scale_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "FREE_SPACE_MODEL_PARAMETERS")) {
    free_space_model_parameters_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "PATHLOSS_EXPONENT")) {
    pathloss_exponent_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "PATHLOSS_0_dB")) {
    pathloss_0_dB_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "SMALL_SCALE")) {
    small_scale_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "RICEAN_8TAP")) {
    ricean_8tap_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "RICE_FACTOR_dB")) {
    rice_factor_dB_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "SHADOWING")) {
    shadowing_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "DECORRELATION_DISTANCE_m")) {
    decorrelation_distance_m_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "VARIANCE_dB")) {
    variance_dB_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "INTER_SITE_CORRELATION")) {
    inter_site_correlation_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "WALL_PENETRATION_LOSS_dB")) {
    wall_penetration_loss_dB_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "SYSTEM_BANDWIDTH_MB")) {
    system_bandwidth_MB_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "SYSTEM_FREQUENCY_GHz")) {
    system_frequency_GHz_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUMBER_OF_RBS_DL")) {
    number_of_rbs_dl_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "TRANSMISSION_MODE")) {
    transmission_mode_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "FRAME_CONFIG")) {
    frame_config_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "FRAME_TYPE")) {
    frame_type_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "TDD_CONFIG")) {
    tdd_config_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "ANTENNA")) {
    antenna_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "eNB_ANTENNA")) {
    eNB_antenna_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "UE_ANTENNA")) {
    UE_antenna_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUMBER_OF_SECTORS")) {
    number_of_sectors_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "BEAM_WIDTH_dB")) {
    beam_width_dB_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "ALPHA_RAD_1")) {
    alpha_rad_1_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "ALPHA_RAD_2")) {
    alpha_rad_2_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "ALPHA_RAD_3")) {
    alpha_rad_3_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "ANTENNA_GAIN_dBi")) {
    antenna_gain_dBi_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "TX_POWER_dBm")) {
    tx_power_dBm_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "RX_NOISE_LEVEL_dB")) {
    rx_noise_level_dB_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "ANTENNA_ORIENTATION_degree1")) {
    antenna_orientation_degree_1_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "ANTENNA_ORIENTATION_degree2")) {
    antenna_orientation_degree_2_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "ANTENNA_ORIENTATION_degree3")) {
    antenna_orientation_degree_3_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "TOPOLOGY_CONFIG")) {
    topology_config_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "AREA")) {
    area_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "X_m")) {
    x_m_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "Y_m")) {
    y_m_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "NETWORK_TYPE")) {
    network_type_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "CELL_TYPE")) {
    cell_type_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "RELAY")) {
    relay_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUMBER_OF_RELAYS")) {
    number_of_relays_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "MOBILITY")) {
    mobility_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "UE_MOBILITY")) {
    UE_mobility_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "UE_MOBILITY_TYPE")) {
    UE_mobility_type_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "GRID_WALK")) {
    grid_walk_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "GRID_MAP")) {
    grid_map_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "HORIZONTAL_GRID")) {
    horizontal_grid_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "VERTICAL_GRID")) {
    vertical_grid_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "GRID_TRIP_TYPE")) {
    grid_trip_type_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "UE_INITIAL_DISTRIBUTION")) {
    UE_initial_distribution_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "RANDOM_UE_DISTRIBUTION")) {
    random_UE_distribution_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUMBER_OF_NODES")) {
    number_of_nodes_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "CONCENTRATED_UE_DISTRIBUTION")) {
    concentrated_UE_distribution_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "GRID_UE_DISTRIBUTION")) {
    grid_UE_distribution_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "RANDOM_GRID")) {
    random_grid_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "BORDER_GRID")) {
    border_grid_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "UE_MOVING_DYNAMICS")) {
    UE_moving_dynamics_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "MIN_SPEED_mps")) {
    min_speed_mps_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "MAX_SPEED_mps")) {
    max_speed_mps_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "MIN_SLEEP_ms")) {
    min_sleep_ms_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "MAX_SLEEP_ms")) {
    max_sleep_ms_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "MIN_JOURNEY_TIME_ms")) {
    min_journey_time_ms_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "MAX_JOURNEY_TIME_ms")) {
    max_journey_time_ms_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "eNB_MOBILITY")) {
    eNB_mobility_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "eNB_MOBILITY_TYPE")) {
    eNB_mobility_type_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "eNB_INITIAL_DISTRIBUTION")) {
    eNB_initial_distribution_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "eNB_INITIAL_COORDINATES")) {
    eNB_initial_coordinates_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "POS_X")) {
    eNB_pos_x_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "POS_Y")) {
    eNB_pos_y_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "RANDOM_eNB_DISTRIBUTION")) {
    random_eNB_distribution_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUMBER_OF_CELLS")) {
    number_of_cells_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "HEXAGONAL_eNB_DISTRIBUTION")) {
    hexagonal_eNB_distribution_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "INTER_eNB_DISTANCE_km")) {
    inter_eNB_distance_km_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "GRID_eNB_DISTRIBUTION")) {
    grid_eNB_distribution_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUMBER_OF_GRID_X")) {
    number_of_grid_x_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUMBER_OF_GRID_Y")) {
    number_of_grid_y_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "TRACE_MOBILITY_FILE")) {
    trace_mobility_file_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_CONFIG")) {
    sumo_config_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_CMD")) {
    sumo_cmd_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_CONFIG_FILE")) {
    sumo_config_file_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_START")) {
    sumo_start_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_END")) {
    sumo_end_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_STEP")) {
    sumo_step_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_HOST_IP")) {
    sumo_hip_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "SUMO_HOST_PORT")) {
    sumo_hport_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "OMV")) {
    omv_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "APPLICATION_CONFIG")) {
    application_config_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "PREDEFINED_TRAFFIC")) {
    predefined_traffic_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "CUSTOMIZED_TRAFFIC")) {
    customized_traffic_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "M2M")) {
    m2m_traffic_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "PROB_OFF_PU")) {
    prob_off_pu_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "PROB_OFF_ED")) {
    prob_off_ed_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "PROB_OFF_PE")) {
    prob_off_pe_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "PROB_PU_ED")) {
    prob_pu_ed_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "PROB_PU_PE")) {
    prob_pu_pe_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "PROB_ED_PE")) {
    prob_ed_pe_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "PROB_ED_PU")) {
    prob_ed_pu_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "HOLDING_TIME_OFF_ED")) {
    holding_time_off_ed_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "HOLDING_TIME_OFF_PU")) {
    holding_time_off_pu_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "HOLDING_TIME_OFF_PE")) {
    holding_time_off_pe_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "HOLDING_TIME_PE_OFF")) {
    holding_time_pe_off_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "PU_SIZE_PKTS")) {
    pu_size_pkts_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "ED_SIZE_PKTS")) {
    ed_size_pkts_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "SOURCE_ID")) {
    source_id_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "DESTINATION_ID")) {
    destination_id_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "APPLICATION_TYPE")) {
    application_type_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "PACKET_GEN_TYPE")) {
    packet_gen_type_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "BACKGROUND_TRAFFIC")) {
    background_traffic_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "TRAFFIC")) {
    traffic_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "TRANSPORT_PROTOCOL")) {
    transport_protocol_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "AGGREGATION_LEVEL")) {
    aggregation_level_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "FLOW_START_ms")) {
    flow_start_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "FLOW_DURATION_ms")) {
    flow_duration_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "IP_VERSION")) {
    ip_version_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "IDT_DIST")) {
    idt_dist_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "IDT_MIN_ms")) {
    idt_min_ms_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "IDT_MAX_ms")) {
    idt_max_ms_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "IDT_STANDARD_DEVIATION")) {
    idt_standard_deviation_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "IDT_LAMBDA")) {
    idt_lambda_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "IDT_SCALE")) {
    idt_scale_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "IDT_SHAPE")) {
    idt_shape_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "SIZE_DIST")) {
    size_dist_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "SIZE_MIN_byte")) {
    size_min_byte_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "SIZE_MAX_byte")) {
    size_max_byte_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "SIZE_STANDARD_DEVIATION")) {
    size_standard_deviation_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "SIZE_LAMBDA")) {
    size_lambda_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "SIZE_SCALE")) {
    size_scale_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "SIZE_SHAPE")) {
    size_shape_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "STREAM")) {
    stream_ = 0;
  } else if (!xmlStrcmp(name, (unsigned char*) "DESTINATION_PORT")) {
    destination_port_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "EMULATION_CONFIG")) {
    emulation_config_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "EMULATION_TIME_ms")) {
    emulation_time_ms_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "CURVE")) {
    curve_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "PROFILING")) {
    profiling_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "TRACE_FILE")) {
    trace_file_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "BACKGROUND_STATS")) {
    background_stats_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "PERFORMANCE_METRICS")) {
    performance_metrics_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "THROUGHPUT")) {
    throughput_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "LATENCY")) {
    latency_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "LOSS_RATE")) {
    loss_rate_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "OWD_RADIO_ACCESS")) {
    owd_radio_access_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "LAYER")) {
    layer_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "PHY")) {
    phy_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "MAC")) {
    mac_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "RLC")) {
    rlc_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "PDCP")) {
    pdcp_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "RRC")) {
    rrc_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "OMG")) {
    omg_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "OTG")) {
    otg_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "EMU")) {
    emu_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "LOG")) {
    log_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "LEVEL")) {
    level_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "VERBOSITY")) {
    verbosity_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "INTERVAL")) {
    interval_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "PACKET_TRACE")) {
    packet_trace_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "SEED_VALUE")) {
    seed_value_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "CLI")) {
    cli_enabled_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "START_ENB")) {
    cli_start_enb_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "START_UE")) {
    cli_start_ue_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "PROTOCOL")) {
    protocol_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "MAC")) {
    mac_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "NUM_GROUPS")) {
    num_groups_ = 0;
  } else if (!xmlStrcmp(name,(unsigned char*) "PROFILE")) {
    profile_ = 0;
  }
}

void characters(void *user_data, const xmlChar *xmlch, int xmllen)   // called once when there is content in each element
{

  int i;
  const char *ch = (const char*) xmlch;
  size_t len = (size_t) xmllen;

  if (oai_emulation_) {
    if (environment_system_config_) {
      if (fading_) {
        if (large_scale_) {
          oai_emulation.environment_system_config.fading.large_scale.selected_option = strndup(ch, len);
        } else if (small_scale_) {
          oai_emulation.environment_system_config.fading.small_scale.selected_option = strndup(ch, len);
        } else if (shadowing_) {
          if (decorrelation_distance_m_) {
            oai_emulation.environment_system_config.fading.shadowing.decorrelation_distance_m = atof(ch);
          } else if (variance_dB_) {
            oai_emulation.environment_system_config.fading.shadowing.variance_dB = atof(ch);
          } else if (inter_site_correlation_) {
            oai_emulation.environment_system_config.fading.shadowing.inter_site_correlation = atof(ch);
          }
        } else if (free_space_model_parameters_) {
          if (pathloss_exponent_) {
            oai_emulation.environment_system_config.fading.free_space_model_parameters.pathloss_exponent = atof(ch);
          } else if (pathloss_0_dB_) {
            oai_emulation.environment_system_config.fading.free_space_model_parameters.pathloss_0_dB = atof(ch);
          }
        } else if (ricean_8tap_) {
          oai_emulation.environment_system_config.fading.ricean_8tap.rice_factor_dB = atof(ch);
        }
      } else if (wall_penetration_loss_dB_) {
        oai_emulation.environment_system_config.wall_penetration_loss_dB = atof(ch);
      } else if (system_bandwidth_MB_) {
        oai_emulation.environment_system_config.system_bandwidth_MB = atof(ch);
      } else if (system_frequency_GHz_) {
        oai_emulation.environment_system_config.system_frequency_GHz = atof(ch);
      } else if (number_of_rbs_dl_) {
        oai_emulation.info.N_RB_DL[0] = atoi(ch);
      } else if (transmission_mode_) {
        oai_emulation.info.transmission_mode[0] = atoi(ch);
      } else if (frame_config_) {
        if (frame_type_) {
          oai_emulation.info.frame_type[0] = atoi(ch);

          if (oai_emulation.info.frame_type[0]) {
            oai_emulation.info.frame_type_name[0] = "TDD";
          } else {
            oai_emulation.info.frame_type_name[0] = "FDD";
          }
        } else if (tdd_config_) {
          oai_emulation.info.tdd_config[0] = atoi(ch);
        }
      } else if (antenna_) {
        if (eNB_antenna_) {
          if (number_of_sectors_) {
            oai_emulation.environment_system_config.antenna.eNB_antenna.number_of_sectors = atoi(ch);
          } else if (beam_width_dB_) {
            oai_emulation.environment_system_config.antenna.eNB_antenna.beam_width_dB = atof(ch);
          } else if (alpha_rad_1_) {
            oai_emulation.environment_system_config.antenna.eNB_antenna.alpha_rad[0] = atof(ch);
          } else if (alpha_rad_2_) {
            oai_emulation.environment_system_config.antenna.eNB_antenna.alpha_rad[1] = atof(ch);
          } else if (alpha_rad_3_) {
            oai_emulation.environment_system_config.antenna.eNB_antenna.alpha_rad[2] = atof(ch);
          } else if (antenna_gain_dBi_) {
            oai_emulation.environment_system_config.antenna.eNB_antenna.antenna_gain_dBi = atof(ch);
          } else if (tx_power_dBm_) {
            oai_emulation.environment_system_config.antenna.eNB_antenna.tx_power_dBm = atof(ch);
          } else if (rx_noise_level_dB_) {
            oai_emulation.environment_system_config.antenna.eNB_antenna.rx_noise_level_dB = atof(ch);
          } else if (antenna_orientation_degree_1_) {
            oai_emulation.environment_system_config.antenna.eNB_antenna.antenna_orientation_degree[0] = atof(ch);
          } else if (antenna_orientation_degree_2_) {
            oai_emulation.environment_system_config.antenna.eNB_antenna.antenna_orientation_degree[1] = atof(ch);
          } else if (antenna_orientation_degree_3_) {
            oai_emulation.environment_system_config.antenna.eNB_antenna.antenna_orientation_degree[2] = atof(ch);
          }
        } else if (UE_antenna_) {
          if (antenna_gain_dBi_) {
            oai_emulation.environment_system_config.antenna.UE_antenna.antenna_gain_dBi = atof(ch);
          } else if (tx_power_dBm_) {
            oai_emulation.environment_system_config.antenna.UE_antenna.tx_power_dBm = atof(ch);
          } else if (rx_noise_level_dB_) {
            oai_emulation.environment_system_config.antenna.UE_antenna.rx_noise_level_dB = atof(ch);
          }
        }
      }

    } else if (topology_config_) {
      if (area_) {
        if (x_m_) {
          oai_emulation.topology_config.area.x_m = atof(ch);
        } else if (y_m_) {
          oai_emulation.topology_config.area.y_m = atof(ch);
        }
      } else if (network_type_) {
        oai_emulation.topology_config.network_type.selected_option = strndup(ch, len);
      } else if (cell_type_) {
        oai_emulation.topology_config.cell_type.selected_option = strndup(ch, len);
      } else if (relay_) {
        if (number_of_relays_) {
          oai_emulation.topology_config.relay.number_of_relays = atoi(ch);
        }
      } else if (mobility_) {
        if (UE_mobility_) {
          if (UE_mobility_type_) {
            oai_emulation.topology_config.mobility.UE_mobility.UE_mobility_type.selected_option = strndup(ch, len);

          } else if (grid_walk_) {
            if (grid_map_) {
              oai_emulation.topology_config.mobility.UE_mobility.grid_walk.grid_map.selected_option = strndup(ch, len);
              /*
                          if (horizontal_grid_) {
                            oai_emulation.topology_config.mobility.UE_mobility.grid_walk.grid_map.horizontal_grid = atoi(ch);
                          } else if (vertical_grid_) {
                            oai_emulation.topology_config.mobility.UE_mobility.grid_walk.grid_map.vertical_grid = atoi(ch);
                          }
              */
            } else if (grid_trip_type_) {
              oai_emulation.topology_config.mobility.UE_mobility.grid_walk.grid_trip_type.selected_option = strndup(ch, len);
            }
          } else if (UE_initial_distribution_) {
            oai_emulation.topology_config.mobility.UE_mobility.UE_initial_distribution.selected_option = strndup(ch, len);
          } else if (random_UE_distribution_) {
            if (number_of_nodes_) {
              oai_emulation.topology_config.mobility.UE_mobility.random_UE_distribution.number_of_nodes = atoi(ch);
              oai_emulation.info.nb_ue_local = oai_emulation.topology_config.mobility.UE_mobility.random_UE_distribution.number_of_nodes;
            }
          } else if (concentrated_UE_distribution_) {
            if (number_of_nodes_) {
              oai_emulation.topology_config.mobility.UE_mobility.concentrated_UE_distribution.number_of_nodes = atoi(ch);
              oai_emulation.info.nb_ue_local = oai_emulation.topology_config.mobility.UE_mobility.concentrated_UE_distribution.number_of_nodes;
            }
          } else if (grid_UE_distribution_) {
            if (random_grid_) {
              if (number_of_nodes_) {
                oai_emulation.topology_config.mobility.UE_mobility.grid_UE_distribution.random_grid.number_of_nodes = atoi(ch);
                oai_emulation.info.nb_ue_local = oai_emulation.topology_config.mobility.UE_mobility.grid_UE_distribution.random_grid.number_of_nodes;
              }
            } else if (border_grid_) {
              if (number_of_nodes_) {
                oai_emulation.topology_config.mobility.UE_mobility.grid_UE_distribution.border_grid.number_of_nodes = atoi(ch);
                oai_emulation.info.nb_ue_local = oai_emulation.topology_config.mobility.UE_mobility.grid_UE_distribution.border_grid.number_of_nodes;
              }
            }
          } else if (UE_moving_dynamics_) {
            if (min_speed_mps_) {
              oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.min_speed_mps = atof(ch);
            } else if (max_speed_mps_) {
              oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.max_speed_mps = atof(ch);
            } else if (min_sleep_ms_) {
              oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.min_sleep_ms = atof(ch);
            } else if (max_sleep_ms_) {
              oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.max_sleep_ms = atof(ch);
            } else if (min_journey_time_ms_) {
              oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.min_journey_time_ms = atof(ch);
            } else if (max_journey_time_ms_) {
              oai_emulation.topology_config.mobility.UE_mobility.UE_moving_dynamics.max_journey_time_ms = atof(ch);
            }
          } else if (trace_mobility_file_) {
            oai_emulation.topology_config.mobility.UE_mobility.trace_config.trace_mobility_file=strndup(ch,len);
          } else if (sumo_config_) {
            if (sumo_cmd_) {
              oai_emulation.topology_config.mobility.UE_mobility.sumo_config.command=strndup(ch,len);
            } else if (sumo_config_file_) {
              oai_emulation.topology_config.mobility.UE_mobility.sumo_config.file=strndup(ch,len);
            } else if (sumo_start_) {
              oai_emulation.topology_config.mobility.UE_mobility.sumo_config.start=atoi(ch);
            } else if (sumo_end_) {
              oai_emulation.topology_config.mobility.UE_mobility.sumo_config.end=atoi(ch);
            } else if (sumo_step_) {
              oai_emulation.topology_config.mobility.UE_mobility.sumo_config.step=atoi(ch);
            } else if (sumo_hip_) {
              oai_emulation.topology_config.mobility.UE_mobility.sumo_config.hip=strndup(ch,len);
            } else if (sumo_hport_) {
              oai_emulation.topology_config.mobility.UE_mobility.sumo_config.hport=atoi(ch);
            }
          }

        } else if (eNB_mobility_) {
          if (eNB_mobility_type_) {
            oai_emulation.topology_config.mobility.eNB_mobility.eNB_mobility_type.selected_option = strndup(ch, len);

          } else if (eNB_initial_distribution_) {
            oai_emulation.topology_config.mobility.eNB_mobility.eNB_initial_distribution.selected_option = strndup(ch, len);
          } else if (eNB_initial_coordinates_) {
            if (eNB_pos_x_) {
              oai_emulation.topology_config.mobility.eNB_mobility.fixed_eNB_distribution.pos_x = atof(ch);
            } else if (eNB_pos_y_) {
              oai_emulation.topology_config.mobility.eNB_mobility.fixed_eNB_distribution.pos_y = atof(ch);
            }
          } else if (random_eNB_distribution_) {
            if (number_of_cells_) {
              oai_emulation.topology_config.mobility.eNB_mobility.random_eNB_distribution.number_of_cells = atoi(ch);
              oai_emulation.info.nb_enb_local = oai_emulation.topology_config.mobility.eNB_mobility.random_eNB_distribution.number_of_cells;
            }
          } else if (hexagonal_eNB_distribution_) {
            if (number_of_cells_) {
              oai_emulation.topology_config.mobility.eNB_mobility.hexagonal_eNB_distribution.number_of_cells = atoi(ch);
              oai_emulation.info.nb_enb_local = oai_emulation.topology_config.mobility.eNB_mobility.hexagonal_eNB_distribution.number_of_cells;
            } else if (inter_eNB_distance_km_) {
              oai_emulation.topology_config.mobility.eNB_mobility.hexagonal_eNB_distribution.inter_eNB_distance_km = atof(ch);
            }
          } else if (grid_eNB_distribution_) {
            if (number_of_grid_x_) {
              oai_emulation.topology_config.mobility.eNB_mobility.grid_eNB_distribution.number_of_grid_x = atoi(ch);
            } else if (number_of_grid_y_) {
              oai_emulation.topology_config.mobility.eNB_mobility.grid_eNB_distribution.number_of_grid_y = atoi(ch);
              oai_emulation.info.nb_enb_local = oai_emulation.topology_config.mobility.eNB_mobility.grid_eNB_distribution.number_of_grid_x *
                                                oai_emulation.topology_config.mobility.eNB_mobility.grid_eNB_distribution.number_of_grid_y;
            }
          } else if (trace_mobility_file_) {
            oai_emulation.topology_config.mobility.eNB_mobility.trace_config.trace_mobility_file=strndup(ch,len);
          }
        }
      } else if (omv_) {
        oai_emulation.topology_config.omv=atoi(ch);
      }

    } else if (application_config_) {
      if (packet_gen_type_) {
        oai_emulation.application_config.packet_gen_type=strndup(ch, len);
      } else if (predefined_traffic_) {
        if (source_id_) {
          oai_emulation.application_config.predefined_traffic.source_id[oai_emulation.info.max_predefined_traffic_config_index] = strndup(ch, len);
        } else if (destination_id_) {
          oai_emulation.application_config.predefined_traffic.destination_id[oai_emulation.info.max_predefined_traffic_config_index] = strndup(ch, len);
        } else if (application_type_) {
          oai_emulation.application_config.predefined_traffic.application_type[oai_emulation.info.max_predefined_traffic_config_index] = strndup(ch, len);
        } else if (background_traffic_) {
          oai_emulation.application_config.predefined_traffic.background[oai_emulation.info.max_predefined_traffic_config_index] = strndup(ch, len);
        } else if (aggregation_level_) {
          oai_emulation.application_config.predefined_traffic.aggregation_level[oai_emulation.info.max_predefined_traffic_config_index] = atoi(ch);
        } else if (flow_start_) {
          oai_emulation.application_config.predefined_traffic.flow_start[oai_emulation.info.max_predefined_traffic_config_index] = atoi(ch);
        } else if (flow_duration_) {
          oai_emulation.application_config.predefined_traffic.flow_duration[oai_emulation.info.max_predefined_traffic_config_index] = atoi(ch);
        }
      } else if (customized_traffic_) {


        if (prob_off_pu_) {
          oai_emulation.application_config.customized_traffic.prob_off_pu[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
          oai_emulation.application_config.customized_traffic.m2m[oai_emulation.info.max_customized_traffic_config_index] = "enable";
        } else if (prob_off_ed_) {
          oai_emulation.application_config.customized_traffic.prob_off_ed[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
          oai_emulation.application_config.customized_traffic.m2m[oai_emulation.info.max_customized_traffic_config_index] = "enable";
        } else if (prob_off_pe_) {
          oai_emulation.application_config.customized_traffic.prob_off_pe[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
          oai_emulation.application_config.customized_traffic.m2m[oai_emulation.info.max_customized_traffic_config_index] = "enable";
        } else if (prob_pu_ed_) {
          oai_emulation.application_config.customized_traffic.prob_pu_ed[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
          oai_emulation.application_config.customized_traffic.m2m[oai_emulation.info.max_customized_traffic_config_index] = "enable";
        } else if (prob_pu_pe_) {
          oai_emulation.application_config.customized_traffic.prob_pu_pe[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
          oai_emulation.application_config.customized_traffic.m2m[oai_emulation.info.max_customized_traffic_config_index] = "enable";
        } else if (prob_ed_pe_) {
          oai_emulation.application_config.customized_traffic.prob_ed_pe[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
          oai_emulation.application_config.customized_traffic.m2m[oai_emulation.info.max_customized_traffic_config_index] = "enable";
        } else if (prob_ed_pu_) {
          oai_emulation.application_config.customized_traffic.prob_ed_pu[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
          oai_emulation.application_config.customized_traffic.m2m[oai_emulation.info.max_customized_traffic_config_index] = "enable";
        } else if (holding_time_off_ed_) {
          oai_emulation.application_config.customized_traffic.holding_time_off_ed[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
          oai_emulation.application_config.customized_traffic.m2m[oai_emulation.info.max_customized_traffic_config_index] = "enable";
        } else if (holding_time_off_pu_) {
          oai_emulation.application_config.customized_traffic.holding_time_off_pu[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
          oai_emulation.application_config.customized_traffic.m2m[oai_emulation.info.max_customized_traffic_config_index] = "enable";
        } else if (holding_time_off_pe_) {
          oai_emulation.application_config.customized_traffic.holding_time_off_pe[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
          oai_emulation.application_config.customized_traffic.m2m[oai_emulation.info.max_customized_traffic_config_index] = "enable";
        } else if (holding_time_pe_off_) {
          oai_emulation.application_config.customized_traffic.holding_time_pe_off[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
        } else if (pu_size_pkts_) {
          oai_emulation.application_config.customized_traffic.pu_size_pkts[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
          oai_emulation.application_config.customized_traffic.m2m[oai_emulation.info.max_customized_traffic_config_index] = "enable";
        } else if (ed_size_pkts_) {
          oai_emulation.application_config.customized_traffic.ed_size_pkts[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
        } else if (background_traffic_) {
          oai_emulation.application_config.customized_traffic.background[oai_emulation.info.max_customized_traffic_config_index] = strndup(ch, len);
        } else if (m2m_traffic_) {
          oai_emulation.application_config.customized_traffic.m2m[oai_emulation.info.max_customized_traffic_config_index] =  strndup(ch, len);
        } else if (source_id_) {
          oai_emulation.application_config.customized_traffic.source_id[oai_emulation.info.max_customized_traffic_config_index] = strndup(ch, len);
        } else if (destination_id_) {
          oai_emulation.application_config.customized_traffic.destination_id[oai_emulation.info.max_customized_traffic_config_index] = strndup(ch, len);
        } else if (transport_protocol_) {
          oai_emulation.application_config.customized_traffic.transport_protocol[oai_emulation.info.max_customized_traffic_config_index] = strndup(ch, len);
        } else if (ip_version_) {
          oai_emulation.application_config.customized_traffic.ip_version[oai_emulation.info.max_customized_traffic_config_index] = strndup(ch, len);
        } else if (aggregation_level_) {
          oai_emulation.application_config.customized_traffic.aggregation_level[oai_emulation.info.max_customized_traffic_config_index] = atoi(ch);
        } else if (flow_start_) {
          oai_emulation.application_config.customized_traffic.flow_start[oai_emulation.info.max_customized_traffic_config_index] = atoi(ch);
        } else if (flow_duration_) {
          oai_emulation.application_config.customized_traffic.flow_duration[oai_emulation.info.max_customized_traffic_config_index] = atoi(ch);
        } else if (idt_dist_) {
          oai_emulation.application_config.customized_traffic.idt_dist[oai_emulation.info.max_customized_traffic_config_index] = strndup(ch, len);
        } else if (idt_min_ms_) {
          oai_emulation.application_config.customized_traffic.idt_min_ms[oai_emulation.info.max_customized_traffic_config_index] = atoi(ch);
        } else if (idt_max_ms_) {
          oai_emulation.application_config.customized_traffic.idt_max_ms[oai_emulation.info.max_customized_traffic_config_index] = atoi(ch);
        } else if (idt_standard_deviation_) {
          oai_emulation.application_config.customized_traffic.idt_standard_deviation[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
        } else if (idt_lambda_) {
          oai_emulation.application_config.customized_traffic.idt_lambda[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
        } else if (idt_scale_) {
          oai_emulation.application_config.customized_traffic.idt_scale[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
        } else if (idt_shape_) {
          oai_emulation.application_config.customized_traffic.idt_shape[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
        } else if (size_dist_) {
          oai_emulation.application_config.customized_traffic.size_dist[oai_emulation.info.max_customized_traffic_config_index] = strndup(ch, len);
        } else if (size_min_byte_) {
          oai_emulation.application_config.customized_traffic.size_min_byte[oai_emulation.info.max_customized_traffic_config_index] = atoi(ch);
        } else if (size_max_byte_) {
          oai_emulation.application_config.customized_traffic.size_max_byte[oai_emulation.info.max_customized_traffic_config_index] = atoi(ch);
        } else if (size_standard_deviation_) {
          oai_emulation.application_config.customized_traffic.size_standard_deviation[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
        } else if (size_lambda_) {
          oai_emulation.application_config.customized_traffic.size_lambda[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
        } else if (size_scale_) {
          oai_emulation.application_config.customized_traffic.size_scale[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
        } else if (size_shape_) {
          oai_emulation.application_config.customized_traffic.size_shape[oai_emulation.info.max_customized_traffic_config_index] = atof(ch);
        } else if (stream_) {
          oai_emulation.application_config.customized_traffic.stream[oai_emulation.info.max_customized_traffic_config_index] = atoi(ch);
        } else if (destination_port_) {
          oai_emulation.application_config.customized_traffic.destination_port[oai_emulation.info.max_customized_traffic_config_index] = atoi(ch);
        }
      }

    } else if (emulation_config_) {
      if (emulation_time_ms_) {
        oai_emulation.emulation_config.emulation_time_ms = atof(ch);
      } else if (curve_) {
        oai_emulation.emulation_config.curve = strndup(ch, len);
      } else if (profiling_) {
        oai_emulation.info.opp_enabled = atoi(ch);
      } else if (trace_file_) {
        oai_emulation.info.opt_enabled = 1;

        if (strncmp(ch, "wireshark", len) == 0) {
          opt_type = OPT_WIRESHARK;
        } else if (strncmp(ch, "pcap", len) == 0) {
          opt_type = OPT_PCAP;
        } else {
          opt_type = OPT_NONE;
          oai_emulation.info.opt_enabled = 0;
        }

        oai_emulation.info.opt_mode = opt_type;
      } else if (background_stats_) {
        oai_emulation.emulation_config.background_stats = strndup(ch, len);
      } else if (performance_metrics_) {
        if (throughput_) {
          oai_emulation.emulation_config.performance_metrics.throughput = strndup(ch, len);
        } else if (latency_) {
          oai_emulation.emulation_config.performance_metrics.latency = strndup(ch, len);
        } else if (loss_rate_) {
          oai_emulation.emulation_config.performance_metrics.loss_rate = strndup(ch, len);
        } else if (owd_radio_access_) {
          oai_emulation.emulation_config.performance_metrics.owd_radio_access = strndup(ch, len);
        }
      } else if (layer_) {
        if (phy_) {
          oai_emulation.emulation_config.layer.phy = atoi(ch);
        } else if (mac_) {
          oai_emulation.emulation_config.layer.mac = atoi(ch);
        } else if (rlc_) {
          oai_emulation.emulation_config.layer.rlc = atoi(ch);
        } else if (pdcp_) {
          oai_emulation.emulation_config.layer.pdcp = atoi(ch);
        } else if (rrc_) {
          oai_emulation.emulation_config.layer.rrc = atoi(ch);
        } else if (omg_) {
          oai_emulation.emulation_config.layer.omg = atoi(ch);
        } else if (otg_) {
          oai_emulation.emulation_config.layer.otg = atoi(ch);
        } else if (emu_) {
          oai_emulation.emulation_config.layer.emu = atoi(ch);
        }
      } else if (log_) {
        if (level_) {
          oai_emulation.emulation_config.log_emu.level = strndup(ch,len);
        } else if (verbosity_) {
          oai_emulation.emulation_config.log_emu.verbosity = strndup(ch,len);
        } else if (interval_) {
          oai_emulation.emulation_config.log_emu.interval = atoi(ch);
        }
      } else if (packet_trace_) {
        oai_emulation.emulation_config.packet_trace.enabled = atoi(ch);
      } else if (seed_value_) {
        oai_emulation.emulation_config.seed.value = atoi(ch);
      } else if (cli_enabled_) {
        oai_emulation.info.cli_enabled = 1;

        if (cli_start_enb_) {
          for (i=0; i < oai_emulation.info.cli_num_enb; i++)
            oai_emulation.info.cli_start_enb[i] = atoi(ch);
        } else if (cli_start_ue_) {
          for (i=0; i < oai_emulation.info.cli_num_ue; i++)
            oai_emulation.info.cli_start_ue[i] =  atoi(ch);
        }
      }
    } else if (protocol_) {
      if (mac_) {
        if (num_groups_) {
          oai_emulation.protocol_config.eNB_mac_config.num_groups=atoi(ch);
        }
      }
    } else if (profile_) {
      oai_emulation.profile = strndup(ch, len);
    }
  }
}


int parse_XML(char src_file[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX])
{
  // config the parser, refer to 'http://www.saxproject.org/apidoc/org/xml/sax/ContentHandler.html'
  xmlSAXHandler sax_handler = { 0 }; // a Simple API for XML (SAX) handler
  sax_handler.startDocument = start_document;
  sax_handler.endDocument = end_document;
  sax_handler.startElement = start_element;
  sax_handler.endElement = end_element;
  sax_handler.characters = characters;

  // Parsing the XML file
  if (xmlSAXUserParseFile(&sax_handler, NULL, src_file) != 0) {
    LOG_E(OCG, "An error occurs while parsing the configuration file!\n");
    return MODULE_ERROR;
  }

  LOG_I(OCG, "The configuration file is parsed\n");
  return MODULE_OK;
}
