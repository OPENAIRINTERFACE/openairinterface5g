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

/*! \file rtai_mem.c
* \brief a wrapper for Unified RTAI real-time memory management.
* \author Florian Kaltenberger
* \date 2011-04-06
* \version 0.1
* \company Eurecom
* \email: florian.kaltenberger@eurecom.fr
* \note
* \bug
* \warning
*/

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

#include <linux/slab.h>

#include <asm/rtai.h>
#include <rtai.h>
#include <rtai_shm.h>
#include <rtai_malloc.h>

/*
void* rt_alloc_wrapper(int size) {

  unsigned long* tmp_ptr;
  static unsigned long name = 0;

  rt_shm_free(name);
  tmp_ptr = (unsigned long*) rt_shm_alloc(name,size+sizeof(unsigned long),GFP_KERNEL);

  if (!tmp_ptr) {
    printk("rt_mem.c: failed to allocate %d bytes for name %ld at %p\n",size,name,tmp_ptr);
    return NULL;
  }

  printk("rt_mem.c: allocated %d bytes for name %ld at %p\n",size,name,tmp_ptr+1);

  tmp_ptr[0] = name;
  name++;

  return (void*) (tmp_ptr+1);

}

int rt_free_wrapper(void* ptr) {

  unsigned long name;

  printk("rt_mem.c: freeing memory at %p, ",ptr);

  name = *(((unsigned long*) ptr)-1);

  printk("name %ld\n",name);

  return rt_shm_free(name);

}

void* rt_realloc_wrapper(void* oldptr, int size) {

  void* newptr;

  printk("rt_mem.c: reallocating %p with %d bytes\n",oldptr,size);

  if (oldptr==NULL)
    newptr = rt_alloc_wrapper(size);
  else {
    newptr = rt_alloc_wrapper(size);
    memcpy(newptr,oldptr,size);
    rt_free_wrapper(oldptr);
  }

  return newptr;
}
*/

void* rt_realloc(void* oldptr, int size)
{

  void* newptr;

  printk("rt_mem.c: reallocating %p with %d bytes\n",oldptr,size);

  if (oldptr==NULL) {
    newptr = rt_malloc(size);
  } else {
    newptr = rt_malloc(size);
    memcpy(newptr,oldptr,size);
    rt_free(oldptr);
  }

  return newptr;
}

void* rt_calloc(int nmemb, int size)
{

  void* newptr;

  printk("rt_mem.c: allocating %d elements with %d bytes\n",nmemb,size);

  newptr = rt_malloc(nmemb*size);
  memset(newptr,0,nmemb*size);

  return newptr;
}
