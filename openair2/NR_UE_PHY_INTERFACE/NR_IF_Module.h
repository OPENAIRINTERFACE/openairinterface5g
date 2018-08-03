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

/* \file NR_IF_Module.h
 * \brief data structures for L1/L2 interface modules
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#ifndef __NR_IF_MODULE_H__
#define __NR_IF_MODULE_H__

#include "platform_types.h"
#include "fapi_nr_ue_interface.h"



typedef struct {
    /// module id
    module_id_t module_id;
    /// gNB index
    uint32_t gNB_index;
    /// component carrier id
    int cc_id;
    /// frame 
    frame_t frame;
    /// slot
    uint8_t slot;

    /// NR UE FAPI-like P7 message, direction: L1 to L2
    /// data reception indication structure
    fapi_nr_rx_indication_t *rx_ind;

    /// dci reception indication structure
    fapi_nr_dci_indication_t *dci_ind;

} nr_downlink_indication_t;


typedef struct {
    /// module id
    module_id_t module_id;
    /// gNB index
    uint32_t gNB_index;
    /// component carrier id
    int cc_id;
    /// frame 
    frame_t frame;
    /// slot
    uint32_t slot;
    /// ssb_index, if ssb is not present in current TTI, thie value set to -1
    int8_t ssb_index;
} nr_uplink_indication_t;

// Downlink subframe P7


typedef struct {
    /// module id
    module_id_t module_id; 
    /// component carrier id
    uint8_t CC_id;
    /// frame
    frame_t frame;
    /// subframe
    sub_frame_t subframe;
    /// slot
    uint8_t slot;

    /// NR UE FAPI-like P7 message, direction: L2 to L1
    /// downlink transmission configuration request structure
    fapi_nr_dl_config_request_t *dl_config;

    /// uplink transmission configuration request structure
    fapi_nr_ul_config_request_t *ul_config;

    /// data transmission request structure
    fapi_nr_tx_request_t *tx_request;

} nr_scheduled_response_t;

typedef struct {
    /// module id
    uint8_t Mod_id;
    /// component carrier id
    uint8_t CC_id;
    
    /// NR UE FAPI-like P5 message
    /// physical layer configuration request structure
    fapi_nr_config_request_t config_req;

} nr_phy_config_t;


/*
 * Generic type of an application-defined callback to return various
 * types of data to the application.
 * EXPECTED RETURN VALUES:
 *  -1: Failed to consume bytes. Abort the mission.
 * Non-negative return values indicate success, and ignored.
 */
typedef int8_t (nr_ue_scheduled_response_f)(nr_scheduled_response_t *scheduled_response);


/*
 * Generic type of an application-defined callback to return various
 * types of data to the application.
 * EXPECTED RETURN VALUES:
 *  -1: Failed to consume bytes. Abort the mission.
 * Non-negative return values indicate success, and ignored.
 */
typedef int8_t (nr_ue_phy_config_request_f)(nr_phy_config_t *phy_config);


/*
 * Generic type of an application-defined callback to return various
 * types of data to the application.
 * EXPECTED RETURN VALUES:
 *  -1: Failed to consume bytes. Abort the mission.
 * Non-negative return values indicate success, and ignored.
 */
typedef int8_t (nr_ue_dl_indication_f)(nr_downlink_indication_t *dl_info);

/*
 * Generic type of an application-defined callback to return various
 * types of data to the application.
 * EXPECTED RETURN VALUES:
 *  -1: Failed to consume bytes. Abort the mission.
 * Non-negative return values indicate success, and ignored.
 */
typedef int8_t (nr_ue_ul_indication_f)(nr_uplink_indication_t *ul_info);

//  TODO check this stuff can be reuse of need modification
typedef struct nr_ue_if_module_s {
    nr_ue_scheduled_response_f *scheduled_response;
    nr_ue_phy_config_request_f *phy_config_request;
    nr_ue_dl_indication_f      *dl_indication;
    nr_ue_ul_indication_f      *ul_indication;

    uint32_t cc_mask;
    uint32_t current_frame;
    uint32_t current_slot;
    //pthread_mutex_t nr_if_mutex;
} nr_ue_if_module_t;


/**\brief reserved one of the interface(if) module instantce from pointer pool and done memory allocation by module_id.
   \param module_id module id*/
nr_ue_if_module_t *nr_ue_if_module_init(uint32_t module_id);


/**\brief done free of memory allocation by module_id and release to pointer pool.
   \param module_id module id*/
int8_t nr_ue_if_module_kill(uint32_t module_id);


/**\brief interface between L1/L2, indicating the downlink related information, like dci_ind and rx_req
   \param dl_info including dci_ind and rx_request messages*/
int8_t nr_ue_dl_indication(nr_downlink_indication_t *dl_info);

//  TODO check
/**\brief handle BCCH-BCH message from dl_indication
   \param pduP            pointer to bch pdu
   \param additional_bits corresponding to 38.212 ch.7
   \param ssb_index       SSB index within 0 - (L_ssb-1) corresponding to 38.331 ch.13 parameter i
   \param ssb_length      corresponding to L1 parameter L_ssb */
int8_t handle_bcch_bch(module_id_t module_id, int cc_id, uint8_t gNB_index, uint8_t *pduP, uint8_t additional_bits, uint32_t ssb_index, uint32_t ssb_length);

//  TODO check
/**\brief handle BCCH-DL-SCH message from dl_indication
   \param pdu_len   length(bytes) of pdu
   \param pduP      pointer to pdu*/
int8_t handle_bcch_dlsch(module_id_t module_id, int cc_id, uint8_t gNB_index, uint32_t sibs_mask, uint8_t *pduP, uint32_t pdu_len);

int8_t handle_dci(module_id_t module_id, int cc_id, uint8_t gNB_index, fapi_nr_dci_pdu_rel15_t *dci, uint16_t rnti, uint32_t dci_type);

#endif

