/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

#include <iostream>
#include <cassert>
#include <typeinfo>
//----------------------------------------------------------------------------
#include "TestScenario.h"
#include "Message.h"
#include "RRC2RRMMessageConnectionRequest.h"
#include "RRC2RRMMessageAddUserRequest.h"
#include "RRC2RRMMessageAddUserConfirm.h"
#include "RRC2RRMMessageAddRadioAccessBearerRequest.h"
#include "RRC2RRMMessageAddRadioAccessBearerConfirm.h"
#include "RRC2RRMMessageRemoveRadioAccessBearerRequest.h"
#include "RRC2RRMMessageRemoveUserRequest.h"
#include "RRC2RRMMessageUserReconfigurationComplete.h"
#include "RRM2RRCMessageConnectionResponse.h"
#include "RRM2RRCMessageAddUserResponse.h"
#include "RRM2RRCMessageAddRadioAccessBearerResponse.h"
#include "RRM2RRCMessageRemoveRadioAccessBearerResponse.h"
#include "RRM2RRCMessageRemoveUserResponse.h"
#include "RRM2RRCMessageUserReconfiguration.h"
#include "RRMMessageHandler.h"
#include "Exceptions.h"
//----------------------------------------------------------------------------
int TestScenario::s_step        = 0;
int TestScenario::s_scenario_id = 0;

int test_scenario_all_is_ok_transaction_id = 0;
int test_scenario_all_is_ok_cell_id        = 0;
int test_scenario_all_is_ok_mobile_id      = 1;
int test_scenario_all_is_ok_rb_id          = 6 + test_scenario_all_is_ok_mobile_id*MAX_RB_PER_MOBILE;
int test_scenario_all_is_ok_rb2_id         = 7 + test_scenario_all_is_ok_mobile_id*MAX_RB_PER_MOBILE;
//----------------------------------------------------------------------------
void TestScenario::Start(const int scenario_idP)
//----------------------------------------------------------------------------
{
    TestScenario::s_scenario_id = scenario_idP;

    switch (TestScenario::s_scenario_id) {

        case TestScenario::SCENARIO_ALL_IS_OK:

            {
                TestScenario::s_step        = 0;
                RRC2RRMMessageConnectionRequest connection_request("0::1", 33334, ++test_scenario_all_is_ok_transaction_id);
                RRMMessageHandler::Instance()->Send2Peer("0::1", 33333, connection_request.GetSerializedMessageBuffer(), connection_request.GetSerializedMessageSize());
                std::cout <<  "Msg RRC2RRMMessageConnectionRequest sent..." << std::endl;
            }
            break;

        default:
            std::cerr << "TestScenario::Start() UNKNOWN SCENARIO" << std::endl;
    }
}
//----------------------------------------------------------------------------
void TestScenario::notifyRxMessage(Message* messageP)
//----------------------------------------------------------------------------
{
    std::cout <<  "[TestScenario] notifyRxMessage()" << std::endl;
    switch (TestScenario::s_scenario_id){

        case TestScenario::SCENARIO_ALL_IS_OK:

            switch (TestScenario::s_step) {
                case 0:
                {
                    std::string type_id_message(typeid(*messageP).name());
                    std::size_t found=type_id_message.find("RRM2RRCMessageConnectionResponse");
                    if (found != std::string::npos) {
                        TestScenario::s_step += 1;

                        RRM2RRCMessageConnectionResponse* message = static_cast<RRM2RRCMessageConnectionResponse*>(messageP);
                        assert(message->GetStatus() == OpenAir_RRM_Response_Status_success);
                        assert(message->GetReason() == OpenAir_RRM_Response_Reason_ok);
                        assert(message->GetTransactionId() == test_scenario_all_is_ok_transaction_id);
                        assert(message->GetENodeBId() == test_scenario_all_is_ok_cell_id);

                        RRC2RRMMessageAddUserRequest add_user_request("0::1", 33334, test_scenario_all_is_ok_cell_id, test_scenario_all_is_ok_mobile_id, ++test_scenario_all_is_ok_transaction_id);
                        RRMMessageHandler::Instance()->Send2Peer("0::1", 33333, add_user_request.GetSerializedMessageBuffer(), add_user_request.GetSerializedMessageSize());
                        std::cout <<  "Msg RRC2RRMMessageAddUserRequest sent..." << std::endl;

                    } else {
                        std::cout << "TestScenario::Start() UNKNOWN RRM2RRCMessageConnectionResponse received" << std::endl;
                        throw test_scenario_message_response_type_error();
                    }
                }
                    break;
                case 1:
                {
                    std::string type_id_message(typeid(*messageP).name());
                    std::size_t found=type_id_message.find("RRM2RRCMessageAddUserResponse");
                    if (found != std::string::npos) {
                        TestScenario::s_step += 1;

                        RRM2RRCMessageAddUserResponse* message = static_cast<RRM2RRCMessageAddUserResponse*>(messageP);
                        assert(message->GetStatus()        == OpenAir_RRM_Response_Status_success);
                        assert(message->GetReason()        == OpenAir_RRM_Response_Reason_ok);
                        assert(message->GetENodeBId()        == test_scenario_all_is_ok_cell_id);
                        assert(message->GetMobileId()      == test_scenario_all_is_ok_mobile_id);
                        assert(message->GetTransactionId() == test_scenario_all_is_ok_transaction_id);

                        //for (int i = 0; i < 1000 ; i++) {

                        RRC2RRMMessageAddUserConfirm add_user_confirm("0::1", 33334, test_scenario_all_is_ok_cell_id, test_scenario_all_is_ok_mobile_id, test_scenario_all_is_ok_transaction_id);
                        RRMMessageHandler::Instance()->Send2Peer("0::1", 33333, add_user_confirm.GetSerializedMessageBuffer(), add_user_confirm.GetSerializedMessageSize());
                        std::cout <<  "Msg RRC2RRMMessageAddUserConfirm sent..." << std::endl;
                        //std::cout <<  "i = " << i << std::endl;
                        //}

                    } else {
                        std::cout << "TestScenario::Start() UNKNOWN RRM2RRCMessageAddUserResponse received" << std::endl;
                        throw test_scenario_message_response_type_error();
                    }
                }
                break;
                case 2:
                {
                    std::string type_id_message(typeid(*messageP).name());
                    std::size_t found=type_id_message.find("RRM2RRCMessageUserReconfiguration");
                    if (found != std::string::npos) {
                        TestScenario::s_step += 1;

                        RRM2RRCMessageUserReconfiguration* message = static_cast<RRM2RRCMessageUserReconfiguration*>(messageP);
                        assert(message->GetStatus()        == OpenAir_RRM_Response_Status_success);
                        assert(message->GetReason()        == OpenAir_RRM_Response_Reason_ok);
                        assert(message->GetENodeBId()        == test_scenario_all_is_ok_cell_id);
                        assert(message->GetMobileId()      == test_scenario_all_is_ok_mobile_id);
                        assert(message->GetTransactionId() == test_scenario_all_is_ok_transaction_id);
                        std::cout <<  "Msg RRM2RRCMessageUserReconfiguration OK" << std::endl;

                        RRC2RRMMessageUserReconfigurationComplete reconfiguration_complete("0::1", 33334,
                                                                                 test_scenario_all_is_ok_cell_id,
                                                                                 test_scenario_all_is_ok_mobile_id,
                                                                                 test_scenario_all_is_ok_transaction_id);
                        RRMMessageHandler::Instance()->Send2Peer("0::1", 33333, reconfiguration_complete.GetSerializedMessageBuffer(), reconfiguration_complete.GetSerializedMessageSize());
                        std::cout <<  "Msg RRC2RRMMessageUserReconfigurationComplete sent..." << std::endl;

                        cout << "End of scenario" << endl;
                        exit(0);
                    } else {
                        std::cout << "TestScenario::Start() UNKNOWN RRM2RRCMessageUserReconfiguration received" << std::endl;
                        throw test_scenario_message_response_type_error();
                    }
                }
                break;
                case 4:
                {
                    std::string type_id_message(typeid(*messageP).name());
                    std::size_t found=type_id_message.find("RRM2RRCMessageAddRadioAccessBearerResponse");
                    if (found != std::string::npos) {
                        TestScenario::s_step += 1;

                        RRM2RRCMessageAddRadioAccessBearerResponse* message = static_cast<RRM2RRCMessageAddRadioAccessBearerResponse*>(messageP);
                        assert(message->GetStatus()        == Message::STATUS_ADD_RADIO_ACCESS_BEARER_SUCCESSFULL);
                        assert(message->GetReason()        == OpenAir_RRM_Response_Reason_ok);
                        assert(message->GetENodeBId()        == test_scenario_all_is_ok_cell_id);
                        assert(message->GetMobileId()      == test_scenario_all_is_ok_mobile_id);
                        assert(message->GetRadioBearerId() == test_scenario_all_is_ok_rb_id);
                        assert(message->GetTransactionId() == test_scenario_all_is_ok_transaction_id);

                        RRC2RRMMessageAddRadioAccessBearerConfirm confirm("0::1", 33334, test_scenario_all_is_ok_cell_id, test_scenario_all_is_ok_mobile_id, test_scenario_all_is_ok_rb_id, test_scenario_all_is_ok_transaction_id);
                        RRMMessageHandler::Instance()->Send2Peer("0::1", 33333, confirm.GetSerializedMessageBuffer(), confirm.GetSerializedMessageSize());
                        std::cout <<  "Msg RRC2RRMMessageAddRadioAccessBearerConfirm sent..." << std::endl;

                        test_scenario_all_is_ok_transaction_id += 1;
                        RRC2RRMMessageRemoveRadioAccessBearerRequest request("0::1", 33334,
                                                                                 test_scenario_all_is_ok_cell_id,
                                                                                 test_scenario_all_is_ok_mobile_id,
                                                                                 test_scenario_all_is_ok_rb_id,
                                                                                 test_scenario_all_is_ok_transaction_id);
                        RRMMessageHandler::Instance()->Send2Peer("0::1", 33333, request.GetSerializedMessageBuffer(), request.GetSerializedMessageSize());
                        std::cout <<  "Msg RRC2RRMMessageRemoveRadioAccessBearerRequest sent..." << std::endl;
                    } else {
                        std::cout << "TestScenario::Start() UNKNOWN RRM2RRCMessageAddRadioAccessBearerResponse received" << std::endl;
                        throw test_scenario_message_response_type_error();
                    }
                }
                break;
                case 5:
                {
                    std::string type_id_message(typeid(*messageP).name());
                    std::size_t found=type_id_message.find("RRM2RRCMessageRemoveRadioAccessBearerResponse");
                    if (found != std::string::npos) {
                        TestScenario::s_step += 1;

                        RRM2RRCMessageRemoveRadioAccessBearerResponse* message = static_cast<RRM2RRCMessageRemoveRadioAccessBearerResponse*>(messageP);
                        assert(message->GetStatus()        == Message::STATUS_REMOVE_RADIO_ACCESS_BEARER_SUCCESSFULL);
                        assert(message->GetReason()        == OpenAir_RRM_Response_Reason_ok);
                        assert(message->GetENodeBId()        == test_scenario_all_is_ok_cell_id);
                        assert(message->GetMobileId()      == test_scenario_all_is_ok_mobile_id);
                        assert(message->GetRadioBearerId() == test_scenario_all_is_ok_rb_id);
                        assert(message->GetTransactionId() == test_scenario_all_is_ok_transaction_id);

                        test_scenario_all_is_ok_transaction_id += 1;
                        RRC2RRMMessageAddRadioAccessBearerRequest add_rb_request("0::1", 33334,
                                                                                 test_scenario_all_is_ok_cell_id,
                                                                                 test_scenario_all_is_ok_mobile_id,
                                                                                 test_scenario_all_is_ok_rb2_id,
                                                                                 3,   //traffic_class
                                                                                 100, //delay
                                                                                 128, //guaranted_bit_rate_uplink
                                                                                 256, //max_bit_rate_uplink
                                                                                 512, //guaranted_bit_rate_downlink
                                                                                 768, //max_bit_rate_downlink
                                                                                 test_scenario_all_is_ok_transaction_id);
                        RRMMessageHandler::Instance()->Send2Peer("0::1", 33333, add_rb_request.GetSerializedMessageBuffer(), add_rb_request.GetSerializedMessageSize());
                        std::cout <<  "Msg RRC2RRMMessageAddRadioAccessBearerRequest sent..." << std::endl;
                    } else {
                        std::cout << "TestScenario::Start() UNKNOWN RRM2RRCMessageRemoveRadioAccessBearerResponse received" << std::endl;
                        throw test_scenario_message_response_type_error();
                    }
                }
                break;
                case 6:
                {
                    std::string type_id_message(typeid(*messageP).name());
                    std::size_t found=type_id_message.find("RRM2RRCMessageAddRadioAccessBearerResponse");
                    if (found != std::string::npos) {
                        TestScenario::s_step += 1;

                        RRM2RRCMessageAddRadioAccessBearerResponse* message = static_cast<RRM2RRCMessageAddRadioAccessBearerResponse*>(messageP);
                        assert(message->GetStatus()        == Message::STATUS_ADD_RADIO_ACCESS_BEARER_SUCCESSFULL);
                        assert(message->GetReason()        == OpenAir_RRM_Response_Reason_ok);
                        assert(message->GetENodeBId()        == test_scenario_all_is_ok_cell_id);
                        assert(message->GetMobileId()      == test_scenario_all_is_ok_mobile_id);
                        assert(message->GetRadioBearerId() == test_scenario_all_is_ok_rb2_id);
                        assert(message->GetTransactionId() == test_scenario_all_is_ok_transaction_id);

                        RRC2RRMMessageAddRadioAccessBearerConfirm confirm("0::1", 33334, test_scenario_all_is_ok_cell_id, test_scenario_all_is_ok_mobile_id, test_scenario_all_is_ok_rb2_id, test_scenario_all_is_ok_transaction_id);
                        RRMMessageHandler::Instance()->Send2Peer("0::1", 33333, confirm.GetSerializedMessageBuffer(), confirm.GetSerializedMessageSize());
                        std::cout <<  "Msg RRC2RRMMessageAddRadioAccessBearerConfirm sent..." << std::endl;

                        test_scenario_all_is_ok_transaction_id += 1;
                        RRC2RRMMessageRemoveUserRequest request("0::1", 33334,
                                                                                 test_scenario_all_is_ok_cell_id,
                                                                                 test_scenario_all_is_ok_mobile_id,
                                                                                 test_scenario_all_is_ok_transaction_id);
                        RRMMessageHandler::Instance()->Send2Peer("0::1", 33333, request.GetSerializedMessageBuffer(), request.GetSerializedMessageSize());
                        std::cout <<  "Msg RRC2RRMMessageRemoveUserRequest sent..." << std::endl;
                    } else {
                        std::cout << "TestScenario::Start() UNKNOWN RRM2RRCMessageAddRadioAccessBearerResponse received" << std::endl;
                        throw test_scenario_message_response_type_error();
                    }
                }
                break;
                case 7:
                {
                    std::string type_id_message(typeid(*messageP).name());
                    std::size_t found=type_id_message.find("RRM2RRCMessageRemoveUserResponse");
                    if (found != std::string::npos) {
                        TestScenario::s_step += 1;

                        RRM2RRCMessageRemoveUserResponse* message = static_cast<RRM2RRCMessageRemoveUserResponse*>(messageP);
                        assert(message->GetStatus()        == Message::STATUS_REMOVE_USER_SUCCESSFULL);
                        assert(message->GetReason()        == OpenAir_RRM_Response_Reason_ok);
                        assert(message->GetENodeBId()        == test_scenario_all_is_ok_cell_id);
                        assert(message->GetMobileId()      == test_scenario_all_is_ok_mobile_id);
                        assert(message->GetTransactionId() == test_scenario_all_is_ok_transaction_id);

                        std::cout <<  "END OF SCENARIO" << std::endl;
                    } else {
                        std::cout << "TestScenario::Start() UNKNOWN RRM2RRCMessageRemoveUserResponse received" << std::endl;
                        throw test_scenario_message_response_type_error();
                    }
                }
                break;



                default:
                    std::cerr << "TestScenario::notifyRxMessage() SCENARIO_ALL_IS_OK UNKNOWN STEP" << std::endl;
            }
            break;

        default:
            std::cerr << "TestScenario::notifyRxMessage() UNKNOWN SCENARIO" << std::endl;

    }
}