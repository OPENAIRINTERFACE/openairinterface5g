#include <stdio.h>
#include "RadioResources.h"
#include "Exceptions.h"
#include "Transactions.h"


RadioResources* RadioResources::s_instance = 0;
// temporary to be improved later (is RRC send his cell id or RRM assign one?)
cell_id_t       RadioResources::s_cell_id_generator = 0;


//-----------------------------------------------------------------
RRM2RRCMessageConnectionResponse* RadioResources::Request(RRC2RRMMessageConnectionRequest &requestP)
//-----------------------------------------------------------------
{
    cell_id_t            e_node_b_id = RadioResources::s_cell_id_generator++;
    if (IsENodeBRegistered(e_node_b_id)) {
        throw enodeb_already_connected_error() << enodeb_id_info(e_node_b_id);
    } else {
        fprintf(stderr, "[RadioResources] Adding new ENodeB %d\n", e_node_b_id);
        m_e_node_b_map.insert(std::pair<cell_id_t,ENodeB*>(e_node_b_id,  new ENodeB(e_node_b_id)));
        ENodeB* const e_Node_b = this->GetENodeB(e_node_b_id);
        RRM_RRC_Message_t*    asn1_message = e_Node_b->GetRRMRRCConfigurationMessage(requestP.GetTransactionId());
        RRM2RRCMessageConnectionResponse *response = new RRM2RRCMessageConnectionResponse(requestP.GetTransactionId(),OpenAir_RRM_Response_Status_success, OpenAir_RRM_Response_Reason_ok, asn1_message);
        return response;
    }
}
//-----------------------------------------------------------------
RRM2RRCMessageAddUserResponse*  RadioResources::Request(RRC2RRMMessageAddUserRequest &requestP)
//-----------------------------------------------------------------
{
    cell_id_t e_node_b_id = requestP.GetENodeBId();
    if (IsENodeBRegistered(e_node_b_id)) {
        mobile_id_t mobile_id  = requestP.GetMobileId();
        if (IsMobileRegistered(e_node_b_id, mobile_id)) {
            throw mobile_already_connected_error() << mobile_id_info(mobile_id);
        } else {
            fprintf(stderr, "[RadioResources] Adding new Mobile %d\n", mobile_id);
            //Transaction* transaction =  new Transaction(requestP.GetTransactionId());
            // keep track of the transaction among message exchanges between RRC and RRM
            //Transactions::Instance()->AddTransaction(transaction);

            ENodeB* const e_Node_b = this->GetENodeB(e_node_b_id);

            Mobile* mobile = new Mobile(e_node_b_id, mobile_id,requestP.GetTransactionId());

            RRM_RRC_Message_t* asn1_message = e_Node_b->AddUserRequest(mobile, requestP.GetTransactionId());

            m_mobile_local_map.insert(std::pair<mobile_node_local_key_t ,Mobile*>(std::pair<cell_id_t,mobile_id_t>(e_node_b_id, mobile_id),  mobile));

            RRM2RRCMessageAddUserResponse *response = new RRM2RRCMessageAddUserResponse(requestP.GetTransactionId(),OpenAir_RRM_Response_Status_success, OpenAir_RRM_Response_Reason_ok, asn1_message);
            return response;
        }
    } else {
        throw enodeb_not_connected_error() << enodeb_id_info(e_node_b_id);
    }
}
//-----------------------------------------------------------------
RRM2RRCMessageUserReconfiguration*  RadioResources::Request(RRC2RRMMessageAddUserConfirm &confirmP)
//-----------------------------------------------------------------
{
    cell_id_t e_node_b_id = confirmP.GetENodeBId();
    if (IsENodeBRegistered(e_node_b_id)) {
        mobile_id_t mobile_id  = confirmP.GetMobileId();
        if (!IsMobileRegistered(e_node_b_id, mobile_id)) {
            throw mobile_not_connected_error() << mobile_id_info(mobile_id);
        } else {
            fprintf(stderr, "[RadioResources] Confirming    new Mobile %d\n", mobile_id);
            fprintf(stderr, "[RadioResources] Reconfiguring new Mobile %d\n", mobile_id);
            ENodeB* const e_Node_b = this->GetENodeB(e_node_b_id);

            Mobile* mobile = GetMobile(e_node_b_id, mobile_id);

            RRM_RRC_Message_t* asn1_message = e_Node_b->AddUserConfirm(mobile, confirmP.GetTransactionId());

            //m_mobile_local_map.insert(std::pair<mobile_node_local_key_t ,Mobile*>(std::pair<cell_id_t,mobile_id_t>(e_node_b_id, mobile_id),  mobile));

            RRM2RRCMessageUserReconfiguration *response = new RRM2RRCMessageUserReconfiguration(confirmP.GetTransactionId(),OpenAir_RRM_Response_Status_success, OpenAir_RRM_Response_Reason_ok, asn1_message);
            return response;
        }
    } else {
        throw enodeb_not_connected_error() << enodeb_id_info(e_node_b_id);
    }
}
//-----------------------------------------------------------------
void  RadioResources::Request(RRC2RRMMessageUserReconfigurationComplete &completeP)
//-----------------------------------------------------------------
{
    cell_id_t e_node_b_id = completeP.GetENodeBId();
    if (IsENodeBRegistered(e_node_b_id)) {
        mobile_id_t mobile_id  = completeP.GetMobileId();
        if (!IsMobileRegistered(e_node_b_id, mobile_id)) {
            throw mobile_not_connected_error() << mobile_id_info(mobile_id);
        } else {
            fprintf(stderr, "[RadioResources] Confirming    new Mobile %d\n", mobile_id);
            fprintf(stderr, "[RadioResources] Reconfiguring new Mobile %d\n", mobile_id);
            ENodeB* const e_Node_b = this->GetENodeB(e_node_b_id);

            Mobile* mobile = GetMobile(e_node_b_id, mobile_id);

            e_Node_b->UserReconfigurationComplete(mobile, completeP.GetTransactionId());
        }
    } else {
        throw enodeb_not_connected_error() << enodeb_id_info(e_node_b_id);
    }
}
//-----------------------------------------------------------------
Transaction*  RadioResources::Request(RRC2RRMMessageRemoveUserRequest &requestP)
//-----------------------------------------------------------------
{
    cell_id_t e_node_b_id = requestP.GetENodeBId();
    if (IsENodeBRegistered(e_node_b_id)) {
        mobile_id_t mobile_id  = requestP.GetMobileId();
        if (!IsMobileRegistered(e_node_b_id, mobile_id)) {
            throw mobile_not_connected_error() << mobile_id_info(mobile_id);
        }else {
            fprintf(stderr, "[RadioResources] Removing mobile %d\n", mobile_id);
            Transaction* transaction =  new Transaction(requestP.GetTransactionId());
            // Do not keep track of the transaction
            //Transactions::Instance()->AddTransaction(transaction);

            ENodeB* const e_Node_b = this->GetENodeB(e_node_b_id);

            Mobile* mobile = GetMobile(e_node_b_id, mobile_id);

            e_Node_b->RemoveUserRequest(mobile,requestP.GetTransactionId());

            return transaction;
        }
    } else {
        throw enodeb_not_connected_error() << enodeb_id_info(e_node_b_id);
    }
}
//-----------------------------------------------------------------
Transaction*  RadioResources::Request(RRC2RRMMessageAddRadioAccessBearerRequest &requestP)
//-----------------------------------------------------------------
{
    cell_id_t e_node_b_id = requestP.GetENodeBId();
    if (IsENodeBRegistered(e_node_b_id)) {
        mobile_id_t mobile_id  = requestP.GetMobileId();
        if (!IsMobileRegistered(e_node_b_id, mobile_id)) {
            throw mobile_not_connected_error() << mobile_id_info(mobile_id);
        } else {
            fprintf(stderr, "[RadioResources] Adding new DRB %d\n", requestP.GetRadioBearerId());
            Transaction* transaction =  new Transaction(requestP.GetTransactionId());
            // keep track of the transaction among message exchanges between RRC and RRM
            Transactions::Instance()->AddTransaction(transaction);

            ENodeB* const e_Node_b = this->GetENodeB(e_node_b_id);

            Mobile* mobile = GetMobile(e_node_b_id, mobile_id);

            /*e_Node_b->AddDataRadioBearer(mobile,
                                         requestP.GetRadioBearerId(),
                                         requestP.GetTrafficClass(),
                                         requestP.GetDelay(),
                                         requestP.GetGuarantedBitRateUplink(),
                                         requestP.GetMaxBitRateUplink(),
                                         requestP.GetGuarantedBitRateDownlink(),
                                         requestP.GetMaxBitRateDownlink(),
                                         transaction);*/

            return transaction;
        }
    } else {
        throw enodeb_not_connected_error() << enodeb_id_info(e_node_b_id);
    }
}
//-----------------------------------------------------------------
Transaction*  RadioResources::Request(RRC2RRMMessageAddRadioAccessBearerConfirm &confirmP)
//-----------------------------------------------------------------
{
    cell_id_t e_node_b_id = confirmP.GetENodeBId();
    if (IsENodeBRegistered(e_node_b_id)) {
        mobile_id_t mobile_id  = confirmP.GetMobileId();
        if (!IsMobileRegistered(e_node_b_id, mobile_id)) {
            throw mobile_not_connected_error() << mobile_id_info(mobile_id);
        } else {
            // TO DO CHECK RADIO BEARER
            fprintf(stderr, "[RadioResources] Confirming new DRB %d\n", confirmP.GetRadioBearerId());
            Transaction* transaction =  Transactions::Instance()->GetTransaction(confirmP.GetTransactionId());
            transaction->Commit(); transaction = NULL;
            Transactions::Instance()->RemoveTransaction(confirmP.GetTransactionId());
            return NULL;
        }
    } else {
        throw enodeb_not_connected_error() << enodeb_id_info(e_node_b_id);
    }
}
//-----------------------------------------------------------------
Transaction*  RadioResources::Request(RRC2RRMMessageRemoveRadioAccessBearerRequest &requestP)
//-----------------------------------------------------------------
{
    cell_id_t e_node_b_id = requestP.GetENodeBId();
    if (IsENodeBRegistered(e_node_b_id)) {
        mobile_id_t mobile_id  = requestP.GetMobileId();
        if (!IsMobileRegistered(e_node_b_id, mobile_id)) {
            throw mobile_not_connected_error() << mobile_id_info(mobile_id);
        } else {
            fprintf(stderr, "[RadioResources] Removing DRB %d\n", requestP.GetRadioBearerId());
            Transaction* transaction =  new Transaction(requestP.GetTransactionId());
            // Do not keep track of the transaction
            //Transactions::Instance()->AddTransaction(transaction);

            ENodeB* const e_Node_b = this->GetENodeB(e_node_b_id);

            Mobile* mobile = GetMobile(e_node_b_id, mobile_id);

            /*e_Node_b->RemoveDataRadioBearer(mobile,
                                         requestP.GetRadioBearerId(),
                                         transaction);*/

            return transaction;
        }
    } else {
        throw enodeb_not_connected_error() << enodeb_id_info(e_node_b_id);
    }
}
//----------------------------------------------------------------------------
ENodeB* const  RadioResources::GetENodeB(cell_id_t e_node_b_idP)  {
    //----------------------------------------------------------------------------
    map<cell_id_t ,ENodeB*>::iterator it;
    it=m_e_node_b_map.find(e_node_b_idP);
    if (it != m_e_node_b_map.end()) {
        return it->second;
    } else {
        throw enodeb_not_connected_error() << enodeb_id_info(e_node_b_idP);
    }
}
//----------------------------------------------------------------------------
bool  RadioResources::IsENodeBRegistered(cell_id_t idP)  {
    //----------------------------------------------------------------------------
    map<cell_id_t ,ENodeB*>::iterator it;
    it=m_e_node_b_map.find(idP);
    if (it != m_e_node_b_map.end()) {
        return true;
    } else {
        return false;
    }
}
//----------------------------------------------------------------------------
Mobile* const  RadioResources::GetMobile(cell_id_t e_node_b_idP, mobile_id_t mobile_idP)  {
    //----------------------------------------------------------------------------
    map<mobile_node_local_key_t,Mobile*>::iterator it;
    it=m_mobile_local_map.find(std::pair<cell_id_t,mobile_id_t>(e_node_b_idP, mobile_idP));
    if (it != m_mobile_local_map.end()) {
        return it->second;
    } else {
        throw mobile_not_connected_error();
    }
}
//----------------------------------------------------------------------------
bool  RadioResources::IsMobileRegistered(cell_id_t e_node_b_idP, mobile_id_t mobile_idP)  {
    //----------------------------------------------------------------------------
    map<mobile_node_local_key_t,Mobile*>::iterator it;
    it=m_mobile_local_map.find(std::pair<cell_id_t,mobile_id_t>(e_node_b_idP, mobile_idP));
    if (it != m_mobile_local_map.end()) {
        return true;
    } else {
        return false;
    }
}
//-----------------------------------------------------------------
RadioResources* RadioResources::Instance()
//-----------------------------------------------------------------
{
    if (RadioResources::s_instance == 0) {
        RadioResources::s_instance = new RadioResources;
    }
    return s_instance;
}
//-----------------------------------------------------------------
RadioResources::RadioResources()
//-----------------------------------------------------------------
{

}
//-----------------------------------------------------------------
RadioResources::~RadioResources()
//-----------------------------------------------------------------
{
    if (!m_e_node_b_map.empty())  m_e_node_b_map.clear();
    if (!m_mobile_local_map.empty())  m_mobile_local_map.clear();
}

