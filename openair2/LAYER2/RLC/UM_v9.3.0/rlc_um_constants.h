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
/*! \file rlc_um_constants.h
* \brief This file defines constant values used inside RLC UM.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/**
* @addtogroup _rlc_um_internal_impl_
* @{
*/
#    ifndef __RLC_UM_CONSTANTS_H__
#        define __RLC_UM_CONSTANTS_H__

/** The sequence numbering modulo (10 bits). */
#        define RLC_UM_SN_10_BITS_MODULO                    1024
/** The sequence numbering binary mask (10 bits). */
#        define RLC_UM_SN_10_BITS_MASK                      0x03FF
/** UM_Window_Size = 512 when a 10 bit SN is configured. */
#        define RLC_UM_WINDOW_SIZE_SN_10_BITS               512
/** The sequence numbering modulo (5 bits). */
#        define RLC_UM_SN_5_BITS_MODULO                     32
/** The sequence numbering binary mask (5 bits). */
#        define RLC_UM_SN_5_BITS_MASK                       0x1F
/** UM_Window_Size = 16 when a 5 bit SN is configured. */
#        define RLC_UM_WINDOW_SIZE_SN_5_BITS                16
// This constant is used by the receiving UM RLC entity to define SNs of those
// UMD PDUs that can be received without causing an advancement of the
// receiving window. UM_Window_Size = 16 when a 5 bit SN is configured,
// UM_Window_Size = 512 when a 10 bit SN is configured and UM_Window_Size = 0
// when the receiving UM RLC entity is configured for MCCH or MTCH.
// li field (values shifted 1 bit left)
#        define RLC_UM_SEGMENT_NB_MAX_LI_PER_PDU            24
//----------------------------------------------------------
// Events defined for state model of the acknowledged mode entity
/** Internal event defined for state model of the RLC UM. */
#        define RLC_UM_RECEIVE_CRLC_CONFIG_REQ_ENTER_NULL_STATE_EVENT                 0x00

/** Internal event defined for state model of the RLC UM. */
#        define RLC_UM_RECEIVE_CRLC_CONFIG_REQ_ENTER_DATA_TRANSFER_READY_STATE_EVENT  0x01

/** Internal event defined for state model of the RLC UM. */
#        define RLC_UM_RECEIVE_CRLC_SUSPEND_REQ_EVENT                                 0x10

/** Internal event defined for state model of the RLC UM. */
#        define RLC_UM_TRANSMIT_CRLC_SUSPEND_CNF_EVENT                                0x11

/** Internal event defined for state model of the RLC UM. */
#        define RLC_UM_RECEIVE_CRLC_RESUME_REQ_EVENT                                  0x12

/** Marker in RX buffer meaning that a PDU has been received. */
#        define RLC_UM_PDU_PREVIOUSLY_RECEIVED 0x01
/** @} */
#    endif
