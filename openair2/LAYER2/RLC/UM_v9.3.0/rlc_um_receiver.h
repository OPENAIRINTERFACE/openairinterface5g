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

/*! \file rlc_um_receiver.h
* \brief This file defines the prototypes of the functions dealing with the first stage of the receiving process.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @addtogroup _rlc_um_receiver_impl_ RLC UM Receiver Implementation
* @{
*/
#    ifndef __RLC_UM_RECEIVER_PROTO_EXTERN_H__
#        define __RLC_UM_RECEIVER_PROTO_EXTERN_H__
#        include "rlc_um_entity.h"
#        include "mac_primitives.h"

/*! \fn void rlc_um_display_rx_window(const protocol_ctxt_t* const ctxtP,rlc_um_entity_t * const rlc_pP)
* \brief    Display the content of the RX buffer, the output stream is targeted to TTY terminals because of escape sequences.
* \param[in]  ctxtP       Running context.
* \param[in]  rlc_pP      RLC UM protocol instance pointer.
*/
void rlc_um_display_rx_window(const protocol_ctxt_t* const ctxtP, rlc_um_entity_t * const rlc_pP);

/*! \fn void rlc_um_receive (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t * const rlc_pP, struct mac_data_ind data_indP)
* \brief    Handle the MAC data indication, retreive the transport blocks and send them one by one to the DAR process.
* \param[in]  ctxtP       Running context.
* \param[in]  rlc_pP      RLC UM protocol instance pointer.
* \param[in]  data_indP   Data indication structure containing transport block received from MAC layer.
*/
void rlc_um_receive (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t * const rlc_pP, struct mac_data_ind data_indP);
/** @} */
#    endif
