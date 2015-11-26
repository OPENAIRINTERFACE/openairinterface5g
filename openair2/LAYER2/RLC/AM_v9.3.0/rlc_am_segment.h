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
/*! \file rlc_am_segment.h
* \brief This file defines the prototypes of the functions dealing with the segmentation of PDCP SDUs.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_am_internal_segment_impl_ RLC AM Segmentation Internal Reference Implementation
* @ingroup _rlc_am_internal_impl_
* @{
*/
#    ifndef __RLC_AM_SEGMENT_H__
#        define __RLC_AM_SEGMENT_H__
//-----------------------------------------------------------------------------
#        ifdef RLC_AM_SEGMENT_C
#            define private_rlc_am_segment(x)    x
#            define protected_rlc_am_segment(x)  x
#            define public_rlc_am_segment(x)     x
#        else
#            ifdef RLC_AM_MODULE
#                define private_rlc_am_segment(x)
#                define protected_rlc_am_segment(x)  extern x
#                define public_rlc_am_segment(x)     extern x
#            else
#                define private_rlc_am_segment(x)
#                define protected_rlc_am_segment(x)
#                define public_rlc_am_segment(x)     extern x
#            endif
#        endif

/*! \fn void rlc_am_pdu_polling (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *const rlcP, rlc_am_pdu_sn_10_t *pduP, int16_t payload_sizeP)
* \brief      Set or not the poll bit in the PDU header depending on RLC AM protocol variables.
* \param[in]  ctxt_pP          Running context.
* \param[in]  rlcP           RLC AM protocol instance pointer.
* \param[in]  pduP           Pointer on the header of the PDU in order to be able to set the poll bit if necessary.
* \param[in]  payload_sizeP  Size of the payload of the PDU.
*/
protected_rlc_am_segment(void rlc_am_pdu_polling (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *const rlcP, rlc_am_pdu_sn_10_t *pduP, int16_t payload_sizeP);)

/*! \fn void rlc_am_segment_10 (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t * const rlcP)
* \brief      Segment a PDU with 10 bits sequence number, based on segmentation information given by MAC (size to transmit).
* \param[in]  ctxt_pP        Running context.
* \param[in]  rlcP         RLC AM protocol instance pointer.
*/
protected_rlc_am_segment(void rlc_am_segment_10 (const protocol_ctxt_t* const  ctxt_pP,rlc_am_entity_t *const rlcP);)
/** @} */
#    endif
