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
Source      EmmCommon.h

Version     0.1

Date        2013/04/19

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines callback functions executed within EMM common procedures
        by the Non-Access Stratum running at the network side.

        Following EMM common procedures can always be initiated by the
        network whilst a NAS signalling connection exists:

        GUTI reallocation
        authentication
        security mode control
        identification
        EMM information

*****************************************************************************/


#include "EmmCommon.h"

#include "commonDef.h"
#include "nas_log.h"
#include "emmData.h"

#include <stdlib.h> // malloc, free
#include <string.h>
#include <assert.h>

#if defined(NAS_BUILT_IN_EPC)
# include "assertions.h"
#endif

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* EMM procedure callback cleanup function  */
static void _emm_common_cleanup(unsigned int ueid);

/* Ongoing EMM procedure callback functions */
typedef struct emm_common_data_s {
  unsigned int ueid;
  int ref_count;
  emm_common_success_callback_t success;
  emm_common_reject_callback_t  reject;
  emm_common_failure_callback_t failure;
  emm_common_abort_callback_t   abort;
  void *args;

#if defined(NAS_BUILT_IN_EPC)
  RB_ENTRY(emm_common_data_s) entries;
#endif
} emm_common_data_t;

#if defined(NAS_BUILT_IN_EPC)
typedef struct emm_common_data_head_s {
  RB_HEAD(emm_common_data_map, emm_common_data_s) emm_common_data_root;
} emm_common_data_head_t;

emm_common_data_head_t emm_common_data_head = { RB_INITIALIZER() };

static inline
int emm_common_data_compare_ueid(struct emm_common_data_s *p1,
                                 struct emm_common_data_s *p2);
struct emm_common_data_s *emm_common_data_context_get(
  struct emm_common_data_head_s *root, unsigned int _ueid);

RB_PROTOTYPE(emm_common_data_map, emm_common_data_s, entries,
             emm_common_data_compare_ueid);

/* Generate functions used for the MAP */
RB_GENERATE(emm_common_data_map, emm_common_data_s, entries,
            emm_common_data_compare_ueid);

static inline
int emm_common_data_compare_ueid(struct emm_common_data_s *p1,
                                 struct emm_common_data_s *p2)
{
  if (p1->ueid > p2->ueid) {
    return 1;
  }

  if (p1->ueid < p2->ueid) {
    return -1;
  }

  /* Matching reference -> return 0 */
  return 0;
}

struct emm_common_data_s *emm_common_data_context_get(
  struct emm_common_data_head_s *root, unsigned int _ueid)
{
  struct emm_common_data_s reference;

  DevAssert(root != NULL);
  DevCheck(_ueid > 0, _ueid, 0, 0);

  memset(&reference, 0, sizeof(struct emm_common_data_s));
  reference.ueid = _ueid;
  return RB_FIND(emm_common_data_map, &root->emm_common_data_root, &reference);
}
#else
static emm_common_data_t *_emm_common_data[EMM_DATA_NB_UE_MAX];
#endif

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_common_initialize()                              **
 **                                                                        **
 ** Description: Initialize EMM procedure callback functions executed for  **
 **      the UE with the given identifier                          **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      success:   EMM procedure executed upon successful EMM **
 **             common procedure completion                **
 **      reject:    EMM procedure executed if the EMM common   **
 **             procedure failed or is rejected            **
 **      failure:   EMM procedure executed upon transmission   **
 **             failure reported by lower layer            **
 **      abort:     EMM common procedure executed when the on- **
 **             going EMM procedure is aborted             **
 **      args:      EMM common procedure argument parameters   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_common_data                           **
 **                                                                        **
 ***************************************************************************/
int emm_proc_common_initialize(unsigned int ueid,
                               emm_common_success_callback_t _success,
                               emm_common_reject_callback_t  _reject,
                               emm_common_failure_callback_t _failure,
                               emm_common_abort_callback_t   _abort,
                               void *args)
{
  struct emm_common_data_s *emm_common_data_ctx = NULL;
  LOG_FUNC_IN;

#if defined(NAS_BUILT_IN_EPC)
  assert(ueid > 0);
  emm_common_data_ctx = emm_common_data_context_get(&emm_common_data_head, ueid);
#else
  assert(ueid < EMM_DATA_NB_UE_MAX);
#endif

  if (emm_common_data_ctx == NULL) {
    emm_common_data_ctx = (emm_common_data_t *)malloc(sizeof(emm_common_data_t));
    emm_common_data_ctx->ueid = ueid;
#if defined(NAS_BUILT_IN_EPC)
    RB_INSERT(emm_common_data_map, &emm_common_data_head.emm_common_data_root,
              emm_common_data_ctx);
#endif

    if (emm_common_data_ctx) {
      emm_common_data_ctx->ref_count = 0;
    }
  }

  if (emm_common_data_ctx) {
    emm_common_data_ctx->ref_count += 1;
    emm_common_data_ctx->success = _success;
    emm_common_data_ctx->reject  = _reject;
    emm_common_data_ctx->failure = _failure;
    emm_common_data_ctx->abort   = _abort;
    emm_common_data_ctx->args    = args;
    LOG_FUNC_RETURN(RETURNok);
  }

  LOG_FUNC_RETURN(RETURNerror);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_common_success()                                 **
 **                                                                        **
 ** Description: The EMM common procedure initiated between the UE with    **
 **      the specified identifier and the MME completed success-   **
 **      fully. The network performs required actions related to   **
 **      the ongoing EMM procedure.                                **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      Others:    _emm_common_data, _emm_data                **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_common_success(unsigned int ueid)
{
  emm_common_data_t             *emm_common_data_ctx = NULL;
  emm_common_success_callback_t  emm_callback;

  int rc = RETURNerror;

  LOG_FUNC_IN;

#if defined(NAS_BUILT_IN_EPC)
  DevCheck(ueid > 0, ueid, 0, 0);
  emm_common_data_ctx = emm_common_data_context_get(&emm_common_data_head, ueid);
#else
  assert(ueid < EMM_DATA_NB_UE_MAX);
  emm_common_data_ctx = _emm_common_data[ueid];
#endif

  assert(emm_common_data_ctx != NULL);

  emm_callback = emm_common_data_ctx->success;

  if (emm_callback) {
    struct emm_data_context_s *ctx = NULL;

#if defined(NAS_BUILT_IN_EPC)
    ctx = emm_data_context_get(&_emm_data, ueid);
#else
    ctx = _emm_data.ctx[ueid];
#endif
    rc = (*emm_callback)(ctx);
  }

  _emm_common_cleanup(ueid);

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_common_reject()                                  **
 **                                                                        **
 ** Description: The EMM common procedure initiated between the UE with    **
 **      the specified identifier and the MME failed or has been   **
 **      rejected. The network performs required actions related   **
 **      to the ongoing EMM procedure.                             **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      Others:    _emm_common_data, _emm_data                **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_common_reject(unsigned int ueid)
{
  emm_common_data_t *emm_common_data_ctx = NULL;
  int rc = RETURNerror;
  emm_common_reject_callback_t emm_callback;

  LOG_FUNC_IN;

#if defined(NAS_BUILT_IN_EPC)
  DevCheck(ueid > 0, ueid, 0, 0);
  emm_common_data_ctx = emm_common_data_context_get(&emm_common_data_head, ueid);
#else
  assert(ueid < EMM_DATA_NB_UE_MAX);
  emm_common_data_ctx = _emm_common_data[ueid];
#endif

  assert(emm_common_data_ctx != NULL);

  emm_callback = emm_common_data_ctx->reject;

  if (emm_callback) {
    struct emm_data_context_s *ctx = NULL;

#if defined(NAS_BUILT_IN_EPC)
    ctx = emm_data_context_get(&_emm_data, ueid);
#else
    ctx = _emm_data.ctx[ueid];
#endif
    rc = (*emm_callback)(ctx);
  }

  _emm_common_cleanup(ueid);

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_common_failure()                                 **
 **                                                                        **
 ** Description: The EMM common procedure has been initiated between the   **
 **      UE with the specified identifier and the MME, and a lower **
 **      layer failure occurred before the EMM common procedure    **
 **      being completed. The network performs required actions    **
 **      related to the ongoing EMM procedure.                     **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      Others:    _emm_common_data, _emm_data                **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_common_failure(unsigned int ueid)
{
  emm_common_data_t             *emm_common_data_ctx = NULL;
  emm_common_failure_callback_t  emm_callback;

  int rc = RETURNerror;

  LOG_FUNC_IN;

#if defined(NAS_BUILT_IN_EPC)
  DevCheck(ueid > 0, ueid, 0, 0);
  emm_common_data_ctx = emm_common_data_context_get(&emm_common_data_head, ueid);
#else
  assert(ueid < EMM_DATA_NB_UE_MAX);
  emm_common_data_ctx = _emm_common_data[ueid];
#endif
  assert(emm_common_data_ctx != NULL);

  emm_callback = emm_common_data_ctx->failure;

  if (emm_callback) {
    struct emm_data_context_s *ctx = NULL;

#if defined(NAS_BUILT_IN_EPC)
    ctx = emm_data_context_get(&_emm_data, ueid);
#else
    ctx = _emm_data.ctx[ueid];
#endif
    rc = (*emm_callback)(ctx);
  }

  _emm_common_cleanup(ueid);

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_common_abort()                                   **
 **                                                                        **
 ** Description: The ongoing EMM procedure has been aborted. The network   **
 **      performs required actions related to the EMM common pro-  **
 **      cedure previously initiated between the UE with the spe-  **
 **      cified identifier and the MME.                            **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      Others:    _emm_common_data                           **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_common_abort(unsigned int ueid)
{
  emm_common_data_t             *emm_common_data_ctx = NULL;
  emm_common_failure_callback_t  emm_callback;

  int rc = RETURNerror;

  LOG_FUNC_IN;

#if defined(NAS_BUILT_IN_EPC)
  DevCheck(ueid > 0, ueid, 0, 0);
  emm_common_data_ctx = emm_common_data_context_get(&emm_common_data_head, ueid);
#else
  assert(ueid < EMM_DATA_NB_UE_MAX);
  emm_common_data_ctx = _emm_common_data[ueid];
#endif
  assert(emm_common_data_ctx != NULL);

  emm_callback = emm_common_data_ctx->abort;

  if (emm_callback) {
    struct emm_data_context_s *ctx = NULL;

#if defined(NAS_BUILT_IN_EPC)
    ctx = emm_data_context_get(&_emm_data, ueid);
#else
    ctx = _emm_data.ctx[ueid];
#endif
    rc = (*emm_callback)(ctx);
  }

  _emm_common_cleanup(ueid);

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_common_get_args()                                **
 **                                                                        **
 ** Description: Returns pointer to the EMM common procedure argument pa-  **
 **      rameters allocated for the UE with the given identifier.  **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      Others:    _emm_common_data                           **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    pointer to the EMM common procedure argu-  **
 **             ment parameters                            **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void *emm_proc_common_get_args(unsigned int ueid)
{
  emm_common_data_t *emm_common_data_ctx = NULL;
  LOG_FUNC_IN;

#if defined(NAS_BUILT_IN_EPC)
  DevCheck(ueid > 0, ueid, 0, 0);
  emm_common_data_ctx = emm_common_data_context_get(&emm_common_data_head, ueid);
#else
  assert(ueid < EMM_DATA_NB_UE_MAX);
  emm_common_data_ctx = _emm_common_data[ueid];
#endif
  assert(emm_common_data_ctx != NULL);

  LOG_FUNC_RETURN(emm_common_data_ctx->args);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_common_cleanup()                                 **
 **                                                                        **
 ** Description: Cleans EMM procedure callback functions upon completion   **
 **      of an EMM common procedure previously initiated within an **
 **      EMM procedure currently in progress between the network   **
 **      and the UE with the specified identifier.                 **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _emm_common_data                           **
 **                                                                        **
 ***************************************************************************/
static void _emm_common_cleanup(unsigned int ueid)
{
  emm_common_data_t *emm_common_data_ctx = NULL;

#if defined(NAS_BUILT_IN_EPC)
  DevCheck(ueid > 0, ueid, 0, 0);
  emm_common_data_ctx = emm_common_data_context_get(&emm_common_data_head, ueid);
#else
  assert(ueid < EMM_DATA_NB_UE_MAX);
  emm_common_data_ctx = _emm_common_data[ueid];
#endif

  if (emm_common_data_ctx) {
    emm_common_data_ctx->ref_count -= 1;

    if (emm_common_data_ctx->ref_count == 0) {
      /* Release the callback functions */
#if defined(NAS_BUILT_IN_EPC)
      RB_REMOVE(emm_common_data_map,
                &emm_common_data_head.emm_common_data_root,
                emm_common_data_ctx);
#endif
      free(emm_common_data_ctx);
      emm_common_data_ctx = NULL;
    }
  }
}
