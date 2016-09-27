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

#ifndef __openair_DEFS_H__
#define __openair_DEFS_H__

#ifndef USER_MODE
#include <linux/kernel.h>
#endif //USER_MODE


#ifdef USER_MODE
#include <stdio.h>
#include <stdlib.h>
#if !defined (msg)
# define msg(aRGS...) LOG_D(PHY, ##aRGS)
#endif
#ifndef malloc16
#  ifdef __AVX2__
#    define malloc16(x) memalign(32,x)
#  else
#    define malloc16(x) memalign(16,x)
#  endif
#endif
#define free16(y,x) free(y)
#define bigmalloc malloc
#define bigmalloc16 malloc16
#define openair_free(y,x) free((y))
#define PAGE_SIZE 4096

#define PAGE_MASK 0xfffff000
#define virt_to_phys(x) (x)

#else // USER_MODE
#include <rtai.h>
#define msg rt_printk

#ifdef BIGPHYSAREA

#define bigmalloc(x) (bigphys_malloc(x))
#define bigmalloc16(x) (bigphys_malloc(x))

#define malloc16(x) (bigphys_malloc(x))
#define free16(y,x)

#define bigfree(y,x)

#else // BIGPHYSAREA

#define bigmalloc(x) (dma_alloc_coherent(pdev[0],(x),&dummy_dma_ptr,0))
#define bigmalloc16(x) (dma_alloc_coherent(pdev[0],(x),&dummy_dma_ptr,0))
#define bigfree(y,x) (dma_free_coherent(pdev[0],(x),(void *)(y),dummy_dma_ptr))
#define malloc16(x) (kmalloc(x,GFP_KERNEL))
#define free16(y,x) (kfree(y))

#endif // BIGPHYSAREA


#ifdef CBMIMO1
#define openair_get_mbox() (*(unsigned int *)mbox)
#else //CBMIMO1
#define openair_get_mbox() (*(unsigned int *)PHY_vars->mbox>>1)
#endif //CBMIMO1


#endif // USERMODE

// #define bzero(s,n) (memset((s),0,(n)))

#define cmax(a,b)  ((a>b) ? (a) : (b))
#define cmin(a,b)  ((a<b) ? (a) : (b))
#endif // /*__openair_DEFS_H__ */


