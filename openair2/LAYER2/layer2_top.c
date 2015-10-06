/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
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


