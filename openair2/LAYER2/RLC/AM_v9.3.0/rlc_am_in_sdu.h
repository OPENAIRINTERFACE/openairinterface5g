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
/*! \file rlc_am_in_sdu.h
* \brief This file defines the prototypes of the utility functions manipulating the incoming SDU buffer.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_am_internal_input_sdu_impl_ RLC AM Input SDU buffer Internal Reference Implementation
* @ingroup _rlc_am_internal_impl_
* @{
*/
#    ifndef __RLC_AM_IN_SDU_H__
#        define __RLC_AM_IN_SDU_H__
//-----------------------------------------------------------------------------
#        ifdef RLC_AM_IN_SDU_C
#            define private_rlc_am_in_sdu(x)    x
#            define protected_rlc_am_in_sdu(x)  x
#            define public_rlc_am_in_sdu(x)     x
#        else
#            ifdef RLC_AM_MODULE
#                define private_rlc_am_in_sdu(x)
#                define protected_rlc_am_in_sdu(x)  extern x
#                define public_rlc_am_in_sdu(x)     extern x
#            else
#                define private_rlc_am_in_sdu(x)
#                define protected_rlc_am_in_sdu(x)
#                define public_rlc_am_in_sdu(x)     extern x
#            endif
#        endif
/*! \fn void rlc_am_free_in_sdu (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *rlcP, unsigned int index_in_bufferP)
* \brief    Free a higher layer SDU stored in input_sdus[] buffer.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlcP                      RLC AM protocol instance pointer.
* \param[in]  index_in_bufferP          Position index of the SDU.
* \note Update also the RLC AM instance variables nb_sdu, current_sdu_index, nb_sdu_no_segmented.
*/
protected_rlc_am_in_sdu(void rlc_am_free_in_sdu      (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *rlcP, unsigned int index_in_bufferP);)


/*! \fn void rlc_am_free_in_sdu_data (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *rlcP, unsigned int index_in_bufferP)
* \brief    Free a higher layer SDU data part, the SDU is stored in input_sdus[] buffer.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlcP                      RLC AM protocol instance pointer.
* \param[in]  index_in_bufferP          Position index of the SDU.
* \note This procedure is called when the SDU segmentation is done for this SDU. Update also the RLC AM instance variable nb_sdu_no_segmented.
*/
protected_rlc_am_in_sdu(void rlc_am_free_in_sdu_data (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *rlcP, unsigned int index_in_bufferP);)


/*! \fn signed int rlc_am_in_sdu_is_empty(const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *rlcP)
* \brief    Indicates if the input SDU buffer for incoming higher layer SDUs is empty or not.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlcP                      RLC AM protocol instance pointer.
* \return 1 if the buffer is empty, else 0.
*/
protected_rlc_am_in_sdu(signed int rlc_am_in_sdu_is_empty(const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *rlcP);)
/** @} */
#    endif
