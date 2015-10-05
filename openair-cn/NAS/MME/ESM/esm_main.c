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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*****************************************************************************
Source      esm_main.c

Version     0.1

Date        2012/12/04

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines the EPS Session Management procedure call manager,
        the main entry point for elementary ESM processing.

*****************************************************************************/

#include "esm_main.h"
#include "commonDef.h"
#include "nas_log.h"

#include "emmData.h"
#include "esmData.h"
#include "esm_pt.h"
#include "esm_ebr.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/


/****************************************************************************
 **                                                                        **
 ** Name:    esm_main_initialize()                                     **
 **                                                                        **
 ** Description: Initializes ESM internal data                             **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void esm_main_initialize(void)
{
  int i;

  LOG_FUNC_IN;

  /* Retreive MME supported configuration data */
  if (mme_api_get_esm_config(&_esm_data.conf) != RETURNok) {
    LOG_TRACE(ERROR, "ESM-MAIN  - Failed to get MME configuration data");
  }

# if !defined(NAS_BUILT_IN_EPC)

  /* Initialize ESM contexts */
  for (i = 0; i < ESM_DATA_NB_UE_MAX; i++) {
    _esm_data.ctx[i] = NULL;
  }

# endif

  /* Initialize the EPS bearer context manager */
  esm_ebr_initialize();

  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:        esm_main_cleanup()                                        **
 **                                                                        **
 ** Description: Performs the EPS Session Management clean up procedure    **
 **                                                                        **
 ** Inputs:      None                                                      **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    None                                       **
 **                  Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void esm_main_cleanup(void)
{
  LOG_FUNC_IN;

  LOG_FUNC_OUT;
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

