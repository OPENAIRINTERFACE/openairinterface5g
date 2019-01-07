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

#include "defs.h"
#include "linux/module.h"

#ifdef BIGPHYSAREA
#ifdef ARCH_64
char *bigphys_ptr,*bigphys_current;
#else //ARCH_64
unsigned int bigphys_ptr,bigphys_current;
#endif //ARCH_64

// return pointer to memory in big physical area aligned to 16 bytes

void* bigphys_malloc(int n)
{



  int n2 = n + ((16-(n%16))%16);
#ifdef ARCH_64
  char *bigphys_old;
#else
  unsigned int bigphys_old;
#endif

  printk("[BIGPHYSAREA] Calling bigphys_malloc for %d (%d) bytes\n",n,n2);

#ifdef ARCH_64
  printk("[BIGPHYSAREA] Allocated Memory @ %p\n",bigphys_current);
#endif
  bigphys_old = bigphys_current;
  bigphys_current += n2;

#ifdef ARCH_64
  printk("[BIGPHYSAREA] Allocated Memory top now @ %p\n",bigphys_current);
  return ((void *)(bigphys_old));
#else //ARCH_64
  //printk("[BIGPHYSAREA] Allocated Memory %d\n",bigphys_current-bigphys_ptr);
  return ((void *)(bigphys_old));
#endif //ARCH_64
}

EXPORT_SYMBOL(bigphys_malloc);
#endif



