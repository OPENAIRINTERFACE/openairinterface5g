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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/*
                                enb_config.c
                             -------------------
  AUTHOR  : Lionel GAUTHIER, navid nikaein, Laurent Winckel
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr, navid.nikaein@eurecom.fr
 */

#include <string.h>
#include <libconfig.h>
#include <inttypes.h>

#include "log.h"
#include "log_extern.h"
#include "assertions.h"
#include "enb_config.h"
#include "UTIL/OTG/otg.h"
#include "UTIL/OTG/otg_externs.h"
#if defined(OAI_EMU)
# include "OCG.h"
# include "OCG_extern.h"
#endif
#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
# if defined(ENABLE_USE_MME)
#   include "s1ap_eNB.h"
#   include "sctp_eNB_task.h"
# endif
#endif
#include "sctp_default_values.h"
#include "SystemInformationBlockType2.h"
#include "LAYER2/MAC/extern.h"

/* those macros are here to help diagnose problems in configuration files
 * if the lookup fails, a warning is printed
 * (yes we can use the function name for the macro itself, the C preprocessor
 * won't die in an infinite loop)
 */
#define config_setting_lookup_int(setting, name, value) \
    (config_setting_lookup_int(setting, name, value) || \
    (printf("WARNING: setting '%s' not found in configuration file\n", name), 0))
#define config_setting_lookup_int64(setting, name, value) \
    (config_setting_lookup_int64(setting, name, value) || \
    (printf("WARNING: setting '%s' not found in configuration file\n", name), 0))
#define config_setting_lookup_float(setting, name, value) \
    (config_setting_lookup_float(setting, name, value) || \
    (printf("WARNING: setting '%s' not found in configuration file\n", name), 0))
#define config_setting_lookup_bool(setting, name, value) \
    (config_setting_lookup_bool(setting, name, value) || \
    (printf("WARNING: setting '%s' not found in configuration file\n", name), 0))
#define config_setting_lookup_string(setting, name, value) \
    (config_setting_lookup_string(setting, name, value) || \
    (printf("WARNING: setting '%s' not found in configuration file\n", name), 0))

#define ENB_CONFIG_STRING_ACTIVE_ENBS                   "Active_eNBs"

#define ENB_CONFIG_STRING_ENB_LIST                      "eNBs"
#define ENB_CONFIG_STRING_ENB_ID                        "eNB_ID"
#define ENB_CONFIG_STRING_CELL_TYPE                     "cell_type"
#define ENB_CONFIG_STRING_ENB_NAME                      "eNB_name"

#define ENB_CONFIG_STRING_TRACKING_AREA_CODE            "tracking_area_code"
#define ENB_CONFIG_STRING_MOBILE_COUNTRY_CODE           "mobile_country_code"
#define ENB_CONFIG_STRING_MOBILE_NETWORK_CODE           "mobile_network_code"

#define ENB_CONFIG_STRING_COMPONENT_CARRIERS                            "component_carriers"

#define ENB_CONFIG_STRING_FRAME_TYPE                                    "frame_type"
#define ENB_CONFIG_STRING_TDD_CONFIG                                    "tdd_config"
#define ENB_CONFIG_STRING_TDD_CONFIG_S                                  "tdd_config_s"
#define ENB_CONFIG_STRING_PREFIX_TYPE                                   "prefix_type"
#define ENB_CONFIG_STRING_EUTRA_BAND                                    "eutra_band"
#define ENB_CONFIG_STRING_DOWNLINK_FREQUENCY                            "downlink_frequency"
#define ENB_CONFIG_STRING_UPLINK_FREQUENCY_OFFSET                       "uplink_frequency_offset"

#define ENB_CONFIG_STRING_NID_CELL                                      "Nid_cell"
#define ENB_CONFIG_STRING_N_RB_DL                                       "N_RB_DL"
#define ENB_CONFIG_STRING_CELL_MBSFN                                  "Nid_cell_mbsfn"
#define ENB_CONFIG_STRING_NB_ANT_PORTS                              "nb_antenna_ports"
#define ENB_CONFIG_STRING_NB_ANT_TX                                 "nb_antennas_tx"
#define ENB_CONFIG_STRING_NB_ANT_RX                                 "nb_antennas_rx"
#define ENB_CONFIG_STRING_TX_GAIN                                       "tx_gain"
#define ENB_CONFIG_STRING_RX_GAIN                                       "rx_gain"
#define ENB_CONFIG_STRING_PRACH_ROOT                                  "prach_root"
#define ENB_CONFIG_STRING_PRACH_CONFIG_INDEX                          "prach_config_index"
#define ENB_CONFIG_STRING_PRACH_HIGH_SPEED                          "prach_high_speed"
#define ENB_CONFIG_STRING_PRACH_ZERO_CORRELATION                  "prach_zero_correlation"
#define ENB_CONFIG_STRING_PRACH_FREQ_OFFSET                         "prach_freq_offset"
#define ENB_CONFIG_STRING_PUCCH_DELTA_SHIFT                         "pucch_delta_shift"
#define ENB_CONFIG_STRING_PUCCH_NRB_CQI                                 "pucch_nRB_CQI"
#define ENB_CONFIG_STRING_PUCCH_NCS_AN                                  "pucch_nCS_AN"
#ifndef Rel10
#define ENB_CONFIG_STRING_PUCCH_N1_AN                                 "pucch_n1_AN"
#endif
#define ENB_CONFIG_STRING_PDSCH_RS_EPRE                                 "pdsch_referenceSignalPower"
#define ENB_CONFIG_STRING_PDSCH_PB                                  "pdsch_p_b"
#define ENB_CONFIG_STRING_PUSCH_N_SB                                  "pusch_n_SB"
#define ENB_CONFIG_STRING_PUSCH_HOPPINGMODE                             "pusch_hoppingMode"
#define ENB_CONFIG_STRING_PUSCH_HOPPINGOFFSET                           "pusch_hoppingOffset"
#define ENB_CONFIG_STRING_PUSCH_ENABLE64QAM                         "pusch_enable64QAM"
#define ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN                  "pusch_groupHoppingEnabled"
#define ENB_CONFIG_STRING_PUSCH_GROUP_ASSIGNMENT                  "pusch_groupAssignment"
#define ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN                 "pusch_sequenceHoppingEnabled"
#define ENB_CONFIG_STRING_PUSCH_NDMRS1                                  "pusch_nDMRS1"
#define ENB_CONFIG_STRING_PHICH_DURATION                          "phich_duration"
#define ENB_CONFIG_STRING_PHICH_RESOURCE                          "phich_resource"
#define ENB_CONFIG_STRING_SRS_ENABLE                                  "srs_enable"
#define ENB_CONFIG_STRING_SRS_BANDWIDTH_CONFIG                          "srs_BandwidthConfig"
#define ENB_CONFIG_STRING_SRS_SUBFRAME_CONFIG                         "srs_SubframeConfig"
#define ENB_CONFIG_STRING_SRS_ACKNACKST_CONFIG                          "srs_ackNackST"
#define ENB_CONFIG_STRING_SRS_MAXUPPTS                                  "srs_MaxUpPts"
#define ENB_CONFIG_STRING_PUSCH_PO_NOMINAL                          "pusch_p0_Nominal"
#define ENB_CONFIG_STRING_PUSCH_ALPHA                                 "pusch_alpha"
#define ENB_CONFIG_STRING_PUCCH_PO_NOMINAL                          "pucch_p0_Nominal"
#define ENB_CONFIG_STRING_MSG3_DELTA_PREAMBLE                         "msg3_delta_Preamble"
#define ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1                          "pucch_deltaF_Format1"
#define ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1b                         "pucch_deltaF_Format1b"
#define ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2                          "pucch_deltaF_Format2"
#define ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2A                         "pucch_deltaF_Format2a"
#define ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2B                         "pucch_deltaF_Format2b"
#define ENB_CONFIG_STRING_RACH_NUM_RA_PREAMBLES                         "rach_numberOfRA_Preambles"
#define ENB_CONFIG_STRING_RACH_PREAMBLESGROUPACONFIG                  "rach_preamblesGroupAConfig"
#define ENB_CONFIG_STRING_RACH_SIZEOFRA_PREAMBLESGROUPA                 "rach_sizeOfRA_PreamblesGroupA"
#define ENB_CONFIG_STRING_RACH_MESSAGESIZEGROUPA                        "rach_messageSizeGroupA"
#define ENB_CONFIG_STRING_RACH_MESSAGEPOWEROFFSETGROUPB                 "rach_messagePowerOffsetGroupB"
#define ENB_CONFIG_STRING_RACH_POWERRAMPINGSTEP                         "rach_powerRampingStep"
#define ENB_CONFIG_STRING_RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER "rach_preambleInitialReceivedTargetPower"
#define ENB_CONFIG_STRING_RACH_PREAMBLETRANSMAX                         "rach_preambleTransMax"
#define ENB_CONFIG_STRING_RACH_RARESPONSEWINDOWSIZE                 "rach_raResponseWindowSize"
#define ENB_CONFIG_STRING_RACH_MACCONTENTIONRESOLUTIONTIMER         "rach_macContentionResolutionTimer"
#define ENB_CONFIG_STRING_RACH_MAXHARQMSG3TX                          "rach_maxHARQ_Msg3Tx"
#define ENB_CONFIG_STRING_PCCH_DEFAULT_PAGING_CYCLE                     "pcch_default_PagingCycle"
#define ENB_CONFIG_STRING_PCCH_NB                                       "pcch_nB"
#define ENB_CONFIG_STRING_BCCH_MODIFICATIONPERIODCOEFF                  "bcch_modificationPeriodCoeff"
#define ENB_CONFIG_STRING_UETIMERS_T300                                 "ue_TimersAndConstants_t300"
#define ENB_CONFIG_STRING_UETIMERS_T301                                 "ue_TimersAndConstants_t301"
#define ENB_CONFIG_STRING_UETIMERS_T310                                 "ue_TimersAndConstants_t310"
#define ENB_CONFIG_STRING_UETIMERS_T311                                 "ue_TimersAndConstants_t311"
#define ENB_CONFIG_STRING_UETIMERS_N310                                 "ue_TimersAndConstants_n310"
#define ENB_CONFIG_STRING_UETIMERS_N311                                 "ue_TimersAndConstants_n311"
#define ENB_CONFIG_STRING_UE_TRANSMISSION_MODE                          "ue_TransmissionMode"

#define ENB_CONFIG_STRING_SRB1                                          "srb1_parameters"
#define ENB_CONFIG_STRING_SRB1_TIMER_POLL_RETRANSMIT                    "timer_poll_retransmit"
#define ENB_CONFIG_STRING_SRB1_TIMER_REORDERING                         "timer_reordering"
#define ENB_CONFIG_STRING_SRB1_TIMER_STATUS_PROHIBIT                    "timer_status_prohibit"
#define ENB_CONFIG_STRING_SRB1_POLL_PDU                                 "poll_pdu"
#define ENB_CONFIG_STRING_SRB1_POLL_BYTE                                "poll_byte"
#define ENB_CONFIG_STRING_SRB1_MAX_RETX_THRESHOLD                       "max_retx_threshold"
#define ENB_CONFIG_STRING_MME_IP_ADDRESS                "mme_ip_address"
#define ENB_CONFIG_STRING_MME_IPV4_ADDRESS              "ipv4"
#define ENB_CONFIG_STRING_MME_IPV6_ADDRESS              "ipv6"
#define ENB_CONFIG_STRING_MME_IP_ADDRESS_ACTIVE         "active"
#define ENB_CONFIG_STRING_MME_IP_ADDRESS_PREFERENCE     "preference"

#define ENB_CONFIG_STRING_SCTP_CONFIG                    "SCTP"
#define ENB_CONFIG_STRING_SCTP_INSTREAMS                 "SCTP_INSTREAMS"
#define ENB_CONFIG_STRING_SCTP_OUTSTREAMS                "SCTP_OUTSTREAMS"

#define ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG     "NETWORK_INTERFACES"
#define ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1_MME "ENB_INTERFACE_NAME_FOR_S1_MME"
#define ENB_CONFIG_STRING_ENB_IPV4_ADDRESS_FOR_S1_MME   "ENB_IPV4_ADDRESS_FOR_S1_MME"
#define ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1U    "ENB_INTERFACE_NAME_FOR_S1U"
#define ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_S1U         "ENB_IPV4_ADDRESS_FOR_S1U"
#define ENB_CONFIG_STRING_ENB_PORT_FOR_S1U              "ENB_PORT_FOR_S1U"

#define ENB_CONFIG_STRING_RRH_GW_CONFIG                   "rrh_gw_config"
#define ENB_CONFIG_STRING_RRH_GW_LOCAL_IF_NAME            "local_if_name"
#define ENB_CONFIG_STRING_RRH_GW_LOCAL_ADDRESS            "local_address"
#define ENB_CONFIG_STRING_RRH_GW_REMOTE_ADDRESS           "remote_address"
#define ENB_CONFIG_STRING_RRH_GW_LOCAL_PORT               "local_port"
#define ENB_CONFIG_STRING_RRH_GW_REMOTE_PORT              "remote_port"
#define ENB_CONFIG_STRING_RRH_GW_ACTIVE                   "rrh_gw_active"
#define ENB_CONFIG_STRING_RRH_GW_TRANSPORT_PREFERENCE     "tr_preference"
#define ENB_CONFIG_STRING_RRH_GW_RF_TARGET_PREFERENCE     "rf_preference"
#define ENB_CONFIG_STRING_RRH_GW_IQ_TXSHIFT               "iq_txshift"
#define ENB_CONFIG_STRING_RRH_GW_TX_SAMPLE_ADVANCE        "tx_sample_advance"
#define ENB_CONFIG_STRING_RRH_GW_TX_SCHEDULING_ADVANCE    "tx_scheduling_advance"

#define ENB_CONFIG_STRING_ASN1_VERBOSITY                   "Asn1_verbosity"
#define ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE              "none"
#define ENB_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING          "annoying"
#define ENB_CONFIG_STRING_ASN1_VERBOSITY_INFO              "info"

// OTG config per ENB-UE DL
#define ENB_CONF_STRING_OTG_CONFIG                         "otg_config"
#define ENB_CONF_STRING_OTG_UE_ID                          "ue_id"
#define ENB_CONF_STRING_OTG_APP_TYPE                       "app_type"
#define ENB_CONF_STRING_OTG_BG_TRAFFIC                     "bg_traffic"

// per eNB configuration
#define ENB_CONFIG_STRING_LOG_CONFIG                       "log_config"
#define ENB_CONFIG_STRING_GLOBAL_LOG_LEVEL                 "global_log_level"
#define ENB_CONFIG_STRING_GLOBAL_LOG_VERBOSITY             "global_log_verbosity"
#define ENB_CONFIG_STRING_HW_LOG_LEVEL                     "hw_log_level"
#define ENB_CONFIG_STRING_HW_LOG_VERBOSITY                 "hw_log_verbosity"
#define ENB_CONFIG_STRING_PHY_LOG_LEVEL                    "phy_log_level"
#define ENB_CONFIG_STRING_PHY_LOG_VERBOSITY                "phy_log_verbosity"
#define ENB_CONFIG_STRING_MAC_LOG_LEVEL                    "mac_log_level"
#define ENB_CONFIG_STRING_MAC_LOG_VERBOSITY                "mac_log_verbosity"
#define ENB_CONFIG_STRING_RLC_LOG_LEVEL                    "rlc_log_level"
#define ENB_CONFIG_STRING_RLC_LOG_VERBOSITY                "rlc_log_verbosity"
#define ENB_CONFIG_STRING_PDCP_LOG_LEVEL                   "pdcp_log_level"
#define ENB_CONFIG_STRING_PDCP_LOG_VERBOSITY               "pdcp_log_verbosity"
#define ENB_CONFIG_STRING_RRC_LOG_LEVEL                    "rrc_log_level"
#define ENB_CONFIG_STRING_RRC_LOG_VERBOSITY                "rrc_log_verbosity"
#define ENB_CONFIG_STRING_GTPU_LOG_LEVEL                   "gtpu_log_level"
#define ENB_CONFIG_STRING_GTPU_LOG_VERBOSITY               "gtpu_log_verbosity"
#define ENB_CONFIG_STRING_UDP_LOG_LEVEL                    "udp_log_level"
#define ENB_CONFIG_STRING_UDP_LOG_VERBOSITY                "udp_log_verbosity"
#define ENB_CONFIG_STRING_OSA_LOG_LEVEL                    "osa_log_level"
#define ENB_CONFIG_STRING_OSA_LOG_VERBOSITY                "osa_log_verbosity"


#define KHz (1000UL)
#define MHz (1000 * KHz)

typedef struct eutra_band_s {
  int16_t             band;
  uint32_t            ul_min;
  uint32_t            ul_max;
  uint32_t            dl_min;
  uint32_t            dl_max;
  lte_frame_type_t    frame_type;
} eutra_band_t;

static const eutra_band_t eutra_bands[] = {
  { 1, 1920    * MHz, 1980    * MHz, 2110    * MHz, 2170    * MHz, FDD},
  { 2, 1850    * MHz, 1910    * MHz, 1930    * MHz, 1990    * MHz, FDD},
  { 3, 1710    * MHz, 1785    * MHz, 1805    * MHz, 1880    * MHz, FDD},
  { 4, 1710    * MHz, 1755    * MHz, 2110    * MHz, 2155    * MHz, FDD},
  { 5,  824    * MHz,  849    * MHz,  869    * MHz,  894    * MHz, FDD},
  { 6,  830    * MHz,  840    * MHz,  875    * MHz,  885    * MHz, FDD},
  { 7, 2500    * MHz, 2570    * MHz, 2620    * MHz, 2690    * MHz, FDD},
  { 8,  880    * MHz,  915    * MHz,  925    * MHz,  960    * MHz, FDD},
  { 9, 1749900 * KHz, 1784900 * KHz, 1844900 * KHz, 1879900 * KHz, FDD},
  {10, 1710    * MHz, 1770    * MHz, 2110    * MHz, 2170    * MHz, FDD},
  {11, 1427900 * KHz, 1452900 * KHz, 1475900 * KHz, 1500900 * KHz, FDD},
  {12,  698    * MHz,  716    * MHz,  728    * MHz,  746    * MHz, FDD},
  {13,  777    * MHz,  787    * MHz,  746    * MHz,  756    * MHz, FDD},
  {14,  788    * MHz,  798    * MHz,  758    * MHz,  768    * MHz, FDD},

  {17,  704    * MHz,  716    * MHz,  734    * MHz,  746    * MHz, FDD},
  {20,  832    * MHz,  862    * MHz,  791    * MHz,  821    * MHz, FDD},
  {33, 1900    * MHz, 1920    * MHz, 1900    * MHz, 1920    * MHz, TDD},
  {33, 1900    * MHz, 1920    * MHz, 1900    * MHz, 1920    * MHz, TDD},
  {34, 2010    * MHz, 2025    * MHz, 2010    * MHz, 2025    * MHz, TDD},
  {35, 1850    * MHz, 1910    * MHz, 1850    * MHz, 1910    * MHz, TDD},
  {36, 1930    * MHz, 1990    * MHz, 1930    * MHz, 1990    * MHz, TDD},
  {37, 1910    * MHz, 1930    * MHz, 1910    * MHz, 1930    * MHz, TDD},
  {38, 2570    * MHz, 2620    * MHz, 2570    * MHz, 2630    * MHz, TDD},
  {39, 1880    * MHz, 1920    * MHz, 1880    * MHz, 1920    * MHz, TDD},
  {40, 2300    * MHz, 2400    * MHz, 2300    * MHz, 2400    * MHz, TDD},
  {41, 2496    * MHz, 2690    * MHz, 2496    * MHz, 2690    * MHz, TDD},
  {42, 3400    * MHz, 3600    * MHz, 3400    * MHz, 3600    * MHz, TDD},
  {43, 3600    * MHz, 3800    * MHz, 3600    * MHz, 3800    * MHz, TDD},
  {44, 703    * MHz, 803    * MHz, 703    * MHz, 803    * MHz, TDD},
};

Enb_properties_array_t enb_properties;

void enb_config_display(void)
{
  int i,j;

  printf( "\n----------------------------------------------------------------------\n");
  printf( " ENB CONFIG FILE CONTENT LOADED (TBC):\n");
  printf( "----------------------------------------------------------------------\n");

  for (i = 0; i < enb_properties.number; i++) {
    printf( "ENB CONFIG for instance %u:\n\n", i);
    printf( "\teNB name:           \t%s:\n",enb_properties.properties[i]->eNB_name);
    printf( "\teNB ID:             \t%"PRIu32":\n",enb_properties.properties[i]->eNB_id);
    printf( "\tCell type:          \t%s:\n",enb_properties.properties[i]->cell_type == CELL_MACRO_ENB ? "CELL_MACRO_ENB":"CELL_HOME_ENB");
    printf( "\tTAC:                \t%"PRIu16":\n",enb_properties.properties[i]->tac);
    printf( "\tMCC:                \t%"PRIu16":\n",enb_properties.properties[i]->mcc);

    if (enb_properties.properties[i]->mnc_digit_length == 3) {
      printf( "\tMNC:                \t%03"PRIu16":\n",enb_properties.properties[i]->mnc);
    } else {
      printf( "\tMNC:                \t%02"PRIu16":\n",enb_properties.properties[i]->mnc);
    }
    
    for (j=0; j< enb_properties.properties[i]->nb_rrh_gw; j++) {
      if (enb_properties.properties[i]->rrh_gw_config[j].active == 1 ){
	printf( "\n\tRRH GW %d config for eNB %u:\n\n", j, i);
	printf( "\tinterface name :       \t%s:\n",enb_properties.properties[i]->rrh_gw_if_name);
	printf( "\tlocal address  :       \t%s:\n",enb_properties.properties[i]->rrh_gw_config[j].local_address);
	printf( "\tlocal port     :       \t%d:\n",enb_properties.properties[i]->rrh_gw_config[j].local_port);
	printf( "\tremote address :       \t%s:\n",enb_properties.properties[i]->rrh_gw_config[j].remote_address);
	printf( "\tremote port    :       \t%d:\n",enb_properties.properties[i]->rrh_gw_config[j].remote_port);
	printf( "\ttx_scheduling_advance :\t%d:\n",enb_properties.properties[i]->rrh_gw_config[j].tx_scheduling_advance);
	printf( "\ttx_sample_advance :    \t%d:\n",enb_properties.properties[i]->rrh_gw_config[j].tx_sample_advance);
	printf( "\tiq_txshift :           \t%d:\n",enb_properties.properties[i]->rrh_gw_config[j].iq_txshift);
	printf( "\ttransport  :           \t%s Ethernet:\n",(enb_properties.properties[i]->rrh_gw_config[j].raw == 1)? "RAW" : "UDP");
	if (enb_properties.properties[i]->rrh_gw_config[j].exmimo == 1) {
	  printf( "\tRF target  :           \tEXMIMO:\n\n");
	} else if (enb_properties.properties[i]->rrh_gw_config[j].usrp_b200 == 1) {
	  printf( "\tRF target  :           \tUSRP_B200:\n\n");
	} else if (enb_properties.properties[i]->rrh_gw_config[j].usrp_x300 == 1) {
	  printf( "\tRF target  :           \tUSRP_X300:\n\n");
	} else if (enb_properties.properties[i]->rrh_gw_config[j].bladerf == 1) {
	  printf( "\tRF target  :           \tBLADERF:\n\n");
	} else if (enb_properties.properties[i]->rrh_gw_config[j].lmssdr == 1) {
	  printf( "\tRF target  :           \tLMSSDR:\n\n");
	} else {
	  printf( "\tRF target  :           \tNONE:\n\n");
	}
      }
    }

    for (j=0; j< enb_properties.properties[i]->nb_cc; j++) {
      printf( "\teutra band for CC %d:         \t%"PRId16":\n",j,enb_properties.properties[i]->eutra_band[j]);
      printf( "\tdownlink freq for CC %d:      \t%"PRIu64":\n",j,enb_properties.properties[i]->downlink_frequency[j]);
      printf( "\tuplink freq offset for CC %d: \t%"PRId32":\n",j,enb_properties.properties[i]->uplink_frequency_offset[j]);

      printf( "\n\tCell ID for CC %d:\t%"PRId16":\n",j,enb_properties.properties[i]->Nid_cell[j]);
      printf( "\tN_RB_DL for CC %d:\t%"PRId16":\n",j,enb_properties.properties[i]->N_RB_DL[j]);
      printf( "\tnb_antenna_ports for CC %d:\t%d:\n",j,enb_properties.properties[i]->nb_antenna_ports[j]);
      printf( "\tnb_antennas_tx for CC %d:\t%d:\n",j,enb_properties.properties[i]->nb_antennas_tx[j]);
      printf( "\tnb_antennas_rx for CC %d:\t%d:\n",j,enb_properties.properties[i]->nb_antennas_rx[j]);

      // RACH-Config
      printf( "\trach_numberOfRA_Preambles for CC %d:\t%ld:\n",j,enb_properties.properties[i]->rach_numberOfRA_Preambles[j]);
      printf( "\trach_preamblesGroupAConfig for CC %d:\t%d:\n",j,enb_properties.properties[i]->rach_preamblesGroupAConfig[j]);

      if (enb_properties.properties[i]->rach_preamblesGroupAConfig[j]) {
        printf( "\trach_sizeOfRA_PreamblesGroupA for CC %d:\t%ld:\n",j,enb_properties.properties[i]->rach_sizeOfRA_PreamblesGroupA[j]);
        printf( "\trach_messageSizeGroupA for CC %d:\t%ld:\n",j,enb_properties.properties[i]->rach_messageSizeGroupA[j]);
        printf( "\trach_messagePowerOffsetGroupB for CC %d:\t%d:\n",j,enb_properties.properties[i]->rach_messagePowerOffsetGroupB[j]);
      }

      printf( "\trach_powerRampingStep for CC %d:\t%ld:\n",j,enb_properties.properties[i]->rach_powerRampingStep[j]);
      printf( "\trach_preambleInitialReceivedTargetPower for CC %d:\t%ld:\n",j,enb_properties.properties[i]->rach_preambleInitialReceivedTargetPower[j]);
      printf( "\trach_preambleTransMax for CC %d:\t%ld:\n",j,enb_properties.properties[i]->rach_preambleTransMax[j]);
      printf( "\trach_raResponseWindowSize for CC %d:\t%ld:\n",j,enb_properties.properties[i]->rach_raResponseWindowSize[j]);
      printf( "\trach_macContentionResolutionTimer for CC %d:\t%ld:\n",j,enb_properties.properties[i]->rach_macContentionResolutionTimer[j]);
      printf( "\trach_maxHARQ_Msg3Tx for CC %d:\t%ld:\n",j,enb_properties.properties[i]->rach_maxHARQ_Msg3Tx[j]);

      // BCCH-Config
      printf( "\tbcch_modificationPeriodCoeff for CC %d:\t%ld:\n",j,enb_properties.properties[i]->bcch_modificationPeriodCoeff[j]);

      // PCCH-Config
      printf( "\tpcch_defaultPagingCycle for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pcch_defaultPagingCycle[j]);
      printf( "\tpcch_nB for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pcch_nB[j]);

      // PRACH-Config
      printf( "\tprach_root for CC %d:\t%ld:\n",j,enb_properties.properties[i]->prach_root[j]);
      printf( "\tprach_config_index for CC %d:\t%ld:\n",j,enb_properties.properties[i]->prach_config_index[j]);
      printf( "\tprach_high_speed for CC %d:\t%d:\n",j,enb_properties.properties[i]->prach_high_speed[j]);
      printf( "\tprach_zero_correlation for CC %d:\t%ld:\n",j,enb_properties.properties[i]->prach_zero_correlation[j]);
      printf( "\tprach_freq_offset for CC %d:\t%ld:\n",j,enb_properties.properties[i]->prach_freq_offset[j]);

      // PDSCH-Config
      printf( "\tpdsch_referenceSignalPower for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pdsch_referenceSignalPower[j]);
      printf( "\tpdsch_p_b for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pdsch_p_b[j]);

      // PUSCH-Config
      printf( "\tpusch_n_SB for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pusch_n_SB[j]);
      printf( "\tpusch_hoppingMode for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pusch_hoppingMode[j]);
      printf( "\tpusch_hoppingOffset for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pusch_hoppingOffset[j]);
      printf( "\tpusch_enable64QAM for CC %d:\t%d:\n",j,enb_properties.properties[i]->pusch_enable64QAM[j]);
      printf( "\tpusch_groupHoppingEnabled for CC %d:\t%d:\n",j,enb_properties.properties[i]->pusch_groupHoppingEnabled[j]);
      printf( "\tpusch_groupAssignment for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pusch_groupAssignment[j]);
      printf( "\tpusch_sequenceHoppingEnabled for CC %d:\t%d:\n",j,enb_properties.properties[i]->pusch_sequenceHoppingEnabled[j]);
      printf( "\tpusch_nDMRS1 for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pusch_nDMRS1[j]);

      // PUCCH-Config

      printf( "\tpucch_delta_shift for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pucch_delta_shift[j]);
      printf( "\tpucch_nRB_CQI for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pucch_nRB_CQI[j]);
      printf( "\tpucch_nCS_AN for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pucch_nCS_AN[j]);
#ifndef Rel10
      printf( "\tpucch_n1_AN for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pucch_n1_AN[j]);
#endif

      // SRS Config
      printf( "\tsrs_enable for CC %d:\t%d:\n",j,enb_properties.properties[i]->srs_enable[j]);

      if (enb_properties.properties[i]->srs_enable[j]) {
        printf( "\tsrs_BandwidthConfig for CC %d:\t%ld:\n",j,enb_properties.properties[i]->srs_BandwidthConfig[j]);
        printf( "\tsrs_BandwidthConfig for CC %d:\t%ld:\n",j,enb_properties.properties[i]->srs_SubframeConfig[j]);
        printf( "\tsrs_ackNackST for CC %d:\t%d:\n",j,enb_properties.properties[i]->srs_ackNackST[j]);
        printf( "\tsrs_MaxUpPts for CC %d:\t%d:\n",j,enb_properties.properties[i]->srs_MaxUpPts[j]);
      }

      // uplinkPowerControlCommon

      printf( "\tpusch_p0_Nominal for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pusch_p0_Nominal[j]);
      printf( "\tpucch_p0_Nominal for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pucch_p0_Nominal[j]);
      printf( "\tpusch_alpha for CC %d:\t%ld:\n",j,enb_properties.properties[i]->pusch_alpha[j]);
      printf( "\tpucch_deltaF_Format1 for CC %d:\t%d:\n",j,enb_properties.properties[i]->pucch_deltaF_Format1[j]);
      printf( "\tpucch_deltaF_Format1b for CC %d:\t%d:\n",j,enb_properties.properties[i]->pucch_deltaF_Format1b[j]);
      printf( "\tpucch_deltaF_Format2 for CC %d:\t%d:\n",j,enb_properties.properties[i]->pucch_deltaF_Format2[j]);
      printf( "\tpucch_deltaF_Format2a for CC %d:\t%d:\n",j,enb_properties.properties[i]->pucch_deltaF_Format2a[j]);
      printf( "\tpucch_deltaF_Format2b for CC %d:\t%d:\n",j,enb_properties.properties[i]->pucch_deltaF_Format2b[j]);
      printf( "\tmsg3_delta_Preamble for CC %d:\t%ld:\n",j,enb_properties.properties[i]->msg3_delta_Preamble[j]);
      printf( "\tul_CyclicPrefixLength for CC %d:\t%ld:\n",j,enb_properties.properties[i]->ul_CyclicPrefixLength[j]);

      // UE Timers and Constants

      printf( "\tue_TimersAndConstants_t300 for CC %d:\t%ld:\n",j,enb_properties.properties[i]->ue_TimersAndConstants_t300[j]);
      printf( "\tue_TimersAndConstants_t301 for CC %d:\t%ld:\n",j,enb_properties.properties[i]->ue_TimersAndConstants_t301[j]);
      printf( "\tue_TimersAndConstants_t310 for CC %d:\t%ld:\n",j,enb_properties.properties[i]->ue_TimersAndConstants_t310[j]);
      printf( "\tue_TimersAndConstants_n310 for CC %d:\t%ld:\n",j,enb_properties.properties[i]->ue_TimersAndConstants_n310[j]);
      printf( "\tue_TimersAndConstants_t311 for CC %d:\t%ld:\n",j,enb_properties.properties[i]->ue_TimersAndConstants_t311[j]);
      printf( "\tue_TimersAndConstants_n311 for CC %d:\t%ld:\n",j,enb_properties.properties[i]->ue_TimersAndConstants_n311[j]);

      printf( "\tue_TransmissionMode for CC %d:\t%ld:\n",j,enb_properties.properties[i]->ue_TransmissionMode[j]);

    }

    for (j=0; j < enb_properties.properties[i]->num_otg_elements; j++) {
      printf( "\n\tOTG Destination UE ID:  \t%"PRIu16, enb_properties.properties[i]->otg_ue_id[j]);
      printf( "\n\tOTG App Type:  \t%"PRIu8, enb_properties.properties[i]->otg_app_type[j]);
      printf( "\n\tOTG Background Traffic:  \t%s\n", (enb_properties.properties[i]->otg_bg_traffic[j]==1) ? "Enabled" : "Disabled");
    }

    printf( "\n\tGlobal log level:  \t%s\n", map_int_to_str(log_level_names,enb_properties.properties[i]->glog_level));
    printf( "\tHW log level:      \t%s\n", map_int_to_str(log_level_names,enb_properties.properties[i]->hw_log_level));
    printf( "\tPHY log level:     \t%s\n", map_int_to_str(log_level_names,enb_properties.properties[i]->phy_log_level));
    printf( "\tMAC log level:     \t%s\n", map_int_to_str(log_level_names,enb_properties.properties[i]->mac_log_level));
    printf( "\tRLC log level:     \t%s\n", map_int_to_str(log_level_names,enb_properties.properties[i]->rlc_log_level));
    printf( "\tPDCP log level:    \t%s\n", map_int_to_str(log_level_names,enb_properties.properties[i]->pdcp_log_level));
    printf( "\tRRC log level:     \t%s\n", map_int_to_str(log_level_names,enb_properties.properties[i]->rrc_log_level));
    printf( "\tUDP log level:     \t%s\n", map_int_to_str(log_level_names,enb_properties.properties[i]->udp_log_level));
    printf( "\tGTP log level:     \t%s\n", map_int_to_str(log_level_names,enb_properties.properties[i]->gtpu_log_level));
    printf( "\tOSA log level:     \t%s\n", map_int_to_str(log_level_names,enb_properties.properties[i]->osa_log_level));

    printf( "\n--------------------------------------------------------\n");
  }
}


static int enb_check_band_frequencies(char* lib_config_file_name_pP,
                                      int enb_properties_index,
                                      int16_t band,
                                      uint32_t downlink_frequency,
                                      int32_t uplink_frequency_offset,
                                      lte_frame_type_t frame_type)
{
  int errors = 0;

  if (band > 0) {
    int band_index;

    for (band_index = 0; band_index < sizeof (eutra_bands) / sizeof (eutra_bands[0]); band_index++) {
      if (band == eutra_bands[band_index].band) {
        uint32_t uplink_frequency = downlink_frequency + uplink_frequency_offset;

        AssertError (eutra_bands[band_index].dl_min < downlink_frequency, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d downlink frequency %u too low (%u) for band %d!",
                     lib_config_file_name_pP, enb_properties_index, downlink_frequency, eutra_bands[band_index].dl_min, band);
        AssertError (downlink_frequency < eutra_bands[band_index].dl_max, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d downlink frequency %u too high (%u) for band %d!",
                     lib_config_file_name_pP, enb_properties_index, downlink_frequency, eutra_bands[band_index].dl_max, band);

        AssertError (eutra_bands[band_index].ul_min < uplink_frequency, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d uplink frequency %u too low (%u) for band %d!",
                     lib_config_file_name_pP, enb_properties_index, uplink_frequency, eutra_bands[band_index].ul_min, band);
        AssertError (uplink_frequency < eutra_bands[band_index].ul_max, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d uplink frequency %u too high (%u) for band %d!",
                     lib_config_file_name_pP, enb_properties_index, uplink_frequency, eutra_bands[band_index].ul_max, band);

        AssertError (eutra_bands[band_index].frame_type == frame_type, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d invalid frame type (%d/%d) for band %d!",
                     lib_config_file_name_pP, enb_properties_index, eutra_bands[band_index].frame_type, frame_type, band);
      }
    }
  }

  return errors;
}

#if defined(ENABLE_ITTI) && defined(ENABLE_USE_MME)
extern int asn_debug;
extern int asn1_xer_print;
#endif

#ifdef LIBCONFIG_LONG
#define libconfig_int long
#else
#define libconfig_int int
#endif
const Enb_properties_array_t *enb_config_init(char* lib_config_file_name_pP)
{
  config_t          cfg;
  config_setting_t *setting                       = NULL;
  config_setting_t *subsetting                    = NULL;
  config_setting_t *setting_component_carriers    = NULL;
  config_setting_t *component_carrier             = NULL;
  config_setting_t *setting_srb1                  = NULL;
  config_setting_t *setting_mme_addresses         = NULL;
  config_setting_t *setting_mme_address           = NULL;
  config_setting_t *setting_rrh_gws               = NULL;
  config_setting_t *setting_rrh_gw                = NULL;
  config_setting_t *setting_enb                   = NULL;
  config_setting_t *setting_otg                   = NULL;
  config_setting_t *subsetting_otg                = NULL;
  int               num_enb_properties            = 0;
  int               enb_properties_index          = 0;
  int               num_enbs                      = 0;
  int               num_mme_address               = 0;
  int               num_rrh_gw                    = 0;
  int               num_otg_elements              = 0;
  int               num_component_carriers        = 0;
  int               i                             = 0;
  int               j                             = 0;
  libconfig_int     enb_id                        = 0;
  const char*       cell_type                     = NULL;
  const char*       tac                           = 0;
  const char*       enb_name                      = NULL;
  const char*       mcc                           = 0;
  const char*       mnc                           = 0;
  const char*       frame_type                    = NULL;
  libconfig_int     tdd_config                    = 0;
  libconfig_int     tdd_config_s                  = 0;
  const char*            prefix_type              = NULL;
  libconfig_int     eutra_band                    = 0;
  long long int     downlink_frequency            = 0;
  libconfig_int     uplink_frequency_offset       = 0;
  libconfig_int     Nid_cell                      = 0;
  libconfig_int     Nid_cell_mbsfn                = 0;
  libconfig_int     N_RB_DL                       = 0;
  libconfig_int     nb_antenna_ports              = 0;
  libconfig_int     nb_antennas_tx                = 0;
  libconfig_int     nb_antennas_rx                = 0;
  libconfig_int     tx_gain                       = 0;
  libconfig_int     rx_gain                       = 0;
  libconfig_int     prach_root                    = 0;
  libconfig_int     prach_config_index            = 0;
  const char*            prach_high_speed         = NULL;
  libconfig_int     prach_zero_correlation        = 0;
  libconfig_int     prach_freq_offset             = 0;
  libconfig_int     pucch_delta_shift             = 0;
  libconfig_int     pucch_nRB_CQI                 = 0;
  libconfig_int     pucch_nCS_AN                  = 0;
#ifndef Rel10
  libconfig_int     pucch_n1_AN                   = 0;
#endif
  libconfig_int     pdsch_referenceSignalPower    = 0;
  libconfig_int     pdsch_p_b                     = 0;
  libconfig_int     pusch_n_SB                    = 0;
  const char *      pusch_hoppingMode             = NULL;
  libconfig_int     pusch_hoppingOffset           = 0;
  const char*          pusch_enable64QAM          = NULL;
  const char*          pusch_groupHoppingEnabled  = NULL;
  libconfig_int     pusch_groupAssignment         = 0;
  const char*          pusch_sequenceHoppingEnabled = NULL;
  libconfig_int     pusch_nDMRS1                  = 0;
  const char*       phich_duration                = NULL;
  const char*       phich_resource                = NULL;
  const char*       srs_enable                    = NULL;
  libconfig_int     srs_BandwidthConfig           = 0;
  libconfig_int     srs_SubframeConfig            = 0;
  const char*       srs_ackNackST                 = NULL;
  const char*       srs_MaxUpPts                  = NULL;
  libconfig_int     pusch_p0_Nominal              = 0;
  const char*       pusch_alpha                   = NULL;
  libconfig_int     pucch_p0_Nominal              = 0;
  libconfig_int     msg3_delta_Preamble           = 0;
  //libconfig_int     ul_CyclicPrefixLength         = 0;
  const char*       pucch_deltaF_Format1          = NULL;
  //const char*       pucch_deltaF_Format1a         = NULL;
  const char*       pucch_deltaF_Format1b         = NULL;
  const char*       pucch_deltaF_Format2          = NULL;
  const char*       pucch_deltaF_Format2a         = NULL;
  const char*       pucch_deltaF_Format2b         = NULL;
  libconfig_int     rach_numberOfRA_Preambles     = 0;
  const char*       rach_preamblesGroupAConfig    = NULL;
  libconfig_int     rach_sizeOfRA_PreamblesGroupA = 0;
  libconfig_int     rach_messageSizeGroupA        = 0;
  const char*       rach_messagePowerOffsetGroupB = NULL;
  libconfig_int     rach_powerRampingStep         = 0;
  libconfig_int     rach_preambleInitialReceivedTargetPower    = 0;
  libconfig_int     rach_preambleTransMax         = 0;
  libconfig_int     rach_raResponseWindowSize     = 0;
  libconfig_int     rach_macContentionResolutionTimer = 0;
  libconfig_int     rach_maxHARQ_Msg3Tx           = 0;
  libconfig_int     pcch_defaultPagingCycle       = 0;
  const char*       pcch_nB                       = NULL;
  libconfig_int     bcch_modificationPeriodCoeff  = 0;
  libconfig_int     ue_TimersAndConstants_t300    = 0;
  libconfig_int     ue_TimersAndConstants_t301    = 0;
  libconfig_int     ue_TimersAndConstants_t310    = 0;
  libconfig_int     ue_TimersAndConstants_t311    = 0;
  libconfig_int     ue_TimersAndConstants_n310    = 0;
  libconfig_int     ue_TimersAndConstants_n311    = 0;
  libconfig_int     ue_TransmissionMode           = 0;


  libconfig_int     srb1_timer_poll_retransmit    = 0;
  libconfig_int     srb1_timer_reordering         = 0;
  libconfig_int     srb1_timer_status_prohibit    = 0;
  libconfig_int     srb1_poll_pdu                 = 0;
  libconfig_int     srb1_poll_byte                = 0;
  libconfig_int     srb1_max_retx_threshold       = 0;

  libconfig_int     my_int;


  char*             if_name                       = NULL;
  char*             ipv4                          = NULL;
  char*             ipv4_remote                   = NULL;
  char*             ipv6                          = NULL;
  char*             active                        = NULL;
  char*             preference                    = NULL;

  char*             tr_preference                 = NULL;
  char*             rf_preference                 = NULL;
  libconfig_int     tx_scheduling_advance         = 0;
  libconfig_int     tx_sample_advance             = 0;
  libconfig_int     iq_txshift                    = 0;
  libconfig_int     local_port                    = 0;
  libconfig_int     remote_port                   = 0;
  const char*       active_enb[MAX_ENB];
  char*             enb_interface_name_for_S1U    = NULL;
  char*             enb_ipv4_address_for_S1U      = NULL;
  libconfig_int     enb_port_for_S1U              = 0;
  char*             enb_interface_name_for_S1_MME = NULL;
  char*             enb_ipv4_address_for_S1_MME   = NULL;
  char             *address                       = NULL;
  char             *cidr                          = NULL;
  char             *astring                       = NULL;
  libconfig_int     otg_ue_id                     = 0;
  char*             otg_app_type                  = NULL;
  char*             otg_bg_traffic                = NULL;
  char*             glog_level                    = NULL;
  char*             glog_verbosity                = NULL;
  char*             hw_log_level                  = NULL;
  char*             hw_log_verbosity              = NULL;
  char*             phy_log_level                 = NULL;
  char*             phy_log_verbosity             = NULL;
  char*             mac_log_level                 = NULL;
  char*             mac_log_verbosity             = NULL;
  char*             rlc_log_level                 = NULL;
  char*             rlc_log_verbosity             = NULL;
  char*             pdcp_log_level                = NULL;
  char*             pdcp_log_verbosity            = NULL;
  char*             rrc_log_level                 = NULL;
  char*             rrc_log_verbosity             = NULL;
  char*             gtpu_log_level                = NULL;
  char*             gtpu_log_verbosity            = NULL;
  char*             udp_log_level                 = NULL;
  char*             udp_log_verbosity             = NULL;
  char*             osa_log_level                 = NULL;
  char*             osa_log_verbosity             = NULL;

  /* for no gcc warnings */
  (void)astring;
  (void)my_int;

  memset((char*) (enb_properties.properties), 0 , MAX_ENB * sizeof(Enb_properties_t *));
  memset((char*)active_enb,     0 , MAX_ENB * sizeof(char*));

  config_init(&cfg);

  if (lib_config_file_name_pP != NULL) {
    /* Read the file. If there is an error, report it and exit. */
    if (! config_read_file(&cfg, lib_config_file_name_pP)) {
      config_destroy(&cfg);
      AssertFatal (0, "Failed to parse eNB configuration file %s!\n", lib_config_file_name_pP);
    }
  } else {
    config_destroy(&cfg);
    AssertFatal (0, "No eNB configuration file provided!\n");
  }

#if defined(ENABLE_ITTI) && defined(ENABLE_USE_MME)

  if (  (config_lookup_string( &cfg, ENB_CONFIG_STRING_ASN1_VERBOSITY, (const char **)&astring) )) {
    if (strcasecmp(astring , ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE) == 0) {
      asn_debug      = 0;
      asn1_xer_print = 0;
    } else if (strcasecmp(astring , ENB_CONFIG_STRING_ASN1_VERBOSITY_INFO) == 0) {
      asn_debug      = 1;
      asn1_xer_print = 1;
    } else if (strcasecmp(astring , ENB_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING) == 0) {
      asn_debug      = 1;
      asn1_xer_print = 2;
    } else {
      asn_debug      = 0;
      asn1_xer_print = 0;
    }
  }

#endif
  // Get list of active eNBs, (only these will be configured)
  setting = config_lookup(&cfg, ENB_CONFIG_STRING_ACTIVE_ENBS);

  if (setting != NULL) {
    num_enbs = config_setting_length(setting);

    for (i = 0; i < num_enbs; i++) {
      setting_enb   = config_setting_get_elem(setting, i);
      active_enb[i] = config_setting_get_string (setting_enb);
      AssertFatal (active_enb[i] != NULL,
                   "Failed to parse config file %s, %uth attribute %s \n",
                   lib_config_file_name_pP, i, ENB_CONFIG_STRING_ACTIVE_ENBS);
      active_enb[i] = strdup(active_enb[i]);
      num_enb_properties += 1;
    }
  }

  /* Output a list of all eNBs. */
  setting = config_lookup(&cfg, ENB_CONFIG_STRING_ENB_LIST);

  if (setting != NULL) {
    enb_properties_index = 0;
    num_enbs = config_setting_length(setting);

    for (i = 0; i < num_enbs; i++) {
      setting_enb = config_setting_get_elem(setting, i);

      if (! config_setting_lookup_int(setting_enb, ENB_CONFIG_STRING_ENB_ID, &enb_id)) {
        /* Calculate a default eNB ID */
# if defined(ENABLE_USE_MME)
        uint32_t hash;

        hash = s1ap_generate_eNB_id ();
        enb_id = i + (hash & 0xFFFF8);
# else
        enb_id = i;
# endif
      }

      if (  !(       config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_CELL_TYPE,           &cell_type)
                    && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_ENB_NAME,            &enb_name)
                    && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_TRACKING_AREA_CODE,  &tac)
                    && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_MOBILE_COUNTRY_CODE, &mcc)
                    && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_MOBILE_NETWORK_CODE, &mnc)


            )
        ) {
        AssertFatal (0,
                     "Failed to parse eNB configuration file %s, %u th enb\n",
                     lib_config_file_name_pP, i);
        continue; // FIXME this prevents segfaults below, not sure what happens after function exit
      }

      // search if in active list
      for (j=0; j < num_enb_properties; j++) {
        if (strcmp(active_enb[j], enb_name) == 0) {
          enb_properties.properties[enb_properties_index] = calloc(1, sizeof(Enb_properties_t));

          enb_properties.properties[enb_properties_index]->eNB_id   = enb_id;

          if (strcmp(cell_type, "CELL_MACRO_ENB") == 0) {
            enb_properties.properties[enb_properties_index]->cell_type = CELL_MACRO_ENB;
          } else  if (strcmp(cell_type, "CELL_HOME_ENB") == 0) {
            enb_properties.properties[enb_properties_index]->cell_type = CELL_HOME_ENB;
          } else {
            AssertFatal (0,
                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
                         lib_config_file_name_pP, i, cell_type);
          }

          enb_properties.properties[enb_properties_index]->eNB_name         = strdup(enb_name);
          enb_properties.properties[enb_properties_index]->tac              = (uint16_t)atoi(tac);
          enb_properties.properties[enb_properties_index]->mcc              = (uint16_t)atoi(mcc);
          enb_properties.properties[enb_properties_index]->mnc              = (uint16_t)atoi(mnc);
          enb_properties.properties[enb_properties_index]->mnc_digit_length = strlen(mnc);
          AssertFatal((enb_properties.properties[enb_properties_index]->mnc_digit_length == 2) ||
                      (enb_properties.properties[enb_properties_index]->mnc_digit_length == 3),
                      "BAD MNC DIGIT LENGTH %d",
                      enb_properties.properties[i]->mnc_digit_length);


          // Parse optional physical parameters


          setting_component_carriers = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_COMPONENT_CARRIERS);
          enb_properties.properties[enb_properties_index]->nb_cc = 0;

          if (setting_component_carriers != NULL) {

            num_component_carriers     = config_setting_length(setting_component_carriers);
            printf("num component carrier %d \n", num_component_carriers);

            //enb_properties.properties[enb_properties_index]->nb_cc = num_component_carriers;
            for (j = 0; j < num_component_carriers /*&& j < MAX_NUM_CCs*/; j++) {
              component_carrier = config_setting_get_elem(setting_component_carriers, j);

              //printf("Component carrier %d\n",component_carrier);
              if (!(config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_FRAME_TYPE, &frame_type)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_TDD_CONFIG, &tdd_config)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_TDD_CONFIG_S, &tdd_config_s)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PREFIX_TYPE, &prefix_type)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_EUTRA_BAND, &eutra_band)
                   && config_setting_lookup_int64(component_carrier, ENB_CONFIG_STRING_DOWNLINK_FREQUENCY, &downlink_frequency)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UPLINK_FREQUENCY_OFFSET, &uplink_frequency_offset)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_NID_CELL, &Nid_cell)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_N_RB_DL, &N_RB_DL)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_CELL_MBSFN, &Nid_cell_mbsfn)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_NB_ANT_PORTS, &nb_antenna_ports)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_NB_ANT_TX, &nb_antennas_tx)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_NB_ANT_RX, &nb_antennas_rx)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_TX_GAIN, &tx_gain)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RX_GAIN, &rx_gain)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PRACH_ROOT, &prach_root)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PRACH_CONFIG_INDEX, &prach_config_index)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PRACH_HIGH_SPEED, &prach_high_speed)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PRACH_ZERO_CORRELATION, &prach_zero_correlation)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PRACH_FREQ_OFFSET, &prach_freq_offset)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUCCH_DELTA_SHIFT, &pucch_delta_shift)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUCCH_NRB_CQI, &pucch_nRB_CQI)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUCCH_NCS_AN, &pucch_nCS_AN)
#ifndef Rel10
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUCCH_N1_AN, &pucch_n1_AN)
#endif
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PDSCH_RS_EPRE, &pdsch_referenceSignalPower)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PDSCH_PB, &pdsch_p_b)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUSCH_N_SB, &pusch_n_SB)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUSCH_HOPPINGMODE, &pusch_hoppingMode)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUSCH_HOPPINGOFFSET, &pusch_hoppingOffset)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUSCH_ENABLE64QAM, &pusch_enable64QAM)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN, &pusch_groupHoppingEnabled)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUSCH_GROUP_ASSIGNMENT, &pusch_groupAssignment)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN, &pusch_sequenceHoppingEnabled)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUSCH_NDMRS1, &pusch_nDMRS1)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PHICH_DURATION, &phich_duration)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PHICH_RESOURCE, &phich_resource)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_SRS_ENABLE, &srs_enable)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUSCH_PO_NOMINAL, &pusch_p0_Nominal)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUSCH_ALPHA, &pusch_alpha)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUCCH_PO_NOMINAL, &pucch_p0_Nominal)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_MSG3_DELTA_PREAMBLE, &msg3_delta_Preamble)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1, &pucch_deltaF_Format1)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1b, &pucch_deltaF_Format1b)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2, &pucch_deltaF_Format2)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2A, &pucch_deltaF_Format2a)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2B, &pucch_deltaF_Format2b)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_NUM_RA_PREAMBLES, &rach_numberOfRA_Preambles)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_RACH_PREAMBLESGROUPACONFIG, &rach_preamblesGroupAConfig)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_POWERRAMPINGSTEP, &rach_powerRampingStep)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER, &rach_preambleInitialReceivedTargetPower)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_PREAMBLETRANSMAX, &rach_preambleTransMax)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_RARESPONSEWINDOWSIZE, &rach_raResponseWindowSize)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_MACCONTENTIONRESOLUTIONTIMER, &rach_macContentionResolutionTimer)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_MAXHARQMSG3TX, &rach_maxHARQ_Msg3Tx)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_MAXHARQMSG3TX, &bcch_modificationPeriodCoeff)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PCCH_DEFAULT_PAGING_CYCLE,  &pcch_defaultPagingCycle)
                   && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PCCH_NB,  &pcch_nB)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_BCCH_MODIFICATIONPERIODCOEFF,  &bcch_modificationPeriodCoeff)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UETIMERS_T300,  &ue_TimersAndConstants_t300)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UETIMERS_T301,  &ue_TimersAndConstants_t301)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UETIMERS_T310,  &ue_TimersAndConstants_t310)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UETIMERS_T311,  &ue_TimersAndConstants_t311)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UETIMERS_N310,  &ue_TimersAndConstants_n310)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UETIMERS_N311,  &ue_TimersAndConstants_n311)
                   && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UE_TRANSMISSION_MODE,  &ue_TransmissionMode)

#ifdef Rel10

#endif
                  )) {
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, Component Carrier %d!\n",
                             lib_config_file_name_pP, enb_properties.properties[enb_properties_index]->nb_cc++);
                continue; // FIXME this prevents segfaults below, not sure what happens after function exit
              }

              enb_properties.properties[enb_properties_index]->nb_cc++;

              enb_properties.properties[enb_properties_index]->tdd_config[j] = tdd_config;
              AssertFatal (tdd_config <= TDD_Config__subframeAssignment_sa6,
                           "Failed to parse eNB configuration file %s, enb %d illegal tdd_config %d (should be 0-%d)!",
                           lib_config_file_name_pP, i, tdd_config, TDD_Config__subframeAssignment_sa6);

              enb_properties.properties[enb_properties_index]->tdd_config_s[j] = tdd_config_s;
              AssertFatal (tdd_config_s <= TDD_Config__specialSubframePatterns_ssp8,
                           "Failed to parse eNB configuration file %s, enb %d illegal tdd_config_s %d (should be 0-%d)!",
                           lib_config_file_name_pP, i, tdd_config_s, TDD_Config__specialSubframePatterns_ssp8);

              if (!prefix_type)
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d define %s: NORMAL,EXTENDED!\n",
                             lib_config_file_name_pP, i, ENB_CONFIG_STRING_PREFIX_TYPE);
              else if (strcmp(prefix_type, "NORMAL") == 0) {
                enb_properties.properties[enb_properties_index]->prefix_type[j] = NORMAL;
              } else  if (strcmp(prefix_type, "EXTENDED") == 0) {
                enb_properties.properties[enb_properties_index]->prefix_type[j] = EXTENDED;
              } else {
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prefix_type choice: NORMAL or EXTENDED !\n",
                             lib_config_file_name_pP, i, prefix_type);
              }

              enb_properties.properties[enb_properties_index]->eutra_band[j] = eutra_band;
              enb_properties.properties[enb_properties_index]->downlink_frequency[j] = (uint32_t) downlink_frequency;
              enb_properties.properties[enb_properties_index]->uplink_frequency_offset[j] = (unsigned int) uplink_frequency_offset;
              enb_properties.properties[enb_properties_index]->Nid_cell[j]= Nid_cell;

              if (Nid_cell>503) {
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for Nid_cell choice: 0...503 !\n",
                             lib_config_file_name_pP, i, Nid_cell);
              }

              enb_properties.properties[enb_properties_index]->N_RB_DL[j]= N_RB_DL;

              if ((N_RB_DL!=6) && (N_RB_DL!=15) && (N_RB_DL!=25) && (N_RB_DL!=50) && (N_RB_DL!=75) && (N_RB_DL!=100)) {
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for N_RB_DL choice: 6,15,25,50,75,100 !\n",
                             lib_config_file_name_pP, i, N_RB_DL);
              }

              if (strcmp(frame_type, "FDD") == 0) {
                enb_properties.properties[enb_properties_index]->frame_type[j] = FDD;
              } else  if (strcmp(frame_type, "TDD") == 0) {
                enb_properties.properties[enb_properties_index]->frame_type[j] = TDD;
              } else {
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for frame_type choice: FDD or TDD !\n",
                             lib_config_file_name_pP, i, frame_type);
              }


              enb_properties.properties[enb_properties_index]->tdd_config[j] = tdd_config;
              AssertFatal (tdd_config <= TDD_Config__subframeAssignment_sa6,
                           "Failed to parse eNB configuration file %s, enb %d illegal tdd_config %d (should be 0-%d)!",
                           lib_config_file_name_pP, i, tdd_config, TDD_Config__subframeAssignment_sa6);


              enb_properties.properties[enb_properties_index]->tdd_config_s[j] = tdd_config_s;
              AssertFatal (tdd_config_s <= TDD_Config__specialSubframePatterns_ssp8,
                           "Failed to parse eNB configuration file %s, enb %d illegal tdd_config_s %d (should be 0-%d)!",
                           lib_config_file_name_pP, i, tdd_config_s, TDD_Config__specialSubframePatterns_ssp8);



              if (!prefix_type)
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d define %s: NORMAL,EXTENDED!\n",
                             lib_config_file_name_pP, i, ENB_CONFIG_STRING_PREFIX_TYPE);
              else if (strcmp(prefix_type, "NORMAL") == 0) {
                enb_properties.properties[enb_properties_index]->prefix_type[j] = NORMAL;
              } else  if (strcmp(prefix_type, "EXTENDED") == 0) {
                enb_properties.properties[enb_properties_index]->prefix_type[j] = EXTENDED;
              } else {
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prefix_type choice: NORMAL or EXTENDED !\n",
                             lib_config_file_name_pP, i, prefix_type);
              }



              enb_properties.properties[enb_properties_index]->eutra_band[j] = eutra_band;
              // printf( "\teutra band:\t%d\n",enb_properties.properties[enb_properties_index]->eutra_band);



              enb_properties.properties[enb_properties_index]->downlink_frequency[j] = (uint32_t) downlink_frequency;
              //printf( "\tdownlink freq:\t%u\n",enb_properties.properties[enb_properties_index]->downlink_frequency);


              enb_properties.properties[enb_properties_index]->uplink_frequency_offset[j] = (unsigned int) uplink_frequency_offset;

              if (enb_check_band_frequencies(lib_config_file_name_pP,
                              enb_properties_index,
                              enb_properties.properties[enb_properties_index]->eutra_band[j],
                              enb_properties.properties[enb_properties_index]->downlink_frequency[j],
                              enb_properties.properties[enb_properties_index]->uplink_frequency_offset[j],
                              enb_properties.properties[enb_properties_index]->frame_type[j])) {
                AssertFatal(0, "error calling enb_check_band_frequencies\n");
              }

              enb_properties.properties[enb_properties_index]->nb_antennas_tx[j] = nb_antennas_tx;

              if ((nb_antenna_ports <1) || (nb_antenna_ports > 2))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for nb_antenna_ports choice: 1..2 !\n",
                             lib_config_file_name_pP, i, nb_antenna_ports);

              enb_properties.properties[enb_properties_index]->nb_antenna_ports[j] = nb_antenna_ports;

              if ((nb_antennas_tx <1) || (nb_antennas_tx > 64))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for nb_antennas_tx choice: 1..4 !\n",
                             lib_config_file_name_pP, i, nb_antennas_tx);

              enb_properties.properties[enb_properties_index]->nb_antennas_rx[j] = nb_antennas_rx;

              if ((nb_antennas_rx <1) || (nb_antennas_rx > 4))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for nb_antennas_rx choice: 1..4 !\n",
                             lib_config_file_name_pP, i, nb_antennas_rx);

              enb_properties.properties[enb_properties_index]->tx_gain[j] = tx_gain;

              if ((tx_gain <0) || (tx_gain > 127))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for tx_gain choice: 0..127 !\n",
                             lib_config_file_name_pP, i, tx_gain);

              enb_properties.properties[enb_properties_index]->rx_gain[j] = rx_gain;

              if ((rx_gain <0) || (rx_gain > 160))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rx_gain choice: 0..160 !\n",
                             lib_config_file_name_pP, i, rx_gain);

              enb_properties.properties[enb_properties_index]->prach_root[j] =  prach_root;

              if ((prach_root <0) || (prach_root > 1023))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_root choice: 0..1023 !\n",
                             lib_config_file_name_pP, i, prach_root);

              enb_properties.properties[enb_properties_index]->prach_config_index[j] = prach_config_index;

              if ((prach_config_index <0) || (prach_config_index > 63))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_config_index choice: 0..1023 !\n",
                             lib_config_file_name_pP, i, prach_config_index);

              if (!prach_high_speed)
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                             lib_config_file_name_pP, i, ENB_CONFIG_STRING_PRACH_HIGH_SPEED);
              else if (strcmp(prach_high_speed, "ENABLE") == 0) {
                enb_properties.properties[enb_properties_index]->prach_high_speed[j] = TRUE;
              } else if (strcmp(prach_high_speed, "DISABLE") == 0) {
                enb_properties.properties[enb_properties_index]->prach_high_speed[j] = FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prach_config choice: ENABLE,DISABLE !\n",
                             lib_config_file_name_pP, i, prach_high_speed);

              enb_properties.properties[enb_properties_index]->prach_zero_correlation[j] =prach_zero_correlation;

              if ((prach_zero_correlation <0) || (prach_zero_correlation > 15))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_zero_correlation choice: 0..15!\n",
                             lib_config_file_name_pP, i, prach_zero_correlation);

              enb_properties.properties[enb_properties_index]->prach_freq_offset[j] = prach_freq_offset;

              if ((prach_freq_offset <0) || (prach_freq_offset > 94))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_freq_offset choice: 0..94!\n",
                             lib_config_file_name_pP, i, prach_freq_offset);


              enb_properties.properties[enb_properties_index]->pucch_delta_shift[j] = pucch_delta_shift-1;

              if ((pucch_delta_shift <1) || (pucch_delta_shift > 3))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_delta_shift choice: 1..3!\n",
                             lib_config_file_name_pP, i, pucch_delta_shift);

              enb_properties.properties[enb_properties_index]->pucch_nRB_CQI[j] = pucch_nRB_CQI;

              if ((pucch_nRB_CQI <0) || (pucch_nRB_CQI > 98))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nRB_CQI choice: 0..98!\n",
                             lib_config_file_name_pP, i, pucch_nRB_CQI);

              enb_properties.properties[enb_properties_index]->pucch_nCS_AN[j] = pucch_nCS_AN;

              if ((pucch_nCS_AN <0) || (pucch_nCS_AN > 7))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nCS_AN choice: 0..7!\n",
                             lib_config_file_name_pP, i, pucch_nCS_AN);

#ifndef Rel10
              enb_properties.properties[enb_properties_index]->pucch_n1_AN[j] = pucch_n1_AN;

              if ((pucch_n1_AN <0) || (pucch_n1_AN > 2047))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_n1_AN choice: 0..2047!\n",
                             lib_config_file_name_pP, i, pucch_n1_AN);

#endif
              enb_properties.properties[enb_properties_index]->pdsch_referenceSignalPower[j] = pdsch_referenceSignalPower;

              if ((pdsch_referenceSignalPower <-60) || (pdsch_referenceSignalPower > 50))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_referenceSignalPower choice:-60..50!\n",
                             lib_config_file_name_pP, i, pdsch_referenceSignalPower);

              enb_properties.properties[enb_properties_index]->pdsch_p_b[j] = pdsch_p_b;

              if ((pdsch_p_b <0) || (pdsch_p_b > 3))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_p_b choice: 0..3!\n",
                             lib_config_file_name_pP, i, pdsch_p_b);

              enb_properties.properties[enb_properties_index]->pusch_n_SB[j] = pusch_n_SB;

              if ((pusch_n_SB <1) || (pusch_n_SB > 4))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_n_SB choice: 1..4!\n",
                             lib_config_file_name_pP, i, pusch_n_SB);

              if (!pusch_hoppingMode)
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d define %s: interSubframe,intraAndInterSubframe!\n",
                             lib_config_file_name_pP, i, ENB_CONFIG_STRING_PUSCH_HOPPINGMODE);
              else if (strcmp(pusch_hoppingMode,"interSubFrame")==0) {
                enb_properties.properties[enb_properties_index]->pusch_hoppingMode[j] = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_interSubFrame;
              }  else if (strcmp(pusch_hoppingMode,"intraAndInterSubFrame")==0) {
                enb_properties.properties[enb_properties_index]->pusch_hoppingMode[j] = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_intraAndInterSubFrame;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingMode choice: interSubframe,intraAndInterSubframe!\n",
                             lib_config_file_name_pP, i, pusch_hoppingMode);

              enb_properties.properties[enb_properties_index]->pusch_hoppingOffset[j] = pusch_hoppingOffset;

              if ((pusch_hoppingOffset<0) || (pusch_hoppingOffset>98))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingOffset choice: 0..98!\n",
                             lib_config_file_name_pP, i, pusch_hoppingMode);

              if (!pusch_enable64QAM)
                AssertFatal (0, 
                             "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                             lib_config_file_name_pP, i, ENB_CONFIG_STRING_PUSCH_ENABLE64QAM);
              else if (strcmp(pusch_enable64QAM, "ENABLE") == 0) {
                enb_properties.properties[enb_properties_index]->pusch_enable64QAM[j] = TRUE;
              }  else if (strcmp(pusch_enable64QAM, "DISABLE") == 0) {
                enb_properties.properties[enb_properties_index]->pusch_enable64QAM[j] = FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_enable64QAM choice: ENABLE,DISABLE!\n",
                             lib_config_file_name_pP, i, pusch_enable64QAM);

              if (!pusch_groupHoppingEnabled)
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                             lib_config_file_name_pP, i, ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN);
              else if (strcmp(pusch_groupHoppingEnabled, "ENABLE") == 0) {
                enb_properties.properties[enb_properties_index]->pusch_groupHoppingEnabled[j] = TRUE;
              }  else if (strcmp(pusch_groupHoppingEnabled, "DISABLE") == 0) {
                enb_properties.properties[enb_properties_index]->pusch_groupHoppingEnabled[j] = FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_groupHoppingEnabled choice: ENABLE,DISABLE!\n",
                             lib_config_file_name_pP, i, pusch_groupHoppingEnabled);


              enb_properties.properties[enb_properties_index]->pusch_groupAssignment[j] = pusch_groupAssignment;

              if ((pusch_groupAssignment<0)||(pusch_groupAssignment>29))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_groupAssignment choice: 0..29!\n",
                             lib_config_file_name_pP, i, pusch_groupAssignment);

              if (!pusch_sequenceHoppingEnabled)
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                             lib_config_file_name_pP, i, ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN);
              else if (strcmp(pusch_sequenceHoppingEnabled, "ENABLE") == 0) {
                enb_properties.properties[enb_properties_index]->pusch_sequenceHoppingEnabled[j] = TRUE;
              }  else if (strcmp(pusch_sequenceHoppingEnabled, "DISABLE") == 0) {
                enb_properties.properties[enb_properties_index]->pusch_sequenceHoppingEnabled[j] = FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_sequenceHoppingEnabled choice: ENABLE,DISABLE!\n",
                             lib_config_file_name_pP, i, pusch_sequenceHoppingEnabled);

              enb_properties.properties[enb_properties_index]->pusch_nDMRS1[j] = pusch_nDMRS1;  //cyclic_shift in RRC!

              if ((pusch_nDMRS1 <0) || (pusch_nDMRS1>7))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_nDMRS1 choice: 0..7!\n",
                             lib_config_file_name_pP, i, pusch_nDMRS1);

              if (strcmp(phich_duration,"NORMAL")==0) {
                enb_properties.properties[enb_properties_index]->phich_duration[j] = normal;
              } else if (strcmp(phich_duration,"EXTENDED")==0) {
                enb_properties.properties[enb_properties_index]->phich_duration[j] = extended;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_duration choice: NORMAL,EXTENDED!\n",
                             lib_config_file_name_pP, i, phich_duration);

              if (strcmp(phich_resource,"ONESIXTH")==0) {
                enb_properties.properties[enb_properties_index]->phich_resource[j] = oneSixth;
              } else if (strcmp(phich_resource,"HALF")==0) {
                enb_properties.properties[enb_properties_index]->phich_resource[j] = half;
              } else if (strcmp(phich_resource,"ONE")==0) {
                enb_properties.properties[enb_properties_index]->phich_resource[j] = one;
              } else if (strcmp(phich_resource,"TWO")==0) {
                enb_properties.properties[enb_properties_index]->phich_resource[j] = two;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_resource choice: ONESIXTH,HALF,ONE,TWO!\n",
                             lib_config_file_name_pP, i, phich_resource);

              if (strcmp(srs_enable, "ENABLE") == 0) {
                enb_properties.properties[enb_properties_index]->srs_enable[j] = TRUE;
              } else if (strcmp(srs_enable, "DISABLE") == 0) {
                enb_properties.properties[enb_properties_index]->srs_enable[j] = FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                             lib_config_file_name_pP, i, srs_enable);

              if (enb_properties.properties[enb_properties_index]->srs_enable[j] == TRUE) {
                if (!(config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_SRS_BANDWIDTH_CONFIG, &srs_BandwidthConfig)
                      && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_SRS_SUBFRAME_CONFIG, &srs_SubframeConfig)
                      && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_SRS_ACKNACKST_CONFIG, &srs_ackNackST)
                      && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_SRS_MAXUPPTS, &srs_MaxUpPts)
                     ))
                  AssertFatal(0,
                              "Failed to parse eNB configuration file %s, enb %d unknown values for srs_BandwidthConfig, srs_SubframeConfig, srs_ackNackST, srs_MaxUpPts\n",
                              lib_config_file_name_pP, i);

                enb_properties.properties[enb_properties_index]->srs_BandwidthConfig[j] = srs_BandwidthConfig;

                if ((srs_BandwidthConfig < 0) || (srs_BandwidthConfig >7))
                  AssertFatal (0, "Failed to parse eNB configuration file %s, enb %d unknown value %d for srs_BandwidthConfig choice: 0...7\n",
                               lib_config_file_name_pP, i, srs_BandwidthConfig);

                enb_properties.properties[enb_properties_index]->srs_SubframeConfig[j] = srs_SubframeConfig;

                if ((srs_SubframeConfig<0) || (srs_SubframeConfig>15))
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for srs_SubframeConfig choice: 0..15 !\n",
                               lib_config_file_name_pP, i, srs_SubframeConfig);

                if (strcmp(srs_ackNackST, "ENABLE") == 0) {
                  enb_properties.properties[enb_properties_index]->srs_ackNackST[j] = TRUE;
                } else if (strcmp(srs_ackNackST, "DISABLE") == 0) {
                  enb_properties.properties[enb_properties_index]->srs_ackNackST[j] = FALSE;
                } else
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                               lib_config_file_name_pP, i, srs_ackNackST);

                if (strcmp(srs_MaxUpPts, "ENABLE") == 0) {
                  enb_properties.properties[enb_properties_index]->srs_MaxUpPts[j] = TRUE;
                } else if (strcmp(srs_MaxUpPts, "DISABLE") == 0) {
                  enb_properties.properties[enb_properties_index]->srs_MaxUpPts[j] = FALSE;
                } else
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_MaxUpPts choice: ENABLE,DISABLE !\n",
                               lib_config_file_name_pP, i, srs_MaxUpPts);
              }

              enb_properties.properties[enb_properties_index]->pusch_p0_Nominal[j] = pusch_p0_Nominal;

              if ((pusch_p0_Nominal<-126) || (pusch_p0_Nominal>24))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_p0_Nominal choice: -126..24 !\n",
                             lib_config_file_name_pP, i, pusch_p0_Nominal);

              if (strcmp(pusch_alpha,"AL0")==0) {
                enb_properties.properties[enb_properties_index]->pusch_alpha[j] = UplinkPowerControlCommon__alpha_al0;
              } else if (strcmp(pusch_alpha,"AL04")==0) {
                enb_properties.properties[enb_properties_index]->pusch_alpha[j] = UplinkPowerControlCommon__alpha_al04;
              } else if (strcmp(pusch_alpha,"AL05")==0) {
                enb_properties.properties[enb_properties_index]->pusch_alpha[j] = UplinkPowerControlCommon__alpha_al05;
              } else if (strcmp(pusch_alpha,"AL06")==0) {
                enb_properties.properties[enb_properties_index]->pusch_alpha[j] = UplinkPowerControlCommon__alpha_al06;
              } else if (strcmp(pusch_alpha,"AL07")==0) {
                enb_properties.properties[enb_properties_index]->pusch_alpha[j] = UplinkPowerControlCommon__alpha_al07;
              } else if (strcmp(pusch_alpha,"AL08")==0) {
                enb_properties.properties[enb_properties_index]->pusch_alpha[j] = UplinkPowerControlCommon__alpha_al08;
              } else if (strcmp(pusch_alpha,"AL09")==0) {
                enb_properties.properties[enb_properties_index]->pusch_alpha[j] = UplinkPowerControlCommon__alpha_al09;
              } else if (strcmp(pusch_alpha,"AL1")==0) {
                enb_properties.properties[enb_properties_index]->pusch_alpha[j] = UplinkPowerControlCommon__alpha_al1;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_Alpha choice: AL0,AL04,AL05,AL06,AL07,AL08,AL09,AL1!\n",
                             lib_config_file_name_pP, i, pusch_alpha);

              enb_properties.properties[enb_properties_index]->pucch_p0_Nominal[j] = pucch_p0_Nominal;

              if ((pucch_p0_Nominal<-127) || (pucch_p0_Nominal>-96))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_p0_Nominal choice: -127..-96 !\n",
                             lib_config_file_name_pP, i, pucch_p0_Nominal);

              enb_properties.properties[enb_properties_index]->msg3_delta_Preamble[j] = msg3_delta_Preamble;

              if ((msg3_delta_Preamble<-1) || (msg3_delta_Preamble>6))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for msg3_delta_Preamble choice: -1..6 !\n",
                             lib_config_file_name_pP, i, msg3_delta_Preamble);


              if (strcmp(pucch_deltaF_Format1,"deltaF_2")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format1[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF_2;
              } else if (strcmp(pucch_deltaF_Format1,"deltaF0")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format1[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF0;
              } else if (strcmp(pucch_deltaF_Format1,"deltaF2")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format1[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF2;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1 choice: deltaF_2,dltaF0,deltaF2!\n",
                             lib_config_file_name_pP, i, pucch_deltaF_Format1);

              if (strcmp(pucch_deltaF_Format1b,"deltaF1")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format1b[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF1;
              } else if (strcmp(pucch_deltaF_Format1b,"deltaF3")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format1b[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF3;
              } else if (strcmp(pucch_deltaF_Format1b,"deltaF5")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format1b[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF5;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1b choice: deltaF1,dltaF3,deltaF5!\n",
                             lib_config_file_name_pP, i, pucch_deltaF_Format1b);


              if (strcmp(pucch_deltaF_Format2,"deltaF_2")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format2[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF_2;
              } else if (strcmp(pucch_deltaF_Format2,"deltaF0")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format2[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF0;
              } else if (strcmp(pucch_deltaF_Format2,"deltaF1")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format2[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF1;
              } else if (strcmp(pucch_deltaF_Format2,"deltaF2")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format2[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF2;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2 choice: deltaF_2,dltaF0,deltaF1,deltaF2!\n",
                             lib_config_file_name_pP, i, pucch_deltaF_Format2);

              if (strcmp(pucch_deltaF_Format2a,"deltaF_2")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format2a[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF_2;
              } else if (strcmp(pucch_deltaF_Format2a,"deltaF0")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format2a[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF0;
              } else if (strcmp(pucch_deltaF_Format2a,"deltaF2")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format2a[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF2;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2a choice: deltaF_2,dltaF0,deltaF2!\n",
                             lib_config_file_name_pP, i, pucch_deltaF_Format2a);

              if (strcmp(pucch_deltaF_Format2b,"deltaF_2")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format2b[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF_2;
              } else if (strcmp(pucch_deltaF_Format2b,"deltaF0")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format2b[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF0;
              } else if (strcmp(pucch_deltaF_Format2b,"deltaF2")==0) {
                enb_properties.properties[enb_properties_index]->pucch_deltaF_Format2b[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF2;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2b choice: deltaF_2,dltaF0,deltaF2!\n",
                             lib_config_file_name_pP, i, pucch_deltaF_Format2b);




              enb_properties.properties[enb_properties_index]->rach_numberOfRA_Preambles[j] = (rach_numberOfRA_Preambles/4)-1;

              if ((rach_numberOfRA_Preambles <4) || (rach_numberOfRA_Preambles>64) || ((rach_numberOfRA_Preambles&3)!=0))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_numberOfRA_Preambles choice: 4,8,12,...,64!\n",
                             lib_config_file_name_pP, i, rach_numberOfRA_Preambles);

              if (strcmp(rach_preamblesGroupAConfig, "ENABLE") == 0) {
                enb_properties.properties[enb_properties_index]->rach_preamblesGroupAConfig[j] = TRUE;

                if (!(config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_SIZEOFRA_PREAMBLESGROUPA, &rach_sizeOfRA_PreamblesGroupA)
                      && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_MESSAGESIZEGROUPA, &rach_messageSizeGroupA)
                      && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_RACH_MESSAGEPOWEROFFSETGROUPB, &rach_messagePowerOffsetGroupB)))
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %d  rach_sizeOfRA_PreamblesGroupA, messageSizeGroupA,messagePowerOffsetGroupB!\n",
                               lib_config_file_name_pP, i);

                enb_properties.properties[enb_properties_index]->rach_sizeOfRA_PreamblesGroupA[j] = (rach_sizeOfRA_PreamblesGroupA/4)-1;

                if ((rach_numberOfRA_Preambles <4) || (rach_numberOfRA_Preambles>60) || ((rach_numberOfRA_Preambles&3)!=0))
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_sizeOfRA_PreamblesGroupA choice: 4,8,12,...,60!\n",
                               lib_config_file_name_pP, i, rach_sizeOfRA_PreamblesGroupA);


                switch (rach_messageSizeGroupA) {
                case 56:
                  enb_properties.properties[enb_properties_index]->rach_messageSizeGroupA[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b56;
                  break;

                case 144:
                  enb_properties.properties[enb_properties_index]->rach_messageSizeGroupA[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b144;
                  break;

                case 208:
                  enb_properties.properties[enb_properties_index]->rach_messageSizeGroupA[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b208;
                  break;

                case 256:
                  enb_properties.properties[enb_properties_index]->rach_messageSizeGroupA[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b256;
                  break;

                default:
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_messageSizeGroupA choice: 56,144,208,256!\n",
                               lib_config_file_name_pP, i, rach_messageSizeGroupA);
                  break;
                }

                if (strcmp(rach_messagePowerOffsetGroupB,"minusinfinity")==0) {
                  enb_properties.properties[enb_properties_index]->rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_minusinfinity;
                }

                else if (strcmp(rach_messagePowerOffsetGroupB,"dB0")==0) {
                  enb_properties.properties[enb_properties_index]->rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB0;
                }

                else if (strcmp(rach_messagePowerOffsetGroupB,"dB5")==0) {
                  enb_properties.properties[enb_properties_index]->rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB5;
                }

                else if (strcmp(rach_messagePowerOffsetGroupB,"dB8")==0) {
                  enb_properties.properties[enb_properties_index]->rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB8;
                }

                else if (strcmp(rach_messagePowerOffsetGroupB,"dB10")==0) {
                  enb_properties.properties[enb_properties_index]->rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB10;
                }

                else if (strcmp(rach_messagePowerOffsetGroupB,"dB12")==0) {
                  enb_properties.properties[enb_properties_index]->rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB12;
                }

                else if (strcmp(rach_messagePowerOffsetGroupB,"dB15")==0) {
                  enb_properties.properties[enb_properties_index]->rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB15;
                }

                else if (strcmp(rach_messagePowerOffsetGroupB,"dB18")==0) {
                  enb_properties.properties[enb_properties_index]->rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB18;
                } else
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_messagePowerOffsetGroupB choice: minusinfinity,dB0,dB5,dB8,dB10,dB12,dB15,dB18!\n",
                               lib_config_file_name_pP, i, rach_messagePowerOffsetGroupB);

              } else if (strcmp(rach_preamblesGroupAConfig, "DISABLE") == 0) {
                enb_properties.properties[enb_properties_index]->rach_preamblesGroupAConfig[j] = FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_preamblesGroupAConfig choice: ENABLE,DISABLE !\n",
                             lib_config_file_name_pP, i, rach_preamblesGroupAConfig);

              enb_properties.properties[enb_properties_index]->rach_preambleInitialReceivedTargetPower[j] = (rach_preambleInitialReceivedTargetPower+120)/2;

              if ((rach_preambleInitialReceivedTargetPower<-120) || (rach_preambleInitialReceivedTargetPower>-90) || ((rach_preambleInitialReceivedTargetPower&1)!=0))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleInitialReceivedTargetPower choice: -120,-118,...,-90 !\n",
                             lib_config_file_name_pP, i, rach_preambleInitialReceivedTargetPower);


              enb_properties.properties[enb_properties_index]->rach_powerRampingStep[j] = rach_powerRampingStep/2;

              if ((rach_powerRampingStep<0) || (rach_powerRampingStep>6) || ((rach_powerRampingStep&1)!=0))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_powerRampingStep choice: 0,2,4,6 !\n",
                             lib_config_file_name_pP, i, rach_powerRampingStep);



              switch (rach_preambleTransMax) {
              case 3:
                enb_properties.properties[enb_properties_index]->rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n3;
                break;

              case 4:
                enb_properties.properties[enb_properties_index]->rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n4;
                break;

              case 5:
                enb_properties.properties[enb_properties_index]->rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n5;
                break;

              case 6:
                enb_properties.properties[enb_properties_index]->rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n6;
                break;

              case 7:
                enb_properties.properties[enb_properties_index]->rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n7;
                break;

              case 8:
                enb_properties.properties[enb_properties_index]->rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n8;
                break;

              case 10:
                enb_properties.properties[enb_properties_index]->rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n10;
                break;

              case 20:
                enb_properties.properties[enb_properties_index]->rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n20;
                break;

              case 50:
                enb_properties.properties[enb_properties_index]->rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n50;
                break;

              case 100:
                enb_properties.properties[enb_properties_index]->rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n100;
                break;

              case 200:
                enb_properties.properties[enb_properties_index]->rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n200;
                break;

              default:
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleTransMax choice: 3,4,5,6,7,8,10,20,50,100,200!\n",
                             lib_config_file_name_pP, i, rach_preambleTransMax);
                break;
              }

              enb_properties.properties[enb_properties_index]->rach_raResponseWindowSize[j] =  (rach_raResponseWindowSize==10)?7:rach_raResponseWindowSize-2;

              if ((rach_raResponseWindowSize<0)||(rach_raResponseWindowSize==9)||(rach_raResponseWindowSize>10))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_raResponseWindowSize choice: 2,3,4,5,6,7,8,10!\n",
                             lib_config_file_name_pP, i, rach_preambleTransMax);


              enb_properties.properties[enb_properties_index]->rach_macContentionResolutionTimer[j] = (rach_macContentionResolutionTimer/8)-1;

              if ((rach_macContentionResolutionTimer<8) || (rach_macContentionResolutionTimer>64) || ((rach_macContentionResolutionTimer&7)!=0))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_macContentionResolutionTimer choice: 8,16,...,56,64!\n",
                             lib_config_file_name_pP, i, rach_preambleTransMax);

              enb_properties.properties[enb_properties_index]->rach_maxHARQ_Msg3Tx[j] = rach_maxHARQ_Msg3Tx;

              if ((rach_maxHARQ_Msg3Tx<0) || (rach_maxHARQ_Msg3Tx>8))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_maxHARQ_Msg3Tx choice: 1..8!\n",
                             lib_config_file_name_pP, i, rach_preambleTransMax);


              switch (pcch_defaultPagingCycle) {
              case 32:
                enb_properties.properties[enb_properties_index]->pcch_defaultPagingCycle[j] = PCCH_Config__defaultPagingCycle_rf32;
                break;

              case 64:
                enb_properties.properties[enb_properties_index]->pcch_defaultPagingCycle[j] = PCCH_Config__defaultPagingCycle_rf64;
                break;

              case 128:
                enb_properties.properties[enb_properties_index]->pcch_defaultPagingCycle[j] = PCCH_Config__defaultPagingCycle_rf128;
                break;

              case 256:
                enb_properties.properties[enb_properties_index]->pcch_defaultPagingCycle[j] = PCCH_Config__defaultPagingCycle_rf256;
                break;

              default:
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_defaultPagingCycle choice: 32,64,128,256!\n",
                             lib_config_file_name_pP, i, pcch_defaultPagingCycle);
                break;
              }

              if (strcmp(pcch_nB, "fourT") == 0) {
                enb_properties.properties[enb_properties_index]->pcch_nB[j] = PCCH_Config__nB_fourT;
              } else if (strcmp(pcch_nB, "twoT") == 0) {
                enb_properties.properties[enb_properties_index]->pcch_nB[j] = PCCH_Config__nB_twoT;
              } else if (strcmp(pcch_nB, "oneT") == 0) {
                enb_properties.properties[enb_properties_index]->pcch_nB[j] = PCCH_Config__nB_oneT;
              } else if (strcmp(pcch_nB, "halfT") == 0) {
                enb_properties.properties[enb_properties_index]->pcch_nB[j] = PCCH_Config__nB_halfT;
              } else if (strcmp(pcch_nB, "quarterT") == 0) {
                enb_properties.properties[enb_properties_index]->pcch_nB[j] = PCCH_Config__nB_quarterT;
              } else if (strcmp(pcch_nB, "oneEighthT") == 0) {
                enb_properties.properties[enb_properties_index]->pcch_nB[j] = PCCH_Config__nB_oneEighthT;
              } else if (strcmp(pcch_nB, "oneSixteenthT") == 0) {
                enb_properties.properties[enb_properties_index]->pcch_nB[j] = PCCH_Config__nB_oneSixteenthT;
              } else if (strcmp(pcch_nB, "oneThirtySecondT") == 0) {
                enb_properties.properties[enb_properties_index]->pcch_nB[j] = PCCH_Config__nB_oneThirtySecondT;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_nB choice: fourT,twoT,oneT,halfT,quarterT,oneighthT,oneSixteenthT,oneThirtySecondT !\n",
                             lib_config_file_name_pP, i, pcch_defaultPagingCycle);



              switch (bcch_modificationPeriodCoeff) {
              case 2:
                enb_properties.properties[enb_properties_index]->bcch_modificationPeriodCoeff[j] = BCCH_Config__modificationPeriodCoeff_n2;
                break;

              case 4:
                enb_properties.properties[enb_properties_index]->bcch_modificationPeriodCoeff[j] = BCCH_Config__modificationPeriodCoeff_n4;
                break;

              case 8:
                enb_properties.properties[enb_properties_index]->bcch_modificationPeriodCoeff[j] = BCCH_Config__modificationPeriodCoeff_n8;
                break;

              case 16:
                enb_properties.properties[enb_properties_index]->bcch_modificationPeriodCoeff[j] = BCCH_Config__modificationPeriodCoeff_n16;
                break;

              default:
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for bcch_modificationPeriodCoeff choice: 2,4,8,16",
                             lib_config_file_name_pP, i, bcch_modificationPeriodCoeff);

                break;
              }


              switch (ue_TimersAndConstants_t300) {
              case 100:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms100;
                break;

              case 200:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms200;
                break;

              case 300:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms300;
                break;

              case 400:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms400;
                break;

              case 600:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms600;
                break;

              case 1000:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms1000;
                break;

              case 1500:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms1500;
                break;

              case 2000:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms2000;
                break;

              default:
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t300 choice: 100,200,300,400,600,1000,1500,2000 ",
                             lib_config_file_name_pP, i, ue_TimersAndConstants_t300);
                break;

              }

              switch (ue_TimersAndConstants_t301) {
              case 100:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms100;
                break;

              case 200:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms200;
                break;

              case 300:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms300;
                break;

              case 400:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms400;
                break;

              case 600:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms600;
                break;

              case 1000:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms1000;
                break;

              case 1500:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms1500;
                break;

              case 2000:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms2000;
                break;

              default:
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t301 choice: 100,200,300,400,600,1000,1500,2000 ",
                             lib_config_file_name_pP, i, ue_TimersAndConstants_t301);
                break;

              }

              switch (ue_TimersAndConstants_t310) {
              case 0:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t310[j] = UE_TimersAndConstants__t310_ms0;
                break;

              case 50:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t310[j] = UE_TimersAndConstants__t310_ms50;
                break;

              case 100:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t310[j] = UE_TimersAndConstants__t310_ms100;
                break;

              case 200:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t310[j] = UE_TimersAndConstants__t310_ms200;
                break;

              case 500:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t310[j] = UE_TimersAndConstants__t310_ms500;
                break;

              case 1000:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t310[j] = UE_TimersAndConstants__t310_ms1000;
                break;

              case 2000:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t310[j] = UE_TimersAndConstants__t310_ms2000;
                break;

              default:
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t310 choice: 0,50,100,200,500,1000,1500,2000 ",
                             lib_config_file_name_pP, i, ue_TimersAndConstants_t310);
                break;

              }

              switch (ue_TimersAndConstants_t311) {
              case 1000:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t311[j] = UE_TimersAndConstants__t311_ms1000;
                break;

              case 3110:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t311[j] = UE_TimersAndConstants__t311_ms3000;
                break;

              case 5000:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t311[j] = UE_TimersAndConstants__t311_ms5000;
                break;

              case 10000:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t311[j] = UE_TimersAndConstants__t311_ms10000;
                break;

              case 15000:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t311[j] = UE_TimersAndConstants__t311_ms15000;
                break;

              case 20000:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t311[j] = UE_TimersAndConstants__t311_ms20000;
                break;

              case 31100:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_t311[j] = UE_TimersAndConstants__t311_ms30000;
                break;

              default:
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t311 choice: 1000,3000,5000,10000,150000,20000,30000",
                             lib_config_file_name_pP, i, ue_TimersAndConstants_t311);
                break;

              }

              switch (ue_TimersAndConstants_n310) {
              case 1:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n1;
                break;

              case 2:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n2;
                break;

              case 3:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n3;
                break;

              case 4:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n4;
                break;

              case 6:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n6;
                break;

              case 8:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n8;
                break;

              case 10:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n10;
                break;

              case 20:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n20;
                break;

              default:
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_n310 choice: 1,2,3,4,6,6,8,10,20",
                             lib_config_file_name_pP, i, ue_TimersAndConstants_n311);
                break;

              }

              switch (ue_TimersAndConstants_n311) {
              case 1:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n1;
                break;

              case 2:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n2;
                break;

              case 3:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n3;
                break;

              case 4:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n4;
                break;

              case 5:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n5;
                break;

              case 6:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n6;
                break;

              case 8:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n8;
                break;

              case 10:
                enb_properties.properties[enb_properties_index]->ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n10;
                break;

              default:
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t311 choice: 1,2,3,4,5,6,8,10",
                             lib_config_file_name_pP, i, ue_TimersAndConstants_t311);
                break;

              }

	      switch (ue_TransmissionMode) {
	      case 1:
		enb_properties.properties[enb_properties_index]->ue_TransmissionMode[j] = AntennaInfoDedicated__transmissionMode_tm1;
		break;
	      case 2:
		enb_properties.properties[enb_properties_index]->ue_TransmissionMode[j] = AntennaInfoDedicated__transmissionMode_tm2;
		break;
	      case 3:
		enb_properties.properties[enb_properties_index]->ue_TransmissionMode[j] = AntennaInfoDedicated__transmissionMode_tm3;
		break;
	      case 4:
		enb_properties.properties[enb_properties_index]->ue_TransmissionMode[j] = AntennaInfoDedicated__transmissionMode_tm4;
		break;
	      case 5:
		enb_properties.properties[enb_properties_index]->ue_TransmissionMode[j] = AntennaInfoDedicated__transmissionMode_tm5;
		break;
	      case 6:
		enb_properties.properties[enb_properties_index]->ue_TransmissionMode[j] = AntennaInfoDedicated__transmissionMode_tm6;
		break;
	      case 7:
		enb_properties.properties[enb_properties_index]->ue_TransmissionMode[j] = AntennaInfoDedicated__transmissionMode_tm7;
		break;
	      default:
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TransmissionMode choice: 1,2,3,4,5,6,7",
                             lib_config_file_name_pP, i, ue_TransmissionMode);
		break;
	      }
            }
          }

          setting_srb1 = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_SRB1);

          if (setting_srb1 != NULL) {
            if (!(config_setting_lookup_int(setting_srb1, ENB_CONFIG_STRING_SRB1_TIMER_POLL_RETRANSMIT, &srb1_timer_poll_retransmit)
                  && config_setting_lookup_int(setting_srb1, ENB_CONFIG_STRING_SRB1_TIMER_REORDERING,      &srb1_timer_reordering)
                  && config_setting_lookup_int(setting_srb1, ENB_CONFIG_STRING_SRB1_TIMER_STATUS_PROHIBIT, &srb1_timer_status_prohibit)
                  && config_setting_lookup_int(setting_srb1, ENB_CONFIG_STRING_SRB1_MAX_RETX_THRESHOLD,    &srb1_max_retx_threshold)
                  && config_setting_lookup_int(setting_srb1, ENB_CONFIG_STRING_SRB1_POLL_PDU,              &srb1_poll_pdu)
                  && config_setting_lookup_int(setting_srb1, ENB_CONFIG_STRING_SRB1_POLL_BYTE,             &srb1_poll_byte)))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d  timer_poll_retransmit, timer_reordering, "
                           "timer_status_prohibit, poll_pdu, poll_byte, max_retx_threshold !\n",
                           lib_config_file_name_pP, i);

            switch (srb1_max_retx_threshold) {
            case 1:
              enb_properties.properties[enb_properties_index]->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t1;
              break;

            case 2:
              enb_properties.properties[enb_properties_index]->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t2;
              break;

            case 3:
              enb_properties.properties[enb_properties_index]->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t3;
              break;

            case 4:
              enb_properties.properties[enb_properties_index]->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t4;
              break;

            case 6:
              enb_properties.properties[enb_properties_index]->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t6;
              break;

            case 8:
              enb_properties.properties[enb_properties_index]->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t8;
              break;

            case 16:
              enb_properties.properties[enb_properties_index]->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t16;
              break;

            case 32:
              enb_properties.properties[enb_properties_index]->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t32;
              break;

            default:
              AssertFatal (0,
                           "Bad config value when parsing eNB configuration file %s, enb %d  srb1_max_retx_threshold %u!\n",
                           lib_config_file_name_pP, i, srb1_max_retx_threshold);
            }

            switch (srb1_poll_pdu) {
            case 4:
              enb_properties.properties[enb_properties_index]->srb1_poll_pdu = PollPDU_p4;
              break;

            case 8:
              enb_properties.properties[enb_properties_index]->srb1_poll_pdu = PollPDU_p8;
              break;

            case 16:
              enb_properties.properties[enb_properties_index]->srb1_poll_pdu = PollPDU_p16;
              break;

            case 32:
              enb_properties.properties[enb_properties_index]->srb1_poll_pdu = PollPDU_p32;
              break;

            case 64:
              enb_properties.properties[enb_properties_index]->srb1_poll_pdu = PollPDU_p64;
              break;

            case 128:
              enb_properties.properties[enb_properties_index]->srb1_poll_pdu = PollPDU_p128;
              break;

            case 256:
              enb_properties.properties[enb_properties_index]->srb1_poll_pdu = PollPDU_p256;
              break;

            default:
              if (srb1_poll_pdu >= 10000)
                enb_properties.properties[enb_properties_index]->srb1_poll_pdu = PollPDU_pInfinity;
              else
                AssertFatal (0,
                             "Bad config value when parsing eNB configuration file %s, enb %d  srb1_poll_pdu %u!\n",
                             lib_config_file_name_pP, i, srb1_poll_pdu);
            }

            enb_properties.properties[enb_properties_index]->srb1_poll_byte             = srb1_poll_byte;

            switch (srb1_poll_byte) {
            case 25:
              enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kB25;
              break;

            case 50:
              enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kB50;
              break;

            case 75:
              enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kB75;
              break;

            case 100:
              enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kB100;
              break;

            case 125:
              enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kB125;
              break;

            case 250:
              enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kB250;
              break;

            case 375:
              enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kB375;
              break;

            case 500:
              enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kB500;
              break;

            case 750:
              enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kB750;
              break;

            case 1000:
              enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kB1000;
              break;

            case 1250:
              enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kB1250;
              break;

            case 1500:
              enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kB1500;
              break;

            case 2000:
              enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kB2000;
              break;

            case 3000:
              enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kB3000;
              break;

            default:
              if (srb1_poll_byte >= 10000)
                enb_properties.properties[enb_properties_index]->srb1_poll_byte = PollByte_kBinfinity;
              else
                AssertFatal (0,
                             "Bad config value when parsing eNB configuration file %s, enb %d  srb1_poll_byte %u!\n",
                             lib_config_file_name_pP, i, srb1_poll_byte);
            }

            if (srb1_timer_poll_retransmit <= 250) {
              enb_properties.properties[enb_properties_index]->srb1_timer_poll_retransmit = (srb1_timer_poll_retransmit - 5)/5;
            } else if (srb1_timer_poll_retransmit <= 500) {
              enb_properties.properties[enb_properties_index]->srb1_timer_poll_retransmit = (srb1_timer_poll_retransmit - 300)/50 + 50;
            } else {
              AssertFatal (0,
                           "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_poll_retransmit %u!\n",
                           lib_config_file_name_pP, i, srb1_timer_poll_retransmit);
            }

            if (srb1_timer_status_prohibit <= 250) {
              enb_properties.properties[enb_properties_index]->srb1_timer_status_prohibit = srb1_timer_status_prohibit/5;
            } else if ((srb1_timer_poll_retransmit >= 300) && (srb1_timer_poll_retransmit <= 500)) {
              enb_properties.properties[enb_properties_index]->srb1_timer_status_prohibit = (srb1_timer_status_prohibit - 300)/50 + 51;
            } else {
              AssertFatal (0,
                           "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_status_prohibit %u!\n",
                           lib_config_file_name_pP, i, srb1_timer_status_prohibit);
            }

            switch (srb1_timer_reordering) {
            case 0:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms0;
              break;

            case 5:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms5;
              break;

            case 10:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms10;
              break;

            case 15:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms15;
              break;

            case 20:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms20;
              break;

            case 25:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms25;
              break;

            case 30:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms30;
              break;

            case 35:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms35;
              break;

            case 40:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms40;
              break;

            case 45:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms45;
              break;

            case 50:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms50;
              break;

            case 55:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms55;
              break;

            case 60:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms60;
              break;

            case 65:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms65;
              break;

            case 70:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms70;
              break;

            case 75:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms75;
              break;

            case 80:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms80;
              break;

            case 85:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms85;
              break;

            case 90:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms90;
              break;

            case 95:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms95;
              break;

            case 100:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms100;
              break;

            case 110:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms110;
              break;

            case 120:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms120;
              break;

            case 130:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms130;
              break;

            case 140:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms140;
              break;

            case 150:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms150;
              break;

            case 160:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms160;
              break;

            case 170:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms170;
              break;

            case 180:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms180;
              break;

            case 190:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms190;
              break;

            case 200:
              enb_properties.properties[enb_properties_index]->srb1_timer_reordering = T_Reordering_ms200;
              break;

            default:
              AssertFatal (0,
                           "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_reordering %u!\n",
                           lib_config_file_name_pP, i, srb1_timer_reordering);
            }
          } else {
            enb_properties.properties[enb_properties_index]->srb1_timer_poll_retransmit = T_PollRetransmit_ms80;
            enb_properties.properties[enb_properties_index]->srb1_timer_reordering      = T_Reordering_ms35;
            enb_properties.properties[enb_properties_index]->srb1_timer_status_prohibit = T_StatusProhibit_ms0;
            enb_properties.properties[enb_properties_index]->srb1_poll_pdu              = PollPDU_p4;
            enb_properties.properties[enb_properties_index]->srb1_poll_byte             = PollByte_kBinfinity;
            enb_properties.properties[enb_properties_index]->srb1_max_retx_threshold    = UL_AM_RLC__maxRetxThreshold_t8;
          }

          setting_mme_addresses = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_MME_IP_ADDRESS);
          num_mme_address     = config_setting_length(setting_mme_addresses);
          enb_properties.properties[enb_properties_index]->nb_mme = 0;

          for (j = 0; j < num_mme_address; j++) {
            setting_mme_address = config_setting_get_elem(setting_mme_addresses, j);

            if (  !(
                   config_setting_lookup_string(setting_mme_address, ENB_CONFIG_STRING_MME_IPV4_ADDRESS, (const char **)&ipv4)
                   && config_setting_lookup_string(setting_mme_address, ENB_CONFIG_STRING_MME_IPV6_ADDRESS, (const char **)&ipv6)
                   && config_setting_lookup_string(setting_mme_address, ENB_CONFIG_STRING_MME_IP_ADDRESS_ACTIVE, (const char **)&active)
                   && config_setting_lookup_string(setting_mme_address, ENB_CONFIG_STRING_MME_IP_ADDRESS_PREFERENCE, (const char **)&preference)
                 )
              ) {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, %u th enb %u th mme address !\n",
                           lib_config_file_name_pP, i, j);
              continue; // FIXME will prevent segfaults below, not sure what happens at function exit...
            }

            enb_properties.properties[enb_properties_index]->nb_mme += 1;

            enb_properties.properties[enb_properties_index]->mme_ip_address[j].ipv4_address = strdup(ipv4);
            enb_properties.properties[enb_properties_index]->mme_ip_address[j].ipv6_address = strdup(ipv6);

            if (strcmp(active, "yes") == 0) {
              enb_properties.properties[enb_properties_index]->mme_ip_address[j].active = 1;
#if defined(ENABLE_USE_MME)
              EPC_MODE_ENABLED = 1;
#endif
            } // else { (calloc)

            if (strcmp(preference, "ipv4") == 0) {
              enb_properties.properties[enb_properties_index]->mme_ip_address[j].ipv4 = 1;
            } else if (strcmp(preference, "ipv6") == 0) {
              enb_properties.properties[enb_properties_index]->mme_ip_address[j].ipv6 = 1;
            } else if (strcmp(preference, "no") == 0) {
              enb_properties.properties[enb_properties_index]->mme_ip_address[j].ipv4 = 1;
              enb_properties.properties[enb_properties_index]->mme_ip_address[j].ipv6 = 1;
            }
          }
	  // RRH Config 
	  setting_rrh_gws = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_RRH_GW_CONFIG);
	  if ( setting_rrh_gws != NULL) {
          num_rrh_gw     = config_setting_length(setting_rrh_gws);
          enb_properties.properties[enb_properties_index]->nb_rrh_gw = 0;

          for (j = 0; j < num_rrh_gw; j++) {
            setting_rrh_gw = config_setting_get_elem(setting_rrh_gws, j);

            if (  !(
                   config_setting_lookup_string(setting_rrh_gw, ENB_CONFIG_STRING_RRH_GW_LOCAL_IF_NAME, (const char **)&if_name)
		   && config_setting_lookup_string(setting_rrh_gw, ENB_CONFIG_STRING_RRH_GW_LOCAL_ADDRESS, (const char **)&ipv4)
                   && config_setting_lookup_string(setting_rrh_gw, ENB_CONFIG_STRING_RRH_GW_REMOTE_ADDRESS , (const char **)&ipv4_remote)
                   && config_setting_lookup_int(setting_rrh_gw, ENB_CONFIG_STRING_RRH_GW_LOCAL_PORT, &local_port)
                   && config_setting_lookup_int(setting_rrh_gw, ENB_CONFIG_STRING_RRH_GW_REMOTE_PORT, &remote_port)
                   && config_setting_lookup_string(setting_rrh_gw, ENB_CONFIG_STRING_RRH_GW_ACTIVE, (const char **)&active)
		   && config_setting_lookup_string(setting_rrh_gw, ENB_CONFIG_STRING_RRH_GW_TRANSPORT_PREFERENCE, (const char **)&tr_preference)
		   && config_setting_lookup_string(setting_rrh_gw, ENB_CONFIG_STRING_RRH_GW_RF_TARGET_PREFERENCE, (const char **)&rf_preference)
		   && config_setting_lookup_int(setting_rrh_gw, ENB_CONFIG_STRING_RRH_GW_IQ_TXSHIFT, &iq_txshift) 
		   && config_setting_lookup_int(setting_rrh_gw, ENB_CONFIG_STRING_RRH_GW_TX_SAMPLE_ADVANCE, &tx_sample_advance)
		   && config_setting_lookup_int(setting_rrh_gw, ENB_CONFIG_STRING_RRH_GW_TX_SCHEDULING_ADVANCE, &tx_scheduling_advance)
                 )
              ) {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, %u th enb %u the RRH GW address !\n",
                           lib_config_file_name_pP, i, j);
              continue; // FIXME will prevent segfaults below, not sure what happens at function exit...
            }

            enb_properties.properties[enb_properties_index]->nb_rrh_gw += 1;

	    enb_properties.properties[enb_properties_index]->rrh_gw_if_name = strdup(if_name);
            enb_properties.properties[enb_properties_index]->rrh_gw_config[j].local_address  = strdup(ipv4);
            enb_properties.properties[enb_properties_index]->rrh_gw_config[j].remote_address = strdup(ipv4_remote);
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].local_port = local_port;
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].remote_port = remote_port;
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].iq_txshift = iq_txshift;
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].tx_sample_advance = tx_sample_advance;
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].tx_scheduling_advance= tx_scheduling_advance;

            if (strcmp(active, "yes") == 0) {
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].active = 1;
            } 

            if (strcmp(tr_preference, "udp") == 0) {
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].udp = 1;
            } else if (strcmp(tr_preference, "raw") == 0) {
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].raw = 1;
            } else {//if (strcmp(preference, "no") == 0) 
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].udp = 1;
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].raw = 1;
            }

	    if (strcmp(rf_preference, "exmimo") == 0) {
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].exmimo = 1;
            } else if (strcmp(rf_preference, "usrp_b200") == 0) {
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].usrp_b200 = 1;
	    } else if (strcmp(rf_preference, "usrp_x300") == 0) {
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].usrp_x300 = 1;
            } else if (strcmp(rf_preference, "bladerf") == 0) {
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].bladerf = 1;
	    } else if (strcmp(rf_preference, "bladerf") == 0) {
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].lmssdr = 1;	      
            } else {//if (strcmp(preference, "no") == 0) 
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].exmimo = 1;
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].usrp_b200 = 1;
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].usrp_x300 = 1;
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].bladerf = 1;    
              enb_properties.properties[enb_properties_index]->rrh_gw_config[j].lmssdr = 1;    

            }
          }
	  } else {
	    enb_properties.properties[enb_properties_index]->nb_rrh_gw = 0;	    
	    enb_properties.properties[enb_properties_index]->rrh_gw_if_name = "none";
            enb_properties.properties[enb_properties_index]->rrh_gw_config[j].local_address  = "0.0.0.0";
            enb_properties.properties[enb_properties_index]->rrh_gw_config[j].remote_address = "0.0.0.0";
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].local_port= 0;
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].remote_port= 0;	    
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].active = 0;	    
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].udp = 0;
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].raw = 0;
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].tx_scheduling_advance = 0;
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].tx_sample_advance = 0;
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].iq_txshift = 0;
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].exmimo = 0;
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].usrp_b200 = 0;
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].usrp_x300 = 0;
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].bladerf = 0;
	    enb_properties.properties[enb_properties_index]->rrh_gw_config[j].lmssdr = 0;
	  }

          // SCTP SETTING
          enb_properties.properties[enb_properties_index]->sctp_out_streams = SCTP_OUT_STREAMS;
          enb_properties.properties[enb_properties_index]->sctp_in_streams  = SCTP_IN_STREAMS;
# if defined(ENABLE_USE_MME)
          subsetting = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_SCTP_CONFIG);

          if (subsetting != NULL) {
            if ( (config_setting_lookup_int( subsetting, ENB_CONFIG_STRING_SCTP_INSTREAMS, &my_int) )) {
            	enb_properties.properties[enb_properties_index]->sctp_in_streams = (uint16_t)my_int;
            }

            if ( (config_setting_lookup_int( subsetting, ENB_CONFIG_STRING_SCTP_OUTSTREAMS, &my_int) )) {
            	enb_properties.properties[enb_properties_index]->sctp_out_streams = (uint16_t)my_int;
            }
          }
#endif

          // NETWORK_INTERFACES
          subsetting = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);

          if (subsetting != NULL) {
            if (  (
                   config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1_MME,
                                                 (const char **)&enb_interface_name_for_S1_MME)
                   && config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_ENB_IPV4_ADDRESS_FOR_S1_MME,
                                                    (const char **)&enb_ipv4_address_for_S1_MME)
                   && config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1U,
                                                    (const char **)&enb_interface_name_for_S1U)
                   && config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_S1U,
                                                    (const char **)&enb_ipv4_address_for_S1U)
                   && config_setting_lookup_int(subsetting, ENB_CONFIG_STRING_ENB_PORT_FOR_S1U,
                                                &enb_port_for_S1U)
                 )
              ) {
              enb_properties.properties[enb_properties_index]->enb_interface_name_for_S1U = strdup(enb_interface_name_for_S1U);
              cidr = enb_ipv4_address_for_S1U;
              address = strtok(cidr, "/");

              if (address) {
                IPV4_STR_ADDR_TO_INT_NWBO ( address, enb_properties.properties[enb_properties_index]->enb_ipv4_address_for_S1U, "BAD IP ADDRESS FORMAT FOR eNB S1_U !\n" );
              }

              enb_properties.properties[enb_properties_index]->enb_port_for_S1U = enb_port_for_S1U;

              enb_properties.properties[enb_properties_index]->enb_interface_name_for_S1_MME = strdup(enb_interface_name_for_S1_MME);
              cidr = enb_ipv4_address_for_S1_MME;
              address = strtok(cidr, "/");

              if (address) {
                IPV4_STR_ADDR_TO_INT_NWBO ( address, enb_properties.properties[enb_properties_index]->enb_ipv4_address_for_S1_MME, "BAD IP ADDRESS FORMAT FOR eNB S1_MME !\n" );
              }
            }
          }

          // OTG _CONFIG
          setting_otg = config_setting_get_member (setting_enb, ENB_CONF_STRING_OTG_CONFIG);

          if (setting_otg != NULL) {
            num_otg_elements  = config_setting_length(setting_otg);
            printf("num otg elements %d \n", num_otg_elements);
            enb_properties.properties[enb_properties_index]->num_otg_elements = 0;

            for (j = 0; j < num_otg_elements; j++) {
              subsetting_otg=config_setting_get_elem(setting_otg, j);

              if (config_setting_lookup_int(subsetting_otg, ENB_CONF_STRING_OTG_UE_ID, &otg_ue_id)) {
                enb_properties.properties[enb_properties_index]->otg_ue_id[j] = otg_ue_id;
              } else {
                enb_properties.properties[enb_properties_index]->otg_ue_id[j] = 1;
              }

              if (config_setting_lookup_string(subsetting_otg, ENB_CONF_STRING_OTG_APP_TYPE, (const char **)&otg_app_type)) {
                if ((enb_properties.properties[enb_properties_index]->otg_app_type[j] = map_str_to_int(otg_app_type_names,otg_app_type))== -1) {
                  enb_properties.properties[enb_properties_index]->otg_app_type[j] = BCBR;
                }
              } else {
                enb_properties.properties[enb_properties_index]->otg_app_type[j] = NO_PREDEFINED_TRAFFIC; // 0
              }

              if (config_setting_lookup_string(subsetting_otg, ENB_CONF_STRING_OTG_BG_TRAFFIC, (const char **)&otg_bg_traffic)) {

                if ((enb_properties.properties[enb_properties_index]->otg_bg_traffic[j] = map_str_to_int(switch_names,otg_bg_traffic)) == -1) {
                  enb_properties.properties[enb_properties_index]->otg_bg_traffic[j]=0;
                }
              } else {
                enb_properties.properties[enb_properties_index]->otg_bg_traffic[j] = 0;
                printf("otg bg %s\n", otg_bg_traffic);
              }

              enb_properties.properties[enb_properties_index]->num_otg_elements+=1;

            }
          }

          // log_config
          subsetting = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_LOG_CONFIG);

          if (subsetting != NULL) {
            // global
            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_GLOBAL_LOG_LEVEL, (const char **)  &glog_level)) {
              if ((enb_properties.properties[enb_properties_index]->glog_level = map_str_to_int(log_level_names, glog_level)) == -1) {
                enb_properties.properties[enb_properties_index]->glog_level = LOG_INFO;
              }

              //printf( "\tGlobal log level :\t%s->%d\n",glog_level, enb_properties.properties[enb_properties_index]->glog_level);
            } else {
              enb_properties.properties[enb_properties_index]->glog_level = LOG_INFO;
            }

            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_GLOBAL_LOG_VERBOSITY,(const char **)  &glog_verbosity)) {
              if ((enb_properties.properties[enb_properties_index]->glog_verbosity = map_str_to_int(log_verbosity_names, glog_verbosity)) == -1) {
                enb_properties.properties[enb_properties_index]->glog_verbosity = LOG_MED;
              }

              //printf( "\tGlobal log verbosity:\t%s->%d\n",glog_verbosity, enb_properties.properties[enb_properties_index]->glog_verbosity);
            } else {
              enb_properties.properties[enb_properties_index]->glog_verbosity = LOG_MED;
            }

            // HW
            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_HW_LOG_LEVEL, (const char **) &hw_log_level)) {
              if ((enb_properties.properties[enb_properties_index]->hw_log_level = map_str_to_int(log_level_names,hw_log_level)) == -1) {
                enb_properties.properties[enb_properties_index]->hw_log_level = LOG_INFO;
              }

              //printf( "\tHW log level :\t%s->%d\n",hw_log_level,enb_properties.properties[enb_properties_index]->hw_log_level);
            } else {
              enb_properties.properties[enb_properties_index]->hw_log_level = LOG_INFO;
            }

            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_HW_LOG_VERBOSITY, (const char **) &hw_log_verbosity)) {
              if ((enb_properties.properties[enb_properties_index]->hw_log_verbosity = map_str_to_int(log_verbosity_names,hw_log_verbosity)) == -1) {
                enb_properties.properties[enb_properties_index]->hw_log_verbosity = LOG_MED;
              }

              //printf( "\tHW log verbosity:\t%s->%d\n",hw_log_verbosity, enb_properties.properties[enb_properties_index]->hw_log_verbosity);
            } else {
              enb_properties.properties[enb_properties_index]->hw_log_verbosity = LOG_MED;
            }

            // phy
            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_PHY_LOG_LEVEL,(const char **) &phy_log_level)) {
              if ((enb_properties.properties[enb_properties_index]->phy_log_level = map_str_to_int(log_level_names,phy_log_level)) == -1) {
                enb_properties.properties[enb_properties_index]->phy_log_level = LOG_INFO;
              }

              //printf( "\tPHY log level :\t%s->%d\n",phy_log_level,enb_properties.properties[enb_properties_index]->phy_log_level);
            } else {
              enb_properties.properties[enb_properties_index]->phy_log_level = LOG_INFO;
            }

            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_PHY_LOG_VERBOSITY, (const char **)&phy_log_verbosity)) {
              if ((enb_properties.properties[enb_properties_index]->phy_log_verbosity = map_str_to_int(log_verbosity_names,phy_log_verbosity)) == -1) {
                enb_properties.properties[enb_properties_index]->phy_log_verbosity = LOG_MED;
              }

              //printf( "\tPHY log verbosity:\t%s->%d\n",phy_log_level,enb_properties.properties[enb_properties_index]->phy_log_verbosity);
            } else {
              enb_properties.properties[enb_properties_index]->phy_log_verbosity = LOG_MED;
            }

            //mac
            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_MAC_LOG_LEVEL, (const char **)&mac_log_level)) {
              if ((enb_properties.properties[enb_properties_index]->mac_log_level = map_str_to_int(log_level_names,mac_log_level)) == -1 ) {
                enb_properties.properties[enb_properties_index]->mac_log_level = LOG_INFO;
              }

              //printf( "\tMAC log level :\t%s->%d\n",mac_log_level,enb_properties.properties[enb_properties_index]->mac_log_level);
            } else {
              enb_properties.properties[enb_properties_index]->mac_log_level = LOG_INFO;
            }

            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_MAC_LOG_VERBOSITY, (const char **)&mac_log_verbosity)) {
              if ((enb_properties.properties[enb_properties_index]->mac_log_verbosity = map_str_to_int(log_verbosity_names,mac_log_verbosity)) == -1) {
                enb_properties.properties[enb_properties_index]->mac_log_verbosity = LOG_MED;
              }

              //printf( "\tMAC log verbosity:\t%s->%d\n",mac_log_verbosity,enb_properties.properties[enb_properties_index]->mac_log_verbosity);
            } else {
              enb_properties.properties[enb_properties_index]->mac_log_verbosity = LOG_MED;
            }

            //rlc
            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_RLC_LOG_LEVEL, (const char **)&rlc_log_level)) {
              if ((enb_properties.properties[enb_properties_index]->rlc_log_level = map_str_to_int(log_level_names,rlc_log_level)) == -1) {
                enb_properties.properties[enb_properties_index]->rlc_log_level = LOG_INFO;
              }

              //printf( "\tRLC log level :\t%s->%d\n",rlc_log_level, enb_properties.properties[enb_properties_index]->rlc_log_level);
            } else {
              enb_properties.properties[enb_properties_index]->rlc_log_level = LOG_INFO;
            }

            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_RLC_LOG_VERBOSITY, (const char **)&rlc_log_verbosity)) {
              if ((enb_properties.properties[enb_properties_index]->rlc_log_verbosity = map_str_to_int(log_verbosity_names,rlc_log_verbosity)) == -1) {
                enb_properties.properties[enb_properties_index]->rlc_log_verbosity = LOG_MED;
              }

              //printf( "\tRLC log verbosity:\t%s->%d\n",rlc_log_verbosity, enb_properties.properties[enb_properties_index]->rlc_log_verbosity);
            } else {
              enb_properties.properties[enb_properties_index]->rlc_log_verbosity = LOG_MED;
            }

            //pdcp
            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_PDCP_LOG_LEVEL, (const char **)&pdcp_log_level)) {
              if ((enb_properties.properties[enb_properties_index]->pdcp_log_level = map_str_to_int(log_level_names,pdcp_log_level)) == -1) {
                enb_properties.properties[enb_properties_index]->pdcp_log_level = LOG_INFO;
              }

              //printf( "\tPDCP log level :\t%s->%d\n",pdcp_log_level, enb_properties.properties[enb_properties_index]->pdcp_log_level);
            } else {
              enb_properties.properties[enb_properties_index]->pdcp_log_level = LOG_INFO;
            }

            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_PDCP_LOG_VERBOSITY, (const char **)&pdcp_log_verbosity)) {
              enb_properties.properties[enb_properties_index]->pdcp_log_verbosity = map_str_to_int(log_verbosity_names,pdcp_log_verbosity);
              //printf( "\tPDCP log verbosity:\t%s->%d\n",pdcp_log_verbosity, enb_properties.properties[enb_properties_index]->pdcp_log_verbosity);
            } else {
              enb_properties.properties[enb_properties_index]->pdcp_log_verbosity = LOG_MED;
            }

            //rrc
            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_RRC_LOG_LEVEL, (const char **)&rrc_log_level)) {
              if ((enb_properties.properties[enb_properties_index]->rrc_log_level = map_str_to_int(log_level_names,rrc_log_level)) == -1 ) {
                enb_properties.properties[enb_properties_index]->rrc_log_level = LOG_INFO;
              }

              //printf( "\tRRC log level :\t%s->%d\n",rrc_log_level,enb_properties.properties[enb_properties_index]->rrc_log_level);
            } else {
              enb_properties.properties[enb_properties_index]->rrc_log_level = LOG_INFO;
            }

            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_RRC_LOG_VERBOSITY, (const char **)&rrc_log_verbosity)) {
              if ((enb_properties.properties[enb_properties_index]->rrc_log_verbosity = map_str_to_int(log_verbosity_names,rrc_log_verbosity)) == -1) {
                enb_properties.properties[enb_properties_index]->rrc_log_verbosity = LOG_MED;
              }

              //printf( "\tRRC log verbosity:\t%s->%d\n",rrc_log_verbosity,enb_properties.properties[enb_properties_index]->rrc_log_verbosity);
            } else {
              enb_properties.properties[enb_properties_index]->rrc_log_verbosity = LOG_MED;
            }

            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_GTPU_LOG_LEVEL, (const char **)&gtpu_log_level)) {
              if ((enb_properties.properties[enb_properties_index]->gtpu_log_level = map_str_to_int(log_level_names,gtpu_log_level)) == -1 ) {
                enb_properties.properties[enb_properties_index]->gtpu_log_level = LOG_INFO;
              }

              //printf( "\tGTPU log level :\t%s->%d\n",gtpu_log_level,enb_properties.properties[enb_properties_index]->gtpu_log_level);
            } else {
              enb_properties.properties[enb_properties_index]->gtpu_log_level = LOG_INFO;
            }

            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_GTPU_LOG_VERBOSITY, (const char **)&gtpu_log_verbosity)) {
              if ((enb_properties.properties[enb_properties_index]->gtpu_log_verbosity = map_str_to_int(log_verbosity_names,gtpu_log_verbosity)) == -1) {
                enb_properties.properties[enb_properties_index]->gtpu_log_verbosity = LOG_MED;
              }

              //printf( "\tGTPU log verbosity:\t%s->%d\n",gtpu_log_verbosity,enb_properties.properties[enb_properties_index]->gtpu_log_verbosity);
            } else {
              enb_properties.properties[enb_properties_index]->gtpu_log_verbosity = LOG_MED;
            }

            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_UDP_LOG_LEVEL, (const char **)&udp_log_level)) {
              if ((enb_properties.properties[enb_properties_index]->udp_log_level = map_str_to_int(log_level_names,udp_log_level)) == -1 ) {
                enb_properties.properties[enb_properties_index]->udp_log_level = LOG_INFO;
              }

              //printf( "\tUDP log level :\t%s->%d\n",udp_log_level,enb_properties.properties[enb_properties_index]->udp_log_level);
            } else {
              enb_properties.properties[enb_properties_index]->udp_log_level = LOG_INFO;
            }

            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_UDP_LOG_VERBOSITY, (const char **)&udp_log_verbosity)) {
              if ((enb_properties.properties[enb_properties_index]->udp_log_verbosity = map_str_to_int(log_verbosity_names,udp_log_verbosity)) == -1) {
                enb_properties.properties[enb_properties_index]->udp_log_verbosity = LOG_MED;
              }

              //printf( "\tUDP log verbosity:\t%s->%d\n",udp_log_verbosity,enb_properties.properties[enb_properties_index]->gtpu_log_verbosity);
            } else {
              enb_properties.properties[enb_properties_index]->udp_log_verbosity = LOG_MED;
            }

            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_OSA_LOG_LEVEL, (const char **)&osa_log_level)) {
              if ((enb_properties.properties[enb_properties_index]->osa_log_level = map_str_to_int(log_level_names,osa_log_level)) == -1 ) {
                enb_properties.properties[enb_properties_index]->osa_log_level = LOG_INFO;
              }

              //printf( "\tOSA log level :\t%s->%d\n",osa_log_level,enb_properties.properties[enb_properties_index]->osa_log_level);
            } else {
              enb_properties.properties[enb_properties_index]->osa_log_level = LOG_INFO;
            }

            if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_OSA_LOG_VERBOSITY, (const char **)&osa_log_verbosity)) {
              if ((enb_properties.properties[enb_properties_index]->osa_log_verbosity = map_str_to_int(log_verbosity_names,osa_log_verbosity)) == -1) {
                enb_properties.properties[enb_properties_index]->osa_log_verbosity = LOG_MED;
              }

              //printf( "\tOSA log verbosity:\t%s->%d\n",osa_log_verbosity,enb_properties.properties[enb_properties_index]->gosa_log_verbosity);
            } else {
              enb_properties.properties[enb_properties_index]->osa_log_verbosity = LOG_MED;
            }

          } else { // not configuration is given
            enb_properties.properties[enb_properties_index]->glog_level         = LOG_INFO;
            enb_properties.properties[enb_properties_index]->glog_verbosity     = LOG_MED;
            enb_properties.properties[enb_properties_index]->hw_log_level       = LOG_INFO;
            enb_properties.properties[enb_properties_index]->hw_log_verbosity   = LOG_MED;
            enb_properties.properties[enb_properties_index]->phy_log_level      = LOG_INFO;
            enb_properties.properties[enb_properties_index]->phy_log_verbosity  = LOG_MED;
            enb_properties.properties[enb_properties_index]->mac_log_level      = LOG_INFO;
            enb_properties.properties[enb_properties_index]->mac_log_verbosity  = LOG_MED;
            enb_properties.properties[enb_properties_index]->rlc_log_level      = LOG_INFO;
            enb_properties.properties[enb_properties_index]->rlc_log_verbosity  = LOG_MED;
            enb_properties.properties[enb_properties_index]->pdcp_log_level     = LOG_INFO;
            enb_properties.properties[enb_properties_index]->pdcp_log_verbosity = LOG_MED;
            enb_properties.properties[enb_properties_index]->rrc_log_level      = LOG_INFO;
            enb_properties.properties[enb_properties_index]->rrc_log_verbosity  = LOG_MED;
            enb_properties.properties[enb_properties_index]->gtpu_log_level     = LOG_INFO;
            enb_properties.properties[enb_properties_index]->gtpu_log_verbosity = LOG_MED;
            enb_properties.properties[enb_properties_index]->udp_log_level      = LOG_INFO;
            enb_properties.properties[enb_properties_index]->udp_log_verbosity  = LOG_MED;
            enb_properties.properties[enb_properties_index]->osa_log_level      = LOG_INFO;
            enb_properties.properties[enb_properties_index]->osa_log_verbosity  = LOG_MED;
          }

          enb_properties_index += 1;
          break;
        }
      }
    }
  }

  enb_properties.number = num_enb_properties;

  AssertFatal (enb_properties_index == num_enb_properties,
               "Failed to parse eNB configuration file %s, mismatch between %u active eNBs and %u corresponding defined eNBs !\n",
               lib_config_file_name_pP, num_enb_properties, enb_properties_index);

  enb_config_display();
  return &enb_properties;

}

const Enb_properties_array_t *enb_config_get(void)
{
  return &enb_properties;
}

