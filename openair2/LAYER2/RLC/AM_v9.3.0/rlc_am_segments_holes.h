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
/*! \file rlc_am_segments_holes.h
* \brief This file defines the prototypes of the functions dealing with the re-segmentation of RLC PDUs stored in the retransmission buffer,.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_am_segment_holes_impl_ RLC AM Resegmentation Implementation
* @ingroup _rlc_am_internal_segment_impl_
* @{
*/

#ifndef __RLC_AM_SEGMENT_HOLES_H__
#    define __RLC_AM_SEGMENT_HOLES_H__
//-----------------------------------------------------------------------------
#    ifdef RLC_AM_SEGMENT_HOLES_C
#        define private_rlc_am_segments_holes(x)    x
#        define protected_rlc_am_segments_holes(x)  x
#        define public_rlc_am_segments_holes(x)     x
#    else
#        ifdef RLC_AM_MODULE
#            define private_rlc_am_segments_holes(x)
#            define protected_rlc_am_segments_holes(x)  extern x
#            define public_rlc_am_segments_holes(x)     extern x
#        else
#            define private_rlc_am_segments_holes(x)
#            define protected_rlc_am_segments_holes(x)
#            define public_rlc_am_segments_holes(x)     extern x
#        endif
#    endif
/*! \fn void rlc_am_clear_holes (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t * const rlcP, rlc_sn_t snP)
* \brief      Remove all marked holes for PDU with sequence number "snP".
* \param[in]  ctxt_pP        Running context.
* \param[in]  rlcP           RLC AM protocol instance pointer.
* \param[in]  snP            Sequence number.
*/
protected_rlc_am_segments_holes(void rlc_am_clear_holes (
                                  const protocol_ctxt_t* const  ctxt_pP,
                                  rlc_am_entity_t *const rlcP,
                                  const rlc_sn_t snP);)

/*! \fn void rlc_am_remove_hole (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t * const rlcP,  rlc_sn_t snP, sdu_size_t so_startP, sdu_size_t so_stopP)
* \brief      Remove for PDU with sequence number "snP" a NACK for byte segment offset [so_startP, so_stopP].
* \param[in]  ctxt_pP        Running context.
* \param[in]  rlcP           RLC AM protocol instance pointer.
* \param[in]  snP            Sequence number.
* \param[in]  so_startP      Start of segment offset.
* \param[in]  so_stopP       End of segment offset.
*/
protected_rlc_am_segments_holes(void rlc_am_remove_hole (
                                  const protocol_ctxt_t* const  ctxt_pP,
                                  rlc_am_entity_t *const rlcP,
                                  const rlc_sn_t   snP,
                                  const sdu_size_t so_startP,
                                  const sdu_size_t so_stopP);)

/*! \fn void rlc_am_get_next_hole (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t * const rlcP, rlc_sn_t snP, sdu_size_t* so_startP, sdu_size_t* so_stopP)
* \brief      Get for PDU with sequence number "snP" the first hole start and stop parameters.
* \param[in]  ctxt_pP        Running context.
* \param[in]  rlcP           RLC AM protocol instance pointer.
* \param[in]  snP            Sequence number.
* \param[in,out]  so_startP  Start of segment offset.
* \param[in,out]  so_stopP   End of segment offset.
*/
protected_rlc_am_segments_holes(void rlc_am_get_next_hole (
                                  const protocol_ctxt_t* const  ctxt_pP,
                                  rlc_am_entity_t *const rlcP,
                                  const rlc_sn_t snP,
                                  sdu_size_t* const so_startP,
                                  sdu_size_t* const so_stopP);)

/*! \fn void rlc_am_add_hole (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t * const rlcP, rlc_sn_t snP, sdu_size_t so_startP, sdu_size_t so_stopP)
* \brief      Mark for PDU with sequence number "snP" a NACK for byte segment offset [so_startP, so_stopP].
* \param[in]  ctxt_pP        Running context.
* \param[in]  rlcP           RLC AM protocol instance pointer.
* \param[in]  snP            Sequence number.
* \param[in,out]  so_startP  Start of segment offset.
* \param[in,out]  so_stopP   End of segment offset.
*/
protected_rlc_am_segments_holes(void rlc_am_add_hole (
                                  const protocol_ctxt_t* const  ctxt_pP,
                                  rlc_am_entity_t *const rlcP,
                                  const rlc_sn_t snP,
                                  const sdu_size_t so_startP,
                                  sdu_size_t so_stopP);)
/** @} */
#endif
