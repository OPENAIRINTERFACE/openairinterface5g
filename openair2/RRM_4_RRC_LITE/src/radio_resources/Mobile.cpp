#include <iostream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <stdio.h>
#include <string.h>

#include "Asn1Utils.h"
#include "Mobile.h"
#include "Exceptions.h"


//-----------------------------------------------------------------
Mobile::Mobile(mobile_id_t idP)
//-----------------------------------------------------------------
{
    m_id = idP;
    memset(&m_mac_main_config,           0, sizeof(MAC_MainConfig_t));
    //memset(&m_physical_config_dedicated, 0, sizeof(PhysicalConfigDedicated_t));

    for (int i = 0 ; i < MAX_DRB; i++) {
        for (int j = 0 ; j < MAX_ENODE_B_PER_MOBILE; j++) {
            m_drb_to_add_mod[i][j] = NULL;
            m_pending_drb_to_add_mod[i][j] = NULL;
            m_pending_drb_to_release[i][j] = -1;
            m_tx_id_pending_drb_to_add_mod[i][j] = -1;
            m_tx_id_pending_drb_to_release[i][j] = -1;
        }
    }
    for (int i = 0 ; i < MAX_SRB; i++) {
        for (int j = 0 ; j < MAX_ENODE_B_PER_MOBILE; j++) {
            m_srb_to_add_mod[i][j] = NULL;
            m_pending_srb_to_add_mod[i][j] = NULL;
            m_tx_id_pending_srb_to_add_mod[i][j] = -1;
        }
    }

    for (int j = 0 ; j < MAX_ENODE_B_PER_MOBILE; j++) {
        m_cell_index2_cell_id[j]   = 255;
    }
    for (int j = 0 ; j < MAX_ENODE_B; j++) {
            m_cell_id2_cell_index[j]   = 255;
    }
}

//-----------------------------------------------------------------
Mobile::Mobile(cell_id_t cell_idP, mobile_id_t idP, transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
    m_id = idP;
    memset(&m_mac_main_config,           0, sizeof(MAC_MainConfig_t));
    //memset(&m_physical_config_dedicated, 0, sizeof(PhysicalConfigDedicated_t));

    for (int i = 0 ; i < MAX_DRB; i++) {
        for (int j = 0 ; j < MAX_ENODE_B_PER_MOBILE; j++) {
            m_drb_to_add_mod[i][j] = NULL;
            m_pending_drb_to_add_mod[i][j] = NULL;
            m_pending_drb_to_release[i][j] = -1;
            m_tx_id_pending_drb_to_add_mod[i][j] = -1;
            m_tx_id_pending_drb_to_release[i][j] = -1;
        }
    }
    for (int i = 0 ; i < MAX_SRB; i++) {
        for (int j = 0 ; j < MAX_ENODE_B_PER_MOBILE; j++) {
            m_srb_to_add_mod[i][j] = NULL;
            m_pending_srb_to_add_mod[i][j] = NULL;
            m_tx_id_pending_srb_to_add_mod[i][j] = -1;
        }
    }

    for (int j = 0 ; j < MAX_ENODE_B_PER_MOBILE; j++) {
        m_cell_index2_cell_id[j]   = 255;
    }
    for (int j = 0 ; j < MAX_ENODE_B; j++) {
            m_cell_id2_cell_index[j]   = 255;
    }
    signed int cell_index = FindCellIndex(cell_idP);
    AddSignallingRadioBearer1(cell_index, transaction_idP);
}
//-----------------------------------------------------------------
signed int Mobile::FindCellIndex(cell_id_t cell_idP)
//-----------------------------------------------------------------
{
    if ( cell_idP >= MAX_ENODE_B) {
        throw enodeb_identifier_out_of_bounds();
    }

    // already registered
    if ((m_cell_id2_cell_index[cell_idP] >= 0) && (m_cell_id2_cell_index[cell_idP] < MAX_ENODE_B)) {
        return m_cell_id2_cell_index[cell_idP];
    }
    // find an index for the new enodeb
    for (int j = 0 ; j < MAX_ENODE_B_PER_MOBILE; j++) {
        if (!((m_cell_index2_cell_id[j] >= 0) && (m_cell_index2_cell_id[j] < MAX_ENODE_B))) {
            m_cell_id2_cell_index[cell_idP] = j;
            m_cell_index2_cell_id[j]        = cell_idP;
            return j;
        }
    }
    throw too_many_enodeb_for_mobile();
}
//-----------------------------------------------------------------
void Mobile::AddSignallingRadioBearer1(signed int cell_indexP, transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
    SRB_ToAddMod_t* srb = static_cast<SRB_ToAddMod_t*>(CALLOC(1,sizeof(SRB_ToAddMod_t)));
    srb->srb_Identity   = 1;
    srb->rlc_Config     = static_cast<SRB_ToAddMod::SRB_ToAddMod__rlc_Config*>(CALLOC(1,sizeof(SRB_ToAddMod::SRB_ToAddMod__rlc_Config)));
    srb->rlc_Config->present = SRB_ToAddMod__rlc_Config_PR_explicitValue;
    srb->rlc_Config->choice.explicitValue.present = RLC_Config_PR_am;
    srb->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms50;
    srb->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU          = PollPDU_p4;
    srb->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte         = PollByte_kB25;
    srb->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t32;

    srb->rlc_Config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering     = T_Reordering_ms100;
    srb->rlc_Config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms5;


    srb->logicalChannelConfig = static_cast<SRB_ToAddMod::SRB_ToAddMod__logicalChannelConfig*>(CALLOC(1,sizeof(SRB_ToAddMod::SRB_ToAddMod__logicalChannelConfig)));
    srb->logicalChannelConfig->present = SRB_ToAddMod__logicalChannelConfig_PR_explicitValue;
    srb->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters = static_cast<LogicalChannelConfig::LogicalChannelConfig__ul_SpecificParameters*>(CALLOC(1,sizeof(LogicalChannelConfig::LogicalChannelConfig__ul_SpecificParameters)));

    srb->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->priority            = 1;
    srb->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->prioritisedBitRate  = LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
    srb->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->bucketSizeDuration  = LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;
    srb->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->logicalChannelGroup = 0;

    m_pending_srb_to_add_mod[srb->srb_Identity-1][cell_indexP]       = srb;
    m_tx_id_pending_srb_to_add_mod[srb->srb_Identity-1][cell_indexP] = transaction_idP;
}
//-----------------------------------------------------------------
void Mobile::AddSignallingRadioBearer2(signed int cell_indexP, transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
    SRB_ToAddMod_t* srb = static_cast<SRB_ToAddMod_t*>(CALLOC(1,sizeof(SRB_ToAddMod_t)));
    srb->srb_Identity   = 2;
    srb->rlc_Config     = static_cast<SRB_ToAddMod::SRB_ToAddMod__rlc_Config*>(CALLOC(1,sizeof(SRB_ToAddMod::SRB_ToAddMod__rlc_Config)));
    srb->rlc_Config->present = SRB_ToAddMod__rlc_Config_PR_explicitValue;
    srb->rlc_Config->choice.explicitValue.present = RLC_Config_PR_am;
    srb->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms50;
    srb->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU          = PollPDU_p4;
    srb->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte         = PollByte_kB25;
    srb->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t32;

    srb->rlc_Config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering     = T_Reordering_ms100;
    srb->rlc_Config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms5;


    srb->logicalChannelConfig = static_cast<SRB_ToAddMod::SRB_ToAddMod__logicalChannelConfig*>(CALLOC(1,sizeof(SRB_ToAddMod::SRB_ToAddMod__logicalChannelConfig)));
    srb->logicalChannelConfig->present = SRB_ToAddMod__logicalChannelConfig_PR_explicitValue;
    srb->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters = static_cast<LogicalChannelConfig::LogicalChannelConfig__ul_SpecificParameters*>(CALLOC(1,sizeof(LogicalChannelConfig::LogicalChannelConfig__ul_SpecificParameters)));

    srb->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->priority            = 1;
    srb->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->prioritisedBitRate  = LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
    srb->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->bucketSizeDuration  = LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;
    srb->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->logicalChannelGroup = 0;

    m_pending_srb_to_add_mod[srb->srb_Identity-1][cell_indexP]       = srb;
    m_tx_id_pending_srb_to_add_mod[srb->srb_Identity-1][cell_indexP] = transaction_idP;
}
//-----------------------------------------------------------------
void Mobile::AddDefaultDataRadioBearer(signed int cell_indexP, transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{

    DRB_ToAddMod_t* drb = static_cast<DRB_ToAddMod_t*>(CALLOC(1,sizeof(DRB_ToAddMod_t)));
    drb->drb_Identity   = 3;
    drb->rlc_Config     = static_cast<RLC_Config_t*>(CALLOC(1,sizeof(RLC_Config_t)));
    drb->rlc_Config->present = RLC_Config_PR_am;
    drb->rlc_Config->choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms50;
    drb->rlc_Config->choice.am.ul_AM_RLC.pollPDU          = PollPDU_p4;
    drb->rlc_Config->choice.am.ul_AM_RLC.pollByte         = PollByte_kB25;
    drb->rlc_Config->choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t32;

    drb->rlc_Config->choice.am.dl_AM_RLC.t_Reordering     = T_Reordering_ms100;
    drb->rlc_Config->choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms5;


    drb->logicalChannelConfig = static_cast<LogicalChannelConfig_t*>(CALLOC(1,sizeof(LogicalChannelConfig_t)));
    drb->logicalChannelConfig->ul_SpecificParameters = static_cast<LogicalChannelConfig::LogicalChannelConfig__ul_SpecificParameters*>(CALLOC(1,sizeof(LogicalChannelConfig::LogicalChannelConfig__ul_SpecificParameters)));

    drb->logicalChannelConfig->ul_SpecificParameters->priority            = 1;
    drb->logicalChannelConfig->ul_SpecificParameters->prioritisedBitRate  = LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
    drb->logicalChannelConfig->ul_SpecificParameters->bucketSizeDuration  = LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;
    drb->logicalChannelConfig->ul_SpecificParameters->logicalChannelGroup = 0;

    m_pending_drb_to_add_mod[drb->drb_Identity-1][cell_indexP]       = drb;
    m_tx_id_pending_drb_to_add_mod[drb->drb_Identity-1][cell_indexP] = transaction_idP;
}
//-----------------------------------------------------------------
void Mobile::CommitTransaction(transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
    for (int i = 0 ; i < MAX_DRB; i++) {
        for (int j = 0 ; j < MAX_ENODE_B_PER_MOBILE; j++) {
            // Look if there is a transaction for removing a DRB
            if (m_tx_id_pending_drb_to_release[i][j] == transaction_idP) {
                if (m_drb_to_add_mod[i][j] == NULL) {
                    throw transaction_overwrite_error();
                } else {
                    asn_DEF_DRB_ToAddMod.free_struct( &asn_DEF_DRB_ToAddMod, m_drb_to_add_mod[i][j],0);
                    cerr << "[RRM] Mobile::CommitTransaction(" << transaction_idP << ") free struct  m_drb_to_add_mod["<< i << "][" << j << "]" << endl;
                    m_drb_to_add_mod[i][j] = NULL;
                }
                m_tx_id_pending_drb_to_release[i][j] = -1;
            }
            if (m_tx_id_pending_drb_to_add_mod[i][j] == transaction_idP) {
                if (m_drb_to_add_mod[i][j] == NULL) {
                    m_drb_to_add_mod[i][j] = m_pending_drb_to_add_mod[i][j];
                    cerr << "[RRM] Mobile::CommitTransaction(" << transaction_idP << ") add struct  m_pending_drb_to_add_mod["<< i << "][" << j << "]" << endl;
                    m_pending_drb_to_add_mod[i][j] = NULL;
                } else {
                    throw transaction_overwrite_error();
                }
                m_tx_id_pending_drb_to_add_mod[i][j] = -1;
            }
        }
    }
    for (int i = 0 ; i < MAX_SRB; i++) {
        for (int j = 0 ; j < MAX_ENODE_B_PER_MOBILE; j++) {
            if (m_tx_id_pending_srb_to_add_mod[i][j] == transaction_idP) {
                if (m_srb_to_add_mod[i][j] == NULL) {
                    m_srb_to_add_mod[i][j] = m_pending_srb_to_add_mod[i][j];
                    cerr << "[RRM] Mobile::CommitTransaction(" << transaction_idP << ") add struct  m_pending_srb_to_add_mod["<< i << "][" << j << "]" << endl;
                    m_pending_srb_to_add_mod[i][j] = NULL;
                } else {
                    throw transaction_overwrite_error();
                }
                m_tx_id_pending_srb_to_add_mod[i][j] = -1;
            }
        }
    }
}
//-----------------------------------------------------------------
RadioResourceConfigDedicated_t* Mobile::GetASN1RadioResourceConfigDedicated(transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
    RadioResourceConfigDedicated_t* config = static_cast<RadioResourceConfigDedicated_t*>(CALLOC(1,sizeof(RadioResourceConfigDedicated_t)));

    config->srb_ToAddModList = NULL;
    config->drb_ToReleaseList = NULL;
    config->drb_ToAddModList  = NULL;

    for (int i = 0 ; i < MAX_DRB; i++) {
        for (int j = 0 ; j < MAX_ENODE_B_PER_MOBILE; j++) {
            // Look if there is a transaction for removing a DRB
            if (m_tx_id_pending_drb_to_release[i][j] == transaction_idP) {
                if (config->drb_ToReleaseList == NULL) {
                    config->drb_ToReleaseList = static_cast<DRB_ToReleaseList_t*>(CALLOC(1,sizeof(DRB_ToReleaseList_t)));
                }
                cerr << "[RRM] Mobile::GetASN1RadioResourceConfigDedicated(" << transaction_idP << ") Releasing DRB "<< m_pending_drb_to_release[i][j] << endl;
                ASN_SEQUENCE_ADD(&config->drb_ToReleaseList->list,Asn1Utils::Clone(&m_pending_drb_to_release[i][j]));
            }
            if (m_tx_id_pending_drb_to_add_mod[i][j] == transaction_idP) {
                if (config->drb_ToAddModList == NULL) {
                    config->drb_ToAddModList = static_cast<DRB_ToAddModList_t*>(CALLOC(1,sizeof(DRB_ToAddModList_t)));
                }
                cerr << "[RRM] Mobile::GetASN1RadioResourceConfigDedicated(" << transaction_idP << ") Adding DRB "<< m_pending_drb_to_add_mod[i][j] << endl;
                ASN_SEQUENCE_ADD(&config->drb_ToAddModList->list, Asn1Utils::Clone(m_pending_drb_to_add_mod[i][j]));
            }
        }
    }
    for (int i = 0 ; i < MAX_SRB; i++) {
        for (int j = 0 ; j < MAX_ENODE_B_PER_MOBILE; j++) {
            if (m_tx_id_pending_srb_to_add_mod[i][j] == transaction_idP) {
                if (config->srb_ToAddModList == NULL) {
                    config->srb_ToAddModList = static_cast<SRB_ToAddModList_t*>(CALLOC(1,sizeof(SRB_ToAddModList_t)));
                }
                cerr << "[RRM] Mobile::GetASN1RadioResourceConfigDedicated(" << transaction_idP << ") Adding SRB "<< m_pending_srb_to_add_mod[i][j] << endl;
                ASN_SEQUENCE_ADD(&config->srb_ToAddModList->list, Asn1Utils::Clone(m_pending_srb_to_add_mod[i][j]));
            }
        }
    }

    config->mac_MainConfig                = static_cast<RadioResourceConfigDedicated::RadioResourceConfigDedicated__mac_MainConfig*>(CALLOC(1,sizeof(RadioResourceConfigDedicated::RadioResourceConfigDedicated__mac_MainConfig)));
    config->mac_MainConfig->present       = RadioResourceConfigDedicated__mac_MainConfig_PR_defaultValue;
    return config;
}
//-----------------------------------------------------------------
void Mobile::RemoveDataRadioBearer(ENodeB* e_node_bP,
                                rb_id_t            radio_bearer_idP,
                                Transaction*       transactionP)
//-----------------------------------------------------------------
{
    signed int cell_index = m_cell_id2_cell_index[e_node_bP->GetId()];
}
//-----------------------------------------------------------------
Mobile::~Mobile()
//-----------------------------------------------------------------
{
    asn_DEF_MAC_MainConfig.free_struct(&asn_DEF_MAC_MainConfig, &m_mac_main_config, 1);
    //asn_DEF_PhysicalConfigDedicated.free_struct(&asn_DEF_PhysicalConfigDedicated, &m_physical_config_dedicated, 1);

    for (int i = 0 ; i < MAX_DRB; i++) {
        for (int j = 0 ; j < MAX_ENODE_B_PER_MOBILE; j++) {
            if (m_drb_to_add_mod[i][j] != NULL)  {
                asn_DEF_DRB_ToAddMod.free_struct(&asn_DEF_DRB_ToAddMod, m_drb_to_add_mod[i][j], 0);
            }
            if (m_pending_drb_to_add_mod[i][j] != NULL)  {
                asn_DEF_DRB_ToAddMod.free_struct(&asn_DEF_DRB_ToAddMod, m_pending_drb_to_add_mod[i][j], 0);
            }
        }
    }
    for (int i = 0 ; i < MAX_SRB; i++) {
        for (int j = 0 ; j < MAX_ENODE_B_PER_MOBILE; j++) {
            if (m_srb_to_add_mod[i][j] != NULL)  {
                asn_DEF_SRB_ToAddMod.free_struct(&asn_DEF_SRB_ToAddMod, m_srb_to_add_mod[i][j], 0);
            }
            if (m_pending_srb_to_add_mod[i][j] != NULL)  {
                asn_DEF_SRB_ToAddMod.free_struct(&asn_DEF_SRB_ToAddMod, m_pending_srb_to_add_mod[i][j], 0);
            }
        }
    }
}

