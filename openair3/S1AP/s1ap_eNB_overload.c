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

/*! \file s1ap_eNB_overload.c
 * \brief s1ap procedures for overload messages within eNB
 * \author Sebastien ROUX <sebastien.roux@eurecom.fr>
 * \date 2012
 * \version 0.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "intertask_interface.h"

#include "s1ap_common.h"
#include "s1ap_ies_defs.h"
#include "s1ap_eNB_defs.h"

#include "s1ap_eNB.h"
#include "s1ap_eNB_ue_context.h"
#include "s1ap_eNB_encoder.h"
#include "s1ap_eNB_overload.h"
#include "s1ap_eNB_management_procedures.h"

#include "assertions.h"

int s1ap_eNB_handle_overload_start(uint32_t               assoc_id,
                                   uint32_t               stream,
                                   struct s1ap_message_s *message_p)
{
  S1ap_OverloadStartIEs_t *overload_start_p;
  s1ap_eNB_mme_data_t     *mme_desc_p;

  DevAssert(message_p != NULL);

  overload_start_p = &message_p->msg.s1ap_OverloadStartIEs;

  DevCheck(overload_start_p->overloadResponse.present ==
           S1ap_OverloadResponse_PR_overloadAction,
           S1ap_OverloadResponse_PR_overloadAction, 0, 0);

  /* Non UE-associated signalling -> stream 0 */
  DevCheck(stream == 0, stream, 0, 0);

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    /* No MME context associated */
    return -1;
  }

  /* Mark the MME as overloaded and set the overload state according to
   * the value received.
   */
  mme_desc_p->state = S1AP_ENB_OVERLOAD;
  mme_desc_p->overload_state =
    overload_start_p->overloadResponse.choice.overloadAction;

  return 0;
}

int s1ap_eNB_handle_overload_stop(uint32_t               assoc_id,
                                  uint32_t               stream,
                                  struct s1ap_message_s *message_p)
{
  /* We received Overload stop message, meaning that the MME is no more
   * overloaded. This is an empty message, with only message header and no
   * Information Element.
   */

  DevAssert(message_p != NULL);

  s1ap_eNB_mme_data_t *mme_desc_p;

  /* Non UE-associated signalling -> stream 0 */
  DevCheck(stream == 0, stream, 0, 0);

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    /* No MME context associated */
    return -1;
  }

  mme_desc_p->state = S1AP_ENB_STATE_CONNECTED;
  mme_desc_p->overload_state = S1AP_NO_OVERLOAD;
  return 0;
}
