/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*________________________openair_rrc_top.c________________________

 Authors : Hicham Anouar
 Company : EURECOM
 Emails  : anouar@eurecom.fr
________________________________________________________________*/


#ifndef USER_MODE
#define __NO_VERSION__

#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/segment.h>
#include <asm/page.h>
#include <asm/delay.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/mm.h>
#include <linux/mman.h>

#include <linux/slab.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h>

#include <linux/errno.h>
#ifdef KERNEL2_6
#include <linux/slab.h>
#endif

#ifdef KERNEL2_4
#include <linux/malloc.h>
#include <linux/wrapper.h>
#endif

#endif



#include "defs.h"
#include "vars.h"

extern MAC_RLC_XFACE* mac_rrc_register(RRC_XFACE*);
extern int mac_rrc_unregister(RRC_XFACE *);

extern void *bigphys_malloc(int);




#ifndef USER_MODE

/*------------------------------------------------*/
/*   Prototypes                                   */
/*------------------------------------------------*/
#ifdef KERNEL2_4
static int   init_module( void );
static void  cleanup_module(void);
#else
static int   openair_rrc_init_module( void );
static void  openair_rrc_cleanup_module(void);
#endif

#ifdef KERNEL2_6
static int __init openair_rrc_init_module( void )
#else
int init_module( void )
#endif //KERNEL2_6
{
  printk("[OPENAIR][RRC][INIT] inserting module\n");


  Rrc_xface = (RRC_XFACE*)malloc16(sizeof(RRC_XFACE));

  if(Rrc_xface == NULL) {
    printk("[RRC] FATAL EROOR: Could not allocate memory for Rrc_xface !!!\n");
    return (-1);
  }

  Mac_rlc_xface=mac_rrc_register(Rrc_xface);

  if( Mac_rlc_xface == NULL ) {
    printk("[OPENAIR][RRC][INIT] Could not get RRC descriptor\n");
    return -1;
  } else {
    printk("[OPENAIR][RRC][INIT] Got RRC descriptor , Rcc_xface %p,Mac_rlc_xface=%p \n",Rrc_xface,Mac_rlc_xface);
  }

  if(rrc_init_global_param()==-1) {
    printk("[OPENAIR][RRC][INIT] FATAL ERROR: INIT_GLOBAL_PARAM_NOK\n");
    return -1;
  }

  return 0;
}

#ifdef KERNEL2_6
static void __exit openair_rrc_cleanup_module(void)
#else
void cleanup_module(void)
#endif //KERNEL2_6
{
#ifndef NO_RRM
  rtf_destroy(RRC2RRM_FIFO);
  rtf_destroy(RRM2RRC_FIFO);
#endif //NO_RRM
  printk("[OPENAIR][RRC][CLEANUP] cleanup module\n");
  mac_rrc_unregister(Rrc_xface);

}

MODULE_AUTHOR
("Lionel GAUTHIER <lionel.gauthier@eurecom.fr>, Raymond KNOPP <raymond.knopp@eurecom.fr>, Aawatif MENOUNI <aawatif.menouni@eurecom.fr>,Dominique NUSSBAUM <dominique.nussbaum@eurecom.fr>, Michelle WETTERWALD <michelle.wetterwald@eurecom.fr>, Maxime GUILLAUD <maxime.guillaud@eurecom.fr, Hicham ANOUAR <hicham.anouar@eurecom.fr>");
MODULE_DESCRIPTION ("openair RRC layer module");
MODULE_LICENSE ("GPL");
module_init (openair_rrc_init_module);
module_exit (openair_rrc_cleanup_module);

#endif //USER_MODE


