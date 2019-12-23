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

/*! \file openair2/ENB_APP/enb_paramdef_mce.h
 * \brief definition of configuration parameters for MCE modules 
 * \author Javier MORGADE
 * \date 2019
 * \version 0.1
 * \company VICOMTECH Spain
 * \email: javier.morgade@ieee.org
 * \note
 * \warning
 */

#include "common/config/config_paramdesc.h"
#include "RRC_paramsvalues.h"



/* mce configuration parameters names */

#define MCE_CONFIG_STRING_MCE_ID                        "MCE_ID"
#define MCE_CONFIG_STRING_MCE_NAME                      "MCE_name"
#define MCE_CONFIG_STRING_MCE_M2                        "enable_mce_m2"
#define MCE_CONFIG_STRING_MCE_M3                        "enable_mce_m3"

#define MCE_PARAMS_DESC {\
{MCE_CONFIG_STRING_MCE_ID,                       NULL,   0,            uptr:NULL,   defintval:0,                 TYPE_UINT,      0},  \
{MCE_CONFIG_STRING_MCE_NAME,                     NULL,   0,            strptr:NULL, defstrval:"OAIMCE",       TYPE_STRING,    0},  \
{MCE_CONFIG_STRING_MCE_M2,                       NULL,   0,            strptr:NULL, defstrval:"no",              TYPE_STRING,    0},  \
{MCE_CONFIG_STRING_MCE_M3,                       NULL,   0,            strptr:NULL, defstrval:"no",              TYPE_STRING,    0},  \
}  

#define MCE_MCE_ID_IDX                  0
#define MCE_MCE_NAME_IDX                1
#define MCE_ENABLE_MCE_M2_IDX           2
#define MCE_ENABLE_MCE_M3_IDX           3

#define MCE_PARAMS_CHECK {                                         \
  { .s5 = { NULL } },                                             \
  { .s5 = { NULL } },                                             \
  { .s5 = { NULL } },                                             \
  { .s5 = { NULL } },                                             \
}


#define MCE_CONFIG_STRING_NETWORK_INTERFACES_CONFIG     "NETWORK_INTERFACES"

#define MCE2_INTERFACE_NAME_FOR_M2_ENB_IDX	0
#define MCE2_IPV4_ADDR_FOR_M2C_IDX            	1
#define MCE2_PORT_FOR_M2C_IDX             	2
#define MCE2_INTERFACE_NAME_FOR_M3_MME_IDX      3
#define MCE2_IPV4_ADDR_FOR_M3C_IDX            	4
#define MCE2_PORT_FOR_M3C_IDX                	5

/* interface configuration parameters names   */
/* M2 interface configuration parameters names */
#define MCE_CONFIG_STRING_MCE_INTERFACE_NAME_FOR_M2_ENB "ENB_INTERFACE_NAME_FOR_M2_ENB"
#define MCE_CONFIG_STRING_MCE_IPV4_ADDRESS_FOR_M2C "MCE_IPV4_ADDRESS_FOR_M2C"
#define MCE_CONFIG_STRING_MCE_PORT_FOR_M2C "MCE_PORT_FOR_M2C"

/* M3 interface configuration parameters names */
#define MCE_CONFIG_STRING_MCE_INTERFACE_NAME_FOR_M3_MME "MCE_INTERFACE_NAME_FOR_M3_MME"
#define MCE_CONFIG_STRING_MCE_IPV4_ADDRESS_FOR_M3C "MCE_IPV4_ADDRESS_FOR_M3C"
#define MCE_CONFIG_STRING_MCE_PORT_FOR_M3C "MCE_PORT_FOR_M3C"


#define MCE_NETPARAMS_DESC {  \
{MCE_CONFIG_STRING_MCE_INTERFACE_NAME_FOR_M2_ENB,        NULL,      0,         strptr:&mce_interface_name_for_m2_enb,    defstrval:"lo",      TYPE_STRING,      0},      \
{MCE_CONFIG_STRING_MCE_IPV4_ADDRESS_FOR_M2C,             NULL,      0,         strptr:&mce_ipv4_address_for_m2c,         defstrval:NULL,      TYPE_STRING,      0},      \
{MCE_CONFIG_STRING_MCE_PORT_FOR_M2C,                     NULL,      0,         uptr:&mce_port_for_m2c,           	 defintval:36443L,    TYPE_UINT,        0},      \
{MCE_CONFIG_STRING_MCE_INTERFACE_NAME_FOR_M3_MME,        NULL,      0,         strptr:&mce_interface_name_for_m3_mme,    defstrval:NULL,      TYPE_STRING,      0},      \
{MCE_CONFIG_STRING_MCE_IPV4_ADDRESS_FOR_M3C,             NULL,      0,         strptr:&mce_ipv4_address_for_m3c,         defstrval:NULL,      TYPE_STRING,      0},      \
{MCE_CONFIG_STRING_MCE_PORT_FOR_M3C,                     NULL,      0,         uptr:&mce_port_for_m3c,           	 defintval:36444L,    TYPE_UINT,        0},      \
} 


/*-------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            M3 configuration parameters                                                             */
/* M3 configuration parameters section name */
#define MCE_CONFIG_STRING_TARGET_MME_M3_IP_ADDRESS                "target_mme_m3_ip_address"
/* M3 configuration parameters names   */
#define MCE_CONFIG_STRING_TARGET_MME_M3_IPV4_ADDRESS              "ipv4"
#define MCE_CONFIG_STRING_TARGET_MME_M3_IPV6_ADDRESS              "ipv6"
#define MCE_CONFIG_STRING_TARGET_MME_M3_IP_ADDRESS_PREFERENCE     "preference"
/*   optname                                          helpstr   paramflags    XXXptr       defXXXval         type           numelt     */
#define M3PARAMS_DESC {  \
{MCE_CONFIG_STRING_TARGET_MME_M3_IPV4_ADDRESS,                   NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
{MCE_CONFIG_STRING_TARGET_MME_M3_IPV6_ADDRESS,                   NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
{MCE_CONFIG_STRING_TARGET_MME_M3_IP_ADDRESS_PREFERENCE,          NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
}

#define MCE2_M3_IPV4_ADDRESS_IDX          0
#define MCE2_M3_IPV6_ADDRESS_IDX          1
#define MCE2_M3_IP_ADDRESS_PREFERENCE_IDX 2


/*-----------------------------------------------------------------------------------------------------------------------------------*/
/*                             MCCH related BCCH Configuration per MBSFN area configuration parameters                                                             */
/* MCCH configuration parameters section */
#define MCE_CONFIG_STRING_MCCH_CONFIG_PER_MBSFN_AREA                	"mcch_config_per_mbsfn_area"
/* M3 configuration parameters names   */
#define MCE_CONFIG_STRING_MCCH_MBSFN_AREA	                  	"mbsfn_area"
#define MCE_CONFIG_STRING_MCCH_PDCCH_LENGTH              		"pdcch_length"
#define MCE_CONFIG_STRING_MCCH_REPETITION_PERIOD     			"repetition_period"
#define MCE_CONFIG_STRING_MCCH_OFFSET     				"offset"
#define MCE_CONFIG_STRING_MCCH_MODIFICATION_PERIOD     			"modification_period"
#define MCE_CONFIG_STRING_MCCH_SF_ALLOCATION_INFO     			"subframe_allocation_info"
#define MCE_CONFIG_STRING_MCCH_MCS     					"mcs"

/*   optname                                          helpstr   paramflags    XXXptr       defXXXval         type           numelt     */

#define MCCH_PARAMS_DESC {  \
{MCE_CONFIG_STRING_MCCH_MBSFN_AREA,         NULL,      0,         uptr:NULL,           	 defuintval:0,    TYPE_UINT,        0},      \
{MCE_CONFIG_STRING_MCCH_PDCCH_LENGTH,       NULL,      0,         uptr:NULL,           	 defuintval:0,    TYPE_UINT,        0},      \
{MCE_CONFIG_STRING_MCCH_REPETITION_PERIOD,  NULL,      0,         uptr:NULL,           	 defuintval:0,    TYPE_UINT,        0},      \
{MCE_CONFIG_STRING_MCCH_OFFSET,             NULL,      0,         uptr:NULL,           	 defuintval:0,    TYPE_UINT,        0},      \
{MCE_CONFIG_STRING_MCCH_MODIFICATION_PERIOD,NULL,      0,         uptr:NULL,           	 defuintval:0,    TYPE_UINT,        0},      \
{MCE_CONFIG_STRING_MCCH_SF_ALLOCATION_INFO, NULL,      0,         uptr:NULL,       	 defuintval:0,    TYPE_UINT,        0},      \
{MCE_CONFIG_STRING_MCCH_MCS,                NULL,      0,         uptr:NULL,           	 defuintval:0,    TYPE_UINT,        0},      \
}

#define MCCH_MBSFN_AREA_IDX          0
#define MCCH_PDCCH_LENGTH_IDX        1
#define MCCH_REPETITION_PERIOD_IDX   2
#define MCCH_OFFSET_IDX 	     3
#define MCCH_MODIFICATION_PERIOD_IDX 4
#define MCCH_SF_ALLOCATION_INFO_IDX  5
#define MCCH_MCS_IDX 		     6
/*-------------------------------------------------------------------------------------------------------------------------------------*/
/* M3 configuration parameters section name */
#define MCE_CONFIG_STRING_PLMN                "plnm"
/* M3 configuration parameters names   */
#define MCE_CONFIG_STRING_MCC              "mcc"
#define MCE_CONFIG_STRING_MNC              "mnc"
#define MCE_CONFIG_STRING_MNC_LENGTH       "mnc_length"
/*   optname                                          helpstr   paramflags    XXXptr       defXXXval         type           numelt     */
#define MCE_PLMN_PARAMS_DESC {  \
{MCE_CONFIG_STRING_MCC,                   NULL,      0,         uptr:NULL,   defuintval:1000,   TYPE_UINT,   0},          \
{MCE_CONFIG_STRING_MNC,                   NULL,      0,         uptr:NULL,   defuintval:1000,   TYPE_UINT,   0},          \
{MCE_CONFIG_STRING_MNC_LENGTH,            NULL,      0,         uptr:NULL,   defuintval:0,   TYPE_UINT,   0},          \
}

#define MCE_CONFIG_STRING_MCC_IDX          0
#define MCE_CONFIG_STRING_MNC_IDX          1
#define MCE_CONFIG_STRING_MNC_LENGTH_IDX   2
/*-------------------------------------------------------------------------------------------------------------------------------------*/

#define MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO			"mbms_sched_info"

#define MCE_CONFIG_STRING_MCCH_UPDATE_TIME			"mcch_update_time"
#define MCE_MBMS_SCHEDULING_INFO_PARAMS_DESC {  \
{MCE_CONFIG_STRING_MCCH_UPDATE_TIME,            NULL,      0,         uptr:NULL,   defuintval:0,   TYPE_UINT,   0},          \
}
#define MCE_CONFIG_STRING_MCCH_UPDATE_TIME_IDX	0

///*-------------------------------------------------------------------------------------------------------------------------------------*/
#define MCE_CONFIG_STRING_MBMS_AREA_CONFIGURATION_LIST			"mbms_area_config_list"

#define MCE_CONFIG_STRING_COMMON_SF_ALLOCATION_PERIOD			"common_sf_allocation_period"
#define MCE_CONFIG_STRING_MBMS_AREA_ID					"mbms_area_id"

#define MCE_MBMS_AREA_CONFIGURATION_LIST_PARAMS_DESC {  \
{MCE_CONFIG_STRING_COMMON_SF_ALLOCATION_PERIOD,         NULL,      0,         uptr:NULL,           	 defuintval:0,    TYPE_UINT,        0},      \
{MCE_CONFIG_STRING_MBMS_AREA_ID,       			NULL,      0,         uptr:NULL,           	 defuintval:0,    TYPE_UINT,        0},      \
}
#define MCE_CONFIG_STRING_COMMON_SF_ALLOCATION_PERIOD_IDX	0
#define MCE_CONFIG_STRING_MBMS_AREA_ID_IDX	1
//
///*-------------------------------------------------------------------------------------------------------------------------------------*/
#define MCE_CONFIG_STRING_PMCH_CONFIGURATION_LIST		"pmch_config_list"

#define MCE_CONFIG_STRING_ALLOCATED_SF_END			"allocated_sf_end"
#define MCE_CONFIG_STRING_DATA_MCS				"data_mcs"
#define MCE_CONFIG_STRING_MCH_SCHEDULING_PERIOD			"mch_scheduling_period"

#define MCE_MBMS_PMCH_CONFIGURATION_LIST_PARAMS_DESC {  \
{MCE_CONFIG_STRING_ALLOCATED_SF_END,            NULL,      0,         uptr:NULL,   defuintval:0,   TYPE_UINT,   0},          \
{MCE_CONFIG_STRING_DATA_MCS,            	NULL,      0,         uptr:NULL,   defuintval:0,   TYPE_UINT,   0},          \
{MCE_CONFIG_STRING_MCH_SCHEDULING_PERIOD,       NULL,      0,         uptr:NULL,   defuintval:0,   TYPE_UINT,   0},          \
}
#define MCE_CONFIG_STRING_ALLOCATED_SF_END_IDX	0
#define MCE_CONFIG_STRING_DATA_MCS_IDX			1
#define MCE_CONFIG_STRING_MCH_SCHEDULING_PERIOD_IDX	2
/*-------------------------------------------------------------------------------------------------------------------------------------*/
#define MCE_CONFIG_STRING_MBMS_SESSION_LIST			"mbms_session_list"

#define MCE_CONFIG_STRING_MBMS_SERVICE_ID			"service_id"
#define MCE_CONFIG_STRING_MBMS_LCID_ID				"lcid_id"
#define MCE_CONFIG_STRING_MBMS_LCID				"lcid"

#define MCE_MBMS_MBMS_SESSION_LIST_DESC {  \
{MCE_CONFIG_STRING_MBMS_SERVICE_ID,            	NULL,      0,         uptr:NULL,   defuintval:0,   TYPE_UINT32,   0},          \
{MCE_CONFIG_STRING_MBMS_LCID_ID,            	NULL,      0,         uptr:NULL,   defuintval:0,   TYPE_UINT,   0},          \
{MCE_CONFIG_STRING_MBMS_LCID,            	NULL,      0,         uptr:NULL,   defuintval:0,   TYPE_UINT,   0},          \
}
#define MCE_CONFIG_STRING_MBMS_SERVICE_ID_IDX	0
#define MCE_CONFIG_STRING_MBMS_LCID_ID_IDX	1
#define MCE_CONFIG_STRING_MBMS_LCID_IDX	2

/*-------------------------------------------------------------------------------------------------------------------------------------*/
#define MCE_CONFIG_STRING_MBMS_SF_CONFIGURATION_LIST			"mbms_sf_config_list"

#define MCE_CONFIG_STRING_RADIOFRAME_ALLOCATION_PERIOD			"radioframe_allocation_period"
#define MCE_CONFIG_STRING_RADIOFRAME_ALLOOCATION_OFFSET			"radioframe_alloocation_offset"
#define MCE_CONFIG_STRING_NUM_FRAME					"num_frame"
#define MCE_CONFIG_STRING_SUBFRAME_ALLOCATION				"subframe_allocation"

#define MCE_MBMS_MBMS_SF_CONFIGURATION_LIST_PARAMS_DESC {  \
{MCE_CONFIG_STRING_RADIOFRAME_ALLOCATION_PERIOD,            NULL,      0,         uptr:NULL,   defintval:0,   TYPE_UINT,   0},          \
{MCE_CONFIG_STRING_RADIOFRAME_ALLOOCATION_OFFSET,            NULL,      0,         uptr:NULL,   defintval:0,   TYPE_UINT,   0},          \
{MCE_CONFIG_STRING_NUM_FRAME,            NULL,      0,         strptr:NULL,   defstrval:"oneFrame",   TYPE_STRING,   0},          \
{MCE_CONFIG_STRING_SUBFRAME_ALLOCATION,            NULL,      0,         uptr:NULL,   defintval:0,   TYPE_UINT,   0},          \
}
#define MCE_CONFIG_STRING_RADIOFRAME_ALLOCATION_PERIOD_IDX 0
#define MCE_CONFIG_STRING_RADIOFRAME_ALLOOCATION_OFFSET_IDX	1
#define MCE_CONFIG_STRING_NUM_FRAME_IDX	2
#define MCE_CONFIG_STRING_SUBFRAME_ALLOCATION_IDX	3



