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
/*! \file rlc_tm.h
* \brief This file, and only this file must be included by code that interact with RLC TM layer.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_tm_impl_ RLC TM Layer Reference Implementation
* @ingroup _rlc_impl_
* @{
*/
#    ifndef __RLC_TM_H__
#        define __RLC_TM_H__
#        ifdef RLC_TM_C
#            define private_rlc_tm(x)
#            define protected_rlc_tm(x)
#            define public_rlc_tm(x)
#        else
#            ifdef RLC_TM_MODULE
#                define private_rlc_tm(x)
#                define protected_rlc_tm(x)  extern x
#                define public_rlc_tm(x)     extern x
#            else
#                define private_rlc_tm(x)
#                define protected_rlc_tm(x)
#                define public_rlc_tm(x)     extern x
#            endif
#        endif
#        include "platform_types.h"
#        include "rlc_def.h"
#        include "rlc_def_lte.h"
#        include "rlc_tm_structs.h"
#        include "rlc_tm_entity.h"
#        include "mem_block.h"
#        include "rlc_tm_init.h"

#define PROTOCOL_RLC_TM_CTXT_FMT PROTOCOL_CTXT_FMT"[%s %02u]"
#define PROTOCOL_RLC_TM_CTXT_ARGS(CTXT_Pp, rLC_Pp) PROTOCOL_CTXT_ARGS(CTXT_Pp),\
          (rLC_Pp->is_data_plane) ? "DRB TM" : "SRB TM",\
          rLC_Pp->rb_id


/*! \fn void rlc_tm_send_sdu (
*        const protocol_ctxt_t* const  ctxt_pP,
*        rlc_tm_entity_t * const rlc_pP,
*        const boolean_t         error_indicationP,
*        uint8_t * const         srcP,
*        const sdu_size_t        length_in_bitsP)
* \brief    Send SDU if any reassemblied to upper layer.
* \param[in]  rlcP                RLC TM protocol instance pointer.
* \param[in]  error_indicationP   Error indicated by lower layers for this SDU.
* \param[in]  srcP                SDU data pointer.
* \param[in]  length_in_bitsP     Length of SDU in bits.
*/
private_rlc_tm(void rlc_tm_send_sdu (
                 const protocol_ctxt_t* const  ctxt_pP,
                 rlc_tm_entity_t * const rlc_pP,
                 const boolean_t         error_indicationP,
                 uint8_t * const         srcP,
                 const sdu_size_t        length_in_bitsP);)

/*! \fn void rlc_tm_no_segment (const protocol_ctxt_t* const  ctxt_pP, rlc_tm_entity_t * const rlcP)
* \brief    Schedule a SDU to be transmited by lower layers.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlcP                RLC TM protocol instance pointer.
*/
private_rlc_tm(void rlc_tm_no_segment (
                 const protocol_ctxt_t* const  ctxt_pP,
                 rlc_tm_entity_t *const rlcP);)

/*! \fn void rlc_tm_rx (const protocol_ctxt_t* const  ctxt_pP,void *const rlcP, struct mac_data_ind data_indP)
* \brief    Process the received PDUs from lower layer.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlcP                      RLC TM protocol instance pointer.
* \param[in]  data_indP                 PDUs from MAC.
*/
private_rlc_tm( void     rlc_tm_rx (
                  const protocol_ctxt_t* const  ctxt_pP,
                  void *const rlcP,
                  struct mac_data_ind data_indP);)


/*! \fn struct mac_status_resp rlc_tm_mac_status_indication (const protocol_ctxt_t* const  ctxt_pP, void * const rlcP, const uint16_t tbs_sizeP, struct mac_status_ind tx_statusP)
* \brief    Request the maximum number of bytes that can be served by RLC instance to MAC and fix the amount of bytes requested by MAC for next RLC transmission.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlcP                      RLC TM protocol instance pointer.
* \param[in]  tbs_sizeP                 Number of bytes requested by MAC for next transmission.
* \param[in]  tx_statusP                Transmission status given by MAC on previous MAC transmission of the PDU.
* \return     The maximum number of bytes that can be served by RLC instance to MAC.
*/
public_rlc_tm( struct mac_status_resp rlc_tm_mac_status_indication (
                 const protocol_ctxt_t* const  ctxt_pP,
                 void * const rlcP,
                 const uint16_t tb_sizeP,
                 struct mac_status_ind tx_statusP);)


/*! \fn struct mac_data_req rlc_tm_mac_data_request (const protocol_ctxt_t* const  ctxt_pP, void * const rlcP)
* \brief    Gives PDUs to lower layer MAC.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlcP                      RLC TM protocol instance pointer.
* \return     A PDU of the previously requested number of bytes, and the updated maximum number of bytes that can be served by RLC instance to MAC for next RLC transmission.
*/
public_rlc_tm( struct mac_data_req  rlc_tm_mac_data_request (
                 const protocol_ctxt_t* const  ctxt_pP,
                 void * const rlcP);)


/*! \fn void     rlc_tm_mac_data_indication (const protocol_ctxt_t* const  ctxt_pP,void * const rlcP, struct mac_data_ind data_indP)
* \brief    Receive PDUs from lower layer MAC.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlcP                      RLC TM protocol instance pointer.
* \param[in]  data_indP                 PDUs from MAC.
*/
public_rlc_tm( void rlc_tm_mac_data_indication (
                 const protocol_ctxt_t* const  ctxt_pP,
                 void * const rlcP,
                 struct mac_data_ind data_indP);)


/*! \fn void     rlc_tm_data_req (const protocol_ctxt_t* const  ctxt_pP,void * const rlcP, mem_block_t * const sduP)
* \brief    Interface with higher layers, buffer higher layer SDUS for transmission.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlcP                      RLC TM protocol instance pointer.
* \param[in]  sduP                      SDU. (A struct rlc_tm_data_req is mapped on sduP->data.)
*/
public_rlc_tm(    void     rlc_tm_data_req (
                    const protocol_ctxt_t* const  ctxt_pP,
                    void * const rlcP,
                    mem_block_t * const sduP);)

/** @} */
#    endif
