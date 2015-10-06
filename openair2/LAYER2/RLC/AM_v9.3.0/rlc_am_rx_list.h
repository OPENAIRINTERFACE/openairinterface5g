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
/*! \file rlc_am_rx_list.h
* \brief This file defines the prototypes of the functions dealing with a RX list data structure supporting re-segmentation.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_am_internal_rx_list_impl_ RLC AM RX List Reference Implementation
* @ingroup _rlc_am_internal_receiver_impl_
* @{
*/

#ifndef __RLC_AM_RX_LIST_H__
#    define __RLC_AM_RX_LIST_H__

#    include "UTIL/MEM/mem_block.h"
//-----------------------------------------------------------------------------
#        ifdef RLC_AM_RX_LIST_C
#            define private_rlc_am_rx_list(x)    x
#            define protected_rlc_am_rx_list(x)  x
#            define public_rlc_am_rx_list(x)     x
#        else
#            ifdef RLC_AM_MODULE
#                define private_rlc_am_rx_list(x)
#                define protected_rlc_am_rx_list(x)  extern x
#                define public_rlc_am_rx_list(x)     extern x
#            else
#                define private_rlc_am_rx_list(x)
#                define protected_rlc_am_rx_list(x)
#                define public_rlc_am_rx_list(x)     extern x
#            endif
#        endif
//-----------------------------------------------------------------------------
#include "platform_types.h"
#include "platform_constants.h"
#include "PHY/defs.h"

//-----------------------------------------------------------------------------
/*! \fn signed int rlc_am_rx_list_insert_pdu(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlcP , mem_block_t* const tbP)
* \brief      Insert a PDU in the RX buffer (implemented with a list).
* \param[in]  ctxt_pP        Running context.
* \param[in]  rlcP         RLC AM protocol instance pointer.
* \param[in]  tbP          A PDU embedded in a mem_block_t.
* \return     Zero if the PDU could be inserted in the RX buffer, a negative value if the PDU could not be inserted.
*/
protected_rlc_am_rx_list( signed int rlc_am_rx_list_insert_pdu(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlcP,  mem_block_t* const tbP);)

/*! \fn void rlc_am_rx_check_all_byte_segments(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlcP, mem_block_t* const tbP)
* \brief      Check if all sub-segments of a PDU are received, if yes then call rlc_am_rx_mark_all_segments_received() procedure.
* \param[in]  ctxt_pP        Running context.
* \param[in]  rlcP         RLC AM protocol instance pointer.
* \param[in]  tbP          A PDU embedded in a mem_block_t.
*/
protected_rlc_am_rx_list( void rlc_am_rx_check_all_byte_segments(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlcP, mem_block_t* const tbP);)

/*! \fn void rlc_am_rx_mark_all_segments_received (const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlcP, mem_block_t* const  first_segment_tbP)
* \brief      Mark all PDUs having the same sequence number as first_segment_tbP with the information that all segments have been received.
* \param[in]  ctxt_pP        Running context.
* \param[in]  rlcP         RLC AM protocol instance pointer.
* \param[in]  first_segment_tbP   A PDU embedded in a mem_block_t, it is the first PDU in the RX buffer (list) that have its sequence number.
*/
protected_rlc_am_rx_list( void rlc_am_rx_mark_all_segments_received(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlcP, mem_block_t* const first_segment_tbP);)

/*! \fn void rlc_am_rx_list_reassemble_rlc_sdus(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlcP)
* \brief      Reassembly all SDUS that it is possible to reassembly by parsing the RX buffer and looking for PDUs having the flag 'all segments received'.
* \param[in]  ctxt_pP        Running context.
* \param[in]  rlcP         RLC AM protocol instance pointer.
*/
protected_rlc_am_rx_list( void rlc_am_rx_list_reassemble_rlc_sdus(const protocol_ctxt_t* const ctxt_pP, rlc_am_entity_t* const rlcP);)

/*! \fn mem_block_t* list2_insert_before_element (mem_block_t * element_to_insertP, mem_block_t * elementP, list2_t * listP)
* \brief      Insert a PDU embedded in a mem_block_t in a list at a position before a designated element of the list.
* \param[in]  element_to_insertP    Element to insert in the list listP, before elementP position.
* \param[in]  elementP              Element in the list.
* \param[in]  listP                 List where elements are linked.
*/
public_rlc_am_rx_list( mem_block_t* list2_insert_before_element (mem_block_t * element_to_insertP, mem_block_t * elementP, list2_t * listP);)

/*! \fn mem_block_t* list2_insert_after_element (mem_block_t * element_to_insertP, mem_block_t * elementP, list2_t * listP)
* \brief      Insert a PDU embedded in a mem_block_t in a list at a position after a designated element of the list.
* \param[in]  element_to_insertP    Element to insert in the list listP, after elementP position.
* \param[in]  elementP              Element in the list.
* \param[in]  listP                 List where elements are linked.
*/
public_rlc_am_rx_list( mem_block_t* list2_insert_after_element (mem_block_t * element_to_insertP, mem_block_t * elementP, list2_t * listP);)

/*! \fn void rlc_am_rx_list_display (const rlc_am_entity_t* const rlcP, char* messageP)
* \brief      Display the dump of the RX buffer.
* \param[in]  rlcP         RLC AM protocol instance pointer.
* \param[in]  messageP     Message to be displayed along with the display of the dump of the RX buffer.
*/
protected_rlc_am_rx_list( void rlc_am_rx_list_display (const rlc_am_entity_t* const rlcP, char* messageP);)
/** @} */
#endif
