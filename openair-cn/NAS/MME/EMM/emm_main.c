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
Source      emm_main.c

Version     0.1

Date        2012/10/10

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the EPS Mobility Management procedure call manager,
        the main entry point for elementary EMM processing.

*****************************************************************************/

#include "emm_main.h"
#include "nas_log.h"
#include "emmData.h"


#if defined(NAS_BUILT_IN_EPC)
# include "mme_config.h"
#endif

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
 ** Name:    emm_main_initialize()                                     **
 **                                                                        **
 ** Description: Initializes EMM internal data                             **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
#if defined(NAS_BUILT_IN_EPC)
void emm_main_initialize(mme_config_t *mme_config_p)
#else
void emm_main_initialize(void)
#endif
{
  LOG_FUNC_IN;

  /* Retreive MME supported configuration data */
#if defined(NAS_BUILT_IN_EPC)

  if (mme_api_get_emm_config(&_emm_data.conf, mme_config_p) != RETURNok)
#else
  if (mme_api_get_emm_config(&_emm_data.conf) != RETURNok)
#endif
  {
    LOG_TRACE(ERROR, "EMM-MAIN  - Failed to get MME configuration data");
  }

#if defined(NAS_BUILT_IN_EPC)
  RB_INIT(&_emm_data.ctx_map);
#endif

  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_cleanup()                                        **
 **                                                                        **
 ** Description: Performs the EPS Mobility Management clean up procedure   **
 **                                                                        **
 ** Inputs:  None                                                      **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **          Return:    None                                       **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void emm_main_cleanup(void)
{
  LOG_FUNC_IN;


  LOG_FUNC_OUT;
}



/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/


