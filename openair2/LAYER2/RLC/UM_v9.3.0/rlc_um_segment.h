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
/*! \file rlc_um_segment.h
* \brief This file defines the prototypes of the functions dealing with the segmentation of PDCP SDUs.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_um_segment_impl_ RLC UM Segmentation Implementation
* @ingroup _rlc_um_impl_
* @{
*/
#    ifndef __RLC_UM_SEGMENT_PROTO_EXTERN_H__
#        define __RLC_UM_SEGMENT_PROTO_EXTERN_H__
//-----------------------------------------------------------------------------
#        include "rlc_um_entity.h"
#        include "rlc_um_structs.h"
#        include "rlc_um_constants.h"
#        include "list.h"
//-----------------------------------------------------------------------------
#        ifdef RLC_UM_SEGMENT_C
#            define private_rlc_um_segment(x)    x
#            define protected_rlc_um_segment(x)  x
#            define public_rlc_um_segment(x)     x
#        else
#            ifdef RLC_UM_MODULE
#                define private_rlc_um_segment(x)
#                define protected_rlc_um_segment(x)  extern x
#                define public_rlc_um_segment(x)     extern x
#            else
#                define private_rlc_um_segment(x)
#                define protected_rlc_um_segment(x)
#                define public_rlc_um_segment(x)     extern x
#            endif
#        endif
/*! \fn void rlc_um_segment_10 (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP)
* \brief    Segmentation procedure with 10 bits sequence number, segment the first SDU in buffer and create a PDU of the size (nb_bytes_to_transmit) requested by MAC if possible and put it in the list "pdus_to_mac_layer".
* \param[in]  ctxtP       Running context.
* \param[in]  rlcP        RLC UM protocol instance pointer.
*/
protected_rlc_um_segment(void rlc_um_segment_10 (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP));


/*! \fn void rlc_um_segment_5 (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP)
* \brief    Segmentation procedure with 5 bits sequence number, segment the first SDU in buffer and create a PDU of the size (nb_bytes_to_transmit) requested by MAC if possible and put it in the list "pdus_to_mac_layer".
* \param[in]  ctxtP       Running context.
* \param[in]  rlcP        RLC UM protocol instance pointer.
*/
protected_rlc_um_segment(void rlc_um_segment_5 (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP));
/** @} */
#    endif
