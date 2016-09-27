/*!\brief SCHED external variables */

#ifndef __SCHED_EXTERN_H__
#define __SCHED_EXTERN_H__

#ifndef USER_MODE
#define __NO_VERSION__


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>

#include <asm/io.h>
#include <asm/bitops.h>

#include <asm/uaccess.h>
#include <asm/segment.h>
#include <asm/page.h>




#ifdef RTAI_ENABLED
#include <rtai.h>
//#include <rtai_posix.h>
#include <rtai_fifos.h>
#include <rtai_sched.h>
#include <rtai_sem.h>
//#include "rt_compat.h"

#else
#include <unistd.h>
#endif

#endif  /* USER_MODE */

#include "defs.h"
//#include "dlc_engine.h"

extern int openair_sched_status;

//extern int exit_PHY;
//extern int exit_PHY_ack;

extern int synch_wait_cnt;


extern int16_t hundred_times_delta_TF[100];
extern uint16_t hundred_times_log10_NPRB[100];
/*
#ifdef EMOS
extern fifo_dump_emos_UE emos_dump_UE;
extern fifo_dump_emos_eNB emos_dump_eNB;
#endif
*/

#endif /*__SCHED_EXTERN_H__ */
