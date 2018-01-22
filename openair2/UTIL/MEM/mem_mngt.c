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
                          mem_mngt.c  -  description
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr


 ***************************************************************************/
#define MEM_MNGT_C
#include "rtos_header.h"
#include "platform.h"
#include "protocol_vars_extern.h"
#include "print.h"

#include "mem_block.h"
#include "mem_pool.h"
#include "lists_proto_extern.h"
#include "umts_sched.h"
//-----------------------------------------------------------------------------
#define DEBUG_MEM_MNGT_FREE
#define DEBUG_MEM_MNGT_ALLOC_SIZE
#define DEBUG_MEM_MNGT_ALLOC
#ifdef DEBUG_MEM_MNGT_ADDR
#    define   PRINT_MEM_MNGT_ADDR msg
#else
#    define   PRINT_MEM_MNGT_ADDR
//
#endif
//-----------------------------------------------------------------------------
uint32_t             counters[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//-----------------------------------------------------------------------------
/*
 * initialize all ures
 */
void           *
pool_buffer_init (void *arg)
{
  //-----------------------------------------------------------------------------

  uint32_t             index, mb_index, pool_index;
  mem_pool       *memory = (mem_pool *) arg;
  int             pool_sizes[11] = { MEM_MNGT_MB0_NB_BLOCKS, MEM_MNGT_MB1_NB_BLOCKS,
                                     MEM_MNGT_MB2_NB_BLOCKS, MEM_MNGT_MB3_NB_BLOCKS,
                                     MEM_MNGT_MB4_NB_BLOCKS, MEM_MNGT_MB5_NB_BLOCKS,
                                     MEM_MNGT_MB6_NB_BLOCKS, MEM_MNGT_MB7_NB_BLOCKS,
                                     MEM_MNGT_MB8_NB_BLOCKS, MEM_MNGT_MB9_NB_BLOCKS,
                                     MEM_MNGT_MBCOPY_NB_BLOCKS
                                   };

  memset (memory, 0, sizeof (mem_pool));
  mb_index = 0;

  // LG_TEST
  for (pool_index = 0; pool_index <= MEM_MNGT_POOL_ID_COPY; pool_index++) {
    init_list (&memory->mem_lists[pool_index], "POOL");

    for (index = 0; index < pool_sizes[pool_index]; index++) {
      //memory->mem_blocks[mb_index + index].previous = NULL; -> done in memset 0
      //memory->mem_blocks[mb_index + index].next     = NULL; -> done in memset 0
      switch (pool_index) {
      case 0:
        memory->mem_blocks[mb_index + index].data = &(memory->mem_pool0[index][0]);
        break;

      case 1:
        memory->mem_blocks[mb_index + index].data = &(memory->mem_pool1[index][0]);
        break;

      case 2:
        memory->mem_blocks[mb_index + index].data = &(memory->mem_pool2[index][0]);
        break;

      case 3:
        memory->mem_blocks[mb_index + index].data = &(memory->mem_pool3[index][0]);
        break;

      case 4:
        memory->mem_blocks[mb_index + index].data = &(memory->mem_pool4[index][0]);
        break;

      case 5:
        memory->mem_blocks[mb_index + index].data = &(memory->mem_pool5[index][0]);
        break;

      case 6:
        memory->mem_blocks[mb_index + index].data = &(memory->mem_pool6[index][0]);
        break;

      case 7:
        memory->mem_blocks[mb_index + index].data = &(memory->mem_pool7[index][0]);
        break;

      case 8:
        memory->mem_blocks[mb_index + index].data = &(memory->mem_pool8[index][0]);
        break;

      case 9:
        memory->mem_blocks[mb_index + index].data = &(memory->mem_pool9[index][0]);
        break;

      default:
        ;
        memory->mem_blocks[mb_index + index].data = NULL;   // pool copy

      }

      memory->mem_blocks[mb_index + index].pool_id = pool_index;
      add_tail (&memory->mem_blocks[mb_index + index], &memory->mem_lists[pool_index]);
    }

    mb_index += pool_sizes[pool_index];
  }

  return 0;
}

//-----------------------------------------------------------------------------
void           *
pool_buffer_clean (void *arg)
{
  //-----------------------------------------------------------------------------
#ifndef NO_THREAD_SAFE
  mem_pool       *memory = (mem_pool *) arg;
  pthread_mutex_destroy (&memory->mem_lists[MEM_MNGT_POOL_ID0].mutex);
  pthread_mutex_destroy (&memory->mem_lists[MEM_MNGT_POOL_ID1].mutex);
  pthread_mutex_destroy (&memory->mem_lists[MEM_MNGT_POOL_ID2].mutex);
  pthread_mutex_destroy (&memory->mem_lists[MEM_MNGT_POOL_ID3].mutex);
  pthread_mutex_destroy (&memory->mem_lists[MEM_MNGT_POOL_ID4].mutex);
  pthread_mutex_destroy (&memory->mem_lists[MEM_MNGT_POOL_ID5].mutex);
  pthread_mutex_destroy (&memory->mem_lists[MEM_MNGT_POOL_ID6].mutex);
  pthread_mutex_destroy (&memory->mem_lists[MEM_MNGT_POOL_ID7].mutex);
  pthread_mutex_destroy (&memory->mem_lists[MEM_MNGT_POOL_ID8].mutex);
  pthread_mutex_destroy (&memory->mem_lists[MEM_MNGT_POOL_ID9].mutex);
  pthread_mutex_destroy (&memory->mem_lists[MEM_MNGT_POOL_ID_COPY].mutex);
#endif
  return 0;
}
//-----------------------------------------------------------------------------
void
free_mem_block (mem_block_t * leP, __func__)
{
  //-----------------------------------------------------------------------------

  if (!(leP)) {
    msg ("[MEM_MNGT][FREE] WARNING FREE NULL MEM_BLOCK\n");
    return;
  }

#ifdef DEBUG_MEM_MNGT_FREE
  msg ("[MEM_MNGT][FREE] free_mem_block() %p pool: %d\n", leP, leP->pool_id, __func__);
#endif
#ifdef DEBUG_MEM_MNGT_ALLOC
  check_free_mem_block (leP, __func__);
#endif

  if (leP->pool_id <= MEM_MNGT_POOL_ID_COPY) {
    add_tail (leP, &mem->mem_lists[leP->pool_id]);
#ifdef DEBUG_MEM_MNGT_ALLOC
    counters[leP->pool_id] -= 1;
#endif
    leP = NULL;                 // this prevent from freeing the block twice
  } else {
    msg ("[MEM_MNGT][FREE] ERROR free_mem_block() unknown pool_id : %d\n", leP->pool_id, __func__);
  }
}


//-----------------------------------------------------------------------------
mem_block_t      *
get_free_mem_block (uint16_t sizeP, __func__)
{
  //-----------------------------------------------------------------------------
  mem_block_t      *le = NULL;
  int             pool_selected;
  int             size;

  if (sizeP > MEM_MNGT_MB9_BLOCK_SIZE) {
    msg ("[MEM_MNGT][ERROR][FATAL] size requested  %d out of bounds %d\n",sizeP,MEM_MNGT_MB9_BLOCK_SIZE);
    wcdma_handle_error (WCDMA_ERROR_OUT_OF_MEM_BLOCK);
    return NULL;
  }

  size = sizeP >> 6;
  pool_selected = 0;

  while ((size)) {
    pool_selected += 1;
    size = size >> 1;
  }

  // pool is selected according to the size requested, now get a block
  // if no block is available pick one in an other pool
  do {
    if ((le = remove_head (&mem->mem_lists[pool_selected]))) {
#ifdef DEBUG_MEM_MNGT_ALLOC
      counters[pool_selected] += 1;
#endif
#ifdef DEBUG_MEM_MNGT_ALLOC_SIZE
      msg ("[MEM_MNGT][INFO] ALLOC MEM_BLOCK SIZE %d bytes pool %d\n", sizeP, pool_selected);
#endif
      return le;
    }

#ifdef DEBUG_MEM_MNGT_ALLOC
    msg ("[MEM_MNGT][ERROR][MINOR] memory pool %d is empty trying next pool alloc count = %d\n", pool_selected, counters[pool_selected]);
    display_mem_load ();
    check_mem_area (mem);
#endif
  } while (pool_selected++ < 9);

  msg ("[MEM_MNGT][ERROR][FATAL] size %d requested out of bounds or memory pools empty\n", sizeP);
  wcdma_handle_error (WCDMA_ERROR_OUT_OF_MEM_BLOCK);
  return NULL;
};

//-----------------------------------------------------------------------------
mem_block_t      *
get_free_copy_mem_block (void)
{
  //-----------------------------------------------------------------------------
  mem_block_t      *le;

  if ((le = remove_head (&mem->mem_lists[MEM_MNGT_POOL_ID_COPY]))) {
    return le;
  } else {
    msg ("[MEM_MNGT][ERROR] POOL COPY IS EMPTY\n");
    display_mem_load ();
#ifdef DEBUG_MEM_MNGT_ALLOC
    check_mem_area (mem);
    break_point ();
#endif
    wcdma_handle_error (WCDMA_ERROR_OUT_OF_MEM_BLOCK);

    return NULL;
  }
}

//-----------------------------------------------------------------------------
mem_block_t      *
copy_mem_block (mem_block_t * leP, mem_block_t * destP)
{
  //-----------------------------------------------------------------------------

  if ((destP != NULL) && (leP != NULL) && (destP->pool_id == MEM_MNGT_POOL_ID_COPY)) {
    destP->data = leP->data;
  } else {
    msg ("[MEM_MNGT][COPY] copy_mem_block() pool dest src or dest is NULL\n");
  }

  return destP;
}

//-----------------------------------------------------------------------------
void
display_mem_load (void)
{
  //-----------------------------------------------------------------------------

  msg ("POOL 0 (%d elements of %d Bytes): \n", MEM_MNGT_MB0_NB_BLOCKS, MEM_MNGT_MB0_BLOCK_SIZE);
  display_list (&mem->mem_lists[MEM_MNGT_POOL_ID0]);
  msg ("POOL 1 (%d elements of %d Bytes): \n", MEM_MNGT_MB1_NB_BLOCKS, MEM_MNGT_MB1_BLOCK_SIZE);
  display_list (&mem->mem_lists[MEM_MNGT_POOL_ID1]);
  msg ("POOL 2 (%d elements of %d Bytes): \n", MEM_MNGT_MB2_NB_BLOCKS, MEM_MNGT_MB2_BLOCK_SIZE);
  display_list (&mem->mem_lists[MEM_MNGT_POOL_ID2]);
  msg ("POOL 3 (%d elements of %d Bytes): \n", MEM_MNGT_MB3_NB_BLOCKS, MEM_MNGT_MB3_BLOCK_SIZE);
  display_list (&mem->mem_lists[MEM_MNGT_POOL_ID3]);
  msg ("POOL 4 (%d elements of %d Bytes): \n", MEM_MNGT_MB4_NB_BLOCKS, MEM_MNGT_MB4_BLOCK_SIZE);
  display_list (&mem->mem_lists[MEM_MNGT_POOL_ID4]);
  msg ("POOL 5 (%d elements of %d Bytes): \n", MEM_MNGT_MB5_NB_BLOCKS, MEM_MNGT_MB5_BLOCK_SIZE);
  display_list (&mem->mem_lists[MEM_MNGT_POOL_ID5]);
  msg ("POOL 6 (%d elements of %d Bytes): \n", MEM_MNGT_MB6_NB_BLOCKS, MEM_MNGT_MB6_BLOCK_SIZE);
  display_list (&mem->mem_lists[MEM_MNGT_POOL_ID6]);
  msg ("POOL 7 (%d elements of %d Bytes): \n", MEM_MNGT_MB7_NB_BLOCKS, MEM_MNGT_MB7_BLOCK_SIZE);
  display_list (&mem->mem_lists[MEM_MNGT_POOL_ID7]);
  msg ("POOL 8 (%d elements of %d Bytes): \n", MEM_MNGT_MB8_NB_BLOCKS, MEM_MNGT_MB8_BLOCK_SIZE);
  display_list (&mem->mem_lists[MEM_MNGT_POOL_ID8]);
  msg ("POOL 9 (%d elements of %d Bytes): \n", MEM_MNGT_MB9_NB_BLOCKS, MEM_MNGT_MB9_BLOCK_SIZE);
  display_list (&mem->mem_lists[MEM_MNGT_POOL_ID9]);
  msg ("POOL C (%d elements): \n", MEM_MNGT_MBCOPY_NB_BLOCKS);
  display_list (&mem->mem_lists[MEM_MNGT_POOL_ID_COPY]);
}

//-----------------------------------------------------------------------------
void
check_mem_area (void *arg)
{
  //-----------------------------------------------------------------------------
  int             index, mb_index;
  mem_pool       *memory = (mem_pool *) arg;

  for (index = 0; index < MEM_MNGT_MB0_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[index].data != &(memory->mem_pool0[index][0])) && (memory->mem_blocks[index].pool_id != MEM_MNGT_POOL_ID0)) {
      msg ("[MEM] ERROR POOL0 block index %d\n", index);
      break_point ();
    }
  }

  mb_index = MEM_MNGT_MB0_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB1_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != &(memory->mem_pool1[index][0])) && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID1)) {
      msg ("[MEM] ERROR POOL1 block index %d\n", index);
      break_point ();
    }
  }

  mb_index += MEM_MNGT_MB1_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB2_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != &(memory->mem_pool2[index][0])) && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID2)) {
      msg ("[MEM] ERROR POOL2 block index %d\n", index);
      break_point ();
    }
  }

  mb_index += MEM_MNGT_MB2_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB3_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != &(memory->mem_pool3[index][0])) && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID3)) {
      msg ("[MEM] ERROR POOL3 block index %d\n", index);
      break_point ();
    }
  }

  mb_index += MEM_MNGT_MB3_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB4_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != &(memory->mem_pool4[index][0])) && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID4)) {
      msg ("[MEM] ERROR POOL4 block index %d\n", index);
      break_point ();
    }
  }

  mb_index += MEM_MNGT_MB4_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB5_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != &(memory->mem_pool5[index][0])) && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID5)) {
      msg ("[MEM] ERROR POOL5 block index %d\n", index);
      break_point ();
    }
  }

  mb_index += MEM_MNGT_MB5_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB6_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != &(memory->mem_pool6[index][0])) && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID6)) {
      msg ("[MEM] ERROR POOL6 block index %d\n", index);
      break_point ();
    }
  }

  mb_index += MEM_MNGT_MB6_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB7_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != &(memory->mem_pool7[index][0])) && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID7)) {
      msg ("[MEM] ERROR POOL7 block index %d\n", index);
      break_point ();
    }
  }

  mb_index += MEM_MNGT_MB7_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB8_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != &(memory->mem_pool8[index][0])) && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID8)) {
      msg ("[MEM] ERROR POOL8 block index %d\n", index);
      break_point ();
    }
  }

  mb_index += MEM_MNGT_MB8_NB_BLOCKS;

  for (index = 0; index < MEM_MNGT_MB9_NB_BLOCKS; index++) {
    if ((memory->mem_blocks[mb_index + index].data != &(memory->mem_pool9[index][0])) && (memory->mem_blocks[mb_index + index].pool_id != MEM_MNGT_POOL_ID9)) {
      msg ("[MEM] ERROR POOL9 block index %d\n", index);
      break_point ();
    }
  }

  mb_index += MEM_MNGT_MB9_NB_BLOCKS;

  for (index = mb_index; index < MEM_MNGT_NB_ELEMENTS; index++) {
    if ((memory->mem_blocks[index].data != NULL) && (memory->mem_blocks[index].pool_id != MEM_MNGT_POOL_ID_COPY)) {
      msg ("[MEM] ERROR POOL COPY block index %d\n", index);
      break_point ();
    }
  }
}

//-----------------------------------------------------------------------------
void
check_free_mem_block (mem_block_t * leP, __func__)
{
  //-----------------------------------------------------------------------------
  int             block_index;

  if ((leP >= &mem->mem_blocks[0]) && (leP <= &mem->mem_blocks[MEM_MNGT_NB_ELEMENTS])) {
    block_index = ((uint32_t) leP - (uint32_t) (&mem->mem_blocks[0])) / sizeof (mem_block_t);

    if (block_index < MEM_MNGT_MB0_NB_BLOCKS) {
      if (((uint32_t) (leP->data) != (uint32_t) (&(mem->mem_pool0[block_index][0]))) && (leP->pool_id != MEM_MNGT_POOL_ID0)) {
        msg ("[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }
    } else if (block_index < (MEM_MNGT_MB0_NB_BLOCKS + MEM_MNGT_MB1_NB_BLOCKS)) {
      if ((leP->data != &(mem->mem_pool1[block_index][0])) && (leP->pool_id != MEM_MNGT_POOL_ID1)) {
        msg ("[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }
    } else if (block_index < MEM_MNGT_MB0_NB_BLOCKS + MEM_MNGT_MB1_NB_BLOCKS + MEM_MNGT_MB2_NB_BLOCKS) {
      if ((leP->data != &(mem->mem_pool2[block_index][0])) && (leP->pool_id != MEM_MNGT_POOL_ID2)) {
        msg ("[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }
    } else if (block_index < MEM_MNGT_MB0_NB_BLOCKS + MEM_MNGT_MB1_NB_BLOCKS + MEM_MNGT_MB2_NB_BLOCKS + MEM_MNGT_MB3_NB_BLOCKS) {
      if ((leP->data != &(mem->mem_pool3[block_index][0])) && (leP->pool_id != MEM_MNGT_POOL_ID3)) {
        msg ("[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }
    } else if (block_index < MEM_MNGT_MB0_NB_BLOCKS + MEM_MNGT_MB1_NB_BLOCKS + MEM_MNGT_MB2_NB_BLOCKS + MEM_MNGT_MB3_NB_BLOCKS + MEM_MNGT_MB4_NB_BLOCKS) {
      if ((leP->data != &(mem->mem_pool4[block_index][0])) && (leP->pool_id != MEM_MNGT_POOL_ID4)) {
        msg ("[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }
    } else if (block_index < MEM_MNGT_MB0_NB_BLOCKS + MEM_MNGT_MB1_NB_BLOCKS + MEM_MNGT_MB2_NB_BLOCKS + MEM_MNGT_MB3_NB_BLOCKS + MEM_MNGT_MB4_NB_BLOCKS +
               MEM_MNGT_MB5_NB_BLOCKS) {
      if ((leP->data != &(mem->mem_pool5[block_index][0])) && (leP->pool_id != MEM_MNGT_POOL_ID5)) {
        msg ("[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }
    } else if (block_index <
               MEM_MNGT_MB0_NB_BLOCKS + MEM_MNGT_MB1_NB_BLOCKS + MEM_MNGT_MB2_NB_BLOCKS + MEM_MNGT_MB3_NB_BLOCKS + MEM_MNGT_MB4_NB_BLOCKS + MEM_MNGT_MB5_NB_BLOCKS +
               MEM_MNGT_MB6_NB_BLOCKS) {
      if ((leP->data != &(mem->mem_pool6[block_index][0])) && (leP->pool_id != MEM_MNGT_POOL_ID6)) {
        msg ("[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }
    } else if (block_index <
               MEM_MNGT_MB0_NB_BLOCKS + MEM_MNGT_MB1_NB_BLOCKS + MEM_MNGT_MB2_NB_BLOCKS + MEM_MNGT_MB3_NB_BLOCKS + MEM_MNGT_MB4_NB_BLOCKS + MEM_MNGT_MB5_NB_BLOCKS +
               MEM_MNGT_MB6_NB_BLOCKS +
               MEM_MNGT_MB7_NB_BLOCKS) {
      if ((leP->data != &(mem->mem_pool7[block_index][0])) && (leP->pool_id != MEM_MNGT_POOL_ID7)) {
        msg ("[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }
    } else if (block_index <
               MEM_MNGT_MB0_NB_BLOCKS + MEM_MNGT_MB1_NB_BLOCKS + MEM_MNGT_MB2_NB_BLOCKS + MEM_MNGT_MB3_NB_BLOCKS + MEM_MNGT_MB4_NB_BLOCKS + MEM_MNGT_MB5_NB_BLOCKS +
               MEM_MNGT_MB6_NB_BLOCKS +
               MEM_MNGT_MB7_NB_BLOCKS + MEM_MNGT_MB8_NB_BLOCKS) {
      if ((leP->data != &(mem->mem_pool8[block_index][0])) && (leP->pool_id != MEM_MNGT_POOL_ID8)) {
        msg ("[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }
    } else if (block_index <
               MEM_MNGT_MB0_NB_BLOCKS + MEM_MNGT_MB1_NB_BLOCKS + MEM_MNGT_MB2_NB_BLOCKS + MEM_MNGT_MB3_NB_BLOCKS + MEM_MNGT_MB4_NB_BLOCKS + MEM_MNGT_MB5_NB_BLOCKS +
               MEM_MNGT_MB6_NB_BLOCKS +
               MEM_MNGT_MB7_NB_BLOCKS + MEM_MNGT_MB8_NB_BLOCKS + MEM_MNGT_MB9_NB_BLOCKS) {
      if ((leP->data != &(mem->mem_pool9[block_index][0])) && (leP->pool_id != MEM_MNGT_POOL_ID9)) {
        msg ("[MEM][ERROR][FATAL] free mem block is corrupted\n");
      }
    }

  } else {
    msg ("[MEM][ERROR][FATAL] free mem block is corrupted\n");
  }
}

//-----------------------------------------------------------------------------
void
break_point (void)
{
  //-----------------------------------------------------------------------------
  int             break_var;

  msg ("[BREAK_POINT]\n");
  break_var = 1;
}
