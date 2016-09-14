#include <iostream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <stdio.h>
#include <string.h>

#include "ENodeB.h"
#include "Exceptions.h"
#include "Asn1Utils.h"

//static uint8_t    g_message_serialize_buffer[MESSAGE_SERIALIZE_BUFFER_SIZE];

//-----------------------------------------------------------------
ENodeB::ENodeB(cell_id_t idP)
//-----------------------------------------------------------------
{
    m_id = idP;

    for (int i = 0 ; i < MAX_DRB; i++) {
        for (int j = 0 ; j < MAX_MOBILE_PER_ENODE_B; j++) {
            m_drb_to_add_mod[i][j]                = NULL;
            m_tx_id_pending_drb_to_add_mod[i][j]  = -1;
            m_pending_drb_to_release[i][j]        = NULL;
            m_tx_id_pending_drb_to_release[i][j]  = -1;
        }
    }
    for (int i = 0 ; i < MAX_SRB; i++) {
        for (int j = 0 ; j < MAX_MOBILE_PER_ENODE_B; j++) {
            m_srb_to_add_mod[i][j] = NULL;
            m_tx_id_pending_srb_to_add_mod[i][j]  = -1;
        }
    }
    //m_radio_resource_config_dedicated = NULL;
}
//-----------------------------------------------------------------
 RRM_RRC_Message_t* ENodeB::AddUserRequest(Mobile* mobileP,  transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
    RRM_RRC_Message_t*                    message;

    AddSignallingRadioBearer1(mobileP->GetId(), transaction_idP);
    message                               = static_cast<RRM_RRC_Message_t*>(CALLOC(1,sizeof(RRM_RRC_Message_t)));

    message->message.present                                                         = RRM_RRC_MessageType_PR_c1;
    message->message.choice.c1.present                                               = RRM_RRC_MessageType__c1_PR_rrcAddUserResponse;
    message->message.choice.c1.choice.rrcAddUserResponse.transactionId               = transaction_idP;
    message->message.choice.c1.choice.rrcAddUserResponse.rrm_Response_Status         = OpenAir_RRM_Response_Status_success;
    message->message.choice.c1.choice.rrcAddUserResponse.rrm_Response_Reason         = OpenAir_RRM_Response_Reason_ok;
    message->message.choice.c1.choice.rrcAddUserResponse.e_NodeB_Identity.physCellId = m_id;
    message->message.choice.c1.choice.rrcAddUserResponse.c_RNTI                      = mobileP->GetId();;

    message->message.choice.c1.choice.rrcAddUserResponse.radioResourceConfigDedicatedE_NodeB = GetASN1RadioResourceConfigDedicated(transaction_idP);
    message->message.choice.c1.choice.rrcAddUserResponse.radioResourceConfigDedicatedMobile  = mobileP->GetASN1RadioResourceConfigDedicated(transaction_idP);

    //xer_fprint(stdout, &asn_DEF_RRM_RRC_Message, (void*)message);

    return message;
}
//-----------------------------------------------------------------
 RRM_RRC_Message_t* ENodeB::AddUserConfirm(Mobile* mobileP,  transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
    //----------------------------------------------------------------------
    // Commit transaction here
    mobileP->CommitTransaction(transaction_idP);
    CommitTransaction(transaction_idP);
    //----------------------------------------------------------------------
    // Then send a reconfiguration message to the RRC
    //
    AddSignallingRadioBearer2(mobileP->GetId(), transaction_idP);
    AddDefaultDataRadioBearer(mobileP->GetId(), transaction_idP);
    mobileP->AddSignallingRadioBearer2(m_id, transaction_idP);
    mobileP->AddDefaultDataRadioBearer(m_id, transaction_idP);

    RRM_RRC_Message_t*                    message                                        = static_cast<RRM_RRC_Message_t*>(CALLOC(1,sizeof(RRM_RRC_Message_t)));

    message->message.present                                                             = RRM_RRC_MessageType_PR_c1;
    message->message.choice.c1.present                                                   = RRM_RRC_MessageType__c1_PR_rrcUserReconfiguration;
    message->message.choice.c1.choice.rrcUserReconfiguration.transactionId               = transaction_idP;
    message->message.choice.c1.choice.rrcUserReconfiguration.rrm_Response_Status         = OpenAir_RRM_Response_Status_success;
    message->message.choice.c1.choice.rrcUserReconfiguration.rrm_Response_Reason         = OpenAir_RRM_Response_Reason_ok;
    message->message.choice.c1.choice.rrcUserReconfiguration.e_NodeB_Identity.physCellId = m_id;
    message->message.choice.c1.choice.rrcUserReconfiguration.c_RNTI                      = mobileP->GetId();
    // ENodeB
    message->message.choice.c1.choice.rrcUserReconfiguration.radioResourceConfigDedicatedE_NodeB = GetASN1RadioResourceConfigDedicated(transaction_idP);
    // Mobile
    message->message.choice.c1.choice.rrcUserReconfiguration.radioResourceConfigDedicatedMobile  = mobileP->GetASN1RadioResourceConfigDedicated(transaction_idP);
    xer_fprint(stdout, &asn_DEF_RRM_RRC_Message, (void*)message);

    return message;
}
//-----------------------------------------------------------------
 void ENodeB::UserReconfigurationComplete(Mobile* mobileP,  transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
    //----------------------------------------------------------------------
    // Commit transaction here
    mobileP->CommitTransaction(transaction_idP);
    CommitTransaction(transaction_idP);
}
//-----------------------------------------------------------------
 RRM_RRC_Message_t* ENodeB::RemoveUserRequest(Mobile* mobileP, transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
    RRM_RRC_Message_t*                    message;
    return message;
}
//-----------------------------------------------------------------
void ENodeB::AddSignallingRadioBearer1(mobile_id_t mobile_idP, transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
    cout << "[RRM] ENodeB::AddSignallingRadioBearer1(mobile="<< mobile_idP << " transaction_id="<< transaction_idP << endl;
    SRB_ToAddMod_t* srb1 = static_cast<SRB_ToAddMod_t*>(CALLOC(1,sizeof(SRB_ToAddMod_t)));
    srb1->srb_Identity   = 1;
    srb1->rlc_Config     = static_cast<SRB_ToAddMod::SRB_ToAddMod__rlc_Config*>(CALLOC(1,sizeof(SRB_ToAddMod::SRB_ToAddMod__rlc_Config)));
    srb1->rlc_Config->present = SRB_ToAddMod__rlc_Config_PR_explicitValue;
    srb1->rlc_Config->choice.explicitValue.present = RLC_Config_PR_am;
    srb1->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms50;
    srb1->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU          = PollPDU_p4;
    srb1->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte         = PollByte_kB25;
    srb1->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t32;

    srb1->rlc_Config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering     = T_Reordering_ms100;
    srb1->rlc_Config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms5;


    srb1->logicalChannelConfig = static_cast<SRB_ToAddMod::SRB_ToAddMod__logicalChannelConfig*>(CALLOC(1,sizeof(SRB_ToAddMod::SRB_ToAddMod__logicalChannelConfig)));
    srb1->logicalChannelConfig->present = SRB_ToAddMod__logicalChannelConfig_PR_explicitValue;
    srb1->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters = static_cast<LogicalChannelConfig::LogicalChannelConfig__ul_SpecificParameters*>(CALLOC(1,sizeof(LogicalChannelConfig::LogicalChannelConfig__ul_SpecificParameters)));

    srb1->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->priority            = 1;
    srb1->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->prioritisedBitRate  = LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
    srb1->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->bucketSizeDuration  = LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;
    srb1->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->logicalChannelGroup = 0;

    m_pending_srb_to_add_mod[srb1->srb_Identity-1][mobile_idP]       = srb1;
    m_tx_id_pending_srb_to_add_mod[srb1->srb_Identity-1][mobile_idP] = transaction_idP;
}
//-----------------------------------------------------------------
void ENodeB::AddSignallingRadioBearer2(mobile_id_t mobile_idP, transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
    cout << "[RRM] ENodeB::AddSignallingRadioBearer2(mobile="<< mobile_idP << " transaction_id="<< transaction_idP << endl;
    SRB_ToAddMod_t* srb2 = static_cast<SRB_ToAddMod_t*>(CALLOC(1,sizeof(SRB_ToAddMod_t)));
    srb2->srb_Identity   = 2;
    srb2->rlc_Config     = static_cast<SRB_ToAddMod::SRB_ToAddMod__rlc_Config*>(CALLOC(1,sizeof(SRB_ToAddMod::SRB_ToAddMod__rlc_Config)));
    srb2->rlc_Config->present = SRB_ToAddMod__rlc_Config_PR_explicitValue;
    srb2->rlc_Config->choice.explicitValue.present = RLC_Config_PR_am;
    srb2->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.t_PollRetransmit = T_PollRetransmit_ms50;
    srb2->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.pollPDU          = PollPDU_p4;
    srb2->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.pollByte         = PollByte_kB25;
    srb2->rlc_Config->choice.explicitValue.choice.am.ul_AM_RLC.maxRetxThreshold = UL_AM_RLC__maxRetxThreshold_t32;

    srb2->rlc_Config->choice.explicitValue.choice.am.dl_AM_RLC.t_Reordering     = T_Reordering_ms100;
    srb2->rlc_Config->choice.explicitValue.choice.am.dl_AM_RLC.t_StatusProhibit = T_StatusProhibit_ms5;


    srb2->logicalChannelConfig = static_cast<SRB_ToAddMod::SRB_ToAddMod__logicalChannelConfig*>(CALLOC(1,sizeof(SRB_ToAddMod::SRB_ToAddMod__logicalChannelConfig)));
    srb2->logicalChannelConfig->present = SRB_ToAddMod__logicalChannelConfig_PR_explicitValue;
    srb2->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters = static_cast<LogicalChannelConfig::LogicalChannelConfig__ul_SpecificParameters*>(CALLOC(1,sizeof(LogicalChannelConfig::LogicalChannelConfig__ul_SpecificParameters)));

    srb2->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->priority            = 1;
    srb2->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->prioritisedBitRate  = LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
    srb2->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->bucketSizeDuration  = LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;
    srb2->logicalChannelConfig->choice.explicitValue.ul_SpecificParameters->logicalChannelGroup = 0;

    m_pending_srb_to_add_mod[srb2->srb_Identity-1][mobile_idP]       = srb2;
    m_tx_id_pending_srb_to_add_mod[srb2->srb_Identity-1][mobile_idP] = transaction_idP;
}
//-----------------------------------------------------------------
void ENodeB::AddDefaultDataRadioBearer(mobile_id_t mobile_idP, transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
    cout << "[RRM] ENodeB::AddDefaultDataRadioBearer(mobile="<< mobile_idP << " transaction_id="<< transaction_idP << endl;

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

    m_pending_drb_to_add_mod[drb->drb_Identity-1][mobile_idP]       = drb;
    m_tx_id_pending_drb_to_add_mod[drb->drb_Identity-1][mobile_idP] = transaction_idP;
}
//-----------------------------------------------------------------
void ENodeB::RemoveAllDataRadioBearers(Mobile* mobileP, transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
/*
    for (int i = 3 ; i < MAX_RB_PER_MOBILE; i++) {
        mobile_id_t mobile_id = mobileP->GetId();
        if (m_radio_bearer_array[i][mobile_id] != NULL) {
            Command* command = new Command(COMMAND_EQUIPMENT_RADIO_GATEWAY,
                                   m_id,
                                   COMMAND_ACTION_REMOVE,
                                   COMMAND_OBJECT_DATA_RADIO_BEARER,
                                   i + (mobile_id * MAX_RB_PER_MOBILE));
            transactionP->AddCommand(command);
            RadioBearer* rb = m_radio_bearer_array[i][mobile_id];
            m_radio_bearer_array[i][mobile_id] =  NULL;
            delete rb;
        }
    }
*/
}

//-----------------------------------------------------------------
ENodeB::~ENodeB()
//-----------------------------------------------------------------
{
    //if (!m_radio_bearer_map.empty())  m_radio_bearer_map.clear();
    //if (!m_mobile_map.empty())        m_mobile_map.clear();

    for (int i = 0 ; i < MAX_DRB; i++) {
        for (int j = 0 ; j < MAX_MOBILE_PER_ENODE_B; j++) {
            if (m_drb_to_add_mod[i][j] != NULL) {
                asn_DEF_DRB_ToAddMod.free_struct(&asn_DEF_DRB_ToAddMod, m_drb_to_add_mod[i][j], 0);
            }
        }
    }
    for (int i = 0 ; i < MAX_SRB; i++) {
        for (int j = 0 ; j < MAX_MOBILE_PER_ENODE_B; j++) {
            if (m_srb_to_add_mod[i][j] != NULL) {
                asn_DEF_SRB_ToAddMod.free_struct(&asn_DEF_SRB_ToAddMod, m_srb_to_add_mod[i][j], 0);
            }
        }
    }
    /*if (m_radio_resource_config_dedicated != NULL) {
        asn_DEF_RadioResourceConfigDedicated.free_struct(&asn_DEF_RadioResourceConfigDedicated, m_radio_resource_config_dedicated, 0);
    }*/
}
//-----------------------------------------------------------------
void ENodeB::CommitTransaction(transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
    for (int i = 0 ; i < MAX_DRB; i++) {
        for (int j = 0 ; j < MAX_MOBILE_PER_ENODE_B; j++) {
            // Look if there is a transaction for removing a DRB
            if (m_tx_id_pending_drb_to_release[i][j] == transaction_idP) {
                if (m_drb_to_add_mod[i][j] == NULL) {
                    throw transaction_overwrite_error();
                } else {
                    cerr << "[RRM] ENodeB::CommitTransaction(" << transaction_idP << ") free_struct  m_drb_to_add_mod["<< i << "][" << j << "]" << endl;
                    asn_DEF_DRB_ToAddMod.free_struct( &asn_DEF_DRB_ToAddMod, m_drb_to_add_mod[i][j],0);
                    m_drb_to_add_mod[i][j] = NULL;
                }
                m_tx_id_pending_drb_to_release[i][j] = -1;
            }
            if (m_tx_id_pending_drb_to_add_mod[i][j] == transaction_idP) {
                if (m_drb_to_add_mod[i][j] == NULL) {
                    m_drb_to_add_mod[i][j] = m_pending_drb_to_add_mod[i][j];
                    cerr << "[RRM] ENodeB::CommitTransaction(" << transaction_idP << ") add struct  m_pending_drb_to_add_mod["<< i << "][" << j << "]" << endl;
                    m_pending_drb_to_add_mod[i][j] = NULL;
                } else {
                    throw transaction_overwrite_error();
                }
                m_tx_id_pending_drb_to_add_mod[i][j] = -1;
            }
        }
    }
    for (int i = 0 ; i < MAX_SRB; i++) {
        for (int j = 0 ; j < MAX_MOBILE_PER_ENODE_B; j++) {
            if (m_tx_id_pending_srb_to_add_mod[i][j] == transaction_idP) {
                if (m_srb_to_add_mod[i][j] == NULL) {
                    m_srb_to_add_mod[i][j] = m_pending_srb_to_add_mod[i][j];
                    cerr << "[RRM] ENodeB::CommitTransaction(" << transaction_idP << ") add struct  m_pending_srb_to_add_mod["<< i << "][" << j << "]" << endl;
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
RadioResourceConfigDedicated_t* ENodeB::GetASN1RadioResourceConfigDedicated(transaction_id_t transaction_idP)
//-----------------------------------------------------------------
{
    RadioResourceConfigDedicated_t* config = static_cast<RadioResourceConfigDedicated_t*>(CALLOC(1,sizeof(RadioResourceConfigDedicated_t)));

    config->srb_ToAddModList = NULL;
    config->drb_ToReleaseList = NULL;
    config->drb_ToAddModList  = NULL;

    for (int i = 0 ; i < MAX_DRB; i++) {
        for (int j = 0 ; j < MAX_MOBILE_PER_ENODE_B; j++) {
            // Look if there is a transaction for removing a DRB
            if (m_tx_id_pending_drb_to_release[i][j] == transaction_idP) {
                if (config->drb_ToReleaseList == NULL) {
                    config->drb_ToReleaseList = static_cast<DRB_ToReleaseList_t*>(CALLOC(1,sizeof(DRB_ToReleaseList_t)));
                }
                cerr << "[RRM] ENodeB::GetASN1RadioResourceConfigDedicated(" << transaction_idP << ") Releasing DRB "<< m_pending_drb_to_release[i][j] << endl;
                ASN_SEQUENCE_ADD(&config->drb_ToReleaseList->list,Asn1Utils::Clone(&m_pending_drb_to_release[i][j]));
            }
            if (m_tx_id_pending_drb_to_add_mod[i][j] == transaction_idP) {
                if (config->drb_ToAddModList == NULL) {
                    config->drb_ToAddModList = static_cast<DRB_ToAddModList_t*>(CALLOC(1,sizeof(DRB_ToAddModList_t)));
                }
                cerr << "[RRM] ENodeB::GetASN1RadioResourceConfigDedicated(" << transaction_idP << ") Adding DRB "<< m_pending_drb_to_add_mod[i][j] << endl;
                ASN_SEQUENCE_ADD(&config->drb_ToAddModList->list, Asn1Utils::Clone(m_pending_drb_to_add_mod[i][j]));
            }
        }
    }
    for (int i = 0 ; i < MAX_SRB; i++) {
        for (int j = 0 ; j < MAX_MOBILE_PER_ENODE_B; j++) {
            if (m_tx_id_pending_srb_to_add_mod[i][j] == transaction_idP) {
                if (config->srb_ToAddModList == NULL) {
                    config->srb_ToAddModList = static_cast<SRB_ToAddModList_t*>(CALLOC(1,sizeof(SRB_ToAddModList_t)));
                }
                cerr << "[RRM] ENodeB::GetASN1RadioResourceConfigDedicated(" << transaction_idP << ") Adding SRB "<< m_pending_srb_to_add_mod[i][j] << endl;
                ASN_SEQUENCE_ADD(&config->srb_ToAddModList->list, Asn1Utils::Clone(m_pending_srb_to_add_mod[i][j]));
            }
        }
    }

    config->mac_MainConfig                = static_cast<RadioResourceConfigDedicated::RadioResourceConfigDedicated__mac_MainConfig*>(CALLOC(1,sizeof(RadioResourceConfigDedicated::RadioResourceConfigDedicated__mac_MainConfig)));
    config->mac_MainConfig->present       = RadioResourceConfigDedicated__mac_MainConfig_PR_defaultValue;
    return config;
}
//-----------------------------------------------------------------
RRM_RRC_Message_t* ENodeB::GetRRMRRCConfigurationMessage(transaction_id_t transaction_idP) {
//-----------------------------------------------------------------
    RRM_RRC_Message_t*                    message;
    SystemInformation_t*                  system_information;
    SystemInformationBlockType1_t*        sib1;

    message              = static_cast<RRM_RRC_Message_t*>(CALLOC(1,sizeof(RRM_RRC_Message_t)));
    system_information   = static_cast<SystemInformation_t*>(CALLOC(1,sizeof(SystemInformation_t)));
    sib1                 = static_cast<SystemInformationBlockType1_t*>(CALLOC(1,sizeof(SystemInformationBlockType1_t)));

    message->message.present                                                                     = RRM_RRC_MessageType_PR_c1;
    message->message.choice.c1.present                                                           = RRM_RRC_MessageType__c1_PR_rrcSystemConfigurationResponse;
    message->message.choice.c1.choice.rrcSystemConfigurationResponse.e_NodeB_Identity.physCellId = m_id;
    message->message.choice.c1.choice.rrcSystemConfigurationResponse.transactionId               = transaction_idP;


    //-----------------------------------------------------------------------------
    // SIB1 CONSTRUCTION
    //-----------------------------------------------------------------------------
    PLMN_IdentityInfo_t* PLMN_identity_info;
    MCC_MNC_Digit_t*      dummy;
    SchedulingInfo_t*    schedulingInfo;
    SIB_Type_t*          sib_type;


    memset(sib1,0,sizeof(SystemInformationBlockType1_t));

    PLMN_identity_info    = static_cast<PLMN_IdentityInfo_t*>(CALLOC(1,sizeof(PLMN_IdentityInfo_t)));
    schedulingInfo        = static_cast<SchedulingInfo_t*>(CALLOC(1,sizeof(SchedulingInfo_t)));
    sib_type              = static_cast<SIB_Type_t*>(CALLOC(1,sizeof(SIB_Type_t)));

    memset(PLMN_identity_info,0,sizeof(PLMN_IdentityInfo_t));
    memset(schedulingInfo,0,sizeof(SchedulingInfo_t));
    memset(sib_type,0,sizeof(SIB_Type_t));

    PLMN_identity_info->plmn_Identity.mcc = static_cast<MCC_t*>(CALLOC(1,sizeof(MCC_t)));
    memset(PLMN_identity_info->plmn_Identity.mcc,0,sizeof(MCC_t));

    asn_set_empty(&PLMN_identity_info->plmn_Identity.mcc->list);//.size=0;


    dummy                 = static_cast<MCC_MNC_Digit_t*>(CALLOC(1,sizeof(MCC_MNC_Digit_t)));
    *dummy=2;ASN_SEQUENCE_ADD(&PLMN_identity_info->plmn_Identity.mcc->list, dummy);

    dummy                 = static_cast<MCC_MNC_Digit_t*>(CALLOC(1,sizeof(MCC_MNC_Digit_t)));
    *dummy=6;ASN_SEQUENCE_ADD(&PLMN_identity_info->plmn_Identity.mcc->list, dummy);

    dummy                 = static_cast<MCC_MNC_Digit_t*>(CALLOC(1,sizeof(MCC_MNC_Digit_t)));
    *dummy=2;ASN_SEQUENCE_ADD(&PLMN_identity_info->plmn_Identity.mcc->list, dummy);


    //dummy=2;ASN_SEQUENCE_ADD(&PLMN_identity_info->plmn_Identity.mcc->list, &dummy);
    //dummy=6;ASN_SEQUENCE_ADD(&PLMN_identity_info->plmn_Identity.mcc->list, &dummy);
    //dummy=2;ASN_SEQUENCE_ADD(&PLMN_identity_info->plmn_Identity.mcc->list, &dummy);

    asn_set_empty(&PLMN_identity_info->plmn_Identity.mnc.list);//.size=0;
    //PLMN_identity_info->plmn_Identity.mnc.list.size=0;
    //PLMN_identity_info->plmn_Identity.mnc.list.count=0;

    dummy                 = static_cast<MCC_MNC_Digit_t*>(CALLOC(1,sizeof(MCC_MNC_Digit_t)));
    *dummy=8;ASN_SEQUENCE_ADD(&PLMN_identity_info->plmn_Identity.mnc.list, dummy);

    dummy                 = static_cast<MCC_MNC_Digit_t*>(CALLOC(1,sizeof(MCC_MNC_Digit_t)));
    *dummy=8;ASN_SEQUENCE_ADD(&PLMN_identity_info->plmn_Identity.mnc.list, dummy);

    //dummy=8;ASN_SEQUENCE_ADD(&PLMN_identity_info->plmn_Identity.mnc.list,&dummy);
    //dummy=0;ASN_SEQUENCE_ADD(&PLMN_identity_info->plmn_Identity.mnc.list,&dummy);
    //assign_enum(&PLMN_identity_info->cellReservedForOperatorUse,PLMN_IdentityInfo__cellReservedForOperatorUse_notReserved);
    PLMN_identity_info->cellReservedForOperatorUse=PLMN_IdentityInfo__cellReservedForOperatorUse_notReserved;


    message->message.choice.c1.choice.rrcSystemConfigurationResponse.systemInformationBlockType1 = sib1;

    ASN_SEQUENCE_ADD(&sib1->cellAccessRelatedInfo.plmn_IdentityList.list,PLMN_identity_info);


    // 16 bits
    sib1->cellAccessRelatedInfo.trackingAreaCode.buf = static_cast<uint8_t*>(MALLOC(2));
    sib1->cellAccessRelatedInfo.trackingAreaCode.buf[0]=0x00;
    sib1->cellAccessRelatedInfo.trackingAreaCode.buf[1]=0x10;
    sib1->cellAccessRelatedInfo.trackingAreaCode.size=2;
    sib1->cellAccessRelatedInfo.trackingAreaCode.bits_unused=0;

    // 28 bits
    sib1->cellAccessRelatedInfo.cellIdentity.buf = static_cast<uint8_t*>(MALLOC(8));
    sib1->cellAccessRelatedInfo.cellIdentity.buf[0]=0x01;
    sib1->cellAccessRelatedInfo.cellIdentity.buf[1]=0x48;
    sib1->cellAccessRelatedInfo.cellIdentity.buf[2]=0x0f;
    sib1->cellAccessRelatedInfo.cellIdentity.buf[3]=0x03;
    sib1->cellAccessRelatedInfo.cellIdentity.size=4;
    sib1->cellAccessRelatedInfo.cellIdentity.bits_unused=4;

    //  assign_enum(&sib1->cellAccessRelatedInfo.cellBarred,SystemInformationBlockType1__cellAccessRelatedInfo__cellBarred_notBarred);
    sib1->cellAccessRelatedInfo.cellBarred=SystemInformationBlockType1__cellAccessRelatedInfo__cellBarred_notBarred;

    //  assign_enum(&sib1->cellAccessRelatedInfo.intraFreqReselection,SystemInformationBlockType1__cellAccessRelatedInfo__intraFreqReselection_allowed);
    sib1->cellAccessRelatedInfo.intraFreqReselection=SystemInformationBlockType1__cellAccessRelatedInfo__intraFreqReselection_allowed;
    sib1->cellAccessRelatedInfo.csg_Indication=0;

    sib1->cellSelectionInfo.q_RxLevMin=-70;
    sib1->cellSelectionInfo.q_RxLevMinOffset=NULL;

    sib1->freqBandIndicator = 2;

    //  assign_enum(&schedulingInfo.si_Periodicity,SchedulingInfo__si_Periodicity_rf8);
    schedulingInfo->si_Periodicity=SchedulingInfo__si_Periodicity_rf8;

    //  assign_enum(&sib_type,SIB_Type_sibType3);
    *sib_type=SIB_Type_sibType3;

    ASN_SEQUENCE_ADD(&schedulingInfo->sib_MappingInfo.list,sib_type);
    ASN_SEQUENCE_ADD(&sib1->schedulingInfoList.list,schedulingInfo);

    sib1->tdd_Config = static_cast<TDD_Config*>(CALLOC(1,sizeof(struct TDD_Config)));

    //assign_enum(&sib1->tdd_Config->subframeAssignment,TDD_Config__subframeAssignment_sa3);
    sib1->tdd_Config->subframeAssignment=TDD_Config__subframeAssignment_sa3;

    //  assign_enum(&sib1->tdd_Config->specialSubframePatterns,TDD_Config__specialSubframePatterns_ssp0);
    sib1->tdd_Config->specialSubframePatterns=TDD_Config__specialSubframePatterns_ssp0;

    //  assign_enum(&sib1->si_WindowLength,SystemInformationBlockType1__si_WindowLength_ms10);
    sib1->si_WindowLength=SystemInformationBlockType1__si_WindowLength_ms10;
    sib1->systemInfoValueTag=0;

    //-----------------------------------------------------------------------------
    // SIB2 CONSTRUCTION
    //-----------------------------------------------------------------------------
    struct SystemInformation_r8_IEs__sib_TypeAndInfo__Member *sib2_part;

    sib2_part = static_cast<SystemInformation_r8_IEs__sib_TypeAndInfo__Member*>(CALLOC(1,sizeof(struct SystemInformation_r8_IEs__sib_TypeAndInfo__Member)));
    memset(sib2_part,0,sizeof(struct SystemInformation_r8_IEs__sib_TypeAndInfo__Member));

    sib2_part->present = SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib2;

    sib2_part->choice.sib2.ac_BarringInfo = NULL;

    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.numberOfRA_Preambles,RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n64);
    sib2_part->choice.sib2.radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.numberOfRA_Preambles=RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n64;
    sib2_part->choice.sib2.radioResourceConfigCommon.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig = NULL;
    //calloc(1,sizeof(*sib2_part->choice.sib2.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig));
    //  assign_enum(&sib2_part->choice.sib2.rach_ConfigCommon.preambleInfo.preamblesGroupAConfig->sizeOfRA_PreamblesGroupA,
    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.powerRampingStep, RACH_ConfigCommon__powerRampingParameters__powerRampingStep_dB2);
    sib2_part->choice.sib2.radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.powerRampingStep=RACH_ConfigCommon__powerRampingParameters__powerRampingStep_dB2;

    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.preambleInitialReceivedTargetPower,RACH_ConfigCommon__powerRampingParameters__preambleInitialReceivedTargetPower_dBm_116 );
    sib2_part->choice.sib2.radioResourceConfigCommon.rach_ConfigCommon.powerRampingParameters.preambleInitialReceivedTargetPower=RACH_ConfigCommon__powerRampingParameters__preambleInitialReceivedTargetPower_dBm_116;

    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.preambleTransMax,RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n10);
    sib2_part->choice.sib2.radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.preambleTransMax=RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n10;

    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.ra_ResponseWindowSize,RACH_ConfigCommon__ra_SupervisionInfo__ra_ResponseWindowSize_sf4);
    sib2_part->choice.sib2.radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.ra_ResponseWindowSize=RACH_ConfigCommon__ra_SupervisionInfo__ra_ResponseWindowSize_sf4;

    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.mac_ContentionResolutionTimer,RACH_ConfigCommon__ra_SupervisionInfo__mac_ContentionResolutionTimer_sf48);
    sib2_part->choice.sib2.radioResourceConfigCommon.rach_ConfigCommon.ra_SupervisionInfo.mac_ContentionResolutionTimer=RACH_ConfigCommon__ra_SupervisionInfo__mac_ContentionResolutionTimer_sf48;

    sib2_part->choice.sib2.radioResourceConfigCommon.rach_ConfigCommon.maxHARQ_Msg3Tx = 4;

    // BCCH-Config
    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.bcch_Config.modificationPeriodCoeff,BCCH_Config__modificationPeriodCoeff_n2);
    sib2_part->choice.sib2.radioResourceConfigCommon.bcch_Config.modificationPeriodCoeff=BCCH_Config__modificationPeriodCoeff_n2;

    // PCCH-Config
    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.pcch_Config.defaultPagingCycle,PCCH_Config__defaultPagingCycle_rf128);
    sib2_part->choice.sib2.radioResourceConfigCommon.pcch_Config.defaultPagingCycle=PCCH_Config__defaultPagingCycle_rf128;

    //  assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.pcch_Config.nB,PCCH_Config__nB_oneT);
    sib2_part->choice.sib2.radioResourceConfigCommon.pcch_Config.nB=PCCH_Config__nB_oneT;

    // PRACH-Config
    sib2_part->choice.sib2.radioResourceConfigCommon.prach_Config.rootSequenceIndex=384;
    sib2_part->choice.sib2.radioResourceConfigCommon.prach_Config.prach_ConfigInfo.prach_ConfigIndex = 3;
    sib2_part->choice.sib2.radioResourceConfigCommon.prach_Config.prach_ConfigInfo.highSpeedFlag = 0;
    sib2_part->choice.sib2.radioResourceConfigCommon.prach_Config.prach_ConfigInfo.zeroCorrelationZoneConfig = 12;
    sib2_part->choice.sib2.radioResourceConfigCommon.prach_Config.prach_ConfigInfo.prach_FreqOffset = 2;

    // PDSCH-Config
    sib2_part->choice.sib2.radioResourceConfigCommon.pdsch_ConfigCommon.referenceSignalPower=15;
    sib2_part->choice.sib2.radioResourceConfigCommon.pdsch_ConfigCommon.p_b=0;

    // PUSCH-Config
    sib2_part->choice.sib2.radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.n_SB=1;
    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode,PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_interSubFrame);
    sib2_part->choice.sib2.radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode=PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_interSubFrame;
    sib2_part->choice.sib2.radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset=0;
    sib2_part->choice.sib2.radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM=0;
    sib2_part->choice.sib2.radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled=1;
    sib2_part->choice.sib2.radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH=0;
    sib2_part->choice.sib2.radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled=0;
    sib2_part->choice.sib2.radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift=0;

    // PUCCH-Config

    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.pucch_ConfigCommon.deltaPUCCH_Shift,PUCCH_ConfigCommon__deltaPUCCH_Shift_ds1);
    sib2_part->choice.sib2.radioResourceConfigCommon.pucch_ConfigCommon.deltaPUCCH_Shift=PUCCH_ConfigCommon__deltaPUCCH_Shift_ds1;
    sib2_part->choice.sib2.radioResourceConfigCommon.pucch_ConfigCommon.nRB_CQI = 1;
    sib2_part->choice.sib2.radioResourceConfigCommon.pucch_ConfigCommon.nCS_AN = 0;
    sib2_part->choice.sib2.radioResourceConfigCommon.pucch_ConfigCommon.n1PUCCH_AN = 32;


    sib2_part->choice.sib2.radioResourceConfigCommon.soundingRS_UL_ConfigCommon.present=SoundingRS_UL_ConfigCommon_PR_release;
    sib2_part->choice.sib2.radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.release=0;

    // uplinkPowerControlCommon

    sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.p0_NominalPUSCH = -96;
    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.alpha,UplinkPowerControlCommon__alpha_al1);
    sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.alpha=UplinkPowerControlCommon__alpha_al1;
    sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.p0_NominalPUCCH = -117;
    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1, DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF0);
    sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1=DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF0;
    //  assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1b, DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF3);
    sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1b=DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF3;

    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2, DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF0);
    sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2=DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF0;

    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2a, DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF0);
    sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2a=DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF0;

    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2b, DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF0);
    sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2b=DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF0;

    sib2_part->choice.sib2.radioResourceConfigCommon.uplinkPowerControlCommon.deltaPreambleMsg3 = 6;

    //assign_enum(&sib2_part->choice.sib2.radioResourceConfigCommon.ul_CyclicPrefixLength, UL_CyclicPrefixLength_len1);
    sib2_part->choice.sib2.radioResourceConfigCommon.ul_CyclicPrefixLength=UL_CyclicPrefixLength_len1;

    //assign_enum(&sib2_part->choice.sib2.ue_TimersAndConstants.t300, UE_TimersAndConstants__t300_ms1000);
    sib2_part->choice.sib2.ue_TimersAndConstants.t300=UE_TimersAndConstants__t300_ms1000;

    //assign_enum(&sib2_part->choice.sib2.ue_TimersAndConstants.t301, UE_TimersAndConstants__t301_ms1000);
    sib2_part->choice.sib2.ue_TimersAndConstants.t301=UE_TimersAndConstants__t301_ms1000;

    //assign_enum(&sib2_part->choice.sib2.ue_TimersAndConstants.t310, UE_TimersAndConstants__t310_ms1000);
    sib2_part->choice.sib2.ue_TimersAndConstants.t310=UE_TimersAndConstants__t310_ms1000;

    //assign_enum(&sib2_part->choice.sib2.ue_TimersAndConstants.n310, UE_TimersAndConstants__n310_n20);
    sib2_part->choice.sib2.ue_TimersAndConstants.n310=UE_TimersAndConstants__n310_n20;

    //assign_enum(&sib2_part->choice.sib2.ue_TimersAndConstants.t311, UE_TimersAndConstants__t311_ms1000);
    sib2_part->choice.sib2.ue_TimersAndConstants.t311=UE_TimersAndConstants__t311_ms1000;

    //assign_enum(&sib2_part->choice.sib2.ue_TimersAndConstants.n311, UE_TimersAndConstants__n311_n1);
    sib2_part->choice.sib2.ue_TimersAndConstants.n311=UE_TimersAndConstants__n311_n1;

    sib2_part->choice.sib2.freqInfo.additionalSpectrumEmission = 1;
    sib2_part->choice.sib2.freqInfo.ul_CarrierFreq = NULL;
    sib2_part->choice.sib2.freqInfo.ul_Bandwidth = NULL;
    sib2_part->choice.sib2.mbsfn_SubframeConfigList = NULL;
    //assign_enum(&sib2_part->choice.sib2.timeAlignmentTimerCommon,TimeAlignmentTimer_sf5120);
    sib2_part->choice.sib2.timeAlignmentTimerCommon=TimeAlignmentTimer_sf5120;
    //-----------------------------------------------------------------------------
    // SIB3 CONSTRUCTION
    //-----------------------------------------------------------------------------
    struct SystemInformation_r8_IEs__sib_TypeAndInfo__Member *sib3_part;

    sib3_part = static_cast<SystemInformation_r8_IEs__sib_TypeAndInfo__Member*>(CALLOC(1,sizeof(struct SystemInformation_r8_IEs__sib_TypeAndInfo__Member)));
    memset(sib3_part,0,sizeof(struct SystemInformation_r8_IEs__sib_TypeAndInfo__Member));
    sib3_part->present = SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib3;

    //  assign_enum(&sib3_part->choice.sib3cellReselectionInfoCommon.q_Hyst,SystemInformationBlockType3__cellReselectionInfoCommon__q_Hyst_dB4);
    sib3_part->choice.sib3.cellReselectionInfoCommon.q_Hyst=SystemInformationBlockType3__cellReselectionInfoCommon__q_Hyst_dB4;

    sib3_part->choice.sib3.cellReselectionInfoCommon.speedStateReselectionPars=NULL;

    sib3_part->choice.sib3.cellReselectionServingFreqInfo.s_NonIntraSearch=NULL;
    sib3_part->choice.sib3.cellReselectionServingFreqInfo.threshServingLow=31;
    sib3_part->choice.sib3.cellReselectionServingFreqInfo.cellReselectionPriority=7;

    sib3_part->choice.sib3.intraFreqCellReselectionInfo.q_RxLevMin = -70;
    sib3_part->choice.sib3.intraFreqCellReselectionInfo.p_Max = NULL;
    sib3_part->choice.sib3.intraFreqCellReselectionInfo.s_IntraSearch = static_cast<ReselectionThreshold_t*>(CALLOC(1,sizeof(sib3_part->choice.sib3.intraFreqCellReselectionInfo.s_IntraSearch)));
    *sib3_part->choice.sib3.intraFreqCellReselectionInfo.s_IntraSearch = 31;
    sib3_part->choice.sib3.intraFreqCellReselectionInfo.allowedMeasBandwidth=static_cast<AllowedMeasBandwidth_t*>(CALLOC(1,sizeof(*sib3_part->choice.sib3.intraFreqCellReselectionInfo.allowedMeasBandwidth)));

    //  assign_enum(sib3_part->choice.sib3.intraFreqCellReselectionInfo.allowedMeasBandwidth,AllowedMeasBandwidth_mbw6);
    *sib3_part->choice.sib3.intraFreqCellReselectionInfo.allowedMeasBandwidth=AllowedMeasBandwidth_mbw6;

    sib3_part->choice.sib3.intraFreqCellReselectionInfo.presenceAntennaPort1 = 0;
    sib3_part->choice.sib3.intraFreqCellReselectionInfo.neighCellConfig.buf = static_cast<uint8_t*>(CALLOC(8,1));
    sib3_part->choice.sib3.intraFreqCellReselectionInfo.neighCellConfig.size = 1;
    sib3_part->choice.sib3.intraFreqCellReselectionInfo.neighCellConfig.buf[0] = 1;
    sib3_part->choice.sib3.intraFreqCellReselectionInfo.neighCellConfig.bits_unused = 6;
    sib3_part->choice.sib3.intraFreqCellReselectionInfo.t_ReselectionEUTRA = 1;
    sib3_part->choice.sib3.intraFreqCellReselectionInfo.t_ReselectionEUTRA_SF = (struct SpeedStateScaleFactors *)NULL;

    //-----------------------------------------------------------------------------
    // SIB AGGREGATION
    //-----------------------------------------------------------------------------
    memset((void*)system_information,0,sizeof(SystemInformation_t));

    message->message.choice.c1.choice.rrcSystemConfigurationResponse.systemInformation = system_information;

    system_information->criticalExtensions.present = SystemInformation__criticalExtensions_PR_systemInformation_r8;
    system_information->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count=0;
    //  asn_set_empty(&systemInformation->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list);//.size=0;
    //  systemInformation->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count=0;
    ASN_SEQUENCE_ADD(&system_information->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list,sib2_part);
    ASN_SEQUENCE_ADD(&system_information->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list,sib3_part);

    //xer_fprint(stdout, &asn_DEF_RRM_RRC_Message, (void*)message);

    return message;
}
