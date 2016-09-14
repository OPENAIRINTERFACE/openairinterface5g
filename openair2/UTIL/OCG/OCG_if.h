/*! \file OCG_if.h
* \brief Interfaces to the outside of OCG
* \author Lusheng Wang and navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __CONFIGEN_IF_H__
#define __CONFIG_IF_H__

#ifdef __cplusplus
extern "C" {
#endif
/*--- INCLUDES ---------------------------------------------------------------*/
#    include "OCG.h"
/*----------------------------------------------------------------------------*/


#    ifdef COMPONENT_CONFIGEN
#        ifdef COMPONENT_CONFIGEN_IF
#            define private_configen_if(x) x
#            define friend_configen_if(x) x
#            define public_configen_if(x) x
#        else
#            define private_configen_if(x)
#            define friend_configen_if(x) extern x
#            define public_configen_if(x) extern x
#        endif
#    else
#        define private_configen_if(x)
#        define friend_configen_if(x)
#        define public_configen_if(x) extern x
#    endif

#ifdef __cplusplus
}
#endif

#endif
