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

/*! \file flexran_dci_conversions.h
 * \brief Conversion helpers from flexran messages to OAI formats DCI  
 * \author Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#ifndef LAYER2_MAC_FLEXRAN_DCI_CONVERISIONS_H__
#define LAYER2_MAC_DCI_FLEXRAN_CONVERISIONS_H__

#define FILL_DCI_FDD_1(TYPE, DCI, FLEXRAN_DCI) \
  ((TYPE*)DCI)->harq_pid = FLEXRAN_DCI->harq_process; \
  ((TYPE*)DCI)->rv = FLEXRAN_DCI->rv[0]; \
  ((TYPE*)DCI)->rballoc = FLEXRAN_DCI->rb_bitmap; \
  ((TYPE*)DCI)->rah = FLEXRAN_DCI->res_alloc; \
  ((TYPE*)DCI)->mcs = FLEXRAN_DCI->mcs[0]; \
  ((TYPE*)DCI)->TPC = FLEXRAN_DCI->tpc; \
  ((TYPE*)DCI)->ndi = FLEXRAN_DCI->ndi[0];

#define FILL_DCI_TDD_1(TYPE, DCI, FLEXRAN_DCI) \
  ((TYPE*)DCI)->harq_pid = FLEXRAN_DCI->harq_process; \
  ((TYPE*)DCI)->rv = FLEXRAN_DCI->rv[0]; \
  ((TYPE*)DCI)->dai = FLEXRAN_DCI->dai; \
  ((TYPE*)DCI)->rballoc = FLEXRAN_DCI->rb_bitmap; \
  ((TYPE*)DCI)->rah = FLEXRAN_DCI->res_alloc; \
  ((TYPE*)DCI)->mcs = FLEXRAN_DCI->mcs[0]; \
  ((TYPE*)DCI)->TPC = FLEXRAN_DCI->tpc; \
  ((TYPE*)DCI)->ndi = FLEXRAN_DCI->ndi[0];

#endif
