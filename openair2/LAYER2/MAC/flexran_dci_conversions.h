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

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

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
