/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef __MODULATION_COMMON__H__
#define __MODULATION_COMMON__H__
#include "PHY/defs_common.h"
/** @addtogroup _PHY_MODULATION_
 * @{
*/
void normal_prefix_mod(int32_t *txdataF,int32_t *txdata,uint8_t nsymb,LTE_DL_FRAME_PARMS *frame_parms);
void do_OFDM_mod(c16_t **txdataF, c16_t **txdata, uint32_t frame,uint16_t next_slot, LTE_DL_FRAME_PARMS *frame_parms);
/** @}*/
#endif
