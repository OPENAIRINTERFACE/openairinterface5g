/***************************************************************************
                          print.h  -  description
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 ***************************************************************************/
#ifndef __PRINT_H__
#    define __PRINT_H__
#    ifdef USER_MODE
#        define msg printf
#    else
#        ifdef FIFO_PRINTF
#            include "fifo_printf_proto_extern.h"
#            define msg fifo_printf
#        else
#                define msg fifo_printf_null
#        endif
#    endif
#endif
