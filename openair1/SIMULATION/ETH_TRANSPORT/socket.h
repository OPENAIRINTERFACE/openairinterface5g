/*! \file socket.h
* \brief
* \author Lionel Gauthier
* \date 2011
* \version 1.0
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#ifndef __SOCKET_H__
#    define __SOCKET_H__
#    ifdef SOCKET_C
#        define private_socket(x) x
#        define public_socket(x) x
#    else
#        define private_socket(x)
#        define public_socket(x) extern x
#    endif
#    include "stdint.h"
public_socket (void socket_setnonblocking (int sockP);
              )
public_socket (int make_socket_inet (int typeP, uint16_t * portP, struct sockaddr_in *ptr_addressP);
              )
#endif
