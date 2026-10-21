/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*
 * \brief functions for FAPI L1 interface
 */

#ifndef __FAPI_NR_UE_L1_H__
#define __FAPI_NR_UE_L1_H__

#include "openair2/NR_UE_PHY_INTERFACE/NR_IF_Module.h"
#include "openair2/LAYER2/NR_MAC_UE/mac_proto.h"

/**\brief NR UE FAPI-like P7 messages, scheduled response from L2 indicating L1
   \param scheduled_response including transmission config(dl_config, ul_config) and data transmission (tx_req)*/
int8_t nr_ue_scheduled_response(nr_scheduled_response_t *scheduled_response);
void sl_handle_scheduled_response(nr_scheduled_response_t *scheduled_response);

int8_t nr_ue_scheduled_response_stub(nr_scheduled_response_t *scheduled_response);

/**\brief NR UE FAPI-like P5 message, physical configuration from L2 to configure L1
   \param scheduled_response including transmission config(dl_config, ul_config) and data transmission (tx_req)*/
void nr_ue_phy_config_request(nr_phy_config_t *phy_config);
void nr_ue_sl_phy_config_request(nr_sl_phy_config_t *phy_config);

/**\brief NR UE FAPI message to schedule a synchronization with target gNB
   \param synch_request including target_Nid_cell*/
void nr_ue_synch_request(nr_synch_request_t *synch_request);

void update_harq_status(NR_UE_MAC_INST_t *mac, uint8_t harq_pid, int cw_idx, uint8_t ack_nack);

#endif
