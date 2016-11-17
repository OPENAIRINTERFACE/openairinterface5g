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

/***************************************************************************
                          mem_block.h  -  description
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr


 ***************************************************************************/
#ifndef __MEM_BLOCK_H__
#    define __MEM_BLOCK_H__

#ifdef USER_MODE
#include <stdint.h>
#endif
#ifdef MEM_BLOCK_C
#    define public_mem_block(x) x
#    define private_mem_block(x) x
#else
#    define public_mem_block(x) extern x
#    define private_mem_block(x)
#endif
//-----------------------------------------------------------------------------

typedef struct mem_block_t {
  struct mem_block_t *next;
  struct mem_block_t *previous;
  unsigned char pool_id;
  unsigned char *data;
} mem_block_t;
#include "UTIL/LISTS/list.h"

//-----------------------------------------------------------------------------

public_mem_block(void        *pool_buffer_init (void);)
public_mem_block(void        *pool_buffer_clean (void *arg);)
public_mem_block(void         free_mem_block (mem_block_t * leP);)
public_mem_block(mem_block_t* get_free_mem_block (uint32_t sizeP);)
public_mem_block(mem_block_t *get_free_copy_mem_block (void);)
public_mem_block(mem_block_t *get_free_copy_mem_block_up (void);)
public_mem_block(mem_block_t *copy_mem_block (mem_block_t * leP, mem_block_t * destP);)
public_mem_block(void         display_mem_load (void);)

public_mem_block(void         check_mem_area (void);)
#    ifdef USER_MODE
private_mem_block(void        check_free_mem_block (mem_block_t * leP);)
#    endif
#ifdef USER_MODE
//#    define MEM_SCALE MAX_MOBILES_PER_ENB*NB_RB_MAX
#    define MEM_SCALE MAX_MOBILES_PER_ENB
#else
#    ifdef NODE_RG
#        define MEM_SCALE 2
#    else
#        define MEM_SCALE 1
#    endif
#endif
// definition of the size of the allocated memory area
#    define MEM_MNGT_MB0_BLOCK_SIZE     64
// 64
#    define MEM_MNGT_MB0_NB_BLOCKS      4096 * MEM_SCALE
#    define MEM_MNGT_POOL_ID0           0

#    define MEM_MNGT_MB1_BLOCK_SIZE     MEM_MNGT_MB0_BLOCK_SIZE*2
// 128
#    define MEM_MNGT_MB1_NB_BLOCKS      4096 * MEM_SCALE
#    define MEM_MNGT_POOL_ID1           1

#    define MEM_MNGT_MB2_BLOCK_SIZE     MEM_MNGT_MB0_BLOCK_SIZE*4
// 256
#    define MEM_MNGT_MB2_NB_BLOCKS      2048 * MEM_SCALE
#    define MEM_MNGT_POOL_ID2           2

#    define MEM_MNGT_MB3_BLOCK_SIZE     MEM_MNGT_MB0_BLOCK_SIZE*8
// 512
#    define MEM_MNGT_MB3_NB_BLOCKS      2048 * MEM_SCALE
#    define MEM_MNGT_POOL_ID3           3

#    define MEM_MNGT_MB4_BLOCK_SIZE     MEM_MNGT_MB0_BLOCK_SIZE*16
// 1024
#    define MEM_MNGT_MB4_NB_BLOCKS      1024 * MEM_SCALE
#    define MEM_MNGT_POOL_ID4           4

#    define MEM_MNGT_MB5_BLOCK_SIZE     MEM_MNGT_MB0_BLOCK_SIZE*32
// 2048
#    define MEM_MNGT_MB5_NB_BLOCKS      1024 * MEM_SCALE // LG WAS 1024
#    define MEM_MNGT_POOL_ID5           5

#    define MEM_MNGT_MB6_BLOCK_SIZE     MEM_MNGT_MB0_BLOCK_SIZE*64
// 4096
#    define MEM_MNGT_MB6_NB_BLOCKS      1024 * MEM_SCALE  // LG WAS 256
#    define MEM_MNGT_POOL_ID6           6

#    define MEM_MNGT_MB7_BLOCK_SIZE     MEM_MNGT_MB0_BLOCK_SIZE*128
// 8192
#    define MEM_MNGT_MB7_NB_BLOCKS      64* MEM_SCALE   // LG WAS 32
#    define MEM_MNGT_POOL_ID7           7

#    define MEM_MNGT_MB8_BLOCK_SIZE     MEM_MNGT_MB0_BLOCK_SIZE*256

#ifdef JUMBO_FRAMES
#    define MEM_MNGT_MB8_NB_BLOCKS      256 * MEM_SCALE
#else
#    define MEM_MNGT_MB8_NB_BLOCKS      16 * MEM_SCALE
// 16384
#endif
#    define MEM_MNGT_POOL_ID8           8

#    define MEM_MNGT_MB9_BLOCK_SIZE     MEM_MNGT_MB0_BLOCK_SIZE*512
// 32768
#    define MEM_MNGT_MB9_NB_BLOCKS      8 * MEM_SCALE
#    define MEM_MNGT_POOL_ID9           9

#    define MEM_MNGT_MB10_BLOCK_SIZE    MEM_MNGT_MB0_BLOCK_SIZE*1024
// 65536
#    define MEM_MNGT_MB10_NB_BLOCKS     0 * MEM_SCALE
#    define MEM_MNGT_POOL_ID10          10

#    define MEM_MNGT_MB11_BLOCK_SIZE    MEM_MNGT_MB0_BLOCK_SIZE*2048
// 131072
#    define MEM_MNGT_MB11_NB_BLOCKS     0 * MEM_SCALE
#    define MEM_MNGT_POOL_ID11          11

#    define MEM_MNGT_MB12_BLOCK_SIZE    MEM_MNGT_MB0_BLOCK_SIZE*4096
// 262144
#    define MEM_MNGT_MB12_NB_BLOCKS     32 * MEM_SCALE
#    define MEM_MNGT_POOL_ID12          12


#    define MEM_MNGT_MBCOPY_NB_BLOCKS   1024
#    define MEM_MNGT_NB_ELEMENTS        MEM_MNGT_MB0_NB_BLOCKS + MEM_MNGT_MB1_NB_BLOCKS + MEM_MNGT_MB2_NB_BLOCKS + MEM_MNGT_MB3_NB_BLOCKS + MEM_MNGT_MB4_NB_BLOCKS + MEM_MNGT_MB5_NB_BLOCKS + MEM_MNGT_MB6_NB_BLOCKS + MEM_MNGT_MB7_NB_BLOCKS + MEM_MNGT_MB8_NB_BLOCKS + MEM_MNGT_MB9_NB_BLOCKS + MEM_MNGT_MB10_NB_BLOCKS + MEM_MNGT_MB11_NB_BLOCKS + MEM_MNGT_MB12_NB_BLOCKS + MEM_MNGT_MBCOPY_NB_BLOCKS
#    define MEM_MNGT_POOL_ID_COPY        13



private_mem_block(typedef struct {
  //-----------------------------------------------------------
  // basic memory management
  //-----------------------------------------------------------
  char              mem_pool0[MEM_MNGT_MB0_NB_BLOCKS][MEM_MNGT_MB0_BLOCK_SIZE];
  char              mem_pool1[MEM_MNGT_MB1_NB_BLOCKS][MEM_MNGT_MB1_BLOCK_SIZE];
  char              mem_pool2[MEM_MNGT_MB2_NB_BLOCKS][MEM_MNGT_MB2_BLOCK_SIZE];
  char              mem_pool3[MEM_MNGT_MB3_NB_BLOCKS][MEM_MNGT_MB3_BLOCK_SIZE];
  char              mem_pool4[MEM_MNGT_MB4_NB_BLOCKS][MEM_MNGT_MB4_BLOCK_SIZE];
  char              mem_pool5[MEM_MNGT_MB5_NB_BLOCKS][MEM_MNGT_MB5_BLOCK_SIZE];
  char              mem_pool6[MEM_MNGT_MB6_NB_BLOCKS][MEM_MNGT_MB6_BLOCK_SIZE];
  char              mem_pool7[MEM_MNGT_MB7_NB_BLOCKS][MEM_MNGT_MB7_BLOCK_SIZE];
  char              mem_pool8[MEM_MNGT_MB8_NB_BLOCKS][MEM_MNGT_MB8_BLOCK_SIZE];
  char              mem_pool9[MEM_MNGT_MB9_NB_BLOCKS][MEM_MNGT_MB9_BLOCK_SIZE];
  char              mem_pool10[MEM_MNGT_MB10_NB_BLOCKS][MEM_MNGT_MB10_BLOCK_SIZE];
  char              mem_pool11[MEM_MNGT_MB11_NB_BLOCKS][MEM_MNGT_MB11_BLOCK_SIZE];
  char              mem_pool12[MEM_MNGT_MB12_NB_BLOCKS][MEM_MNGT_MB12_BLOCK_SIZE];
  mem_block_t     mem_blocks[MEM_MNGT_NB_ELEMENTS];
  list_t          mem_lists[14];

} mem_pool;)

private_mem_block(mem_pool  mem_block_var;)


#endif
