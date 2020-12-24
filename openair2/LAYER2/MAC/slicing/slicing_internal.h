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

/*!
 * \file   slicing_internal.h
 * \brief  Internal slice helper functions
 * \author Robert Schmidt
 * \date   2020
 * \email  robert.schmidt@eurecom.fr
 */

#ifndef __SLICING_INTERNAL_H__
#define __SLICING_INTERNAL_H__

#include "slicing.h"

void slicing_add_UE(slice_info_t *si, int UE_id);

void _remove_UE(slice_t **s, uint8_t *assoc, int UE_id);
void slicing_remove_UE(slice_info_t *si, int UE_id);

void _move_UE(slice_t **s, uint8_t *assoc, int UE_id, int to);
void slicing_move_UE(slice_info_t *si, int UE_id, int idx);

slice_t *_add_slice(uint8_t *n, slice_t **s);
slice_t *_remove_slice(uint8_t *n, slice_t **s, uint8_t *assoc, int idx);

#endif /* __SLICING_INTERNAL_H__ */
