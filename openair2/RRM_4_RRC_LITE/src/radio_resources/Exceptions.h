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

