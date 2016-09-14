/***************************************************************************
                          fifo_printf_proto_extern.h  -  description
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr



 ***************************************************************************/
#ifndef __FIFO_PRINTF_PROTO_EXTERN_H__
#    define __FIFO_PRINTF_PROTO_EXTERN_H__
#ifdef FIFO_PRINTF
extern void     fifo_printf_init (void);
extern void     fifo_printf_clean_up (void);
extern int      fifo_printf (const char *fmt, ...);
#else
extern int      fifo_printf_null (const char *fmt, ...);
#endif
#endif
