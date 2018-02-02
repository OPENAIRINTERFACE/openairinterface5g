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

/*! \file rlc_am_test.h
* \brief This file defines the prototypes of RLC AM test functions.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
* \note
* \bug
* \warning
*/
#    ifndef __RLC_AM_TEST_H__
#        define __RLC_AM_TEST_H__
#        ifdef RLC_AM_TEST_C
#            define private_rlc_am_test(x)    x
#            define protected_rlc_am_test(x)  x
#            define public_rlc_am_test(x)     x
#        else
#            ifdef RLC_AM_MODULE
#                define private_rlc_am_test(x)
#                define protected_rlc_am_test(x)  extern x
#                define public_rlc_am_test(x)     extern x
#            else
#                define private_rlc_am_test(x)
#                define protected_rlc_am_test(x)
#                define public_rlc_am_test(x)     extern x
#            endif
#        endif
public_rlc_am_test(rlc_am_entity_t       g_am_tx;)
public_rlc_am_test(rlc_am_entity_t       g_am_rx;)

protected_rlc_am_test(void rlc_am_v9_3_0_test_windows();)
protected_rlc_am_test(void rlc_am_v9_3_0_test_read_write_bit_field();)
protected_rlc_am_test(void rlc_am_v9_3_0_test_data_conf(module_id_t module_idP, rb_id_t rb_idP, mui_t muiP, rlc_tx_status_t statusP);)
protected_rlc_am_test(void rlc_am_v9_3_0_test_send_sdu(rlc_am_entity_t *am_txP, int sdu_indexP);)
protected_rlc_am_test(void rlc_am_v9_3_0_test_exchange_pdus(rlc_am_entity_t *am_txP,rlc_am_entity_t *am_RxP,
                      uint16_t bytes_txP,uint16_t bytes_rxP);)
protected_rlc_am_test(void rlc_am_v9_3_0_test_mac_rlc_loop (struct mac_data_ind *data_indP,  struct mac_data_req *data_requestP, int* drop_countP, int *tx_packetsP, int* dropped_tx_packetsP);)
protected_rlc_am_test(void rlc_am_v9_3_0_test_data_ind (module_id_t module_idP, rb_id_t rb_idP, sdu_size_t sizeP,
                      mem_block_t *sduP);)
protected_rlc_am_test(void rlc_am_v9_3_0_test_tx_rx();)
public_rlc_am_test(   void rlc_am_v9_3_0_test_print_trace (void);)
public_rlc_am_test(   void rlc_am_v9_3_0_test());
#    endif
