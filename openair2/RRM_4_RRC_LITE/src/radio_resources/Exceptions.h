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

#ifndef _EXCEPTIONS_H
#    define _EXCEPTIONS_H

#include <boost/exception/all.hpp>

struct enodeb_not_connected_error:            virtual boost::exception { };
struct enodeb_already_connected_error:        virtual boost::exception { };
struct enodeb_identifier_out_of_bounds:       virtual boost::exception { };
struct radio_bearer_identifier_out_of_bounds: virtual boost::exception { };
struct too_many_enodeb_for_mobile:            virtual boost::exception { };
struct mobile_not_connected_error:            virtual boost::exception { };
struct mobile_already_connected_error:        virtual boost::exception { };
struct null_transaction_error:                virtual boost::exception { };
struct no_such_transaction_error:             virtual boost::exception { };
struct no_such_radio_bearer_error:            virtual boost::exception { };
struct transaction_overwrite_error:           virtual boost::exception { };
struct transaction_already_exist_error:       virtual boost::exception { };
struct command_deserialize_length_error:      virtual boost::exception { };
struct command_data_overwrite_error:          virtual boost::exception { };
struct asn1_encoding_error:                   virtual boost::exception { };

struct test_scenario_message_response_type_error: virtual boost::exception { };

typedef boost::error_info<struct tag_enodeb_id,unsigned int> enodeb_id_info;
typedef boost::error_info<struct tag_mobile_id,unsigned int> mobile_id_info;



#endif

