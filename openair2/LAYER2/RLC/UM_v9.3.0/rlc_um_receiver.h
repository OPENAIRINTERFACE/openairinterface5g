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
#        ifdef RLC_UM_RECEIVER_C
#            define private_rlc_um_receiver(x)    x
#            define protected_rlc_um_receiver(x)  x
#            define public_rlc_um_receiver(x)     x
#        else
#            ifdef RLC_UM_MODULE
#                define private_rlc_um_receiver(x)
#                define protected_rlc_um_receiver(x)  extern x
#                define public_rlc_um_receiver(x)     extern x
#            else
#                define private_rlc_um_receiver(x)
#                define protected_rlc_um_receiver(x)
#                define public_rlc_um_receiver(x)     extern x
#            endif
#        endif

#        include "rlc_um_entity.h"
#        include "mac_primitives.h"

/*! \fn void rlc_um_display_rx_window(const protocol_ctxt_t* const ctxtP,rlc_um_entity_t * const rlc_pP)
* \brief    Display the content of the RX buffer, the output stream is targeted to TTY terminals because of escape sequences.
* \param[in]  ctxtP       Running context.
* \param[in]  rlc_pP      RLC UM protocol instance pointer.
*/
protected_rlc_um_receiver( void rlc_um_display_rx_window(const protocol_ctxt_t* const ctxtP, rlc_um_entity_t * const rlc_pP);)

/*! \fn void rlc_um_receive (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t * const rlc_pP, struct mac_data_ind data_indP)
* \brief    Handle the MAC data indication, retreive the transport blocks and send them one by one to the DAR process.
* \param[in]  ctxtP       Running context.
* \param[in]  rlc_pP      RLC UM protocol instance pointer.
* \param[in]  data_indP   Data indication structure containing transport block received from MAC layer.
*/
protected_rlc_um_receiver( void rlc_um_receive (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t * const rlc_pP, struct mac_data_ind data_indP));
/** @} */
#    endif
