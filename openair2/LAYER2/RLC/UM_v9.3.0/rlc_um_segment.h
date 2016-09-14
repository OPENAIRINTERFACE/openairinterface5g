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
