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

/***************************************************************************
                          mem_block.c  -  description
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr


 ***************************************************************************/
#define MEM_BLOCK_C
//#include "rtos_header.h"
#include "mem_block.h"
#include "mem_pool.h"
#include "list.h"
#include "LAYER2/MAC/mac_extern.h"
#include "assertions.h"
/* all function calls are protected by a mutex
 * to ensure that many threads calling them at
 * the same time don't mess up.
 * We might be more clever in the future, it's a
 * bit overkill.
 * Commenting this define removes the protection,
 * so be careful with it.
 */
#define MEMBLOCK_BIG_LOCK

#ifdef MEMBLOCK_BIG_LOCK
static pthread_mutex_t mtex = PTHREAD_MUTEX_INITIALIZER;
#endif

//-----------------------------------------------------------------------------
//#define DEBUG_MEM_MNGT_FREE
//#define DEBUG_MEM_MNGT_ALLOC_SIZE
//#define DEBUG_MEM_MNGT_ALLOC
//-----------------------------------------------------------------------------
#if defined(DEBUG_MEM_MNGT_ALLOC)
uint32_t             counters[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif
//-----------------------------------------------------------------------------
/*
 * initialize all ures
 */
extern mem_pool  *memBlockVar;
void           *
pool_buffer_init (void)
{
  //-----------------------------------------------------------------------------

  uint32_t             index, mb_index, pool_index;
  mem_pool       *memory = (mem_pool *) &mem_block_var;
  memBlockVar=malloc(sizeof(mem_pool));
  int             pool_sizes[14] = { MEM_MNGT_MB0_NB_BLOCKS, MEM_MNGT_MB1_NB_BLOCKS,
                                     MEM_MNGT_MB2_NB_BLOCKS, MEM_MNGT_MB3_NB_BLOCKS,
                                     MEM_MNGT_MB4_NB_BLOCKS, MEM_MNGT_MB5_NB_BLOCKS,
                                     MEM_MNGT_MB6_NB_BLOCKS, MEM_MNGT_MB7_NB_BLOCKS,
                                     MEM_MNGT_MB8_NB_BLOCKS, MEM_MNGT_MB9_NB_BLOCKS,
                                     MEM_MNGT_MB10_NB_BLOCKS, MEM_MNGT_MB11_NB_BLOCKS,
                                     MEM_MNGT_MB12_NB_BLOCKS, MEM_MNGT_MBCOPY_NB_BLOCKS
                                   };

#ifdef MEMBLOCK_BIG_LOCK
  if (pthread_mutex_lock(&mtex)) abort();
#endif

  memset (memory, 0, sizeof (mem_pool));
  mb_index = 0;

  // LG_TEST
  for (pool_index = 0; pool_index <= MEM_MNGT_POOL_ID_COPY; pool_index++) {
    list_init (&memory->mem_lists[pool_index], "POOL");

    for (index = 0; index < pool_sizes[pool_index]; index++) {
      //memory->mem_blocks[mb_index + index].previous = NULL; -> done in memset 0
      //memory->mem_blocks[mb_index + index].next     = NULL; -> done in memset 0
      switch (pool_index) {
      case 0:
        memory->mem_blocks[mb_index + index].data = (unsigned char*)&(memory->mem_pool0[index][0]);
        break;

      case 1:
        memory->mem_blocks[mb_index + index].data = (unsigned char*)&(memory->mem_pool1[index][0]);
        break;

      case 2:
        memory->mem_blocks[mb_index + index].data = (unsigned char*)&(memory->mem_pool2[index][0]);
        break;

      case 3:
        memory->mem_blocks[mb_index + index].data = (unsigned char*)&(memory->mem_pool3[index][0]);
        break;

      case 4:
        memory->mem_blocks[mb_index + index].data = (unsigned char*)&(memory->mem_pool4[index][0]);
        break;

      case 5:
        memory->mem_blocks[mb_index + index].data = (unsigned char*)&(memory->mem_pool5[index][0]);
        break;

      case 6:
        memory->mem_blocks[mb_index + index].data = (unsigned char*)&(memory->mem_pool6[index][0]);
        break;

      case 7:
        memory->mem_blocks[mb_index + index].data = (unsigned char*)&(memory->mem_pool7[index][0]);
        break;

      case 8:
        memory->mem_blocks[mb_index + index].data = (unsigned char*)&(memory->mem_pool8[index][0]);
        break;

      case 9:
        memory->mem_blocks[mb_index + index].data = (unsigned char*)&(memory->mem_pool9[index][0]);
        break;

      case 10:
        memory->mem_blocks[mb_index + index].data = (unsigned char*)&(memory->mem_pool10[index][0]);
        break;

      case 11:
        memory->mem_blocks[mb_index + index].data = (unsigned char*)&(memory->mem_pool11[index][0]);
        break;

      case 12:
        memory->mem_blocks[mb_index + index].data = (unsigned char*)&(memory->mem_pool12[index][0]);
        break;

      default:
        ;
        memory->mem_blocks[mb_index + index].data = NULL;   // pool copy

      }

      memory->mem_blocks[mb_index + index].pool_id = pool_index;
      list_add_tail_eurecom (&memory->mem_blocks[mb_index + index], &memory->mem_lists[pool_index]);
    }

    mb_index += pool_sizes[pool_index];
  }

#ifdef MEMBLOCK_BIG_LOCK
  if (pthread_mutex_unlock(&mtex)) abort();
#endif

  return 0;
}

//-----------------------------------------------------------------------------
void           *
pool_buffer_clean (void *arg)
{
  //-----------------------------------------------------------------------------
  return 0;
}
//-----------------------------------------------------------------------------
void
free_mem_block (mem_block_t * leP, const char* caller)
{
  //-----------------------------------------------------------------------------

  if (!(leP)) {
    LOG_W (RLC,"[MEM_MNGT][FREE] WARNING FREE NULL MEM_BLOCK\n");
    return;
  }

#ifdef MEMBLOCK_BIG_LOCK
  if (pthread_mutex_lock(&mtex)) abort();
#endif

#ifdef DEBUG_MEM_MNGT_FREE
  LOG_D (RLC,"[MEM_MNGT][FREE] free_mem_block() %p pool: %d\n", leP, leP->pool_id);
#endif
#ifdef DEBUG_MEM_MNGT_ALLOC
  check_free_mem_block (leP);
#endif

  if (leP->pool_id <= MEM_MNGT_POOL_ID_COPY) {
    list_add_tail_eurecom (leP, &mem_block_var.mem_lists[leP->pool_id]);
#ifdef DEBUG_MEM_MNGT_ALLOC
    counters[leP->pool_id] -= 1;
    LGO_D (RLC,"[%s][MEM_MNGT][INFO] after pool[%2d] freed: counters = {%2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d}\n",
        caller, leP->pool_id,
        counters[0],counters[1],counters[2],counters[3],counters[4],
        counters[5],counters[6],counters[7],counters[8],counters[9],
        counters[10],counters[11]);
#endif
    leP = NULL;                 // this prevent from freeing the block twice
  } else {
    LOG_E (RLC,"[MEM_MNGT][FREE] ERROR free_mem_block() unknown pool_id : %d\n", leP->pool_id);
  }

#ifdef MEMBLOCK_BIG_LOCK
  if (pthread_mutex_unlock(&mtex)) abort();
#endif
}

//-----------------------------------------------------------------------------
mem_block_t      *
get_free_mem_block (uint32_t sizeP, const char* caller)
{
  //-----------------------------------------------------------------------------
  mem_block_t      *le = NULL;
  int             pool_selected;
  int             size;

  if (sizeP > MEM_MNGT_MB12_BLOCK_SIZE) {
    LOG_E (RLC,"[MEM_MNGT][ERROR][FATAL] size requested %d out of bounds\n", sizeP);
    display_mem_load ();
    AssertFatal(1==0,"get_free_mem_block size requested out of bounds");
    return NULL;
  }

#ifdef MEMBLOCK_BIG_LOCK
  if (pthread_mutex_lock(&mtex)) abort();
#endif

  size = sizeP >> 6;
  pool_selected = 0;

  while ((size)) {
    pool_selected += 1;
    size = size >> 1;
  }

  // pool is selected according to the size requested, now get a block
  // if no block is available pick one in an other pool
  do {
    if ((le = list_remove_head (&mem_block_var.mem_lists[pool_selected]))) {
#ifdef DEBUG_MEM_MNGT_ALLOC
      counters[pool_selected] += 1;
      LOG_D (RLC,"[%s][MEM_MNGT][INFO] after pool[%2d] allocated: counters = {%2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d}\n",
          caller,
          pool_selected,
          counters[0],counters[1],counters[2],counters[3],counters[4],
          counters[5],counters[6],counters[7],counters[8],counters[9],
          counters[10],counters[11]);
#endif
#ifdef DEBUG_MEM_MNGT_ALLOC_SIZE
      LOG_D (RLC,"[MEM_MNGT][INFO] ALLOC MEM_BLOCK SIZE %d bytes pool %d (%p)\n", sizeP, pool_selected,le);
#endif

      AssertFatal(le->pool_id == pool_selected, "Unexpected pool ID!");

#ifdef MEMBLOCK_BIG_LOCK
  if (pthread_mutex_unlock(&mtex)) abort();
#endif

      return le;
    }

#ifdef DEBUG_MEM_MNGT_ALLOC
    LOG_E (RLC,"[MEM_MNGT][ERROR][MINOR] memory pool %d is empty trying next pool alloc count = %d\n", pool_selected, counters[pool_selected]);
    //    display_mem_load ();
    //    check_mem_area ((void *)&mem_block_var);
#endif
  } while (pool_selected++ < 12);

  LOG_E(PHY, "[MEM_MNGT][ERROR][FATAL] failed allocating MEM_BLOCK size %d byes (pool_selected=%d size=%d)\n", sizeP, pool_selected, size);
//  display_mem_load();
//  AssertFatal(1==0,"get_free_mem_block failed");
  LOG_E(MAC,"[MEM_MNGT][ERROR][FATAL] get_free_mem_block failed!!!\n");
#ifdef MEMBLOCK_BIG_LOCK
  if (pthread_mutex_unlock(&mtex)) abort();
#endif

  return NULL;
};

//-----------------------------------------------------------------------------
mem_block_t      *
get_free_copy_mem_block (void)
{
  //-----------------------------------------------------------------------------
  mem_block_t      *le;

#ifdef MEMBLOCK_BIG_LOCK
  AssertFatal(0, "This function is not handled properly but not used anywhere. FIXME?\n");
#endif

  if ((le = list_remove_head (&mem_block_var.mem_lists[MEM_MNGT_POOL_ID_COPY]))) {
#ifdef DEBUG_MEM_MNGT_ALLOC_SIZE
    LOG_D (RLC,"[MEM_MNGT][INFO] ALLOC COPY MEM BLOCK (%p)\n",le);
#endif
#ifdef DEBUG_MEM_MNGT_ALLOC
      counters[MEM_MNGT_POOL_ID_COPY] += 1;
      LOG_D (RLC,"[MEM_MNGT][INFO] pool counters = {%2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d}\n",
          counters[0],counters[1],counters[2],counters[3],counters[4],
          counters[5],counters[6],counters[7],counters[8],counters[9],
          counters[10],counters[11]);
#endif
    return le;
  } else {
    LOG_E (RLC,"[MEM_MNGT][ERROR] POOL COPY IS EMPTY\n");
    //#ifdef DEBUG_MEM_MNGT_ALLOC
    check_mem_area ();
    //    break_point ();
    //#endif

    AssertFatal(1==0,"mem pool is empty");
    return NULL;
  }
}

//-----------------------------------------------------------------------------
mem_block_t      *
copy_mem_block (mem_block_t * leP, mem_block_t * destP)
{
  //-----------------------------------------------------------------------------

#ifdef MEMBLOCK_BIG_LOCK
  AssertFatal(0, "This function is not handled properly but not used anywhere. FIXME?\n");
#endif

  if ((destP != NULL) && (leP != NULL) && (destP->pool_id == MEM_MNGT_POOL_ID_COPY)) {
    destP->data = leP->data;
  } else {
    LOG_E (RLC,"[MEM_MNGT][COPY] copy_mem_block() pool dest src or dest is NULL\n");
  }

  return destP;
}

//-----------------------------------------------------------------------------
void
display_mem_load (void)
{
  //-----------------------------------------------------------------------------
#ifdef MEMBLOCK_BIG_LOCK
  /* this function does not need to be protected, do nothing */
#endif

  mem_pool       *memory = (mem_pool *) &mem_block_var;

  LOG_D (RLC,"POOL 0 (%d elements of %d Bytes): ", MEM_MNGT_MB0_NB_BLOCKS, MEM_MNGT_MB0_BLOCK_SIZE);
  list_display (&memory->mem_lists[MEM_MNGT_POOL_ID0]);
  LOG_D (RLC,"POOL 1 (%d elements of %d Bytes): ", MEM_MNGT_MB1_NB_BLOCKS, MEM_MNGT_MB1_BLOCK_SIZE);
  list_display (&memory->mem_lists[MEM_MNGT_POOL_ID1]);
  LOG_D (RLC,"POOL 2 (%d elements of %d Bytes): ", MEM_MNGT_MB2_NB_BLOCKS, MEM_MNGT_MB2_BLOCK_SIZE);
  list_display (&memory->mem_lists[MEM_MNGT_POOL_ID2]);
  LOG_D (RLC,"POOL 3 (%d elements of %d Bytes): ", MEM_MNGT_MB3_NB_BLOCKS, MEM_MNGT_MB3_BLOCK_SIZE);
  list_display (&memory->mem_lists[MEM_MNGT_POOL_ID3]);
  LOG_D (RLC,"POOL 4 (%d elements of %d Bytes): ", MEM_MNGT_MB4_NB_BLOCKS, MEM_MNGT_MB4_BLOCK_SIZE);
  list_display (&memory->mem_lists[MEM_MNGT_POOL_ID4]);
  LOG_D (RLC,"POOL 5 (%d elements of %d Bytes): ", MEM_MNGT_MB5_NB_BLOCKS, MEM_MNGT_MB5_BLOCK_SIZE);
  list_display (&memory->mem_lists[MEM_MNGT_POOL_ID5]);
  LOG_D (RLC,"POOL 6 (%d elements of %d Bytes): ", MEM_MNGT_MB6_NB_BLOCKS, MEM_MNGT_MB6_BLOCK_SIZE);
  list_display (&memory->mem_lists[MEM_MNGT_POOL_ID6]);
  LOG_D (RLC,"POOL 7 (%d elements of %d Bytes): ", MEM_MNGT_MB7_NB_BLOCKS, MEM_MNGT_MB7_BLOCK_SIZE);
  list_display (&memory->mem_lists[MEM_MNGT_POOL_ID7]);
  LOG_D (RLC,"POOL 8 (%d elements of %d Bytes): ", MEM_MNGT_MB8_NB_BLOCKS, MEM_MNGT_MB8_BLOCK_SIZE);
  list_display (&memory->mem_lists[MEM_MNGT_POOL_ID8]);
  LOG_D (RLC,"POOL 9 (%d elements of %d Bytes): ", MEM_MNGT_MB9_NB_BLOCKS, MEM_MNGT_MB9_BLOCK_SIZE);
  list_display (&memory->mem_lists[MEM_MNGT_POOL_ID9]);
  LOG_D (RLC,"POOL 10 (%d elements of %d Bytes): ", MEM_MNGT_MB10_NB_BLOCKS, MEM_MNGT_MB10_BLOCK_SIZE);
  list_display (&memory->mem_lists[MEM_MNGT_POOL_ID10]);
  LOG_D (RLC,"POOL 11 (%d elements of %d Bytes): ", MEM_MNGT_MB11_NB_BLOCKS, MEM_MNGT_MB11_BLOCK_SIZE);
  list_display (&memory->mem_lists[MEM_MNGT_POOL_ID11]);
  LOG_D (RLC,"POOL 12 (%d elements of %d Bytes): ", MEM_MNGT_MB12_NB_BLOCKS, MEM_MNGT_MB12_BLOCK_SIZE);
  list_display (&memory->mem_lists[MEM_MNGT_POOL_ID12]);
  LOG_D (RLC,"POOL C (%d elements): ", MEM_MNGT_MBCOPY_NB_BLOCKS);
  list_display (&memory->mem_lists[MEM_MNGT_POOL_ID_COPY]);
}

//-----------------------------------------------------------------------------
void
check_mem_area (void)
{
  //-----------------------------------------------------------------------------
  int             index, mb_index;
  mem_pool       *memory = (mem_pool *) &mem_block_var;

#ifdef MEMBLOCK_BIG_LOCK
  AssertFatal(0, "This function is not handled properly but not used anywhere. FIXME?\n");
#endif

  for (index = 0; index < MEM_MNGT_MB0_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[index].data != (unsigned char*)&(memory->mem_pool0[index][0])) && (memory->mem_blocks[index].pool_id != MEM_MNGT_POOL_ID0)) {
      LOG_D (RLC,"[MEM] ERROR POOL0 block index %d\n", index);
    }
  }

  mb_index = MEM_MNGT_MB0_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB1_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != (unsigned char*)&(memory->mem_pool1[index][0]))
        && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID1)) {
      LOG_D (RLC,"[MEM] ERROR POOL1 block index %d\n", index);
    }
  }

  mb_index += MEM_MNGT_MB1_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB2_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != (unsigned char*)&(memory->mem_pool2[index][0]))
        && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID2)) {
      LOG_D (RLC,"[MEM] ERROR POOL2 block index %d\n", index);
    }
  }

  mb_index += MEM_MNGT_MB2_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB3_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != (unsigned char*)&(memory->mem_pool3[index][0]))
        && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID3)) {
      LOG_D (RLC,"[MEM] ERROR POOL3 block index %d\n", index);
    }
  }

  mb_index += MEM_MNGT_MB3_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB4_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != (unsigned char*)&(memory->mem_pool4[index][0]))
        && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID4)) {
      LOG_D (RLC,"[MEM] ERROR POOL4 block index %d\n", index);
    }
  }

  mb_index += MEM_MNGT_MB4_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB5_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != (unsigned char*)&(memory->mem_pool5[index][0]))
        && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID5)) {
      LOG_D (RLC,"[MEM] ERROR POOL5 block index %d\n", index);
    }
  }

  mb_index += MEM_MNGT_MB5_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB6_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != (unsigned char*)&(memory->mem_pool6[index][0]))
        && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID6)) {
      LOG_D (RLC,"[MEM] ERROR POOL6 block index %d\n", index);
    }
  }

  mb_index += MEM_MNGT_MB6_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB7_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != (unsigned char*)&(memory->mem_pool7[index][0]))
        && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID7)) {
      LOG_D (RLC,"[MEM] ERROR POOL7 block index %d\n", index);
    }
  }

  mb_index += MEM_MNGT_MB7_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB8_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != (unsigned char*)&(memory->mem_pool8[index][0]))
        && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID8)) {
      LOG_D (RLC,"[MEM] ERROR POOL8 block index %d\n", index);
    }
  }

  mb_index += MEM_MNGT_MB8_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB9_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != (unsigned char*)&(memory->mem_pool9[index][0]))
        && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID9)) {
      LOG_D (RLC,"[MEM] ERROR POOL9 block index %d\n", index);
    }
  }

  mb_index += MEM_MNGT_MB9_NB_BLOCKS;

  for (index = mb_index; index < MEM_MNGT_MB10_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != (unsigned char*)&(memory->mem_pool10[index][0]))
        && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID10)) {
      LOG_D (RLC,"[MEM] ERROR POOL10 block index %d\n", index);
    }
  }

  mb_index += MEM_MNGT_MB10_NB_BLOCKS;

  for (index = mb_index; index < MEM_MNGT_MB11_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != (unsigned char*)&(memory->mem_pool11[index][0]))
        && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID11)) {
      LOG_D (RLC,"[MEM] ERROR POOL11 block index %d\n", index);
    }
  }

  mb_index += MEM_MNGT_MB11_NB_BLOCKS;

  for (index = mb_index; index < MEM_MNGT_MB12_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != (unsigned char*)&(memory->mem_pool12[index][0]))
        && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID12)) {
      LOG_D (RLC,"[MEM] ERROR POOL12 block index %d\n", index);
    }
  }

  mb_index += MEM_MNGT_MB12_NB_BLOCKS;

  for (index = mb_index; index < MEM_MNGT_NB_ELEMENTS; index++) {
    if ((memory->mem_blocks[index].data != NULL) && (memory->mem_blocks[index].pool_id != MEM_MNGT_POOL_ID_COPY)) {
      LOG_D (RLC,"[MEM] ERROR POOL COPY block index %d\n", index);
    }
  }
}

//-----------------------------------------------------------------------------
void
check_free_mem_block (mem_block_t * leP)
{
  //-----------------------------------------------------------------------------
  ptrdiff_t             block_index;

#ifdef MEMBLOCK_BIG_LOCK
  /* this function does not SEEM TO need to be protected, do nothing (for the moment) */
#endif

  if ((leP >= mem_block_var.mem_blocks) && (leP <= &mem_block_var.mem_blocks[MEM_MNGT_NB_ELEMENTS-1])) {
    // the pointer is inside the memory region
    block_index = leP - mem_block_var.mem_blocks;
    // block_index is now: 0 <= block_index < MEM_MNGT_NB_ELEMENTS

    if (block_index < MEM_MNGT_MB0_NB_BLOCKS) {
      if ((leP->data != (unsigned char*)mem_block_var.mem_pool0[block_index]) && (leP->pool_id != MEM_MNGT_POOL_ID0)) {
        LOG_D (RLC,"[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }

      return;
    }

    block_index -= MEM_MNGT_MB0_NB_BLOCKS;

    if (block_index < MEM_MNGT_MB1_NB_BLOCKS) {
      if ((leP->data != (unsigned char*)mem_block_var.mem_pool1[block_index]) && (leP->pool_id != MEM_MNGT_POOL_ID1)) {
        LOG_D (RLC,"[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }

      return;
    }

    block_index -= MEM_MNGT_MB1_NB_BLOCKS;

    if (block_index < MEM_MNGT_MB2_NB_BLOCKS) {
      if ((leP->data != (unsigned char*)mem_block_var.mem_pool2[block_index]) && (leP->pool_id != MEM_MNGT_POOL_ID2)) {
        LOG_D (RLC,"[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }

      return;
    }

    block_index -= MEM_MNGT_MB2_NB_BLOCKS;

    if (block_index < MEM_MNGT_MB3_NB_BLOCKS) {
      if ((leP->data != (unsigned char*)mem_block_var.mem_pool3[block_index]) && (leP->pool_id != MEM_MNGT_POOL_ID3)) {
        LOG_D (RLC,"[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }

      return;
    }

    block_index -= MEM_MNGT_MB3_NB_BLOCKS;

    if (block_index < MEM_MNGT_MB4_NB_BLOCKS) {
      if ((leP->data != (unsigned char*)mem_block_var.mem_pool4[block_index]) && (leP->pool_id != MEM_MNGT_POOL_ID4)) {
        LOG_D (RLC,"[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }

      return;
    }

    block_index -= MEM_MNGT_MB4_NB_BLOCKS;

    if (block_index < MEM_MNGT_MB5_NB_BLOCKS) {
      if ((leP->data != (unsigned char*)mem_block_var.mem_pool5[block_index]) && (leP->pool_id != MEM_MNGT_POOL_ID5)) {
        LOG_D (RLC,"[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }

      return;
    }

    block_index -= MEM_MNGT_MB5_NB_BLOCKS;

    if (block_index < MEM_MNGT_MB6_NB_BLOCKS) {
      if ((leP->data != (unsigned char*)mem_block_var.mem_pool6[block_index]) && (leP->pool_id != MEM_MNGT_POOL_ID6)) {
        LOG_D (RLC,"[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }

      return;
    }

    block_index -= MEM_MNGT_MB6_NB_BLOCKS;

    if (block_index < MEM_MNGT_MB7_NB_BLOCKS) {
      if ((leP->data != (unsigned char*)mem_block_var.mem_pool7[block_index]) && (leP->pool_id != MEM_MNGT_POOL_ID7)) {
        LOG_D (RLC,"[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }

      return;
    }

    block_index -= MEM_MNGT_MB7_NB_BLOCKS;

    if (block_index < MEM_MNGT_MB8_NB_BLOCKS) {
      if ((leP->data != (unsigned char*)mem_block_var.mem_pool8[block_index]) && (leP->pool_id != MEM_MNGT_POOL_ID8)) {
        LOG_D (RLC,"[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }

      return;
    }

    block_index -= MEM_MNGT_MB8_NB_BLOCKS;

    if (block_index < MEM_MNGT_MB9_NB_BLOCKS) {
      if ((leP->data != (unsigned char*)mem_block_var.mem_pool9[block_index]) && (leP->pool_id != MEM_MNGT_POOL_ID9)) {
        LOG_D (RLC,"[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }

      return;
    }

    block_index -= MEM_MNGT_MB9_NB_BLOCKS;

    if (block_index < MEM_MNGT_MB10_NB_BLOCKS) {
      if ((leP->data != (unsigned char*)mem_block_var.mem_pool10[block_index]) && (leP->pool_id != MEM_MNGT_POOL_ID10)) {
        LOG_D (RLC,"[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }

      return;
    }

    block_index -= MEM_MNGT_MB10_NB_BLOCKS;

    if (block_index < MEM_MNGT_MB11_NB_BLOCKS) {
      if ((leP->data != (unsigned char*)mem_block_var.mem_pool11[block_index]) && (leP->pool_id != MEM_MNGT_POOL_ID11)) {
        LOG_D (RLC,"[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }

      return;
    }

    block_index -= MEM_MNGT_MB11_NB_BLOCKS;

    if (block_index < MEM_MNGT_MB12_NB_BLOCKS) {
      if ((leP->data != (unsigned char*)mem_block_var.mem_pool12[block_index]) && (leP->pool_id != MEM_MNGT_POOL_ID12)) {
        LOG_D (RLC,"[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }

      return;
    }
  } else {
    LOG_D (RLC,"[MEM][ERROR][FATAL] free mem block is corrupted\n");
  }

  // the block is ok
}
