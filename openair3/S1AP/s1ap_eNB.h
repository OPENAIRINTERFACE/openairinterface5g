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

#include <stdio.h>
#include <stdint.h>

/** @defgroup _s1ap_impl_ S1AP Layer Reference Implementation for eNB
 * @ingroup _ref_implementation_
 * @{
 */

#ifndef S1AP_ENB_H_
#define S1AP_ENB_H_

typedef struct s1ap_eNB_config_s {
  // MME related params
  unsigned char mme_enabled;          ///< MME enabled ?
} s1ap_eNB_config_t;

#if defined(OAI_EMU)
# define EPC_MODE_ENABLED       oai_emulation.info.s1ap_config.mme_enabled
#else
extern s1ap_eNB_config_t s1ap_config;

# define EPC_MODE_ENABLED       s1ap_config.mme_enabled
#endif

void *s1ap_eNB_task(void *arg);

uint32_t s1ap_generate_eNB_id(void);

#endif /* S1AP_ENB_H_ */

/**
 * @}
 */
