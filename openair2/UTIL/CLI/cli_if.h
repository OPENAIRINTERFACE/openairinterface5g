/*! \file cli_if.h
* \brief cli interface
* \author Navid Nikaein
* \date 2011 - 2014
* \version 0.1
* \warning This component can be runned only in the user-space
* @ingroup util
*/
#ifndef __CLI_IF_H__
#    define __CLI_IF_H__


/*--- INCLUDES ---------------------------------------------------------------*/
#    include "cli.h"
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

#    ifdef COMPONENT_CLI
#        ifdef COMPONENT_CLI_IF
#            define private_cli_if(x) x
#            define friend_cli_if(x) x
#            define public_cli_if(x) x
#        else
#            define private_cli_if(x)
#            define friend_cli_if(x) extern x
#            define public_cli_if(x) extern x
#        endif
#    else
#        define private_cli_if(x)
#        define friend_cli_if(x)
#        define public_cli_if(x) extern x
#    endif

/** @defgroup _cli_if Interfaces of CLI
 * @{*/


public_cli_if( void cli_init (void); )
public_cli_if( int cli_server_init(cli_handler_t handler); )
public_cli_if(void cli_server_cleanup(void);)
public_cli_if(void cli_server_recv(const void * data, socklen_t len);)
/* @}*/

#ifdef __cplusplus
}
#endif

#endif

