/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*________________________openair_w3g4free_top.c________________________

 Authors : Hicham Anouar, Raymond Knopp
 Company : EURECOM
 Emails  : anouar@eurecom.fr,  knopp@eurecom.fr
________________________________________________________________*/

#ifndef USER_MODE
#define __NO_VERSION__

#include <rtai.h>
#include <rtai_fifos.h>


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
//#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h>

#include <linux/errno.h>

#ifdef KERNEL2_6
//#include <linux/config.h>
#include <linux/slab.h>
#endif

#ifdef KERNEL2_4
#include <linux/malloc.h>
#include <linux/wrapper.h>
#endif

#endif


#include <linux/bigphysarea.h>

//#include "defs.h"
//#include "openair_types.h"
#include "LAYER2/MAC/vars.h"
#include "LAYER2/MAC/defs.h"

extern void  macphy_scheduler(uint8_t);
extern MAC_xface *mac_register(void *, void*, void*, void*,void*);
extern int mac_unregister(MAC_xface *);
extern MAC_xface *mac_xface;
extern int mac_top_init();
extern int mac_init_global_param(void);
extern int pdcp_module_init(void);
extern void pdcp_module_cleanup(void);

/*------------------------------------------------*/
void w3g4free_mac_init(void)
{

  int ret;

#ifndef USER_MODE
  printk("[OPENAIR][MAC][TOP] INIT...\n");
#else
  printf("[OPENAIR][MAC][TOP] INIT...\n");
#endif// USER_MODE
  // if(mac_xface->is_cluster_head)
  // mac_top_init();
  //else
  mac_top_init();

#ifndef USER_MODE
  printk("[OPENAIR][MAC][TOP] INIT DONE.\n");
#endif//USER_MODE



}
/*------------------------------------------------*/
#ifndef USER_MODE

/*------------------------------------------------*/
/*   Prototypes                                   */
/*------------------------------------------------*/
#ifdef KERNEL2_4
static int   init_module( void );
static void  cleanup_module(void);
#else
static int   openair_mac_init_module( void );
static void  openair_mac_cleanup_module(void);
#endif

#ifdef KERNEL2_6
static int __init openair_mac_init_module( void )
#else
int init_module( void )
#endif //KERNEL2_6
{
  printk("[OPENAIR][MAC][INIT] inserting module\n");

  //    mac_init_global_param();

  mac_xface=mac_register(macphy_scheduler,NULL,w3g4free_mac_init,mrbch_phy_sync_failure,chbch_phy_sync_success);

  if( mac_xface == NULL ) {
    printk("[OPENAIR][MAC][INIT] Could not get MAC descriptor\n");
    return -1;
  } else {
    printk("[OPENAIR][MAC][INIT] Got MAC descriptor \n");

    if(mac_init_global_param()==-1) {
      mac_unregister(mac_xface);
      return -1;
    }
  }

  if (pdcp_module_init()!=0) {
    mac_unregister(mac_xface);
    return(-1);
  } else {
    printk("[OPENAIR][MAC][INIT] PDCP INIT OK\n");
  }


  return 0;
}

#ifdef KERNEL2_6
static void __exit openair_mac_cleanup_module(void)
#else
void cleanup_module(void)
#endif //KERNEL2_6
{
  printk("[OPENAIR][MAC][CLEANUP] cleanup module\n");
  mac_unregister(mac_xface);
  pdcp_module_cleanup();
}

MODULE_AUTHOR
("Lionel GAUTHIER <lionel.gauthier@eurecom.fr>, Raymond KNOPP <raymond.knopp@eurecom.fr>, Aawatif MENOUNI <aawatif.menouni@eurecom.fr>,Dominique NUSSBAUM <dominique.nussbaum@eurecom.fr>, Michelle WETTERWALD <michelle.wetterwald@eurecom.fr>, Maxime GUILLAUD <maxime.guillaud@eurecom.fr, Hicham ANOUAR <hicham.anouar@eurecom.fr>");
MODULE_DESCRIPTION ("openair MAC layer module");
MODULE_LICENSE ("GPL");
module_init (openair_mac_init_module);
module_exit (openair_mac_cleanup_module);


#endif //USER_MODE


